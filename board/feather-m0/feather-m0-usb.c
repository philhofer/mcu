#include <usb.h>
#include <usb-internal.h>
#include <usb-cdc-acm.h>
#include <drivers/sam/usb.h>
#include <default-usb.h>
#include <libc.h>

struct acm_data default_acm_data = {
	.discard_in = true,
};

struct usb_dev default_usb = {
	.class = &usb_cdc_acm,
	.drv = &sam_usb_driver,
	.classdata = &default_acm_data,
};

USBSERIAL_OUT(usbttyout, &default_usb);

const struct output *stdout = &usbttyout;

void usb_irq_entry(void) {
	sam_usb_irq(&default_usb);
}
