#include "pdp6.h"

word memory[256*1024];
//hword maxmem = 256*1024;
hword maxmem = 64*1024;
word fmem[16];
word membus0, membus1;
word *hold;

void
initmem(void)
{
	FILE *f;
	word w;
	hword a;
	if(f = fopen("mem", "r"), f == NULL)
		return;
	for(a = 0; a < maxmem && fscanf(f, "%lo", &w) != EOF; a++)
		memory[a] = w;
	fclose(f);
}

void
wakemem(void)
{
	hword a;
	if(membus0 & MEMBUS_RQ_CYC){
		a = membus0>>4 & 037777;
		if(membus0 & MEMBUS_MA21_1) a |= 0040000;
		if(membus0 & MEMBUS_MA20_1) a |= 0100000;
		if(membus0 & MEMBUS_MA19_1) a |= 0200000;
		if(membus0 & MEMBUS_MA18_1) a |= 0400000;
		if(a >= maxmem ||
		   membus0 & MEMBUS_MA_FMC_SEL1 && a >= 16)
			return;

		membus0 |= MEMBUS_MAI_ADDR_ACK;
		hold = membus0 & MEMBUS_MA_FMC_SEL1 ? &fmem[a] : &memory[a];
//		printf(" ACK: %o\n", a);
		if(membus0 & MEMBUS_RD_RQ){
			membus1 = *hold;
			membus0 |= MEMBUS_MAI_RD_RS;
			hold = NULL;
		}
	}
	if(membus0 & MEMBUS_MAI_WR_RS && hold){
		*hold = membus1;
		membus0 &= ~MEMBUS_MAI_WR_RS;
		hold = NULL;
	}
}
