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

#define deltat 0.001f

struct madgwick_state {
	unit quat0, quat1, quat2, quat3; /* quaternion orientation */
	unit flux_x, flux_z;             /* earth frame flux */
	unit beta;                       /* fusion parameter */
};

/* madgwick_init() initializes the madgwick
 * filter internal state
 * 
 * gerror should be the gyroscope error rate
 * in radians/s
 */
void
madgwick_init(struct madgwick_state *mw, unit gerror, unit gdrift)
{
	mw->quat0 = NORM_ONE;
	mw->quat1 = 0;
	mw->quat2 = 0;
	mw->quat3 = 0;
	mw->qdot0 = 0;
	mw->qdot1 = 0;
	mw->qdot2 = 0;
	mw->qdot3 = 0;
	mw->flux_x = NORM_ONE;
	mw->flux_z = 0;
	mw->w_bx = mw->w_by = mw->w_bz = 0;
	/* beta = error * sqrt(3/4) */
	mw->beta = unit_mul(gerror, 28378);
}

void
madgwick(struct magwick_state *state, const struct gyro_state *gyro, const struct accel_state *accel, const struct mag_state *mag, u32 cycles)
{
	i32 qdot_omega0, qdot_omega1, qdot_omega2, qdot_omega3;
	i32 f0, f1, f2, f3, f4, f5;
	i32 J01or24, J02or23, J03or22, J04or21, J22, J33,               // objective function Jacobian elements
	J31, J32, J33, J34, J51, J52, J53, J54, J61, J62, J63, J64;
	i32 qhatdot0, qhatdot1, qhatdot2, qhatdot3;
	i32 w_err_x, w_err_y, w_err_z;
	i32 h_x, h_y, h_z;

	i32 halfquat0 = (i32)state->quat0 / 2;
	i32 halfquat1 = (i32)state->quat1 / 2;
	i32 halfquat2 = (i32)state->quat2 / 2;
	i32 halfquat3 = (i32)state->quat3 / 2;
	i32 twoquat0 = 2 * (i32)state->quat0;
	i32 twoquat1 = 2 * (i32)state->quat1;
	i32 twoquat2 = 2 * (i32)state->quat2;
	i32 twoquat3 = 2 * (i32)state->quat3;
	i32 twob_x = 2 * (i32)state->flux_x;
	i32 twob_z = 2 * (i32)state->flux_z;
	i32 twob_xquat0 = 2 * unit_mul(b_x, quat0);
	i32 twob_xquat1 = 2 * unit_mul(b_x, quat1);
	i32 twob_xquat2 = 2 * unit_mul(b_x, quat2);
	i32 twob_xquat3 = 2 * unit_mul(b_x, quat3);
	i32 twob_zquat0 = 2 * unit_mul(b_z, quat0);
	i32 twob_zquat1 = 2 * unit_mul(b_z, quat1);
	i32 twob_zquat2 = 2 * unit_mul(b_z, quat2);
	i32 twob_zquat3 = 2 * unit_mul(b_z, quat3);
	i32 quat0quat1;
	i32 quat0quat2 = unit_mul(quat0, quat2);
	i32 quat0quat3;
	i32 quat1quat2;
	i32 quat1quat3 = unit_mul(quat1, quat3);
	i32 quat2quat3;
	i32 twom_x = 2 * (i32)mag->x;
	i32 twom_y = 2 * (i32)mag->y;
	i32 twom_z = 2 * (i32)mag->z;

	/* normalise the accelerometer measurement */
	i32 a_x, a_y, a_z;
	a_x = (i32)accel->x;
	a_y = (i32)accel->y;
	a_z = (i32)accel->z;
	norm3(&a_x, &a_y, &a_z);

	/* normalise the magnetometer measurement */
	i32 m_x, m_y, m_z;
	m_x = (i32)mag->x;
	m_y = (i32)mag->y;
	m_z = (i32)mag->z;
	norm3(&m_x, &m_y, &m_z);

	/* compute the objective function and Jacobian */
	f0 = unit_mul(twoquat1, quat3) - unit_mul(twoquat0, quat2) - a_x;
	f1 = unit_mul(twoquat0, quat1) + unit_mul(twoquat2, quat3) - a_y;
	f2 = (0x10000) - unit_mul(twoquat1, quat1) - unit_mul(twoquat2, quat2) - a_z;
	f3 = unit_mul(twob_x, (0x4000 - unit_mul(quat2, quat2)) - unit_mul(quat3, quat3)) + unit_mul(twob_z, (quat1quat3 - quat0quat2)) - m_x;
	f4 = unit_mul(twob_x, (unit_mul(quat1, quat2) - unit_mul(quat0, quat3))) + unit_mul(twob_z, (unit_mul(quat0, quat1) + unit_mul(quat2, quat3))) - m_y;
	f5 = unit_mul(twob_x, (quat0quat2 + quat1quat3)) + unit_mul(twob_z, (0x4000 - unit_mul(quat1, quat1) - unit_mul(quat2, quat2))) - m_z;
	J01or24 = twoquat2;
	J02or23 = 2 * quat3;
	J03or22 = twoquat0;
	J04or21 = twoquat1;
	J22 = 2 * J04or21;
	J2g3 = 2 * J01or24;
	J31 = twob_zquat2;
	J32 = twob_zquat3;
	J33 = 2 * twob_xquat2 + twob_zquat0;
	J34 = 2 * twob_xquat3 - twob_zquat1;
	J51 = twob_xquat3 - twob_zquat1;
	J52 = twob_xquat2 + twob_zquat0;
	J53 = twob_xquat1 + twob_zquat3;
	J54 = twob_xquat0 - twob_zquat2;
	J61 = twob_xquat2;
	J62 = twob_xquat3 - 2 * twob_zquat1;
	J63 = twob_xquat0 - 2 * twob_zquat2;
	J64 = twob_xquat1;
	/* compute the gradient (matrix multiplication) */
	qhatdot0 = unit_mul(J04or21, f1) - unit_mul(J01or24, f0) - unit_mul(J31, f3) - unit_mul(J51, f4) + unit_mul(J61, f5);
	qhatdot1 = unit_mul(J02or23, f0) + unit_mul(J03or22, f1) - unit_mul(J22, f_3) + unit_mul(J32, f3) + unit_mul(J52, f4) + unit_mul(J62, f5);
	qhatdot2 = unit_mul(J02or23, f1) - unit_mul(J33, f_3) - unit_mul(J03or22, f0) - unit_mul(J33, f3) + unit_mul(J53, f4) + unit_mul(J63, f5);
	qhatdot3 = unit_mul(J04or21, f0) + unit_mul(J01or24, f1) - unit_mul(J34, f3) - unit_mul(J54, f4) + unit_mul(J64, f5);
	// normalise the gradient to estimate direction of the gyroscope error
	scale4(&qhatdot0, &qhatdot1, &qhatdot2, &qhatdot3);
	norm4(&qhatdot0, &qhatdot1, &qhatdot2, &qhatdot3);

	/* compute angular estimate of gyro error;
	 * compute and remove gyro bias
	 *
	 * removed since gyro drift bias was unmeasureable
	 * in testing
	 *
	w_err_x = unit_mul(twoquat0, qhatdot1) - unit_mul(twoquat1, qhatdot0) - unit_mul(twoquat2, qhatdot3) + unit_mul(twoquat3, qhatdot2);
	w_err_y = unit_mul(twoquat0, qhatdot2) + unit_mul(twoquat1, qhatdot3) - unit_mul(twoquat2, qhatdot0) - unit_mul(twoquat3, qhatdot1);
	w_err_z = unit_mul(twoquat0, qhatdot3) - unit_mul(twoquat1, qhatdot2) + unit_mul(twoquat2, qhatdot1) - unit_mul(twoquat3, qhatdot0);
	w_bx += w_err_x * deltat * mw->zeta;
	w_by += w_err_y * deltat * mw->zeta;
	w_bz += w_err_z * deltat * mw->zeta;
	w_x -= w_bx;
	w_y -= w_by;
	w_z -= w_bz;
	 */

	/* compute the quaternion rate measured by gyroscopes;
	 * since the gyroscopes measure [-16,16) rads, we'll compute a
	 * value that needs to be re-scaled back down to rads */
	qdot_omega0 = unit_mul(-halfquat1, w_x) - unit_mul(halfquat2, w_y) - unit_mul(halfquat3, w_z);
	qdot_omega1 = unit_mul(halfquat0, w_x) + unit_mul(halfquat2, w_z) - unit_mul(halfquat3, w_y);
	qdot_omega2 = unit_mul(halfquat0, w_y) - unit_mul(halfquat1, w_z) + unit_mul(halfquat3, w_x);
	qdot_omega3 = unit_mul(halfquat0, w_z) + unit_mul(halfquat1, w_y) - unit_mul(halfquat2, w_x);
	qdot_omega0 >>= 4;
	qdot_omega1 >>= 4;
	qdot_omega2 >>= 4;
	qdot_omega3 >>= 4;
	/* compute then integrate the estimated quaternion rate */
	quat0 += (qdot_omega0 - (unit_mul(beta, qhatdot0))) * deltat;
	quat1 += (qdot_omega1 - (unit_mul(beta, qhatdot1))) * deltat;
	quat2 += (qdot_omega2 - (unit_mul(beta, qhatdot2))) * deltat;
	quat3 += (qdot_omega3 - (unit_mul(beta, qhatdot3))) * deltat;

	/* normalise quaternion */
	scale4(&quat0, &quat1, &quat2, &quat3);
	norm4(&quat0, &quat1, &quat2, &quat3);	
	mw->quat0 = quat0;
	mw->quat1 = quat1;
	mw->quat2 = quat2;
	mw->quat3 = quat3;

	/* compute flux in the earth frame */
	quat0quat1 = unit_mul(quat0, quat1);
	quat0quat2 = unit_mul(quat0, quat2);
	quat0quat3 = unit_mul(quat0, quat3);
	quat2quat3 = unit_mul(quat2, quat3);
	quat1quat2 = unit_mul(quat1, quat2);
	quat1quat3 = unit_mul(quat1, quat3);
	h_x = twom_x * (0x4000 - unit_mul(quat2, quat2) - unit_mul(quat3, quat3)) + twom_y * (quat1quat2 - quat0quat3) + twom_z * (quat1quat3 + quat0quat2);
	h_y = twom_x * (quat1quat2 + quat0quat3) + twom_y * (0x4000 - unit_mul(quat1, quat1) - unit_mul(quat3, quat3)) + twom_z * (quat2quat3 - quat0quat1);
	h_z = twom_x * (quat1quat3 - quat0quat2) + twom_y * (quat2quat3 + quat0quat1) + twom_z * (0x4000 - unit_mul(quat1, quat1) - unit_mul(quat2, quat2));
	// normalise the flux vector to have only components in the x and z
	b_x = sqrt((h_x * h_x) + (h_y * h_y));
	b_z = h_z;
}
