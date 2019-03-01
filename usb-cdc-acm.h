#ifndef __USB_CDC_ACM_H_
#define __USB_CDC_ACM_H_
#include <bits.h>
#include <usb.h>
#include <libc.h>

extern const struct usb_class usb_cdc_acm;

struct acm_data {
	uchar inbuf[64] __attribute__((aligned(4)));
	uchar outbuf[64] __attribute__((aligned(4)));

	uchar read;
	uchar inread;
	i8    instatus;
	i8    outstatus;

	bool  discard_in;
};

extern long
acm_odev_write(const struct output *odev, const char *data, ulong len);

extern long
acm_idev_read(const struct input *idev, void *dst, ulong dstlen);

#define USBSERIAL_OUT(name, usb) \
	const struct output name = { .ctx = usb, .write = acm_odev_write }

#define USBSERIAL_IN(name, usb) \
	const struct input name = { .ctx = usb, .read = acm_idev_read }


#endif
