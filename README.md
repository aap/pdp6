# PDP-6 Emulator and Verilog Simulation

This project aims to revive the PDP-6 (and later PDP-10)
computers by DEC.

I started by writing a very low level emulator in C based on
the schematics. Later I also wrote an accurate verilog simulation
that also works on an FPGA.

Both are driven by a virtual front panel but the plan is to create
a replica of the original panel.

The maintenance manual has flow charts, schematics and explanations:
[Volume1](http://bitsavers.trailing-edge.com/pdf/dec/pdp6/F-67_166instrManVol1_Sep65.pdf)
[Volume2](http://bitsavers.trailing-edge.com/pdf/dec/pdp6/F-67_166instrManVol2_Sep65.pdf)

![Screenshot of the front panel](https://raw.githubusercontent.com/aap/pdp6/master/art/screenshot.png)

## C Emulator

The code is more or less a transcription of the schematics into C.
This means you will not understand it unless you're familiar with the maintenance manual.
Pulses are represented as functions, when a pulse triggers another pulse
it does so by the `pulse` function which queues a pulse in a list
of pulses that are to happen, sorted chronologically.
Between pulses that happen at different times
various things are done like checking external pulses and advancing the
simulation state.
The timing is not yet 100% accurate but it's pretty close.

### Building

The supplied makefile assumes gcc (there are flags to silence some stupid warnings).
Otherwise you need SDL and pthread.

### Running

The cpu (apr), console tty, paper tape and punch,
the data control and DECtape are implemented.
The only things missing from the cpu is the repeat key mechanism.

## Verilog Simulation

The verilog code is a very accurate transcription of the schematics as well.
Since the real machine is asynchronous I had to pull some tricks to make it
work on an FPGA.
The real machine uses delays that are triggered by pulses and output another
pulse after some time. Instead of pulses I use clock enables, and delays are
implemented by a counter synchronized to the 100MHz system clock.

### FPGA

My FPGA board is a Terasic Cyclone V GX Starter Kit.
Communication with the virtual front panel is done over I²C via
GPIO pins 2 (SCL) and 3 (SDA).
The board's SRAM can also be read and written over I²C.
The TTY is connected to UART over GPIO pins 4 (RX) and 5 (TX)

## File tree
 
```
emu	the C emulator
emu/main_panel.c	main file for emulator with panel simulation
emu/main_serial.c	main file for emulator with panel over serial line
emu/emu.c	top level emulator code
emu/cmd.c	command line interface
emu/apr.c	Arithmetic Processor 166 emulation
emu/mem.c	core and fast memory emulation
emu/tty.c	Teleprinter 626 emulator
emu/pt.c	Paper tape reader 760 and punch 761 emulation
emu/dc.c	Data Control 136 emulation
emu/dt.c	DECtape 551 and 555 emulation
emu/netmem.c	network protocol for shared memory
emu/util.c	various utility functions
emu/util.h
emu/test_*.c	test code, not too important anymore
emu/pdp6.h	main header
emu/args.h	argument parsing
emu/elements.inc	panel definition
emu/cmds.txt	command line interface documentation
emu/init.ini	emulator init file
emu/mem_*	core memory backing store

tools
tools/dtr2dta.c	convert between raw (dtr) and simh (dta) DECtape format
tools/mkpty.c	make a pty and connect to the controlling tty
tools/mkpty33.c	as above but try to pretend an ASR33
tools/as6.c	an assembler, roughly modeled on MACRO
tools/ld6.c	a loader or relocatable files
tools/pdp6bin.h
tools/pdp6common.c	useful functions for PDP-6 code
tools/pdp6common.h
tools/rel.c	I have no recollection of this code
tools/reltest.c	old test code to create a REL file
tools/test.s	old test code for the assembler/linker
tools/test2.s
tools/ptdump.c	print a paper tape file in octal
tools/dtdump.c	print dtr DECtape

verilog
verilog/apr.v	Arithmetic Processor 166 simulation
verilog/core161c.v	Core memory 161C simulation
verilog/fast162.v	Fast memory 162 simulation
verilog/modules.v	utility modules
verilog/pdp6.v		top level module
verilog/quartus		various files for my terasic board
verilog/test_dec.v	inst decoding test
verilog/test.v		misc tests
verilog/test1.inc
verilog/test2.inc
verilog/test_fp.inc

code	random code for the PDP-6, mostly testing
code/bootstrap.txt	a list of boot loaders
code/dtboot.s		loads the first block from a DECtape
code/main.s		random entry
code/tty.s		tty character IO

panel	stand alone panel with lots of duplicate code

art	image files for the panel

misc	nothing important
```

## To do

- repeat and maint. switches
- improve timing
- implement 340 display
- do more tests
