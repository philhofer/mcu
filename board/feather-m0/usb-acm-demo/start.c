#include <arch.h>
#include <libc.h>
#include <config.h>
#include <usb.h>
#include <gpio.h>
#include <idle.h>
#include <red-led.h>
#include <default-usb.h>

void
start(void)
{
	u64 nextcycles;

	usb_init(&default_usb);
	gpio_reset(&red_led, GPIO_OUT);
	gpio_write(&red_led, 1);

	/* you can 'cat /dev/ttyACM0' to
	 * see the output here getting streamed
	 * to you in real time */
	do {
		nextcycles = getcycles() + CPU_HZ;
		gpio_toggle(&red_led);
		print("Toggled.\n");
		while (getcycles() < nextcycles)
			idle_step(true);
	} while (1);
}
