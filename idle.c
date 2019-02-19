#include <bits.h>
#include <idle.h>
#include <arch.h>
#include <config.h>

struct workq {
	struct work *head;
	struct work *tail;
};

struct workq global;

static void
workq_push(struct workq *wq, struct work *work)
{
	if (wq->head == NULL)
		wq->head = work;
	else
		wq->tail->next = work;
	wq->tail = work;
}

static struct work *
workq_pop(struct workq *wq)
{
	struct work *rv;
	if ((rv = wq->head) == NULL)
		return NULL;
	if ((wq->head = rv->next) == NULL)
		wq->tail = NULL;
	rv->next = NULL;
	return rv;
}

void
schedule_work(struct work *work)
{
	if (!in_irq())
		irq_disable();

	/* don't schedule more than once */
	if (!work->next && work != global.tail)
		workq_push(&global, work);

	if (!in_irq())
		irq_enable();
}

void
idle_step(bool sleep)
{
	struct work *w;
	bool worked = false;
	if (in_irq())
		return;

	irq_disable();
	while ((w = workq_pop(&global)) != NULL) {
		irq_enable();
		w->func(w->udata);
		worked = true;
		irq_disable();
	}
	if (sleep && !worked)
		cpu_idle_hint();
	irq_enable();
}
