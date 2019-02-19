#ifndef __GPIO_H_
#define __GPIO_H_
#include <bits.h>

#define GPIO_IN       (1UL << 0) /* pin is an input pin (default) */
#define GPIO_OUT      (1UL << 1) /* pin is an output pin */
#define GPIO_PULLUP   (1UL << 2) /* input pin needs a pull-up */
#define GPIO_PULLDOWN (1UL << 3) /* input pin needs pull-down */

struct gpio_ops;

struct gpio {
	union {
		const void *ptr;
		u32         number;
	} driver;
	const struct gpio_ops *ops;
};

int gpio_reset(const struct gpio *gp, int flags);

int gpio_write(const struct gpio *gp, int up);

int gpio_read(const struct gpio *gp);

int gpio_toggle(const struct gpio *gp);

#endif
