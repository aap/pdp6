SRC=main.c apr.c mem.c tty.c pt.c
# clang
#CFLAGS= -Wno-shift-op-parentheses -Wno-logical-op-parentheses \
#        -Wno-bitwise-op-parentheses
CFLAGS=  -fno-diagnostics-show-caret \
	`sdl-config --cflags` `pkg-config SDL_image --cflags`

LIBS=	`sdl-config --libs` `pkg-config SDL_image --libs` -lpthread


pdp6: $(SRC) pdp6.h
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o pdp6

