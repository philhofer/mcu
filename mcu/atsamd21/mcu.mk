objects += board.o nvmctrl.o sysctrl.o samd21.o

# these drivers are mandatory
VPATH += $(rootdir)/drivers/sam/
objects += gclk.o sercom.o port.o

ifeq ($(CONFIG_I2C), y)
	objects += sercom-i2c-master.o
endif

ifeq ($(CONFIG_SPI), y)
	objects += sercom-spi.o
endif

ifeq ($(CONFIG_USB), y)
	objects += sam-usb.o
endif

ifeq ($(CONFIG_EXTIRQ), y)
	objects += eic.o
endif

userrow.elf: userrow.o | userrow.ld
	$(LD) -T $| -o $@ $^

userrow.mot: userrow.elf
	$(OBJCOPY) -Osrec $^ $@
