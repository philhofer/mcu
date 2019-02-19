#include <bits.h>
#include <arch.h>
#include <error.h>
#include <buffer.h>
#include <i2c.h>
#include <drivers/i2c.h>

/* i2c_start_iov() is essentially a direct invocation
 * of the driver code with some additionnal sanity checking;
 * the only abstraction here is the bus-number-to-driver indirection */
int
i2c_start_iov(struct i2c_dev *dev, i8 addr, struct mbuf *bufs, int nbufs, void (*done)(int, void *), void *uctx)
{
	return dev->ops->start(dev, addr, bufs, nbufs, done, uctx);
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
	struct mbuf bufs[2];
	int status[2] = {0, 0}; /* status[0] = done; status[1] = err */
	int err;

	buf_init_out(&bufs[0], &reg, sizeof(reg));
	buf_init_in(&bufs[1], dst, dstlen);

	if ((err = i2c_start_iov(dev, addr, bufs, 2, op_done, status)) != 0)
		return err;	

	while (!status[0])
		idle_step(true);
	return status[1];
}

/* synchronous I2C write transaction implemented
 * using the asynchronous API and polling */
int
i2c_write_sync(struct i2c_dev *dev, i8 addr, void *data, ulong size)
{
	struct mbuf bufs[1];
	int status[2] = {0, 0};
	int err;

	buf_init_out(&bufs[0], data, size);

	if ((err = i2c_start_iov(dev, addr, bufs, 1, op_done, status)) != 0)
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
