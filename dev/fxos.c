#include <i2c.h>
#include <arch.h>
#include <error.h>
#include <extirq.h>
#include <dev/fxos.h>

#define DEV_WHOAMI 0xC7
#define DEV_ADDR   0x1F

#define STATUS     0x00
#define ACCEL_BASE 0x01 /* 6 bytes of big-endian x,y,z */
#define F_SETUP    0x09
#define TRIG_CFG   0x0A
#define SYSMOD     0x0B
#define AINT_SRC   0x0C
#define WHO_AM_I   0x0D
#define XYZ_CFG    0x0E
#define HP_FILT    0x0F
#define CTRL_REG1  0x2A
#define CTRL_REG2  0x2B
#define CTRL_REG3  0x2C
#define CTRL_REG4  0x2D
#define CTRL_REG5  0x2E
#define AOFF_X     0x2F /* accel. offset, followed by y, z */
#define M_STATUS   0x32
#define MAG_BASE   0x33 /* 6 bytes of big-endian x,y,z */
#define ACMP_X     0x39 /* accelerometer x, y, z; time-aligned with magnetometer */
#define MOFF_X     0x3F /* mag. offset, followed by y, z */
#define MMAX_X     0x45 /* mag. maximum, followed by y,z */
#define MMIN_X     0x4B /* mag. minimum, followed by y,z */
#define TEMP       0x51
#define CTRLM_REG1 0x5B
#define CTRLM_REG2 0x5C
#define CTRLM_REG3 0x5D
#define MINT_SRC   0x5E

/* AINT_SRC flags */
#define AINT_DRDY  (1U << 0) /* data ready */
#define AINT_VECM  (1U << 1) /* vector-magnitude trigger */
#define AINT_FFMT  (1U << 2) /* free-fall motion trigger */
#define AINT_PULSE (1U << 3) /* pulse detect */
#define AINT_LNDPR (1U << 4) /* landscape/portrait trigger */
#define AINT_TRANS (1U << 5) /* transient acceleration */
#define AINT_FIFO  (1U << 6) /* fifo threshold */
#define AINT_ASLP  (1U << 7) /* auto sleep/wake */

#define CTRL1_ACTIVE  (1U << 0)
#define CTRL1_FREAD   (1U << 1) /* "fast-read" mode (8-bit data) */
#define CTRL1_LNOISE  (1U << 2) /* low-noise mode; only works for 2g and 4g range */
#define CTRL1_ODR(x)  (((x)&7)<<3) /* output data rate */
#define CTRL1_ASLP(x) (((x)&3)<<6) /* auto sleep/wakeup rate */

/* valid values for CTRL1_ODR();
 * note that the ODR is cut in half when
 * running in hybrid mode! */
#define ODR_800MHZ 0
#define ODR_400MHZ 1
#define ODR_200MHZ 2
#define ODR_100MHZ 3
#define ODR_50MHZ  4
#define ODR_12MHZ  5 /* 12.5 MHz */
#define ODR_6MHZ   6 /* 6.25 MHz */
#define ODR_1MHZ   7 /* 1.5625 MHz */

#define CTRL2_MODS(x)  ((x)&3)      /* wake mode OSR */
#define CTRL2_SLPE     (1U << 2)    /* sleep mode */
#define CTRL2_SMODS(x) (((x)&3)<<3) /* sleep mode OSR */
#define CTRL2_RST      (1U << 6)    /* reset */
#define CTRL2_ST       (1U << 7)    /* self-test */

/* values for CTRL2_(S)MODS() */
#define OSR_NORMAL  0
#define OSR_LONOISE 1
#define OSR_HIRES   2
#define OSR_LOPOWER 3

#define CTRL3_PP_OD (1U << 0) /* push-pull / open-drain */
#define CTRL3_IPOL  (1U << 1) /* interrupt output polarity */
#define CTRL3_VECM  (1U << 2) /* vector-magnitude enabled in sleep mode */
#define CTRL3_FFMT  (1U << 3) /* free-fall enabled in sleep mode */
#define CTRL3_PULSE (1U << 4) /* pulse detect enabled in sleep mode */
#define CTRL3_LNDPR (1U << 5) /* landscape/portrait enabled in sleep mode */
#define CTRL3_TRANS (1U << 6) /* transient detect enabled in sleep mode */
#define CTRL3_FIFO  (1U << 7) /* fifo is blocked on wake-to-sleep transition */

/* CTRL4 is the interrupt-enable register */
#define CTRL4_DRDY  (1U << 0)
#define CTRL4_VECM  (1U << 1)
#define CTRL4_FFMT  (1U << 2)
#define CTRL4_PULSE (1U << 3)
#define CTRL4_LNDPR (1U << 4)
#define CTRL4_TRANS (1U << 5)
#define CTRL4_FIFO  (1U << 6)
#define CTRL4_ASLP  (1U << 7)

/* CTRL5 is the interrupt-routing register;
 * setting the bit moves the interrupt to pin 1
 * instead of pin 2 (default) */
#define CTRL5_DRDY  (1U << 0)
#define CTRL5_VECM  (1U << 1)
#define CTRL5_FFMT  (1U << 2)
#define CTRL5_PULSE (1U << 3)
#define CTRL5_LNDPR (1U << 4)
#define CTRL5_TRANS (1U << 5)
#define CTRL5_FIFO  (1U << 6)
#define CTRL5_ASLP  (1U << 7)

/* XYZ_CONFIG register bits */
#define XYZ_FS(x)  ((x)&3)   /* one of the FS_xx scale values */
#define XYZ_HPFILT (1U << 4) /* high-pass filter */

#define FS_2G 0
#define FS_4G 1
#define FS_8G 2

#define MCTRL1_HMS(x) ((x)&3)      /* hybrid mode setting */
#define MCTRL1_ODR(x) (((x)&7)<<2) /* output data rate */
#define MCTRL1_OST    (1U << 5)    /* one-shot reading */
#define MCTRL1_RST    (1U << 6)    /* degauss reset */
#define MCTRL1_ACAL   (1U << 7)    /* automatic hard-iron calibration */

#define MCTRL2_RSTF(x) ((x)&3)   /* degauss requency */
#define MCTRL2_MINMAX  (1U << 2) /* min/max reset */
#define MCTRL2_MMTHDIS (1U << 3) /* disable min/max on threshold */
#define MCTRL2_MMDIS   (1U << 4) /* disable min/max */
#define MCTRL2_AUTOINC (1U << 5) /* hybrid auto-increment mode */

#define MCTRL3_XYZUPD  (1U << 3) /* only reference values is updated on threshold trigger */
#define MCTRL3_OSR(x)  (((x)&3)<<4)
#define MCTRL3_RAW     (1U << 7) /* disable hard-iron compensation in output */

/* values for MINT_SRC register */
#define MINT_DRDY (1U << 0)
#define MINT_VECM (1U << 1)
#define MINT_THS  (1U << 2)


int
fxos_enable(struct i2c_dev *dev)
{
	u8 buf[4];
	int err;

	err = i2c_read_sync(dev, DEV_ADDR, WHO_AM_I, buf, 1);
	if (err)
		return err;
	if (buf[0] != DEV_WHOAMI)
		return -EINVAL;

	/* program CTRL2-5 */
	buf[0] = CTRL2_MODS(OSR_NORMAL);
	buf[1] = 0; /* push-pull active-low */
	buf[2] = CTRL4_DRDY; /* enable DRDY */
	buf[3] = 0; /* deliver DRDY on INT2 */
	err = i2c_write_sync(dev, DEV_ADDR, CTRL_REG2, buf, 4);
	if (err)
		return err;

	/* set scale to +/- 4G */
	buf[0] = XYZ_FS(FS_4G);
	err = i2c_write_sync(dev, DEV_ADDR, XYZ_CFG, buf, 1);
	if (err)
		return err;

	/* program MCTRL1-3 
	 * note that enabling hybrid mode cuts the ODR in half */
	buf[0] = MCTRL1_HMS(3)|MCTRL1_ODR(ODR_400MHZ)|MCTRL1_ACAL;
	buf[1] = 0;
	buf[2] = 0;
	err = i2c_write_sync(dev, DEV_ADDR, CTRLM_REG1, buf, 3);
	if (err)
		return err;

	/* activate the device */
	buf[0] = CTRL1_ACTIVE|CTRL1_ODR(ODR_400MHZ)|CTRL1_LNOISE;
	return i2c_write_sync(dev, DEV_ADDR, CTRL_REG1, buf, 1);
}

static i16
getraw(u8 *data)
{
	return ((i16)data[0] << 8)|((i16)data[1]);
}

static void
fxos_post_read(int err, void *ctx)
{
	struct fxos_state *state = ctx;
	if ((state->last_err = err))
		return;


	state->mag.x = getraw(state->buf);
	state->mag.y = getraw(state->buf + 2);
	state->mag.z = getraw(state->buf + 4);
	/* accel is 14 bits; scale to 16 */
	state->accel.x = getraw(state->buf + 6)<<2;
	state->accel.y = getraw(state->buf + 8)<<2;
	state->accel.z = getraw(state->buf + 10)<<2;
	state->last_update = getcycles();
}

int
fxos_read_state(struct i2c_dev *dev, struct fxos_state *state)
{
	/* read accel x,y,z + mag x,y,z all in one go */
	return i2c_read_reg(dev, DEV_ADDR, MAG_BASE, state->buf, 12, fxos_post_read, state);	
}

