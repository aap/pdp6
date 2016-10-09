#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "pdp6common.h"

word fw(hword l, hword r) { return ((word)l << 18) | (word)r; }
word point(word pos, word sz, hword p){ return (pos<<30)|(sz<<24)|p; }
hword left(word w) { return (w >> 18) & 0777777; }
hword right(word w) { return w & 0777777; }

/* just a subset here */
enum ItemType
{
	Nothing = 0,
	Code    = 1,
	Symbols = 2,
	Entry   = 4,
	End     = 5,
	Name    = 6,
	Start   = 7,
};

enum SymType
{
	SymName    = 000,
	SymGlobal  = 004,
	SymLocal   = 010,
	SymBlock   = 014,
	SymGlobalH = 044,	/* hidden
	SymLocalH  = 050,	 */
	SymUndef   = 060,
};

enum
{
	MAXSYM = 1000
};

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

FILE *in;
hword itemsz;

word mem[01000000];
hword rel, loc;

int error;

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

/* Insert a symbol into the symbol table. Handle linking. */
void
loadsym(word *newsym)
{
	word *sym;
	int type, newtype;
	char name[8];

	sym = findsym(newsym[0]);
	newtype = (newsym[0]>>30)&074;
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
			fprintf(stderr, "Error: multiple definitions: %s\n", name);
			error = 1;
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
		error = 1;
		printf(" Error: undefined: %s %012lo\n", name, symtab[p+1]);
	}
}

word block[18];
hword blocksz;

word
getword(FILE *f)
{
	int i, b;
	word w;
	w = 0;
	for(i = 0; i < 6; i++){
		if(b = getc(f), b == EOF)
			return ~0;
		w = (w << 6) | (b & 077);
	}
	return w;
}

void
readblock(int mode)
{
	word reloc;
	hword i;
	hword l, r;
	int bits;

	blocksz = 0;
	if(itemsz == 0)
		return;

	reloc = getword(in);
	i = 18;
	while(itemsz && i--){
		block[blocksz++] = getword(in);
		itemsz--;
	}

	/* relocate block */
	for(i = 0; i < 18; i++){
		bits = (reloc >> 34) & 03;
		reloc <<= 2;
		if(mode == 1){
			l = left(block[i]);
			r = right(block[i]);
			if(bits & 1) r += rel;
			if(bits & 2) l += rel;
			block[i] = fw(l, r);
		/* This is weird but necessary for full word reloc */
		}else if(mode == 2){
			if(bits & 1)
				block[i] = (block[i] + rel) & 0777777777777;
		}
	}
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
			loc++;
		}
	}
}

void
handlestart(void)
{
	word w;
	readblock(1);
	w = block[0];
	while(itemsz)
		readblock(1);
}

void
handlesym(void)
{
	int i;
	word w1, w2;

	while(itemsz){
		readblock(2);
		i = 0;
		while(i < blocksz){
			loadsym(&block[i]);
			i += 2;
		}
	}
}

void
skipitem(void)
{
	while(itemsz)
		readblock(0);
}

int
main()
{
	word w;
	hword i, type;
	void (*typesw[8])(void) = {
		skipitem,
		handlecode,
		handlesym,
		skipitem,
		skipitem,
		skipitem,
		handlename,
		handlestart,
	};

//	rel = 01000;
	in = mustopen("test.rel", "rb");
	while(w = getword(in), w != ~0){
		type = left(w);
		itemsz = right(w);
		typesw[type]();
	}
	fclose(in);

	checkundef();
	if(error)
		return 1;

	disasmrange(findsym(rad50(0, "LINKSR"))[1],
	            findsym(rad50(0, "LINEP"))[1]);
	disasmrange(0600+rel, 0603+rel);

	return 0;
}
