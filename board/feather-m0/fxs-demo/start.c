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
#include <usb-cdc-acm.h>
#include <madgwick.h>
#include <feather-m0-i2c.h>
#include <feather-m0-pins.h>
#include <dev/fxas.h>
#include <dev/fxos.h>
#include <red-led.h>

struct fxas_state gstate; /* gyro state */
struct fxos_state mstate; /* mag/accel state */

static void
print_axyz(i16 *m)
{
	/* four 6-character numbers, three commas, and a carriage return */
	char outbuf[6*4 + 3 + 1];
	char *p = outbuf;
	i16 v;
	int i;

	*p++ = '\r';
	for (i=0; i<4; i++) {
		v = m[i];
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
		if (i != 3)
			*p++ = ',';
	}
	/* deliberately ignore errors here;
	 * we'll get -EAGAIN if the host isn't reading fast
	 * enough, but we need to continue doing orientation
	 * updates regardless */
	acm_odev_write(&usbttyout, outbuf, p - outbuf);
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
#define FXOS_DRDY_PIN PIN_A1

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

static void
accel_drdy(void)
{
	int err;

	err = fxos_read_state(&default_i2c, &mstate);
	assert(err == 0);
}

/* global external interrupt table */
const struct extirq extirq_table[EIC_MAX] = {
	[EIC_NUM(FXAS_DRDY_PIN)] = {.func = gyro_drdy,  .trig = FXAS_DRDY_TRIGGER, .pin = FXAS_DRDY_PIN},
	[EIC_NUM(FXOS_DRDY_PIN)] = {.func = accel_drdy, .trig = FXOS_DRDY_TRIGGER, .pin = FXOS_DRDY_PIN},
};

void start(void) {
	int err;
	u64 starttime;
	struct madgwick_state mw;

	gpio_reset(&red_led, GPIO_OUT);
	gpio_write(&red_led, 0);

	usb_init(&default_usb);

	madgwick_init(&mw, 10);

	err = i2c_dev_reset(&default_i2c, I2C_SPEED_NORMAL);
	assert(err == 0);

	err = fxas_enable(&default_i2c);
	assert(err == 0);

	err = fxos_enable(&default_i2c);
	assert(err == 0);

	err = extirq_init();
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
		extirq_clear_enable(FXOS_DRDY_PIN);
		debias(&gstate);
		while (mstate.last_update < starttime) {
			assert(mstate.last_err == 0);
			idle_step(true);
		}
		/* TODO: actually calculate the time-delta here instead
		 * of assuming that we are always doing 200Hz...
		 *
		 * we aren't turning on debiasing until we've run the
		 * loop for two seconds and the system has presumably
		 * gotten close to a steady-state */
		madgwick(&mw, &gstate.gyro, &mstate.accel, &mstate.mag, (32768/200), starttime > 2*CPU_HZ);
		print_axyz(&mw.quat0);
	}
}
