#include "test.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "hps_0.h"
#include "led.h"

#define H2F_BASE (0xC0000000)

#define PERIPH_BASE (0xFC000000)
#define PERIPH_SPAN (0x04000000)
#define PERIPH_MASK (PERIPH_SPAN - 1)

#define LWH2F_BASE (0xFF200000)

u64 *h2f_base;
u32 *virtual_base;
u32 *getLWH2Faddr(u32 offset)
{
	return (u32*)((u32)virtual_base - PERIPH_BASE + (LWH2F_BASE+offset));
}
u64 *getH2Faddr(u32 offset)
{
	return (u64*)((u32)h2f_base + offset);
}

volatile u64 *h2f_axi_onchipmem;
volatile u32 *h2f_lw_led_addr;
volatile u32 *h2f_lw_sw_addr;


int
main(int argc, char **argv)
{
	int fd;
	int i;

	if((fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1) {
		fprintf(stderr, "ERROR: could not open /dev/mem...\n");
		return 1;
	}
	virtual_base = (u32*)mmap(nil, PERIPH_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, PERIPH_BASE);
	if(virtual_base == MAP_FAILED) {
		fprintf(stderr, "ERROR: mmap() failed...\n");
		close(fd);
		return 1;
	}
	h2f_base = (u64*)mmap(nil, 0x100000, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, H2F_BASE);
	if(h2f_base == MAP_FAILED) {
		fprintf(stderr, "ERROR: mmap() failed...\n");
		close(fd);
		return 1;
	}

	h2f_lw_led_addr = getLWH2Faddr(LED_PIO_BASE);
	h2f_lw_sw_addr = getLWH2Faddr(DIPSW_PIO_BASE);
	h2f_axi_onchipmem = getH2Faddr(0);

	volatile u32 *cntr_addr = getLWH2Faddr(0x10100);

	printf("%d\n", *cntr_addr);
	int n = 0;
//	for(i = 0; i < 50000000; i++)
//	for(i = 0; i < 1000000; i++)
//		n = *cntr_addr;
//	printf("%d\n", n);
//	return 0;

	h2f_axi_onchipmem[0] = 0xFFFF12345678;
	h2f_axi_onchipmem[1] = 0xEEEE11111111;
	h2f_axi_onchipmem[2] = 0xDDDD22222222;
	h2f_axi_onchipmem[3] = 0xCCCC33333333;
	h2f_axi_onchipmem[4] = 0xBBBB44444444;

	for(i = 0; i < 5; i++)
		printf("%llX\n", h2f_axi_onchipmem[i]);

	while(1) {
		printf("LED ON\n");
		for(i=0; i<=8; i++) {
			LEDR_LightCount(i);
			usleep(100*1000);
		}

		printf("LED OFF\n");
		for(i=0; i<=8; i++) {
			LEDR_OffCount(i);
			usleep(100*1000);
		}

		printf("sw: %X\n", *h2f_lw_sw_addr);
	}

	if(munmap(virtual_base, PERIPH_SPAN) != 0) {
		fprintf(stderr, "ERROR: munmap() failed...\n");
		close(fd);
		return 1;

	}
	close(fd);

	return 0;
}
