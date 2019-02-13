#ifndef __I2C_DRIVER_H_
#define __I2C_DRIVER_H_
#include <bits.h>
#include <error.h>
#include <i2c.h>

struct i2c_bus;

extern struct i2c_bus _si2cdriver;
extern struct i2c_bus _ei2cdriver;

/* i2c_bus_ops is the set of operations
 * implemented by an I2C driver */
struct i2c_bus_ops {
	/* init() should perform driver-specific
	 * initialization */
	int (*init)(struct i2c_bus *bus);

	/* start() should perform a (possibly combined)
	 * transaction with the given set of buffers, and
	 * then it should call done(err, uctx) from interrupt
	 * context when the operation has completed or an error
 	 * is encountered. */
	int (*start)(struct i2c_bus *bus, i8 addr, struct mbuf *bufs, int nbufs, void (*done)(int, void *), void *uctx);

	/* interrupt() is the interrupt handler */
	void (*interrupt)(struct i2c_bus *bus);
};

/* state for a specific bus */
struct i2c_bus {
	const struct i2c_bus_ops *ops;    /* driver functions */
	const void               *driver; /* actual driver instance */
	struct i2c_tx             tx;     /* current state */
};

static inline struct i2c_bus *i2c_get_bus(int num) {
	struct i2c_bus *out;

	out = (&_si2cdriver) + num;
	if (out >= &_ei2cdriver)
		return NULL;
	return out;	
}

#define DECLARE_I2C_BUS(num, drv, udata)     \
__attribute__((section(".i2cdriver." #num))) \
struct i2c_bus i2c_bus_##num = {	     \
	.ops = drv, 			     \
	.driver = (void *)udata,	     \
}

#endif
