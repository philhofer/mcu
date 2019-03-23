#include <bits.h>
#include <arch.h>
#include <kernel/sam/sercom.h>

void
sercom_reset(const struct sercom_config *conf)
{
	ulong syncbusy;
	sercom_t sc;
	ulong ctrla;

	sc = conf->base;
	ctrla = sercom_ctrla(sc);
	write32(ctrla, CTRLA_SWRST);

	syncbusy = sercom_syncbusy(sc);
	while ((read32(syncbusy)&SYNC_SWRST)|(read32(ctrla)&CTRLA_SWRST)) ;
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
