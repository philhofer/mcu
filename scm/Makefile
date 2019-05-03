.SUFFIXES :=
.DEFAULT_GOAL := all
CSC := csc
EXTENSIONS := sharp-string
CSCFLAGS = -X sharp-string -O2

src=config-base.scm arm.scm sam.scm samd21.scm samd09.scm feather-m0.scm itsybitsy-m0.scm

# special rules for the sharp string syntax compiler extension
sharp-string.so sharp-string.import.scm: sharp-string.scm
	@echo "CSC $<"
	@$(CSC) -s -J -O2 $<

%.import.hash: %.import.scm
	@sha256sum $< > $@.tmp
	@if test -r $@; then cmp $@ $@.tmp || mv -f $@.tmp $@; else mv $@.tmp $@; fi

%.so %.types %.import.scm: %.scm | sharp-string.so
	@echo "CSC $<"
	@$(CSC) -s -J $(CSCFLAGS) $< -ot $*.types

%.dep: %.scm scmdep
	@echo "DEP $<"
	@./scmdep $< > $@

include $(src:.scm=.dep)

.PHONY: all clean
all: $(src:.scm=.so)
clean:
	$(RM) *.so *.types *.import.scm *.dep
