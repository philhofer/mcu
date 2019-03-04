#ifndef __HOST_ASSERT_H_
#define __HOST_ASSERT_H_
#include <stdio.h>
#include <stdlib.h>

#define assert(cond) \
	do { if (!(cond)) { puts("assertion failed: " #cond); exit(1); }} while (0)

#endif
