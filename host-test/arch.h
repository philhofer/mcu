#ifndef __HOST_ARCH_H_
#define __HOST_ARCH_H_
#include <bits.h>

void reboot(void);

void irq_disable(void);
void irq_enable(void);
bool irq_enabled(void);

u32 read32(ulong addr);
u16 read16(ulong addr);
u8  read8(ulong addr);

void write32(ulong addr, u32 val);
void write16(ulong addr, u16 val);
void write8(ulong addr, u8 val);

int irq_number(void);

static inline bool
in_irq(void)
{
	return irq_number() >= 0;
}

void bkpt(void);

void irq_disable_num(unsigned n);
void irq_enable_num(unsigned n);
bool irq_num_is_enabled(unsigned n);
void irq_clear_pending(unsigned n);
void irq_set_pending(unsigned n);
int irq_next_pending(void);
void irq_clear_all_pending(void);

void cpu_idle_hint(void);

u64 getcycles(void);

/* host-specific interfaces */

/* claim an interrupt number and handler */
void claim(unsigned num, void (*func)(void));

/* raise a registered interrupt */
void raise(unsigned num);

/* async_raise() raises an interrupt by
 * spawning a new thread and calling
 * raise() from that thread (which simulates
 * an external interrupt) */
void async_raise(unsigned num);

#endif
