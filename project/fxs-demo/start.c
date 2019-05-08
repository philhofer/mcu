#include <bits.h>
#include <error.h>
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

static void
timing_error(void)
{
	const char *msg = "\r\nTiming Error!\r\n";
	while (acm_odev_write(&usbttyout, msg, 17) == -EAGAIN) ;
	bkpt();
}

static u64
main_iter(struct madgwick_state *mw, u64 lastupd, bool calibrate)
{
	u64 updtime;
	while (!accel_updated) {
		if (lastupd && (getcycles()-lastupd) > (CPU_HZ/10))
			timing_error();
		idle_step(true);
	}
	updtime = getcycles();
	accel_updated = false;
	madgwick(mw, &gstate.gyro, &mstate.accel, &mstate.mag, (32768/200), calibrate);
	gyro_drdy_clear_enable();
	print_axyz(&mw->quat0);
	return updtime;
}

void
start(void) {
	int err;
	u64 lastupd;
	struct madgwick_state mw;

	default_usb_acm_data.discard_in = true;
	gstate.on_update = gyro_on_update;
	mstate.on_update = accel_on_update;

	gpio_reset(&red_led, GPIO_OUT);
	gpio_write(&red_led, 0);

	default_usb_init();
	assert(default_i2c_init() == 0);
	extint_init();

	madgwick_init(&mw, 10);

	err = fxas_enable(&default_i2c);
	assert(err == 0);

	err = fxos_enable(&default_i2c);
	assert(err == 0);

	/* initialization complete */
	gpio_write(&red_led, 1);

	lastupd = 0;
	gyro_drdy_clear_enable();

	/* run about three seconds of calibration */
	for (unsigned i=0; i<600; i++)
		lastupd = main_iter(&mw, lastupd, true);
	for (;;)
		lastupd = main_iter(&mw, lastupd, false);
}
