SRC=emu.c apr.c mem.c tty.c pt.c dc.c dt.c netmem.c cmd.c util.c threading.c rtc.c ../tools/pdp6common.c
H=pdp6.h ../tools/pdp6common.h threading.h
# clang
#CFLAGS= -Wno-shift-op-parentheses -Wno-logical-op-parentheses \
#        -Wno-bitwise-op-parentheses
CFLAGS=  -O3 -Wall -Wno-parentheses -fno-diagnostics-show-caret
#CFLAGS=  -g -Wall -Wno-parentheses -fno-diagnostics-show-caret
SDLFLAGS=`sdl2-config --cflags` `pkg-config SDL2_image --cflags`

LIBS=`sdl2-config --libs` `pkg-config SDL2_image --libs` -lpthread


pdp6: main_panel.c $(SRC) $(H)
	$(CC) -o $@ $(CFLAGS) $(SDLFLAGS) main_panel.c $(SRC) $(LIBS)
pdp6_s: main_serial.c $(SRC) $(H)
	$(CC) -o $@ $(CFLAGS) $(SDLFLAGS) main_serial.c $(SRC) $(LIBS)

test_dc: test_dc.c dc.c $(H)
	$(CC) -o $@ $(CFLAGS) test_dc.c dc.c

test_dt: test_dt.c dc.c dt.c $(H)
	$(CC) -o $@ $(CFLAGS) test_dt.c dc.c dt.c
