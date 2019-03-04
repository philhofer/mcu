
submakes=$(shell find . -name Makefile -type f -mindepth 2)
subdirs=$(foreach file,$(submakes),$(dir $(file)))
nproc=$(shell nproc)

.DEFAULT_GOAL := all
.PHONY: all clean test
all:
	set -e; for d in $(subdirs); do $(MAKE) -j$(nproc) -C $$d all; done
clean:
	set -e; for d in $(subdirs); do $(MAKE) -C $$d clean; done
test:
	$(MAKE) -C host-test/ all
