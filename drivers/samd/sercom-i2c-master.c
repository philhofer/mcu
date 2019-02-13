#include <arch.h>
#include <bits.h>
#include <error.h>
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

/* type of opaque 'driver' pointer */
struct sercom_i2c_config {
	sercom_t base;
	ulong    baud;
	u8       pad0;
	u8       pad1;
	u8       irq;
};

static inline struct sercom_i2c_config *
bus_config(struct i2c_bus *bus)
{
	return (struct sercom_i2c_config *)bus->driver;
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
bus_disable_irq(struct i2c_bus *bus)
{
	struct sercom_i2c_config *conf = bus_config(bus);

	write8(intenclr_reg(conf->base), INT_ERR|INT_SB|INT_MB);
	irq_disable_num(conf->irq);
}

static void
bus_enable_irq(struct i2c_bus *bus)
{
	struct sercom_i2c_config *conf = bus_config(bus);

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
set_baud(const sercom_t sc, u8 baud, u8 baudlo, u8 hsbaud, u8 hsbaudlo)
{
	u32 rv = 0;
	ulong reg = baud_reg(sc);

	rv = (u32)baud;
	rv |= (u32)baudlo << 8;
	rv |= (u32)hsbaud << 16;
	rv |= (u32)hsbaudlo << 24;

	write32(reg, rv);
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

static int
sercom_i2c_init(struct i2c_bus *bus)
{
	u32 reg;
	sercom_t sc = bus_config(bus)->base;

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

	/* TODO: set up pins/ports for 10k or 2.2k pull-up */
	/* TODO: support baud rates other than 100kHz and 400kHz */
	/* TODO: set config->baud */
	set_baud(sc, 0, 0, 0, 0);
	
	sercom_enable(sc);

	/* force the bus state to the idle state */
	write16(status_reg(sc), BUS_IDLE);
	opwait(sc);

	return 0;
}

/* driver start() routine */
static int
sercom_i2c_start(struct i2c_bus *bus, i8 addr, struct mbuf *bufs, int nbufs, void (*done)(int, void *), void *uctx)
{
	sercom_t sc = bus_config(bus)->base;

	/* bus must be available */
	if (bus->tx.status != I2C_STATUS_NONE || read16(status_reg(sc)) != BUS_IDLE)
		return -EAGAIN;
	if (nbufs == 0)
		return 0;

	bus->tx.status = I2C_STATUS_MASTER;

	/* low bit of ADDR reg indicates read;
	 * see if we get an ACK or NACK from sending
	 * the address bits */
	write16(addr_reg(sc), ((u16)addr << 1) | !!(bufs[0].dir == IO_IN));
	opwait(sc);

	/* ensure the internal state of the module is
	 * arranged before enabling interrupts; otherwise
	 * an early interrupt arrival would see strange
	 * internal state */
	bus->tx.done = done;
	bus->tx.uctx = uctx;
	bus->tx.bufs = bufs;
	bus->tx.nbufs = nbufs;
	bus->tx.addr = addr;
	bus_enable_irq(bus);
	return 0;	
}

static void
stop(const sercom_t sc)
{
	write32(sercom_ctrlb(sc), CTRLB_STOP);
	opwait(sc);
}

static void
tx_complete(struct i2c_bus *bus, int err)
{
	sercom_t sc = bus_config(bus)->base;
	void (*cb)(int, void *);
	void *uctx;

	if (read16(status_reg(sc))&STATUS_CLKHOLD)
		stop(sc);
	bus_disable_irq(bus);

	cb = bus->tx.done;
	uctx = bus->tx.uctx;

	bus->tx = (struct i2c_tx){ 0 };

	/* the tx complete callback must be the very last
	 * thing that happens on completion, because it may
	 * arrange for another transaction to execute */
	if (cb)
		cb(err, uctx);
}

/* driver interrupt() routine */
static void
sercom_i2c_irq(struct i2c_bus *bus)
{
	sercom_t sc = bus_config(bus)->base;
	u16 status;
	u8 iflags;
	int err;

	iflags = read8(intflag_reg(sc));
	status = read16(status_reg(sc));

	/* spurious? */
	if (iflags == 0 || bus->tx.status == I2C_STATUS_NONE || bus->tx.nbufs == 0)
		return;

	/* bus error */
	if (status && (err = status_err(status)) < 0) {
		tx_complete(bus, err);
		return;
	}

	/* find the next buffer pointer to send/receive */
	while (buf_ateof(bus->tx.bufs)) {
		if (--bus->tx.nbufs == 0) {
			/* if we were transmitting, then we're done;
			 * we shouldn't get here in the rx path... */
			stop(sc);
			tx_complete(bus, 0);
			return;
		}
		/* if the next buffer isn't going in the same direction as the previous
		 * one, then we need to generate a repeated start */
		if (bus->tx.bufs[0].dir != bus->tx.bufs[1].dir) {
			write16(addr_reg(sc), ((u16)bus->tx.addr << 1) | !!(bus->tx.bufs[1].dir == IO_IN));
			bus->tx.bufs++;
			opwait(sc);
			return;
		}
		bus->tx.bufs++;
	}

	if ((iflags & INT_MB) && bus->tx.bufs[0].dir == IO_OUT) {
		write16(data_reg(sc), (u16)buf_getc(bus->tx.bufs));
	} else if ((iflags & INT_SB) && bus->tx.bufs[0].dir == IO_IN) {
		/* if this is the last byte of data, send a NACK when we read it */
		if (buf_todo(bus->tx.bufs) == 1 && bus->tx.nbufs == 1) {
			write32(sercom_ctrlb(sc), CTRLB_ACKA);
			opwait(sc);
		}
		buf_putc(bus->tx.bufs, (u8)read16(data_reg(sc)));
	}
	opwait(sc);
}

const struct i2c_bus_ops sercom_i2c_bus_ops = {
	.init = sercom_i2c_init,
	.start = sercom_i2c_start,
	.interrupt = sercom_i2c_irq,
};

