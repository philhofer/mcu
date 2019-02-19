# Microcontroller HAL

## Navigating the Source

The source tree is organized such that more generic code lives
at higher levels of the source tree, and more device-specific /
non-portable code lives deeper in the tree. Consequently, as a general
rule, files deeper in the tree can depend upon (i.e. include or link against)
files higher up in the tree, but not vice-versa.

Conventionally, the first two parts of the directory path for a device
are "arch" and "board," respectively. For example, the Atmega ATSAMD21G18 board
is an ARMv6-M device, so its configuration lives at `armv6m/atsamd21g18/`.

