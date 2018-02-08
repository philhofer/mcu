#ifndef __BH_H_
#define __BH_H_
#include <bits.h>

struct bh;

/* struct bh: an irq handler "bottom half"
 *
 * A "bottom half" is a chunk of code that runs
 * after an interrupt has fired, but doesn't
 * execute in the interrupt context. Consequently,
 * it doesn't block other interrupts from being
 * handled while it is running.
 *
 * Typically, one would choose to use a bh for
 * interrupt handling code that isn't latency-critical.
 * For example, if you have a peripheral that delivers
 * a "data ready" interrupt, you might choose to have the
 * interrupt handler simply enqueue a bottom half that
 * fetches the data from the peripheral. */
struct bh {
	struct bh *next;
	void (*exec)(struct bh *self);
	int status;
};

/* bh_enqueue() enqueues a bottom half if it
 * is not already enqueued. It is safe to call
 * this function from either regular or irq contexts,
 * but call sites that will always be in irq handlers
 * should call bh_enqueue_from_irq() instead. */
void bh_enqueue(struct bh *bh);

/* bh_enqueue_from_irq() enqueues a bottom half
 * from irq context. In case it isn't obvious,
 * this function should not be called from non-irq contexts. */
void bh_enqueue_from_irq(struct bh *bh);

/* idle_step() runs one bh, provided one is enqueued */
void idle_step(void);

/* idle_loop() does the following, forever:
 *
 *   1. run all pending bottom-halves
 *   2. pause the CPU until an interrupt is delivered
 *   3. goto 1
 */
void idle_loop(void);

#endif
