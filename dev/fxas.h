#ifndef __DEV_FXAS_H_
#define __DEV_FXAS_H_
#include <gyro.h>

/* wrapper code for
 * NXP FXAS21002CQ MEMS gyroscope */

struct fxas_state {
	struct gyro_state gyro;
	u64               last_update;
	int               last_err;
	uchar             buf[6];
};

int fxas_enable(struct i2c_dev *dev);

int fxas_read_state(struct i2c_dev *dev, struct fxas_state *state);

/* fxas_attach_drdy() tells the device to output the 'data ready'
 * interrupt on the INT2 device pin, and then calls extirq_configure()
 * to configure the external IRQ to trigger on the falling edge of
 * the given pin. (By default, the data ready interrupt is active-low,
 * so the interrupt is set up as a level-triggered interrupt on the
 * low signal.)
 */
int fxas_attach_drdy(struct i2c_dev *dev, unsigned pin);

#endif
