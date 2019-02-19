#ifndef __I2C_H_
#define __I2C_H_
#include <bits.h>
#include <buffer.h>
#include <idle.h>

enum i2c_status {
	I2C_STATUS_NONE,
	I2C_STATUS_MASTER,
	I2C_STATUS_DONE,
	I2C_STATUS_ERR,
};

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
	struct mbuf     *bufs;   /* buffers to be read/written as a combined transaction */
	int              nbufs;  /* total number of buffers */
	void           (*done)(int err, void *ctx);
	void            *uctx; 
	enum i2c_status  status; /* current status */
	i8               addr;   /* target address for transaction */
};

struct i2c_dev {
	const struct i2c_bus_ops *ops;    /* driver functions */
	const void               *driver; /* actual driver instance */
	struct i2c_tx             tx;     /* current state */
};

/* i2c_start_iov() begins an I2C transaction with the given buffers
 * and calls the done() callback from interrupt context when the
 * transaction is complete or an error is encountered.
 * -EAGAIN will be returned immediately if the bus is not available
 * for transactions. */
int i2c_start_iov(struct i2c_dev *dev,
		i8 addr, struct mbuf *bufs, int nbufs,
		void (*done)(int, void *), void *ctx);

/* i2c_read_sync() performs a synchronous I2C read transaction on the
 * given bus. */
int i2c_read_sync(struct i2c_dev *dev, i8 addr, u8 reg, void *dst, ulong dstlen);

/* i2c_write_sync() performs a synchronous I2C write transaction
 * on the given bus. */
int i2c_write_sync(struct i2c_dev *dev, i8 addr, void *src, ulong srclen);

/* i2c_bus_state() returns the current state of the I2C bus. */
static inline enum i2c_status
i2c_dev_state(struct i2c_dev *dev) {
	return dev->tx.status;
}

/* i2c_dev_reset() performs hardware- and software-dependent
 * bus initialization code for the given I2C bus.
 * 'flags' should be the bitwise-OR of the necessary
 * I2C_* flag constants. */
int i2c_dev_reset(struct i2c_dev *dev, int flags);

#endif
