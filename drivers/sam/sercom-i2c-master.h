#ifndef __SAM_SERCOM_I2C_MASTER_H_
#define __SAM_SERCOM_I2C_MASTER_H_
#include <drivers/sam/sercom.h>
#include <i2c.h>

extern const struct i2c_bus_ops sercom_i2c_bus_ops;

void sercom_i2c_master_irq(struct i2c_dev *dev);

#define SERCOM_I2C_MASTER(name, num) \
	struct i2c_dev name = { .driver = &sercoms[num], .ops = &sercom_i2c_bus_ops }

#endif
