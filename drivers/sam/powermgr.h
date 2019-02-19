#ifndef __SAM_POWERMGR_H_
#define __SAM_POWERMGR_H_
#include <bits.h>
#include <arch.h>
#include <board.h>

#define PM_CTRL     0x0
#define PM_SLEEP    0x1
#define PM_CPUSEL   0x8
#define PM_APBSEL0  0x9 /* APBxSEL start here; 1-byte registers */
#define PM_AHBMASK  0x14
#define PM_APBMASK0 0x18 /* APBxMASK start here; 4-byte registers */
#define PM_INTENCLR 0x34
#define PM_INTENSET 0x35
#define PM_INTFLAG  0x36
#define PM_RCAUSE   0x38

/* interrupts in PM_INT{ENCLR,ENSET,FLAG} regs */
#define INT_CKRDY 0x1

/* bits in RCAUSE (reset cause) register */
#define RCAUSE_POR   0x1 /* power-on reset */
#define RCAUSE_BOD12 0x2 /* 1.2V BOD reset */
#define RCAUSE_BOD33 0x4 /* 3.3V BOD reset */
#define RCAUSE_EXT   0x16 /* external reset */
#define RCAUSE_WDT   0x32 /* watchdog */
#define RCAUSE_SYST  0x64 /* system reset requested */

/* The bit-location of peripheral APB masks is heavily
 * board-dependent, so you'll need to consult the datasheet
 * for your board to figure out which PM register holds the
 * mask for your peripheral.
 * Note that many peripherals default to 'on,' so
 * you'll typically only need this for e.g. SERCOM peripherals. */
static inline void
powermgr_apb_mask(unsigned abp, unsigned index, bool on) {
	u32 rv, bf;
	ulong reg;

	reg = PM_BASE + PM_APBMASK0 + (4 * abp);
	bf = 1 << index;

	rv = read32(reg);
	if (on && (rv&bf) == 0) {
		write32(reg, rv|bf);
	} else if (!on && !!(rv&bf)) {
		write32(reg, rv&(~bf));
	}
}

#endif
