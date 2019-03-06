#include <bits.h>
#include <config.h>
#include <arch.h>
#include <assert.h>
#include <i2c.h>
#include <feather-m0-i2c.h>
#include <red-led.h>

u64 starttime;
u64 success;

void start(void) {
	int err;
	char buf[1];

	gpio_reset(&red_led, GPIO_OUT);
	gpio_write(&red_led, 0);

	err = i2c_dev_reset(&default_i2c, I2C_SPEED_NORMAL);
	assert(err == 0);
	gpio_write(&red_led, 1);

	starttime = getcycles();
	for (;;) {
		err = i2c_read_sync(&default_i2c, 0x21, 0x00, buf, 1);
		assert(err == 0);
		success++;
	}
}
