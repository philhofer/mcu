#include <bits.h>
#include <libc.h>

void *
memset(void *p, int c, ulong sz)
{
	uchar *dst = p;
	uchar b = (uchar)c;

	while (sz--)
		*dst++ = b;
	return p;
}

__attribute__ ((weak, alias("memmove")))
void *memcpy(void *dst, const void *src, ulong sz);

void *
memmove(void *dst, const void *src, ulong sz)
{
	const uchar *sp = src;
	uchar *dp = dst;

	while (sz--)
		*dp++ = *sp++;
	return dst;
}
