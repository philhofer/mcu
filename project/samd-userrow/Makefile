CROSS:=arm-unknown-eabi
CC=${CROSS}-gcc
LD=${CROSS}-ld
AS=${CROSS}-as
OBJCOPY=${CROSS}-objcopy
CFLAGS="-g" "-Os"
LDFLAGS=$(shell CC=$(CC) ../../auto-ldflags)

objects += userrow.o

userrow.o: userrow.c
	$(CC) $(CFLAGS) -c $< -o $@

userrow.elf: userrow.o userrow.ld
	$(LD) -T userrow.ld $< -o $@

.PHONY: all
all: userrow.elf
