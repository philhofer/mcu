objects += $(archdir)/arch.o $(archdir)/boot.o

$(archdir)/boot.s.inc: $(realpath mkboot.sh)
	$> > $@

