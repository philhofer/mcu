#include <bits.h>
#include <config.h>
#include <arch.h>
#include <board.h>
#include <assert.h>
#include <i2c.h>
#include <usb.h>
#include <extirq.h>
#include <libc.h>
#include <default-usb.h>
#include <feather-m0-i2c.h>
#include <feather-m0-pins.h>
#include <dev/fxas.h>
#include <red-led.h>

struct fxas_state gstate;

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
	state->gyro.x -= 21;
	state->gyro.y -= 10;
	state->gyro.z -= 5;

}

/* select the pin adjacent to SDA/SCL
 * as the gyro data ready interrupt line */
#define FXAS_DRDY_PIN PIN_A0

static void
gyro_drdy(void)
{
	int err;

	/* since the extirq interrupt is masked before
	 * calling the handler, we do not expect to
	 * see EAGAIN here; we don't re-enable the interrupt
	 * until after the i2c transaction has completed and
	 * we have handled the output data */
	err = fxas_read_state(&default_i2c, &gstate);
	assert(err == 0);
}

/* global external interrupt table */
void (* const extirq_table[EIC_MAX])(void) = {
	[EIC_NUM(FXAS_DRDY_PIN)] = gyro_drdy,
};

void start(void) {
	int err;
	u64 starttime;

	gpio_reset(&red_led, GPIO_OUT);
	gpio_write(&red_led, 0);

	usb_init(&default_usb);

	err = i2c_dev_reset(&default_i2c, I2C_SPEED_NORMAL);
	assert(err == 0);

	err = fxas_enable(&default_i2c);
	assert(err == 0);

	err = fxas_attach_drdy(&default_i2c, FXAS_DRDY_PIN);
	assert(err == 0);

	/* initialization complete */
	gpio_write(&red_led, 1);

	for (;;) {
		starttime = getcycles();
		extirq_clear_enable(FXAS_DRDY_PIN);
		while (gstate.last_update < starttime) {
			assert(gstate.last_err == 0);
			idle_step(true);
		}
		debias(&gstate);
		print_gyro();
	}
}
