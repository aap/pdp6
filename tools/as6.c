#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <stdarg.h>
#include "pdp6common.h"
#include "pdp6bin.h"
#include "../src/args.h"

#define nil NULL

enum
{
	MAXSYM  = 4000,
	MAXLINE = 256,
	MAXOPND = 20
};

enum Toktype
{
	Unused = 0,
	Symbol,
	Word,

	/* These must be kept as a block for expression parsing */
	Plus,
	Minus,
	Star,
	Slash,
	Exclam,
	Amper,
	Shift,

	Radix10,
	Radix8,
	Radix2,

	Eol,

	/* char classes */
	Letter,
	Digit,
	Ignore,
};

enum Symtype
{
	Undef  = 000,

	Local  = 001,	/* defined, Intern may modify this */
	Intern = 002,	/* exported */
	Extern = 004,	/* defined in other module */

	Def    = 010,
	Hide   = 020,	/* not visible to debugger */
	Label  = 040,	/* to forbid redefinition */

	Operator   = 0100,	/* primary instruction and opdef */
	IoOperator,		/* io instruction */
	Pseudo
	/* Macro etc... */
};

typedef struct Value Value;
struct Value
{
	word val;
	int rel;	/* 0 = absolute, else relative */
};

typedef struct Additive Additive;
struct Additive
{
	int l;
	Value v;
	Additive *next;
};

typedef struct Sym Sym;
struct Sym
{
	word name;	/* sixbit */
	int type;	/* Symtype */
	union {
		Value v;
		void (*f)(void);
	};
	Additive *add;
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

int ctab[] = {
	Unused, Unused, Unused, Unused, Unused, Unused, Unused, Unused,
	Unused, Unused, Unused, Unused, Unused, Unused, Unused, Unused,
	Unused, Unused, Unused, Unused, Unused, Unused, Unused, Unused,
	Unused, Unused, Unused, Unused, Unused, Unused, Unused, Unused,

	Ignore, Exclam, Unused, Unused, Letter, Letter,  Amper, Unused,
	   '(',    ')',   Star,   Plus, Unused,  Minus, Letter,  Slash,
	 Digit,  Digit,  Digit,  Digit,  Digit,  Digit,  Digit,  Digit,
	 Digit,  Digit, Unused,    ';',    '<', Unused,    '>', Unused,

	Unused, Letter, Letter, Letter, Letter, Letter, Letter, Letter,
	Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter,
	Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter,
	Letter, Letter, Letter, Unused, Unused, Unused,    '^',  Shift,

	Unused, Letter, Letter, Letter, Letter, Letter, Letter, Letter,
	Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter,
	Letter, Letter, Letter, Letter, Letter, Letter, Letter, Letter,
	Letter, Letter, Letter, Unused, Unused, Unused, Unused, Unused,
};

char *argv0;
char *filename;
char **files;
int nfiles;
FILE *infp, *tmpfp, *relfp, *lstfp;
char progtitle[MAXLINE];
char progsubtitle[MAXLINE];
int hadstart;
Value start;
char line[MAXLINE];
char curline[MAXLINE];	/* unmodified line for listing */
char *lp;	/* pointer to input */
char *ops[MAXOPND];	/* tokenized operands */
int numops;
int lineno;
int pass2;
int error;
Sym symtab[MAXSYM];
Sym *dot;
Value absdot, reldot;	/* saved values of dot */
int radix = 8;
Token peekt;

/*
 * for REL output
 */
word item[01000000];
hword itemp;
hword itemtype, itemsz;

hword relocp;
word blockreloc;
int blocksz;

/* Helpers */

void
err(int n, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "%s:%d: ", filename, lineno);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	error |= n;
}

void
panic(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(1);
}

FILE*
mustopen(const char *name, const char *mode)
{
	FILE *f;
	if(f = fopen(name, mode), f == nil)
		panic("couldn't open file: %s", name);
	return f;
}

/* Output/listing */

void
writeout(void)
{
	hword i;
	for(i = 0; i < itemp-1; i++){
		writew(item[i], relfp);
	//	printf("\t\tDUMP: %06o %06o\n", left(item[i]), right(item[i]));
	}
	printf("\n");
}

/* Start a REL block */
void
startblock(void)
{
	if(blocksz){
		blockreloc <<= 2*(18-blocksz);
		item[relocp] = blockreloc;
		relocp = itemp++;
		blockreloc = 0;
		blocksz = 0;
	}
}

/* Put a relocated word into REL block */
void
putword(word w, int reloc)
{
	item[itemp++] = w;
	blockreloc = blockreloc<<2 | reloc;
	blocksz++;
	itemsz++;
	if(blocksz == 18)
		startblock();
}

/* Start a REL item and write out the last one */
void
startitem(hword it)
{
	startblock();
	if(itemsz){
		item[0] = fw(itemtype, itemsz);
		writeout();
	}

	itemtype = it;
	itemsz = 0;
	itemp = 1;
	relocp = itemp++;
}

int lastline;	/* last line number printed in listing */
void
listline(void)
{
	if(lastline != lineno)
		fprintf(lstfp, "%05d\t%s\n", lineno, curline);
	else
		fprintf(lstfp, "\n");
	lastline = lineno;
}

word relbreak, absbreak;
Value lastdot;

/* Emit a relocated word at dot and advance dot */
void
putv(word w, int rel)
{
	if(pass2){
		fprintf(lstfp, "%06o%c\t%06o%c%06o%c\t",
			right(dot->v.val), dot->v.rel ? '\'' : ' ',
			left(w), rel & 2 ? '\'' : ' ',
			right(w), rel & 1 ? '\'' : ' ');
		listline();

		if(dot->v.rel != lastdot.rel ||
		   dot->v.val != lastdot.val+1){
			startitem(Code);
			putword(dot->v.val, dot->v.rel);
		}
		putword(w, rel);
	}else{
//		printf("\t%06o%c\t%06o%c%06o%c\n",
//			right(dot->v.val), dot->v.rel ? '\'' : ' ',
//			left(w), rel & 2 ? '\'' : ' ',
//			right(w), rel & 1 ? '\'' : ' ');
	}
	lastdot = dot->v;
	dot->v.val++;
	if(dot->v.rel){
		if(dot->v.val > relbreak)
			relbreak = dot->v.val;
	}else{
		if(dot->v.val > absbreak)
			absbreak = dot->v.val;
	}
}

/* Input */

char*
getln(void)
{
	int c;
	char *s;
	static char filen[MAXLINE];

	if(pass2)
		for(;;){
			/* if no words were produced,
			 * list the original line here */
			if(lastline == lineno-1){
				fprintf(lstfp, "\t%14s\t", "");
				listline();
			}

			c = getc(tmpfp);
			if(c == 1){		/* filename */
				s = filen;
				while(c = getc(tmpfp), c)
					*s++ = c;
				*s = '\0';
				filename = filen;
				fprintf(lstfp, "\n\t%s\n\n", filename);
				lineno = 0;
				lastline = 0;
			}else if(c == 2){	/* line */
				s = line;
				while(c = getc(tmpfp), c)
					*s++ = c;
				*s = '\0';
				lineno++;
				strcpy(curline, line);
				return line;
			}else if(c == 3)	/* EOF */
				return nil;
		}

	s = line;
	lineno++;
	while(c = getc(infp), c != EOF){
		if(c == 037){	/* line continuation */
			lineno++;
			getc(infp);
			continue;
		}
		if(c == '\n')
			break;
		if(s < &line[MAXLINE])
			*s++ = c & 0177;	/* force 7 bit */
	}
	if(s >= &line[MAXLINE])
		err(0, "warning: line too long");
	if(c == EOF)
		return nil;
	*s = '\0';
	putc(2, tmpfp);
	fputs(line, tmpfp);
	putc(0, tmpfp);
	return line;
}

void
skipwhite(void)
{
	while(isspace(*lp))
		lp++;
}

/* Symbols */

/* Just find symbol, or return nil.
 * TODO: improve linear search... */
Sym*
findsym(word name)
{
	Sym *s;
	for(s = symtab; s < &symtab[MAXSYM]; s++)
		if(s->name == name)
			return s;
	return nil;
}

/* Get symbol, add to table if not present */
Sym*
getsym(word name)
{
	Sym *s;
	s = findsym(name);
	if(s == nil){
		for(s = symtab; s < &symtab[MAXSYM]; s++)
			if(s->name == 0)
				goto found;
		panic("symbol table full");
found:
		s->name = name;
		s->type = Undef;
		s->v = (Value){ 0, 0 };
	}
	return s;
}

void
updateext(Sym *s, Value *v, int l)
{
	Additive *a;

	if(!pass2 || s == nil)
		return;

	if(l){
		a = malloc(sizeof(Additive));
		a->l = 1;
		a->v = dot->v;
		a->next = s->add;
		s->add = a;
	}else{
		if(v->val == 0 && v->rel == 0){
			/* add to request chain */
			*v = s->v;
			s->v = dot->v;
		}else{
			a = malloc(sizeof(Additive));
			a->l = 0;
			a->v = dot->v;
			a->next = s->add;
			s->add = a;
		}
	}
}

/* Token parsing */

/* parse a potential number, fall back to symbol */
Token
symnum(char *s)
{
	Token t;
	int isnum;
	word n;
	char *symp;

	isnum = 1;
	symp = s;
	n = 0;
	while(*s){
		if(!isdigit(*s))
			isnum = 0;
		else if(isnum){
			symp++;	/* ignore leading digits for symbols */
			n = n*radix + *s-'0';
		}
		s++;
	}
	if(isnum){
		t.type = Word;
		t.w = n;
	}else{
		t.type = Symbol;
		t.s = getsym(sixbit(symp));
	}
	return t;
}

Token
token(void)
{
	Token t;
	char tok[100];
	char *p;

	if(peekt.type != Unused){
		t = peekt;
		peekt.type = Unused;
		return t;
	}

	skipwhite();
	p = tok;
	while(*lp)
		switch(ctab[*lp]){
		case Letter:
		case Digit:
			while(*lp && !isspace(*lp) && israd50(*lp))
				*p++ = *lp++;
			*p = '\0';
			t = symnum(tok);
			return t;

		case ';':
			while(*lp)
				lp++;
			t.type = Eol;
			return t;

		case '^':
			lp++;
			if(*lp == 'D')
				t.type = Radix10;
			else if(*lp == 'O')
				t.type = Radix8;
			else if(*lp == 'B')
				t.type = Radix2;
			else{
				err(1, "error: unknown Radix ^%c\n", *lp);
				t.type = Radix8;
			}
			lp++;
			return t;

		case Unused:
			err(0, "warning: unknown char %c", *lp);
			/* fallthrough */
		case Ignore:
			lp++;
			break;

		default:
			t.type = ctab[*lp++];
			return t;
		}
	t.type = Eol;
	return t;
}

/* Expressions */

/* This holds an expression AST */
typedef struct Expr Expr;
struct Expr
{
	int type;	/* word, symbol or operator */
	word w;		/* type 0 */
	Sym *s;		/* type 1 */
	int op;		/* type 2 (unary), 3 (binary) */
	Expr *l, *r;
};

Expr*
newexpr(int type)
{
	Expr *e;
	e = malloc(sizeof(Expr));
	memset(e, 0, sizeof(Expr));
	e->type = type;
	return e;
}

void
freeexpr(Expr *e)
{
	if(e->type >= 2)
		freeexpr(e->r);
	if(e->type == 3)
		freeexpr(e->l);
	free(e);
}

int
isop(int op)
{
	static int prectab[] = {
		0, 0,	/* Plus, Minus */
		1, 1,	/* Star, Slash */
		2, 2,	/* Exclam, Amper */
		3,	/* Shift */
	};
	if(op >= Plus && op <= Shift)
		return prectab[op-Plus];
	return -1;
}

/* Evaluate an expression AST, also record an additive external symbol.
 * If pass is 1 or 2, the expression must be defined in pass 1 or 2 resp. */
Value
eval(Expr *e, Sym **ext, int pass)
{
	int shft;
	Value v, v2;
	Sym *ex1, *ex2;
	char name[8];

	switch(e->type){
	case 0:
		v.val = e->w;
		v.rel = 0;
		return v;

	case 1:
		if(!pass2 && pass == 1 && (e->s->type & Def) == 0 ||
		    pass2 && pass == 2 && e->s->type & Extern){
			unsixbit(e->s->name, name);
			err(1, "error: symbol %s must be defined", name);
			return (Value){ 0, 0 };
		}
		if(e->s->type & Extern){
			*ext = e->s;
			return (Value){ 0, 0 };
		}
		return e->s->v;

	case 2:
		if(e->op == Minus){
			v = eval(e->r, ext, pass);
			if(*ext)
				err(1, "error: Invalid use of external");
			v.val = negw(v.val);
			return v;
		}
		panic("expression operator");

	case 3:
		ex1 = nil;
		ex2 = nil;
		v = eval(e->l, &ex1, pass);
		v2 = eval(e->r, &ex2, pass);
		if(ex1 == nil)
			*ext = ex2;
		else if(ex2 == nil)
			*ext = ex1;
		else
			err(1, "error: Invalid use of external");

		shft = v2.val;
		if(isneg(v2.val))
			shft = -negw(v2.val);

		switch(e->op){
		case Shift:
			if(shft < 0)
				v.val >>= -shft;
			else
				v.val <<= shft;
			break;
		case Exclam:
			v.val |= v2.val;
			break;
		case Amper:
			v.val &= v2.val;
			break;
		case Star:
			v.val *= v2.val;
			break;
		case Slash:
			v.val /= v2.val;
			break;
		case Plus:
			v.val += v2.val;
			break;

		case Minus:
			v.val -= v2.val;
			if(v.rel == v2.rel)
				v.rel = 0;
			goto ret;
		default:
			panic("expression operator");
		}
		v.rel += v2.rel;
		if(v.rel > 1){
			err(1, "error: invalid expression type");
			return (Value){ 0, 0 };
		}
	ret:
		v.val &= 0777777777777;
		return v;
	}
}


Expr*
parseexpr(int i)
{
	Expr *e, *e2;
	Token t;
	int oldradix;

	t = token();
	if(t.type == Eol){
		e = newexpr(0);
		e->w = 0;
		return e;
	}

	/* unary and atoms */
	if(i == 4){
		if(t.type == Word){
			e = newexpr(0);
			e->w = t.w;
		}else if(t.type == Symbol){
			e = newexpr(1);
			e->s = t.s;
		}else if(t.type == Minus){
			e = newexpr(2);
			e->op = Minus;
			e->r = parseexpr(i);
		}else if(t.type == Radix10 || t.type == Radix8 || t.type == Radix2){
			oldradix = radix;
			if(t.type == Radix10)
				radix = 10;
			else if(t.type == Radix8)
				radix = 8;
			else if(t.type == Radix2)
				radix = 2;
			e = parseexpr(i);
			radix = oldradix;
		}else if(t.type == '<'){
			e = parseexpr(0);
			t = token();
			if(t.type != '>'){
				err(1, "error: '>' expected");
				peekt = t;
			}
		}else{
			err(1, "error: expression syntax %o", t.type);
			return nil;
		}
		return e;
	}

	/* n-ary expressions */
	if(t.type == Word || t.type == Symbol ||
	   t.type == Minus || t.type == '<' ||
	   t.type == Radix10 || t.type == Radix8 || t.type == Radix2 ){
		peekt = t;
		e = parseexpr(i+1);
	}else{
		err(1, "error: expression syntax %o", t.type);
		return nil;
	}

	for(t = token(); isop(t.type) == i; t = token()){
		e2 = newexpr(3);
		e2->op = t.type;
		e2->l = e;
		e2->r = parseexpr(i+1);
		e = e2;
	}
	if(t.type != Eol)
		peekt = t;
	return e;
}

void
printexpr(Expr *e)
{
	char name[8];
	char *opnames[] = {
		"+", "-", "*", "/", "!", "&", "_", "^D", "^O", "^B"
	};
	if(e->type == 0)
		printf("%lo", e->w);
	else if(e->type == 1){
		unsixbit(e->s->name, name);
		printf("%s", name);
	}else if(e->type == 2){
		printf("(%s ", opnames[e->op - Plus]);
		printexpr(e->r);
		printf(")");
	}else if(e->type == 3){
		printf("(");
		printexpr(e->l);
		printf(" %s ", opnames[e->op - Plus]);
		printexpr(e->r);
		printf(")");
	}
}

Value
expr(int pass, Sym **s)
{
	/* -			5
	 * ^D ^O ^B ^F ^L	4
	 * B _			3
	 * logical	! &	2
	 * mul/div	* /	1
	 * add/sub	+ -	0
	 * <>
	 */

	Expr *e;
	Sym *ext;
	Value v;
	char name[8];

/*	printf("EXP LINE: %s %o\n", lp, peekt.type); */
	ext = nil;
	e = parseexpr(0);
/*	printf("EXPR: ");
	printexpr(e);
	printf("\n");*/
	v = eval(e, &ext, pass);
	if(s)
		*s = ext;
/*
	if(ext){
		unsixbit(ext->name, name);
		printf("SAW EXTERNAL: %s\n", name);
	}
*/
	freeexpr(e);
	return v;
}

/* Statements */

/* split operands separated by ',' starting at lp; remove whitespace */
void
splitop(void)
{
	int nparn;

	numops = 0;
	skipwhite();
	if(*lp == ';' || *lp == '\0')
		return;

	/* this routine is terrible, null everything
	 * we don't care about and drag along pointers
	 * to the beginnig of operands */
	ops[numops++] = lp;
	for(; *lp && *lp != ';'; lp++){
		if(isspace(*lp))
			*lp = '\0';
		else if(*lp == ','){
			*lp = '\0';
			ops[numops++] = lp;
			if(numops >= MAXOPND){
				err(0, "warning: operand overflow");
				return;
			}
		}else if(*lp == '('){
			/* read parenthesized operands as one */
			if(*ops[numops-1] == '\0')
				ops[numops-1] = lp;
			lp++;
			nparn = 1;
			for(; *lp && *lp != ';' && nparn != 0; lp++)
				if(*lp == '(')
					nparn++;
				else if(*lp == ')')
					nparn--;
		}else{
			if(*ops[numops-1] == '\0')
				ops[numops-1] = lp;
		}
	}
	*lp = '\0';

}

/* Primary and IO instruction statement */
void
opline(word w, int io)
{
	char *acp, *ep;
	Sym *ext;
	Value ac, x, y;
	Token t;
	int i;

	splitop();
	/*
	 * OP AC,
	 * OP   ,E
	 * OP    E
	 * E: @Y(X)
	 */
	switch(numops){
	case 0:
		acp = "";
		ep = "";
		break;
	case 1:
		acp = "";
		ep = ops[0];
		break;
	default:
		err(0, "warning: too many operands");
	case 2:
		acp = ops[0];
		ep = ops[1];
	}

	lp = acp;
	ac = expr(2, nil);

	i = 0;
	lp = ep;
	if(*lp == '@'){
		lp++;
		i = 1;
	}
	ext = nil;
	y = expr(0, &ext);
	updateext(ext, &y, 0);

	x = (Value){ 0, 0 };
	t = token();
	if(t.type == '('){
		/* TODO: this is probably a statement */
		x = expr(2, nil);
		t = token();
		if(t.type != ')')
			err(1, "error: ')' expected");
		x.val = fw(right(x.val), left(x.val));
	}

	/* TODO: is this correct or should we mask out some stuff? */
	if(io){
		w += (ac.val&0774) << 24;
	}else
		w += (ac.val&017) << 23;
	if(ac.rel)
		err(0, "warning: AC relocation ignored");
	if(i)
		w |= 0000020000000;
	w = fw(left(w), right(w)+right(y.val));
	/* TODO: really warn about this? */
	if(left(y.val))
		err(0, "warning: address too large");
	w = fw(left(w)+left(x.val), right(w)+right(x.val));
	if(x.rel)
		err(0, "warning: X relocation ignored");
	putv(w, y.rel);
}

/*
 * Pseudo Ops
 */

void
internal(void)
{
	Sym *s;
	int i;
	if(pass2)
		return;
	splitop();
	for(i = 0; i < numops; i++){
		s = getsym(sixbit(ops[i]));
		if(s->type >= Operator || s->type & Extern)
			err(0, "warning: can't INTERN symbol %s", ops[i]);
		else
			s->type |= Intern;
	}
}

void
external(void)
{
	Sym *s;
	int i;
	if(pass2)
		return;
	splitop();
	for(i = 0; i < numops; i++){
		s = getsym(sixbit(ops[i]));
		if(s->type >= Operator || s->type & Intern)
			err(0, "warning: can't EXTERN symbol %s", ops[i]);
		else
			s->type |= Extern;
	}
}

void
xwd(void)
{
	Value l, r;
	Sym *exl, *exr;

	splitop();
	l = (Value){ 0, 0 };
	r = (Value){ 0, 0 };
	if(numops != 2)
		err(0, "warning: need two operands");
	else{
		exl = nil;
		exr = nil;
		lp = ops[0];
		l = expr(0, &exl);
		lp = ops[1];
		r = expr(0, &exr);
		updateext(exl, &l, 1);
		updateext(exr, &r, 0);
	}
	putv(fw(right(l.val), right(r.val)), (l.rel<<1) | r.rel);
}

void
title(void)
{
	skipwhite();
	strncpy(progtitle, lp, MAXLINE);
}

void
subttl(void)
{
	skipwhite();
	strncpy(progsubtitle, lp, MAXLINE);
}

void
locOp(void)
{
	skipwhite();
	if(dot->v.rel) reldot = dot->v;
	else absdot = dot->v;
	if(*lp && *lp != ';'){
		dot->v = expr(1, nil);
		dot->v.val &= 0777777;
		dot->v.rel = 0;
	}else
		dot->v = absdot;
}

void
relocOp(void)
{
	skipwhite();
	if(dot->v.rel) reldot = dot->v;
	else absdot = dot->v;
	if(*lp && *lp != ';'){
		dot->v = expr(1, nil);
		dot->v.val &= 0777777;
		dot->v.rel = 1;
	}else
		dot->v = reldot;
}

int asciiz;

void
ascii(void)
{
	char delim;
	word w;
	int i;

	skipwhite();
	delim = *lp++;
	if(delim == ';' || delim == '\0'){
		putv(0, 0);
		return;
	}
	w = 0;
	i = 0;
	for(; *lp != delim; lp++){
		/* This is a hack since LF is not part of the line
		 * but we have to output it in multiline strings */
		if(*lp == '\0')
			*lp = '\n';

		w = (w<<7) | *lp;
		if(++i == 5){
			w <<= 1;
			putv(w, 0);
			i = 0;
			w = 0;
		}

		/* load the next line when we recognized EOL earlier */
		if(*lp == '\n'){
			lp = getln();
			if(lp == nil){
				err(1, "error: unexpected EOF");
				break;
			}
			lp--;
		}
	}
	if(asciiz){
		i++;
		w <<= 7;
	}
	w <<= 36-i*7;
	if(w || asciiz)
		putv(w, 0);
}

void
asciz(void)
{
	asciiz = 1;
	ascii();
	asciiz = 0;
}

void
sixbitOp(void)
{
	char delim;
	char c;
	word w;
	int i;

	skipwhite();
	delim = *lp++;
	if(delim == ';' || delim == '\0'){
		putv(0, 0);
		return;
	}
	w = 0;
	i = 0;
	for(; *lp && *lp != delim; lp++){
		c = ascii2sixbit(*lp);
		if(c < 0){
			err(0, "warning: ignoring non-SIXBIT char %c", *lp);
			continue;
		}
		w = (w<<6) | c;
		if(++i == 6){
			putv(w, 0);
			i = 0;
			w = 0;
		}
	}
	if(*lp != delim)
		err(1, "error: '%c' expected", delim);
	w <<= 36-i*6;
	if(w)
		putv(w, 0);
}

void
radixOp(void)
{
	int r;
	Value v;

	skipwhite();
	if(*lp == '\0' || *lp == ';')
		return;
	r = radix;
	radix = 10;
	v = expr(1, nil);
	radix = r;
	if(v.val < 2 || v.val > 10)
		err(1, "error: invalid radix %d", v.val);
	else
		radix = v.val;
}

void
blockOp(void)
{
	Value v;
	skipwhite();
	v = expr(1, nil);
	if(v.rel)
		err(1, "error: non-absolute block size");
	else
		dot->v.val += v.val;
}

void
endOp(void)
{
	skipwhite();
	start = expr(2, nil);
	hadstart = 1;
}

void
statement(void)
{
	Token t;
	Value val;
	int type;
	Sym *s, *ext;
	char name[8];

	while(t = token(), t.type != Eol)
		switch(t.type){
		case Symbol:
			s = t.s;
			unsixbit(s->name, name);
			if(*lp == ':'){
				/* label:
				 * :   label
				 * :!  label          hide
				 * ::  label internal
				 * ::! label internal hide
				 */
				lp++;
				type = Label | Local | Def;
				if(*lp == ':'){
					lp++;
					type |= Intern;
				}
				if(*lp == '!'){
					lp++;
					type |= Hide;
				}
				if(!pass2){
					if(s->type & Def){
						err(1, "error: redefinition of %s", name);
						return;
					}
					s->type = (s->type&(Intern|Extern|Hide)) | type;
					s->v = dot->v;
				}
			}else if(*lp == '='){
				/* assignment:
				 * =   assign
				 * ==  assign          hide
				 * =:  assign internal
				 * ==: assign internal hide
				 */
				lp++;
				type = Local | Def;
				if(*lp == '='){
					lp++;
					type |= Hide;
				}
				if(*lp == ':'){
					lp++;
					type |= Intern;
				}
				val = expr(1, nil);
				if(!pass2){
					if(s->type & Label){
						err(1, "error: redefinition of %s", name);
						return;
					}
					s->type = (s->type&(Intern|Extern|Hide)) | type;
					s->v = val;
				}
			}else if(s->type == Operator){
				unsixbit(s->name, name);
				opline(s->v.val, 0);
				return;
			}else if(s->type == IoOperator){
				unsixbit(s->name, name);
				opline(s->v.val, 1);
				return;
			}else if(s->type == Pseudo){
				s->f();
				return;
			}else
				goto exp;
			break;

		exp:
		case Word:
		case Radix10: case Radix8: case Radix2:
		case Minus:
			peekt = t;
			ext = nil;
			val = expr(0, &ext);
			updateext(ext, &val, 0);
			putv(val.val, val.rel);
			break;

		default:
			err(0, "unknown token %c(%o)", t.type, t.type);
		}
}

void
assemble(void)
{
	while(lp = getln()){
		peekt.type = Unused;
		statement();
	}
}

void
resetdot(void)
{
	dot = getsym(sixbit("."));
	dot->type = Local | Def | Hide;
	absdot = (Value){ 0, 0 };
	reldot = (Value){ 0, 1 };
	dot->v = reldot;
	lastdot = dot->v;
	/* invalidate lastdot */
	lastdot.val = 0777777777777;
}

void
checkundef(int glob)
{
	char name[8];
	Sym *s;
	for(s = symtab; s < &symtab[MAXSYM]; s++)
		if(s->name &&
		   s->type != Operator && s->type != IoOperator && s->type != Pseudo){
			unsixbit(s->name, name);
			if(s->type == Undef){
				if(glob)
					s->type |= Extern;
				else
					err(1, "undefined symbol: %s\n", name);
			}
		}
}

int
symcmp(const void *a, const void *b)
{
	char s1[8], s2[8];
	unsixbit((*(Sym**)a)->name, s1);
	unsixbit((*(Sym**)b)->name, s2);
	return strcmp(s1, s2);
}

void
writesymtab(void)
{
	hword l, r;
	char name[8];
	Sym *s;
	Sym *sortlist[MAXSYM];
	Additive *add;
	word rad;
	int i, nsym;
	int type;

	nsym = 0;
	for(s = symtab; s < &symtab[MAXSYM]; s++){
		if(s->name == 0 || s == dot ||
		   s->type == Operator || s->type == IoOperator || s->type == Pseudo)
			continue;
		sortlist[nsym++] = s;
	}
	qsort(sortlist, nsym, sizeof(Sym*), symcmp);


	startitem(Symbols);
	fprintf(lstfp, "\nSYMBOL TABLE\n\n");
	for(i = 0; i < nsym; i++){
		s = sortlist[i];

		if(s->type & Extern &&
		   s->v.val == 0 && s->v.rel == 0)
			continue;

		type = 0;
		if(s->type & Intern)
			type = SymGlobal;
		else if(s->type & Local)
			type = SymLocal;
		if(s->type & Hide)
			type |= 040;
		if(s->type & Extern)
			type = SymUndef;

		unsixbit(s->name, name);
		l = left(s->v.val);
		r = right(s->v.val);
		fprintf(lstfp, "%s%6s", name, "");
		if(l)
			fprintf(lstfp, "%06o", l);
		else
			fprintf(lstfp, "%6s", "");
		fprintf(lstfp, "%06o%c ", r, s->v.rel ? '\'' : ' ');
		if(s->type & Extern)
			fprintf(lstfp, "EXT");
		if(s->type & Intern)
			fprintf(lstfp, "INT");
		fprintf(lstfp, "\n");

		rad = rad50(type, name);
		putword(rad, 0);
		putword(fw(0, right(s->v.val)), s->v.rel);

		printf("%s %3o: %012lo %o\n", name, s->type, s->v.val, s->v.rel);
		for(add = s->add; add; add = add->next){
			printf(" %c %06o\n", "rl"[add->l], right(add->v.val));

			putword(rad, 0);
			putword(fw(0400000 | (add->l ? 0200000 : 0), right(add->v.val)),
			        add->v.rel);
		}
	}
}

/* Basic program flow */

void
cleanup(void)
{
	remove("a.tmp");
}

void
usage(void)
{
	panic("usage: %s", argv0);
}

void initsymtab(void);

int
main(int argc, char *argv[])
{
	int i;
	char *outfile;
	char *lstfile;


	outfile = "a.rel";
	lstfile = "/dev/null";

	ARGBEGIN{
	case 'o':
		outfile = EARGF(usage());
		break;
	case 'l':
		lstfile = EARGF(usage());
		break;
	default:
		usage();
		break;
	}ARGEND;

	nfiles = argc;
	files = argv;

	tmpfp = mustopen("a.tmp", "w");
	atexit(cleanup);

	initsymtab();

	if(0){
	Sym *s;
	s = getsym(sixbit("HELLO"));
	s->type = Local | Def;
	s->v.val = 0432100001234;
	s->v.rel = 1;
	s = getsym(sixbit("ASDF"));
	s->type = Intern | Def;
	s->v.val = 0100;
	s->v.rel = 1;
	s = getsym(sixbit("BAR"));
	s->type = Local | Def;
	s->v.val = 0100;
	s->v.rel = 0;
	s = getsym(sixbit("QUUX"));
	s->type = Extern;
	s->v.val = 0;
	s->v.rel = 0;
	s = getsym(sixbit("QWERTY"));
	s->type = Label | Local | Def;
	s->v.val = 010;
	s->v.rel = 1;
	s = getsym(sixbit("QWERTYUIOP"));
	}

	resetdot();
	pass2 = 0;
	if(argc){
		for(i = 0; i < argc; i++){
			filename = argv[i];
			infp = mustopen(filename, "r");
			putc(1, tmpfp);
			fputs(filename, tmpfp);
			putc(0, tmpfp);
			lineno = 0;

			assemble();
			fclose(infp);
		}
	}else{
		filename = "stdin";
		infp = stdin;
		putc(1, tmpfp);
		fputs(filename, tmpfp);
		putc(0, tmpfp);
		lineno = 0;

		assemble();
	}
	putc(3, tmpfp);

	fclose(tmpfp);
	checkundef(1);

	if(error & 1)
		panic("Error\n*****");

	lstfp = mustopen(lstfile, "w");
	relfp = mustopen(outfile, "w");
	tmpfp = mustopen("a.tmp", "r");

	resetdot();
	pass2 = 1;
//	printf("\n   PASS2\n\n");

	startitem(Name);
	putword(rad50(0, progtitle), 0);

	assemble();

	writesymtab();

	if(hadstart){
		startitem(Start);
		putword(right(start.val), start.rel);
	}

	startitem(End);
	putword(right(relbreak), 1);
	putword(right(absbreak), 0);
	startitem(Nothing);

	fclose(tmpfp);
	fclose(relfp);
	fclose(lstfp);

	return 0;
}






typedef struct Op Op;
struct Op
{
	const char *name;
	int type;
	word val;
};

typedef struct Ps Ps;
struct Ps
{
	const char *name;
	void (*f)(void);
};

extern Op oplist[];
extern Ps pslist[];
void
initsymtab(void)
{
	Op *op;
	Ps *ps;
	Sym *s;

	for(op = oplist; op->name[0]; op++){
		s = getsym(sixbit(op->name));
		s->type = op->type;
		s->v.val = op->val;
		s->v.rel = 0;
	}
	for(ps = pslist; ps->name[0]; ps++){
		s = getsym(sixbit(ps->name));
		s->type = Pseudo;
		s->f = ps->f;
	}
}

Ps pslist[] = {
	{ "INTERNAL", internal },
	{ "EXTERNAL", external },
	{ "XWD",      xwd },
	{ "TITLE",    title },
	{ "SUBTTL",   subttl },
	{ "LOC",      locOp },
	{ "RELOC",    relocOp },
	{ "ASCII",    ascii },
	{ "ASCIZ",    asciz },
	{ "SIXBIT",   sixbitOp },
	{ "RADIX",    radixOp },
	{ "BLOCK",    blockOp },
	{ "END",      endOp },
/*
   " '
   RADIX50 SQUOZE
   DEC
   OCT
   EXP
   BYTE
   POINT
   IOWD
    INTEGER
    ARRAY
   LIT
   ENTRY
   OPDEF
*/
	{ "", nil }
};

Op oplist[] = {
	{ "Z",   Operator, 0 },

	{ "UUO1",   Operator, 0001000000000 },
	{ "UUO2",   Operator, 0002000000000 },

	{ "FSC",   Operator, 0132000000000 },
	{ "IBP",   Operator, 0133000000000 },
	{ "ILDB",  Operator, 0134000000000 },
	{ "LDB",   Operator, 0135000000000 },
	{ "IDPB",  Operator, 0136000000000 },
	{ "DPB",   Operator, 0137000000000 },

	{ "FAD",   Operator, 0140000000000 },
	{ "FADL",  Operator, 0141000000000 },
	{ "FADM",  Operator, 0142000000000 },
	{ "FADB",  Operator, 0143000000000 },
	{ "FADR",  Operator, 0144000000000 },
	{ "FADRL", Operator, 0145000000000 },
	{ "FADRM", Operator, 0146000000000 },
	{ "FADRB", Operator, 0147000000000 },
	{ "FSB",   Operator, 0150000000000 },
	{ "FSBL",  Operator, 0151000000000 },
	{ "FSBM",  Operator, 0152000000000 },
	{ "FSBB",  Operator, 0153000000000 },
	{ "FSBR",  Operator, 0154000000000 },
	{ "FSBRL", Operator, 0155000000000 },
	{ "FSBRM", Operator, 0156000000000 },
	{ "FSBRB", Operator, 0157000000000 },
	{ "FMP",   Operator, 0160000000000 },
	{ "FMPL",  Operator, 0161000000000 },
	{ "FMPM",  Operator, 0162000000000 },
	{ "FMPB",  Operator, 0163000000000 },
	{ "FMPR",  Operator, 0164000000000 },
	{ "FMPRL", Operator, 0165000000000 },
	{ "FMPRM", Operator, 0166000000000 },
	{ "FMPRB", Operator, 0167000000000 },
	{ "FDV",   Operator, 0170000000000 },
	{ "FDVL",  Operator, 0171000000000 },
	{ "FDVM",  Operator, 0172000000000 },
	{ "FDVB",  Operator, 0173000000000 },
	{ "FDVR",  Operator, 0174000000000 },
	{ "FDVRL", Operator, 0175000000000 },
	{ "FDVRM", Operator, 0176000000000 },
	{ "FDVRB", Operator, 0177000000000 },

	{ "MOVE",  Operator, 0200000000000 },
	{ "MOVEI", Operator, 0201000000000 },
	{ "MOVEM", Operator, 0202000000000 },
	{ "MOVES", Operator, 0203000000000 },
	{ "MOVS",  Operator, 0204000000000 },
	{ "MOVSI", Operator, 0205000000000 },
	{ "MOVSM", Operator, 0206000000000 },
	{ "MOVSS", Operator, 0207000000000 },
	{ "MOVN",  Operator, 0210000000000 },
	{ "MOVNI", Operator, 0211000000000 },
	{ "MOVNM", Operator, 0212000000000 },
	{ "MOVNS", Operator, 0213000000000 },
	{ "MOVM",  Operator, 0214000000000 },
	{ "MOVMI", Operator, 0215000000000 },
	{ "MOVMM", Operator, 0216000000000 },
	{ "MOVMS", Operator, 0217000000000 },

	{ "IMUL",  Operator, 0220000000000 },
	{ "IMULI", Operator, 0221000000000 },
	{ "IMULM", Operator, 0222000000000 },
	{ "IMULB", Operator, 0223000000000 },
	{ "MUL",   Operator, 0224000000000 },
	{ "MULI",  Operator, 0225000000000 },
	{ "MULM",  Operator, 0226000000000 },
	{ "MULB",  Operator, 0227000000000 },
	{ "IDIV",  Operator, 0230000000000 },
	{ "IDIVI", Operator, 0231000000000 },
	{ "IDIVM", Operator, 0232000000000 },
	{ "IDIVB", Operator, 0233000000000 },
	{ "DIV",   Operator, 0234000000000 },
	{ "DIVI",  Operator, 0235000000000 },
	{ "DIVM",  Operator, 0236000000000 },
	{ "DIVB",  Operator, 0237000000000 },

	{ "ASH",   Operator, 0240000000000 },
	{ "ROT",   Operator, 0241000000000 },
	{ "LSH",   Operator, 0242000000000 },
	{ "ASHC",  Operator, 0244000000000 },
	{ "ROTC",  Operator, 0245000000000 },
	{ "LSHC",  Operator, 0246000000000 },

	{ "EXCH",  Operator, 0250000000000 },
	{ "BLT",   Operator, 0251000000000 },
	{ "AOBJP", Operator, 0252000000000 },
	{ "AOBJN", Operator, 0253000000000 },
	{ "JRST",  Operator, 0254000000000 },
	{ "JFCL",  Operator, 0255000000000 },
	{ "XCT",   Operator, 0256000000000 },

	{ "PUSHJ", Operator, 0260000000000 },
	{ "PUSH",  Operator, 0261000000000 },
	{ "POP",   Operator, 0262000000000 },
	{ "POPJ",  Operator, 0263000000000 },
	{ "JSR",   Operator, 0264000000000 },
	{ "JSP",   Operator, 0265000000000 },
	{ "JSA",   Operator, 0266000000000 },
	{ "JRA",   Operator, 0267000000000 },

	{ "ADD",   Operator, 0270000000000 },
	{ "ADDI",  Operator, 0271000000000 },
	{ "ADDM",  Operator, 0272000000000 },
	{ "ADDB",  Operator, 0273000000000 },
	{ "SUB",   Operator, 0274000000000 },
	{ "SUBI",  Operator, 0275000000000 },
	{ "SUBM",  Operator, 0276000000000 },
	{ "SUBB",  Operator, 0277000000000 },

	{ "CAI",   Operator, 0300000000000 },
	{ "CAIL",  Operator, 0301000000000 },
	{ "CAIE",  Operator, 0302000000000 },
	{ "CAILE", Operator, 0303000000000 },
	{ "CAIA",  Operator, 0304000000000 },
	{ "CAIGE", Operator, 0305000000000 },
	{ "CAIN",  Operator, 0306000000000 },
	{ "CAIG",  Operator, 0307000000000 },
	{ "CAM",   Operator, 0310000000000 },
	{ "CAML",  Operator, 0311000000000 },
	{ "CAME",  Operator, 0312000000000 },
	{ "CAMLE", Operator, 0313000000000 },
	{ "CAMA",  Operator, 0314000000000 },
	{ "CAMGE", Operator, 0315000000000 },
	{ "CAMN",  Operator, 0316000000000 },
	{ "CAMG",  Operator, 0317000000000 },
	{ "JUMP",  Operator, 0320000000000 },
	{ "JUMPL", Operator, 0321000000000 },
	{ "JUMPE", Operator, 0322000000000 },
	{ "JUMPLE",Operator, 0323000000000 },
	{ "JUMPA", Operator, 0324000000000 },
	{ "JUMPGE",Operator, 0325000000000 },
	{ "JUMPN", Operator, 0326000000000 },
	{ "JUMPG", Operator, 0327000000000 },
	{ "SKIP",  Operator, 0330000000000 },
	{ "SKIPL", Operator, 0331000000000 },
	{ "SKIPE", Operator, 0332000000000 },
	{ "SKIPLE",Operator, 0333000000000 },
	{ "SKIPA", Operator, 0334000000000 },
	{ "SKIPGE",Operator, 0335000000000 },
	{ "SKIPN", Operator, 0336000000000 },
	{ "SKIPG", Operator, 0337000000000 },
	{ "AOJ",   Operator, 0340000000000 },
	{ "AOJL",  Operator, 0341000000000 },
	{ "AOJE",  Operator, 0342000000000 },
	{ "AOJLE", Operator, 0343000000000 },
	{ "AOJA",  Operator, 0344000000000 },
	{ "AOJGE", Operator, 0345000000000 },
	{ "AOJN",  Operator, 0346000000000 },
	{ "AOJG",  Operator, 0347000000000 },
	{ "AOS",   Operator, 0350000000000 },
	{ "AOSL",  Operator, 0351000000000 },
	{ "AOSE",  Operator, 0352000000000 },
	{ "AOSLE", Operator, 0353000000000 },
	{ "AOSA",  Operator, 0354000000000 },
	{ "AOSGE", Operator, 0355000000000 },
	{ "AOSN",  Operator, 0356000000000 },
	{ "AOSG",  Operator, 0357000000000 },
	{ "SOJ",   Operator, 0360000000000 },
	{ "SOJL",  Operator, 0361000000000 },
	{ "SOJE",  Operator, 0362000000000 },
	{ "SOJLE", Operator, 0363000000000 },
	{ "SOJA",  Operator, 0364000000000 },
	{ "SOJGE", Operator, 0365000000000 },
	{ "SOJN",  Operator, 0366000000000 },
	{ "SOJG",  Operator, 0367000000000 },
	{ "SOS",   Operator, 0370000000000 },
	{ "SOSL",  Operator, 0371000000000 },
	{ "SOSE",  Operator, 0372000000000 },
	{ "SOSLE", Operator, 0373000000000 },
	{ "SOSA",  Operator, 0374000000000 },
	{ "SOSGE", Operator, 0375000000000 },
	{ "SOSN",  Operator, 0376000000000 },
	{ "SOSG",  Operator, 0377000000000 },

	{ "SETZ",  Operator, 0400000000000 },
	{ "SETZI", Operator, 0401000000000 },
	{ "SETZM", Operator, 0402000000000 },
	{ "SETZB", Operator, 0403000000000 },
	{ "AND",   Operator, 0404000000000 },
	{ "ANDI",  Operator, 0405000000000 },
	{ "ANDM",  Operator, 0406000000000 },
	{ "ANDB",  Operator, 0407000000000 },
	{ "ANDCA", Operator, 0410000000000 },
	{ "ANDCAI",Operator, 0411000000000 },
	{ "ANDCAM",Operator, 0412000000000 },
	{ "ANDCAB",Operator, 0413000000000 },
	{ "SETM",  Operator, 0414000000000 },
	{ "SETMI", Operator, 0415000000000 },
	{ "SETMM", Operator, 0416000000000 },
	{ "SETMB", Operator, 0417000000000 },
	{ "ANDCM", Operator, 0420000000000 },
	{ "ANDCMI",Operator, 0421000000000 },
	{ "ANDCMM",Operator, 0422000000000 },
	{ "ANDCMB",Operator, 0423000000000 },
	{ "SETA",  Operator, 0424000000000 },
	{ "SETAI", Operator, 0425000000000 },
	{ "SETAM", Operator, 0426000000000 },
	{ "SETAB", Operator, 0427000000000 },
	{ "XOR",   Operator, 0430000000000 },
	{ "XORI",  Operator, 0431000000000 },
	{ "XORM",  Operator, 0432000000000 },
	{ "XORB",  Operator, 0433000000000 },
	{ "IOR",   Operator, 0434000000000 },
	{ "IORI",  Operator, 0435000000000 },
	{ "IORM",  Operator, 0436000000000 },
	{ "IORB",  Operator, 0437000000000 },
	{ "ANDCB", Operator, 0440000000000 },
	{ "ANDCBI",Operator, 0441000000000 },
	{ "ANDCBM",Operator, 0442000000000 },
	{ "ANDCBB",Operator, 0443000000000 },
	{ "EQV",   Operator, 0444000000000 },
	{ "EQVI",  Operator, 0445000000000 },
	{ "EQVM",  Operator, 0446000000000 },
	{ "EQVB",  Operator, 0447000000000 },
	{ "SETCA", Operator, 0450000000000 },
	{ "SETCAI",Operator, 0451000000000 },
	{ "SETCAM",Operator, 0452000000000 },
	{ "SETCAB",Operator, 0453000000000 },
	{ "ORCA",  Operator, 0454000000000 },
	{ "ORCAI", Operator, 0455000000000 },
	{ "ORCAM", Operator, 0456000000000 },
	{ "ORCAB", Operator, 0457000000000 },
	{ "SETCM", Operator, 0460000000000 },
	{ "SETCMI",Operator, 0461000000000 },
	{ "SETCMM",Operator, 0462000000000 },
	{ "SETCMB",Operator, 0463000000000 },
	{ "ORCM",  Operator, 0464000000000 },
	{ "ORCMI", Operator, 0465000000000 },
	{ "ORCMM", Operator, 0466000000000 },
	{ "ORCMB", Operator, 0467000000000 },
	{ "ORCB",  Operator, 0470000000000 },
	{ "ORCBI", Operator, 0471000000000 },
	{ "ORCBM", Operator, 0472000000000 },
	{ "ORCBB", Operator, 0473000000000 },
	{ "SETO",  Operator, 0474000000000 },
	{ "SETOI", Operator, 0475000000000 },
	{ "SETOM", Operator, 0476000000000 },
	{ "SETOB", Operator, 0477000000000 },

	{ "HLL",   Operator, 0500000000000 },
	{ "HLLI",  Operator, 0501000000000 },
	{ "HLLM",  Operator, 0502000000000 },
	{ "HLLS",  Operator, 0503000000000 },
	{ "HRL",   Operator, 0504000000000 },
	{ "HRLI",  Operator, 0505000000000 },
	{ "HRLM",  Operator, 0506000000000 },
	{ "HRLS",  Operator, 0507000000000 },
	{ "HLLZ",  Operator, 0510000000000 },
	{ "HLLZI", Operator, 0511000000000 },
	{ "HLLZM", Operator, 0512000000000 },
	{ "HLLZS", Operator, 0513000000000 },
	{ "HRLZ",  Operator, 0514000000000 },
	{ "HRLZI", Operator, 0515000000000 },
	{ "HRLZM", Operator, 0516000000000 },
	{ "HRLZS", Operator, 0517000000000 },
	{ "HLLO",  Operator, 0520000000000 },
	{ "HLLOI", Operator, 0521000000000 },
	{ "HLLOM", Operator, 0522000000000 },
	{ "HLLOS", Operator, 0523000000000 },
	{ "HRLO",  Operator, 0524000000000 },
	{ "HRLOI", Operator, 0525000000000 },
	{ "HRLOM", Operator, 0526000000000 },
	{ "HRLOS", Operator, 0527000000000 },
	{ "HLLE",  Operator, 0530000000000 },
	{ "HLLEI", Operator, 0531000000000 },
	{ "HLLEM", Operator, 0532000000000 },
	{ "HLLES", Operator, 0533000000000 },
	{ "HRLE",  Operator, 0534000000000 },
	{ "HRLEI", Operator, 0535000000000 },
	{ "HRLEM", Operator, 0536000000000 },
	{ "HRLES", Operator, 0537000000000 },
	{ "HRR",   Operator, 0540000000000 },
	{ "HRRI",  Operator, 0541000000000 },
	{ "HRRM",  Operator, 0542000000000 },
	{ "HRRS",  Operator, 0543000000000 },
	{ "HLR",   Operator, 0544000000000 },
	{ "HLRI",  Operator, 0545000000000 },
	{ "HLRM",  Operator, 0546000000000 },
	{ "HLRS",  Operator, 0547000000000 },
	{ "HRRZ",  Operator, 0550000000000 },
	{ "HRRZI", Operator, 0551000000000 },
	{ "HRRZM", Operator, 0552000000000 },
	{ "HRRZS", Operator, 0553000000000 },
	{ "HLRZ",  Operator, 0554000000000 },
	{ "HLRZI", Operator, 0555000000000 },
	{ "HLRZM", Operator, 0556000000000 },
	{ "HLRZS", Operator, 0557000000000 },
	{ "HRRO",  Operator, 0560000000000 },
	{ "HRROI", Operator, 0561000000000 },
	{ "HRROM", Operator, 0562000000000 },
	{ "HRROS", Operator, 0563000000000 },
	{ "HLRO",  Operator, 0564000000000 },
	{ "HLROI", Operator, 0565000000000 },
	{ "HLROM", Operator, 0566000000000 },
	{ "HLROS", Operator, 0567000000000 },
	{ "HRRE",  Operator, 0570000000000 },
	{ "HRREI", Operator, 0571000000000 },
	{ "HRREM", Operator, 0572000000000 },
	{ "HRRES", Operator, 0573000000000 },
	{ "HLRE",  Operator, 0574000000000 },
	{ "HLREI", Operator, 0575000000000 },
	{ "HLREM", Operator, 0576000000000 },
	{ "HLRES", Operator, 0577000000000 },

	{ "TRN",   Operator, 0600000000000 },
	{ "TLN",   Operator, 0601000000000 },
	{ "TRNE",  Operator, 0602000000000 },
	{ "TLNE",  Operator, 0603000000000 },
	{ "TRNA",  Operator, 0604000000000 },
	{ "TLNA",  Operator, 0605000000000 },
	{ "TRNN",  Operator, 0606000000000 },
	{ "TLNN",  Operator, 0607000000000 },
	{ "TDN",   Operator, 0610000000000 },
	{ "TSN",   Operator, 0611000000000 },
	{ "TDNE",  Operator, 0612000000000 },
	{ "TSNE",  Operator, 0613000000000 },
	{ "TDNA",  Operator, 0614000000000 },
	{ "TSNA",  Operator, 0615000000000 },
	{ "TDNN",  Operator, 0616000000000 },
	{ "TSNN",  Operator, 0617000000000 },
	{ "TRZ",   Operator, 0620000000000 },
	{ "TLZ",   Operator, 0621000000000 },
	{ "TRZE",  Operator, 0622000000000 },
	{ "TLZE",  Operator, 0623000000000 },
	{ "TRZA",  Operator, 0624000000000 },
	{ "TLZA",  Operator, 0625000000000 },
	{ "TRZN",  Operator, 0626000000000 },
	{ "TLZN",  Operator, 0627000000000 },
	{ "TDZ",   Operator, 0630000000000 },
	{ "TSZ",   Operator, 0631000000000 },
	{ "TDZE",  Operator, 0632000000000 },
	{ "TSZE",  Operator, 0633000000000 },
	{ "TDZA",  Operator, 0634000000000 },
	{ "TSZA",  Operator, 0635000000000 },
	{ "TDZN",  Operator, 0636000000000 },
	{ "TSZN",  Operator, 0637000000000 },
	{ "TRC",   Operator, 0640000000000 },
	{ "TLC",   Operator, 0641000000000 },
	{ "TRCE",  Operator, 0642000000000 },
	{ "TLCE",  Operator, 0643000000000 },
	{ "TRCA",  Operator, 0644000000000 },
	{ "TLCA",  Operator, 0645000000000 },
	{ "TRCN",  Operator, 0646000000000 },
	{ "TLCN",  Operator, 0647000000000 },
	{ "TDC",   Operator, 0650000000000 },
	{ "TSC",   Operator, 0651000000000 },
	{ "TDCE",  Operator, 0652000000000 },
	{ "TSCE",  Operator, 0653000000000 },
	{ "TDCA",  Operator, 0654000000000 },
	{ "TSCA",  Operator, 0655000000000 },
	{ "TDCN",  Operator, 0656000000000 },
	{ "TSCN",  Operator, 0657000000000 },
	{ "TRO",   Operator, 0660000000000 },
	{ "TLO",   Operator, 0661000000000 },
	{ "TROE",  Operator, 0662000000000 },
	{ "TLOE",  Operator, 0663000000000 },
	{ "TROA",  Operator, 0664000000000 },
	{ "TLOA",  Operator, 0665000000000 },
	{ "TRON",  Operator, 0666000000000 },
	{ "TLON",  Operator, 0667000000000 },
	{ "TDO",   Operator, 0670000000000 },
	{ "TSO",   Operator, 0671000000000 },
	{ "TDOE",  Operator, 0672000000000 },
	{ "TSOE",  Operator, 0673000000000 },
	{ "TDOA",  Operator, 0674000000000 },
	{ "TSOA",  Operator, 0675000000000 },
	{ "TDON",  Operator, 0676000000000 },
	{ "TSON",  Operator, 0677000000000 },

	{ "BLKI",  IoOperator, 0700000000000 },
	{ "BLKO",  IoOperator, 0700100000000 },
	{ "DATAI", IoOperator, 0700040000000 },
	{ "DATAO", IoOperator, 0700140000000 },
	{ "CONO",  IoOperator, 0700200000000 },
	{ "CONI",  IoOperator, 0700240000000 },
	{ "CONSZ", IoOperator, 0700300000000 },
	{ "CONSO", IoOperator, 0700340000000 },

	{ "JEN",   Operator, 0254500000000 },	/* JRST 12, */
	{ "HALT",  Operator, 0254200000000 },	/* JRST 4, */
	{ "JRSTF", Operator, 0254100000000 },	/* JRST 2, */
	{ "JOV",   Operator, 0255400000000 },	/* JFCL 10, */
	{ "JCRY0", Operator, 0255200000000 },	/* JFCL 4, */
	{ "JCRY1", Operator, 0255100000000 },	/* JFCL 2, */
	{ "JCRY",  Operator, 0255300000000 },	/* JFCL 6, */
	{ "JPC",   Operator, 0255040000000 },	/* JFCL 1, */
	{ "RSW", IoOperator, 0700040000000 },

	{ "", 0, 0 }
};
