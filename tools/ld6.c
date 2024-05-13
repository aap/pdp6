#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <assert.h>
#include "pdp6common.h"
#include "pdp6bin.h"
#include "args.h"

#define nil NULL

enum
{
	MAXSYM = 1000
};

typedef struct Add Add;
struct Add
{
	word name;
	int l;
	hword addr;
	Add *next;
};

enum
{
	RIM = 0,
	RIM10,
	SAV,
	SBLK
};

char *argv0;
FILE *in;
hword itemsz;
word mem[01000000];
hword rel, loc;
hword locmax, locmin;
hword start;
Add *addlist;
int error;
char **files;
int nfiles;
int fmt = RIM;

FILE*
mustopen(const char *name, const char *mode)
{
	FILE *f;
	if(f = fopen(name, mode), f == NULL){
		fprintf(stderr, "couldn't open file: %s\n", name);
		exit(1);
	}
	return f;
}

void
err(int n, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "error: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	error |= n;
}

/*
 * debugging helpers
 */

void
printinst(hword p)
{
	printf("%06o: %06o %06o %s\n", p, left(mem[p]), right(mem[p]), disasm(mem[p]));
}

void
disasmrange(hword start, hword end)
{
	for(; start < end; start++)
		printinst(start);
}

/* emulate PDP-6 IBP instruction */
word
ibp(word bp)
{
	word pos, sz, p;
	pos = (bp>>30)&077;
	sz = (bp>>24)&077;
	p = right(bp);
	if(pos >= sz)
		return point(pos-sz, sz, p);
	return point(36-sz, sz, p+1);
}

/* emulate PDP-6 LDB instruction */
word
ldb(word bp)
{
	word pos, sz, p;
	pos = (bp>>30)&077;
	sz = (bp>>24)&077;
	p = right(bp);
	return (mem[p]>>pos) & (1<<sz)-1;
}

/* print an ASCII string in memory */
void
printascii(hword loc)
{
	word bp;
	char c;
	bp = point(36, 7, loc);
	for(;;){
		c = ldb(bp = ibp(bp));
		if(c == 0)
			break;
		putchar(c);
	}
	putchar('\n');
}


word symtab[MAXSYM*2];
word symp;

/* Get symbol by name */
word*
findsym(word name)
{
	word p;
	word mask;
	mask = 0037777777777;
	for(p = 0; p < symp; p += 2)
		if((symtab[p]&mask) == (name&mask))
			return &symtab[p];
	return NULL;
}

/* Extend a global request chain by writing hw at the end of
 * the chain beginning at p */
void
extendreq(hword p, hword hw)
{
	while(right(mem[p]))
		p = right(mem[p]);
	mem[p] = fw(left(mem[p]), hw);
}

/* Resolve a global request chain by writing hw to all locations */
void
resolvereq(hword p, hword hw)
{
	hword next;
	do{
		next = right(mem[p]);
		mem[p] = fw(left(mem[p]), hw);
		p = next;
	}while(next);
}

/* Perform additive external fixup */
void
fixadd(void)
{
	Add *a;
	word *s;
	char name[8];

	for(a = addlist; a; a = a->next){
		s = findsym(a->name);
		if(s == nil){
			unrad50(a->name, name);
			fprintf(stderr, "Need symbol %s\n", name);
			continue;
		}
		if(a->l)
			mem[a->addr] = fw(
				left(mem[a->addr]) + right(s[1]),
				right(mem[a->addr]));
		else
			mem[a->addr] = fw(
				left(mem[a->addr]),
				right(mem[a->addr]) + right(s[1]));
	}
}

/* Insert a symbol into the symbol table. Handle linking. */
void
loadsym(word *newsym)
{
	word *sym;
	int type, newtype;
	char name[8];

	newtype = (newsym[0]>>30)&074;
	if(newtype == SymUndef && left(newsym[1]) & 0400000){
		/* Additive fixup */
		Add *a;
		a = malloc(sizeof(Add));
		a->name = newsym[0];
		a->l = !!(left(newsym[1]) & 0200000);
		a->addr = right(newsym[1]);
		a->next = addlist;
		addlist = a;
		return;
	}

	sym = findsym(newsym[0]);
	if(sym == NULL){
		/* Symbol not in table yet.
		 * TODO: ignore locals? */
//		if(newtype == SymGlobal || newtype == SymGlobalH ||
//		   newtype == SymUndef){
			assert(symp < MAXSYM);
			symtab[symp++] = newsym[0];
			symtab[symp++] = newsym[1];
//		}
		return;
	}
	type = unrad50(sym[0], name);

	if(type == SymUndef){
		/* global request is in table */
		if(newtype == SymGlobal || newtype == SymGlobalH){
			/* found definition, resolve request chain
			 * and insert definition into table */
			resolvereq(right(sym[1]), right(newsym[1]));
			sym[0] = newsym[0];
			sym[1] = newsym[1];
		}else if(newtype == SymUndef)
			/* extend request chain */
			extendreq(right(sym[1]), right(newsym[1]));
	}else if(type == SymGlobal || type == SymGlobalH){
		/* global definition is in table */
		if(newtype == SymGlobal || newtype == SymGlobalH){
			if(newtype == type &&
			   newsym[1] == sym[1])
				return;
			err(1, "multiple definitions: %s", name);
		}else if(newtype == SymUndef)
			/* just resolve request */
			resolvereq(right(newsym[1]), right(sym[1]));
	}
}

/* print symbol table, not needed */
void
dumpsym(void)
{
	int type;
	char name[8];
	word p;
	for(p = 0; p < symp; p += 2){
		type = unrad50(symtab[p], name);
		printf(" %02o %s %012lo\n", type, name, symtab[p+1]);
	}
}

/* Make sure there are no undefined symbols left */
void
checkundef(void)
{
	int type;
	char name[8];
	word p;
	for(p = 0; p < symp; p += 2){
		type = unrad50(symtab[p], name);
		if(type != SymUndef)
			continue;
		err(1, "undefined: %s %012lo", name, symtab[p+1]);
	}
}

word block[18];
hword blocksz;

void
readblock(int doreloc)
{
	word reloc;
	hword i;
	int bits;

	blocksz = 0;
	if(itemsz == 0)
		return;

	reloc = readw(in);
	i = 18;
	while(itemsz && i--){
		block[blocksz++] = readw(in);
		itemsz--;
	}

	/* relocate block */
	for(i = 0; i < 18; i++){
		bits = (reloc >> 34) & 03;
		reloc <<= 2;
		if(doreloc){
			if(bits & 1)
				block[i] = fw(
					left(block[i]),
					right(block[i]) + rel);
			if(bits & 2)
				block[i] = fw(
					left(block[i]) + rel,
					right(block[i]));
		}
	}
}

void
skipitem(void)
{
	while(itemsz)
		readblock(0);
}

void
handlecode(void)
{
	int i;

	readblock(1);
	loc = right(block[0]);
	i = 1;
	if(i < blocksz)	// hacky
		goto loop;
	while(itemsz){
		readblock(1);
		for(i = 0; i < blocksz; i++){
	loop:
			mem[loc] = block[i];
			if(loc < locmin) locmin = loc;
			if(loc > locmax) locmax = loc;
			loc++;
		}
	}
}

void
handlesym(void)
{
	int i;

	while(itemsz){
		readblock(1);
		i = 0;
		while(i < blocksz){
			loadsym(&block[i]);
			i += 2;
		}
	}
}

void
handleend(void)
{
	word abs;
	char name[8];

	readblock(1);
	rel = block[0];
	/* not sure what to do with this... */
	abs = block[1];
	while(itemsz)
		readblock(0);
}

void
handlename(void)
{
	word w;
	int id;
	char name[8];

	readblock(0);
	w = block[0];
	while(itemsz)
		readblock(0);

	id = unrad50(w, name);
}

void
handlestart(void)
{
	readblock(1);
	start = right(block[0]);
	while(itemsz)
		readblock(1);
}

/* Saves in RIM format. Read with this:
 *	LOC	20
 *	CONO	PTR,60
 * A:	CONSO	PTR,10
 *	JRST	.-1
 *	DATAI	PTR,B
 *	CONSO	PTR,10
 *	JRST	.-1
 * B:	0
 *	JRST	A
 */
void
saverim(const char *filename)
{
	FILE *out;
	hword i;
	out = mustopen(filename, "wb");
	for(i = 0; i < 32; i++) putc(0, out);
	for(i = locmin; i <= locmax; i++){
		writew(fw(0710440, i), out);	/* DATAI PTR,i */
		writew(mem[i], out);
	}
	writew(fw(0254200, start), out);	/* HALT start */
	writew(0, out);
	for(i = 0; i < 32; i++) putc(0, out);
	fclose(out);
}

/* Saves in RIM10 format. Begins with the RIM10B loader,
 *  after that only one block is written.
 *	IOWD count,origin
 *	word
 *	...
 *	word
 *	checksum
 *	...
 *	JRST start
 */
word rim10b[] = {
	0777762000000,
	0710600000060,
	0541400000004,
	0710740000010,
	0254000000003,
	0710470000007,
	0256010000007,
	0256010000012,
	0364400000000,
	0312740000016,
	0270756000001,
	0331740000016,
	0254200000001,
	0253700000003,
	0254000000002,
};
void
saverim10(const char *filename)
{
	FILE *out;
	hword i;
	word w;
	word checksum;

	out = mustopen(filename, "wb");
	checksum = 0;

	/* write RIM10B loader */
	for(i = 0; i < 017; i++)
		writew(rim10b[i], out);

	w = fw(-(locmax+1-locmin), locmin-1);
	writew(w, out);	/* IOWD count,origin */
	checksum += w;
	for(i = locmin; i <= locmax; i++){
		writew(mem[i], out);
		checksum += mem[i];
	}
	checksum &= 0777777777777;
//	writew(-checksum, out);	/* this was apparently wrong? */
	writew(checksum, out);

	writew(fw(0254000, start), out);	/* JRST start */
	fclose(out);
}

/* Saves in SAV format. Only one block is saved.
 *	IOWD count,origin
 *	word
 *	...
 *	word
 *	...
 *	JRST start
 */
void
savesav(const char *filename)
{
	FILE *out;
	hword i;
	out = mustopen(filename, "wb");
	writewbak(fw(-(locmax+1-locmin), locmin-1), out);	/* IOWD count,origin */
	for(i = locmin; i <= locmax; i++)
		writewbak(mem[i], out);
	writewbak(fw(0254000, start), out);	/* JRST start */
	fclose(out);
}

/* Saves in SAV format. Only one block is saved.
 *	-count,,origin
 *	word
 *	...
 *	word
 *	checksum
 *	...
 *	JRST start
 */
void
savesblk(const char *filename)
{
	FILE *out;
	word w, chk;
	hword i;
	out = mustopen(filename, "wb");
	/* write fake loader */
	writewits(0254000000001, out);

	chk = fw(-(locmax+1-locmin), locmin);
	writewits(chk, out);
	for(i = locmin; i <= locmax; i++) {
		w = mem[i];
		chk = ((chk<<1 | chk>>35) + w) & 0777777777777;
		writewits(w, out);
	}
	writewits(chk, out);

	writewits(fw(0254000, start), out);	/* JRST start */
	fclose(out);
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-r relbase] [-f fmt] [-o outfile] input1 input2 ...\n", argv0);
	exit(1);
}

int
main(int argc, char *argv[])
{
	word w;
	hword type;
	int i;
	char *outfile;
	char *fmtstr;
	void (*typesw[8])(void) = {
		skipitem,
		handlecode,
		handlesym,
		skipitem,
		skipitem,
		handleend,
		handlename,
		handlestart,
	};

	outfile = nil;
	rel = 0100;

	ARGBEGIN{
	case 'r':
		rel = strtol(EARGF(usage()), NULL, 8);
		break;

	case 'f':
		fmtstr = EARGF(usage());
		if(strcmp(fmtstr, "rim") == 0)
			fmt = RIM;
		else if(strcmp(fmtstr, "rim10") == 0)
			fmt = RIM10;
		else if(strcmp(fmtstr, "sav") == 0)
			fmt = SAV;
		else if(strcmp(fmtstr, "sblk") == 0)
			fmt = SBLK;
		else
			usage();
		break;

	case 'o':
		outfile = EARGF(usage());
		break;

	default:
		usage();
	}ARGEND;

	if(outfile == nil){
		if(fmt == RIM || fmt == RIM10)
			outfile = "a.rim";
		else if(fmt == SAV)
			outfile = "a.sav";
		else if(fmt == SBLK)
			outfile = "a.bin";
	}

	nfiles = argc;
	files = argv;

	locmax = 0;
	locmin = 0777777;

	for(i = 0; i < nfiles; i++){
		in = mustopen(files[i], "rb");
		while(w = readw(in), w != ~0){
			type = left(w);
			itemsz = right(w);
			typesw[type]();
		}
		fclose(in);
	}
	fixadd();

	dumpsym();

	checkundef();
	if(error)
		return 1;

	printf("%06o %06o\n", locmin, locmax);

//	disasmrange(findsym(rad50(0, "LINKSR"))[1],
//	            findsym(rad50(0, "LINEP"))[1]);
//	disasmrange(0600+rel, 0603+rel);
//	disasmrange(0200+rel, 0212+rel);

	if(fmt == RIM)
		saverim(outfile);
	else if(fmt == RIM10)
		saverim10(outfile);
	else if(fmt == SAV)
		savesav(outfile);
	else if(fmt == SBLK)
		savesblk(outfile);

	return 0;
}
