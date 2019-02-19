#ifndef __SAM_PORT_H_
#define __SAM_PORT_H_
#include <gpio.h>
#include <bits.h>

extern const struct gpio_ops port_gpio_ops;

#define PORT_GPIO_PIN(name, xgroup, xpin)		\
	const struct gpio name = {		\
		.driver.number = ((xgroup) << 8) | (xpin),  \
		.ops = &port_gpio_ops 		\
	}

/* port_pmux_init() sets a pin within a port group
 * to the given peripheral role. If 'input' is set,
 * the pin is configured as an input pin with the
 * internal pull-up/down resistor enabled.
 *
 * Consult the datasheet for your microcontroller
 * for the pin 'role' multiplexing table. Roles
 * are assigned using a single capital letter
 * beginning at 'A'. */
int port_pmux_init(unsigned group, unsigned pin, uchar role);

#endif

