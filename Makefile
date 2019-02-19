include config.mk

rootdir=.
driverdir=$(rootdir)/drivers
archdir=$(rootdir)/$(CONFIG_ARCH)
boarddir=$(rootdir)/$(CONFIG_ARCH)/$(CONFIG_BOARD)

include $(archdir)/arch.mk
include $(boarddir)/board.mk

CFLAGS += -Wall -ggdb -I$(rootdir) -I$(archdir) -I$(boarddir)

objects += idle.o start.o libc.o

ifeq ($(CONFIG_I2C), y)
	objects += i2c.o
endif

ifeq ($(CONFIG_GPIO), y)
	objects += gpio.o
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

%.elf: $(objects) | $(boarddir)/board.ld
	@echo "LD $@"
	@$(LD) $(LDFLAGS) -T $| -o $@ $^ -lgcc
	@chmod -x $@

%.h: %.h.inc
	@echo "CONF $@"
	@./confsubst $(boarddir)/board.config $^ > $@

%.s: %.s.inc
	@echo "CONF $@"
	@./confsubst $(boarddir)/board.config $^ > $@

# generate dependency information for C files;
# comes directly from the GNU Make manual:
# https://www.gnu.org/software/make/manual/html_node/Automatic-Prerequisites.html#Automatic-Prerequisites
# with minor modifications to make sure that rules
# are generated with the full directory path included
# and only after all the generated headers have been
# created (so that gcc -M doesn't complain about missing headers)
%.d: %.c | $(headers)
	@echo "DEP $@"
	@set -e; $(CC) -M $(CFLAGS) $< | sed "s,\($(notdir $*)\)\.o[ :]*,$(dir $<)\1.o $@: ,g" > $@;

sources=$(wildcard $(rootdir)/*.c $(archdir)/*.c $(boarddir)/*.c)
cdeps=$(sources:.c=.d)
include $(cdeps)

.PHONY: clean
clean:
	$(RM) $(objects) $(headers) $(cdeps) $(boarddir)/kernel.elf $(boarddir)/kernel.bin

.PHONY: all
all: $(boarddir)/kernel.bin
