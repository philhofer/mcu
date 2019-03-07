CC := gcc
LD := gcc
CFLAGS = -Wall -Werror -I. -I../ -O1 -g -ggdb
LDFLAGS = -g -ggdb

VPATH += ../

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.d: %.c
	@echo "DEP $@"
	@set -e; $(CC) -M $(CFLAGS) $< | sed 's,\($*\)\.o[ :]*,\1.o $@: ,g' > $@;

sources = $(foreach src, $(foreach dir, $(VPATH), $(wildcard $(dir)/*.c)), $(notdir $(src)))
sources += $(wildcard *.c)
cdeps = $(sources:.c=.d)
include $(cdeps)

%-test: %-test.o
	$(LD) $(LDFLAGS) $^ -o $@

# It isn't clear we can avoid listing
# explicit object file dependencies here
# for each test... oh well
idle-test: idle-test.o idle.o arch.o

cdc-acm-test: cdc-acm-test.o idle.o arch.o usb.o usb-cdc-acm.o usb-loopback.o

norm-test: norm.o

.DEFAULT_GOAL := all
.PHONY: all clean
all: $(wildcard *-test)
	@set -e; for tst in $^; do ./$$tst; echo "$$tst PASS"; done
clean:
	rm $(cdeps) $(wildcard *.o)
