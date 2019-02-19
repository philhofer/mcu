#include <arch.h>
#include <sysctrl.h>
#include <board.h>
#include <drivers/sam/port.h>

#define stub __attribute__((weak, alias("default_handler")))

void
default_handler(void)
{
	irq_disable_num(irq_number());
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

/* Feather-M0 red LED light is on PA17 */
PORT_GPIO_PIN(red_led, PINGRP_A, 17);

void
board_setup(void)
{
	clock_init_defaults();
}
