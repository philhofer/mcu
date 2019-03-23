#ifndef __SAM_EIC_H_
#define __SAM_EIC_H_

void eic_configure(unsigned num, unsigned sense);

void eic_enable(void);

void eic_irq_entry(void);

#endif
