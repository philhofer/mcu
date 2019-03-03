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

## Kernel Details

### Execution Contexts

The kernel supports three rudimentary execution contexts, which
are as follows:

 - Normal: ordinary execution, beginning with `start()`
 - Interrupt: execution from within an interrupt handler
 - Idle: execution from within `idle_step()` (see idle.c).

The "normal" and "interrupt" contexts behave as one would expect:
interrupts can arrive and preempt "normal" execution at any time,
with the following caveats:

 - `irq_disable()` disables all interrupt sources from within normal context
 - `irq_enable()` enables all interrupt sources from within normal context
 - `irq_disable_num()` disables a specific interrupt from any context
 - `irq_enable_num()` enables a specific interrupt from any context

The `start()` function begins with interrupts enabled, but typically there
are few or zero interrupt sources enabled until peripherals that use
interrupts are initialized. Generally, peripherals will manage their
own interrupts.

The "idle" context is unusual, but it serves an important architectural
purpose: it executes synchronously with the "normal" context, and only
on demand. Typically, peripheral interrupt handlers will only do a little
bit of work, and then call `schedule_work()` on further work that needs
to be done to handle the interrupt. That work isn't executed until
`idle_step()` is called some time later in normal context. Consequently,
normal context work can perform time-sensitive work without worrying that
interrupt context preemption will introduce jitter. Moreover, normal context
code can typically be written to avoid disabling interrupts for safely mutating
data structures or performing time-critical work. (Handling critical sections
correctly is a source of complexity and difficult-to-reproduce bugs!)

