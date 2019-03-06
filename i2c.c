#include <bits.h>
#include <arch.h>
#include <error.h>
#include <i2c.h>
#include <drivers/i2c.h>

int
i2c_write_reg(struct i2c_dev *dev, i8 addr, u8 reg, void *buf, unsigned buflen,
		void (*done)(int, void *), void *uctx)
{
	if (dev->tx.state != I2C_STATE_NONE)
		return -EAGAIN;
	dev->tx.buf = buf;
	dev->tx.ackd = 0;
	dev->tx.buflen = buflen;
	dev->tx.done = done;
	dev->tx.uctx = uctx;
	dev->tx.addr = addr;
	dev->tx.reg = reg;

	return dev->ops->start(dev, I2C_STATE_SEND_REG_WRITE);
}

int
i2c_read_reg(struct i2c_dev *dev, i8 addr, u8 reg,
		void *buf, unsigned buflen,
		void (*done)(int, void *), void *uctx)
{
	if (dev->tx.state != I2C_STATE_NONE)
		return -EAGAIN;
	dev->tx.buf = buf;
	dev->tx.ackd = 0;
	dev->tx.buflen = buflen;
	dev->tx.done = done;
	dev->tx.uctx = uctx;
	dev->tx.addr = addr;
	dev->tx.reg = reg;

	return dev->ops->start(dev, I2C_STATE_SEND_REG_READ);
}

/* callback from interrupt context */
static void
op_done(int err, void *ctx)
{
	int *p = ctx;
	p[0] = 1;
	p[1] = err;
}

/* synchronous I2C register read implemented
 * using the asynchronous API and polling */
int
i2c_read_sync(struct i2c_dev *dev, i8 addr, u8 reg, void *dst, ulong dstlen)
{
	int status[2] = {0, 0}; /* status[0] = done; status[1] = err */
	int err;

	if ((err = i2c_read_reg(dev, addr, reg, dst, dstlen, op_done, status)) != 0)
		return err;	

	while (!status[0])
		idle_step(true);
	return status[1];
}

/* synchronous I2C write transaction implemented
 * using the asynchronous API and polling */
int
i2c_write_sync(struct i2c_dev *dev, i8 addr, u8 reg, void *data, ulong size)
{
	int status[2] = {0, 0};
	int err;

	if ((err = i2c_write_reg(dev, addr, reg, data, size, op_done, status)) != 0)
		return err;

	while (!status[0])
		idle_step(true);
	return status[1];
}

int
i2c_dev_reset(struct i2c_dev *dev, int flags)
{
	return dev->ops->reset(dev, flags);
}
