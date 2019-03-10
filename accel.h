#ifndef __ACCEL_H_
#define __ACCEL_H_

/* mgs hold a fixed-point accelerometer
 * reading between [-8,8) Gs, so the
 * resting reading would be
 *   x = 0, y = 0, z = 0x1ff */
typedef i16 mgs;

struct accel_state {
	mgs x, y, z;
};

#endif
