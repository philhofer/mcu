#include <config.h>
#include <arch.h>
#include <bits.h>

#define stub __attribute__((weak, alias("empty_handler")))

/* called from assembly via vector-table */
void reset_entry(void);
void systick_entry(void);

/* TODO: implement these or perhaps make them
 * fail more gracefully in the absence of a proper
 * implementation... */
stub void svcall_entry(void);
stub void pendsv_entry(void);
stub void nmi_entry(void);

extern void start(void);
extern void board_setup(void);

extern ulong _sbss;
extern ulong _ebss;
extern ulong _sdata;
extern ulong _edata;
extern const ulong _sidata;

static void systick_setup(void);

ulong systicks;

static void
setup_data(void)
{
	ulong *dst = &_sdata;
	const ulong *src = &_sidata;

	while (dst < &_edata)
		*dst++ = *src++;
}

static void
setup_bss(void)
{
	ulong *dst = &_sbss;

	while (dst < &_ebss)
		*dst++ = 0;
}

void
reset_entry(void)
{
	/* interrupts are off */
	setup_bss();
	setup_data();

	/* kill any stray interrupts
	 * and turn off external interrupt sources;
	 * we'll re-enable them if they end up being
	 * used */
	write32(NVIC_ICER, 0xffffffff);
	irq_clear_all_pending();

	board_setup();
	systick_setup();

	irq_enable();
	start();
	reboot();
	while (1) ;
}

void
hardfault_entry(void)
{
	/* TODO: something else? see if a debugger is attached? */
	reboot();
}

void
empty_handler(void)
{
	int n;

	/* if nothing is configured to handle
	 * an interrupt source at compile time,
	 * we should keep it disabled */
	if ((n = irq_number()) >= 0)
		irq_disable_num((unsigned)n);
}

void
reboot(void)
{
	/* VECTKEY must always be 0x05fa */
	ulong word = ((ulong)0x05fa << 16) | (1 << 2);
	write32(SCB_AIRCR, word);
	return;
}

/* SysTick registers */

#define TICK_MASK (((ulong)1 << 24) - 1)

void
systick_entry(void)
{
	systicks++;
}

ulong
getcycles(void)
{
	ulong val = read32(SYST_CVR)&TICK_MASK;
	return systicks*100 + val;
}

static void
systick_setup(void)
{
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

void
irq_enable_num(unsigned n)
{
	write32(NVIC_ISER, ((ulong)1 << n));
}

void
irq_disable_num(unsigned n)
{
	if (n >= 32)
		return;
	write32(NVIC_ICER, 1 << n);
	arch_dsb();
	arch_isb();
}

int
irq_next_pending(void)
{
	u32 reg = read32(NVIC_ISPR);
	return reg ? __builtin_ctz(reg) : -1;
}

bool
irq_num_is_enabled(unsigned n)
{
	return n < 32 && (read32(NVIC_ISER)&(1 << n)) != 0;
}

bool
irq_num_is_pending(unsigned n)
{
	return n < 32 && (read32(NVIC_ISPR)&(1 << n)) != 0;
}

void irq_set_pending(unsigned n)
{
	if (n < 32)
		write32(NVIC_ISPR, 1<<n);
}

void
irq_clear_pending(unsigned n)
{
	if (n < 32)
		write32(NVIC_ICPR, 1 << n);
}

void
irq_clear_all_pending(void)
{
	write32(NVIC_ICPR, 0xffffffff);
	arch_dsb();
	arch_isb();
}
