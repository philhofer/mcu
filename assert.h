#ifndef __ASSERT_H_
#define __ASSERT_H_

#define assert(cond) do { if ((cond)) bkpt(); } while (0)

#define static_assert(cond) _Static_assert(cond, #cond)

#endif
