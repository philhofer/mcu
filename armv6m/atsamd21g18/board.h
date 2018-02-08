#include <arch.h>

#define SYSCTRL_INTENCLR 0x40000800UL /* interrupt enable clear register */
#define SYSCTRL_INTENSET 0x40000804UL /* interrupt enable set register */
#define SYSCTRL_PCLKSR   0x4000080CUL /* power and clock status register */
#define SYSCTRL_INTFLAG  0x40000808UL /* interrupt flag status */
#define SYSCTRL_OSC8M    0x40000820UL /* 8MHz oscillator control */
#define SYSCTRL_DFLLCTRL 0x40000824UL /* DFLL control */
#define SYSCTRL_DFLLVAL  0x40000828UL /* DFLL calibration value */
#define SYSCTRL_DFLLMUL  0x4000082CUL /* DFLL clock multiplier */

#define GCLK_CTRL    0x40000C00UL /* GCLK control register (8 bits) */
#define GCLK_STATUS  0x40000C01UL /* status register (8 bits) */
#define GCLK_CLKCTRL 0x40000C02UL /* clock control register (16 bits) */
#define GCLK_GENCTRL 0x40000C04UL /* clock generator control register */
#define GCLK_GENDIV  0x40000C08UL /* generator divisor register */
