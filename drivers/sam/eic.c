#include <board.h>
#include <error.h>
#include <drivers/sam/port.h>
#include <drivers/sam/gclk.h>
#include <extirq.h>

#define EIC_CTRL     0x00
#define EIC_STATUS   0x01
#define EIC_NMICTRL  0x02
#define EIC_NMIFLAG  0x03
#define EIC_EVCTRL   0x04
#define EIC_INTENCLR 0x08
#define EIC_INTENSET 0x0C
#define EIC_INTFLAG  0x10
#define EIC_WAKEUP   0x14
#define EIC_CONFIG   0x18

#define CTRL_SWRST  (1U << 0)
#define CTRL_ENABLE (1U << 1)

#define STATUS_SYNCBUSY (1U << 7)

static void
eic_sync(void)
{
	while (read8(EIC_BASE + EIC_STATUS)&STATUS_SYNCBUSY)
		;
}

static void
eic_enable(void)
{
	write8(EIC_BASE + EIC_CTRL, CTRL_ENABLE);
	eic_sync();
}

static void 
eic_set_config(unsigned num, enum trigger trig)
{
	u32 reg;
	ulong addr;

	/* the CONFIGn registers are actually 8 4-bit
	 * registers, so we need to assemble that value
	 * and then shift it into position */
	reg = 0;
	switch (trig) {
	case TRIG_RISE:
		reg |= 1;
		break;
	case TRIG_FALL:
		reg |= 2;
		break;
	case TRIG_BOTH:
		reg |= 3;
		break;
	case TRIG_HIGH:
		reg |= 4;
		break;
	case TRIG_LOW:
		reg |= 5;
		break;
	default:
		assert(false && "bad external interrupt trigger mode?");
	}
	reg <<= ((num&7)<<2);
	addr = EIC_BASE + EIC_CONFIG + ((num>>3)<<2);
	write32(addr, read32(addr)|reg);
}

int
extirq_configure(unsigned pin, enum trigger trig, int flags)
{
	unsigned num;
	u32 mask, reg;
	int err;

	num = EIC_NUM(pin);
	if (num > EIC_MAX)
		return -EINVAL;

	mask = 1U << num;
	if ((err = port_pmux_pin(PORT_GROUP(pin), PORT_NUM(pin), EIC_PINROLE)) < 0)
		return err;

	gclk_enable_clock(CLK_GEN_ULP32K, CLK_DST_EIC);

	/* fail if we've already claimed this EIC number */
	reg = read32(EIC_BASE + EIC_WAKEUP);
	if (reg&mask)
		return -EINVAL;
	write32(EIC_BASE + EIC_WAKEUP, reg|mask);

	eic_set_config(num, trig);
	eic_enable();

	/* errata 1.9.1 -- some configuration 
	 * steps may cause a spurious interrupt
	 * flag to be raised */
	write32(EIC_BASE + EIC_INTFLAG, mask);
	irq_enable_num(EIC_IRQ_NUM);
	return 0;
}

void
extirq_enable(unsigned pin)
{
	unsigned num = EIC_NUM(pin);
	write32(EIC_BASE + EIC_INTENSET, 1U << num);
}

void
extirq_clear_enable(unsigned pin)
{
	unsigned num = EIC_NUM(pin);
	write32(EIC_BASE + EIC_INTFLAG, 1U << num);
	write32(EIC_BASE + EIC_INTENSET, 1U << num);
}

void
extirq_disable(unsigned pin)
{
	unsigned num = EIC_NUM(pin);
	write32(EIC_BASE + EIC_INTENCLR, 1U << num);
}

bool
extirq_triggered(unsigned pin)
{
	unsigned num = EIC_NUM(pin);
	return !!(read32(EIC_BASE + EIC_INTFLAG)&(1U << num));
}

void
eic_irq_entry(void)
{
	unsigned i;
	u32 iflags;

	iflags = read32(EIC_BASE + EIC_INTFLAG);
	if (!iflags)
		return;

	/* disable any of the interrupts we are about to handle.
	 * level-triggered interrupts will keep getting triggered;
	 * make the code explicitly re-enable the interrupts it is
	 * interested in receiving */
	write32(EIC_BASE + EIC_INTENCLR, iflags);
	write32(EIC_BASE + EIC_INTFLAG, iflags);
	for (i=0; i<EIC_MAX && iflags; i++) {
		if ((iflags&1) && extirq_table[i])
			extirq_table[i]();
		iflags >>= 1;
	}
}
