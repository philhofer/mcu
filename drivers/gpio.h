#ifndef __DRIVER_GPIO_H_
#define __DRIVER_GPIO_H_
#include <gpio.h>

struct gpio_ops {
	int (*reset)(const struct gpio *gp, int flags);
	int (*set)(const struct gpio *gp, int up);
	int (*get)(const struct gpio *gp);
	int (*toggle)(const struct gpio *gp); /* optional! */
};

#endif

