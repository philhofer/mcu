#include <i2c.h>
#include <drivers/sam/sercom-i2c-master.h>

SERCOM_I2C_MASTER(default_i2c, 3);

void sercom3_irq_entry(void) {
	sercom_i2c_master_irq(&default_i2c);
}
