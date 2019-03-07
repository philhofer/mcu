#ifndef __GYRO_H_
#define __GYRO_H_
#include <bits.h>

/* Our arbitrary fixed-point rotational units
 * are scaled so that the range of one mrad
 * is [-16 rad/s, 16 rad/s), so each bit is
 * one 2048th of a rad/s (or one degree/s
 * is about 5-and-a-half bits ) */
typedef i16 mrads;

struct gyro_state {
	mrads x, y, z;
};

#endif
