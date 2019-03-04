#include <bits.h>
#include <arch.h>
#include <assert.h>
#include <usb-internal.h>

/* general method of operation:
 *
 * USB control messages are a strange beast,
 * since there are at least two phases of each transfer
 * (and potentially more). After the SETUP packet,
 * we should expect some IN or OUT packets, or some
 * 'status' packets (zero-length IN or OUT packets).
 * The way we manage this is by calling
 * either expect_out() or expect_in() on the driver,
 * and setting on_out() or on_in() to whatever code
 * needs to be executed when that happens. Generally
 * the end of that chain of callbacks should call
 * ep0_default(), which stalls the endpoint */

static inline void ep0_default(struct usb_dev *dev);

static void expect_in_zlp(struct usb_dev *dev);
static void expect_out_zlp(struct usb_dev *dev);

void
usb_ignore_out(struct usb_dev *dev)
{
	dev->ep0_out = expect_in_zlp;
	dev->drv->expect_out(dev, 0x00, dev->ctrlbuf, 64);
}

/* expect a zero-length IN status packet */
static inline void
expect_in_zlp(struct usb_dev *dev)
{
	dev->ep0_in = ep0_default;
	dev->drv->expect_in(dev, 0x80, dev->ctrlbuf, 0);
}

/* expect a zero-length OUT status packet */
static inline void
expect_out_zlp(struct usb_dev *dev)
{
	dev->ep0_out = ep0_default;
	dev->drv->expect_out(dev, 0, dev->ctrlbuf, 0);
}

static inline void
ep0_stall(struct usb_dev *dev)
{
	dev->drv->stall_ep(dev, 0);
	dev->drv->stall_ep(dev, 0x80);
}

static inline void
ep0_default(struct usb_dev *dev)
{
}

static int
handle_get_descriptor(struct usb_dev *dev, u8 type, u8 index, u16 outlen)
{
	switch (type) {
	case 1: /* device */
		if (outlen > 18)
			outlen = 18;
		usb_ep0_queue_response(dev, dev->class->devdesc, outlen);
		return 0;
	case 2: /* configuration */

		/* only config 0 is supported */
		if (index == 0) {
			if (outlen > dev->class->conflen)
				outlen = dev->class->conflen;
			usb_ep0_queue_response(dev, dev->class->confdesc, outlen);
			return 0;
		}
		break;
		/* not implemented */
	case 3: /* string */
	case 4: /* interface */
	case 5: /* endpoint */
	case 6: /* device qualifier */
	case 7: /* "other_speed_configuration" */
	case 8: /* interface power */
		break;
	}
	ep0_stall(dev);
	return -1;
}

static int
handle_get_config(struct usb_dev *dev, u16 raw)
{
	/* we're always on config 1!
	 * According to the standard, we should
	 * return 0 here before we have been
	 * assigned an address... */
	uchar buf[1] = {1};
	usb_ep0_queue_response(dev, buf, 1);
	return 0;
}

static int
handle_set_config(struct usb_dev *dev, u16 raw)
{
	/* we only support config=1 */
	return 0;
}

static int
handle_get_interface(struct usb_dev *dev, u16 raw)
{
	/* we don't support alternate interfaces */
	uchar buf[1] = {0};
	usb_ep0_queue_response(dev, buf, 1);
	return 0;
}

static int
handle_class_setup(struct usb_dev *dev, struct usb_setup *setup)
{
	if (dev->class->handle_setup(dev, setup) < 0) {
		if (setup->datalen > 0)
			ep0_stall(dev);
		return -1;
	}
	return 0;
}

void
usb_ep0_rx(struct usb_dev *dev, int err)
{
	void (*fn)(struct usb_dev *);

	if (dev->ep0_out) {
		fn = dev->ep0_out;
		dev->ep0_out = NULL;
		fn(dev);
	}
}

void
usb_ep0_tx(struct usb_dev *dev, int err)
{
	void (*fn)(struct usb_dev *);

	if (dev->ep0_in) {
		fn = dev->ep0_in;
		dev->ep0_in = NULL;
		fn(dev);
	}
}

static void
set_queued_addr(struct usb_dev *dev)
{
	if (dev->queued_addr) {
		dev->drv->set_addr(dev, dev->queued_addr);
		dev->queued_addr = 0;
	}
	ep0_default(dev);
}

static void
set_dev_address(struct usb_dev *dev, u16 raw)
{
	dev->queued_addr = raw & 0x7f;
	dev->ep0_in = set_queued_addr;
}

static void
send_more(struct usb_dev *dev)
{
	if (dev->tx_more && dev->more_len)
		usb_ep0_queue_response(dev, dev->tx_more, dev->more_len);
	else
		ep0_default(dev); /* we shouldn't get here... */
}

/* usb_ep0_queue_response() handles
 * control transfers with data packets
 * in the IN (device-to-host) direction */
void
usb_ep0_queue_response(struct usb_dev *dev, const uchar *data, ulong len)
{
	ulong tosend = len;

	dev->tx_more = NULL;
	dev->more_len = 0;
	if (tosend > sizeof(dev->ctrlbuf)) {
		tosend = sizeof(dev->ctrlbuf);
		dev->tx_more = data + tosend;
		dev->more_len = len - tosend;
		dev->ep0_in = send_more;
	} else {
		dev->ep0_in = expect_out_zlp;
	}
	memcpy(dev->ctrlbuf, data, tosend);
	dev->drv->expect_in(dev, 0x80, dev->ctrlbuf, tosend);
}

static int
handle_get_status(struct usb_dev *dev)
{
	/* don't care about the recipient;
	 * we don't implemnt remote-wakeup
	 * or self-powered mode right now... */
	uchar buf[2] = {0};
	usb_ep0_queue_response(dev, buf, 2);
	return 0;
}

void
usb_unhandled(unsigned n)
{
	/* for gdb dprintf */
}

static inline int
decode_setup(struct usb_setup *dst, const uchar *data, u8 len)
{
	if (len < 8)
		return -EINVAL;
	dst->rtype = data[0];
	dst->rcode = data[1];
	dst->value = data[2] | ((u16)data[3] << 8);
	dst->index = data[4] | ((u16)data[5] << 8);
	dst->datalen = data[6] | ((u16)data[7] << 8);
	return 0;
}

int
usb_handle_setup(struct usb_dev *dev, const uchar *in, u8 len)
{
	struct usb_setup setup;
	int err;

	if ((err = decode_setup(&setup, in, len)) < 0)
		return err;

	/* first, sane defaults for everyone:
	 * if this is a host-to-device (OUT)
	 * setup packet, then expect a zero-length
	 * IN as an ACK; */
	if (setup.datalen == 0) {
		if (USB_REQ_DIR(setup.rtype) == REQ_HOST_TO_DEV)
			expect_in_zlp(dev);
		else
			expect_out_zlp(dev);
	}

	switch (USB_REQ_TYPE(setup.rtype)) {
	case REQ_TYPE_STANDARD:
		switch (setup.rcode) {
		case USB_GET_STATUS:
			return handle_get_status(dev);
		case USB_CLEAR_FEATURE:
		case USB_SET_FEATURE:
		case USB_SET_INTERFACE:
			/* unimplemented, but don't
			 * bail out, just accept the data */
			return 0;
		case USB_SET_ADDRESS:
			set_dev_address(dev, setup.value);
			return 0;
		case USB_GET_DESCRIPTOR:
			return handle_get_descriptor(dev,
					setup.value >> 8,
					setup.value & 0xff,
					setup.datalen);

		case USB_GET_CONFIGURATION:
			return handle_get_config(dev, setup.value);
		case USB_SET_CONFIGURATION:
			return handle_set_config(dev, setup.value);
		case USB_GET_INTERFACE:
			return handle_get_interface(dev, setup.value);
		}
		break;
	case REQ_TYPE_CLASS:
		return handle_class_setup(dev, &setup);
	}
	/* If datalen was zero, we pretend to handle the transfer.
	 * Otherwise, we haven't arranged for anything to happen,
	 * and we need to yield a STALL in the data/setup phase. */
	if (setup.datalen != 0)
		ep0_stall(dev);
	return -1;
}

void
usb_reset(struct usb_dev *dev)
{
	dev->ep0_out = NULL;
	dev->ep0_in = NULL;
	dev->drv->init_ep(dev, 0x00, EP_CONTROL);
	dev->drv->init_ep(dev, 0x80, EP_CONTROL);
	ep0_default(dev);
	dev->class->handle_reset(dev);
}

void
usb_init(struct usb_dev *dev)
{
	dev->drv->init(dev);
}
