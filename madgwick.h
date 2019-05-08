#ifndef __MADGWICK_H_
#define __MADGWICK_H_
#include <bits.h>
#include <norm.h>
#include <gyro.h>
#include <accel.h>
#include <mag.h>

/* algorithm from:
 *
 * Sebastian O.H. Madgwick,
 * "An efficient orientation filter for inertial
 * and inertial/magnetic sensor arrays"
 *
 * converted to signed Q16.15 fixed-point math
 */

struct madgwick_state {
	unit quat0, quat1, quat2, quat3; /* quaternion orientation */
	i32  gerrx, gerry, gerrz;        /* gyro bias filters */
	unit flux_x, flux_z;             /* earth frame flux */
	unit beta;                       /* fusion parameter */
};

/* madgwick_init() initializes the madgwick
 * filter internal state
 * 
 * gerror should be the gyroscope error rate
 * in radians/s
 */
static inline void
madgwick_init(struct madgwick_state *mw, unit gerror)
{
	mw->quat0 = NORM_ONE;
	mw->quat1 = mw->quat2 = mw->quat3 = 0;
	mw->gerrx = mw->gerry = mw->gerrz = 0;
	mw->flux_x = NORM_ONE;
	mw->flux_z = 0;
	/* beta = error * sqrt(3/4) */
	mw->beta = unit_mul(gerror, 28378);
}

/* madgwick() updates the internal state of the Madgwick orientation filter.
 * The 'tstep' value should be normalized to fractions-of-a-second in Q0.15 format.
 *
 * If 'calibrate' is true, the angular rate around all axes should be zero. (This
 * allows us to produce an accurate initial estimate of the gyro zero-rate error.) */
void madgwick(struct madgwick_state *mw, const struct gyro_state *g, const struct accel_state *a, const struct mag_state *m, unit tstep, bool calibrate);

#endif
