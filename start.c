#include <arch.h>
#include <libc.h>
#include <config.h>
#include <gpio.h>

extern struct gpio red_led;

void
start(void)
{
	u64 cycles, nextcycles;
	gpio_reset(&red_led, GPIO_OUT);
	gpio_write(&red_led, 1);

	/* super simple demo that toggles
	 * an LED light every second
	 * (assuming the CPU clocks are
	 * configured correctly!) */
	do {
		cycles = getcycles();
		nextcycles = cycles + CPU_HZ;

		while ((cycles = getcycles()) < nextcycles) ;
		gpio_toggle(&red_led);

		print("Hello, World!\n");
	} while (1);
}
