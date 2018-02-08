#include <bh.h>
#include <bits.h>
#include <arch.h>

static struct bh *bh_head;
static struct bh *bh_tail;

void bh_enqueue_from_irq(struct bh *bh) {
	if (bh->status != 0)
		return;

	if (bh_tail == NULL)
		bh_head = bh;
	else
		bh_tail->next = bh;
	bh_tail = bh;
	bh->status = 1;
}

void bh_enqueue(struct bh *bh) {
	bool inirq = in_irq();
	if (inirq)
		irq_disable();
	bh_enqueue_from_irq(bh);
	if (inirq)
		irq_enable();
}

static struct bh *pop(void) {
	struct bh *head;
	irq_disable();
	head = bh_head;
	if (head) {
		head->status = 0;
		if ((bh_head = head->next) == NULL)
			bh_tail = NULL;
	}
	irq_enable();
	return head;
}

static struct bh *pop_or_idle(void) {
	struct bh *bh;
	irq_disable();
	while (bh_head == NULL)
		arch_wfi();
	bh = bh_head;
	bh->status = 0;
	if ((bh_head = bh->next) == NULL)
		bh_tail = NULL;
	irq_enable();
	return bh;
}

void idle_step(void) {
	struct bh *bh = pop();
	if (bh)
		bh->exec(bh);
}

void idle_loop(void) {
	struct bh *bh;
	while (true) {
		bh = pop_or_idle();
		bh->exec(bh);
	}
}
