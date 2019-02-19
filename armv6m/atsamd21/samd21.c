#include <bits.h>
#include <board.h>
#include <drivers/sam/sercom.h>

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

/* NOTE: the way the SERCOM pins are mux'd, you won't be able to
 * use all 6 units with all 4 pads unless you have the SAMD21J (64-pin) variant.
 * Since there are two sercom pin 'roles,' you can peek at the datasheet and
 * change the table below if you want to use an alternate pin muxing. This
 * muxing was selected to prevent the peripheral pins from overlapping. */
const struct sercom_config sercoms[6] = {
	{.base = APB_C_BASE + 0x0800, .apb_index = 2, .gclk = 0x14, .irq = 9,  .pingrp = PINGRP_A, .pinrole = ROLE_C, .pins = {8, 9, 10, 11}},
	{.base = APB_C_BASE + 0x0C00, .apb_index = 3, .gclk = 0x15, .irq = 10, .pingrp = PINGRP_A, .pinrole = ROLE_C, .pins = {16, 17, 18, 19}},
	{.base = APB_C_BASE + 0x1000, .apb_index = 4, .gclk = 0x16, .irq = 11, .pingrp = PINGRP_A, .pinrole = ROLE_C, .pins = {12, 13, 14, 15}},
	{.base = APB_C_BASE + 0x1400, .apb_index = 5, .gclk = 0x17, .irq = 12, .pingrp = PINGRP_A, .pinrole = ROLE_C, .pins = {22, 23, 24, 25}},
	{.base = APB_C_BASE + 0x1800, .apb_index = 6, .gclk = 0x18, .irq = 13, .pingrp = PINGRP_B, .pinrole = ROLE_C, .pins = {12, 13, 14, 15}},
	{.base = APB_C_BASE + 0x1C00, .apb_index = 7, .gclk = 0x19, .irq = 14, .pingrp = PINGRP_B, .pinrole = ROLE_D, .pins = {2, 3, 0, 1}},
};

/* address for calculating the PAC for a peripheral given its bus address */
ulong
pac_base(ulong busaddr)
{
	switch (busaddr >> 24) {
	case 0x40:
		return APB_A_BASE;
	case 0x41:
		return APB_B_BASE;
	case 0x42:
		return APB_C_BASE;
	default:
		return 0;
	}
}

