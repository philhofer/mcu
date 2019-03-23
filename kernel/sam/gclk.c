#include <bits.h>
#include <arch.h>
#include <config.h>

#define GCLK_CTRL    0x0
#define GCLK_STATUS  0x1
#define GCLK_CLKCTRL 0x2
#define GCLK_GENCTRL 0x4
#define GCLK_GENDIV  0x8

#define CTRL_SWRST 0x1

#define STATUS_SYNCBUSY 0x80

/* CLKCTRL[16]: low 6 bits are ID, [8:10] are GEN */
#define CLKCTRL_CLKEN     (1UL<<14)
#define CLKCTRL_WRTLOCK   (1UL<<15)
#define CLKCTRL_GEN_SHIFT 8

/* GENCTRL[32]: low 4 bits are ID; [8:12] are SRC */
#define GENCTRL_SRC_SHIFT 8
#define GENCTRL_GENEN     (1UL<<16)
#define GENCTRL_IDC       (1UL<<17)
#define GENCTRL_OOV       (1UL<<18)
#define GENCTRL_OE        (1UL<<19)
#define GENCTRL_DIVSEL    (1UL<<20)
#define GENCTRL_RUNSTDBY  (1UL<<21)

/* GENDIV[32]: low 4 bits are ID; [8:23] are DIV */
#define GENDIV_DIVSHIFT 8

static void
gclk_sync(void)
{
	while (read8(GCLK_BASE + GCLK_STATUS)&STATUS_SYNCBUSY) ;
}

/* clock generator -> clock user */
void
gclk_enable_clock(unsigned genid, unsigned clksel)
{
	write16(GCLK_BASE + GCLK_CLKCTRL, CLKCTRL_CLKEN|(genid << CLKCTRL_GEN_SHIFT)|clksel);
	gclk_sync();
}

/* clock source -> clock generator */
void
gclk_configure_clock(unsigned srcid, unsigned genid, unsigned div)
{
	if (div)
		write32(GCLK_BASE + GCLK_GENDIV, (div << GENDIV_DIVSHIFT)|genid);
	write32(GCLK_BASE + GCLK_GENCTRL, (srcid << GENCTRL_SRC_SHIFT)|genid|GENCTRL_GENEN|GENCTRL_RUNSTDBY);
	gclk_sync();
}
