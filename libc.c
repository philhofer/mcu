#include <bits.h>
#include <libc.h>
#include <error.h>
#include <idle.h>

__attribute__((weak))
const struct output *stdout;

__attribute__((weak))
const struct input *stdin;

void *
memset(void *p, int c, ulong sz)
{
	uchar *dst = p;
	uchar b = (uchar)c;

	for (; sz; sz--)
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
	
	if (dst < src) {
		for (; sz; sz--)	
			*dp++ = *sp++;
	} else {
		while (sz) {
			sz--;
			dp[sz] = sp[sz];
		}
	}
	return dst;
}

ulong
strlen(const char *str)
{
	ulong rv = 0;
	while (*str++) rv++;
	return rv;
}

static long
write_all(const struct output *odev, const char *str, ulong len)
{
	long rc, rv = 0;

	while (rv < len) {
		rc = odev->write(odev, str + rv, len - rv);
		if (rc < 0) {
			if (rc == -EAGAIN) {
				idle_step(true);
				continue;
			}
			return rc;
		}
		rv += rc;
	}
	return rv;
}

void
print(const char *str)
{
	if (stdout)
		write_all(stdout, str, strlen(str));
}
