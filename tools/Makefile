all: as6 ld6 ptdump

as6: as6.c pdp6common.c
	cc -g as6.c pdp6common.c -o as6
ld6: ld6.c pdp6common.c
	cc -g ld6.c pdp6common.c -o ld6
ptdump: ptdump.c pdp6common.c
	cc -g ptdump.c pdp6common.c -o ptdump
