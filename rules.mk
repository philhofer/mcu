include config

# default Makefile rules for all targets

# targets will be in board/<boardname>/<targetname>

rootdir=../../../
boarddir=../
archdir=$(rootdir)/arch/$(CONFIG_ARCH)
mcudir=$(rootdir)/mcu/$(CONFIG_MCU)

# for essentially all targets, all we're
# interested in building is the ELF objects
# (but for some targets with weird flash programmers
# we may want raw binary, .srec, etc.)
.DEFAULT_GOAL ?= kernel.elf

VPATH += $(rootdir) $(archdir) $(mcudir) $(boarddir) .
CFLAGS += -I. -I$(rootdir) -I$(archdir) -I$(boarddir) -I$(mcudir)
CFLAGS += -Wall -ggdb

CONFSUBST = $(rootdir)/confsubst

include $(archdir)/arch.mk
include $(mcudir)/mcu.mk
include $(boarddir)/board.mk

objects += idle.o libc.o

ifeq ($(CONFIG_I2C), y)
	objects += i2c.o
endif

ifeq ($(CONFIG_GPIO), y)
	objects += gpio.o
endif

ifeq ($(CONFIG_USB), y)
	objects += usb.o
endif

ifeq ($(CONFIG_USBSERIAL), y)
	objects += usb-cdc-acm.o
endif

headers += config.h

%.o: %.c
	@echo "CC $@"
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	@echo "AS $@"
	@$(AS) $(ASFLAGS) $^ -o $@

%.bin: %.elf
	@echo "OBJCOPY $@"
	@$(OBJCOPY) -Obinary $^ $@
	@chmod -x $@

%.elf: $(objects) | mcu.ld
	@echo "LD $@"
	@$(LD) $(LDFLAGS) -T $| -o $@ $^ -lgcc
	@chmod -x $@

%.h: %.h.inc | config
	@echo "CONF $@"
	@$(CONFSUBST) $| $^ > $@

%.s: %.s.inc | config
	@echo "CONF $@"
	@$(CONFSUBST) $| $^ > $@

# generate dependency information for C files;
# comes directly from the GNU Make manual:
# https://www.gnu.org/software/make/manual/html_node/Automatic-Prerequisites.html#Automatic-Prerequisites
# with minor modifications to make sure that rules
# are generated with the full directory path included
# and only after all the generated headers have been
# created (so that gcc -M doesn't complain about missing headers)
%.d: %.c | $(headers)
	@echo "DEP $@"
	@set -e; $(CC) -M $(CFLAGS) $< | sed 's,\($*\)\.o[ :]*,\1.o $@: ,g' > $@;

# detect dependencies for all .c files within VPATH
sources=$(foreach src, $(foreach dir, $(VPATH), $(wildcard $(dir)/*.c)), $(notdir $(src)))
cdeps=$(sources:.c=.d)
include $(cdeps)

.PHONY: clean
clean:
	$(RM) $(cdeps) $(headers) $(objects) $(wildcard *.elf) $(wildcard *.mot)

