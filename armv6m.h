#ifndef __ARMV6M_H_
#define __ARMV6M_H_
#include "bits.h"

/* reboot() reboots the board.
 * Note that reboot may return, since
 * reboot requests are not guaranteed to
 * be serviced instantly. */
void reboot(void);

/* enable all interrupt sources */
static inline void irq_disable(void) {
	__asm__ volatile (
		"cpsie i \n\t"
		:
		:
		: "memory"
		);
}

/* disable all interrupt sources */
static inline void irq_enable(void) {
	__asm__ volatile (
		"cpsid i \n\t"
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

/* set_vtor() sets the vector table offset register.
 * The vector table needs to be naturally-aligned. */
void set_vtor(ulong addr);

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

#endif
