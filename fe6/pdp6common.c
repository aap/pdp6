#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "pdp6common.h"

word fw(hword l, hword r) { return ((word)right(l) << 18) | (word)right(r); }
word point(word pos, word sz, hword p) { return (pos<<30)|(sz<<24)|p; }
hword left(word w) { return (w >> 18) & 0777777; }
hword right(word w) { return w & 0777777; }
word negw(word w) { return (~w + 1) & 0777777777777; }
int isneg(word w) { return !!(w & 0400000000); }

/* write word in rim format */
void
writew(word w, FILE *fp)
{
	int j;
	w &= 0777777777777;
	for(j = 5; j >= 0; j--)
		putc(0200 | (w>>j*6)&077, fp);
}

word
readw(FILE *fp)
{
	int i, b;
	word w;
	w = 0;
	for(i = 0; i < 6; i++){
	cont:
		if(b = getc(fp), b == EOF)
			return ~0;
		if((b & 0200) == 0)
			goto cont;
		w = (w << 6) | (b & 077);
	}
	return w;
}

/* write word in backup format. This is what pdp10-ka expects. */
void
writewbak(word w, FILE *fp)
{
	putc((w >> 29) & 0177, fp);
	putc((w >> 22) & 0177, fp);
	putc((w >> 15) & 0177, fp);
	putc((w >> 8) & 0177, fp);
	putc((w>>1) & 0177 | (w & 1) << 7, fp);
}

word
readwbak(FILE *fp)
{
	char buf[5];
	if(fread(buf, 1, 5, fp) != 5)
		return ~0;
	return (word)buf[0] << 29 |
	       (word)buf[1] << 22 |
	       (word)buf[2] << 15 |
	       (word)buf[3] << 8 |
	       ((word)buf[4]&0177) << 1 |
	       ((word)buf[4]&0200) >> 7;
}


static int prevbyte = -1;
static void
flush(FILE *fp)
{
	if(prevbyte == 015)
		putc(0356, fp);
	else if(prevbyte == 0177)
		putc(0357, fp);
	prevbyte = -1;
}

static void
binword(word w, FILE *fp)
{
	flush(fp);

	putc(w>>32 & 017 | 0360, fp);
	putc(w>>24 & 0377, fp);
	putc(w>>16 & 0377, fp);
	putc(w>>8 & 0377, fp);
	putc(w & 0377, fp);
}

static void
asciiword(word w, FILE *fp)
{
	int i;
	char b, bytes[5];

	bytes[0] = w>>29 & 0177;
	bytes[1] = w>>22 & 0177;
	bytes[2] = w>>15 & 0177;
	bytes[3] = w>>8 & 0177;
	bytes[4] = w>>1 & 0177;

	for(i = 0; i < 5; i++){
		b = bytes[i];

again:
		if(prevbyte == 015){
			prevbyte = -1;
			if(b == 012)
				putc(012, fp);
			else{
				putc(0356, fp);
				goto again;
			}
		}else if(prevbyte == 0177){
			prevbyte = -1;
			switch(b){
			case 7: putc(0177, fp); break;
			case 012: putc(0215, fp); break;
			case 015: putc(0212, fp); break;
			case 0177: putc(0207, fp); break;
			default:
				if(b <= 0155)
					putc(b + 0200, fp);
				else{
					putc(0357, fp);
					goto again;
				}
			}
		}else if(b == 015 || b == 0177)
			prevbyte = b;
		else if(b == 012)
			putc(015, fp);
		else
			putc(b, fp);
	}
}

/* write word in ITS evacuate format. */
void
writewits(word w, FILE *fp)
{
	if(w == ~0){
		flush(fp);
		return;
	}

	if(w & 1)
		binword(w, fp);
	else
		asciiword(w, fp);
}

/* read word in ITS evacuate format. */
static int leftover = -1;
word
readwits(FILE *fp)
{
#define PUSH(x) w = (w<<7) | ((x) & 0177); bits += 7
	int o;
	word w;
	int bits;

	if(fp == NULL || feof(fp)){
		leftover = -1;
		return ~0;
	}

	w = 0;
	bits = 0;

	if(leftover >= 0){
		w = leftover;
		bits = 7;
		leftover = -1;
	}

	while(bits < 36){
		o = getc(fp);
		if(o == EOF)
			if(bits == 0)
				return ~0;

		if(o == 012){
			PUSH(015);
			PUSH(012);
		}else if(o == 015){
			PUSH(012);
		}else if(o <= 0176){
			PUSH(o);
		}else if(o == 0177){
			PUSH(0177);
			PUSH(7);
		}else if(o == 0207){
			PUSH(0177);
			PUSH(0177);
		}else if(o == 0212){
			PUSH(0177);
			PUSH(015);
		}else if(o == 0215){
			PUSH(0177);
			PUSH(012);
		}else if(o <= 0355){
			PUSH(0177);
			PUSH(o - 0200);
		}else if(o == 0356){
			PUSH(015);
		}else if(o == 0357){
			PUSH(0177);
		}else{
			if(bits != 0){
				fprintf(stderr, "[error in 36-bit file format]\n");
				exit(1);
			}
			w = o & 017;
			w = (w << 8) | getc(fp);
			w = (w << 8) | getc(fp);
			w = (w << 8) | getc(fp);
			w = (w << 8) | getc(fp);
			bits = 36;
		}

		if(bits == 35){
			w <<= 1;
			bits++;
		}else if(bits == 42){
			leftover = w & 0177;
			w >>= 7;
			w <<= 1;
		}
	}

	if(w > 0777777777777){
		fprintf(stderr, "[error in 36-bit file format (word too large)]\n");
		exit(1);
	}

	return w;
}

/* decompose a double into sign, exponent and mantissa */
void
decompdbl(double d, int *s, word *e, uint64_t *m)
{
	uint64_t x;
	union {
		uint64_t i;
		double d;
	} u;

	u.d = d; x = u.i;
	*s = !!(x & 0x8000000000000000);
	*e = (x >> 52) & 0x7FF;
	*m = x & 0xFFFFFFFFFFFFF;
	if(x != 0)
		*m |= 0x10000000000000;
}

/* convert double to PDP-6 float */
word
dtopdp(double d)
{
	uint64_t x, e, m;
	int sign;
	word f;
	union {
		uint64_t i;
		double d;
	} u;

	sign = 0;
	if(d < 0.0){
		sign = 1;
		d *= -1.0;
	}
	u.d = d; x = u.i;
	/* sign is guaranteed to be 0 now */
	e = (x >> 52) & 0x7FF;
	m = x & 0xFFFFFFFFFFFFF;
	e += 128-1023;
	m >>= 25;
	/* normalize */
	if(x != 0){
		m >>= 1;
		m |= 0400000000;
		e += 1;
	}else
		e = 0;
	f = e << 27;
	f |= m;
	if(sign)
		f = -f & 0777777777777;
	return f;
}

/* convert PDP-6 float to double */
double
pdptod(word f)
{
	uint64_t x, s, e, m;
	union {
		uint64_t i;
		double d;
	} u;

	s = 0;
	if(f & 0400000000000){
		f = -f & 0777777777777;
		s = 1;
	}
	e = (f >> 27) & 0377;
	m = f & 0777777777;
	e += 1023-128;
	/* normalize */
	if(m != 0){
		m &= ~0400000000;
		m <<= 1;
		e -= 1;
	}else
		e = 0;
	m <<= 25;
	x = m;
	x |= (e & 0x7FF) << 52;
	x |= s << 63;
	u.i = x;
	return u.d;
}

/* map ascii to radix50/squoze, also map lower to upper case */
char
ascii2rad(char c)
{
	static char tab[] = {
		 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
		 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
		 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
		 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
		  0,  -1,  -1,  -1, 046, 047,  -1,  -1,
		 -1,  -1,  -1,  -1,  -1,  -1, 045,  -1,
		001, 002, 003, 004, 005, 006, 007, 010,
		011, 012,  -1,  -1,  -1,  -1,  -1,  -1,
		 -1, 013, 014, 015, 016, 017, 020, 021,
		022, 023, 024, 025, 026, 027, 030, 031,
		032, 033, 034, 035, 036, 037, 040, 041,
		042, 043, 044,  -1,  -1,  -1,  -1,  -1,
		 -1, 013, 014, 015, 016, 017, 020, 021,
		022, 023, 024, 025, 026, 027, 030, 031,
		032, 033, 034, 035, 036, 037, 040, 041,
		042, 043, 044,  -1,  -1,  -1,  -1,  -1,
	};
	return tab[c&0177];
}

static char rad50tab[] = {
	' ', '0', '1', '2', '3', '4', '5', '6',
	'7', '8', '9', 'A', 'B', 'C', 'D', 'E',
	'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U',
	'V', 'W', 'X', 'Y', 'Z', '.', '$', '%',
	0
};

/* map radix50/squoze to ascii */
char
rad2ascii(char c)
{
	return rad50tab[c%050];
}

int
israd50(char c)
{
	return strchr(rad50tab, toupper(c)) != NULL;
}

/* convert ascii string + code to radix50 */
word
rad50(int n, const char *s)
{
	word r;
	char c;
	int i;

	r = 0;
	i = 0;
	for(i = 0; i < 6 && *s; i++){
		c = ascii2rad(*s++);
		if(c < 0)
			break;
		r = r*050 + c;
	}
	for(; i < 6; i++)
		r = r*050;
	r |= (word)(n&074) << 30;
	return r;
}

/* get null-terminated ascii string and code from radix50 */
int
unrad50(word r, char *s)
{
	int i;
	int n;
	n = r>>30 & 074;
	r &= ~0740000000000;
	s += 6;
	*s-- = '\0';
	for(i = 0; i < 6; i++){
		*s-- = rad2ascii(r%050);
		r /= 050;
	}
	return n;
}

/* map ascii to sixbit, also map lower to upper case */
char
ascii2sixbit(char c)
{
	static char tab[] = {
		 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
		 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
		 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
		 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,

		000, 001, 002, 003, 004, 005, 006, 007,
		010, 011, 012, 013, 014, 015, 016, 017,
		020, 021, 022, 023, 024, 025, 026, 027,
		030, 031, 032, 033, 034, 035, 036, 037,
		040, 041, 042, 043, 044, 045, 046, 047,
		050, 051, 052, 053, 054, 055, 056, 057,
		060, 061, 062, 063, 064, 065, 066, 067,
		070, 071, 072, 073, 074, 075, 076, 077,

		040, 041, 042, 043, 044, 045, 046, 047,
		050, 051, 052, 053, 054, 055, 056, 057,
		060, 061, 062, 063, 064, 065, 066, 067,
		070, 071, 072, 073, 074, 075, 076, 077,
	};
	return tab[c&0177];
}

static char sixbittab[] = {
	' ', '!', '"', '#', '$', '%', '&', '\'',
	'(', ')', '*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', ':', ';', '<', '=', '>', '?',
	'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	'X', 'Y', 'Z', '[', '\\', ']', '^', '_',
	0
	
};


/* map sixbit to ascii */
char
sixbit2ascii(char c)
{
	return sixbittab[c&077];
}

int
issixbit(char c)
{
	return strchr(sixbittab, toupper(c)) != NULL;
}

/* convert ascii string to sixbit */
word
sixbit(const char *s)
{
	word sx;
	char c;
	int i;
	sx = 0;
	i = 0;
	for(i = 0; i < 6 && *s; i++){
		c = ascii2sixbit(*s++);
		if(c < 0)
			break;
		sx = (sx<<6) + c;
	}
	sx <<= 6*(6-i);
	return sx;
}

/* get null-terminated ascii string from sixbit */
void
unsixbit(word sx, char *s)
{
	int i;
	s += 6;
	*s-- = '\0';
	for(i = 0; i < 6; i++){
		*s-- = sixbit2ascii(sx&077);
		sx >>= 6;
	}
}


enum
{
	NOINST = 1,	// not a valid instruction
	NUMY = 2,	// numeric Y
	NOAC = 4,	// don't print AC if zero
	NOY = 8,	// don't print Y if zero
};


int instflags[0700] = {
	NOINST, NOINST, NOINST, NOINST,
	NOINST, NOINST, NOINST, NOINST,
	NOINST, NOINST, NOINST, NOINST,
	NOINST, NOINST, NOINST, NOINST,
	NOINST, NOINST, NOINST, NOINST,
	NOINST, NOINST, NOINST, NOINST,
	NOINST, NOINST, NOINST, NOINST,
	NOINST, NOINST, NOINST, NOINST,
	NOINST, NOINST, NOINST, NOINST,
	NOINST, NOINST, NOINST, NOINST,
	NOINST, NOINST, NOINST, NOINST,
	NOINST, NOINST, NOINST, NOINST,
	NOINST, NOINST, NOINST, NOINST,
	NOINST, NOINST, NOINST, NOINST,
	NOINST, NOINST, NOINST, NOINST,
	NOINST, NOINST, NOINST, NOINST,

	NOINST, NOINST, NOINST, NOINST,
	NOINST, NOINST, NOINST, NOINST,
	NOINST, NOINST, NOINST, NOINST,
	NOINST, NOINST, NOINST, NOINST,
	NOINST, NOINST, NOINST, NOINST,
	NOINST, NOINST, NOINST, NOINST,
	NOINST, NOINST, NUMY, NOAC,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,

	0, 0, 0, NOAC,
	0, 0, 0, NOAC,
	0, 0, 0, NOAC,
	0, 0, 0, NOAC,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	NUMY, NUMY, NUMY, NOINST,
	NUMY, NUMY, NUMY, NOINST,
	0, 0, 0, 0,
	NOAC, NOAC, NOAC, NOINST,
	0, 0, 0, NOY,
	NOAC, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,

	NOAC|NOY, 0, 0, 0,
	NOAC|NOY, 0, 0, 0,
	NOAC|NOY, 0, 0, 0,
	NOAC|NOY, 0, 0, 0,
	NOAC|NOY, 0, 0, 0,
	NOAC|NOY, 0, 0, 0,
	NOAC, NOAC, NOAC, NOAC,
	NOAC, NOAC, NOAC, NOAC,
	0, 0, 0, 0,
	0, 0, 0, 0,
	NOAC, NOAC, NOAC, NOAC,
	NOAC, NOAC, NOAC, NOAC,
	0, 0, 0, 0,
	0, 0, 0, 0,
	NOAC, NOAC, NOAC, NOAC,
	NOAC, NOAC, NOAC, NOAC,

	NOY, NOY, NOAC, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, NOAC, 0,
	0, 0, 0, 0,
	NOY, NOY, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	NOY, NOY, 0, 0,
	0, 0, 0, 0,
	0, 0, NOAC, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	NOY, NOY, NOAC, 0,

	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,

	NOAC|NOY|NUMY, NOAC|NOY|NUMY, NUMY, NUMY,
	NOAC|NOY|NUMY, NOAC|NOY|NUMY, NUMY, NUMY,
	NOAC|NOY, NOAC|NOY, 0, 0,
	NOAC|NOY, NOAC|NOY, 0, 0,
	NUMY, NUMY, NUMY, NUMY,
	NUMY, NUMY, NUMY, NUMY,
	0, 0, 0, 0,
	0, 0, 0, 0,
	NUMY, NUMY, NUMY, NUMY,
	NUMY, NUMY, NUMY, NUMY,
	0, 0, 0, 0,
	0, 0, 0, 0,
	NUMY, NUMY, NUMY, NUMY,
	NUMY, NUMY, NUMY, NUMY,
	0, 0, 0, 0,
	0, 0, 0, 0,
};

int ioinstflags[] = {
	0, 0, 0, 0,
	NUMY, 0, NUMY, NUMY
};

char *devlist[] = {
	// 000
	"APR", "PI", NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	// 100
	"PTP", "PTR", "CP", "CR", "TTY", "LPT", "DIS", NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	// 200
	"DC", "UTC", "UTS", "MT", "MTS", "MTM", NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	// 300
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	// 400
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	// 500
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	// 600
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	// 700
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};


char *mnemonics[0700] = {
	"UUO00", "UUO01", "UUO02", "UUO03",
	"UUO04", "UUO05", "UUO06", "UUO07",
	"UUO10", "UUO11", "UUO12", "UUO13",
	"UUO14", "UUO15", "UUO16", "UUO17",
	"UUO20", "UUO21", "UUO22", "UUO23",
	"UUO24", "UUO25", "UUO26", "UUO27",
	"UUO30", "UUO31", "UUO32", "UUO33",
	"UUO34", "UUO35", "UUO36", "UUO37",
	"UUO40", "UUO41", "UUO42", "UUO43",
	"UUO44", "UUO45", "UUO46", "UUO47",
	"UUO50", "UUO51", "UUO52", "UUO53",
	"UUO54", "UUO55", "UUO56", "UUO57",
	"UUO60", "UUO61", "UUO62", "UUO63",
	"UUO64", "UUO65", "UUO66", "UUO67",
	"UUO70", "UUO71", "UUO72", "UUO73",
	"UUO74", "UUO75", "UUO76", "UUO77",

	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
//	"", "", "FSC", "CAO",
//	"LDCI", "LDC", "DPCI", "DPC",
	"", "", "FSC", "IBP",
	"ILDB", "LDB", "IDPB", "DPB",
	"FAD", "FADL", "FADM", "FADB",
	"FADR", "FADLR", "FADMR", "FADBR",
	"FSB", "FSBL", "FSBM", "FSBB",
	"FSBR", "FSBLR", "FSBMR", "FSBBR",
	"FMP", "FMPL", "FMPM", "FMPB",
	"FMPR", "FMPLR", "FMPMR", "FMPBR",
	"FDV", "FDVL", "FDVM", "FDVB",
	"FDVR", "FDVLR", "FDVMR", "FDVBR",

	"MOVE", "MOVEI", "MOVEM", "MOVES",
	"MOVS", "MOVSI", "MOVSM", "MOVSS",
	"MOVN", "MOVNI", "MOVNM", "MOVNS",
	"MOVM", "MOVMI", "MOVMM", "MOVMS",
	"IMUL", "IMULI", "IMULM", "IMULB",
	"MUL", "MULI", "MULM", "MULB",
	"IDIV", "IDIVI", "IDIVM", "IDIVB",
	"DIV", "DIVI", "DIVM", "DIVB",
	"ASH", "ROT", "LSH", "XX243",
	"ASHC", "ROTC", "LSHC", "XX247",
	"EXCH", "BLT", "AOBJP", "AOBJN",
	"JRST", "JFCL", "XCT", "XX257",
	"PUSHJ", "PUSH", "POP", "POPJ",
	"JSR", "JSP", "JSA", "JRA",
	"ADD", "ADDI", "ADDM", "ADDB",
	"SUB", "SUBI", "SUBM", "SUBB",

	"CAI", "CAIL", "CAIE", "CAILE",
	"CAIA", "CAIGE", "CAIN", "CAIG",
	"CAM", "CAML", "CAME", "CAMLE",
	"CAMA", "CAMGE", "CAMN", "CAMG",
	"JUMP", "JUMPL", "JUMPE", "JUMPLE",
	"JUMPA", "JUMPGE", "JUMPN", "JUMPG",
	"SKIP", "SKIPL", "SKIPE", "SKIPLE",
	"SKIPA", "SKIPGE", "SKIPN", "SKIPG",
	"AOJ", "AOJL", "AOJE", "AOJLE",
	"AOJA", "AOJGE", "AOJN", "AOJG",
	"AOS", "AOSL", "AOSE", "AOSLE",
	"AOSA", "AOSGE", "AOSN", "AOSG",
	"SOJ", "SOJL", "SOJE", "SOJLE",
	"SOJA", "SOJGE", "SOJN", "SOJG",
	"SOS", "SOSL", "SOSE", "SOSLE",
	"SOSA", "SOSGE", "SOSN", "SOSG",

	"SETZ", "SETZI", "SETZM", "SETZB",
	"AND", "ANDI", "ANDM", "ANDB",
	"ANDCA", "ANDCAI", "ANDCAM", "ANDCAB",
	"SETM", "SETMI", "SETMM", "SETMB",
	"ANDCM", "ANDCMI", "ANDCMM", "ANDCMB",
	"SETA", "SETAI", "SETAM", "SETAB",
	"XOR", "XORI", "XORM", "XORB",
	"IOR", "IORI", "IORM", "IORB",
	"ANDCB", "ANDCBI", "ANDCBM", "ANDCBB",
	"EQV", "EQVI", "EQVM", "EQVB",
	"SETCA", "SETCAI", "SETCAM", "SETCAB",
	"ORCA", "ORCAI", "ORCAM", "ORCAB",
	"SETCM", "SETCMI", "SETCMM", "SETCMB",
	"ORCM", "ORCMI", "ORCMM", "ORCMB",
	"ORCB", "ORCBI", "ORCBM", "ORCBB",
	"SETO", "SETOI", "SETOM", "SETOB",

	"HLL", "HLLI", "HLLM", "HLLS",
	"HRL", "HRLI", "HRLM", "HRLS",
	"HLLZ", "HLLZI", "HLLZM", "HLLZS",
	"HRLZ", "HRLZI", "HRLZM", "HRLZS",
	"HLLO", "HLLOI", "HLLOM", "HLLOS",
	"HRLO", "HRLOI", "HRLOM", "HRLOS",
	"HLLE", "HLLEI", "HLLEM", "HLLES",
	"HRLE", "HRLEI", "HRLEM", "HRLES",
	"HRR", "HRRI", "HRRM", "HRRS",
	"HLR", "HLRI", "HLRM", "HLRS",
	"HRRZ", "HRRZI", "HRRZM", "HRRZS",
	"HLRZ", "HLRZI", "HLRZM", "HLRZS",
	"HRRO", "HRROI", "HRROM", "HRROS",
	"HLRO", "HLROI", "HLROM", "HLROS",
	"HRRE", "HRREI", "HRREM", "HRRES",
	"HLRE", "HLREI", "HLREM", "HLRES",

	"TRN", "TLN", "TRNE", "TLNE",
	"TRNA", "TLNA", "TRNN", "TLNN",
	"TDN", "TSN", "TDNE", "TSNE",
	"TDNA", "TSNA", "TDNN", "TSNN",
	"TRZ", "TLZ", "TRZE", "TLZE",
	"TRZA", "TLZA", "TRZN", "TLZN",
	"TDZ", "TSZ", "TDZE", "TSZE",
	"TDZA", "TSZA", "TDZN", "TSZN",
	"TRC", "TLC", "TRCE", "TLCE",
	"TRCA", "TLCA", "TRCN", "TLCN",
	"TDC", "TSC", "TDCE", "TSCE",
	"TDCA", "TSCA", "TDCN", "TSCN",
	"TRO", "TLO", "TROE", "TLOE",
	"TROA", "TLOA", "TRON", "TLON",
	"TDO", "TSO", "TDOE", "TSOE",
	"TDOA", "TSOA", "TDON", "TSON",
};

char *iomnemonics[] = {
	"BLKI",
	"DATAI",
	"BLKO",
	"DATAO",
	"CONO",
	"CONI",
	"CONSZ",
	"CONSO"
};

/*
char*
disasm(word w)
{
	static char s[100];
	char *p;
	hword i, x, y;
	hword ac, dev;
	hword op;
	hword f;

	y = w & 0777777;
	if((w & 0777000000000) == 0){
		sprintf(s, "%llo", w);
		return s;
	}
	x = (w >> 18) & 017;
	i = (w >> 22) & 1;
	ac = (w >> 23) & 017;
	op = (w >> 27) & 0777;
	dev = (w >> 24) & 0774;
	f = ac & 07;
	p = s;
	if((op & 0700) == 0700){
		p += sprintf(p, "%s ", iomnemonics[f]);
		p += sprintf(p, "%o,", dev);
	}else{
		if(mnemonics[op][0] == '\0'){
			sprintf(s, "%llo", w);
			return s;
		}
		p += sprintf(p, "%s ", mnemonics[op]);
		p += sprintf(p, "%o,", ac);
	}
	if(i)
		p += sprintf(p, "@");
	p += sprintf(p, "%o", y);
	if(x)
		p += sprintf(p, "(%o)", x);
	return s;
}
*/

char *findacsym(word v);
char *findsymval(word v);

char*
disasm(word w)
{
	static char s[100];
	char *p;
	hword i, x, y;
	hword ac, dev;
	hword op;
	hword f;
	char *sym;
	int flags;

	if((w & 0777000000000) == 0){
		sprintf(s, "%llo", w);
		return s;
	}

	y = w & 0777777;
	x = (w >> 18) & 017;
	i = (w >> 22) & 1;
	ac = (w >> 23) & 017;
	op = (w >> 27) & 0777;
	dev = (w >> 24) & 0774;
	f = ac & 07;
	p = s;
	if((op & 0700) == 0700){
		flags = ioinstflags[f];
		p += sprintf(p, "%s ", iomnemonics[f]);
		if(devlist[dev>>2])
			p += sprintf(p, "%s,", devlist[dev>>2]);
		else
			p += sprintf(p, "%o,", dev);
	}else{
		if(mnemonics[op][0] == '\0'){
			sprintf(s, "%llo", w);
			return s;
		}
		flags = instflags[op];
		p += sprintf(p, "%s ", mnemonics[op]);
		if(ac || (flags & NOAC) == 0){
			sym = findacsym(ac);
			if(sym)
				p += sprintf(p, "%s,", sym);
			else
				p += sprintf(p, "%o,", ac);
		}
	}
	if(flags & NOINST){
		sprintf(s, "%llo", w);
		return s;
	}
	if(i)
		p += sprintf(p, "@");

	if(y || (flags & NOY) == 0){
		sym = findsymval(y);
		if(flags & NUMY && !i || sym == NULL)
			p += sprintf(p, "%o", y);
		else
			p += sprintf(p, "%s", sym);
	}

	if(x){
		sym = findacsym(x);
		if(sym)
			p += sprintf(p, "(%s)", sym);
		else
			p += sprintf(p, "(%o)", x);
	}
	return s;
}
