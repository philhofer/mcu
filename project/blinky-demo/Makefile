CROSS:=arm-unknown-eabi
CC=${CROSS}-gcc
LD=${CROSS}-ld
AS=${CROSS}-as
OBJCOPY=${CROSS}-objcopy
CFLAGS="-g" "-Os"
LDFLAGS=$(shell CC=$(CC) ../../auto-ldflags)
objects += start.o

include ../../rules.mk

.PHONY: all
all: kernel.elf userrow.elf
