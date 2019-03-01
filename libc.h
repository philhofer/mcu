#ifndef __LIBC_H_
#define __LIBC_H_
#include <bits.h>

/* These function identically to their
 * POSIX equivalents. */
void *memset(void *p, int c, ulong sz);
void *memcpy(void *p, const void *s, ulong sz);
void *memmove(void *p, const void *s, ulong sz);

/* print() works like puts(3), except that it
 * does not append a '\n' to the input string;
 * it prints the input string verbatim to stdout. */
void print(const char *str);

/* struct output is a generic output stream */
struct output {
	void  *ctx;
	long (*write)(const struct output *self, const char *data, ulong len);
};

/* struct input is a generic input stream */
struct input {
	void *ctx;
	long (*read)(const struct input *self, void *dst, ulong dstlen);
};

/* external code can set stdin and stdout
 * as the default intput/output streams
 * for stdlib functions */
extern const struct output *stdout;
extern const struct input  *stdin;

#endif
