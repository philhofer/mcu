#include <arch.h>
#include <assert.h>
#include "../idle.h"

int work_count;
struct work wrk;

static void
work_func(void *arg)
{
	*(int *)arg += 1;
}

static void
do_interrupt(void)
{
	schedule_work(&wrk);
}

int
main(void)
{
	init_work(&wrk, work_func, (void *)&work_count);
	assert(work_count == 0);

	schedule_work(&wrk);
	schedule_work(&wrk);
	idle_step(true);
	if (work_count != 1) {
		printf("expected work_count=1; got %d\n", work_count);
		return 1;
	}


	/* now test scheduling from interrupt context;
	 * we're creating a race that idle_step() has
	 * to sort out for us */
	claim(1, do_interrupt);
	async_raise(1);
	irq_enable_num(1);

	/* we expect to iterate at least once
	 * (to actually run the scheduled work),
	 * but we'll have to wait once before that
	 * if the interrupt hasn't been delivered
	 * by the time we reach this code */
	int max_wait = 2;
	while (work_count != 2 && max_wait) {
		idle_step(true);
		max_wait--;
	}
	if (work_count != 2) {
		printf("expected work_count=2; got %d\n", work_count);
		return 1;
	}
	return 0;
}

