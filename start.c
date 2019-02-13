#include <arch.h>
#include <idle.h>

void
start(void)
{
	for (;;)
		idle_step(true);
}
