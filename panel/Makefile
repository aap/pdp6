SRC=panel6.c ../emu/util.c
# clang
#CFLAGS= -Wno-shift-op-parentheses -Wno-logical-op-parentheses \
#        -Wno-bitwise-op-parentheses
CFLAGS=  -g -fno-diagnostics-show-caret \
	`sdl2-config --cflags` `pkg-config SDL2_image --cflags`

LIBS=	`sdl2-config --libs` `pkg-config SDL2_image --libs` -lpthread


panel6: $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o panel6

