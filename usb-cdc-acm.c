#include <bits.h>
#include <error.h>
#include <libc.h>
#include <arch.h>
#include <assert.h>
#include <usb-internal.h>
#include <usb-cdc-acm.h>

/* class-specific control commands
 * (the ones we care about) */
#define CDC_SEND_ENCAPSULATED_COMMAND 0x00
#define CDC_GET_ENCAPSULATED_RESPONSE 0x01
#define CDC_REQ_SET_LINE_CODING       0x20
#define CDC_REQ_GET_LINE_CODING       0x21
#define CDC_REQ_SET_CONTROL_LINE_STATE    0x22
#define CDC_REQ_SEND_BREAK            0x23

struct cdc_line_coding {
	u32 rate;
	u8  format;
	u8  parity;
	u8  bits;
} __attribute__((packed));


/* USB2.1 CDC-ACM device descriptor; one config */
const uchar usb_acm_desc[18] = {
	18,         /* bLength (device descriptor length) */
	0x01,       /* bDescriptorType = Device Descriptor (0x01) */
	0x00, 0x02, /* bcdUSB = USB 2 */
	0x02,       /* USB-CDC class */
	0x00,       /* sub-class (none) */
	0x00,       /* protocol (none; see interfaces) */
	64,         /* max packet size for control endpoint = 64 */
	0x66, 0x66, /* vendor ID */
	0x66, 0x66, /* product ID */
	0x00, 0x01, /* device version */
	0x00,       /* manufacturer string index (none) */
	0x00,       /* product string index (none) */
	0x00,       /* serial number index (none) */
	0x01,       /* bNumConfigurations = 1 */
};

/* USB2.1 CDC-ACM device config
 *
 * Typically CDC-ACM devices are two interfaces where interface
 * zero is the 'control interface' and interface one is the
 * 'data' interface. Mostly the requirements here were
 * cargo-culted from what the Linux 'cdc_acm'
 * driver wanted to see, which in turn appears to be
 * cargo-culted from Table 18 of the USB CDC class standard
 * document.
 *
 * NOTE: we've skipped the (technically required) 'Call Management'
 * USB-CDC descriptor because it keeps the config length under
 * the magical 64-byte mark, and because most drivers appear to
 * default to using the CDC Union descriptor master interface
 * anyway, which is what we want. Also, who the heck is
 * using ttyACMx to make calls on a modem these days...? */
const uchar usb_acm_config[] = {
	9,     /* descriptor length = 9 */
	0x02,  /* descriptor type = Configuration Descriptor (0x02) */
	67, 0x00, /* total length = 62 bytes */
	2,     /* two interfaces */
	0x01,  /* configuration index = 1 */
	0x00,  /* configuration string index (none) */
	0x80,  /* attributes (0x80 == bus-powered) */
	50,    /* FIXME: max power = 100mA */

	/* first interface */
	9,     /* descriptor length = 9 */
	0x04,  /* interface descriptor */
	0x00,  /* interface number = 0 */
	0x00,  /* alternate setting */
	1,     /* number of endpoints in interface */
	0x02,  /* communications interface class */
	0x02,  /* ACM sub-class */
	0x00,  /* no protocol (should we select vt.250 here?) */
	0x00,  /* interface string descriptor (none) */

	/* class-specific interface descriptors */

	5,          /* descriptor length = 5 */
	0x24,       /* CS_INTERFACE */
	0x00,       /* header subtype */
	0x10, 0x01, /* USB CDC specification version */

	4,     /* descriptor length = 4 */
	0x24,  /* CS_INTERFACE */
	0x02,  /* ACM subtype */
	0x00,  /* ACM capabilities (none; please don't sent control transfers!) */

	5,     /* descriptor length = 5 */
	0x24,  /* CS_INTERFACE */
	0x01,  /* Call Management */
	0x03,  /* receive call mgmt over data interface;
		  device handles call mgmt itself */
	0x01,  /* data interface = interface 1 */

	5,     /* descriptor length = 5 */
	0x24,  /* CS_INTERFACE */
	0x06,  /* CDC Union */
	0x00,  /* interface 0 = control interface */
	0x01,  /* interface 1 = data interface */

	/* endpoint descriptor for interface 0 (control interface) */
	7,     /* 7 bytes */
	0x05,  /* endpoint descriptor */
	0x82,  /* endpoint address (2, IN) */
	0x03,  /* interrupt transfer type; usage = data */
	64, 0x00, /* max packet size = 64 */
	0xff,  /* interval */

	/* second interface */
	9,    /* descriptor length = 9 */
	0x04, /* interface descriptor */
	0x01, /* interface number = 1 */
	0x00, /* alternate setting = 0 */
	2,    /* number of endpoints in interface */
	0x0A, /* CDC Data interface class */
	0x00, /* CDC Data subclass code */
	0x00, /* transparent protocol */
	0x00, /* interface string descriptor (none) */

	/* endpoint descriptor (output) */
	7,    /* 7 bytes */
	0x05, /* endpoint descriptor */
	0x01, /* endpoint address (1, OUT) */
	0x02, /* bulk transfer type; usage = data */
	64, 0x00, /* max packet size = 64 */
	0x00, /* interval (irrelevant for bulk transfers) */

	7,    /* 7 bytes */
	0x05, /* endpoint descriptor */
	0x81, /* endpoint address (1, IN) */
	0x02, /* bulk transfer type; usage = data */
	64, 0x00, /* max packet size = 64 */
	0x00, /* interval (irrelevant for bulk transfers) */
};

/* make sure this value matches what is
 * reported in the descriptor! */
static_assert(sizeof(usb_acm_config) == 67);

static inline struct acm_data *
classdata(struct usb_dev *dev)
{
	return dev->classdata;
}

static inline long
acm_read(struct usb_dev *dev, uchar *dst, ulong len)
{
	/* TODO: lock irq */
	long rd = 0;
	struct acm_data *acm = classdata(dev);

	if (acm->discard_in)
		return -EINVAL;

	if (acm->instatus)
		return acm->instatus;
	while (acm->read < acm->inread && len) {
		*dst++ = acm->inbuf[acm->read++];
		len--;
		rd++;
	}
	/* if we wanted to read more, allow
	 * more host OUT packets */
	if (acm->read == acm->inread && len) {
		acm->instatus = -EAGAIN;
		acm->read = 0;
		acm->inread = 0;
		dev->drv->expect_out(dev, 1, acm->inbuf, sizeof(acm->inbuf));
	}
	return rd;
}

static inline long
acm_write(struct usb_dev *dev, const uchar *src, ulong len)
{
	/* TODO: lock irq */
	struct acm_data *acm = classdata(dev);

	if (acm->outstatus)
		return (long)acm->outstatus;

	if (len == 0)
		return 0;

	if (len > sizeof(acm->outbuf))
		len = sizeof(acm->outbuf);

	acm->outstatus = -EAGAIN;
	memcpy(acm->outbuf, src, len);
	dev->drv->expect_in(dev, 0x81, acm->outbuf, sizeof(acm->outbuf));
	return len;
}

long
acm_odev_write(const struct output *odev, const char *data, ulong len)
{
	return acm_write(odev->ctx, (const uchar *)data, len);
}

long
acm_idev_read(const struct input *idev, void *data, ulong dstlen)
{
	return acm_read(idev->ctx, (uchar *)data, dstlen);
}

/* called in interrupt context after an OUT */
static void
post_out(struct usb_dev *dev, u8 ep, void *buf, uchar len, int err)
{
	struct acm_data *acm = classdata(dev);
	if (ep != 0x01)
		return;
	acm->read = 0;
	acm->inread = len;
	acm->instatus = err;

	/* if we're discarding input, just keep
	 * accepting reads into the buffer */
	if (acm->discard_in)
		dev->drv->expect_out(dev, 0x01, acm->inbuf, sizeof(acm->inbuf));
}

/* called in interrupt context after an IN */
static void
post_in(struct usb_dev *dev, u8 ep, int err)
{
	if (ep == 0x81)
		classdata(dev)->outstatus = err;
}

static int
handle_setup(struct usb_dev *dev, struct usb_setup *setup)
{
	/* TODO: handle GET/SET_LINE_CODING, SEND_BREAK
	 *
	 * the capabilities descriptor we include above
	 * doesn't say we support any of the class-specific
	 * control messages, so in theory we shouldn't get
	 * here at all.. however, the Linux cdc_acm driver
	 * ignores the capabilities descriptor :(
	 * the default control message behavior will pretend
	 * to handle messages with zero-length data phases
	 * for us, so we just need to handle setup->datalen > 0
	 */

	if (USB_REQ_DIR(setup->rtype) == REQ_HOST_TO_DEV && setup->datalen > 0) {
		if (setup->datalen > 64)
			return -1;
		usb_ignore_out(dev);
		return 0;
	} else if (USB_REQ_DIR(setup->rtype) == REQ_DEV_TO_HOST && setup->datalen > 0) {
		/* TERRIBLE HACK:
		 * if we get a request that we don't know how
		 * to answer, but we know how many bytes the response
		 * should be, then send all zeros! */
		if (setup->datalen > 64)
			return -1;
		memset(dev->ctrlbuf, 0, setup->datalen);
		usb_ep0_queue_response(dev, dev->ctrlbuf, setup->datalen);
	}
	return -1;
}

static void
acm_reset(struct usb_dev *dev)
{
	struct acm_data *acm = classdata(dev);

	dev->drv->init_ep(dev, 0x01, EP_BULK);
	dev->drv->init_ep(dev, 0x81, EP_BULK);

	/* this endpoint is unimplemented, but
	 * it's part of the descriptor, so we
	 * send NAKs for it */
	dev->drv->init_ep(dev, 0x82, EP_INTERRUPT);

	/* if we were sending data, clear errors
	 * so we can start sending/receiving again */
	acm->outstatus = 0;
	acm->instatus = 0;

	/* if we're discarding input data, make sure
	 * we start ACKing it immediately */
	if (acm->discard_in)
		dev->drv->expect_out(dev, 0x01, acm->inbuf, sizeof(acm->inbuf));
}

/* note: endpoint 0x82 doesn't have an implementation;
 * some ttyACM implementations expect it to exist */
const struct usb_class usb_cdc_acm = {
	.devdesc = &usb_acm_desc,
	.confdesc = usb_acm_config,
	.conflen = sizeof(usb_acm_config),
	.handle_setup = handle_setup,
	.handle_reset = acm_reset,
	.ep_rx = post_out,
	.ep_tx = post_in,
};
