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

static inline ushort read16(ulong addr) {
	ushort out;
	__asm__ volatile (
		"ldrh %0, [%1] \n\t"
		: "=r"(out)
		: "r"(addr)
		: "memory"
		);
	return out;
}

static inline uchar read8(ulong addr) {
	uchar out;
	__asm__ volatile (
		"ldrb %0, [%1] \n\t"
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

static inline void write16(ulong addr, ushort val) {
	__asm__ volatile (
		"strh %0, [%1] \n\t"
		:
		: "r"(val), "r"(addr)
		: "memory"
		);
}

static inline void write8(ulong addr, uchar val) {
	__asm__ volatile (
		"strb %0, [%1] \n\t"
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
		: "memory", "cc"
		);
	return ipsr & 0x1f;
}

static inline void arch_wfi(void) {
	__asm__ volatile (
		"wfi \n\t"
		:
		:
		: "memory", "cc"
		);
}

static inline void arch_dsb(void) {
	__asm__ volatile (
		"dsb \n\t"
		:
		:
		: "memory", "cc"
		);
}

static inline void arch_isb(void) {
	__asm__ volatile (
		"isb \n\t"
		:
		:
		: "memory", "cc"
		);
}

static inline bool in_irq(void) {
	return irq_number() != 0;
}

/* for armv6-m, just issue a wfi when the cpu is idle */
#define cpu_idle_hint() arch_wfi()

/* irq_disable_num() disables irq number 'n' */
void irq_disable_num(unsigned n);

/* irq_enable_num() enables irq number 'n' */
void irq_enable_num(unsigned n);

/* irq_num_is_enabled() returns whether or not
 * irq 'n' is enabled. */
bool irq_num_is_enabled(unsigned n);

/* irq_clear_pending() clears all pending interrupts */
void irq_clear_pending(void);

/* idle() idles the cpu */
void idle(void);

/* begin non-portable declarations and definitions */

/* system control block registers */
#define SCB_ACTLR 0xE000E008UL /* implementation-defined "auxilliary control register */
#define SCB_CPUID 0xE000ED00UL /* CPUID base register */
#define SCB_ICSR  0xE000ED04UL /* interrupt control state register */
#define SCB_VTOR  0xE000ED08UL /* vector table offset register */
#define SCB_AIRCR 0xE000ED0CUL /* application interrupt and reset control register */
#define SCB_SCR   0xE000ED10UL /* system control register */
#define SCB_CCR   0xE000ED14UL /* configuration and control register */
#define SCB_SHPR2 0xE000ED1CUL /* system handler priority register 2 */
#define SCB_SHPR3 0xE000ED20UL /* system handler priority register 3 */
#define SCB_SHCSR 0xE000ED24UL /* system handler control and stat register */
#define SCB_DFSR  0xE000ED30UL /* debug fault status register */

/* SysTick registers */
#define SYST_CSR   0xE000E010UL  /* control and status register */
#define SYST_RVR   0xE000E014UL  /* reload value register */
#define SYST_CVR   0xE000E018UL  /* current value register */
#define SYST_CALIB 0xE000E01CUL  /* implementation-defined calibration register */

/* NVIC registers */
#define NVIC_ISER 0xE000E100UL /* interrupt set-enable register */
#define NVIC_ICER 0xE000E180UL /* interrupt clear-enable register */
#define NVIC_ISPR 0xE000E200UL /* interrupt set-pending register */
#define NVIC_ICPR 0xE000E280UL /* interrupt clear-pending register */
#define NVIC_IPR0 0xE000E400UL /* interrupt priority register base */

#endif
