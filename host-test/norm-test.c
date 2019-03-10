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

	/* test multiply slow path */
	half *= 4;
	assert(unit_mul(half, half) == (half << 1));

	i32 a, b, c, d;

	/* for every vector where a=b=c, the normalized
	 * vector should be a=b=c=+/-sqrt(3) */
	for (i16 i = -32768; i<32767; i++) {
		if (i == 0)
			continue;
		a = b = c = i;

		assert(scale3(&a, &b, &c) == 0);
		assert(a == i && b == i && c == i);
		norm3(&a, &b, &c);

		/* this is sqrt(3) in Q0.15 */
		i16 want = 18918;
		if (i < 0)
			want = -want;
		if (a != b || b != c || !unit_approx(c, want)) {
			printf("vector %d: want %d; got (%d, %d, %d)\n", i, want, a, b ,c);
			return 1;
		}

		/* for 4-vectors, the unit vector should be
		 *   a = b = c = d = +/- 1/2 */
		a = b = c = d = i;
		assert(scale4(&a, &b, &c, &d) == 0);
		assert(a == i && b == i && c == i && d == i);
		norm4(&a, &b, &c, &d);
		want = 0x4000;
		if (i < 0)
			want = -want;
		if (a != b || b != c || c != d || !unit_approx(a, want)) {
			printf("vector %d: want %d, got (%d, %d, %d, %d)\n", i, want, a, b, c, d);
			return 1;
		}

		/* if we up-scale one of the vector components,
		 * then everything else should scale down accordingly
		 *
		 * here |[2, 1, 1, 1]| -> [2/sqrt(7), 1/sqrt(7), 1/sqrt(7), 1/sqrt(7)] */
		a *= 2;
		scale4(&a, &b, &c, &d);
		norm4(&a, &b, &c, &d);
		want = 12385;
		if (a < 0)
			want = -want;
		if (!unit_approx(a, 2*want)) {
			printf("first component should be %d; got %d\n", 2*want, a);
			return 1;
		}
		if (b != c || c != d || !unit_approx(b, want)) {
			printf("got unexpected vector (%d, %d, %d, %d)\n", a, b, c, d);
			return 1;
		}

	}

	return 0;
}
