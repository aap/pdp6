SRC=main.c apr.c mem.c io.c
# clang
#CFLAGS= -Wno-shift-op-parentheses -Wno-logical-op-parentheses \
#        -Wno-bitwise-op-parentheses
CFLAGS=  \
	-fno-diagnostics-show-caret \
        -L/usr/local/lib -I/usr/local/include -lSDL -lSDL_image -lpthread


pdp6: $(SRC) pdp6.h
	$(CC) $(CFLAGS) $(SRC) -o pdp6

as: test.s
	as10 <test.s
	rim2mem <a.rim >mem
