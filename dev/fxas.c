#include <bits.h>
#include <arch.h>
#include <i2c.h>
#include <gpio.h>
#include <error.h>
#include <gyro.h>
#include <extirq.h>
#include <dev/fxas.h>

#define DEV_ADDR   0x21
#define DEV_WHOAMI 0xD7

#define STATUS    0x00
/* registers 1 through 6 inclusive are x,y,z in big-endian byte order */
#define DR_STATUS 0x07
#define F_STATUS  0x08 /* fifo status */
#define F_SETUP   0x09 /* fifo setup */
#define F_EVENT   0x0B /* fifo event */
#define INT_SRC_FLAG 0x0B /* interrupt source flag */
#define WHO_AM_I  0x0C
#define CTRL_REG0 0x0D
#define RT_CFG    0x0E
#define RT_SRC    0x0F
#define RT_THS    0x10
#define RT_COUNT  0x11
#define TEMP      0x12
#define CTRL_REG1 0x13
#define CTRL_REG2 0x14
#define CTRL_REG3 0x15

/* STATUS register values when in active mode */
#define DR_STATUS_XDR (1U << 0) /* data ready */
#define DR_STATUS_YDR (1U << 1)
#define DR_STATUS_ZDR (1U << 2)
#define DR_STATUS_ZYXDR (1U << 3)
#define DR_STATUS_XOW (1U << 4) /* data overwritten (not polling fast enough) */
#define DR_STATUS_YOW (1U << 5)
#define DR_STATUS_ZOW (1U << 6)
#define DR_STATUS_ZYXOW (1U << 7)

#define F_MODE_NOFIFO   0
#define F_MODE_CIRCULAR (1U << 6)
#define F_MODE_STOP     (1U << 7)
#define F_WMRK(x)       ((x) & 0x1f)

#define F_STATUS_OVF   (1U << 7)
#define F_STATUS_WMKF  (1U << 6)
#define F_STATUS_CNT(x) ((x) & 0x1f)

/* flags in INT_SRC_FLAG */
#define INT_BOOTEND  (1U << 3)
#define INT_SRC_FIFO (1U << 2)
#define INT_SRC_RT   (1U << 1)
#define INT_SRC_DRDY (1U << 0)

#define CTRL0_BW(x)  (((x)&3)<<6)  /* LPF setting */
#define CTRL0_SPIW   (1U << 5)
#define CTRL0_SEL(x) (((x)&3)<<3)  /* HPF setting */
#define CTRL0_HPF_EN (1U << 2)     /* HPF enable */
#define CTRL0_FS(x)  ((x)&3)

#define FS_2000DPS 0
#define FS_1000DPS 1
#define FS_500DPS  2
#define FS_250DPS  3

/* 500DPS fits within our +/- 16 rad/s scheme,
 * so it also maintains the most precision when
 * we perform the Q0.15 conversion */
#define SELECTED_FS FS_500DPS

#define CTRL1_RST   (1U << 6) /* software reset */
#define CTRL1_ST    (1U << 5) /* self-test */
#define CTRL1_ODR(x) (((x)&7)<<2)
#define CTRL1_ACTIVE (1U << 1)
#define CTRL1_READY  (1U << 0)

/* output data rate choices */
#define ODR_800HZ 0
#define ODR_400HZ 1
#define ODR_200HZ 2
#define ODR_100HZ 3
#define ODR_50HZ  4
#define ODR_25HZ  3
#define ODR_12HZ  2

#define CTRL2_CFG_FIFO (1U << 7) /* send FIFO interrupt to pin1 instead of pin2 */
#define CTRL2_INT_FIFO (1U << 6) /* enable FIFO interrupt */
#define CTRL2_CFG_RT   (1U << 5) /* send RT interrupt to pin2 instead of pin2 */
#define CTRL2_INT_RT   (1U << 4) /* enable RT interrupt */
#define CTRL2_CFG_DRDY (1U << 3)
#define CTRL2_INT_DRDY (1U << 2)
#define CTRL2_IPOL     (1U << 1) /* set logic active high instead of active low */
#define CTRL2_PP_OD    (1U << 0) /* set open-source/open-drain instead of push-pull output driver */

static inline i16
getraw(uchar *raw)
{
	u16 v;
	v = ((u16)raw[0] << 8) | (u16)raw[1];
	return (i16)v;
}

/* convert raw measurement to
 * the 16rad/s internal scale */
static inline mrads
raw_to_mrad(i16 in, unsigned scale)
{
	i32 ext = (i32)in;

	/* we've sign-extended the 16-bit signed
	 * raw value into 32 bits, which means we
	 * can multiply it by up to 2^16 without
	 * overflowing the value
	 *
	 * our internal scale is 1lsb = 27.98 mdps
	 *
	 * multiplicands are computed as (mdps/27.98)*(2^15) */
	switch (scale) {
	case FS_2000DPS:
		/* 62.5 mdps ... will overflow */
		ext = (ext * 73195) >> 15;
		break;
	case FS_1000DPS:
		/* 32.25 mdps ... will overflow */
		ext = (ext * 37767) >> 15;
		break;
	case FS_500DPS:
		/* 16.125 mdps */
		ext = (ext * 18884) >> 15;
		break;
	case FS_250DPS:
		/* etc. */
		ext = (ext * 9442) >> 15;
		break;
	}
	if (ext > 0x7fff)
		return 0x7fff;
	if (ext < 0 && -ext > 0x10000)
		return -0x7fff;
	return (mrads)ext;
}

int
fxas_enable(struct i2c_dev *dev)
{
	int err;
	uchar v;

	err = i2c_read_sync(dev, DEV_ADDR, WHO_AM_I, &v, 1);
	if (err < 0)
		return err;
	if (v != DEV_WHOAMI)
		return -EINVAL;

	v = CTRL0_BW(0)|CTRL0_FS(SELECTED_FS);
	err = i2c_write_sync(dev, DEV_ADDR, CTRL_REG0, &v, 1);
	if (err < 0)
		return err;

	v = CTRL1_ACTIVE|CTRL1_READY|CTRL1_ODR(ODR_200HZ);
	err = i2c_write_sync(dev, DEV_ADDR, CTRL_REG1, &v, 1);
	if (err < 0)
		return err;

	/* configure DRDY as push-pull / active-low */
	v = CTRL2_INT_DRDY;
	err = i2c_write_sync(dev, DEV_ADDR, CTRL_REG2, &v, 1);
	return err;
}

static void
fxas_post_tx(int err, void *ctx)
{
	struct fxas_state *dst = ctx;
	if (err) {
		dst->last_err = err;
		return;
	}
	dst->gyro.x = raw_to_mrad(getraw(dst->buf+0), SELECTED_FS);
	dst->gyro.y = raw_to_mrad(getraw(dst->buf+2), SELECTED_FS);
	dst->gyro.z = raw_to_mrad(getraw(dst->buf+4), SELECTED_FS);
	dst->last_update = getcycles();
}

int
fxas_read_state(struct i2c_dev *dev, struct fxas_state *dst)
{
	return i2c_read_reg(dev, DEV_ADDR, 1, dst->buf, 6, fxas_post_tx, dst);
}

