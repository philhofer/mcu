#include "armv6m.h"

#define stub __attribute__((weak, alias("empty_irq_handler")))

void empty_irq_handler(void) {
	return;
}

/* irq 0 */
stub void powermgr_irq_entry(void);

/* irq 1 */
stub void sysctrl_irq_entry(void);

/* irq 2 */
stub void watchdog_irq_entry(void);

/* irq 3 */
stub void rtc_irq_entry(void);

/* irq 4 */
stub void eic_irq_entry(void);

/* irq 5 */
stub void nvmctrl_irq_entry(void);

/* irq 6 */
stub void dmac_irq_entry(void);

/* irq 7 */
stub void usb_irq_entry(void);

/* irq 8 */
stub void evsys_irq_entry(void);

/* irq 9 */
stub void sercom0_irq_entry(void);

/* irq 10 */
stub void sercom1_irq_entry(void);

/* irq 11 */
stub void sercom2_irq_entry(void);

/* irq 12 */
stub void sercom3_irq_entry(void);

/* irq 13 */
stub void sercom4_irq_entry(void);

/* irq 14 */
stub void sercom5_irq_entry(void);

/* irq 15 */
stub void tcc0_irq_entry(void);

/* irq 16 */
stub void tcc1_irq_entry(void);

/* irq 17 */
stub void tcc2_irq_entry(void);

/* irq 18 */
stub void tc3_irq_entry(void);

/* irq 19 */
stub void tc4_irq_entry(void);

/* irq 20 */
stub void tc5_irq_entry(void);

/* irq 21 */
stub void tc6_irq_entry(void);

/* irq 22 */
stub void tc7_irq_entry(void);

/* irq 23 */
stub void adc_irq_entry(void);

/* irq 24 */
stub void ac_irq_entry(void);

/* irq 25 */
stub void dac_irq_entry(void);

/* irq 26 */
stub void ptc_irq_entry(void);

/* irq 27 */
stub void i2s_irq_entry(void);

#define SYSCTRL_INTENCLR 0x40000800U /* interrupt enable clear register */
#define SYSCTRL_INTENSET 0x40000804U /* interrupt enable set register */
#define SYSCTRL_PCLKSR   0x4000080CU /* power and clock status register */
#define SYSCTRL_INTFLAG  0x40000808U /* interrupt flag status */
#define SYSCTRL_OSC8M    0x40000820U /* 8MHz oscillator control */
#define SYSCTRL_DFLLCTRL 0x40000824U /* DFLL control */
#define SYSCTRL_DFLLVAL  0x40000828U /* DFLL calibration value */
#define SYSCTRL_DFLLMUL  0x4000082CU /* DFLL clock multiplier */

#define or_bit(val, bit) val |= (1UL << (bit));

#define clear_bits(val, start, end) val &= ~mask((start), (end)-(start))

#define mask(off, width) (((1UL << (width)) - 1) << (off))

#define set_bits(val, start, end, bitval)				\
	do {								\
		clear_bits(val, start, end);				\
		val |= ((bitval) & mask((end)-(start), 0)) << (start);	\
	} while (0)

/* calibrate DFLL based on factory calibration
 * data present in the NVM calibration space */
static void dfll_calibrate(void) {
	ulong coarse, fine, reg;

	coarse = read32(0x080624UL) >> 26;
	fine = read32(0x080628UL) & mask(0, 10);

	/* DFLLVAL[0:9] = fine; DFLLVAL[10:15] = coarse */
	reg = read32(SYSCTRL_DFLLVAL);
	set_bits(reg, 0, 16, fine|(coarse<<10));
	write32(SYSCTRL_DFLLVAL, reg);

	/* low 16 bits of DFLLMUL are clock multiplier */
	reg = read32(SYSCTRL_DFLLMUL);
	set_bits(reg, 0, 16, 48000);
	write32(SYSCTRL_DFLLMUL, reg);
}

static void dfll_disable(void) {
	write32(SYSCTRL_DFLLCTRL, 0); /* Errata 9905 */
	while (read32(SYSCTRL_PCLKSR)&(1 << 4)) ;
}

static void dfll_enable(void) {
	/* enable all the things (but no the clock, yet) */
	ulong reg = read32(SYSCTRL_DFLLCTRL);
	/* bit 1: enable
	 * bit 3: stable: FINE calibration register value will be fixed
	 * bit 5: usbcrm: usb clock recovery mode enable
	 * bit 8: ccdis: chill cycle disable
	 * bit 10: bplckc: bypass coarse lock procedure  */
	reg |= 0b10100101010;
	write32(SYSCTRL_DFLLCTRL, reg);
	while (read32(SYSCTRL_PCLKSR)&(1 << 4)) ;
}

static void go_to_48mhz(void) {

}

void board_setup(void) {
	ulong reg;

	/* set OSC8M prescaler (bits 8:9) to 0 */
	reg = read32(SYSCTRL_OSC8M);
	clear_bits(reg, 8, 10);
	write32(SYSCTRL_OSC8M, reg);

	/* Errata 9905: calibrating the DFLL clock with
	 * the clock enabled can lock up the board. */
	dfll_disable();
	dfll_calibrate();
	dfll_enable();

	/* now set the 48MHz clock to the cpu clock */
	go_to_48mhz();
}
