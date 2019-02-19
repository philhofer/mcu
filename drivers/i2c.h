#ifndef __I2C_DRIVER_H_
#define __I2C_DRIVER_H_
#include <bits.h>
#include <error.h>
#include <i2c.h>

/* i2c_bus_ops is the set of operations
 * implemented by an I2C driver */
struct i2c_bus_ops {
	/* reset() should perform driver-specific
	 * initialization */
	int (*reset)(struct i2c_dev *dev, int flags);

	/* start() should perform a (possibly combined)
	 * transaction with the given set of buffers, and
	 * then it should call done(err, uctx) from interrupt
	 * context when the operation has completed or an error
 	 * is encountered. */
	int (*start)(struct i2c_dev *dev, i8 addr, struct mbuf *bufs, int nbufs, void (*done)(int, void *), void *uctx);
};

#endif
