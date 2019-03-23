#ifndef __I2C_DRIVER_H_
#define __I2C_DRIVER_H_
#include <bits.h>
#include <error.h>
#include <i2c.h>

/* mini i2c master state machine */
#define I2C_STATE_NONE           0 /* none -> SEND_REG_READ, -> SEND_REG_WRITE */
#define I2C_STATE_SEND_REG_READ  1 /* -> RSTART_READ */
#define I2C_STATE_SEND_REG_WRITE 2 /* -> SEND_DATA, -> DONE */
#define I2C_STATE_RSTART_READ    3 /* -> RECV_DATA */
#define I2C_STATE_SEND_DATA      4 /* -> SEND_DATA, -> DONE */
#define I2C_STATE_RECV_DATA      5 /* -> RECV_DATA, -> none */
#define I2C_STATE_DONE           6 /* -> none */

/* i2c_bus_ops is the set of operations
 * implemented by an I2C driver */
struct i2c_bus_ops {
	/* reset() should perform driver-specific
	 * initialization */
	int (*reset)(struct i2c_dev *dev, int flags);

	int (*start)(struct i2c_dev *dev, u8 state);
};

static inline bool
i2c_tx_needs_nack(struct i2c_tx *tx)
{
	return tx->ackd == tx->buflen-1;
}

static inline bool
i2c_tx_has_more(struct i2c_tx *tx)
{
	return tx->ackd < tx->buflen;
}

static inline void
i2c_tx_done(struct i2c_tx *tx, int err)
{
	void (*cb)(int, void *);
	void *uctx;

	uctx = tx->uctx;
	cb = tx->done;
	tx->uctx = NULL;
	tx->done = NULL;
	tx->addr = 0;
	tx->reg = 0;
	tx->state = I2C_STATE_NONE;
	cb(err, uctx);
}

static inline u8
i2c_tx_pop_byte(struct i2c_tx *tx)
{
	assert(tx->ackd < tx->buflen);
	return tx->buf[tx->ackd++];
}

static inline void
i2c_tx_push_byte(struct i2c_tx *tx, u8 byte)
{
	assert(tx->ackd < tx->buflen);
	tx->buf[tx->ackd++] = byte;
}

#endif
