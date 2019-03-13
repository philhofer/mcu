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

enum trigger {
	TRIG_NONE, /* no trigger */
	TRIG_RISE, /* trigger on rising edge */
	TRIG_FALL, /* trigger on falling edge */
	TRIG_BOTH, /* trigger on any edge */
	TRIG_HIGH, /* level trigger high */
	TRIG_LOW,  /* level trigger low */
};

struct extirq {
	void (*func)(void);
	enum trigger trig;
	unsigned pin;
};

/* extirq_table[] holds the table of external
 * interrupt handlers. */
extern const struct extirq extirq_table[];

/* extirq_configure() configures an external interrupt source
 * to trigger an interrupt with the given trigger type.
 *
 * Software can receive the interrupt by implementing
 * a function named 'extirq_n()' (the external interrupt
 * table is resolved at link-time) 
int extirq_configure(unsigned pin, enum trigger trig, int flags);
*/

int extirq_init(void);

/* extirq_enable() unmasks the interrupt handling
 * routine for an external interrupt handler */
void extirq_enable(unsigned pin);

/* extirq_clear_enable() clears the pending status
 * of an interrupt pin before enabling it */
void extirq_clear_enable(unsigned pin);

/* extirq_disable() masks the interrupt handling
 * routine for an external interrupt handler */
void extirq_disable(unsigned pin);

/* extint_triggered() returns whether or not the
 * external interrupt source is triggered.
 * Note that triggering is independent from the
 * interrupt handling routine from being enabled. */
bool extint_triggered(unsigned pin);

#endif
