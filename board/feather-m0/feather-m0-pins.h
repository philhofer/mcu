#ifndef __FEATHER_M0_PINS_H_
#define __FEATHER_M0_PINS_H_
#include <board.h>
#include <drivers/sam/port.h>

/* Feather-M0 pin mapping
 *
 * Pins are named based on the
 * labels on the board. See
 * https://learn.adafruit.com/assets/46244
 * for reference.
 */

#define port(g, n) PORT_PINNUM(PINGRP_##g, n)

/* "analog" pins on the lhs of the board */
#define PIN_AREF port(A, 3)
#define PIN_A0 port(A, 2)
#define PIN_A1 port(B, 8)
#define PIN_A2 port(B, 9)
#define PIN_A3 port(A, 4)
#define PIN_A4 port(A, 5)
#define PIN_A5 port(B, 2)

/* "digital" pins on the rhs of the board */
#define PIN_D5  port(A, 15)
#define PIN_D6  port(A, 20)
#define PIN_D9  port(A, 7)  /* NOTE: attached to lipoly voltage divider! */
#define PIN_D10 port(A, 18)
#define PIN_D11 port(A, 16)
#define PIN_D12 port(A, 19)
#define PIN_D13 port(A, 17) /* NOTE: attached to red LED */

#endif
