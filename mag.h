#ifndef __MAG_H_
#define __MAG_H_
#include <bits.h>

/* mag_state holds magnetometer data
 * in three axes in units of 0.1uT per lsb
 *
 * (the Earth's magnetic field is 25 to 65 uT,
 * so we'd expect the magnitude of the vector
 * to be 250 to 650) */
struct mag_state {
	i16 x, y, z;
};

#endif
