#include <bits.h>
#include <arch.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>

/* whether or not lock() is held */
static bool ienabled = true;

/* the default state when entering start()
 * is to have interrupts enabled, but all
 * external interrupt sources masked */
static u32 imask = 0xffffffff; /* masked interrupts */
static u32 ipending;           /* pending interrupts */
static int curint = -1;        /* current interrupt number */

/* interrupts are enabled when the interrupt lock
 * isn't held (and they are implemented by external
 * code locking this lock and either executing the
 * interrupt or setting the pending bit)
 * so interrupts always execute with the lock held */
pthread_mutex_t ilock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t icond = PTHREAD_COND_INITIALIZER;

/* interrupt handlers */
void (*interrupts[32])(void);

static void
lock(void)
{
	pthread_mutex_lock(&ilock);
}

static void
unlock(void)
{
	pthread_mutex_unlock(&ilock);
}

static void
irq_check(void)
{
	for (int i=0; i<32; i++) {
		if ((ipending&(1U<<i)) && !(imask&(1U<<i))) {
			ipending &= ~(1U << i);
			if (interrupts[i] == NULL)
				continue;
			curint = i;
			interrupts[i]();
			curint = -1;
		}
	}
}

void
claim(unsigned num, void (*func)(void))
{
	assert(num < 32);
	assert(interrupts[num] == NULL);
	interrupts[num] = func;
}

void
raise(unsigned num)
{
	assert(interrupts[num]);
	lock();
	if (ipending & (1U<<num))
		goto done;
	if (imask & (1U<<num)) {
		ipending |= (1U<<num);
	} else {
		curint = num;
		interrupts[num]();
		curint = -1;
	}
	pthread_cond_broadcast(&icond);
done:
	unlock();
}

static void *
raise_in_thread(void *arg)
{
	unsigned *n = arg;
	raise(*n);
	free(n);
	return NULL;
}

void
async_raise(unsigned num)
{
	assert(interrupts[num]);
	unsigned *arg;
	pthread_t pt;
	int err;

	arg = calloc(sizeof(unsigned), 1);
	assert(arg);
	*arg = num;

	err = pthread_create(&pt, NULL, raise_in_thread, arg);
	assert(err == 0);
}

void
reboot(void)
{
	printf("REBOOT\n");
	exit(1);
}

void
irq_disable(void)
{
	assert(ienabled);
	ienabled = false;
	lock();
}

void
irq_enable(void)
{
	assert(!ienabled);
	ienabled = true;
	irq_check();
	unlock();
}

bool
irq_enabled(void)
{
	return ienabled;
}

u32
read32(ulong addr)
{
	printf("unhandled read32 @ %lx\n", addr);
	exit(1);
}

u16
read16(ulong addr)
{
	printf("unhandled read16 @ %lx\n", addr);
	exit(1);
}

u8
read8(ulong addr)
{
	printf("unhandled read8 @ %lx\n", addr);
	exit(1);
}

void
write32(ulong addr, u32 val)
{
	printf("unhandled write32 @ %lx = %ux\n", addr, val);
	exit(1);
}

void
write16(ulong addr, u16 val)
{
	printf("unhandled write16 @ %lx = %xs\n", addr, val);
	exit(1);
}

void
write8(ulong addr, u8 val)
{
	printf("unhandled write8 @ %lx = %xc\n", addr, val);
	exit(1);
}

int
irq_number(void)
{
	return curint;
}

void
bkpt(void)
{
	printf("unhandled BKPT\n");
	exit(1);
}

void
irq_disable_num(unsigned n)
{
	if (!in_irq())
		lock();
	imask |= (1U << n);
	if (!in_irq())
		unlock();
}

void
irq_enable_num(unsigned n)
{
	if (!in_irq())
		lock();
	imask &= ~(1U << n);
	if (!in_irq()) {
		irq_check();
		unlock();
	}
}

bool
irq_num_is_enabled(unsigned n)
{
	return (imask & (1U << n)) != 0;
}

void
irq_clear_pending(unsigned n)
{
	if (!in_irq())
		lock();
	ipending &= ~(1U << n);
	if (!in_irq())
		unlock();
}

void
irq_set_pending(unsigned n)
{
	if (!in_irq())
		lock();
	ipending |= (1U << n);
	if (!in_irq()) {
		irq_check();
		unlock();
	}
}

int
irq_next_pending(void)
{
	int rv;
	if (!in_irq())
		lock();
	if (ipending == 0)
		rv = -1;
	else
		rv = __builtin_ctz(ipending);
	if (!in_irq())
		unlock();
	return rv;
}

void
irq_clear_all_pending(void)
{
	if (!in_irq())
		lock();
	ipending = 0;
	if (!in_irq())
		unlock();
}

void
cpu_idle_hint(void)
{
	/* preserving the right semantics here is
	 * a bit strange... we need to drop the lock
	 * while waiting on the condition variable,
	 * but no interrupts should execute from within
	 * this routine, so we artificially mask everything
	 * before dropping the lock and then unmask it
	 * once something becomes pending
	 *
	 * see the idle loop in idle.c for the correct
	 * usage of cpu_idle_hint() and the expected
	 * semantics (we're allowed to sleep until
	 * an interrupt becomes pending) */

	unsigned old_imask = imask;
	imask = 0xffffffff;

	/* we must already be lock()'d */
	assert(!ienabled);
	while (ipending == 0)
		pthread_cond_wait(&icond, &ilock);
	imask = old_imask;
}

u64
getcycles(void)
{
	return (u64)clock();
}

