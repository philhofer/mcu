#include <arch.h>
#include <drivers/i2c.h>
#include "board.h"

#define stub __attribute__((weak, alias("default_handler")))

void
default_handler(void)
{
	irq_disable_num(irq_number());
	return;
}

extern struct i2c_bus_ops sercom_i2c_bus_ops;

/* i2c bus 0 is sercom1 */
DECLARE_I2C_BUS(0, &sercom_i2c_bus_ops, 0x42000C00);

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

/* calibrate DFLL based on factory calibration
 * data present in the NVM calibration space */
static void
dfll_calibrate(void)
{
	/* coarse reading is 6 bits; fine is 10 bits */
	ulong coarse = read32(0x080624UL) >> 26;
	ulong fine = read32(0x080628UL) & 0x3ff;

	/* low 16 bits of DFLLMUL are clock multiplier */
	write32(SYSCTRL_DFLLMUL, 48000);

	/* DFLLVAL[0:9] = fine; DFLLVAL[10:15] = coarse */
	write32(SYSCTRL_DFLLVAL, fine|(coarse<<10));
}

static void
dfll_disable(void)
{
	write16(SYSCTRL_DFLLCTRL, 0);
	while (read32(SYSCTRL_PCLKSR)&(1 << 4));
}

static void
dfll_enable(void)
{
	/* bit 1: enable
	 * bit 3: stable: FINE calibration register value will be fixed
	 * bit 5: usbcrm: usb clock recovery mode enable
	 * bit 8: ccdis: chill cycle disable
	 * bit 10: bplckc: bypass coarse lock procedure  */
	ushort reg = 0b10100101010;
	write16(SYSCTRL_DFLLCTRL, reg);
	while (read32(SYSCTRL_PCLKSR)&(1 << 4));
}

static void
go_to_48mhz(void)
{
	/* set clock 0 source to DFLL48MHz */
	ulong reg = 0;
	reg |= 1UL<<21; /* RUNSTDBY: run in standby */
	reg |= 1UL<<16; /* CLKEN: enable */
	reg |= 7UL<<8;  /* SRC: DFLL48M */
	/* bits 0:3 = 0: GCLKGEN0 */
	write32(GCLK_GENCTRL, reg);
	while (read8(GCLK_STATUS)&(1 << 7));
}

void
board_setup(void)
{
	ulong reg;

	/* set OSC8M prescaler (bits 8:9) to 0 */
	reg = read32(SYSCTRL_OSC8M);
	reg &= ~(3UL << 8);
	write32(SYSCTRL_OSC8M, reg);

	/* Errata 9905: calibrating the DFLL clock with
	 * the clock enabled can lock up the board. */
	dfll_disable();
	dfll_calibrate();
	dfll_enable();

	/* now set the 48MHz clock to the cpu clock */
	go_to_48mhz();
}
