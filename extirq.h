#ifndef __EXTIRQ_H_
#define __EXTIRQ_H_

/* External Interrupts
 *
 * External interrupt sources have their own
 * vector table, and can be edge- or level-triggered.
 * Before external interrupts are handled, the
 * interrupt source is masked so that level-triggered
 * interrupts behave like edge-triggered interrupts.
 * Interrupts can be re-enabled by calling extirq_enable()
 * (usually at the end of the interrupt handler routine).
 * The extirq_triggered() function can check the status
 * of a particular external interrupt independent of its
 * enabled/disabled status.
 */

int extirq_init(void);

/* extirq_enable() unmasks the interrupt handling
 * routine for an external interrupt handler */
void extirq_enable(unsigned num);

bool extirq_enabled(unsigned num);

/* extirq_clear_enable() clears the pending status
 * of an interrupt pin before enabling it */
void extirq_clear_enable(unsigned num);

/* extirq_disable() masks the interrupt handling
 * routine for an external interrupt handler */
void extirq_disable(unsigned num);

/* extint_triggered() returns whether or not the
 * external interrupt source is triggered.
 * Note that triggering is independent from the
 * interrupt handling routine from being enabled. */
bool extint_triggered(unsigned num);

#endif
