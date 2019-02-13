#include <bits.h>
#include <arch.h>
#include "sercom.h"

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
sercom_enable(const sercom_t sc)
{
	ulong syncbusy;
	ulong ctrla;

	ctrla = sercom_ctrla(sc);
	write32(ctrla, read32(ctrla)|CTRLA_ENABLE);

	syncbusy = sercom_syncbusy(sc);
	while (read32(syncbusy) & SYNC_ENABLE) ;
}
