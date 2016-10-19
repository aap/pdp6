#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "pdp6common.h"

word fw(hword l, hword r) { return ((word)l << 18) | (word)r; }
word point(word pos, word sz, hword p) { return (pos<<30)|(sz<<24)|p; }
hword left(word w) { return (w >> 18) & 0777777; }
hword right(word w) { return w & 0777777; }
word negw(word w) { return (~w + 1) & 0777777777777; }
int isneg(word w) { return !!(w & 0400000000); }

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
		070, 071, 072,  -1,  -1,  -1,  -1,  -1,
		 -1, 041, 042, 043, 044, 045, 046, 047,
		050, 051, 052, 053, 054, 055, 056, 057,
		060, 061, 062, 063, 064, 065, 066, 067,
		070, 071, 072,  -1,  -1,  -1,  -1,  -1,
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

	"XX100", "XX101", "XX102", "XX103",
	"XX104", "XX105", "XX106", "XX107",
	"XX110", "XX111", "XX112", "XX113",
	"XX114", "XX115", "XX116", "XX117",
	"XX120", "XX121", "XX122", "XX123",
	"XX124", "XX125", "XX126", "XX127",
	"XX130", "XX131", "FSC", "CAO",
	"LDCI", "LDC", "DPCI", "DPC",
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
	x = (w >> 18) & 017;
	i = (w >> 22) & 1;
	ac = (w >> 23) & 017;
	op = (w >> 27) & 0777;
	dev = (w >> 24) & 0774;
	f = ac & 07;
	p = s;
	if((op & 0700) == 0700){
		p += sprintf(p, "%s	", iomnemonics[f]);
		p += sprintf(p, "%o,", dev);
	}else{
		p += sprintf(p, "%s	", mnemonics[op]);
		p += sprintf(p, "%o,", ac);
	}
	if(i)
		p += sprintf(p, "@");
	p += sprintf(p, "%o", y);
	if(x)
		p += sprintf(p, "(%o)", x);
	return s;
}
