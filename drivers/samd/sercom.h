#ifndef __SERCOM_DEF_H_
#define __SERCOM_DEF_H_
#include <bits.h>

/* CTRLA register fields used across SERCOM modes */
#define CTRLA_SWRST   0x1
#define CTRLA_ENABLE  0x2
#define CTRLA_RUNSTBY (1 << 7) /* run in standby */

/* SYNCBUSY register fields used across modes */
#define SYNC_SWRST (1UL << 0)
#define SYNC_ENABLE (1UL << 1)

/* sercome_mode are the mode bits that
 * can be OR'd into the CTRLA register
 * to set the device mode
 * (this occupies bits 2 through 4) */
enum sercom_mode {
	SERCOM_USART_EXT_CLK = 0,
	SERCOM_USART_INT_CLK = (1 << 2),
	SERCOM_SPI_SLAVE = (2 << 2),
	SERCOM_SPI_MASTER = (3 << 2),
	SERCOM_I2C_SLAVE = (4 << 2),
	SERCOM_I2C_MASTER = (5 << 2),
};

/* sercom modules are represent simply
 * as the base address of the peripheral's registers */
typedef ulong sercom_t;

static inline ulong
sercom_ctrla(const sercom_t sc)
{
	return sc + 0;
}

static inline ulong
sercom_ctrlb(const sercom_t sc)
{
	return sc + 4;
}

static inline ulong
sercom_syncbusy(const sercom_t sc)
{
	return sc + 0x1c;
}

/* sercom_reset() resets a sercom module */
void sercom_reset(const sercom_t sc);

/* sercom_enable() sets the enable bit on
 * the sercom module and waits for clock
 * synchronization */
void sercom_enable(const sercom_t sc);

#endif
