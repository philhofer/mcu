#ifndef __NORM_H_
#define __NORM_H_
#include <bits.h>

/* Some notes on the fixed-point math strategy:
 *
 * Sensor measurements will typically be
 * constrained such that the range falls
 * into one 'unit' (or [-32768,32767)), or
 * signed Q0.15.
 * Math performed on those measurements is
 * done with the unit values sign-extended
 * into a signed 32-bit integer, which is
 * effectively Q16.15 math. This allows us
 * to handle reasonably-sized multiples of the
 * unit values for intermediate calculations,
 * and then scale them back down to Q0.15 when
 * we're done. (One obvious advantage of doing
 * things this way is that it saves memory, but
 * another is that multiplication can typically
 * be performed as a regular 32x32->32, rather
 * than always doing 64-bit multiplicands.)
 */

/* NORM_ONE is the maximum positive Q0.15 value */
#define NORM_ONE       0x7fff
/* NORM_HALF is 1/2 in Q0.15 */
#define NORM_HALF      16384

/* 'unit' is a fixed-point (0.15) signed
 * number representing a value in the
 * range [-1, 1) */
typedef i16 unit;

/* the scalex() functions shift all of the
 * input values right such that the magnitude
 * of the largest input does not exceed
 * the range of 'unit' (or Q0.15) */
int scale3(i32 *a, i32 *b, i32 *c);
int scale4(i32 *a, i32 *b, i32 *c, i32 *d);

/* the normx() functions scale the magnitude
 * of the input vector so that it is equal to
 * the logical fixed-point '1'; it expects
 * all of its inputs to be in Q0.15 range */
void norm3(i32 *a, i32 *b, i32 *c);
void norm4(i32 *a, i32 *b, i32 *c, i32 *d);

i16 geomean2(i32 a, i32 b);

/* slow-path fallback for 32x32->64 multiplication */
i32 __unit_mul_slow(i32, i32);

/* unit_mul() performs a signed Qx.15 multiplication
 * with rounding towards zero */
static inline i32
unit_mul(i32 a, i32 b)
{
	i32 prod;
	if (__builtin_mul_overflow(a, b, &prod))
		return __unit_mul_slow(a, b);
	return (prod+NORM_HALF) >> 15;
}

#endif
