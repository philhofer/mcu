#include <arch.h>
#include <bits.h>
#include <error.h>
#include <config.h>
#include <i2c.h>
#include <kernel/i2c.h>
#include <kernel/sam/sercom-i2c-master.h>
#include "sercom.h"

/* register descriptions start
 * in section 28.10 of Atmel's
 * 'SAMD21 Datasheet' */

#define CTRLA_PINOUT   (1 << 8)  /* enable 4-wire mode */
#define CTRLA_SDAHOLD75  (1 << 20);
#define CTRLA_SDAHOLD450 (2 << 20);
#define CTRLA_SDAHOLD600 (3 << 20);
#define CTRLA_MEXTTOEN (1 << 22) /* 'master SCL low extend timeout' */
#define CTRLA_SEXTTOEN (1 << 23) /* 'slave SCL low extend timeout' */
#define CTRLA_SCLSM    (1 << 27) /* SCL 'clock stretch mode' */
#define CTRLA_LOTOUTEN (1 << 30) /* SCL low timeout enable */

#define CTRLB_SMEN (1 << 8)  /* 'smart mode' enable (automatic ACK/NACK) */
#define CTRLB_QCEN (1 << 9)  /* 'quick command' enable */
#define CTRLB_ACKA (1 << 18) /* 'acknowledge action' for 'smart mode' */
#define CTRLB_STOP (3 << 16) /* two-bit command field: ack with stop bit */

/* status register bits */
#define STATUS_BUSERR  (1UL << 0)
#define STATUS_ARBLOST (1UL << 1)
#define STATUS_RXNACK  (1UL << 2)
#define STATUS_LOTOUT  (1UL << 6)
#define STATUS_CLKHOLD (1UL << 7)
#define STATUS_MEXTTOUT (1UL << 8)
#define STATUS_SEXTTOUT (1UL << 9)
#define STATUS_LENERR  (1UL << 10)

enum bus_state {
	BUS_UNKNOWN = 0x0 << 4,
	BUS_IDLE = 0x1 << 4,
	BUS_OWNER = 0x2 << 4,
	BUS_BUSY = 0x3 << 4,
};

/* syncbusy register bits */
#define SYNC_SYSOP  (1UL << 2)

/* addr register bits */
#define ADDR_LENEN  (1UL << 13)
#define ADDR_HS     (1UL << 14)
#define ADDR_TENBIT (1UL << 15)

/* int{enclr,enset,flag} register bits */
#define INT_MB  (1UL << 0) /* master-on-bus enable/disable/flag */
#define INT_SB  (1UL << 1) /* slave-on-bus enable/disable/flag */
#define INT_ERR (1UL << 7) /* error interrupt enable/disable/flag */

static inline const struct sercom_config *
dev_config(struct i2c_dev *dev)
{
	return (const struct sercom_config *)dev->driver;
}

/* INTENCLR:8 disables the INT_* interrupts when written to */
static ulong
intenclr_reg(const sercom_t sc)
{
	return sc + 0x14;
}

/* INTENSET:8 enables the INT_* interrupts when written to */
static ulong
intenset_reg(const sercom_t sc)
{
	return sc + 0x16;
}

/* INTFLAG:8 tests/sets the INT_* interrupt status when read/written to */
static ulong
intflag_reg(const sercom_t sc)
{
	return sc + 0x18;
}

static void
dev_disable_irq(struct i2c_dev *dev)
{
	const struct sercom_config *conf = dev_config(dev);

	write8(intenclr_reg(conf->base), INT_ERR|INT_SB|INT_MB);
	irq_disable_num(conf->irq);
}

static void
dev_enable_irq(struct i2c_dev *dev)
{
	const struct sercom_config *conf = dev_config(dev);

	write8(intflag_reg(conf->base), INT_ERR|INT_SB|INT_MB);
	write8(intenset_reg(conf->base), INT_ERR|INT_SB|INT_MB);
	irq_enable_num(conf->irq);
}

/* 32-bit register address */
static ulong
baud_reg(const sercom_t sc)
{
	return sc + 0x0C;
}

/* 16-bit register address */
static ulong
status_reg(const sercom_t sc)
{
	return sc + 0x1A;
}

/* 32-bit register address */
static ulong
addr_reg(const sercom_t sc)
{
	return sc + 0x24;
}

/* 16-bit register address */
static ulong
data_reg(const sercom_t sc)
{
	return sc + 0x28;
}

static void
opwait(const sercom_t sc)
{
	while (read32(sercom_syncbusy(sc))&SYNC_SYSOP) ;
}

/* status_err() converts the status
 * register to an error code */
static int
status_err(u16 status, u8 iflags)
{
	if (status&(STATUS_BUSERR|STATUS_LENERR|STATUS_RXNACK))
		return -EIO;

	/* another master appeared on the bus */
	if (status&STATUS_ARBLOST)
		return -EAGAIN;

	/* some protocol-level timeout */
	if (status&(STATUS_LOTOUT|STATUS_MEXTTOUT|STATUS_SEXTTOUT))
		return -ETIMEDOUT;

	/* not sure we should ever get here */
	if (iflags&INT_ERR)
		return -EIO;

	return 0;
}

/* rise times, in nanoseconds,
 * of the bus given the configuration
 * see table 37-69 in Electrical Characteristics
 * in the datasheet */
#define RISE_TIME_SLOW 230 /* assuming Cb = 400pF */
#define RISE_TIME_FAST 60  /* assuminb Cb = 500pF */
#define RISE_TIME_HSM  30  /* assuming Cb = 100pF */

/* fSCL = fGCLK / (10 + 2*baud + fGCLK * rT)
 * therefore
 * 2*baud = (fGCLK/fSCL) - (fGCLK * rT) - 10 */
static inline int
baud_period(ulong core, ulong rise, ulong target)
{
	/* n.b.: we pre-scale the core speed here so that
	 * it's less likely that the rise time times the
	 * core clock leads to an overflow */
	return (core/target) - (((core/1000)*rise)/1e6) - 10;
}

/* FIXME: this isn't true if someone has gone
 * and mucked with GCLK_MAIN, which uses
 * the same clock generator that we point at
 * the sercom devices by default... */
#define get_core_clock() CPU_HZ

static u32
calc_baud(struct i2c_dev *dev, unsigned speed)
{
	u32 reg = 0;

	/* TODO: add error handling for higher speeds here;
	 * the theoretical limit to the baud rate generator
	 * is GCLK divided by ten, so there are speeds that
	 * we may not be able to generate in certain clock
	 * configurations */
	switch (speed) {
	case I2C_SPEED_NORMAL:
		reg = (u8)(baud_period(get_core_clock(), RISE_TIME_SLOW, 100000))/2;
		break;
	case I2C_SPEED_FULL:
		reg = (u8)(baud_period(get_core_clock(), RISE_TIME_SLOW, 400000))/2;
		break;
	case I2C_SPEED_FAST:
		/* TODO */
		break;
	case I2C_SPEED_HIGH:
		/* TODO */
		break;
	}
	return reg;
}

static int
sercom_i2c_init(struct i2c_dev *dev, int flags)
{
	u32 reg;
	sercom_t sc = dev_config(dev)->base;

	if (dev->tx.state != I2C_STATE_NONE)
		return -EAGAIN;

	sercom_reset(dev_config(dev));

	/* 'smart mode' means that master read operations
	 * will automatically send an ACK/NACK bit, which
	 * saves a little bit of code on our end */
	reg = CTRLB_SMEN;
	write32(sercom_ctrlb(sc), reg);

	/* Enabling timeouts seems like a sensible default,
	 * since returning an error is friendlier behavior than
	 * locking up the I2C bus.
	 * We're implicitly setting the 'speed' bits to 0,
	 * which works for 100kHz (Sm) and 400kHz (Fm) operation. */
	reg = CTRLA_SEXTTOEN|CTRLA_MEXTTOEN|CTRLA_LOTOUTEN;
	reg |= CTRLA_SDAHOLD75; /* TODO: configurable? */
	reg |= CTRLA_RUNSTBY;
	reg |= SERCOM_I2C_MASTER;
	write32(sercom_ctrla(sc), reg);

	/* TODO: support baud rates other than 100kHz and 400kHz */
	reg = calc_baud(dev, flags&3);
	if (!reg)
		return -EOPNOTSUPP;
	write32(baud_reg(sc), reg);
	opwait(sc);
	
	sercom_enable(sc);

	/* force the bus state to the idle state */
	write16(status_reg(sc), BUS_IDLE);
	opwait(sc);

	return 0;
}

static void
clear_nack(sercom_t sc)
{
	/* just clear the upper half of the register */
	write16(sercom_ctrlb(sc)+2, 0);
	opwait(sc);
}

static unsigned
bus_state(sercom_t sc)
{
	return read16(status_reg(sc))&BUS_OWNER;
}

static void
stop(sercom_t sc);

/* driver start() routine */
static int
sercom_i2c_start(struct i2c_dev *dev, u8 state)
{
	sercom_t sc = dev_config(dev)->base;

	/* bus must be available */
	if (dev->tx.state != I2C_STATE_NONE)
		return -EAGAIN;

	if (state != I2C_STATE_SEND_REG_READ && state != I2C_STATE_SEND_REG_WRITE)
		return -EINVAL;

	clear_nack(sc);

	/* technically we want the bus state
	 * to be IDLE here, but, unlike what the
	 * datasheet says, we end up in OWNED rather
	 * than IDLE after we send a stop condition...? */
	if (bus_state(sc) == BUS_BUSY)
		return -EAGAIN;

	/* we always start with a write transaction,
	 * since we need to write the address register */
	dev->tx.state = state;
	write16(addr_reg(sc), (u16)dev->tx.addr << 1);
	opwait(sc);

	/* ensure the internal state of the module is
	 * arranged before enabling interrupts; otherwise
	 * an early interrupt arrival would see strange
	 * internal state */
	dev_enable_irq(dev);
	return 0;	
}

static void
stop(const sercom_t sc)
{
	write16(sercom_ctrlb(sc)+2, (CTRLB_ACKA|CTRLB_STOP)>>16);
	opwait(sc);
}

static void
tx_complete(struct i2c_dev *dev, int err)
{
	sercom_t sc = dev_config(dev)->base;

	opwait(sc);
	stop(sc);
	dev_disable_irq(dev);
	i2c_tx_done(&dev->tx, err);
}

/* state-transition functions for
 * each of the interrupt flags */
static u8 sercom_i2c_mb_step(struct i2c_dev *dev);
static u8 sercom_i2c_sb_step(struct i2c_dev *dev);

void
sercom_i2c_master_irq(struct i2c_dev *dev)
{
	sercom_t sc = dev_config(dev)->base;
	u16 status;
	u8 iflags;
	int err;

	iflags = read8(intflag_reg(sc));
	status = read16(status_reg(sc));

	/* spurious? */
	if (iflags == 0 || dev->tx.state == I2C_STATE_NONE)
		return;

	/* dev error */
	if (status && (err = status_err(status, iflags)) < 0) {
		tx_complete(dev, err);
		return;
	}

	/* ensure that we actually handle
	 * each interrupt by asserting that
	 * the flag cleared when the handlers
	 * return */
	if (iflags & INT_MB)
		dev->tx.state = sercom_i2c_mb_step(dev);
	else if (iflags & INT_SB)
		dev->tx.state = sercom_i2c_sb_step(dev);

	if (dev->tx.state == I2C_STATE_NONE) {
		tx_complete(dev, 0);
		return;
	}
	opwait(sc);
	if (iflags&INT_MB)
		assert(!(read8(intflag_reg(sc))&INT_MB));
	if (iflags&INT_SB)
		assert(!(read8(intflag_reg(sc))&INT_SB));
}

static u8
sercom_i2c_mb_step(struct i2c_dev *dev)
{
	sercom_t sc = dev_config(dev)->base;

	switch (dev->tx.state) {
	case I2C_STATE_SEND_REG_READ:
		write16(data_reg(sc), (u16)dev->tx.reg);
		return I2C_STATE_RSTART_READ;
	case I2C_STATE_SEND_REG_WRITE:
		write16(data_reg(sc), (u16)dev->tx.reg);
		return i2c_tx_has_more(&dev->tx) ?
			I2C_STATE_SEND_DATA : I2C_STATE_DONE;
	case I2C_STATE_RSTART_READ:
		/* weird special-case: 0-byte read?
		 * this leads us to send a STOP immediately,
		 * which is wrong, but 0-byte reads have
		 * unclear semantics anyway... */
		assert(i2c_tx_has_more(&dev->tx));
		write16(addr_reg(sc), ((u16)dev->tx.addr << 1)|1);
		return I2C_STATE_RECV_DATA;
	case I2C_STATE_SEND_DATA:
		write16(data_reg(sc), (u16)i2c_tx_pop_byte(&dev->tx));
		return i2c_tx_has_more(&dev->tx) ?
			I2C_STATE_SEND_DATA : I2C_STATE_DONE;
	case I2C_STATE_DONE:
		/* last written byte was ACK'd */
		return I2C_STATE_NONE;
	}
	assert(false && "unknown i2c state");
	return I2C_STATE_NONE;
}

static u8
sercom_i2c_sb_step(struct i2c_dev *dev)
{
	sercom_t sc = dev_config(dev)->base;

	/* there is input data to read */
	switch (dev->tx.state) {
	case I2C_STATE_RECV_DATA:
		if (i2c_tx_needs_nack(&dev->tx))
			stop(sc);
		i2c_tx_push_byte(&dev->tx, (u8)read16(data_reg(sc)));
		if (!i2c_tx_has_more(&dev->tx))
			return I2C_STATE_NONE;
		return I2C_STATE_RECV_DATA;
	}
	assert(false && "unknown i2c state");
	return I2C_STATE_NONE;
}

const struct i2c_bus_ops sercom_i2c_bus_ops = {
	.reset = sercom_i2c_init,
	.start = sercom_i2c_start,
};

