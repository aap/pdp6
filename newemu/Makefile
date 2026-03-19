INC=`pkg-config lua --cflags`
LIBS=-lpthread -lm -lSDL2 `pkg-config lua --libs`
#CFLAGS=-Wall -Wextra -g -O3
#CFLAGS=-g -O3 -mcpu=cortex-a53 -mtune=cortex-a53
CFLAGS=-g -Wall -Wno-parentheses

pdp6: main.c panel6.c pdp6.c netmem.c tty.c pt.c dis340.c dc.c ut.c ge.c audio.c ttytelnet.c common.c lua.c
	cc $(CFLAGS) -o $@ $^ $(INC) $(LIBS)

run: pdp6
	./pdp6
