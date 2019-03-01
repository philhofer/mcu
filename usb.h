#ifndef __USB_H_
#define __USB_H_
#include <bits.h>

struct usb_class;
struct usb_driver;
struct usb_dev;

struct usb_dev {
	const struct usb_class  *class;
	const struct usb_driver *drv;

	/* the class and driver implementations can
	 * store data here */
	void *classdata;
	void *driverdata;

	/* used by control state machine */
	void (*ep0_out)(struct usb_dev *dev);
	void (*ep0_in)(struct usb_dev *dev);

	uchar ctrlbuf[64] __attribute__((aligned(4)));

	/* if more than 64 bytes need to be sent
	 * (for large descriptors, etc.), then these
	 * fields are set */
	const uchar *tx_more;
	ulong more_len;
	u8    queued_addr;
} __attribute__((aligned(4)));

void usb_init(struct usb_dev *usb);

#endif
