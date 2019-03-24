#include <arch.h>
#include <assert.h>
#include "../idle.h"

int work0_count;
int work1_count;

void
work_func(void *arg)
{
	*(int *)arg += 1;
}

declare_work(wrk0, work_func, &work0_count);
declare_work(wrk1, work_func, &work1_count);

static void
do_interrupt0(void)
{
	schedule_work(&wrk0);
}

static void
do_interrupt1(void)
{
	schedule_work(&wrk1);
}

int
main(void)
{
	assert(work0_count == 0);

	schedule_work(&wrk0);
	schedule_work(&wrk1);
	schedule_work(&wrk0);
	schedule_work(&wrk1);
	idle_step(true);
	if (work0_count != 1) {
		printf("expected work0_count=1; got %d\n", work0_count);
		return 1;
	}
	if (work1_count != 1) {
		printf("expected work1_count=1; got %d\n", work1_count);
		return 1;
	}


	/* now test scheduling from interrupt context;
	 * we're creating a race that idle_step() has
	 * to sort out for us */
	claim(1, do_interrupt0);
	claim(2, do_interrupt1);
	irq_enable_num(2);
	async_raise(1);
	async_raise(2);
	irq_enable_num(1);

	/* we expect to iterate at least once
	 * (to actually run the scheduled work),
	 * but we'll have to wait once before that
	 * if the interrupt hasn't been delivered
	 * by the time we reach this code
	 * (in other words, the worst-case wait
	 * is 2 calls to idle_step() per interrupt) */
	int max_wait = 4;
	while ((work0_count != 2 || work1_count != 2) && max_wait) {
		idle_step(true);
		max_wait--;
	}
	if (work0_count != 2) {
		printf("expected work0_count=2; got %d\n", work0_count);
		return 1;
	}
	if (work1_count != 2) {
		printf("expected work1_count=2; got %d\n", work1_count);
		return 1;
	}
	return 0;
}

