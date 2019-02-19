#include <arch.h>
#include <bits.h>
#include <error.h>
#include <config.h>
#include <i2c.h>
#include <drivers/i2c.h>
#include "sercom.h"

/* register descriptions start
 * in section 28.10 of Atmel's
 * 'SAMD21 Datasheet' */

#define CTRLA_PINOUT   (1 << 8)  /* enable 4-wire mode */
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
status_err(u16 status)
{
	if (status&(STATUS_BUSERR|STATUS_LENERR|STATUS_RXNACK))
		return -EIO;

	/* another master appeared on the bus */
	if (status&STATUS_ARBLOST)
		return -EAGAIN;

	/* some protocol-level timeout */
	if (status&(STATUS_LOTOUT|STATUS_MEXTTOUT|STATUS_SEXTTOUT))
		return -ETIMEDOUT;

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

	if (dev->tx.status != I2C_STATUS_NONE)
		return -EAGAIN;

	sercom_reset(sc);

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
	reg |= CTRLA_RUNSTBY|CTRLA_SCLSM;
	reg |= SERCOM_I2C_MASTER;
	write32(sercom_ctrla(sc), reg);

	/* TODO: support baud rates other than 100kHz and 400kHz */
	reg = calc_baud(dev, flags&3);
	if (!reg)
		return -EOPNOTSUPP;
	write32(baud_reg(sc), reg);
	
	sercom_enable(sc);

	/* force the bus state to the idle state */
	write16(status_reg(sc), BUS_IDLE);
	opwait(sc);

	return 0;
}

/* driver start() routine */
static int
sercom_i2c_start(struct i2c_dev *dev, i8 addr, struct mbuf *bufs, int nbufs, void (*done)(int, void *), void *uctx)
{
	sercom_t sc = dev_config(dev)->base;

	/* bus must be available */
	if (dev->tx.status != I2C_STATUS_NONE || read16(status_reg(sc)) != BUS_IDLE)
		return -EAGAIN;
	if (nbufs == 0)
		return 0;

	dev->tx.status = I2C_STATUS_MASTER;

	/* low bit of ADDR reg indicates read;
	 * see if we get an ACK or NACK from sending
	 * the address bits */
	write16(addr_reg(sc), ((u16)addr << 1) | !!(bufs[0].dir == IO_IN));
	opwait(sc);

	/* ensure the internal state of the module is
	 * arranged before enabling interrupts; otherwise
	 * an early interrupt arrival would see strange
	 * internal state */
	dev->tx.done = done;
	dev->tx.uctx = uctx;
	dev->tx.bufs = bufs;
	dev->tx.nbufs = nbufs;
	dev->tx.addr = addr;
	dev_enable_irq(dev);
	return 0;	
}

static void
stop(const sercom_t sc)
{
	write32(sercom_ctrlb(sc), CTRLB_STOP);
	opwait(sc);
}

static void
tx_complete(struct i2c_dev *dev, int err)
{
	sercom_t sc = dev_config(dev)->base;
	void (*cb)(int, void *);
	void *uctx;

	if (read16(status_reg(sc))&STATUS_CLKHOLD)
		stop(sc);
	dev_disable_irq(dev);

	cb = dev->tx.done;
	uctx = dev->tx.uctx;

	dev->tx = (struct i2c_tx){ 0 };

	/* the tx complete callback must be the very last
	 * thing that happens on completion, because it may
	 * arrange for another transaction to execute */
	if (cb)
		cb(err, uctx);
}

/* driver interrupt() routine */
void
sercom_i2c_irq(struct i2c_dev *dev)
{
	sercom_t sc = dev_config(dev)->base;
	u16 status;
	u8 iflags;
	int err;

	iflags = read8(intflag_reg(sc));
	status = read16(status_reg(sc));

	/* spurious? */
	if (iflags == 0 || dev->tx.status == I2C_STATUS_NONE || dev->tx.nbufs == 0)
		return;

	/* dev error */
	if (status && (err = status_err(status)) < 0) {
		tx_complete(dev, err);
		return;
	}

	/* find the next buffer pointer to send/receive */
	while (buf_ateof(dev->tx.bufs)) {
		if (--dev->tx.nbufs == 0) {
			/* if we were transmitting, then we're done;
			 * we shouldn't get here in the rx path... */
			stop(sc);
			tx_complete(dev, 0);
			return;
		}
		/* if the next buffer isn't going in the same direction as the previous
		 * one, then we need to generate a repeated start */
		if (dev->tx.bufs[0].dir != dev->tx.bufs[1].dir) {
			write16(addr_reg(sc), ((u16)dev->tx.addr << 1) | !!(dev->tx.bufs[1].dir == IO_IN));
			dev->tx.bufs++;
			opwait(sc);
			return;
		}
		dev->tx.bufs++;
	}

	if ((iflags & INT_MB) && dev->tx.bufs[0].dir == IO_OUT) {
		write16(data_reg(sc), (u16)buf_getc(dev->tx.bufs));
	} else if ((iflags & INT_SB) && dev->tx.bufs[0].dir == IO_IN) {
		/* if this is the last byte of data, send a NACK when we read it */
		if (buf_todo(dev->tx.bufs) == 1 && dev->tx.nbufs == 1) {
			write32(sercom_ctrlb(sc), CTRLB_ACKA);
			opwait(sc);
		}
		buf_putc(dev->tx.bufs, (u8)read16(data_reg(sc)));
	}
	opwait(sc);
}

const struct i2c_bus_ops sercom_i2c_bus_ops = {
	.reset = sercom_i2c_init,
	.start = sercom_i2c_start,
};

