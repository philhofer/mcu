#include <bits.h>
#include <softirq.h>
#include <arch.h>
#include <config.h>

#define MASK_WORDS ((NUM_IRQ + 31)>>5)

static u32 bh_mask[MASK_WORDS];

static inline void set_mask_bit(u32 set[MASK_WORDS], unsigned bit) {
	set[bit>>5] |= 1UL << (bit & 31);
}

static inline bool test_mask_bit(u32 set[MASK_WORDS], unsigned bit) {
	return (set[bit>>5]&(1UL << (bit&31))) != 0;
}

void softirq_trigger_from_irq(ushort num) {
	if (num < NUM_IRQ)
		set_mask_bit(bh_mask, num);
}

void softirq_trigger(ushort num) {
	if (num >= NUM_IRQ)
		return;
	bool crit = !in_irq();
	if (crit)
		irq_disable();
	softirq_trigger_from_irq(num);
	if (crit)
		irq_enable();
}

extern const struct softirq _ssoftirq;
extern const struct softirq _esoftirq;

static void exec_mask(u32 mask[MASK_WORDS]) {
	const struct softirq *start;

	for (start = &_ssoftirq; start < &_esoftirq; start++) {
		if (!test_mask_bit(mask, start->num))
			continue;

		if (start->flags&SOFTIRQ_REENABLE)
			irq_enable_num(start->num);

		start->exec();
	}
}

void idle_step(void) {
	u32 mask[MASK_WORDS];

	irq_disable();
	for (unsigned i=0; i<MASK_WORDS; i++) {
		mask[i] = bh_mask[i];
		bh_mask[i] = 0;
	}
	irq_enable();

	exec_mask(mask);
}

void idle_loop(void) {
	u32 mask[MASK_WORDS];
	u32 accum = 0;

	irq_disable();
again:
	for (unsigned i=0; i<MASK_WORDS; i++) {
		mask[i] = bh_mask[i];
		accum |= mask[i];
		bh_mask[i] = 0;
	}
	if (accum == 0) {
		cpu_idle_hint();
		goto again;
	}
	irq_enable();

	exec_mask(mask);
}
