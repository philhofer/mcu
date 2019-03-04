#ifndef __HOST_USB_LOOPBACK_H_
#define __HOST_USB_LOOPBACK_H_
#include <arch.h>
#include <bits.h>
#include "../usb.h"
#include "../usb-internal.h"

#define USB_EMULATOR_IRQ 7
#define USB_BUFSIZE 64

extern const struct usb_driver host_emulator;
struct usb_lo;

extern bool usb_trace;

/* interrupt handler */
void usb_emu_irq(struct usb_dev *dev);

struct usb_lo *new_usb_loopback(void);

static inline struct usb_lo *
host_dev_init(struct usb_dev *dev, const struct usb_class *class, void *classdata)
{
	dev->class = class;
	dev->drv = &host_emulator;
	dev->classdata = classdata;
	dev->driverdata = new_usb_loopback();

	dev->ep0_out = NULL;
	dev->ep0_in = NULL;

	dev->tx_more = NULL;
	dev->more_len = 0;
	dev->queued_addr = 0;

	return dev->driverdata;
}

/* host_control() sends a setup packet to
 * the usb device. If the setup packet has 
 * a data phase, 'dataphase' points to wDataLength
 * bytes that are either read or written to
 * depending on the direction of the data phase.
 *
 * like most host_xxx() functions, this function
 * blocks until all phases of the control transfer
 * are completed */
int host_control(struct usb_dev *dev, const struct usb_setup *setup, void *dataphase);

static inline int
host_get_status(struct usb_dev *dev, u16 value, u16 *status)
{
	struct usb_setup setup = {
		.rtype = REQ_TO_DEVICE|REQ_TYPE_STANDARD|REQ_DEV_TO_HOST,
		.rcode = USB_GET_STATUS,
		.value = value,
		.datalen = 2,
	};
	return host_control(dev, &setup, status);
}

static inline int
host_set_address(struct usb_dev *dev, u8 address)
{
	struct usb_setup setup = {
		.rtype = REQ_TO_DEVICE|REQ_TYPE_STANDARD|REQ_HOST_TO_DEV,
		.rcode = USB_SET_ADDRESS,
		.value = address,
	};
	return host_control(dev, &setup, NULL);
}

static inline int
host_get_descriptor(struct usb_dev *dev, u16 value, char *buf, unsigned maxlen)
{
	struct usb_setup setup = {
		.rtype = REQ_TO_DEVICE|REQ_TYPE_STANDARD|REQ_DEV_TO_HOST,
		.rcode = USB_GET_DESCRIPTOR,
		.value = value,
		.datalen = maxlen,
	};
	return host_control(dev, &setup, buf);
}

static inline int
host_get_config(struct usb_dev *dev, u8 number, u8 *out)
{
	struct usb_setup setup = {
		.rtype = REQ_TO_DEVICE|REQ_TYPE_STANDARD|REQ_DEV_TO_HOST,
		.rcode = USB_GET_CONFIGURATION,
		.value = number,
		.datalen = 1,
	};
	return host_control(dev, &setup, out);
}

static inline int
host_set_config(struct usb_dev *dev, u8 number)
{
	struct usb_setup setup = {
		.rtype = REQ_TO_DEVICE|REQ_TYPE_STANDARD|REQ_HOST_TO_DEV,
		.rcode = USB_SET_CONFIGURATION,
		.value = number,
	};
	return host_control(dev, &setup, NULL);
}

int host_reset(struct usb_dev *dev);

int host_out(struct usb_dev *dev, u8 ep, const void *buffer, u8 len);

int host_in(struct usb_dev *dev, u8 ep, void *buffer, u8 maxlen);

static inline int
host_out_full(struct usb_dev *dev, u8 ep, const void *buffer, int len)
{
	const char *bp = buffer;
	int rc;
	u8 wr;
	do {
		wr = (len > USB_BUFSIZE) ? USB_BUFSIZE : len;
		rc = host_out(dev, ep, bp, wr);
		if (rc < 0)
			return rc;
		if (rc != wr)
			return -EPROTO;
		bp += rc;
		len -= rc;
	} while (len);
	return 0;
}

static inline int
host_in_full(struct usb_dev *dev, u8 ep, void *buffer, int len)
{
	char *bp = buffer;
	int rc;
	u8 wr;
	do {
		wr = (len > USB_BUFSIZE) ? USB_BUFSIZE : len;
		rc = host_in(dev, ep, bp, wr);
		if (rc < 0)
			return rc;
		if (rc != wr)
			return -EPROTO;
		bp += rc;
		len -= rc;
	} while (len);
	return 0;
}
#endif
