#ifndef __DEV_FXAS_H_
#define __DEV_FXAS_H_
#include <gyro.h>
#include <extirq.h>

/* wrapper code for
 * NXP FXAS21002CQ MEMS gyroscope */

struct fxas_state {
	struct gyro_state gyro;
	void              (*on_update)(void);
	int               last_err;
	uchar             buf[6];
};

int fxas_enable(struct i2c_dev *dev);

int fxas_read_state(struct i2c_dev *dev, struct fxas_state *state);

#define FXAS_DRDY_TRIGGER TRIG_LOW

#endif
