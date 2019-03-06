#ifndef __I2C_H_
#define __I2C_H_
#include <bits.h>
#include <idle.h>

/* flags for i2c_dev_reset() */
#define I2C_SPEED_NORMAL 0          /* 100 kbps */
#define I2C_SPEED_FULL   (1UL << 0) /* 400 kbps */
#define I2C_SPEED_FAST   (1UL << 1) /* 1 Mbps */
#define I2C_SPEED_HIGH   (3UL << 1) /* 3.4 Mbps */

struct i2c_bus_ops;
struct i2c_tx;

/* bus state
 * NOT for end-user consumption; please ignore this */
struct i2c_tx {
	uchar           *buf;
	unsigned         ackd;
	unsigned         buflen;
	void           (*done)(int err, void *ctx);
	void            *uctx;
	i8               addr;  /* address for transaction */
	u8               reg;   /* register to read/write */
	u8               state; /* current state in state machine */
};

struct i2c_dev {
	const struct i2c_bus_ops *ops;    /* driver functions */
	const void               *driver; /* actual driver instance */
	struct i2c_tx             tx;     /* current state */
};

int i2c_write_reg(struct i2c_dev *dev,
		i8 addr,
		u8 reg,
		void *buf,
		unsigned len,
		void (*done)(int, void *), void *ctx);

int i2c_read_reg(struct i2c_dev *dev,
		i8 addr,
		u8 reg,
		void *buf,
		unsigned len,
		void (*done)(int, void *), void *ctx);

/* i2c_read_sync() performs a synchronous I2C read transaction on the
 * given bus. */
int i2c_read_sync(struct i2c_dev *dev, i8 addr, u8 reg, void *dst, ulong dstlen);

/* i2c_write_sync() performs a synchronous I2C write transaction
 * on the given bus. */
int i2c_write_sync(struct i2c_dev *dev, i8 addr, u8 reg, void *src, ulong srclen);

/* i2c_dev_reset() performs hardware- and software-dependent
 * bus initialization code for the given I2C bus.
 * 'flags' should be the bitwise-OR of the necessary
 * I2C_* flag constants. */
int i2c_dev_reset(struct i2c_dev *dev, int flags);

#endif
