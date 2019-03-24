#include <bits.h>
#include <norm.h>
#include <gyro.h>
#include <accel.h>
#include <mag.h>
#include <lpf.h>
#include <madgwick.h>

/* algorithm from:
 *
 * Sebastian O.H. Madgwick,
 * "An efficient orientation filter for inertial
 * and inertial/magnetic sensor arrays"
 *
 * converted to signed Q16.15 fixed-point math
 */

void
madgwick(struct madgwick_state *state, const struct gyro_state *gyro, const struct accel_state *accel, const struct mag_state *mag, unit tstep, bool debias)
{
	i32 qdot_omega0, qdot_omega1, qdot_omega2, qdot_omega3;
	i32 f0, f1, f2, f3, f4, f5;
	i32 J11or24, J12or23, J13or22, J14or21, J32, J33,
	J41, J42, J43, J44, J51, J52, J53, J54, J61, J62, J63, J64;
	i32 qhatdot0, qhatdot1, qhatdot2, qhatdot3;
	i32 w_err_x, w_err_y, w_err_z;
	i32 h_x, h_y, h_z;
	i32 w_x, w_y, w_z;
	i32 quat0 = (i32)state->quat0;
	i32 quat1 = (i32)state->quat1;
	i32 quat2 = (i32)state->quat2;
	i32 quat3 = (i32)state->quat3;
	i32 halfquat0 = quat0 / 2;
	i32 halfquat1 = quat1 / 2;
	i32 halfquat2 = quat2 / 2;
	i32 halfquat3 = quat3 / 2;
	i32 twoquat0 = 2 * quat0;
	i32 twoquat1 = 2 * quat1;
	i32 twoquat2 = 2 * quat2;
	i32 twoquat3 = 2 * quat3;
	i32 twob_x = 2 * (i32)state->flux_x;
	i32 twob_z = 2 * (i32)state->flux_z;
	i32 twob_xquat0 = 2 * unit_mul(state->flux_x, quat0);
	i32 twob_xquat1 = 2 * unit_mul(state->flux_x, quat1);
	i32 twob_xquat2 = 2 * unit_mul(state->flux_x, quat2);
	i32 twob_xquat3 = 2 * unit_mul(state->flux_x, quat3);
	i32 twob_zquat0 = 2 * unit_mul(state->flux_z, quat0);
	i32 twob_zquat1 = 2 * unit_mul(state->flux_z, quat1);
	i32 twob_zquat2 = 2 * unit_mul(state->flux_z, quat2);
	i32 twob_zquat3 = 2 * unit_mul(state->flux_z, quat3);
	i32 quat0quat1;
	i32 quat0quat2 = unit_mul_short(quat0, quat2);
	i32 quat0quat3;
	i32 quat1quat2;
	i32 quat1quat3 = unit_mul_short(quat1, quat3);
	i32 quat2quat3;
	i32 twom_x = 2 * (i32)mag->x;
	i32 twom_y = 2 * (i32)mag->y;
	i32 twom_z = 2 * (i32)mag->z;

	i32 a_x, a_y, a_z;
	a_x = (i32)accel->x;
	a_y = (i32)accel->y;
	a_z = (i32)accel->z;
	norm3(&a_x, &a_y, &a_z);

	i32 m_x, m_y, m_z;
	m_x = (i32)mag->x;
	m_y = (i32)mag->y;
	m_z = (i32)mag->z;
	norm3(&m_x, &m_y, &m_z);

	/* compute the objective function and Jacobian;
	 *
	 * use short multiplies when multiplying unscaled versor
	 * components, as we know those have been scaled to Q0.15 */
	f0 = unit_mul(twoquat1, quat3) - unit_mul(twoquat0, quat2) - a_x;
	f1 = unit_mul(twoquat0, quat1) + unit_mul(twoquat2, quat3) - a_y;
	f2 = (1U<<15) - unit_mul(twoquat1, quat1) - unit_mul(twoquat2, quat2) - a_z;
	f3 = unit_mul(twob_x, (0x4000 - unit_mul_short(quat2, quat2)) - unit_mul_short(quat3, quat3)) + unit_mul(twob_z, (quat1quat3 - quat0quat2)) - m_x;
	f4 = unit_mul(twob_x, (unit_mul_short(quat1, quat2) - unit_mul_short(quat0, quat3))) + unit_mul(twob_z, (unit_mul_short(quat0, quat1) + unit_mul_short(quat2, quat3))) - m_y;
	f5 = unit_mul(twob_x, (quat0quat2 + quat1quat3)) + unit_mul(twob_z, (0x4000 - unit_mul(quat1, quat1) - unit_mul(quat2, quat2))) - m_z;
	J11or24 = twoquat2;
	J12or23 = 2 * quat3;
	J13or22 = twoquat0;
	J14or21 = twoquat1;
	J32 = 2 * J14or21;
	J33 = 2 * J11or24;
	J41 = twob_zquat2;
	J42 = twob_zquat3;
	J43 = 2 * twob_xquat2 + twob_zquat0;
	J44 = 2 * twob_xquat3 - twob_zquat1;
	J51 = twob_xquat3 - twob_zquat1;
	J52 = twob_xquat2 + twob_zquat0;
	J53 = twob_xquat1 + twob_zquat3;
	J54 = twob_xquat0 - twob_zquat2;
	J61 = twob_xquat2;
	J62 = twob_xquat3 - 2 * twob_zquat1;
	J63 = twob_xquat0 - 2 * twob_zquat2;
	J64 = twob_xquat1;
	/* compute the gradient (matrix multiplication) */
	qhatdot0 = unit_mul(J14or21, f1) - unit_mul(J11or24, f0) - unit_mul(J41, f3) - unit_mul(J51, f4) + unit_mul(J61, f5);
	qhatdot1 = unit_mul(J12or23, f0) + unit_mul(J13or22, f1) - unit_mul(J32, f2) + unit_mul(J42, f3) + unit_mul(J52, f4) + unit_mul(J62, f5);
	qhatdot2 = unit_mul(J12or23, f1) - unit_mul(J33, f2) - unit_mul(J13or22, f0) - unit_mul(J43, f3) + unit_mul(J53, f4) + unit_mul(J63, f5);
	qhatdot3 = unit_mul(J14or21, f0) + unit_mul(J11or24, f1) - unit_mul(J44, f3) - unit_mul(J54, f4) + unit_mul(J64, f5);
	scale4(&qhatdot0, &qhatdot1, &qhatdot2, &qhatdot3);
	norm4(&qhatdot0, &qhatdot1, &qhatdot2, &qhatdot3);

	/* compute angular estimate of gyro error */
	w_err_x = unit_mul(twoquat0, qhatdot1) - unit_mul(twoquat1, qhatdot0) - unit_mul(twoquat2, qhatdot3) + unit_mul(twoquat3, qhatdot2);
	w_err_y = unit_mul(twoquat0, qhatdot2) + unit_mul(twoquat1, qhatdot3) - unit_mul(twoquat2, qhatdot0) - unit_mul(twoquat3, qhatdot1);
	w_err_z = unit_mul(twoquat0, qhatdot3) - unit_mul(twoquat1, qhatdot2) + unit_mul(twoquat2, qhatdot1) - unit_mul(twoquat3, qhatdot0);

	/* remove gyro bias:
	 * unlike the Madgwick paper, we use a low-pass filter here,
	 * since the (error*zeta*deltat) figure is typically below
	 * 1/32768, and so the integrator never actually gets going
	 *
	 * Using K=8 here means we estimate the average error
	 * over ~561 samples, which is nearly 3 seconds for a 200Hz
	 * sample rate.
	 */
	w_x = (i32)gyro->x << 4;
	w_y = (i32)gyro->y << 4;
	w_z = (i32)gyro->z << 4;
	if (debias) {
		w_x -= lpf_cycle(&state->gerrx, w_err_x, 8);
		w_y -= lpf_cycle(&state->gerry, w_err_y, 8);
		w_z -= lpf_cycle(&state->gerrz, w_err_z, 8);
	}

	/* compute the quaternion rate measured by gyroscopes;
	 * since the gyroscopes measure [-16,16) rads, we need to scale
	 * up the input
	 *
	 * TODO: there is an opportunity here to CAREFULLY use
	 * short multiplies, since we know that w_[xyz] and halfquat[0123]
	 * are both scaled to Q0.15 at this point */
	qdot_omega0 = unit_mul(-halfquat1, w_x) - unit_mul(halfquat2, w_y) - unit_mul(halfquat3, w_z);
	qdot_omega1 = unit_mul(halfquat0, w_x) + unit_mul(halfquat2, w_z) - unit_mul(halfquat3, w_y);
	qdot_omega2 = unit_mul(halfquat0, w_y) - unit_mul(halfquat1, w_z) + unit_mul(halfquat3, w_x);
	qdot_omega3 = unit_mul(halfquat0, w_z) + unit_mul(halfquat1, w_y) - unit_mul(halfquat2, w_x);
	/* compute then integrate the estimated quaternion rate
	 *
	 * use short multiplication when multiplying qhatdot by beta;
	 * qhatdot has been normalized to Q0.15 */
	quat0 += unit_mul(qdot_omega0 - unit_mul_short(state->beta, qhatdot0), tstep);
	quat1 += unit_mul(qdot_omega1 - unit_mul_short(state->beta, qhatdot1), tstep);
	quat2 += unit_mul(qdot_omega2 - unit_mul_short(state->beta, qhatdot2), tstep);
	quat3 += unit_mul(qdot_omega3 - unit_mul_short(state->beta, qhatdot3), tstep);
	scale4(&quat0, &quat1, &quat2, &quat3);
	norm4(&quat0, &quat1, &quat2, &quat3);
	state->quat0 = quat0;
	state->quat1 = quat1;
	state->quat2 = quat2;
	state->quat3 = quat3;

	/* compute flux in the earth frame
	 * and normalize the earth flux so that it
	 * only has x and z components (in other words,
	 * it has the same inclination as the measured flux)
	 *
	 * can cheat a bit and use short multiplication here
	 * because the quaternion has just been normalized, so
	 * everything is in Q0.15 by definition */
	quat0quat1 = unit_mul_short(quat0, quat1);
	quat0quat2 = unit_mul_short(quat0, quat2);
	quat0quat3 = unit_mul_short(quat0, quat3);
	quat2quat3 = unit_mul_short(quat2, quat3);
	quat1quat2 = unit_mul_short(quat1, quat2);
	quat1quat3 = unit_mul_short(quat1, quat3);
	h_x = unit_mul(twom_x, (0x4000 - unit_mul_short(quat2, quat2) - unit_mul_short(quat3, quat3))) + unit_mul(twom_y, (quat1quat2 - quat0quat3)) + unit_mul(twom_z, (quat1quat3 + quat0quat2));
	h_y = unit_mul(twom_x, (quat1quat2 + quat0quat3)) + unit_mul(twom_y, (0x4000 - unit_mul_short(quat1, quat1) - unit_mul_short(quat3, quat3))) + unit_mul(twom_z, (quat2quat3 - quat0quat1));
	h_z = unit_mul(twom_x, (quat1quat3 - quat0quat2)) + unit_mul(twom_y, (quat2quat3 + quat0quat1)) + unit_mul(twom_z, (0x4000 - unit_mul_short(quat1, quat1) - unit_mul_short(quat2, quat2)));
	state->flux_x = geomean2(h_x, h_y);
	state->flux_z = h_z;
}
