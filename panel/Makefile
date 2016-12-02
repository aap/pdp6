SRC=panel6.c
# clang
#CFLAGS= -Wno-shift-op-parentheses -Wno-logical-op-parentheses \
#        -Wno-bitwise-op-parentheses
CFLAGS=  -g -fno-diagnostics-show-caret \
	`sdl-config --cflags` `pkg-config SDL_image --cflags`

LIBS=	`sdl-config --libs` `pkg-config SDL_image --libs` -lpthread


panel6: $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o panel6

