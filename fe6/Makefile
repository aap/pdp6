XX=/u/aap/de0-nano-soc/gcc-linaro-6.5.0-2018.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
CFLAGS=-DTEST

all: fe6_emu fe6_fake fe6

fe6_emu: fe6.c emu6.c cmd.c util.c pdp6common.c
	$(CC) $(CFLAGS) -o $@ $^

fe6_fake: fe6.c fake.c cmd.c util.c pdp6common.c
	$(CC) $(CFLAGS) -o $@ $^

fe6: fe6.c real6.c cmd.c util.c pdp6common.c
	$(XX)gcc $(CFLAGS) -o $@ $^
