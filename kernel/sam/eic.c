#include <config.h>
#include <error.h>
#include <kernel/sam/eic.h>
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

extern void eic_vec(u32 vec);

static void
eic_sync(void)
{
	while (read8(EIC_BASE + EIC_STATUS)&STATUS_SYNCBUSY)
		;
}

void
eic_enable(void)
{
	write32(EIC_BASE + EIC_INTFLAG, 0xffffffff);
	write8(EIC_BASE + EIC_CTRL, CTRL_ENABLE);
	eic_sync();
}

void 
eic_configure(unsigned num, unsigned sense)
{
	u32 reg;
	ulong addr;
	assert(sense >= 0 && sense < 6);

	/* the CONFIGn registers are actually 8 4-bit
	 * registers, so we need to assemble that value
	 * and then shift it into position */
	reg = (sense << ((num&7)<<2));
	addr = EIC_BASE + EIC_CONFIG + ((num>>3)<<2);
	write32(addr, read32(addr)|reg);
}

void
extirq_enable(unsigned num)
{
	write32(EIC_BASE + EIC_INTENSET, 1U << num);
}

void
extirq_clear_enable(unsigned num)
{
	write32(EIC_BASE + EIC_INTFLAG, 1U << num);
	write32(EIC_BASE + EIC_INTENSET, 1U << num);
}

void
extirq_disable(unsigned num)
{
	write32(EIC_BASE + EIC_INTENCLR, 1U << num);
}

bool
extirq_triggered(unsigned num)
{
	return !!(read32(EIC_BASE + EIC_INTFLAG)&(1U << num));
}

void
eic_irq_entry(void)
{
	u32 iflags;

	iflags = read32(EIC_BASE + EIC_INTFLAG);
	/* make sure we don't respond to masked
	 * external interrupts when others are unmasked */
	iflags &= read32(EIC_BASE + EIC_INTENSET);
	if (!iflags)
		return;

	/* disable any of the interrupts we are about to handle.
	 * level-triggered interrupts will keep getting triggered;
	 * make the code explicitly re-enable the interrupts it is
	 * interested in receiving */
	write32(EIC_BASE + EIC_INTENCLR, iflags);
	write32(EIC_BASE + EIC_INTFLAG, iflags);
	eic_vec(iflags);
}
