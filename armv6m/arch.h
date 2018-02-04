#ifndef __ARMV6M_H_
#define __ARMV6M_H_
#include <bits.h>

/* reboot() reboots the board.
 * Note that reboot may return, since
 * reboot requests are not guaranteed to
 * be serviced instantly. */
void reboot(void);

/* disable all interrupt sources */
static inline void irq_disable(void) {
	__asm__ volatile (
		"cpsid i \n\t"
		:
		:
		: "memory"
		);
}

/* enable all interrupt sources */
static inline void irq_enable(void) {
	__asm__ volatile (
		"cpsie i \n\t"
		:
		:
		: "memory"
		);
}

static inline ulong read32(ulong addr) {
	ulong out;
	__asm__ volatile (
		"ldr %0, [%1] \n\t"
		: "=r"(out)
		: "r"(addr)
		: "memory"
		);
	return out;
}

static inline void write32(ulong addr, ulong val) {
	__asm__ volatile (
		"str %0, [%1] \n\t"
		:
		: "r"(val), "r"(addr)
		: "memory"
		);
}

/* irq_number() returns the number of the
 * current irq number, provided that the
 * code is executing in irq context. Otherwise,
 * zero is returned. */
static inline unsigned irq_number(void) {
	ulong ipsr;
	__asm__ volatile (
		"mrs %0, ipsr \n\t"
		: "=r"(ipsr)
		:
		: "memory"
		);
	return ipsr & 0x1f;
}

static inline bool in_irq(void) {
	return irq_number() != 0;
}

/* irq_disable_num() disables irq number 'n' */
void irq_disable_num(unsigned n);

/* irq_enable_num() enables irq number 'n' */
void irq_enable_num(unsigned n);

/* irq_set_priority() sets the priority of irq 'n'
 * Valid priorities are 0-3, inclusive, with lower
 * priorities being served first. */
void irq_set_priority(unsigned n, unsigned prio);

/* irq_get_priority() gets the priority of irq 'n'
 * Valid priorities are 0-3, inlusive, with lower
 * priorities being served first. */
unsigned irq_get_priority(unsigned n);

/* irq_num_is_enabled() returns whether or not
 * irq 'n' is enabled. */
bool irq_num_is_enabled(unsigned n);

/* irq_clear_pending() clears all pending interrupts */
void irq_clear_pending(void);

/* system control block registers */
#define SCB_ACTLR 0xE000E008 /* implementation-defined "auxilliary control register */
#define SCB_CPUID 0xE000ED00 /* CPUID base register */
#define SCB_ICSR  0xE000ED04 /* interrupt control state register */
#define SCB_VTOR  0xE000ED08 /* vector table offset register */
#define SCB_AIRCR 0xE000ED0C /* application interrupt and reset control register */
#define SCB_SCR   0xE000ED10 /* system control register */
#define SCB_CCR   0xE000ED14 /* configuration and control register */
#define SCB_SHPR2 0xE000ED1C /* system handler priority register 2 */
#define SCB_SHPR3 0xE000ED20 /* system handler priority register 3 */
#define SCB_SHCSR 0xE000ED24 /* system handler control and stat register */
#define SCB_DFSR  0xE000ED30 /* debug fault status register */

/* SysTick registers */
#define SYST_CSR   0xE000E010  /* control and status register */
#define SYST_RVR   0xE000E014  /* reload value register */
#define SYST_CVR   0xE000E018  /* current value register */
#define SYST_CALIB 0xE000E01C  /* implementation-defined calibration register */

/* NVIC registers */
#define NVIC_ISER 0xE000E100 /* interrupt set-enable register */
#define NVIC_ICER 0xE000E180 /* interrupt clear-enable register */
#define NVIC_ISPR 0xE000E200 /* interrupt set-pending register */
#define NVIC_ICPR 0xE000E280 /* interrupt clear-pending register */
#define NVIC_IPR0 0xE000E400 /* interrupt priority register base */

#endif
