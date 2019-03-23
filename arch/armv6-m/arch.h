#ifndef __ARMV6M_H_
#define __ARMV6M_H_
#include <bits.h>

/* reboot() reboots the board.
 * Note that reboot may return, since
 * reboot requests are not guaranteed to
 * be serviced instantly. */
void reboot(void);

/* disable all interrupt sources */
static inline void
irq_disable(void)
{
	__asm__ volatile (
		"cpsid i \n\t"
		:
		:
		: "memory"
		);
}

/* enable all interrupt sources */
static inline void
irq_enable(void)
{
	__asm__ volatile (
		"cpsie i \n\t"
		:
		:
		: "memory"
		);
}

static inline bool
irq_enabled(void)
{
	ulong out;
	__asm__ volatile (
		"mrs %0, primask \n\t"
		: "=r"(out)
		:
		: "memory"
		);
	return (bool)(out&1);
}

static inline ulong
read32(ulong addr)
{
	ulong out;
	__asm__ volatile (
		"ldr %0, [%1] \n\t"
		: "=r"(out)
		: "r"(addr)
		: "memory"
		);
	return out;
}

static inline ushort
read16(ulong addr)
{
	ushort out;
	__asm__ volatile (
		"ldrh %0, [%1] \n\t"
		: "=l"(out)
		: "r"(addr)
		: "memory"
		);
	return out;
}

static inline uchar
read8(ulong addr)
{
	uchar out;
	__asm__ volatile (
		"ldrb %0, [%1] \n\t"
		: "=l"(out)
		: "r"(addr)
		: "memory"
		);
	return out;
}

static inline void
write32(ulong addr, ulong val)
{
	__asm__ volatile (
		"str %0, [%1] \n\t"
		:
		: "r"(val), "r"(addr)
		: "memory"
		);
}

static inline void
write16(ulong addr, ushort val)
{
	__asm__ volatile (
		"strh %0, [%1] \n\t"
		:
		: "l"(val), "r"(addr)
		: "memory"
		);
}

static inline void
write8(ulong addr, uchar val)
{
	__asm__ volatile (
		"strb %0, [%1] \n\t"
		:
		: "l"(val), "r"(addr)
		: "memory"
		);
}

static inline unsigned
arch_exception_number(void)
{
	ulong ipsr;
	__asm__ volatile (
		"mrs %0, ipsr \n\t"
		: "=r"(ipsr)
		:
		: "memory", "cc"
		);
	return ipsr & 0x1f;
}

/* irq_number() returns the number of the
 * current irq number, provided that the
 * code is executing in irq context. Otherwise,
 * -1 is returned. */
static inline int 
irq_number(void)
{
	unsigned n = arch_exception_number();
	if (n < 16)
		return -1;
	return (int)(n - 16);
}

static inline void
arch_wfi(void)
{
	__asm__ volatile (
		"wfi \n\t"
		:
		:
		: "memory", "cc"
		);
}

static inline void
arch_dsb(void)
{
	__asm__ volatile (
		"dsb \n\t"
		:
		:
		: "memory", "cc"
		);
}

static inline void
arch_isb(void)
{
	__asm__ volatile (
		"isb \n\t"
		:
		:
		: "memory", "cc"
		);
}

static inline void
bkpt(void)
{
	__asm__ volatile (
		"bkpt \n\t"
		:
		:
		: "memory"
		);
}

static inline bool in_irq(void) {
	return arch_exception_number() != 0;
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

/* irq_clear_pending() clears a pending interrupt */
void irq_clear_pending(unsigned n);

/* irq_set_pending() raises a pending interrupt */
void irq_set_pending(unsigned n);

/* irq_next_pending() returns the lowest-numbered
 * pending interrupt, or -1 if no interrupts are pending. */
int irq_next_pending(void);

/* irq_clear_all_pending() clears all pending interrupts */
void irq_clear_all_pending(void);

/* idle() idles the cpu */
void idle(void);

/* getcycles() provides access to the CPU's
 * cycle counter. Note that changes in the CPU's
 * core clock and clock jitter mean that this isn't
 * a terribly good time-keeping tool, but it is
 * a monotonic time source. */
u64 getcycles(void);

#endif
