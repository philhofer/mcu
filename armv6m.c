#include "config.h"
#include "armv6m.h"
#include "bits.h"

#define stub __attribute__((weak, alias("empty_handler")))

/* called from assembly via vector-table */
void reset_entry(void);
void systick_entry(void);
stub void svcall_entry(void);
stub void pendsv_entry(void);
stub void nmi_entry(void);
stub void hardfault_entry(void);

extern void start(void);
extern void board_setup(void);

extern ulong _sbss;
extern ulong _ebss;
extern ulong _sdata;
extern ulong _edata;
extern const ulong _sidata;

static void systick_setup(void);

ulong systicks;

void memcpy_aligned(ulong *dst, const ulong *src, ulong words) {
	while (words--)
		*dst++ = *src++;
}

void memclr_aligned(ulong *start, ulong words) {
	while (words--)
		*start++ = 0;
}

static void setup_data(void) {
	memcpy_aligned(&_sdata, &_sidata, &_edata - &_sdata);
}

static void setup_bss(void) {
	memclr_aligned(&_sbss, &_ebss - &_sbss);
}

void reset_entry(void) {
	setup_bss();
	setup_data();

	/* kill any stray interrupts */
	irq_clear_pending();
	board_setup();
	systick_setup();

	irq_enable();
	start();
	reboot();
	while (1) ;
}

void empty_handler(void) {
	while (1) ;
}

void reboot(void) {
	/* VECTKEY must always be 0x05fa */
	ulong word = ((ulong)0x05fa << 16) | (1 << 2);
	write32(SCB_AIRCR, word);
	return;
}

/* SysTick registers */

#define TICK_MASK (((ulong)1 << 24) - 1)

void systick_entry(void) {
	systicks++;
}

/*
static bool systick_pending(void) {
	return ((read32(SCB_ICSR)>>26)&1) != 0;
}
*/

ulong getcycles(void) {
	ulong val = read32(SYST_CVR)&TICK_MASK;
	return systicks*100 + val;
}

static void systick_setup(void) {
	/* TODO: the mfg. can specify a
	 * calibration value for 10ms ticks */
	ulong tenms = CPU_HZ/100;

	/* set systick priority to 0 (highest) */
	write32(SCB_SHPR3, 0);
	/* clear the current systick value (if any) */
	write32(SYST_CVR, 0);
	/* set the reset value to 10ms */
	write32(SYST_RVR, tenms);
	/* enable systick interrupts; enable systick */
	write32(SYST_CSR, 3);
}

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

void irq_clear_pending(void) {
	write32(NVIC_ICPR, ~(ulong)0);
}

void irq_set_priority(unsigned num, unsigned prio) {
	if (prio > 3 || num >= NUM_IRQ)
		return;
	/* there are 8 NVIC_IPRn registers, each with 4 priorities */
	ulong val = read32(NVIC_IPR0 + num);
	ulong shift = 6 + 8 * (num % 4);
	val &= ~(3 << shift);
	val |= (prio << shift);
	write32(NVIC_IPR0 + num, val);
}

unsigned irq_get_priority(unsigned num) {
	if (num >= NUM_IRQ)
		return 0;
	ulong val = read32(NVIC_IPR0 + num);
	ulong shift = 6 + 8 * (num % 4);
	return (val & (3 << shift)) >> shift;
}
