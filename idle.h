#ifndef __IDLE_H_
#define __IDLE_H_
#include <bits.h>

/* 'work' represents some code
 * to be executed in the regular
 * threading context when the system
 * would otherwise be idling. */
struct work;

struct work {
	struct work *next;
	void *udata;
	void (*func)(void *udata);
};

static inline void
init_work(struct work *w, void (*fn)(void *), void *ctx)
{
	w->next = NULL;
	w->udata = ctx;
	w->func = fn;
}

/* schedule_work() schedules work to
 * the global queue of work to execute
 * at idle. Calling this function on work
 * that is already scheduled has no effect. */
void schedule_work(struct work *work);

/* idle_step() runs all pending work.
 *
 * If no pending work is available and
 * 'sleep' is set, then the routine will
 * idle the CPU until an interrupt handler
 * is fired, and then return.
 *
 * Code that needs to busy-wait on a condition
 * set from an interrupt should look like
 *
 *     while (!condition())
 *         idle_step(true);
 *
 */
void idle_step(bool sleep);

#endif
