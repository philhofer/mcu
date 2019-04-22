#ifndef __DEV_FXOS_H_
#define __DEV_FXOS_H_
#include <i2c.h>
#include <extirq.h>
#include <accel.h>
#include <mag.h>

/* wrapper for NXP FXOS8700CQ accelerometer and magnetometer */

struct fxos_state {
	struct accel_state accel;
	struct mag_state   mag;
	void               (*on_update)(void);
	int                last_err;

	u8 buf[12];
};

int fxos_enable(struct i2c_dev *dev);

#define FXOS_DRDY_TRIGGER TRIG_LOW

int fxos_read_state(struct i2c_dev *dev, struct fxos_state *state);

#endif
