#ifndef __LIBC_H_
#define __LIBC_H_
#include <bits.h>

void *memset(void *p, int c, ulong sz);
void *memcpy(void *p, const void *s, ulong sz);
void *memmove(void *p, const void *s, ulong sz);

#endif
