#ifndef __SAM_USB_H_
#define __SAM_USB_H_
#include <usb.h>

/* sam_usb_irq_bh() is the SAM USB driver interrupt
 * handler and should be called from idle context */
extern void sam_usb_irq_bh(struct usb_dev *dev);

extern const struct usb_driver sam_usb_driver;

#endif
