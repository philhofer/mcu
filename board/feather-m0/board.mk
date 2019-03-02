ifeq ($(CONFIG_USB),y)
	objects += feather-m0-usb.o
endif
ifeq ($(CONFIG_GPIO),y)
	objects += red-led.o
endif
