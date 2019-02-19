objects += $(archdir)/arch.o $(archdir)/boot.o

$(archdir)/boot.s.inc: $(archdir)/mkboot.sh
	NUM_IRQ=$(CONFIG_NUM_IRQ) $^ > $@

