SRC=emu.c apr.c mem.c tty.c pt.c dc.c dt.c netmem.c netcons.c cmd.c util.c threading.c rtc.c ../tools/pdp6common.c
H=pdp6.h ../tools/pdp6common.h threading.h
# clang
#CFLAGS= -Wno-shift-op-parentheses -Wno-logical-op-parentheses \
#        -Wno-bitwise-op-parentheses
#GCCFLAGS=-fno-diagnostics-show-caret
CFLAGS= -g -O3 -Wall -Wno-parentheses $(GCCFLAGS)
#CFLAGS= -g -Wall -Wno-parentheses -fno-diagnostics-show-caret
SDLFLAGS=`sdl2-config --cflags` `pkg-config SDL2_image --cflags` -DGRAPHICS

LIBS=-lpthread -lm
SDLLIBS=`sdl2-config --libs` `pkg-config SDL2_image --libs`


pdp6: main_panel.c dis340.c chargen.inc joy.c $(SRC) $(H)
	$(CC) -o $@ $(CFLAGS) $(SDLFLAGS) main_panel.c dis340.c joy.c $(SRC) $(LIBS) $(SDLLIBS)
pdp6_s: main_serial.c $(SRC) $(H)
	$(CC) -o $@ $(CFLAGS) main_serial.c $(SRC) $(LIBS)
pdp6_n: main_net.c $(SRC) $(H)
	$(CC) -o $@ $(CFLAGS) main_net.c $(SRC) $(LIBS)

test_dc: test_dc.c dc.c $(H)
	$(CC) -o $@ $(CFLAGS) test_dc.c dc.c

test_dt: test_dt.c dc.c dt.c $(H)
	$(CC) -o $@ $(CFLAGS) test_dt.c dc.c dt.c

test_apr: test_apr.c main_panel.c dis340.c $(SRC) $(H)
	$(CC) -o $@ $(CFLAGS) $(SDLFLAGS) test_apr.c main_panel.c dis340.c $(SRC) $(LIBS) $(SDLLIBS)
