#include <bits.h>
#include <arch.h>
#include <board.h>
#include <drivers/sam/port.h>
#include <drivers/sam/powermgr.h>
#include <drivers/sam/gclk.h>
#include <drivers/sam/sercom.h>

void
sercom_reset(const struct sercom_config *conf, int pins)
{
	ulong syncbusy;
	sercom_t sc;
	ulong ctrla;
	assert(pins <= 4);

	/* unmask the device in its APB;
	 * give it access to the CPU clock;
	 * ensure SERCOM_CLOCK_SLOW is also attached
	 * to something */
	powermgr_apb_mask(apb_num(conf->base), conf->apb_index, true);
	gclk_enable_clock(0, conf->gclk);
	gclk_enable_clock(CLK_GEN_ULP32K, CLK_DST_SERCOMSLOW);

	for (int i=0; i<pins; i++)
		port_pmux_pin(conf->pingrp, conf->pins[i], conf->pinrole);

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
