#ifndef __NORM_H_
#define __NORM_H_
#include <bits.h>

#define NORM_ONE       0x7fff

/* 'unit' is a fixed-point (0.15) signed
 * number representing a value in the
 * range [-1, 1) */
typedef i16 unit;

/* norm3 normalizes a 3-unit vector
 * so that its magnitude is NORM_ONE
 *
 * the input values should be sign-extended
 * unit values (in the range -32768,32767) */
void norm3(i32 *a, i32 *b, i32 *c);

/* norm4 normalizes a 4-unit vector
 * so that its magnitude is NORM_ONE
 *
 * the input values should be sign-extended
 * unit values (in the range -32768,32767) */
void norm4(i32 *a, i32 *b, i32 *c, i32 *d);

static inline i32
unit_mul(i32 a, i32 b)
{
	i32 prod;
	if (__builtin_mul_overflow(a, b, &prod))
		return NORM_ONE;
	prod >>= 15;
	return prod;
}

#endif
