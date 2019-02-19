
## Flashing

The generated `kernel.bin` file should be flashed onto the
board at address 0. Note that the lower part of the flash
range may be write-protected due to `BOOTPROT` being set
in the NVM User Row. In that case, follow the instructions
for NVM User Row programming.

### NVM User Row Programming

The generated file `userrow.mot` is an SREC-format
binary that many/most flash programmers will consume.
Use that file (e.g. `loadfile userrow.mot`) to reset
the user row to the factory defaults.

See `userrow.c` for changing user row values.

## Default Pinout

TODO
