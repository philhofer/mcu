#include <arch.h>
#include <gpio.h>
#include <config.h>

void start(void) {
	u64 nextcycles;

	gpio_reset(&red_led, GPIO_OUT);
	gpio_write(&red_led, 1);

	for (;;) {
		/* clumsy busy-waiting for 1 second:
		 * if the light toggles every second,
		 * then the cycle counter agrees with
		 * CONFIG_CPU_HZ */
		nextcycles = getcycles() + CPU_HZ;
		while (getcycles() < nextcycles)
			;
		gpio_toggle(&red_led);
	}
}
