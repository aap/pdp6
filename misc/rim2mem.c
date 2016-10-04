#include <stdio.h>
#include <stdint.h>

typedef uint64_t word;
typedef uint32_t hword;

word mem[1<<18];
hword maxmem, pc;

word
rimword(void)
{
	int i, b;
	word w;
	w = 0;
	for(i = 0; i < 6;){
		if(b = getchar(), b == EOF)
			return ~0;
		if(b & 0200){
			w = (w << 6) | (b & 077);
			i++;
		}
	}
	return w;
}

int
readrim(void)
{
	word w, n, sum;
	word p;
	for(;;){
		sum = n = rimword();
		if(n == ~0){
			fprintf(stderr, "EOF at header\n");
			return 0;
		}
		if((n & 0777000000000) == 0254000000000){
			pc = n & 0777777;
			break;
		}
		p = n+1 & 0777777;
		for(; n & 0400000000000; n += 01000000){
			w = rimword();
			if(w == ~0){
				fprintf(stderr, "EOF at data\n");
				return 0;
			}
			mem[p++] = w;
			sum += w;
		}
		if(p > maxmem)
			maxmem = p;
		w = rimword();
		if(w == ~0){
			fprintf(stderr, "EOF at checksum\n");
			return 0;
		}
		if((sum + w) & 0777777777777){
			fprintf(stderr, "checksum error %lo %lo\n", sum, w);
			return 0;
		}
	}
	return 1;
}

void
writemem(void)
{
	hword i;
	for(i = 0; i < maxmem; i++)
		printf("%lo\n", mem[i]);
}

int
main()
{
	if(readrim()){
		writemem();
		fprintf(stderr, "pc: %o\n", pc);
		return 0;
	}
	return 1;
}
