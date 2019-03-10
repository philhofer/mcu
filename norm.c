#include <bits.h>
#include <norm.h>

static u32
abs32(i32 v)
{
	if (v < 0)
		return -v;
	return v;
}

int
scale3(i32 *a, i32 *b, i32 *c)
{
	u32 w = 0;
	int shift;

	w |= abs32(*a);
	w |= abs32(*b);
	w |= abs32(*c);
	shift = 16 - __builtin_clz(w);
	if (shift <= 0)
		return 0;
	*a >>= shift;
	*b >>= shift;
	*c >>= shift;
	return shift;
}

int
scale4(i32 *a, i32 *b, i32 *c, i32 *d)
{
	u32 w = 0;
	int shift;

	w |= abs32(*a);
	w |= abs32(*b);
	w |= abs32(*c);
	w |= abs32(*d);
	shift = 16 - __builtin_clz(w);
	if (shift <= 0)
		return 0;
	*a >>= shift;
	*b >>= shift;
	*c >>= shift;
	*d >>= shift;
	return shift;
}

/* compute (num * scale) / 2^(exp/2)
 *       = (num * sclae) * 2^-(exp/2)
 *       = (num * scale) * 2^-(exp>>2 * 2) * 2^-((exp&1)/2)
 * handling the fractional exponent */
static i32
fp_scale(i32 num, i32 scale, unsigned exp)
{
	i32 rv, p;
	p = num * scale;
	rv = p >> (exp>>1);
	/* 23170 = sqrt(2) in Q.15 */
	if (exp&1)
		rv = (rv * 23170)>>15;
	return rv;
}

static u32 recipsqrt(u32, int *);

void
norm3(i32 *a, i32 *b, i32 *c)
{
	u32 mag;
	int shift;
	/* squaring and then summing 3 Q0.15 integers
	 * gives us a max value of '3' in Q2.30, which
	 * is perfect; it fits in a u32 */
	mag = (u32)(*a * *a) + (u32)(*b * *b) + (u32)(*c * *c);
	if (mag == 0) {
		/* eek... return an arbitrary unit vector */
		*a = NORM_ONE;
		*b = 0;
		*c = 0;
		return;
	}
	mag = recipsqrt(mag, &shift);
	*a = fp_scale(*a, mag, shift);
	*b = fp_scale(*b, mag, shift);
	*c = fp_scale(*c, mag, shift);
	return;
}

void
norm4(i32 *a, i32 *b, i32 *c, i32 *d)
{
	int shift;
	u32 mag;

	/* the one overflow condition here is that
	 * each and every value is the logical -1 (-32768),
	 * which means the sum of products will be zero.
	 * In that case, we already know the square root
	 * is sqrt(2^32) = 2^16 */
	mag = (u32)(*a * *a) + (u32)(*b * *b) + (u32)(*c * *c) + (u32)(*d * *d);
	if (mag == 0) {
		if (*a != -32768) {
			/* null vector ? */
			*a = NORM_ONE;
			*b = 0;
			*c = 0;
			*d = 0;
			return;
		}
		/* each value should simply be the logical 1/2 value */
		*a = *b = *c = *d = -16384;
		return;
	}
	mag = recipsqrt(mag, &shift);
	*a = fp_scale(*a, mag, shift);
	*b = fp_scale(*b, mag, shift);
	*c = fp_scale(*c, mag, shift);
	*d = fp_scale(*d, mag, shift);
}

#define Q30_ONE (1U << 30)
#define Q30_THREE (3U << 30)

/* recipsqrt() performs an integer
 * reciprocal square root with a floating
 * point in order to preserve precision
 *
 * it returns a number 'x' and point 'p'
 * such that (v>>shift)*((x*x)>>16) == Q30_ONE
 *
 * the 'point' value can be positive or negative;
 * negative values imply a left- instead of right-shift
 * of the input value
 */
static u32
recipsqrt(u32 v, int *point)
{
	/* find x such that v*x*x = Q30_ONE,
	 * keeping in mind that we're computing
	 * a Q0.16 number, so we'll need to shift
	 * by the right amount */
	unsigned lz;
	int shift;
	u32 x, nx, p;

	/* normalize the input to 16 bits of precision
	 * by selecting a shift that is a multiple of two
	 * (since we will need to divide it by two to get
	 * the appropriate shift for Q.15 */
	lz = __builtin_clz(v);
	shift = 16 - (int)lz;
	if (shift < 0)
		v <<= -shift;
	else
		v >>= shift;
	*point = 16+shift;

	/* now find a value such that
	 *   (x * x)>>16 * v == Q30_ONE
	 * given that we've constrained 'v'
	 * such that it has 16 significant bits
	 *
	 * our initial guess of 1<<15 constrains
	 * the initial output range to
	 *    0 < p < 2
	 *  which is narrower than the required
	 *  range of
	 *    0 < p < 3
	 *  for Newton-Raphson to converge
	 */

	x = 1U<<15;
	unsigned count = 4;
	while ((p = ((x*x)>>16)*v) != Q30_ONE && count) {
		nx = (x * (((Q30_THREE - p)>>1)>>14))>>16;
		if (nx == x) {
			x = nx;
			break;
		}
		x = nx;
		count--;
	}
	return x;
}

/* slow-path (assuming 32-bit hardware) */
i32
__unit_mul_slow(i32 a, i32 b)
{
	i64 rv;

	rv = (i64)a * (i64)b;
	rv >>= 15;
	return rv;
}
