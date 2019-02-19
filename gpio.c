#include <gpio.h>
#include <drivers/gpio.h>

int
gpio_reset(const struct gpio *gp, int flags)
{
	return gp->ops->reset(gp, flags);
}

int
gpio_write(const struct gpio *gp, int up)
{
	return gp->ops->set(gp, up);
}

int
gpio_read(const struct gpio *gp)
{
	return gp->ops->get(gp);
}

int
gpio_toggle(const struct gpio *gp)
{
	int rv;

	if (gp->ops->toggle)
		return gp->ops->toggle(gp);

	rv = gpio_read(gp);
	return rv < 0 ? rv : gpio_write(gp, !rv);		
}

