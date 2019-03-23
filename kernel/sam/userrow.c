#include <bits.h>

/* this file is used to generate
 * an SREC file to program the NVM User Row */

/* TODO: add some define-s for the values
 * in the user row to make this a little
 * less opaque */

#define USER_ROW_DEFAULT 0xFFFFFC5DD8E0C7FF

__attribute__((section(".userrow")))
u64 userrow = USER_ROW_DEFAULT;

