# default Makefile rules for all targets

.SUFFIXES:

# necessary for csi invocation
rootdir=../../
scmdir=$(rootdir)/scm
CHICKEN_REPOSITORY_PATH="$(shell chicken-install -repository):$(realpath $(scmdir))"

config.mk config.c config.h boot.s board.ld board.h: configure $(wildcard $(scmdir)/*.so)
	@CHICKEN_REPOSITORY_PATH=$(CHICKEN_REPOSITORY_PATH) ./configure

include config.mk # defines CONFIG_ARCH
archdir=$(rootdir)/arch/$(CONFIG_ARCH)
mcudir=$(rootdir)/kernel/$(CONFIG_MCU_CLASS)

# for essentially all targets, all we're
# interested in building is the ELF objects
# (but for some targets with weird flash programmers
# we may want raw binary, .srec, etc.)
.DEFAULT_GOAL:=kernel.elf

VPATH += $(rootdir) $(archdir) $(mcudir) .
CFLAGS += -I. -I$(rootdir) -I$(archdir)
CFLAGS += -Wall -ggdb

# fake prerequisite for determining C deps;
# otherwise computing deps will fail before
# configure is run
headers += config.h board.h

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

%.elf: $(objects) | board.ld
	@echo "LD $@"
	@$(LD) $(LDFLAGS) -T $| -o $@ $^ -lgcc
	@chmod -x $@


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

