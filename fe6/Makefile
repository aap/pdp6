XX=/u/aap/de0-nano-soc/gcc-linaro-6.5.0-2018.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
CFLAGS=-DTEST
LDFLAGS=-lpthread

SRC=fe.c cmd.c util.c netmem.c pdp6common.c threading.c

all: fe6_emu fe6_fake fe6 feka
clean:
	rm fe6_emu fe6_fake fe6

fe6: $(SRC) 6/real6.c
	$(XX)gcc -I6 $(CFLAGS) $(LDFLAGS) -o $@ $^

fe6_fake: $(SRC) fake.c
	$(CC) -I6 $(CFLAGS) $(LDFLAGS) -o $@ $^

fe6_emu: $(SRC) 6/emu6.c
	$(CC) -I6 $(CFLAGS) $(LDFLAGS) -o $@ $^


fe6_fastemu: $(SRC) fastemu.c ../femu/fpdp6.c
	$(CC) -I6 $(CFLAGS) $(LDFLAGS) -o $@ $^

feka: $(SRC) ka/real.c
	$(XX)gcc -Ika $(CFLAGS) $(LDFLAGS) -o $@ $^
