#include <config.h>
#include <arch.h>
#include <bits.h>

#define SCB_ACTLR 0xE000E008UL /* implementation-defined "auxilliary control register" */
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

#define SCR_SEVONPEND (1UL << 4) /* interrupt inactive->pending creates a SEV */

#define ICSR_PENDSTSET (1UL << 26) /* ICSR: systick pending */

#define SYST_CSR   0xE000E010UL  /* control and status register */
#define SYST_RVR   0xE000E014UL  /* reload value register */
#define SYST_CVR   0xE000E018UL  /* current value register */
#define SYST_CALIB 0xE000E01CUL  /* implementation-defined calibration register */

/* SysTick CSR registers */
#define CSR_ENABLE    (1UL << 0) /* turn on the counter */
#define CSR_TICKINT   (1UL << 1) /* counter underflow causes an interrupt */
#define CSR_CLKSOURCE (1UL << 2) /* 1 means use the cpu clock; 0 means (optional!) external clock */
#define CSR_COUNTFLAG (1UL << 16) /* the counter has underflowed since the last CSR read or CVR write */

#define NVIC_ISER 0xE000E100UL /* interrupt set-enable register */
#define NVIC_ICER 0xE000E180UL /* interrupt clear-enable register */
#define NVIC_ISPR 0xE000E200UL /* interrupt set-pending register */
#define NVIC_ICPR 0xE000E280UL /* interrupt clear-pending register */
#define NVIC_IPR0 0xE000E400UL /* interrupt priority register base */

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

static volatile u32 systicks;

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

#define TICK_BITS 24
#define TICK_MASK ((1UL << TICK_BITS) - 1)

void
systick_entry(void)
{
	systicks++;
}

u64
getcycles(void)
{
	u32 ticks, bits, addend;

	/* we need to ensure the value of 'ticks'
	 * is consistent with the state of the counter register;
	 * we want to avoid the case where we read 'ticks' just
	 * before an overflow and SYST_CVR just after it.
	 *
	 * similarly, if we're in a context in which the systick
	 * interrupt is masked, pretend to observe one additional tick */
	do {
		addend = !!(read32(SCB_ICSR)&ICSR_PENDSTSET);
		ticks = systicks + addend;
		bits = read32(SYST_CVR)&TICK_MASK;
	} while (systicks + addend != ticks);

	return ((u64)ticks << TICK_BITS) | (u64)(TICK_MASK - bits);
}

static void
systick_setup(void)
{
	/* NOTE: with the systick register set to 0xffffff,
	 * the overflow interval for the register at 48MHz
	 * is 2.861Hz, which seems infrequent enough that
	 * enabling the cycle counter by default isn't going
	 * to cause too much jitter.
	 * At that frequency, the 64-bit cycle counter would
	 * overflow after 47.6 years, which seems conservative
	 * enough. */

	/* ensure that generating a SysTick interrupt
	 * while sleeping still causes a 'wfi' to return */
	write32(SCB_SCR, SCR_SEVONPEND);

	/* set systick priority to 0 (highest) */
	write32(SCB_SHPR3, 0);
	/* clear the current systick value (if any) */
	write32(SYST_CVR, 0);
	/* set the reload counter */
	write32(SYST_RVR, TICK_MASK);
	/* enable systick interrupts; enable systick; use the core clock */
	write32(SYST_CSR, CSR_CLKSOURCE|CSR_TICKINT|CSR_ENABLE);
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

void
irq_set_pending(unsigned n)
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

