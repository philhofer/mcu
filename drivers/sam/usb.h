#ifndef __SAM_USB_H_
#define __SAM_USB_H_
#include <usb.h>

extern void sam_usb_irq(struct usb_dev *dev);

extern const struct usb_driver sam_usb_driver;

#endif
