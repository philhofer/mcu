#ifndef __USB_INTERNAL_H_
#define __USB_INTERNAL_H_
#include <bits.h>
#include <usb.h>
#include <libc.h>
#include <error.h>

/* A minimal USB driver interface
 *
 * Some notes on restrictions, etc.:
 *
 *   - some control messages are not implemented
 *   - only 1 device configuration descriptor is supported
 *     (this is typical anyway)
 */

/* values for 'Recipient' in bmReqeustType */
#define REQ_TO_DEVICE    0
#define REQ_TO_INTERFACE 1
#define REQ_TO_ENDPOINT  2
#define REQ_TO_OTHER     3

#define USB_REQ_TO(x) (x & 3)

#define REQ_TYPE_STANDARD 0
#define REQ_TYPE_CLASS    (1U << 5)
#define REQ_TYPE_VENDOR   (2U << 5)
#define REQ_TYPE_RESERVED (3U << 5)

#define USB_REQ_TYPE(x) (x & (3U << 5))

#define REQ_HOST_TO_DEV   0
#define REQ_DEV_TO_HOST   (1U << 7)

#define USB_REQ_DIR(x) (x & 0x80)

#define USB_GET_STATUS        0x00
#define USB_CLEAR_FEATURE     0x01
#define USB_SET_FEATURE       0x03
#define USB_SET_ADDRESS       0x05
#define USB_GET_DESCRIPTOR    0x06
#define USB_SET_DESCRIPTOR    0x07
#define USB_GET_CONFIGURATION 0x08
#define USB_SET_CONFIGURATION 0x09

#define USB_GET_INTERFACE    0x0A
#define USB_SET_INTERFACE    0x11

#define USB_SYNCH_FRAME 0x12

struct usb_setup {
	u8   rtype;
	u8   rcode;
	u16  value;
	u16  index;
	u16  datalen;
};

enum ep_type {
	EP_CONTROL     = 1,
	EP_ISOCHRONOUS = 2,
	EP_BULK        = 3,
	EP_INTERRUPT   = 4,
};

struct usb_class {
	const void *devdesc; /* device descriptor; must be 8 bytes */
	const void *confdesc; /* config descriptor */
	ulong       conflen;  /* config descriptor length */

	/* handlers for class-specific setup packets */
	int (*handle_setup)(struct usb_dev *dev, struct usb_setup *setup);

	/* handle a driver reset */
	void (*handle_reset)(struct usb_dev *dev);

	/* handle class-specific endpoint start and end;
	 * called by USB driver in interrupt context; these
	 * can in turn call expect_out() or expect_in() again
	 * on the usb_dev struct */

	/* ep_rx() is called after data has
	 * been received due to a host OUT packet */
	void (*ep_rx)(struct usb_dev *dev, u8 ep, void *buf, uchar len, int err);

	/* ep_tx() is called after data is
	 * sent due to a host IN packet */
	void (*ep_tx)(struct usb_dev *dev, u8 ep, int err);
};

struct usb_driver {
	void (*init)(struct usb_dev *dev);
	
	/* set device address */
	void (*set_addr)(struct usb_dev *dev, u8 addr);

	int (*init_ep)(struct usb_dev *dev, u8 ep, enum ep_type type);

	/* initialize data in preparation for a host OUT on an endpoint */
	int (*expect_out)(struct usb_dev *dev, u8 ep, void *buf, u8 len);

	/* initialize data in preparation for a host IN on an endpoint */
	int (*expect_in)(struct usb_dev *dev, u8 ep, void *buf, u8 len);

	int (*stall_ep)(struct usb_dev *dev, u8 ep0);
};

/* usb_handle_setup() should be called by drivers
 * when a setup packet is received on the 0 (control)
 * interface. It in turn will call the correct driver
 * or class functions depending on the message contents. */
int usb_handle_setup(struct usb_dev *dev, const uchar *data, u8 len);

/* usb_reset() is called by drivers when
 * a host sents a reset */
void usb_reset(struct usb_dev *dev);

/* post-OUT hook */
void usb_ep0_rx(struct usb_dev *dev, int err);

/* post-IN hook */
void usb_ep0_tx(struct usb_dev *dev, int err);

/* usb_ep0_queue_response() is called
 * when a SETUP packet needs a non-zero-length
 * data phase. If multiple IN packets need to
 * be sent to the host, that is arranged for
 * automatically */
void usb_ep0_queue_response(struct usb_dev *dev, const uchar *data, ulong len);

void usb_ignore_out(struct usb_dev *dev);

#endif
