#ifndef __SAM_PORT_H_
#define __SAM_PORT_H_
#include <gpio.h>
#include <bits.h>

extern const struct gpio_ops port_gpio_ops;

/* SAM boards devide pins into "port groups"
 * of 32 pins each, so we use the lower 5 bits
 * of the pin number for the pin number within
 * the port group, and bits above that (at least 3?)
 * for the port group number */
#define PORT_PINNUM(xgrp, xpin) (((xgrp)<<5)|(xpin))
#define PORT_GROUP(num)         ((num)>>5)
#define PORT_NUM(num)           ((num)&0x1f)

#define PORT_GPIO_PIN(name, xgroup, xpin)		\
	const struct gpio name = {			\
		.driver.number = PORT_PINNUM(xgroup, xpin),  \
		.ops = &port_gpio_ops 			\
	}

/* EIC_NUM() turns a raw pin number into the
 * expected SAM EIC interrupt number
 *
 * TODO: confirm this mapping holds true on
 * devices other than the SAMD21 */
#define EIC_NUM(pin) ((pin)&0xf)

/* port_pmux_pin() sets a pin within a port group
 * to the given peripheral role. If 'input' is set,
 * the pin is configured as an input pin with the
 * internal pull-up/down resistor enabled.
 *
 * Consult the datasheet for your microcontroller
 * for the pin 'role' multiplexing table. Roles
 * are assigned using a single capital letter
 * beginning at 'A'. */
int port_pmux_pin(unsigned group, unsigned pin, uchar role);

#endif

