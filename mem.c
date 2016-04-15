#include "pdp6.h"
#include <ctype.h>

word memory[256*1024];
//hword maxmem = 256*1024;
hword maxmem = 64*1024;
word fmem[16];
word membus0, membus1;
word *hold;

void
readmem(char *file, word *mem, word size)
{
	FILE *f;
	char buf[100], *s;
	hword a;
	word w;
	if(f = fopen(file, "r"), f == nil)
		return;
	a = 0;
	while(s = fgets(buf, 100, f)){
		while(*s){
			if(*s == ';')
				break;
			else if('0' <= *s && *s <= '7'){
				w = strtol(s, &s, 8);
				if(*s == ':'){
					a = w;
					s++;
				}else
					mem[a++] = w;
			}else
				s++;
		}
	}
	fclose(f);
}

void
initmem(void)
{
	readmem("mem", memory, maxmem);
	readmem("fmem", fmem, 16);
}

void
dumpmem(void)
{
	hword a;
	FILE *f;

	if(f = fopen("memdump", "w"), f == nil)
		return;
	for(a = 0; a < 16; a++)
		fprint(f, "%02o: %012llo\n", a, fmem[a]);
	for(a = 0; a < maxmem; a++){
		if(memory[a]){
			fprint(f, "%06o: ", a);
			fprint(f, "%012llo\n", memory[a]);
		}
	}
	fclose(f);
}

/* When a cycle is requested we acknowledge the address
 * by pulsing the processor through the bus.
 * A read is completed immediately and signalled by a second pulse.
 * A write is completed on a second call. */
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
		if(membus0 & MEMBUS_RD_RQ){
			membus1 = *hold & FW;
			membus0 |= MEMBUS_MAI_RD_RS;
			hold = NULL;
		}
	}
	if(membus0 & MEMBUS_MAI_WR_RS && hold){
		*hold = membus1 & FW;
		membus0 &= ~MEMBUS_MAI_WR_RS;
		hold = NULL;
	}
}
