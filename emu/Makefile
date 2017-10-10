SRC=main.c cmd.c apr.c mem.c tty.c pt.c dc.c ../tools/pdp6common.c
H=pdp6.h ../tools/pdp6common.h
# clang
#CFLAGS= -Wno-shift-op-parentheses -Wno-logical-op-parentheses \
#        -Wno-bitwise-op-parentheses
CFLAGS=  -g -Wall -Wno-parentheses -fno-diagnostics-show-caret \
	`sdl-config --cflags` `pkg-config SDL_image --cflags`

LIBS=	`sdl-config --libs` `pkg-config SDL_image --libs` -lpthread


pdp6: $(SRC) $(H)
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o pdp6

