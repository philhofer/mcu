#include <board.h>
#include <arch.h>
#include <drivers/sam/gclk.h>
#include <nvmctrl.h>

#define SYSCTRL_INTENCLR  0x0
#define SYSCTRL_INTENSET  0x4
#define SYSCTRL_INTFLAG   0x8
#define SYSCTRL_PCLKSR    0xC
#define SYSCTRL_XOSC      0x10
#define SYSCTRL_XOSC32K   0x14
#define SYSCTRL_OSC32K    0x18
#define SYSCTRL_OSCULP32K 0x1C
#define SYSCTRL_OSC8M     0x20
#define SYSCTRL_DFLLCTRL  0x24
#define SYSCTRL_DFLLVAL   0x28
#define SYSCTRL_DFLLMUL   0x2C
#define SYSCTRL_DFLLSYNC  0x30
#define SYSCTRL_BOD33     0x34
#define SYSCTRL_VREG      0x3C
#define SYSCTRL_VREF      0x40
#define SYSCTRL_DPLLCTRLA 0x44
#define SYSCTRL_DPLLRATIO 0x48
#define SYSCTRL_DPLLCTRLB 0x4C
#define SYSCTRL_DPLLSTATUS 0x50

/* bit positions in INT* and PCLKSR regs */
#define INT_XOSCRDY    (1UL << 0)
#define INT_XOSC32KRDY (1UL << 1)
#define INT_OSC32KRDY  (1UL << 2)
#define INT_OSC8MRDY   (1UL << 3)
#define INT_DFLLRDY    (1UL << 4)
#define INT_DFLLOOB    (1UL << 5)
#define INT_DFLLLCKF   (1UL << 6)
#define INT_DFLLLCKC   (1UL << 7)
#define INT_DFLLRCS    (1UL << 8)
#define INT_BOD33RDY   (1UL << 9)
#define INT_BOD33DET   (1UL << 10)
#define INT_B33SRDY    (1UL << 11)
#define INT_DPLLLCKR   (1UL << 15)
#define INT_DPLLLCKF   (1UL << 16)
#define INT_DPLLLTO    (1UL << 17)

/* bits shared for each oscillator register */
#define ONDEMAND (1UL << 7)
#define RUNSTDBY (1UL << 6)
#define ENABLE   (1UL << 1)

/* bits in XOSC[:16] reg */
#define XOSC_XTALEN     (1UL << 2)
#define XOSC_GAIN_SHIFT 8
#define XOSC_AMP_GC     (1UL << 11)
#define XOSC_STARTUP_SHIFT 12

/* bits in XOSC32K[:16] reg */
#define XOSC32K_XTALEN (1UL << 2)
#define XOSC32K_EN32K  (1UL << 3)
#define XOSC32K_AAMPEN (1UL << 5)
#define XOSC32K_STARTUP_SHIFT 8
#define XOSC32K_WRTLOCK (1UL << 12)

/* bits in OSC32K[:32] reg */
#define OSC32K_CALIB_SHIFT   16
#define OSC32K_WRTLOCK       (1UL << 12)
#define OSC32K_STARTUP_SHIFT 8
#define OSC32K_EN32K         (1UL << 3)

/* bits in OSC8M[:32] reg */
#define OSC8M_PRESC_SHIFT  8
#define OSC8M_CALIB_SHIFT  16
#define OSC8M_FRANGE_SHIFT 30

/* bits in DFLLCTRL reg */
#define DFLLCTRL_MODE   (1UL << 2)
#define DFLLCTRL_STABLE (1UL << 3)
#define DFLLCTRL_LLAW   (1UL << 4)
#define DFLLCTRL_USBCRM (1UL << 5)
#define DFLLCTRL_CCDIS  (1UL << 8)
#define DFLLCTRL_QLDIS  (1UL << 9)
#define DFLLCTRL_BPLCKC (1UL << 10)
#define DFLLCTRL_WAITLOCK (1UL << 11)

#define DFLLVAL_COARSE_SHIFT 10
#define DFLLVAL_DIFF_SHIFT   16

#define DFLLMUL_FSTEP_SHIFT  16
#define DFLLMUL_CSTEP_SHIFT  26

static void
pclkr_sync(unsigned mask)
{
	while (!(read32(SYSCTRL_BASE + SYSCTRL_PCLKSR)&mask)) ;
}

/* NOTE: calibrating the DFLL while it is enabled
 * will cause the board to lock up (errata such-and-such);
 * do this close to boot when the DFLL hasn't been enabled yet. */
static void
dfll_calibrate(void)
{
	/* coarse reading is 6 bits; fine is 10 bits */
	ulong coarse = read32(0x806024UL) >> 26;
	ulong fine = read32(0x806028UL) & 0x3ff;

	write16(SYSCTRL_BASE + SYSCTRL_DFLLVAL, fine|(coarse<<DFLLVAL_COARSE_SHIFT));
}

void
clock_init_defaults(void)
{
	/* Datasheet section 37 ('Electrical Characteristics')
	 * says at least 1 waitstate is necessary for 48MHz operation.
	 * NOTE: you'll need more waitstates if you run at VDD < 2.7V.
	 * The presumption here is 3.3V VDD. */
	nvmctrl_set_waitstates(1);

	/* clear the DFLLRDY bit if it is set; we'll need it...*/
	write16(SYSCTRL_BASE + SYSCTRL_INTFLAG, INT_DFLLRDY);

	/* errata 1.2.1 */
	write16(SYSCTRL_BASE + SYSCTRL_DFLLCTRL, 0);
	pclkr_sync(INT_DFLLRDY);

	/* enable DFLL48 in open-loop mode */
	dfll_calibrate();
	write16(SYSCTRL_BASE + SYSCTRL_DFLLCTRL, ENABLE);

	/* we _could_ decide to wait for coarse/fine lock here;
	 * it's not clear why we would need to do that */
	pclkr_sync(INT_DFLLRDY);

	/* set clock generator 0 (which feeds the CPU clock)
	 * to DFLL48M; at return we should be running at 48MHz */
	gclk_configure_clock(CLK_SRC_DFLL48M, 0, 0);		
}
