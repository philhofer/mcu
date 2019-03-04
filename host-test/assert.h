#ifndef __HOST_ASSERT_H_
#define __HOST_ASSERT_H_
#include <stdio.h>
#include <stdlib.h>

#define assert(cond) \
	do { if (!(cond)) { printf("%s:%d: assertion failed: " #cond"\n", __FILE__, __LINE__); exit(1); }} while (0)

#define static_assert(cond) _Static_assert(cond, #cond)

#endif
