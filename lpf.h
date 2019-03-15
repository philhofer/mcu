#ifndef __LPF_H_
#define __LPF_H_

/* lpf_cycle() performs one cycle of low-pass
 * filtering on the given register and input
 * with the parameter 'k' and returns the new
 * filter value
 *
 * see:
 * https://www.edn.com/design/systems-design/4320010/A-simple-software-lowpass-filter-suits-embedded-system-applications
 *
 * Bandwidths and rise times for
 * values of 'k' are as follows:
 *
 * k   Bandwidth  Rise Time
 * 1    0.1197       3
 * 2    0.0466       8
 * 3    0.0217       16
 * 4    0.0104       34
 * 5    0.0051       69
 * 6    0.0026       140
 * 7    0.0012       280
 * 8    0.0007       561
 */
static inline i32
lpf_cycle(i32 *reg, i32 input, unsigned k)
{
	*reg -= (*reg >> k);
	*reg += input;
	return *reg >> k;
}

#endif
