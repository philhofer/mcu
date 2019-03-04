#include <bits.h>
#include <assert.h>
#include <error.h>
#include <libc.h>
#include <board.h>
#include <idle.h>
#include <usb-internal.h>
#include <drivers/sam/usb.h>
#include <drivers/sam/port.h>
#include <drivers/sam/powermgr.h>
#include <drivers/sam/gclk.h>

#define USB_CTRLA     0x00
#define USB_SYNCBUSY  0x02
#define USB_QOSCTRL   0x03
#define USB_CTRLB     0x08
#define USB_DADD      0x0A
#define USB_STATUS    0x0C
#define USB_FSMSTATUS 0x0D
#define USB_FNUM      0x10
#define USB_INTENCLR  0x14
#define USB_INTENSET  0x18
#define USB_INTFLAG   0x1C
#define USB_EPINTSMRY 0x20
#define USB_DESCADD   0x24
#define USB_PADCAL    0x28
#define USB_EPCFGn    0x100
#define USB_EPSTATUSCLRn 0x104
#define USB_EPSTATUSSETn 0x105
#define USB_EPSTATUSn   0x106
#define USB_EPINTFLAGn  0x107
#define USB_EPINTENCLRn 0x108
#define USB_EPINTENSETn 0x109

#define CTRLA_SWRST    (1UL << 0)
#define CTRLA_ENABLE   (1UL << 1)
#define CTRLA_RUNSTDBY (1UL << 2)
#define CTRLA_MODE     (1UL << 7) /* host mode when set (not used here) */

#define SYNCBUSY_SWRST  (1UL << 0)
#define SYNCBUSY_ENABLE (1UL << 1)

#define PADCAL(trim, transn, transp) \
	((((u32)trim << 12) & 7) | (((u32)transn << 6) & 0x1f) | ((u32)transp & 0x1f))

#define DADD_EN (1UL << 7)

#define CTRLB_DETACH    (1UL << 0) /* set to 1 by default; clear in order to attach to the bus */
#define CTRLB_UPRSM     (1UL << 1) /* upstream resume */
#define CTRLB_FULLSPEED (0UL << 2)
#define CTRLB_LOWSPEED  (1UL << 2)
#define CTRLB_NREPLY    (1UL << 4) /* enable the 'no reply' feature (ignore everything except SETUP) */
#define CTRLB_GNAK      (1UL << 9) /* global NAK */
#define CTRLB_LPM_NONE  (0UL << 10) /* no link power management handshake */
#define CTRLB_LPM_ACK   (1UL << 10) /* ACK link power management */
#define CTRLB_LPM_NYET  (2UL << 10) /* NYET link power management */

#define TRANSP_SHIFT 0
#define TRANSN_SHIFT 6
#define TRIM_SHIFT   12

/* possible values for the USB_FSMSTATUS register */
#define FSM_OFF      0x01
#define FSM_ON       0x02
#define FSM_SUSPEND  0x04
#define FSM_DNRESUME 0x10
#define FSM_UPRESUME 0x20
#define FSM_RESET    0x40

/* possible values for INT{ENCLR,ENSET,FLAG} registers */
#define INT_SUSPEND (1UL << 0) /* suspend interrupt */
#define INT_SOF     (1UL << 2) /* start-of-frame interrupt */
#define INT_EORST   (1UL << 3) /* end-of-reset interrupt */
#define INT_WAKEUP  (1UL << 4) /* wakeup interrupt */
#define INT_EORSM   (1UL << 5) /* end-of-resume interrupt */
#define INT_UPRSM   (1UL << 6) /* upstream resume */
#define INT_RAMACER (1UL << 7) /* ram access error interrupt */
#define INT_LPMNYET (1UL << 8) /* link power management not yet interrupt */
#define INT_LPMSUSP (1UL << 9) /* link power management suspend interrupt */

#define EP1_CTRLIN (1UL << 4) /* control IN */
#define EP1_ISOIN  (2UL << 4) /* isochronous IN */
#define EP1_BULKIN (3UL << 4) /* bulk IN */
#define EP1_INTRIN (4UL << 4) /* interrupt IN */
#define EP1_DBOUT  (5UL << 4) /* dual-bank OUT (type set in EP0) */

#define EP0_CTRLOUT (1UL) /* control OUT */
#define EP0_ISOOUT  (2UL) /* isochronous OUT */
#define EP0_BULKOUT (3UL) /* bulk OUT */
#define EP0_INTROUT (4UL) /* interrupt OUT */
#define EP0_DBIN    (5UL) /* dual-bank IN (type set in EP1) */

#define EP_DTGLOUT  (1UL << 0) /* expect PID of next OUT transaction to be 0 */
#define EP_DTGLIN   (1UL << 1) /* expect PID of next input tx to be 1 */
#define EP_CURBK    (1UL << 2) /* current bank (0 or 1) for next single/multi packet */
#define EP_STALLRQ0 (1UL << 4) /* send STALL when receiving setup on bank 0 */
#define EP_STALLRQ1 (1UL << 5) /* send STALL when receiving setup on bank 1 */
#define EP_BK0RDY   (1UL << 6) /* bank 0 is ready */
#define EP_BK1RDY   (1UL << 7) /* bank 1 is ready */

#define EPINT_TRCPT0  (1UL << 0) /* transfer complete interrupt */
#define EPINT_TRCPT1  (1UL << 1)
#define EPINT_TRFAIL0 (1UL << 2) /* transfer failed interrupt */
#define EPINT_TRFAIL1 (1UL << 3)
#define EPINT_RXSTP   (1UL << 4) /* received setup interrupt */
#define EPINT_STALL0  (1UL << 5) /* transmit stall */
#define EPINT_STALL1  (1UL << 6)

/* TODO: we can support more descriptors (8)
 * than this, but it eats up unnecessary RAM... */
#define MAX_ENDPOINTS 3

/* Driver Notes
 *
 * The SAM usb peripheral handles the transport-level
 * concerns of the USB protocol. It has a built-in
 * DMA engine which feeds data to/from dual-banked
 * "endpoint descriptors." The endpoint descriptors
 * are laid out in RAM and contain pointers to the buffers
 * that the peripheral needs to shuttle about.
 *
 * For each endpoint, bank 0 is typically for OUT
 * messages (and, for endpoint 0, SETUP messages).
 * (The peripheral supports using both banks' buffers
 * as a single buffer, but we're ignoring that feature for
 * now.) Similarly, bank 1 is for IN messages.
 * The BKxRDY flags (BK0RDY, BK1RDY) indicate that the
 * buffers in the bank contain valid data -- note that
 * this means these flags have opposite meaning in terms
 * of the _completion_ of a DMA depending on which bank
 * you're considering.
 *
 * Also note that on endpoint 0, the RXSTP interrupt
 * and the DMA of the SETUP packet IGNORE THE BK0RDY FLAG,
 * so that means a host can clobber the buffer for that
 * descriptor regardless of whether or not it is set up.
 * Consequently, we always treat endpoint 0 as though
 * it is 'armed' to receive data.
 * It seems like this also means we have to process SETUP
 * packets in interrupt context.
 *
 * TODO: change MAX_ENDPOINTS? It wastes memory
 * if we make it too large...
 */

struct endpoint {
	/* bank 0 (typically for OUT) */
	void *base0;
	u32   pcksize0;
	u16   extreg;
	u8    status_bk0;
	u8    __reserved0;
	u32   __reserved1;

	/* bank 1 (typically for IN) */
	void *base1;
	u32   pcksize1;
	u16   __reserved2;
	u8    status_bk1;
	u8    __reserved3;
	u32   __reserved4;
} __attribute__((packed));

static_assert(sizeof(struct endpoint) == 32);

/* RAM descriptor base */
struct {
	struct endpoint eps[MAX_ENDPOINTS];
} sam_usb_descriptor;

#define PCKSIZE_MULTI_SHIFT 14
#define PCKSIZE_AUTO_ZLP    (1UL << 31)
#define PCKSIZE_64          (3UL << 28)

#define BK_CRCERR    (1UL << 0) /* crc error */
#define BK_ERRORFLOW (1UL << 1) /* flow error (for OUT, a NAK has been sent) */

static inline void
set_descriptor(void)
{
	write32(USB_BASE + USB_DESCADD, (u32)(&sam_usb_descriptor));
}

static inline u8
epcfg_get(unsigned ep)
{
	return read8(USB_BASE + USB_EPCFGn + (ep * 0x20));
}

static inline void
epcfg_set(unsigned ep, u8 cfg)
{
	write8(USB_BASE + USB_EPCFGn + (ep * 0x20), cfg);
}

static void
set_address(struct usb_dev *dev, u8 addr)
{
	write8(USB_BASE + USB_DADD, addr | DADD_EN); /* ENABLE */
}

static inline void
epstatus_set(unsigned epn, u8 flags)
{
	write8(USB_BASE + USB_EPSTATUSSETn + (epn * 0x20), flags);
}

static inline u8
epstatus_get(unsigned epn)
{
	return read8(USB_BASE + USB_EPSTATUSn + (epn * 0x20));
}

static inline void
epstatus_clr(unsigned epn, u8 flags)
{
	write8(USB_BASE + USB_EPSTATUSCLRn + (epn * 0x20), flags);
}

static inline u8
epint_get(unsigned epn)
{
	return read8(USB_BASE + USB_EPINTFLAGn + (epn * 0x20));
}

static inline void
epint_enable(unsigned epn, u8 flags)
{
	write8(USB_BASE + USB_EPINTENSETn + (epn * 0x20), flags);
}

static inline void
epint_clr(unsigned epn, u8 flags)
{
	write8(USB_BASE + USB_EPINTFLAGn + (epn * 0x20), flags);
}

static inline u8
int_get(void)
{
	return read8(USB_BASE + USB_INTFLAG);
}

static inline void
int_enable(u8 flags)
{
	write8(USB_BASE + USB_INTENSET, flags);
}

static inline void
int_clr(u8 flags)
{
	write8(USB_BASE + USB_INTFLAG, flags);
}

static void dev_init(struct usb_dev *dev);

static int expect_out(struct usb_dev *dev, u8 ep, void *data, u8 len);
static int expect_in(struct usb_dev *dev, u8 ep, void *data, u8 len);

void
sam_usb_init(struct usb_dev *dev)
{
	/* unmask the device in the power manager */
	powermgr_apb_mask(apb_num(USB_BASE), USB_APB_INDEX, true);

	/* assigned the DP and DM pins to the USB peripheral */
	port_pmux_pin(USB_PINGRP, USB_DP_PIN, USB_PINROLE);
	port_pmux_pin(USB_PINGRP, USB_DM_PIN, USB_PINROLE);

	/* use the same generator as the CPU
	 * clock for the USB device */
	/* NOTE: the cpu needs to be running
	 * on 48MHz in order for the USB module
	 * to work correctly */
	gclk_enable_clock(0, CLK_DST_USB);

	write8(USB_BASE + USB_CTRLA, CTRLA_SWRST);
	while (read8(USB_BASE + USB_SYNCBUSY) & SYNCBUSY_SWRST) ;

	/* calibrate from NVM */
	write16(USB_BASE + USB_PADCAL,
			(usb_trim() << TRIM_SHIFT)|
			(usb_transn() << TRANSN_SHIFT)|
			(usb_transp() << TRANSP_SHIFT));

	/* make sure we're detached from the bus
	 * while we do the rest of the initialization
	 * (we're also implicitly choosing Full Speed here... */
	write16(USB_BASE + USB_CTRLB, CTRLB_GNAK|CTRLB_DETACH);
	write8(USB_BASE + USB_CTRLA, CTRLA_ENABLE|CTRLA_RUNSTDBY);
	while (read8(USB_BASE + USB_SYNCBUSY) & SYNCBUSY_ENABLE) ;

	dev_init(dev);

	int_clr(INT_EORST);
	int_enable(INT_EORST);
	irq_enable_num(USB_IRQ_NUM);

	/* attach; should be OK now */
	write16(USB_BASE + USB_CTRLB, CTRLB_LPM_NYET);
}

static void
dev_init(struct usb_dev *dev)
{

	struct endpoint *ctrl = &sam_usb_descriptor.eps[0];
	set_descriptor();

	ctrl->base0 = dev->ctrlbuf;
	ctrl->pcksize0 = PCKSIZE_64 | (64 << PCKSIZE_MULTI_SHIFT);
	ctrl->status_bk0 = 0;
	ctrl->base1 = dev->ctrlbuf;
	ctrl->pcksize1 = PCKSIZE_64;
	ctrl->status_bk1 = 0;

	epint_enable(0, EPINT_RXSTP);
	usb_reset(dev);
}

static int
stall_ep(struct usb_dev *dev, u8 ep)
{
	u8 epn = ep & 0x3f;
	assert(epn < MAX_ENDPOINTS);
	epstatus_set(epn, (ep & 0x80) ? EP_STALLRQ1 : EP_STALLRQ0);
	return 0;
}

static int
init_ep(struct usb_dev *dev, u8 ep, enum ep_type type)
{
	u8 epn = ep & 0x3f;
	u8 cfg;

	/* don't let folks configure 
	 * bad endpoints */
	assert(epn < MAX_ENDPOINTS);

	cfg = epcfg_get(epn);

	if (ep & 0x80) {
		switch (type) {
		case EP_CONTROL:
			assert(epn == 0);
			cfg |= EP1_CTRLIN;
			break;
		case EP_ISOCHRONOUS:
			cfg |= EP1_ISOIN;
			break;
		case EP_BULK:
			cfg |= EP1_BULKIN;
			break;
		case EP_INTERRUPT:
			cfg |= EP1_INTRIN;
			break;
		}
		epstatus_clr(epn, EP_DTGLIN);
	} else {
		switch (type) {
		case EP_CONTROL:
			assert(epn == 0);
			cfg |= EP0_CTRLOUT;
			break;
		case EP_ISOCHRONOUS:
			cfg |= EP0_ISOOUT;
			break;
		case EP_BULK:
			cfg |= EP0_BULKOUT;
			break;
		case EP_INTERRUPT:
			cfg |= EP0_INTROUT;
			break;
		}
		epstatus_clr(epn, EP_DTGLOUT);
	}

	epcfg_set(epn, cfg);

	/* ensure that no DMAs happen on these descriptors yet */
	epstatus_set(epn, EP_BK0RDY);
	epstatus_clr(epn, EP_BK1RDY|EP_STALLRQ1|EP_STALLRQ0);

	if (ep == 0)
		epint_enable(0, EPINT_RXSTP);
	return 0;
}

static int
expect_out(struct usb_dev *dev, u8 ep, void *buf, u8 len)
{
	struct endpoint *ept;
	u8 enable;

	assert((ep & 0x80) == 0 && ep < MAX_ENDPOINTS && len <= 64);
	assert(((ulong)buf & 3) == 0); /* alignment */

	if ((epstatus_get(ep)&EP_BK0RDY) == 0)
		return -EAGAIN;

	ept = &sam_usb_descriptor.eps[ep];
	ept->base0 = buf;
	ept->pcksize0 = PCKSIZE_64 | (64 << PCKSIZE_MULTI_SHIFT);
	ept->status_bk0 = 0;

	enable = EPINT_TRCPT0;
	if (ep == 0)
		enable |= EPINT_RXSTP;

	/* interrupts have to be cleared _before_
	 * the bank status is set; otherwise we
	 * could end up clearing a valid interrupt */
	epint_clr(ep, enable);
	epint_enable(ep, enable);
	epstatus_clr(ep, EP_BK0RDY|EP_STALLRQ0);

	return 0;
}

static int
expect_in(struct usb_dev *dev, u8 ep, void *buf, u8 len)
{
	struct endpoint *ept;

	assert((ep & 0x80) != 0 && (ep & 0x3f) < MAX_ENDPOINTS && len <= 64);
	ep &= 0x3f;

	assert(((ulong)buf & 3) == 0); /* alignment */

	if (epstatus_get(ep)&EP_BK1RDY)
		return -EAGAIN;

	ept = &sam_usb_descriptor.eps[ep];
	ept->base1 = buf;
	ept->pcksize1 = PCKSIZE_64 | len;
	ept->status_bk1 = 0;

	epint_clr(ep, EPINT_TRCPT1|EPINT_TRFAIL1);
	epint_enable(ep, EPINT_TRCPT1);

	epstatus_set(ep, EP_BK1RDY);
	return 0;
}

static void
ep_irq(struct usb_dev *dev, int i)
{
	struct endpoint *ept = &sam_usb_descriptor.eps[i];
	u8 flags = epint_get(i);
	void *buf;
	uchar len;

	epint_clr(i, flags);

	if ((flags & EPINT_RXSTP)) {
		assert(i == 0);
		usb_handle_setup(dev, ept->base0, ept->pcksize0 & 0x7f);
		return;
	}
	if ((flags & (EPINT_TRCPT0))) {
		if (i == 0) {
			usb_ep0_rx(dev, 0);
		} else {
			buf = ept->base0;
			len = ept->pcksize0 & 0x7f;
			ept->base0 = NULL;
			ept->pcksize0 = 0;
			ept->status_bk0 = 0;
			dev->class->ep_rx(dev, i, buf, len, 0);
		}
	}
	if ((flags & (EPINT_TRCPT1))) {
		if (i == 0) {
			usb_ep0_tx(dev, 0);
		} else {
			ept->base1 = NULL;
			ept->pcksize1 = 0;
			ept->status_bk1 = 0;
			dev->class->ep_tx(dev, i | 0x80, 0);
		}
	}
}

/* peripheral interrupts are largely handled
 * outside of the actual interrupt handler;
 * the handler simply queues the 'bottom half'
 * and disables USB interrupts until the bottom
 * half enables them again */
struct work sam_usb_work;

static void sam_usb_bh(struct usb_dev *);

void
sam_usb_irq(struct usb_dev *dev)
{
	sam_usb_work.udata = dev;
	sam_usb_work.func = (void(*)(void *))sam_usb_bh;
	schedule_work(&sam_usb_work);
	irq_disable_num(USB_IRQ_NUM);
}

static void
sam_usb_bh(struct usb_dev *dev)
{
	u8 status = int_get();
	u32 epirq;

	/* any additional interrupts that have been
	 * raised since this work was scheduled are
	 * duplicates; we'll be handling those causes
	 * too. */
	irq_clear_pending(USB_IRQ_NUM);

	/* paranoia: clear everything */
	int_clr(status);

	if (status & INT_EORST) {
		/* reset everything */
		dev_init(dev);
		irq_enable_num(USB_IRQ_NUM);
		return;
	}

	epirq = read32(USB_BASE + USB_EPINTSMRY);
	for (unsigned i=0; i<MAX_ENDPOINTS; i++) {
		if (epirq & (1 << i))
			ep_irq(dev, i);
	}
	irq_enable_num(USB_IRQ_NUM);
}

const struct usb_driver sam_usb_driver = {
	.init = sam_usb_init,
	.set_addr = set_address,
	.init_ep = init_ep,
	.expect_out = expect_out,
	.expect_in = expect_in,
	.stall_ep = stall_ep,
};
