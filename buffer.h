#ifndef __MKAL_BUFFER_H_
#define __MKAL_BUFFER_H_

enum iodir {
	IO_OUT = 0,
	IO_IN = 1,
};

struct mbuf {
	uchar *base;
	uchar *cur;
	ulong size;
	enum iodir dir;	
};

static inline void buf_init_out(struct mbuf *buf, void *mem, ulong memsz) {
	buf->base = buf->cur = mem;
	buf->size = memsz;
	buf->dir = IO_OUT;
}

static inline void buf_init_in(struct mbuf *buf, void *mem, ulong memsz) {
	buf->base = buf->cur = mem;
	buf->size = memsz;
	buf->dir = IO_IN;
}

static inline ulong buf_todo(const struct mbuf *buf) {
	return buf->size - (ulong)(buf->cur - buf->base);
}

static inline bool buf_ateof(const struct mbuf *buf) {
	return buf_todo(buf) == 0;
}

static inline uchar buf_getc(struct mbuf *buf) {
	/* TODO: assert(buf->dir == IO_OUT) */
	return *buf->cur++;
}

static inline void buf_putc(struct mbuf *buf, uchar byte) {
	/* TODO: assert(buf->dir == IO_IN) */
	*buf->cur++ = byte;
}

#endif
