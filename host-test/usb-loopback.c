#include <arch.h>
#include <bits.h>
#include <assert.h>
#include <usb-loopback.h>
#include <pthread.h>
#include "../idle.h"
#include "../usb.h"
#include "../usb-internal.h"

#define MAX_ENDPOINTS 3

bool usb_trace;

#define tracef(fmt, ...) \
	do { if (usb_trace) printf(fmt, __VA_ARGS__); } while (0)

#define trace(line) \
	do { if (usb_trace) puts(line); } while (0)

enum ep_state {
	EP_DISABLED = 0,
	EP_NAK      = 1,
	EP_DATA     = 2,
	EP_STALL    = 3,
};

struct endpoint {
	enum ep_state state;
	enum ep_type  type;
	void *buf;
	u8    buflen;

	/* accesses to the 'flags'
	 * register are synchronized
	 * across the host and device
	 * threads; once the host delivers
	 * EP_INT and/or EP_SETUP, it won't
	 * access this endpoint again until
	 * those flags are cleared */
	pthread_mutex_t reglock;
	pthread_cond_t  regcond;
	u8              epflags;
};

#define EP_INT   (1U << 0)
#define EP_SETUP (1U << 1)

struct epbank {
	struct endpoint out;
	struct endpoint in;
};

struct usb_lo {
	pthread_mutex_t lock;
	pthread_cond_t  cond;
	struct epbank   banks[MAX_ENDPOINTS];
	u8              addr;
	u8              irq;
	unsigned        flags;

	struct work irq_bh;
};

struct usb_lo *
new_usb_loopback(void)
{
	struct usb_lo *lo = calloc(sizeof(struct usb_lo), 1);
	lo->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	lo->cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	lo->irq = USB_EMULATOR_IRQ;
	return lo;
}

static void
dev_lock(struct usb_lo *lo)
{
	pthread_mutex_lock(&lo->lock);
}

static void
dev_unlock(struct usb_lo *lo)
{
	pthread_mutex_unlock(&lo->lock);
}

static void
dev_unlock_notify(struct usb_lo *lo)
{
	dev_unlock(lo);
	pthread_cond_broadcast(&lo->cond);
}

static void
dev_wait(struct usb_lo *lo)
{
	pthread_cond_wait(&lo->cond, &lo->lock);
}

static struct endpoint *
ep_get(struct usb_lo *lo, u8 ep)
{
	struct epbank *bk;
	assert((ep&0x3f) < MAX_ENDPOINTS);
	bk = &lo->banks[(ep&0x3f)];
	return (ep & 0x80) ? &bk->in : &bk->out;
}

static struct usb_lo *
driver(struct usb_dev *dev)
{
	return dev->driverdata;
}

#define LO_ENABLED     (1U << 0)
#define LO_RESET       (1U << 1)

static bool
getflags(struct usb_lo *lo, unsigned flags)
{
	return (lo->flags&flags) == flags;
}

static void emu_init(struct usb_dev *dev);

static void
emu_irq_bh(struct usb_dev *dev)
{
	trace("device bottom-half");
	struct usb_lo *lo = driver(dev);
	struct epbank *bk;

	/* there's a lot of locking and unlocking
	 * happening here because control and class
	 * callbacks can make calls back into the
	 * device implementation
	 */

	dev_lock(lo);
	if (getflags(lo, LO_RESET)) {
		dev_unlock(lo);
		emu_init(dev);
		goto done;
	}

	for (int i=0; i<MAX_ENDPOINTS; i++) {
		bk = &lo->banks[i];
		if (i == 0 && (bk->out.epflags&EP_SETUP)) {
			bk->out.epflags = 0;
			dev_unlock(lo);
			trace("device setup in bh");
			usb_handle_setup(dev, bk->out.buf, bk->out.buflen);
			dev_lock(lo);
		} else if (bk->out.epflags&EP_INT) {
			bk->out.epflags = 0;
			dev_unlock(lo);
			if (i == 0) {
				usb_ep0_rx(dev, 0);
			} else {
				dev->class->ep_rx(dev, i, bk->out.buf, bk->out.buflen, 0);
			}
			dev_lock(lo);
		}
		if (bk->in.epflags&EP_INT) {
			bk->in.epflags = 0;
			dev_unlock(lo);
			if (i == 0) {
				usb_ep0_tx(dev, 0);
			} else {
				dev->class->ep_tx(dev, i|0x80, 0);
			}
			dev_lock(lo);
		}
	}

	dev_unlock_notify(lo);
done:
	irq_enable_num(USB_EMULATOR_IRQ);
}

void
usb_emu_irq(struct usb_dev *dev)
{
	struct usb_lo *lo = driver(dev);
	lo->irq_bh.func = (void(*)(void *))emu_irq_bh;
	lo->irq_bh.udata = dev;
	schedule_work(&lo->irq_bh);
	irq_disable_num(USB_EMULATOR_IRQ);
}

static void
emu_init(struct usb_dev *dev)
{
	struct usb_lo *lo = driver(dev);
	dev_lock(lo);
	lo->flags = LO_ENABLED;
	lo->addr = 0;

	memset(lo->banks, 0, sizeof(lo->banks));
	lo->banks[0].out.buf = dev->ctrlbuf;
	lo->banks[0].out.buflen = sizeof(dev->ctrlbuf);
	lo->banks[0].in.buf = dev->ctrlbuf;

	dev_unlock_notify(lo);
	usb_reset(dev);
}

static void
emu_set_addr(struct usb_dev *dev, u8 addr)
{
	struct usb_lo *lo = driver(dev);
	dev_lock(lo);
	assert(getflags(lo, LO_ENABLED));
	assert(lo->addr == 0);
	lo->addr = addr;
	dev_unlock_notify(lo);
}

/* raise the device interrupt */
static void
emu_raise(struct usb_lo *lo)
{
	raise(lo->irq);
}

static int
emu_init_ep(struct usb_dev *dev, u8 ep, enum ep_type type)
{
	struct endpoint *ept;
	struct usb_lo *lo = driver(dev);

	dev_lock(lo);
	assert(getflags(lo, LO_ENABLED));
	ept = ep_get(lo, ep);
	assert(ept->state == EP_DISABLED);

	/* only endpoint 0/0x80 is control */
	assert((!(ep&0x3f)) == (type == EP_CONTROL));

	ept->type = type;
	ept->state = EP_NAK;
	ept->epflags = EP_INT;
	dev_unlock_notify(lo);
	return 0;
}

static int
emu_expect_out(struct usb_dev *dev, u8 ep, void *buf, u8 len)
{
	struct endpoint *ept;
	struct usb_lo *lo = driver(dev);

	tracef("expect out %u %u\n", ep, len);

	dev_lock(lo);
	assert(getflags(lo, LO_ENABLED));
	assert((ep&0x80) == 0);
	ept = ep_get(lo, ep);
	assert(ept->state == EP_NAK || ept->state == EP_STALL);

	assert(len <= 64);
	ept->buf = buf;
	ept->buflen = len;
	ept->state = EP_DATA;
	ept->epflags = 0;
	dev_unlock_notify(lo);
	return 0;
}

static int
emu_expect_in(struct usb_dev *dev, u8 ep, void *buf, u8 len)
{

	struct endpoint *ept;
	struct usb_lo *lo = driver(dev);

	dev_lock(lo);
	assert(getflags(lo, LO_ENABLED));
	assert((ep&0x80) != 0);
	ept = ep_get(lo, ep);
	tracef("expect in %u %u\n", ep, len);
	assert(ept->state == EP_NAK || ept->state == EP_STALL);

	assert(len <= 64);
	ept->buf = buf;
	ept->buflen = len;
	ept->state = EP_DATA;
	ept->epflags = 0;
	dev_unlock_notify(lo);
	return 0;
}

static int
emu_stall_ep(struct usb_dev *dev, u8 ep)
{
	struct endpoint *ept;
	struct usb_lo *lo = driver(dev);

	dev_lock(lo);
	assert(getflags(lo, LO_ENABLED));
	ept = ep_get(lo, ep);
	assert(ept->state == EP_NAK);
	ept->state = EP_STALL;
	dev_unlock_notify(lo);
	return 0;
}

const struct usb_driver host_emulator = {
	.init = emu_init,
	.set_addr = emu_set_addr,
	.init_ep = emu_init_ep,
	.expect_out = emu_expect_out,
	.expect_in = emu_expect_in,
	.stall_ep = emu_stall_ep,
};

static int
host_expect_zlp_in(struct usb_dev *dev)
{
	char buf[1];
	int rc;

	rc = host_in(dev, 0x80, buf, 0);
	if (rc <= 0)
		return rc;
	return -EPROTO;
}

static int
host_zlp_out(struct usb_dev *dev)
{
	char buf[1];
	int rc;

	rc = host_out(dev, 0, buf, 0);
	if (rc <= 0)
		return rc;
	return -EPROTO;
}

int
host_control(struct usb_dev *dev,
             const struct usb_setup *setup,
             void *dataphase)
{
	char *buf;
	int err;
	struct usb_lo *lo = driver(dev);
	struct endpoint *ept = ep_get(lo, 0);

	trace("host setup");

	dev_lock(lo);
	/* devices can't NAK setup packets,
	 * so we only need to wait for the
	 * SETUP bit to be cleared before
	 * we can start forcing data into
	 * the device */
	while (!getflags(lo, LO_ENABLED) ||
		getflags(lo, LO_RESET) ||
		(ept->epflags & EP_SETUP) ||
		ept->state == EP_DISABLED)
		dev_wait(lo);

	if (ept->state == EP_DATA)
		trace("usb-loopback: WARN: clobbering endpoint 0 data with setup");

	assert(ept->buf != NULL);
	assert((setup->datalen != 0) == (dataphase != NULL));
	buf = ept->buf;
	buf[0] = setup->rtype;
	buf[1] = setup->rcode;
	buf[2] = setup->value & 0xff;
	buf[3] = setup->value >> 8;
	buf[4] = setup->index & 0xff;
	buf[5] = setup->index >> 8;
	buf[6] = setup->datalen & 0xff;
	buf[7] = setup->datalen >> 8;
	ept->buflen = 8;
	ept->epflags = EP_INT|EP_SETUP;
	dev_unlock(lo);
	emu_raise(lo);
	
	/* now that we've done the setup phase,
	 * do the data and status phases */
	err = 0;
	if (USB_REQ_DIR(setup->rtype) == REQ_HOST_TO_DEV) {
		if (setup->datalen > 0) {
			if ((err = host_out_full(dev, 0, dataphase, setup->datalen)) < 0)
				return err;
		}
		return host_expect_zlp_in(dev);
	}
	if (setup->datalen > 0) {
		if ((err = host_in_full(dev, 0x80, dataphase, setup->datalen)) < 0)
			return err;
	}
	return host_zlp_out(dev);
}

int
host_reset(struct usb_dev *dev)
{
	trace("host reset");
	struct usb_lo *lo = driver(dev);
	dev_lock(lo);
	while (!getflags(lo, LO_ENABLED))
		dev_wait(lo);
	lo->flags |= LO_RESET;
	dev_unlock(lo);
	emu_raise(lo);

	dev_lock(lo);
	while (getflags(lo, LO_RESET))
		dev_wait(lo);
	dev_unlock(lo);
	return 0;
}

static void
wait_for_ep(struct usb_lo *lo, struct endpoint *ep)
{
	while (!getflags(lo, LO_ENABLED) ||
		getflags(lo, LO_RESET) ||
		ep->epflags ||
		ep->state == EP_NAK ||
		ep->state == EP_DISABLED)
		dev_wait(lo);
}

int
host_out(struct usb_dev *dev, u8 ep, const void *buffer, u8 len)
{
	struct usb_lo *lo = driver(dev);
	struct endpoint *ept = ep_get(lo, ep);
	u8 copiedlen;

	tracef("host out %u %u\n", ep, len);
	assert((ep&0x80) == 0);

	dev_lock(lo);
	wait_for_ep(lo, ept);
	if (ept->state == EP_STALL) {
		dev_unlock(lo);
		return -EIO;
	}

	assert(ept->buf);
	copiedlen = len;
	if (copiedlen > ept->buflen)
		copiedlen = ept->buflen;
	memmove(ept->buf, buffer, copiedlen);
	ept->buflen = copiedlen;

	ept->state = EP_NAK;
	ept->epflags = EP_INT;
	dev_unlock(lo);
	emu_raise(lo);
	return copiedlen;
}

int
host_in(struct usb_dev *dev, u8 ep, void *buffer, u8 maxlen)
{
	struct usb_lo *lo = driver(dev);
	struct endpoint *ept = ep_get(lo, ep);
	u8 copiedlen;

	tracef("host in %u %u\n", ep, maxlen);
	assert((ep&0x80) != 0);

	dev_lock(lo);
	wait_for_ep(lo, ept);
	if (ept->state == EP_STALL) {
		dev_unlock(lo);
		return -EIO;
	}

	assert(ept->buf);
	copiedlen = maxlen;
	if (copiedlen > ept->buflen) {
		trace("WARN: truncating IN message?");
		copiedlen = ept->buflen;
	}
	memmove(buffer, ept->buf, copiedlen);
	ept->state = EP_NAK;
	ept->epflags = EP_INT;
	dev_unlock(lo);
	emu_raise(lo);

	return copiedlen;
}
