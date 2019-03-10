#include <bits.h>
#include <config.h>
#include <arch.h>
#include <assert.h>
#include <i2c.h>
#include <usb.h>
#include <libc.h>
#include <default-usb.h>
#include <feather-m0-i2c.h>
#include <dev/fxas.h>
#include <red-led.h>

struct fxas_state gstate;

static void
pause(u64 cycles)
{
	u64 start = getcycles();
	while (getcycles() < start + cycles)
		idle_step(true);
}

/* three 6-character numbers, two commas, a newline, and a null */
static char outbuf[6*3 + 2 + 1 + 1];

static void
print_gyro(void)
{
	char *p = outbuf;
	mrads *m, v;
	int i;

	m = &gstate.gyro.x;
	for (i=0; i<3; i++) {
		v = *m++;
		if (v < 0) {
			*p++ = '-';
			v = -v;
		}
		p += 5;
		*--p = '0' + (v%10);
		v /= 10;
		*--p = '0' + (v%10);
		v /= 10;
		*--p = '0' + (v%10);
		v /= 10;
		*--p = '0' + (v%10);
		v /= 10;
		*--p = '0' + v;
		p += 5;
		if (i != 2)
			*p++ = ',';
	}
	*p++ = '\n';
	*p++ = 0;
	print(outbuf);
}

static void
debias(struct fxas_state *state)
{
	/* these were my measured zero-rate biases */
	state->gyro.x -= 18;
	state->gyro.y -= 10;

}

void start(void) {
	int err;
	u64 starttime;

	gpio_reset(&red_led, GPIO_OUT);
	gpio_write(&red_led, 0);

	usb_init(&default_usb);

	err = i2c_dev_reset(&default_i2c, I2C_SPEED_NORMAL);
	assert(err == 0);
	gpio_write(&red_led, 1);

	err = fxas_enable(&default_i2c);
	assert(err == 0);

	/* wait one second */
	pause(CPU_HZ);

	for (;;) {
		starttime = getcycles();
		err = fxas_read_state(&default_i2c, &gstate);
		assert(err == 0);
		while (gstate.last_update < starttime) {
			assert(gstate.last_err == 0);
			idle_step(true);
		}
		/* these were my measured gyro biases */
		debias(&gstate);
		print_gyro();
	}
}
