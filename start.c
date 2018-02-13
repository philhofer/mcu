#include <arch.h>
#include <softirq.h>

static void do_nothing(void) {
	/* wheeeee */
}

softirq_decl(5, SOFTIRQ_REENABLE, do_nothing);

void start(void) {
	idle_loop();
}
