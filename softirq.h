#ifndef __SOFTIRQ_H_
#define __SOFTIRQ_H_
#include <bits.h>

#define SOFTIRQ_SECTION ".softirq"

#define SOFTIRQ_REENABLE 1

struct softirq;

/* NOTE: don't declare softirqs directly; use softirq_decl() */
struct softirq {
	ushort num;
	ushort flags;
	void (*exec)(void);
};

/* softirq_decl(num, flags, func) is a macro that declares a static softirq handler.
 *
 * The default interrupt handler for all peripheral interrupt sources is to
 * disable iterrupts from that source (via irq_disable_num()) and then call
 * softirq_trigger(), which queues 'func' to run at some later point in time.
 * If SOFTIRQ_REENABLE is set, then the interrupt will be re-enabled just before
 * 'func' is executed. */
#define softirq_decl(n, f, exc)						\
	__attribute__((section(SOFTIRQ_SECTION)))			\
	const struct softirq softirq_##n = {				\
		.num = (n), .flags = (f), .exec = exc			\
			}

/* softirq_trigger() enqueues a softirq if it
 * is not already enqueued. It is safe to call
 * this function from either regular or irq contexts,
 * but call sites that will always be in irq handlers
 * should call softirq_trigger_from_irq() instead. */
void softirq_trigger(ushort num);

/* softirq_trigger_from_irq() enqueues a softirq
 * from irq context. In case it isn't obvious,
 * this function should not be called from non-irq contexts. */
void softirq_trigger_from_irq(ushort num);

/* idle_step runs all pending softirqs */
void idle_step(void);

/* idle_loop() does the following, forever:
 *
 *   1. run all pending softirqs
 *   2. pause the CPU until an interrupt is delivered
 *   3. goto 1
 */
void idle_loop(void);

#endif
