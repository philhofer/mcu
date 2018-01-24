#include "armv6m.h"
#include "bits.h"

/* called from assembly via vector-table */
void reset_entry(void);
void systick_entry(void);
void svcall_entry(void);
void pendsv_entry(void);

extern void start(void);

extern uchar _sbss;
extern uchar _ebss;

static void systick_setup(void);

ulong systicks;
ulong tickbias;

void reset_entry(void) {
	/* clear the bss section */
	ulong bss_start = (ulong)(&_sbss);
	ulong bss_end = (ulong)(&_ebss);
	while (bss_start != bss_end) {
		write32(bss_start, 0);
		bss_start += 4;
	}

	/* kill any stray interrupts */
	irq_clear_pending();

	systick_setup();

	start();
	reboot();
	while (1) ;
}

void svcall_entry(void) {
	/* TODO */
}

void pendsv_entry(void) {
	/* TODO */
}

void nmi_entry(void) {
	/* TODO */
	while (1) ;
}

void hardfault_entry(void) {
	/* TODO */
	while (1) ;
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

#define TICK_MASK (((ulong)1 << 24) - 1)

void systick_entry(void) {
	ulong overrun = read32(SYST_CVR) & TICK_MASK;
	ulong tenms = read32(SYST_CALIB) & TICK_MASK;
	if (overrun < tenms) {
		tickbias += tenms - overrun;
		while (tickbias >= tenms) {
			tickbias -= tenms;
			systicks++;
		}
	}
	systicks++;
}

static void systick_setup(void) {
	ulong calib = read32(SYST_CALIB);
	ulong tenms = calib & TICK_MASK;
	if (tenms == 0)
		return; /* TODO: find another way to calibrate the timer... */

	write32(SYST_CVR, 0);
	write32(SYST_RVR, tenms);
	write32(SYST_CSR, 3); /* enable systick interrupts; turn systick on */
}

#define NVIC_ISER 0xE000E100 /* interrupt set-enable register */
#define NVIC_ICER 0xE000E180 /* interrupt clear-enable register */
#define NVIC_ISPR 0xE000E200 /* interrupt set-pending register */
#define NVIC_ICPR 0xE000E280 /* interrupt clear-pending register */
#define NVIC_IPR0 0xE000E400 /* interrupt priority register base */

#define set_bit(reg, n) write32(reg, read32(reg) | ((ulong)1 << (n)));

void irq_enable_num(unsigned n) {
	set_bit(NVIC_ISER, n);
}

void irq_disable_num(unsigned n) {
	set_bit(NVIC_ICER, n);
}

bool irq_num_is_enabled(unsigned n) {
	return (read32(NVIC_ISER)&(1 << n)) != 0;
}

/* clear all pending interrupts */
void irq_clear_pending(void) {
	write32(NVIC_ICPR, ~(ulong)0);
}

void irq_set_priority(unsigned num, unsigned prio) {
	if (prio > 3 || num > 31)
		return;
	/* there are 8 NVIC_IPRn registers, each with 4 priorities */
	ulong val = read32(NVIC_IPR0 + num);
	ulong shift = 6 + 8 * (num % 4);
	val &= ~(3 << shift);
	val |= (prio << shift);
	write32(NVIC_IPR0 + num, val);
}

unsigned irq_get_priority(unsigned num) {
	if (num > 31)
		return 0;
	ulong val = read32(NVIC_IPR0 + num);
	ulong shift = 6 + 8 * (num % 4);
	return (val & (3 << shift)) >> shift;
}
