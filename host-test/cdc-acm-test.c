#include <pthread.h>
#include <arch.h>
#include <bits.h>
#include <usb-loopback.h>
#include <assert.h>
#include "../usb.h"
#include "../usb-internal.h"
#include "../usb-cdc-acm.h"
#include "../idle.h"

struct usb_dev usbdev;
struct acm_data acmdata;

static void
do_emulator_irq(void)
{
	usb_emu_irq(&usbdev);
}

USBSERIAL_OUT(ttyout, &usbdev);
USBSERIAL_IN(ttyin, &usbdev);

#define please(expr) 		\
	do { 			\
		int __rc; 	\
		if ((__rc = (expr)) < 0) { \
			printf("%s: %s\n", #expr, strerror(-__rc)); \
			exit(1); \
		} 		 \
	} while (0)

/* device runs in this thread */
static void *
device_thread(void *ignored)
{
	char buf[64];
	long rc;

	/* simply echo input content
	 *
	 * since the buffer is 64 bytes, we should
	 * never experience a short write */
	do {
read_again:
		rc = ttyin.read(&ttyin, buf, sizeof(buf));
		if (rc == 0)
			goto read_again;
		if (rc < 0) {
			if (rc == -EAGAIN) {
				idle_step(true);
				goto read_again;
			}
			printf("ttyin.read: %s\n", strerror(-rc));
			exit(1);
		}
write_again:
		rc = ttyout.write(&ttyout, buf, rc);
		if (rc < 0) {
			if (rc == -EAGAIN) {
				idle_step(true);
				goto write_again;
			}
			printf("ttyout.write: %s\n", strerror(-rc));
			exit(1);
		}
	} while (1);
}

/* step through a configuration
 * one descriptor at a time and
 * confirm that they imply the expected
 * total configuration length */
static void
validate_config(char *buf, int expectlen)
{
	u8 dlen;
	do {
		dlen = buf[0];
		buf += dlen;
		expectlen -= dlen;
	} while (expectlen > 0);
	if (expectlen < 0) {
		printf("descriptor length mismatch: %d-byte overrun\n", -expectlen);
		exit(1);
	}
}

int main(void) {
	pthread_t dev_thread;
	char buf[128];

	usb_trace = getenv("TRACE") != NULL;

	host_dev_init(&usbdev, &usb_cdc_acm, &acmdata);

	usb_init(&usbdev);

	claim(USB_EMULATOR_IRQ, do_emulator_irq);
	irq_enable_num(USB_EMULATOR_IRQ);

	/* main is the host thread */
	pthread_create(&dev_thread, NULL, device_thread, NULL);

	please(host_reset(&usbdev));
	
	/* get device descriptor (always 18 bytes);
	 * we expect 1 configuration */
	please(host_get_descriptor(&usbdev, 1 << 8, buf, 18));
	assert(buf[0] == 18);
	assert(buf[1] == 0x01); /* Device Descriptor tag */
	assert(buf[7] == 64);   /* 64-byte control transfer size */
	assert(buf[17] == 1);   /* 1 configuration */

	/* get config 1 descriptor (header only!);
	 * this will tell us how many bytes to fetch
	 * in order to read the whole thing
	 * (this appears to be what Linux and OSX do,
	 * so let's emulate it) */
	please(host_get_descriptor(&usbdev, (2 << 8) | 0, buf, 9));
	assert(buf[0] == 9);
	assert(buf[1] == 0x02); /* configuration descriptor */
	u16 len = buf[2] | ((u16)buf[3] << 8);

	if (len > sizeof(buf)) {
		printf("configuration descriptor is %d bytes; need larger buffer\n", (int)len);
		return 1;
	}

	please(host_get_descriptor(&usbdev, (2 << 8) | 0, buf, len));
	assert(buf[0] == 9);
	assert(buf[1] == 0x02);
	assert(((u16)buf[2] | ((u16)buf[3] << 8)) == len);

	validate_config(buf, len);

	please(host_reset(&usbdev));
	please(host_set_address(&usbdev, 55));

	for (int i=0; i<5; i++) {
		snprintf(buf, sizeof(buf), "Hello, World!\n");

		please(host_out_full(&usbdev, 1, buf, strlen(buf)+1));
		memset(buf, 0, sizeof(buf));

		please(host_in_full(&usbdev, 0x81, buf, sizeof("Hello, World!\n")));
		memset(buf, 0, sizeof(buf));
	}

	/* now send the device some data we know
	 * it can't handle and confirm we don't
	 * lock up (but instead get -EIO) */
	int err;
	if ((err = host_get_descriptor(&usbdev, 0xff << 8 | 4, buf, 68)) != -EIO) {
		printf("expected EIO on bad control xfer; got %s\n", strerror(-err));
		return 1;
	}
	
	/* test that we can return to a good state */
	please(host_reset(&usbdev));
	please(host_set_address(&usbdev, 68));
	please(host_get_descriptor(&usbdev, (2 << 8)|0, buf, len));
	return 0;
}
