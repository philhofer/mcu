#include <bits.h>
#include <idle.h>
#include <arch.h>
#include <assert.h>
#include <config.h>

extern const struct work __start_work;
extern const struct work __stop_work;

/* each bit in 'scheduled' indicates whether
 * or not work is pending */
static u32 scheduled;

void
schedule_work(const struct work *w)
{
	unsigned i, m;

	assert(w >= &__start_work && w < &__stop_work);
	i = (unsigned)(w - &__start_work);
	m = (1 << i);

	/* the 'scheduled' bit may be set from
	 * an interrupt handler, but it isn't
	 * cleared except for explicit calls to
	 * idle_step(), so we can speculatively
	 * examine the scheduled state and bail
	 * early without introducing a critical
	 * section */
	if (scheduled&m)
		return;
	if (in_irq()) {
		scheduled |= (1 << i);
		return;
	}

	/* TODO: add atomic primitives to arch.h;
	 * then use atomic_or() everywhere */
	irq_disable();
	scheduled |= (1 << i);
	irq_enable();
}

void
idle_step(bool sleep)
{
	const struct work *w;
	bool worked = false;
	unsigned i;
	u32 bf;
	if (in_irq())
		return;

	irq_disable();
	bf = scheduled;
	scheduled = 0;
	i = 0;
	while (bf) {
		if (bf&1) {
			irq_enable();
			w = (&__start_work) + i;
			w->func(w->udata);
			worked = true;
			irq_disable();
		}
		bf >>= 1;
		i++;
	}
	if (sleep && !worked)
		cpu_idle_hint();
	irq_enable();
}
