CC := gcc
LD := gcc
CFLAGS ?= -Wall -Werror -I. -I../ -O1 -ggdb

VPATH += ../

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.d: %.c
	@echo "DEP $@"
	@set -e; $(CC) -M $(CFLAGS) $< | sed 's,\($*\)\.o[ :]*,\1.o $@: ,g' > $@;

sources = $(foreach src, $(foreach dir, $(VPATH), $(wildcard $(dir)/*.c)), $(notdir $(src)))
cdeps = $(sources:.c=.d)
include $(cdeps)

%-test: %-test.o
	$(LD) $^ -o $@

# It isn't clear we can avoid listing
# explicit object file dependencies here
# for each test... oh well
idle-test: idle-test.o idle.o arch.o

.PHONY: all
all: $(wildcard *-test)
	@set -e; for tst in $^; do ./$$tst; echo "$$tst PASS"; done
