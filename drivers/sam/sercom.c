#include <bits.h>
#include <arch.h>
#include <board.h>
#include <drivers/sam/powermgr.h>
#include <drivers/sam/gclk.h>
#include <drivers/sam/sercom.h>

void
sercom_reset(const sercom_t sc)
{
	ulong syncbusy;
	ulong ctrla;

	ctrla = sercom_ctrla(sc);
	write32(ctrla, CTRLA_SWRST);

	syncbusy = sercom_syncbusy(sc);
	while ((read32(syncbusy)&SYNC_SWRST)|(read32(ctrla)&CTRLA_SWRST)) ;
}

void
sercom_clock_setup(unsigned n)
{
	const struct sercom_config *conf;

	conf = &sercoms[n];

	/* unmask the peripheral in the power manager */
	powermgr_apb_mask(apb_num(conf->base), conf->apb_index, true);

	/* use GCLKGEN0, which feeds the CPU, as a clock source for this peripheral as well */
	gclk_enable_clock(0, conf->gclk);
}

void
sercom_enable(const sercom_t sc)
{
	ulong syncbusy;
	ulong ctrla;

	ctrla = sercom_ctrla(sc);
	write32(ctrla, read32(ctrla)|CTRLA_ENABLE);

	syncbusy = sercom_syncbusy(sc);
	while (read32(syncbusy) & SYNC_ENABLE) ;
}
