# Microcontroller HAL

## Navigating the Source

 - Portable code lives at the top level
 - Architecture-specific code lives in `arch/<arch>/`
 - MCU-specific code lives in `mcu/<mcu>/`
 - Board-specific code lives in `board/<board>`
 - Semi-portable code lives in `drivers/` (for example,
   code for vendor peripherals that isn't MCU-specific)

"Projects" live underneath the board directories and contain
a Makefile for building the necessary artifacts for the project.

## Configuration Format

Configuration files in the source tree use a simple text-based
format so that they can be read as POSIX shell scripts and as
Makefiles. The general form is:

```
# comment
CONFIG_KEY=value
```

Note that each variable begins with `CONFIG_`, and there are no
spaces around the `=` sign.

To keep parsers maximally dumb, comment lines must have '#'
as the first character of the line.
