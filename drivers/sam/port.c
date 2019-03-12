#include <bits.h>
#include <arch.h>
#include <board.h>
#include <error.h>
#include <gpio.h>
#include <drivers/gpio.h>
#include <drivers/sam/port.h>

#define PORT_DIR      0x0
#define PORT_DIRCLR   0x4
#define PORT_DIRSET   0x8
#define PORT_DIRTGL   0xC
#define PORT_OUT      0x10
#define PORT_OUTCLR   0x14
#define PORT_OUTSET   0x18
#define PORT_OUTTGL   0x1C
#define PORT_IN       0x20
#define PORT_CTRL     0x24
#define PORT_WRCONFIG 0x28

#define PINCFG_MUXEN  0x1
#define PINCFG_INEN   0x2
#define PINCFG_PULLEN 0x3

struct portpin {
	u8 grp;
	u8 num;
};

static inline ulong
port_pmux_reg4(u8 group, unsigned pin)
{
	/* PMUX regs are packed 4-bit fields starting at 0x30 */
	return PORT_BASE + (0x80 * group) + 0x30 + (pin >> 1);
}

static inline void
setbit(unsigned reg, u8 group, unsigned pin)
{
	u32 v = 1UL << pin;
	write32(PORT_BASE + (0x80 * group) + reg, v);
}

static inline int
getbit(unsigned reg, u8 group, unsigned pin)
{
	return (read32(PORT_BASE + (0x80 * group) + reg) >> pin) & 1;
}

static inline ulong
port_pincfg_reg8(u8 group, unsigned pin)
{
	/* PINCFG regs are packed 8-bit fields starting at 0x40 */
	return PORT_BASE + (0x80 * group) + 0x40 + pin;
}

int
port_pmux_pin(unsigned group, unsigned pin, uchar role)
{
	ulong reg;
	u8 mask;

	if ((role&0xf) != role)
		return -EINVAL;

	/* even-numbered pins are the low 4 bits;
	 * odd-numbered pins are the upper 4 bits;
	 * we need to do a read-modify-write to actually
	 * set only the bits we need */
	mask = 0xf0;	
	if (pin&1) {
		role <<= 4;
		mask >>= 4;
	}
	reg = port_pmux_reg4(group, pin);
	write8(reg, (read8(reg)&mask)|role);

	/* turn it on */
	write8(port_pincfg_reg8(group, pin), PINCFG_MUXEN);
	return 0;
}

static unsigned
gp_pinnum(const struct gpio *gp)
{
	return PORT_NUM(gp->driver.number);
}

static unsigned
gp_pingrp(const struct gpio *gp)
{
	return PORT_GROUP(gp->driver.number);
}

static int
port_reset(const struct gpio *gp, int flags)
{
	unsigned pnum, pgrp;
	u8 cfg, wantcfg;

	pnum = gp_pinnum(gp);
	pgrp = gp_pingrp(gp);

	/* not a realistic configuration */
	if ((flags&(GPIO_PULLUP|GPIO_PULLDOWN)) == (GPIO_PULLUP|GPIO_PULLDOWN))
		return -EINVAL;

	/* guard against clobbering a peripheral */
	cfg = read8(port_pincfg_reg8(pgrp, pnum));
	if (cfg&PINCFG_MUXEN)
		return -EINVAL;

	wantcfg = 0;
	if (flags&GPIO_IN)
		wantcfg |= PINCFG_INEN;
	if (flags&(GPIO_PULLUP|GPIO_PULLDOWN))
		wantcfg |= PINCFG_PULLEN;

	/* don't perform unnecessary pin configuration
	 * if we're just switching pin directions */
	if (cfg != wantcfg)
		write8(port_pincfg_reg8(pgrp, pnum), cfg);

	if (flags&GPIO_OUT)
		setbit(PORT_DIRSET, pgrp, pnum);
	else if (flags&GPIO_IN)
		setbit(PORT_DIRCLR, pgrp, pnum);
	if (flags&GPIO_PULLUP)
		setbit(PORT_OUTSET, pgrp, pnum);
	else if (flags&GPIO_PULLDOWN)
		setbit(PORT_OUTCLR, pgrp, pnum);
	return 0;
}

static int
port_read(const struct gpio *gp)
{
	unsigned pnum, pgrp;

	pnum = gp_pinnum(gp);
	pgrp = gp_pingrp(gp);
	return getbit(PORT_IN, pgrp, pnum);

}

static int
port_write(const struct gpio *gp, int up)
{
	unsigned pnum, pgrp;

	pnum = gp_pinnum(gp);
	pgrp = gp_pingrp(gp);

	setbit(up ? PORT_OUTSET : PORT_OUTCLR, pgrp, pnum);
	return !!up;
}

static int
port_toggle(const struct gpio *gp)
{
	unsigned pnum, pgrp;

	pnum = gp_pinnum(gp);
	pgrp = gp_pingrp(gp);

	setbit(PORT_OUTTGL, pgrp, pnum);
	return getbit(PORT_OUT, pgrp, pnum);
}

const struct gpio_ops port_gpio_ops = {
	.reset = port_reset,
	.set = port_write,
	.get = port_read,
	.toggle = port_toggle,
};

unsigned
port_eic_num(unsigned group, unsigned num)
{
	/* for the SAMD21 at least, the EIC line
	 * is just the lowest 4 bits of the pin
	 * number, EXCEPT for the SWCLK/SWIO pins
	 * and the USB DM/DP pins, but who is
	 * using those for generating interrupts...?
	 * TODO: double-check this is true on other SAM boards */
	return (num & 0xf);
}
