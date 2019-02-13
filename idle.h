#ifndef __IDLE_H_
#define __IDLE_H_
#include <bits.h>

struct work;

struct work {
	struct work *next;
	void *udata;
	void (*func)(void *udata);
};

void schedule_work(struct work *work);

/* idle_step() runs all pending work.
 *
 * If 'sleep' is set, then idle_step() will wait
 * for an interrupt handler to fire when no work
 * is available. */
void idle_step(bool sleep);

#endif
