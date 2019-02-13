objects += $(boarddir)/board.o

ifeq ($(CONFIG_I2C), y)
	objects += $(driverdir)/samd/sercom.o $(driverdir)/samd/sercom-i2c-master.o
endif
