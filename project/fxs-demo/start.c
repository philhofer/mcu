#include <bits.h>
#include <config.h>
#include <arch.h>
#include <board.h>
#include <assert.h>
#include <i2c.h>
#include <usb.h>
#include <extirq.h>
#include <libc.h>
#include <usb-cdc-acm.h>
#include <madgwick.h>
#include <dev/fxas.h>
#include <dev/fxos.h>

struct fxas_state gstate; /* gyro state */
struct fxos_state mstate; /* mag/accel state */

USBSERIAL_OUT(usbttyout, &default_usb);

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

/* both the accel+magnetometer and gyro devices
 * are on the same bus, so we have to build a little
 * state machine here to ensure we sequence things appropriately
 *
 * the order of events here is
 *   gryo_drdy_clear_enable()
 *     (wait for interrupt)
 *   gyro_drdy_entry() -> fxas_read_state()
 *     (wait for tx done callback)
 *   gyro_on_update() -> accel_drdy_clear_enable()
 *     (wait for interrupt)
 *   accel_drdy_entry() -> fxos_read_state()
 *     (wait for tx done callback)
 *   accel_on_update()
 */
static bool accel_updated;

static void
gyro_on_update(void)
{
	accel_drdy_clear_enable();
}

static void
accel_on_update(void)
{
	accel_updated = true;
}

void
gyro_drdy_entry(void)
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

void
accel_drdy_entry(void)
{
	int err;

	err = fxos_read_state(&default_i2c, &mstate);
	assert(err == 0);
}

void start(void) {
	int err;
	struct madgwick_state mw;

	gstate.on_update = gyro_on_update;
	mstate.on_update = accel_on_update;

	gpio_reset(&red_led, GPIO_OUT);
	gpio_write(&red_led, 0);

	default_usb_init();
	assert(default_i2c_init() == 0);

	madgwick_init(&mw, 10);

	err = i2c_dev_reset(&default_i2c, I2C_SPEED_NORMAL);
	assert(err == 0);

	err = fxas_enable(&default_i2c);
	assert(err == 0);

	err = fxos_enable(&default_i2c);
	assert(err == 0);

	extint_init();

	/* initialization complete */
	gpio_write(&red_led, 1);

	/* start the measurement update event loop */
	gyro_drdy_clear_enable();
	for (;;) {
		while (!accel_updated)
			idle_step(true);
		accel_updated = false;
		debias(&gstate);
		/* TODO: actually calculate the time-delta here instead
		 * of assuming that we are always doing 200Hz... */
		madgwick(&mw, &gstate.gyro, &mstate.accel, &mstate.mag, (32768/200));
		gyro_drdy_clear_enable();
		print_axyz(&mw.quat0);
	}
}
