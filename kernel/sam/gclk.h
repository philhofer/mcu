#ifndef __SAM_GCLK_H_
#define __SAM_GCLK_H_

/* gclk_enable_clock() arranges for the clock signal
 * from the given 'generic clock generator' (genid)
 * to be provided to the given clock consumer
 * (consult your device datasheet to figure out
 * clksel values for your peripherals)
 *
 * You'll need to call gclk_configure_clock() with
 * the given generic clock generator id to ensure that
 * it has a clock signal routed to it. */
void gclk_enable_clock(unsigned genid, unsigned clksel);

/* gclk_configure_clock() arranges for the given clock source
 * (srcid) to be routed to the given 'generic clock generator'
 * (genid) with the given divisor (or none, if 'div' is 0 or 1)
 * (consult your device datasheet to figure out clock
 * source ids) */
void gclk_configure_clock(unsigned srcid, unsigned genid, unsigned div);

#endif
