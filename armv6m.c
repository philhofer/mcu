#include "armv6m.h"
#include "bits.h"

/* called from assembly via vector-table */
void reset_entry(void);
void irq_entry(void);
void systick_entry(void);
void svcall_entry(void);
void pendsv_entry(void);

extern void main(void);

void reset_entry(void) {
	main();
	reboot();
}

typedef void (*irq_handler)(unsigned);

static irq_handler handlers[16];

void irq_entry(void) {
	int n = irq_number();
	irq_handler handler = handlers[n];
	if (handler != 0)
		handler(n);
	return;
}

void irq_register(unsigned num, void (*fn)(unsigned)) {
	handlers[num] = fn;
}

static void clear_systick();
static void clear_pendsv();

void systick_entry(void) {
	clear_systick();
	return;
}

void svcall_entry(void) {
	/* TODO */
	return;
}

void pendsv_entry(void) {
	clear_pendsv();
	return;
}

#define SCB_ACTLR 0xE000E008   /* implementation-defined "auxilliary control register */
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

static void clear_pendsv(void) {
	write32(SCB_ICSR, read32(SCB_ICSR) | 1 << 27);
}

static void clear_systick(void) {
	write32(SCB_ICSR, read32(SCB_ICSR) | 1 << 25);
}

/* relocate the vector table to a different address */
void set_vtor(ulong addr) {
	write32(SCB_VTOR, addr);
}

void reboot(void) {
	/* VECTKEY must always be 0x05fa */
	ulong word = ((ulong)0x05fa << 16) | (1 << 2);
	write32(SCB_AIRCR, word);
	return;
}

/* SysTick registers */
#define SYST_CSR   0xE000E010  /* control and status register */
#define SYST_RVR   0xE000E014  /* reload value register */
#define SYST_CVR   0xE000E018  /* current value register */
#define SYST_CALIB 0xE000E01C  /* implementation-defined calibration register */

#define NVIC_ISER 0xE000E100 /* interrupt set-enable register */
#define NVIC_ICER 0xE000E180 /* interrupt clear-enable register */
#define NVIC_ISPR 0xE000E200 /* interrupt set-pending register */
#define NVIC_ICPR 0xE000E280 /* interrupt clear-pending register */
#define NVIC_IPR0 0xE000E400 /* interrupt priority register base */

void irq_enable_num(unsigned n) {
	write32(NVIC_ISER, read32(NVIC_ISER) | (1 << n));
}

void irq_disable_num(unsigned n) {
	write32(NVIC_ICER, read32(NVIC_ICER) | (1 << n));
}

bool irq_num_is_enabled(unsigned n) {
	return (read32(NVIC_ISER)&(1 << n)) != 0;
}

/* clear all pending interrupts */
void irq_clear_pending(void) {
	write32(NVIC_ICPR, ~(ulong)0);
}

void irq_set_priority(unsigned num, unsigned prio) {
	if (prio > 3)
		return;
	/* there are 8 NVIC_IPRn registers, each with 4 priorities */
	ulong reg = NVIC_IPR0 + 4*(num / 4);
	ulong shift = num % 4;
	ulong mask = 3;
	ulong val = read32(reg);
	val &= ~(mask << shift);
	val |= (prio << shift);
	write32(reg, val);
}
