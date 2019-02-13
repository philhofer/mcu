#include <bits.h>
#include <libc.h>

void *memset(void *p, int c, ulong sz) {
	uchar *dst = p;
	uchar b = (uchar)c;

	while (sz--)
		*dst++ = b;
	return p;
}

void *memcpy(void *dst, const void *src, ulong sz) {
	return memmove(dst, src, sz);
}

void *memmove(void *dst, const void *src, ulong sz) {
	const uchar *sp = src;
	uchar *dp = dst;

	while (sz--)
		*dp++ = *sp++;
	return dst;
}
