objects += $(boarddir)/board.o $(boarddir)/nvmctrl.o $(boarddir)/sysctrl.o $(boarddir)/samd21.o

# these drivers are mandatory
objects += $(driverdir)/sam/gclk.o $(driverdir)/sam/sercom.o $(driverdir)/sam/port.o

ifeq ($(CONFIG_I2C), y)
	objects += $(driverdir)/sam/sercom-i2c-master.o
endif

ifeq ($(CONFIG_SPI), y)
	objects += $(driverdir)/sam/sercom-spi.o
endif

ifeq ($(CONFIG_USB), y)
	objects += $(driverdir)/sam/usb.o
endif

$(boarddir)/userrow.elf: $(boarddir)/userrow.o | $(boarddir)/userrow.ld
	$(LD) -T $| -o $@ $^

$(boarddir)/userrow.mot: $(boarddir)/userrow.elf
	$(OBJCOPY) -Osrec $^ $@
