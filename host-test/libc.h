#ifndef __HOST_LIBC_H_
#define __HOST_LIBC_H_
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <bits.h>

struct output {
	void *ctx;
	long (*write)(const struct output *self, const char *data, ulong len);
};

struct input {
	void *ctx;
	long (*read)(const struct input *self, void *data, ulong len);
};

#endif
