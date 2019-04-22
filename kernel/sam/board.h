#ifndef __SAM_D21_H_
#define __SAM_D21_H_
#include <bits.h>
#include <arch.h>

/* driver bus addresses for singleton
 * drivers just get def'd here and used
 * by the drivers directly */
#define PM_BASE      0x40000400
#define SYSCTRL_BASE 0x40000800
#define GCLK_BASE    0x40000C00
#define WDT_BASE     0x40001000
#define RTC_BASE     0x40001400
#define EIC_BASE     0x40001800
#define DSU_BASE     0x41002000
#define PORT_BASE    0x41004400
#define DMAC_BASE    0x41004800
#define USB_BASE     0x41005000
#define MTB_BASE     0x41006000
#define SYSCTRL_BASE 0x40000800
#define NVMCTRL_BASE 0x41004000
#define EVSYS_BASE   0x42000400

/* peripheral bus and pin multiplexing constants */
#define ABP_A 0
#define APB_B 1
#define ABP_C 2
#define ROLE_A 0
#define ROLE_B 1
#define ROLE_C 2
#define ROLE_D 3
#define ROLE_E 4
#define ROLE_F 5
#define APB_A_BASE 0x40000000
#define APB_B_BASE 0x41000000
#define APB_C_BASE 0x42000000

/* PORT pin groups */
#define PINGRP_A 0
#define PINGRP_B 1

/* USB peripheral configuration */
#define USB_APB_INDEX 5
#define USB_PINGRP    PINGRP_A
#define USB_SOF_PIN   23
#define USB_DM_PIN    24
#define USB_DP_PIN    25
#define USB_PINROLE   6
#define USB_IRQ_NUM   7

/* EIC peripheral configuration */
#define EIC_MAX     18
#define EIC_IRQ_NUM 4
#define EIC_PINROLE 0

/* values for GENCTRL.SRC */
#define CLK_SRC_XOSC      0x0
#define CLK_SRC_GCLKIN    0x1 /* generator input pad */
#define CLK_SRC_GCLKGEN1  0x2 /* generator 1 output */
#define CLK_SRC_OSCULP32K 0x3
#define CLK_SRC_OSC32K    0x4
#define CLK_SRC_XOSC32K   0x5
#define CLK_SRC_OSC8M     0x6
#define CLK_SRC_DFLL48M   0x7
#define CLK_SRC_FDPLL96M  0x8

/* some of the generic clocks;
 * in general peripherals will know their
 * generic clock and so they don't all need
 * to be enumerated here */
#define CLK_DST_DFLL48MREF 0x00
#define CLK_DST_DPLL       0x01
#define CLK_DST_DPLL32K    0x02
#define CLK_DST_WDT        0x03
#define CLK_DST_RTC        0x04
#define CLK_DST_EIC        0x05
#define CLK_DST_USB        0x06
#define CLK_DST_SERCOMSLOW 0x13

/* for now we use these two clock generators */
#define CLK_GEN_MAIN   0x00
#define CLK_GEN_ULP32K 0x02

static inline unsigned
apb_num(ulong addr)
{
	switch (addr >> 24) {
	case 0x40:
		return 0;
	case 0x41:
		return 1;
	case 0x42:
		return 2;
	default:
		bkpt();
		return 0;
	}
}

/* accessors for USB calibration in
 * the NVM Calibration Area  (used by drivers) */
unsigned usb_trim(void);
unsigned usb_transn(void);
unsigned usb_transp(void);

#endif