# MCU Kernel-As-A-Library

## Getting Started

You'll need to run `config.sh` to generate `tup.config`, which tells
the build system what files to be built.

Put your kernel source files in the root directory; regular C and assembly
source files will be built automatically and linked into your kernel.

The kernel entry point for all architectures is `start()`. Note that plenty
of architecture- and device-specific code has executed at this point.

Running `tup` will build your kernel; generally you'll find an ELF image
(`kernel.elf`) and a raw binary (`kernel.bin`) in the directory of the device target.

## Navigating the Source

The source tree is organized such that more generic code lives
at higher levels of the source tree, and more device-specific /
non-portable code lives deeper in the tree. Consequently, as a general
rule, files deeper in the tree can depend upon (i.e. include or link against)
files higher up in the tree, but not vice-versa.

Conventionally, the first two parts of the directory path for a device
are "arch" and "board," respectively. For example, the Atmega ATSAMD21G18 board
is an ARMv6-M device, so its configuration lives at `armv6m/atsamd21g18/`.
