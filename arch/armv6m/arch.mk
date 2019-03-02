objects += arch.o boot.o

boot.s.inc: $(archdir)/mkboot.sh
	NUM_IRQ=$(CONFIG_NUM_IRQ) $^ > $@

boot.o: boot.s

