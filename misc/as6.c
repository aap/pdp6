#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#define SYMTABSZ 8000
#define nil NULL

typedef int64_t word;
typedef uint64_t uword;
typedef int32_t hword;

#define WORD(l, r) (((l)&0777777) << 18 | ((r)&0777777))
#define LW(w) (((w) >> 18)&0777777)
#define RW(w) ((w)&0777777)

enum Valtype
{
	Undef = 0,
	Abs,
	Op,
	Io,
	Asm,
};

enum Toktype
{
	Unused = 0,
	Eof,
	Symbol,
	Word,
	Newline,
	Space,
	Not,
	DQuote,
	Mod,
	And,
	LParen,
	RParen,
	Times,
	Plus,
	Comma,
	Cons,
	Minus,
	Period,
	Div,
	Colon,
	Semi,
	Less,
	Equal,
	Great,
	At,
	LBrack,
	RBrack,
	LBrace,
	Or,
	RBrace,

	/* only used as character classes */
	Digit,
	Letter,
};

typedef struct Value Value;
struct Value
{
	int type;
	word value;
};

typedef struct Sym Sym;
struct Sym
{
	char name[24];
	Value v;
};

typedef struct Token Token;
struct Token
{
	int type;
	union {
		word w;
		Sym *s;
	};
};

typedef struct Section Section;
struct Section
{
	hword start;
	hword size;
};

int chartab[] = {
	Unused, Unused, Unused, Unused, Unused, Unused, Unused, Unused,
	Unused, Unused, Newline, Unused, Unused, Unused, Unused, Unused,
	Unused, Unused, Unused, Unused, Unused, Unused, Unused, Unused,
	Unused, Unused, Unused, Unused, Unused, Unused, Unused, Unused,
	Space,  Not,    DQuote, Unused, Unused, Mod,    And,    Unused,
	LParen, RParen, Times,  Plus,   Comma,  Minus,  Period, Div,
	Digit,  Digit,  Digit,  Digit,  Digit,  Digit,  Digit,  Digit,
	Digit,  Digit,  Colon,  Semi,   Less,   Equal,  Great,  Unused,
	At,     Letter, Letter, Letter, Letter, Letter, Letter, Letter,
	Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter,
	Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter,
	Letter, Letter, Letter, LBrack, Unused, RBrack, Unused, Letter,
	Unused, Letter, Letter, Letter, Letter, Letter, Letter, Letter,
	Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter,
	Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter,
	Letter, Letter, Letter, LBrace, Or,     RBrace, Unused, Unused
};
#define CTYPE(c) chartab[(c)&0177]

Sym *symtab[SYMTABSZ];
Sym syms[] = {
	{ ".", Abs, 0 },
	{ "ASCII", Asm, 0 },

	{ "UUO00", Op, 000},
	{ "UUO01", Op, 001},
	{ "UUO02", Op, 002},
	{ "UUO03", Op, 003},
	{ "UUO04", Op, 004},
	{ "UUO05", Op, 005},
	{ "UUO06", Op, 006},
	{ "UUO07", Op, 007},
	{ "UUO10", Op, 010},
	{ "UUO11", Op, 011},
	{ "UUO12", Op, 012},
	{ "UUO13", Op, 013},
	{ "UUO14", Op, 014},
	{ "UUO15", Op, 015},
	{ "UUO16", Op, 016},
	{ "UUO17", Op, 017},
	{ "UUO20", Op, 020},
	{ "UUO21", Op, 021},
	{ "UUO22", Op, 022},
	{ "UUO23", Op, 023},
	{ "UUO24", Op, 024},
	{ "UUO25", Op, 025},
	{ "UUO26", Op, 026},
	{ "UUO27", Op, 027},
	{ "UUO30", Op, 030},
	{ "UUO31", Op, 031},
	{ "UUO32", Op, 032},
	{ "UUO33", Op, 033},
	{ "UUO34", Op, 034},
	{ "UUO35", Op, 035},
	{ "UUO36", Op, 036},
	{ "UUO37", Op, 037},
	{ "UUO40", Op, 040},
	{ "UUO41", Op, 041},
	{ "UUO42", Op, 042},
	{ "UUO43", Op, 043},
	{ "UUO44", Op, 044},
	{ "UUO45", Op, 045},
	{ "UUO46", Op, 046},
	{ "UUO47", Op, 047},
	{ "UUO50", Op, 050},
	{ "UUO51", Op, 051},
	{ "UUO52", Op, 052},
	{ "UUO53", Op, 053},
	{ "UUO54", Op, 054},
	{ "UUO55", Op, 055},
	{ "UUO56", Op, 056},
	{ "UUO57", Op, 057},
	{ "UUO60", Op, 060},
	{ "UUO61", Op, 061},
	{ "UUO62", Op, 062},
	{ "UUO63", Op, 063},
	{ "UUO64", Op, 064},
	{ "UUO65", Op, 065},
	{ "UUO66", Op, 066},
	{ "UUO67", Op, 067},
	{ "UUO70", Op, 070},
	{ "UUO71", Op, 071},
	{ "UUO72", Op, 072},
	{ "UUO73", Op, 073},
	{ "UUO74", Op, 074},
	{ "UUO75", Op, 075},
	{ "UUO76", Op, 076},
	{ "UUO77", Op, 077},

	{ "FSC",   Op, 0132 },
	{ "IBP",   Op, 0133 },
	{ "ILDB",  Op, 0134 },
	{ "LDB",   Op, 0135 },
	{ "IDPB",  Op, 0136 },
	{ "DPB",   Op, 0137 },

	{ "FAD",   Op, 0140 },
	{ "FADL",  Op, 0141 },
	{ "FADM",  Op, 0142 },
	{ "FADB",  Op, 0143 },
	{ "FADR",  Op, 0144 },
	{ "FADRL", Op, 0145 },
	{ "FADRM", Op, 0146 },
	{ "FADRB", Op, 0147 },
	{ "FSB",   Op, 0150 },
	{ "FSBL",  Op, 0151 },
	{ "FSBM",  Op, 0152 },
	{ "FSBB",  Op, 0153 },
	{ "FSBR",  Op, 0154 },
	{ "FSBRL", Op, 0155 },
	{ "FSBRM", Op, 0156 },
	{ "FSBRB", Op, 0157 },
	{ "FMP",   Op, 0160 },
	{ "FMPL",  Op, 0161 },
	{ "FMPM",  Op, 0162 },
	{ "FMPB",  Op, 0163 },
	{ "FMPR",  Op, 0164 },
	{ "FMPRL", Op, 0165 },
	{ "FMPRM", Op, 0166 },
	{ "FMPRB", Op, 0167 },
	{ "FDV",   Op, 0170 },
	{ "FDVL",  Op, 0171 },
	{ "FDVM",  Op, 0172 },
	{ "FDVB",  Op, 0173 },
	{ "FDVR",  Op, 0174 },
	{ "FDVRL", Op, 0175 },
	{ "FDVRM", Op, 0176 },
	{ "FDVRB", Op, 0177 },

	{ "MOVE",  Op, 0200 },
	{ "MOVEI", Op, 0201 },
	{ "MOVEM", Op, 0202 },
	{ "MOVES", Op, 0203 },
	{ "MOVS",  Op, 0204 },
	{ "MOVSI", Op, 0205 },
	{ "MOVSM", Op, 0206 },
	{ "MOVSS", Op, 0207 },
	{ "MOVN",  Op, 0210 },
	{ "MOVNI", Op, 0211 },
	{ "MOVNM", Op, 0212 },
	{ "MOVNS", Op, 0213 },
	{ "MOVM",  Op, 0214 },
	{ "MOVMI", Op, 0215 },
	{ "MOVMM", Op, 0216 },
	{ "MOVMS", Op, 0217 },

	{ "IMUL",  Op, 0220 },
	{ "IMULI", Op, 0221 },
	{ "IMULM", Op, 0222 },
	{ "IMULB", Op, 0223 },
	{ "MUL",   Op, 0224 },
	{ "MULI",  Op, 0225 },
	{ "MULM",  Op, 0226 },
	{ "MULB",  Op, 0227 },
	{ "IDIV",  Op, 0230 },
	{ "IDIVI", Op, 0231 },
	{ "IDIVM", Op, 0232 },
	{ "IDIVB", Op, 0233 },
	{ "DIV",   Op, 0234 },
	{ "DIVI",  Op, 0235 },
	{ "DIVM",  Op, 0236 },
	{ "DIVB",  Op, 0237 },

	{ "ASH",   Op, 0240 },
	{ "ROT",   Op, 0241 },
	{ "LSH",   Op, 0242 },
	{ "ASHC",  Op, 0244 },
	{ "ROTC",  Op, 0245 },
	{ "LSHC",  Op, 0246 },

	{ "EXCH",  Op, 0250 },
	{ "BLT",   Op, 0251 },
	{ "AOBJP", Op, 0252 },
	{ "AOBJN", Op, 0253 },
	{ "JRST",  Op, 0254 },
	{ "JFCL",  Op, 0255 },
	{ "XCT",   Op, 0256 },

	{ "PUSHJ", Op, 0260 },
	{ "PUSH",  Op, 0261 },
	{ "POP",   Op, 0262 },
	{ "POPJ",  Op, 0263 },
	{ "JSR",   Op, 0264 },
	{ "JSP",   Op, 0265 },
	{ "JSA",   Op, 0266 },
	{ "JRA",   Op, 0267 },

	{ "ADD",   Op, 0270 },
	{ "ADDI",  Op, 0271 },
	{ "ADDM",  Op, 0272 },
	{ "ADDB",  Op, 0273 },
	{ "SUB",   Op, 0274 },
	{ "SUBI",  Op, 0275 },
	{ "SUBM",  Op, 0276 },
	{ "SUBB",  Op, 0277 },

	{ "CAI",   Op, 0300 },
	{ "CAIL",  Op, 0301 },
	{ "CAIE",  Op, 0302 },
	{ "CAILE", Op, 0303 },
	{ "CAIA",  Op, 0304 },
	{ "CAIGE", Op, 0305 },
	{ "CAIN",  Op, 0306 },
	{ "CAIG",  Op, 0307 },
	{ "CAM",   Op, 0310 },
	{ "CAML",  Op, 0311 },
	{ "CAME",  Op, 0312 },
	{ "CAMLE", Op, 0313 },
	{ "CAMA",  Op, 0314 },
	{ "CAMGE", Op, 0315 },
	{ "CAMN",  Op, 0316 },
	{ "CAMG",  Op, 0317 },
	{ "JUMP",  Op, 0320 },
	{ "JUMPL", Op, 0321 },
	{ "JUMPE", Op, 0322 },
	{ "JUMPLE",Op, 0323 },
	{ "JUMPA", Op, 0324 },
	{ "JUMPGE",Op, 0325 },
	{ "JUMPN", Op, 0326 },
	{ "JUMPG", Op, 0327 },
	{ "SKIP",  Op, 0330 },
	{ "SKIPL", Op, 0331 },
	{ "SKIPE", Op, 0332 },
	{ "SKIPLE",Op, 0333 },
	{ "SKIPA", Op, 0334 },
	{ "SKIPGE",Op, 0335 },
	{ "SKIPN", Op, 0336 },
	{ "SKIPG", Op, 0337 },
	{ "AOJ",   Op, 0340 },
	{ "AOJL",  Op, 0341 },
	{ "AOJE",  Op, 0342 },
	{ "AOJLE", Op, 0343 },
	{ "AOJA",  Op, 0344 },
	{ "AOJGE", Op, 0345 },
	{ "AOJN",  Op, 0346 },
	{ "AOJG",  Op, 0347 },
	{ "AOS",   Op, 0350 },
	{ "AOSL",  Op, 0351 },
	{ "AOSE",  Op, 0352 },
	{ "AOSLE", Op, 0353 },
	{ "AOSA",  Op, 0354 },
	{ "AOSGE", Op, 0355 },
	{ "AOSN",  Op, 0356 },
	{ "AOSG",  Op, 0357 },
	{ "SOJ",   Op, 0360 },
	{ "SOJL",  Op, 0361 },
	{ "SOJE",  Op, 0362 },
	{ "SOJLE", Op, 0363 },
	{ "SOJA",  Op, 0364 },
	{ "SOJGE", Op, 0365 },
	{ "SOJN",  Op, 0366 },
	{ "SOJG",  Op, 0367 },
	{ "SOS",   Op, 0370 },
	{ "SOSL",  Op, 0371 },
	{ "SOSE",  Op, 0372 },
	{ "SOSLE", Op, 0373 },
	{ "SOSA",  Op, 0374 },
	{ "SOSGE", Op, 0375 },
	{ "SOSN",  Op, 0376 },
	{ "SOSG",  Op, 0377 },

	{ "SETZ",  Op, 0400 },
	{ "SETZI", Op, 0401 },
	{ "SETZM", Op, 0402 },
	{ "SETZB", Op, 0403 },
	{ "AND",   Op, 0404 },
	{ "ANDI",  Op, 0405 },
	{ "ANDM",  Op, 0406 },
	{ "ANDB",  Op, 0407 },
	{ "ANDCA", Op, 0410 },
	{ "ANDCAI",Op, 0411 },
	{ "ANDCAM",Op, 0412 },
	{ "ANDCAB",Op, 0413 },
	{ "SETM",  Op, 0414 },
	{ "SETMI", Op, 0415 },
	{ "SETMM", Op, 0416 },
	{ "SETMB", Op, 0417 },
	{ "ANDCM", Op, 0420 },
	{ "ANDCMI",Op, 0421 },
	{ "ANDCMM",Op, 0422 },
	{ "ANDCMB",Op, 0423 },
	{ "SETA",  Op, 0424 },
	{ "SETAI", Op, 0425 },
	{ "SETAM", Op, 0426 },
	{ "SETAB", Op, 0427 },
	{ "XOR",   Op, 0430 },
	{ "XORI",  Op, 0431 },
	{ "XORM",  Op, 0432 },
	{ "XORB",  Op, 0433 },
	{ "IOR",   Op, 0434 },
	{ "IORI",  Op, 0435 },
	{ "IORM",  Op, 0436 },
	{ "IORB",  Op, 0437 },
	{ "ANDCB", Op, 0440 },
	{ "ANDCBI",Op, 0441 },
	{ "ANDCBM",Op, 0442 },
	{ "ANDCBB",Op, 0443 },
	{ "EQV",   Op, 0444 },
	{ "EQVI",  Op, 0445 },
	{ "EQVM",  Op, 0446 },
	{ "EQVB",  Op, 0447 },
	{ "SETCA", Op, 0450 },
	{ "SETCAI",Op, 0451 },
	{ "SETCAM",Op, 0452 },
	{ "SETCAB",Op, 0453 },
	{ "ORCA",  Op, 0454 },
	{ "ORCAI", Op, 0455 },
	{ "ORCAM", Op, 0456 },
	{ "ORCAB", Op, 0457 },
	{ "SETCM", Op, 0460 },
	{ "SETCMI",Op, 0461 },
	{ "SETCMM",Op, 0462 },
	{ "SETCMB",Op, 0463 },
	{ "ORCM",  Op, 0464 },
	{ "ORCMI", Op, 0465 },
	{ "ORCMM", Op, 0466 },
	{ "ORCMB", Op, 0467 },
	{ "ORCB",  Op, 0470 },
	{ "ORCBI", Op, 0471 },
	{ "ORCBM", Op, 0472 },
	{ "ORCBB", Op, 0473 },
	{ "SETO",  Op, 0474 },
	{ "SETOI", Op, 0475 },
	{ "SETOM", Op, 0476 },
	{ "SETOB", Op, 0477 },

	{ "HLL",   Op, 0500 },
	{ "HLLI",  Op, 0501 },
	{ "HLLM",  Op, 0502 },
	{ "HLLS",  Op, 0503 },
	{ "HRL",   Op, 0504 },
	{ "HRLI",  Op, 0505 },
	{ "HRLM",  Op, 0506 },
	{ "HRLS",  Op, 0507 },
	{ "HLLZ",  Op, 0510 },
	{ "HLLZI", Op, 0511 },
	{ "HLLZM", Op, 0512 },
	{ "HLLZS", Op, 0513 },
	{ "HRLZ",  Op, 0514 },
	{ "HRLZI", Op, 0515 },
	{ "HRLZM", Op, 0516 },
	{ "HRLZS", Op, 0517 },
	{ "HLLO",  Op, 0520 },
	{ "HLLOI", Op, 0521 },
	{ "HLLOM", Op, 0522 },
	{ "HLLOS", Op, 0523 },
	{ "HRLO",  Op, 0524 },
	{ "HRLOI", Op, 0525 },
	{ "HRLOM", Op, 0526 },
	{ "HRLOS", Op, 0527 },
	{ "HLLE",  Op, 0530 },
	{ "HLLEI", Op, 0531 },
	{ "HLLEM", Op, 0532 },
	{ "HLLES", Op, 0533 },
	{ "HRLE",  Op, 0534 },
	{ "HRLEI", Op, 0535 },
	{ "HRLEM", Op, 0536 },
	{ "HRLES", Op, 0537 },
	{ "HRR",   Op, 0540 },
	{ "HRRI",  Op, 0541 },
	{ "HRRM",  Op, 0542 },
	{ "HRRS",  Op, 0543 },
	{ "HLR",   Op, 0544 },
	{ "HLRI",  Op, 0545 },
	{ "HLRM",  Op, 0546 },
	{ "HLRS",  Op, 0547 },
	{ "HRRZ",  Op, 0550 },
	{ "HRRZI", Op, 0551 },
	{ "HRRZM", Op, 0552 },
	{ "HRRZS", Op, 0553 },
	{ "HLRZ",  Op, 0554 },
	{ "HLRZI", Op, 0555 },
	{ "HLRZM", Op, 0556 },
	{ "HLRZS", Op, 0557 },
	{ "HRRO",  Op, 0560 },
	{ "HRROI", Op, 0561 },
	{ "HRROM", Op, 0562 },
	{ "HRROS", Op, 0563 },
	{ "HLRO",  Op, 0564 },
	{ "HLROI", Op, 0565 },
	{ "HLROM", Op, 0566 },
	{ "HLROS", Op, 0567 },
	{ "HRRE",  Op, 0570 },
	{ "HRREI", Op, 0571 },
	{ "HRREM", Op, 0572 },
	{ "HRRES", Op, 0573 },
	{ "HLRE",  Op, 0574 },
	{ "HLREI", Op, 0575 },
	{ "HLREM", Op, 0576 },
	{ "HLRES", Op, 0577 },

	{ "TRN",   Op, 0600 },
	{ "TLN",   Op, 0601 },
	{ "TRNE",  Op, 0602 },
	{ "TLNE",  Op, 0603 },
	{ "TRNA",  Op, 0604 },
	{ "TLNA",  Op, 0605 },
	{ "TRNN",  Op, 0606 },
	{ "TLNN",  Op, 0607 },
	{ "TDN",   Op, 0610 },
	{ "TSN",   Op, 0611 },
	{ "TDNE",  Op, 0612 },
	{ "TSNE",  Op, 0613 },
	{ "TDNA",  Op, 0614 },
	{ "TSNA",  Op, 0615 },
	{ "TDNN",  Op, 0616 },
	{ "TSNN",  Op, 0617 },
	{ "TRZ",   Op, 0620 },
	{ "TLZ",   Op, 0621 },
	{ "TRZE",  Op, 0622 },
	{ "TLZE",  Op, 0623 },
	{ "TRZA",  Op, 0624 },
	{ "TLZA",  Op, 0625 },
	{ "TRZN",  Op, 0626 },
	{ "TLZN",  Op, 0627 },
	{ "TDZ",   Op, 0630 },
	{ "TSZ",   Op, 0631 },
	{ "TDZE",  Op, 0632 },
	{ "TSZE",  Op, 0633 },
	{ "TDZA",  Op, 0634 },
	{ "TSZA",  Op, 0635 },
	{ "TDZN",  Op, 0636 },
	{ "TSZN",  Op, 0637 },
	{ "TRC",   Op, 0640 },
	{ "TLC",   Op, 0641 },
	{ "TRCE",  Op, 0642 },
	{ "TLCE",  Op, 0643 },
	{ "TRCA",  Op, 0644 },
	{ "TLCA",  Op, 0645 },
	{ "TRCN",  Op, 0646 },
	{ "TLCN",  Op, 0647 },
	{ "TDC",   Op, 0650 },
	{ "TSC",   Op, 0651 },
	{ "TDCE",  Op, 0652 },
	{ "TSCE",  Op, 0653 },
	{ "TDCA",  Op, 0654 },
	{ "TSCA",  Op, 0655 },
	{ "TDCN",  Op, 0656 },
	{ "TSCN",  Op, 0657 },
	{ "TRO",   Op, 0660 },
	{ "TLO",   Op, 0661 },
	{ "TROE",  Op, 0662 },
	{ "TLOE",  Op, 0663 },
	{ "TROA",  Op, 0664 },
	{ "TLOA",  Op, 0665 },
	{ "TRON",  Op, 0666 },
	{ "TLON",  Op, 0667 },
	{ "TDO",   Op, 0670 },
	{ "TSO",   Op, 0671 },
	{ "TDOE",  Op, 0672 },
	{ "TSOE",  Op, 0673 },
	{ "TDOA",  Op, 0674 },
	{ "TSOA",  Op, 0675 },
	{ "TDON",  Op, 0676 },
	{ "TSON",  Op, 0677 },

	{ "BLKI",  Io, 070000 },
	{ "BLKO",  Io, 070010 },
	{ "DATAI", Io, 070004 },
	{ "DATAO", Io, 070014 },
	{ "CONO",  Io, 070020 },
	{ "CONI",  Io, 070024 },
	{ "CONSZ", Io, 070030 },
	{ "CONSO", Io, 070034 },
	{ "", 0, 0 }
};

hword assemble(void);

int pass2;
int error;
word origin;
word mem[01000000];

#define MAXSECT 100	 /* increase if necessary */
Section sects[MAXSECT];
int nsect;
hword nextdot;

FILE *tokf;
FILE *wf;
FILE *outf;

int nextc;
int toksp;
Token tokstk[2];
Sym *dot;

#define PUSHT(t) tokstk[toksp++] = (t)

void
putw10(word w, FILE *f)
{
	int i;
	w &= 0777777777777;
	for(i = 5; i >= 0; i--)
		putc(((w>>i*6)&077)|0200, f);
}

void
writeblock(word origin, word count)
{
	int i;
	word chksm;
	word w;

	chksm = 0;
	putw10(w = WORD(-count, origin-1), outf);
	chksm += w;
	for(i = 0; i < count; i++){
		chksm += mem[i+origin];
		putw10(mem[i+origin], outf);
	}
	putw10(-chksm, outf);
}

int
ch(void)
{
	int c;
	if(nextc){
		c = nextc;
		nextc = 0;
		return c;
	}
	c = getchar();
tail:
	if(c == '#'){
		while(c = getchar(), c != '\n')
			if(c == EOF) return c;
		goto tail;
	}
	if(c == '\t' || c == '\r')
		c = ' ';
	return c;
}

int
chsp(void)
{
	int c;
	while(c = ch(), c == ' ');
	return c;
}

int
hash(char *s)
{
	int hsh;
	hsh = 0;
	for(; *s != '\0'; s++)
		hsh += *s;
	return hsh % SYMTABSZ;
}

Sym**
findsym(char *name)
{
	int hsh;
	char *p;
	Sym **s;

	hsh = hash(name);
	s = &symtab[hsh];
	for(;;){
		if(*s == nil ||
		   strncmp((*s)->name, name, 24) == 0)
			return s;
		s++;
		if(s == &symtab[SYMTABSZ])
			s = symtab;
		if(s == &symtab[hsh]){
			fprintf(stderr, "symbol table full\n");
			exit(1);
		}
	}
}

Sym*
getsym(char *name)
{
	Sym **p;
	p = findsym(name);
	if(*p)
		return *p;
	*p = malloc(sizeof(Sym));
	strncpy((*p)->name, name, 24);
	(*p)->v.type = Undef;
	(*p)->v.value = 0;
	return *p;
}

word
dtopdp(double d)
{
	uint64_t x, s, e, m;
	int sign;
	word f;
	sign = 0;
	if(d < 0.0){
		sign = 1;
		d *= -1.0;
	}
	x = *(uint64_t*)&d;
	s = (x >> 63) & 1;
	e = (x >> 52) & 0x7FF;
	m = x & 0xFFFFFFFFFFFFF;
	e += 128-1023;
	m >>= 25;
	/* normalize */
	if(x != 0){
		m >>= 1;
		m |= 0400000000;
		e += 1;
	}
	f = s << 35;
	f |= e << 27;
	f |= m;
	if(sign)
		f = -f & 0777777777777;
	return f;
}

word
number(void)
{
	int c;
	word n, oct, dec;
	double f, e;
	int flt;
	int sign;

	sign = 1;
	if(c = ch(), c == '-')
		sign = -1;
	else
		nextc = c;
	oct = dec = 0;
	f = 0.0;
	while(c = ch(), CTYPE(c) == Digit){
		oct = (oct << 3) | (c-'0');
		dec = dec*10 + (c-'0');
		f = f*10 + (c-'0');
	}
	if(c == '.'){
		n = dec;
		e = 1.0/10.0;
		flt = 0;
		while(c = ch(), CTYPE(c) == Digit){
			f += (c-'0')*e;
			e /= 10.0;
			flt = 1;
		}
		nextc = c;
		if(flt)
			return sign*dtopdp(f);
	}else{
		n = oct;
		nextc = c;
	}
	return (n*sign)&0777777777777;
}

int
strchar(int delim)
{
	int c;
	c = getchar();
	if(delim && c == delim)
		return 0400;
	if(c != '\\')
		return c;
	c = getchar();
	switch(c){
	case 'b':
		c = '\b';
		break;

	case 'n':
		c = '\n';
		break;

	case 'r':
		c = '\r';
		break;

	case '\\':
		c = '\\';
		break;

	case '0':
		c = 0;
		break;
	}
	return c;
}

Token
token(void)
{
	char name[24];
	char *p;
	int c;
	Token t;

	if(toksp){
		t = tokstk[--toksp];
		return t;
	}
	if(pass2){
		fread(&t, sizeof(t), 1, tokf);
		return t;
	}
	c = chsp();
	if(c == EOF)
		t.type = Eof;
	else
		switch(CTYPE(c)){
		case Period:
		case Letter:
			p = name;
			do{
				if(p < name+23)
					*p++ = toupper(c);
			}while(c = ch(), CTYPE(c) == Letter ||
			                 CTYPE(c) == Period ||
			                 CTYPE(c) == Digit);
			nextc = c;
			*p++ = '\0';
			t.type = Symbol;
			t.s = getsym(name);
			break;

		case Digit:
			nextc = c;
			t.type = Word;
			t.w = number();
			break;

		case DQuote:
			t.type = Word;
			t.w = strchar(0);
			break;

		case Comma:
			c = chsp();
			if(CTYPE(c) == Comma)
				t.type = Cons;
			else{
				t.type = Comma;
				nextc = c;
			}
			break;

		case Unused:
			fprintf(stderr, "unknown char: %c\n", c);
			break;

		default:
			t.type = CTYPE(c);
			break;
		}
	fwrite(&t, sizeof(t), 1, tokf);
	return t;
}

void
directive(int type)
{
	uword w;
	int c, i, s;

	if(type == 0){	/* ascii */
		/* save ascii words in word file because they're not tokens */
		if(!pass2){
			c = chsp();
			if(c != '"'){
				fprintf(stderr, "\" expected\n");
				return;
			}
			w = 0;
			i = 0;
			s = 29;
			while(c = strchar('"'), c != 0400 && c != EOF){
				c &= 0177;
				w = w | ((uword)c << s);
				s -= 7;
				if(++i == 5){
					fwrite(&w, sizeof(w), 1, wf);
					dot->v.value++;
					i = 0;
					s = 29;
					w = 0;
				}
			}
			if(i){
				fwrite(&w, sizeof(w), 1, wf);
				dot->v.value++;
			}
			w = ~0;
			fwrite(&w, sizeof(w), 1, wf);
			dot->v.value++;
		}else{
			while(fread(&w, sizeof(w), 1, wf), w != ~0)
				mem[dot->v.value++] = w;
		}
	}
}

Value
apply(int op, Value v1, Value v2)
{
	switch(op){
	case Not:
		v1.value |= ~v2.value;
		break;

	case Plus:
		v1.value += v2.value;
		break;

	case Minus:
		v1.value -= v2.value;
		break;

	case Times:
		v1.value *= v2.value;
		break;

	case Div:
		v1.value /= v2.value;
		break;

	case Mod:
		v1.value %= v2.value;
		break;

	case And:
		v1.value &= v2.value;
		break;

	case Or:
		v1.value |= v2.value;
		break;

	case Less:
		v1.value <<= v2.value;
		break;

	case Great:
		v1.value >>= v2.value;
		break;

	case Cons:
		v1.value = WORD(v1.value, v2.value);
		break;

	default:
		fprintf(stderr, "unknown op %d\n", op);
		return v2;
	}
	v1.value &= 0777777777777;
	return v1;
}

Value
expr(void)
{
	Token t;
	int op;
	Value v, v2;
	hword savedot;

	v.type = Abs;
	v.value = 0;
	op = Plus;

	/* TODO: just accept possible tokens */
	while(t = token(),
	      t.type != RBrack && t.type != RBrace && t.type != Comma &&
	      t.type != LParen && t.type != RParen &&
	      t.type != Newline && t.type != Semi){
		switch(t.type){
		case Word:
			v2.type = Abs;
			v2.value = t.w;
			v = apply(op, v, v2);
			op = Plus;
			break;

		case Symbol:
			v = apply(op, v, t.s->v);
			op = Plus;
			break;

		case LBrack:
			v2 = expr();
			t = token();
			if(t.type != RBrack){
				fprintf(stderr, "']' expected\n");
				error = 1;
				PUSHT(t);
			}
			v = apply(op, v, v2);
			op = Plus;
			break;

		case LBrace:
			v2.type = Abs;
			savedot = dot->v.value;
			v2.value = assemble();
			dot->v.value = savedot;
			t = token();
			if(t.type != RBrace){
				fprintf(stderr, "'}' expected\n");
				error = 1;
				PUSHT(t);
			}
			v = apply(op, v, v2);
			op = Plus;
			break;

		case Not:
		case Plus:
		case Minus:
		case Times:
		case Div:
		case Mod:
		case And:
		case Or:
		case Less:
		case Great:
		case Cons:
			op = t.type;
			break;

		default:
			fprintf(stderr, "can't parse expr %d\n", t.type);
			error = 1;
			v.type = Abs;
			v.value = 0;
		}
	}
	PUSHT(t);
	return v;
}

word
opline(void)
{
	word inst;
	word op, ac, i, x, y;
	word w;
	Token t;
	Value v;
	int type;

	type = 0;
	ac = i = x = y = 0;
	t = token();
	if(t.s->v.type == Op){
		op = t.s->v.value & 0777;
	}else if(t.s->v.type == Io){
		type = 1;
		op = t.s->v.value & 077774;
	}else{
		fprintf(stderr, "expected opcode\n");
		error = 1;
	}

	/* So many gotos, sorry Edsger */
	t = token();
	if(t.type == At){
		i = 1;
		t = token();
		goto Y;
	}
	if(t.type == LParen)
		goto X;
	if(t.type == Newline || t.type == Semi){
		PUSHT(t);
		goto end;
	}
	PUSHT(t);	/* has to be AC or Y now */
	w = expr().value;
	t = token();
	if(t.type == Comma)
		ac = w & (type == 0 ? 017 : 0774);
	else{
		y = w & 0777777;
		if(t.type == LParen)
			goto X;
		PUSHT(t);
		goto end;
	}
	t = token();
	if(t.type == Newline || t.type == Semi){
		PUSHT(t);
		goto end;
	}
	if(t.type == At){
		i = 1;
		t = token();
	}
Y:
	if(t.type != LParen){
		PUSHT(t);
		y = expr().value & 0777777;
		t = token();
		if(t.type == LParen)
			goto X;
		PUSHT(t);
	}else{
X:
		x = expr().value & 017;
		t = token();
		if(t.type != RParen){
			fprintf(stderr, ") expected\n");
			error = 1;
			PUSHT(t);
		}
	}

end:
	if(type == 0)
		inst = op << 27 |
		       ac << 23 |
			i << 22 |
			x << 18 |
			y;
	else{
		inst = op << 21 |
		       ac << 24 |	/* this is correct */
			i << 22 |
			x << 18 |
			y;
	}
//	printf("inst: %lo\n", inst);
	return inst;
}

hword
assemble(void)
{
	Token t, tt;
	Section *sect;
	sect = &sects[nsect++];
	if(sect == &sects[MAXSECT]){
		fprintf(stderr, "ran out of sections\n");
		exit(1);
	}
	dot->v.value = sect->start;
//	printf("section %d %o %o\n", nsect, sect->start, sect->size);

	while(t = token(), t.type != Eof && t.type != RBrace){
		switch(t.type){
		case Symbol:
			if(t.s->v.type == Asm){
				directive(t.s->v.value);
				t = token();
				break;
			}
			if(t.s->v.type == Op ||
			   t.s->v.type == Io){
				PUSHT(t);
				mem[dot->v.value] = opline();
				dot->v.value++;
				t = token();
				break;
			}
			tt = token();
			if(tt.type == Colon){
//				if(t.s->v.type != Undef &&
//			           t.s->v.value != dot->v.value)
//					fprintf(stderr, "redefining %s\n",
//				                t.s->name);
				t.s->v = dot->v;
				continue;
			}else if(tt.type == Equal){
				t.s->v = expr();
				t = token();
			}else{
				PUSHT(tt);
				goto exp;
			}
			break;

		exp:
		case Not:
		case LBrace:
		case LBrack:
		case Minus:
		case Word:
			PUSHT(t);
			mem[dot->v.value++] = expr().value;
			t = token();
			break;

		default:
			if(t.type != Newline && t.type != Semi){
				fprintf(stderr, "can't parse %d\n", t.type);
				error = 1;
			}
		}

//		if(!pass2)
//			printf("statement %d\n", t.type);
		if(t.type == RBrace)
			break;
		if(t.type != Newline && t.type != Semi){
			PUSHT(t);
			fprintf(stderr, "statement not ended %d\n", t.type);
			error = 1;
			while(t.type != Newline && t.type != Semi)
				t = token();
		}
	}
	PUSHT(t);

	if(!pass2){
		sect->size = dot->v.value - sect->start;
		sect->start = nextdot;
		nextdot += sect->size;
//		printf("size: %o %o\n", sect->size, sect->start);
	}
	return sect->start;
}

void
dumpsymbols(void)
{
	int i;
	for(i = 0; i < SYMTABSZ; i++)
		if(symtab[i] && symtab[i]->v.type != Op &&
		   symtab[i]->v.type != Io)
			fprintf(stderr, "%s %o %lo\n", symtab[i]->name,
			        symtab[i]->v.type, symtab[i]->v.value);
}

void
dumpmem(word start, word end)
{
	for(; start < end; start++)
		if(mem[start])
			printf("%lo: %012lo\n", start, mem[start]);
}

void
initsyms(void)
{
	int hsh;
	Sym *s, **sp;

	for(s = syms; s->name[0]; s++){
		sp = findsym(s->name);
		*sp = s;
	}
	dot = getsym(".");
}

int
main(int argc, char *argv[])
{
	Section *s;
	Sym **start;
	hword blkstart, blksize;

	origin = 0;
	initsyms();

	/* first pass - figure out sections mostly */
	tokf = fopen("a.tok", "wb");
	if(tokf == NULL)
		return 1;
	wf = fopen("a.w", "wb");
	if(wf == NULL)
		return 1;
	nextdot = origin;
	assemble();
	fclose(tokf);
	fclose(wf);

	/* second pass - figure out addresses */
	if(error){
		fprintf(stderr, "errors\n");
		return 1;
	}
	pass2++;
	tokf = fopen("a.tok", "rb");
	if(tokf == NULL)
		return 1;
	wf = fopen("a.w", "rb");
	if(wf == NULL)
		return 1;
	toksp = 0;
	nsect = 0;
	assemble();

	/* third pass */
	fseek(tokf, 0, 0);
	fseek(wf, 0, 0);
	toksp = 0;
	nsect = 0;
	assemble();
	fclose(tokf);
	fclose(wf);

	outf = fopen("a.rim", "wb");
	if(outf == NULL)
		return 1;

/*
	blkstart = sects[nsect-1].start;
	blksize = 0;
	for(s = &sects[nsect-1]; s >= sects; s--){
		printf("sect %ld: %o %o\n", s-sects, s->start, s->size);
		blksize += s->size;
	}
	writeblock(blkstart, blksize);
	dumpmem(blkstart, blkstart+blksize);
*/
	for(s = &sects[nsect-1]; s >= sects; s--){
		writeblock(s->start, s->size);
		dumpmem(s->start, s->start+s->size);
	}
	if(start = findsym("START"), *start)
		putw10(WORD(0254000L, (*start)->v.value), outf);
	else
		putw10(WORD(0254000L, origin), outf);
	fclose(outf);

	dumpsymbols();
}
