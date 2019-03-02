#include <bits.h>
#include <arch.h>
#include <board.h>
#include <nvmctrl.h>

#define NVMCTRL_CTRLA 0x0
#define NVMCTRL_CTRLB 0x4

#define CTRLA_CMDEX (0xA5UL << 8) /* CMDEX 'key' for CTRLA writes */

/* values for the lower half of CTRLA: */
#define CMD_ER      0x2 /* erase row */
#define CMD_WP      0x4 /* write page */
#define CMD_EAR     0x5 /* erase auxilliary row */
#define CMD_WAP     0x6 /* write auxilliary page */
#define CMD_RWWEEER 0x1A /* RWEEE erase row */
#define CMD_RWWEEWP 0x1C /* RWEEE write page */
#define CMD_LR      0x40 /* lock region */
#define CMD_UR      0x41 /* unlock region */
#define CMD_SPRM    0x42 /* set power reduction mode */
#define CMD_CPRM    0x43 /* clear power reduction mode */
#define CMD_PBC     0x44 /* page buffer clear */
#define CMD_SSB     0x45 /* set security bit */
#define CMD_INVALL  0x46 /* invalidate all cache lines */
#define CMD_LDR     0x47 /* lock data region */
#define CMD_UDR     0x48 /* unlock data region */

#define CTRLB_RWS_SHIFT      1           /* read wait-state counter is [4:1] */
#define CTRLB_MANW           (1UL << 7)  /* manual write (default 1) */ 
#define CTRLB_CACHEDIS       (1UL << 18) /* disable cache (default 0) */
#define CTRLB_READMODE_SHIFT 16          /* one of the READMODE* constants here */
#define CTRLB_SLEEPRWM_SHIFT 8           /* one of the SLEEPWRM constants here */

#define READMODE_NO_MISS_PENALTY 0
#define READMODE_LOW_POWER       1
#define READMODE_DETERMINISTIC   2

#define SLEEPRWM_WAKEUPACCESS  0
#define SLEEPRWM_WAKEUPINSTANT 1
#define SLEEPRWM_DISABLED      3

void
nvmctrl_set_waitstates(unsigned n)
{
	ulong addr = NVMCTRL_BASE + NVMCTRL_CTRLB;
	write32(addr, read32(addr)|((n&0xf)<<CTRLB_RWS_SHIFT));
}
