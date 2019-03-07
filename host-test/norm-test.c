#include <bits.h>
#include <norm.h>
#include <assert.h>

static bool
is_unit(i32 a)
{
	return ((i16)a) == a;
}

static bool
unit_approx(i32 a, i32 b)
{
	assert(is_unit(a));
	assert(is_unit(b));
	return abs(a-b) < 3;
}

int main(void) {
	/* 1*1 == 1 */
	assert(unit_approx(unit_mul(NORM_ONE, NORM_ONE), NORM_ONE));

	/* half * half = quarter */
	i32 half = 0x4000;
	assert(unit_approx(unit_mul(half, half), half>>1));

	i32 a, b, c, d;

	/* for every vector where a=b=c, the normalized
	 * vector should be a=b=c=+/-sqrt(3) */
	for (i16 i = -32768; i<32767; i++) {
		if (i == 0)
			continue;
		a = b = c = i;
		norm3(&a, &b, &c);

		/* this is sqrt(3) in Q0.15 */
		i16 want = 18918;
		if (i < 0)
			want = -want;
		if (!unit_approx(a, want) || !unit_approx(b, want) || !unit_approx(c, want)) {
			printf("vector %d: want %d; got (%d, %d, %d)\n", i, want, a, b ,c);
			return 1;
		}

		/* for 4-vectors, the unit vector should be
		 *   a = b = c = d = +/- 1/2 */
		a = b = c = d = i;
		norm4(&a, &b, &c, &d);
		want = 0x4000;
		if (i < 0)
			want = -want;
		if (!unit_approx(a, want) || !unit_approx(b, want) ||
			!unit_approx(c, want) || !unit_approx(d, want)) {
			printf("vector %d: want %d, got (%d, %d, %d, %d)\n", i, want, a, b, c, d);
			return 1;
		}

	}

	return 0;
}
