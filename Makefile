SRC=main.c apr.c mem.c
CFLAGS=-Wno-shift-op-parentheses -Wno-logical-op-parentheses\
	 -L/usr/local/lib -I/usr/local/include -lSDL -lSDL_image -lpthread
pdp6: $(SRC) pdp6.h
	$(CC) $(CFLAGS) $(SRC) -o pdp6
