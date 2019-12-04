#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <setjmp.h>

#include "fe.h"

struct termios tiosaved;
char erasec, killc, intrc;
jmp_buf errjmp;

int
raw(int fd)
{
	struct termios tio;
	if(tcgetattr(fd, &tio) < 0) return -1;
	tiosaved = tio;
	cfmakeraw(&tio);
	if(tcsetattr(fd, TCSAFLUSH, &tio) < 0) return -1;
	return 0;
}

int
reset(int fd)
{
	if(tcsetattr(0, TCSAFLUSH, &tiosaved) < 0) return -1;
	return 0;
}

#define ALT 033

char
tyi(void)
{
	char c;
	read(0, &c, 1);
	return c;
}

#define CTL(c) ((c) & 037)

void
tyo_(char c)
{
	write(1, &c, 1);
}

void
tyo(char c)
{
	static char *alt = "◊";
	static char *ctrl = "↑";

	if(c < 040){
		switch(c){
		case ALT:
			write(1, alt, strlen(alt));
			return;
		case CTL('J'):
			write(1, &c, 1);
			c = CTL('M');
		case CTL('M'):
		case CTL('H'):
		case CTL('I'):
			write(1, &c, 1);
			break;
		default:
			write(1, ctrl, strlen(ctrl));
			c += 0100;
			write(1, &c, 1);
		}
	}else
		write(1, &c, 1);
}

#define ADDRMASK 03777777

enum {
	BUF_ERR = ~0,
	BUF_EMPTY = ~0-1,

	/* typeout modes */
	MODE_NONE = 0,
	MODE_NUM,
	MODE_HALF,
	MODE_SYM,
	MODE_ASCII,
	MODE_SIXBIT,
	MODE_SQUOZE,
	MODE_FLOAT,

	/* flags */
	CF = 1,		// one altmode
	CCF = 2,	// two altmodes

	/* fields */
	F_A = 1,
	F_I = 2,
	F_AC = 4,
	F_FW = 8,

	/* line modes */
	LM_COLON = 1,
	LM_LOAD,
	LM_DUMP,
	LM_MOUNT,
	LM_UNMOUNT
};

#define BUFLEN 200

char buf[BUFLEN+1];
char *bufstart;
int nbuf;

int flags;
char ch;
int base = 8;
hword dot;
hword addr;
int opened;
word q;
word number;
int hasnum;
int permmode = MODE_SYM;
int curmode;
struct Assembler
{
	word fields[4];
	int f;

	word exp;
	int op;
	int gotexp;
};
struct Assembler as;
struct Assembler astack[8];
int asp;
hword starta;
int linemode;

void
typenum(word n)
{
	word d;
	d = n % base;
	n /= base;
	if(n)
		typenum(n);
	tyo(d + '0');
}

void
typestr(char *str)
{
	while(*str)
		tyo(*str++);
}

void
typestrnl(char *str)
{
	while(*str){
		if(*str == '\n')
			tyo('\r');
		tyo(*str++);
	}
}

void
typestrnl_(char *str)
{
	while(*str){
		if(*str == '\n')
			tyo_('\r');
		tyo_(*str++);
	}
}

void
err(char *str)
{
	typestr(str);
	longjmp(errjmp, 0);
}

void
num(void)
{
	number = number*base + ch-'0';
	hasnum = 1;
}

void
ins(void)
{
	buf[nbuf++] = ch;
	nbuf %= BUFLEN;
	buf[nbuf] = '\0';
}

word
parsenum(void)
{
	word w;
	char *c;

	w = 0;
	for(c = bufstart; c < &buf[nbuf]; c++){
		if(*c >= '0' && *c <= '9')
			w = w*base + *c-'0';
		else
			return BUF_ERR;
		w &= 0777777777777;
	}
	return w;
}

typedef struct Symbol Symbol;
struct Symbol
{
	char *sym;
	word value;
};

Symbol symtab[] = {
	{ "SM", 01000000 },
	{ "LED", 01001000 },
	{ "SW", 01001001 },

	/* some more instructions */
	{ "HALT", 0254200000000 },

	/* devices, decorated with % */
	{ "%APR", 0 },
	{ "%PI", 4 },
	{ "%PTP", 0100 },
	{ "%PTR", 0104 },
	{ "%TTY", 0120 },
	{ "%DIS", 0130 },
	{ "%DC", 0200 },
	{ "%UTC", 0204 },
	{ "%UTS", 0210 },

#define X(str, name) { str, name },
PDP_REGS
PDP_REGS_TEST
#undef X

	{ nil, 0 },
};

// for disasm
char *findacsym(word v) { return nil; }
char *findsymval(word v) { return nil; }

word
parsesym(void)
{
	word i;

	if(strcmp(bufstart, ".") == 0)
		return dot;

	for(i = 0; symtab[i].sym; i++)
		if(strcasecmp(bufstart, symtab[i].sym) == 0)
			return symtab[i].value;

	for(i = 0; i < 0700; i++)
		if(mnemonics[i] && strcasecmp(mnemonics[i], bufstart) == 0)
			return i << 27;
	for(i = 0; i < 8; i++)
		if(strcasecmp(iomnemonics[i], bufstart) == 0)
			return (word)0700<<27 | i<<23;
	return BUF_ERR;
}

void
trimbuf(void)
{
	bufstart = buf;
	while(isspace(*bufstart)) bufstart++;
	while(&buf[nbuf] > bufstart && isspace(buf[nbuf-1])) nbuf--;
	buf[nbuf] = '\0';
}

/* parse input buffer, error if can't parse
 * return 0 if empty */
int
parse(word *t)
{
	word w;

	// this should be unnecessary now
	trimbuf();

	w = parsenum();
	if(bufstart >= &buf[nbuf])
		return 0;
	if((w = parsenum()) == BUF_ERR && (w = parsesym()) == BUF_ERR)
		err("?U? ");
	nbuf = 0;
	*t = w;
	return 1;
}

void
resetexp(void)
{
	as.fields[0] = 0;
	as.fields[1] = 0;
	as.fields[2] = 0;
	as.fields[3] = 0;
	as.f = 0;

	as.exp = 0;
	as.op = 0;
	as.gotexp = 0;
}

void
pushexp(void)
{
	astack[asp] = as;
	asp++;
	if(asp >= 8)
		err(" XXX ");
	resetexp();
}

void
popexp(void)
{
	if(asp < 0)
		err(" XXX ");
	asp--;
	as = astack[asp];
}

/* combine word t into current expression */
void
combine(word t)
{
	switch(as.op){
	case '+':
	default:
		as.exp += t;
		break;
	case '-':
		as.exp -= t;
		break;
	case '*':
		as.exp *= t;
		break;
	case '|':
		as.exp /= t;
		break;
	}
	as.gotexp = 1;
	as.op = 0;
}

/* assemble storage word and set *w
 * if no word, don't change *w and return 0 */
int
endword(word *w)
{
	word t;

	if(!parse(&t)){
		if(!as.gotexp)
			return 0;
		t = 0;
	}
	combine(t);
	if(as.f & (F_FW|F_AC)){
		// add to address field
		as.fields[3] += as.exp;
		as.f |= F_A;
	}else{
		as.fields[0] += as.exp;
		as.f |= F_FW;
	}
	t = as.fields[0];
	if(as.f & F_AC){
		if((t & 0700000000000) == 0700000000000)
			t += (as.fields[1]&0774)<<24;
		else
			t += (as.fields[1]&017)<<23;
	}
	if(as.f & F_I)
		if((t & 020000000) == 0)
			t += as.fields[2]&020000000;
	if(as.f & F_A)
		t = (t&LT) + ((t+as.fields[3])&RT);
	t &= LT|RT;
	*w = t;
	resetexp();
	return 1;
}

void
prword(int mode, word wd)
{
	int i;
	char c;
	char s[7];
	double f;

	switch(mode){
	case MODE_ASCII:
		tyo(ALT);
		tyo('0' + (wd&1));
		tyo('"');
		for(i = 0; i < 5; i++){
			c = (wd >> ((7*(4-i))+1)) & 0177;
			if(c < 040 || c == 0177){
				tyo('^');
				tyo(c ^ 0100);
			}else
				tyo(c);
		} 
		tyo(ALT);
		break;

	case MODE_SIXBIT:
		tyo(ALT);
		tyo('1');
		tyo('\'');
		for(i = 0; i < 6; i++){
			c = (wd >> (6*(5-i))) & 077;
			tyo(c + 040);
		} 
		tyo(ALT);
		break;

	case MODE_SQUOZE:
		tyo(ALT);
		typenum(unrad50(wd, s));
		tyo('&');
		typestr(s);
		tyo(ALT);
		break;

		break;

	case MODE_SYM:
		typestr(disasm(wd));
		break;

	case MODE_HALF:
		tyo('(');
		typenum(wd&0777777);
		tyo(')');
		typenum((wd>>18)&0777777);
		break;

	case MODE_FLOAT:
		f = pdptod(wd);
		printf("%lf", f);
		fflush(stdout);
		break;

	default:
		typenum(wd);
	}
}

void
typeout(int mode)
{
	endword(&q);
	prword(mode, q);
	typestr("   ");
}

void
propen(hword a)
{
	prword(curmode, a);
	tyo('/');
}

void
aclose(int ins)
{
	// this is a bit ugly...
	// we don't want ^M to set Q if no location is open
	word t;
	t = q;
	if(endword(&q) && opened && ins)
		deposit(addr, q);
	addr = q & ADDRMASK;
	if(!opened)
		q = t;
	opened = 0;
}

void
aopen(int mode)
{
	opened = 1;
	q = examine(addr);
	typestr("   ");
	if(mode != MODE_NONE)
		typeout(mode);
}

void
modechange(int mode)
{
	curmode = mode;
	if(flags & CCF){
		permmode = mode;
		typestr("   ");
	}
}

void
runline(void)
{
	/* TODO: this can be improved */
	switch(linemode){
	case LM_COLON:
		coloncmd(buf);
		break;
	case LM_LOAD:
		docmd("load", buf);
		break;
	case LM_DUMP:
		docmd("dump", buf);
		break;
	case LM_MOUNT:
		docmd("mount", buf);
		break;
	case LM_UNMOUNT:
		docmd("unmount", buf);
		break;
	}
}

void
zerocore(void)
{
	hword a;
	for(a = 0; a < MAXMEM; a++)
		deposit(a, 0);
	typestr("\r\n");
}

void
quit(void)
{
	reset(0);
	putchar('\n');

	deinit6();
	exit(0);
}

int started;

int
threadmain(int argc, char *argv[])
{
	char chu;
	word t;

	init6();
	initcrt("soma");
//	initnetmem("10.0.0.222", 10006);

	raw(0);
	erasec = tiosaved.c_cc[VERASE];
	killc = tiosaved.c_cc[VKILL];
	intrc = tiosaved.c_cc[VINTR];

	setjmp(errjmp);
	nbuf = 0;
	opened = 0;
	resetexp();
	flags = 0;
	curmode = permmode;
	number = 0;
	hasnum = 0;
	linemode = 0;

	for(;;){
		if(hasinput(0)){
			ch = tyi();
		}else{
			if(started && (!isrunning() || isstopped())){
				t = examine(APR_PC);
				/* TODO: maybe do something different on stop? */
				typestrnl("\n");
				typenum(t);
				if(!isrunning())
					typestr(">>");
				else
					typestr("<<");
				t = examine(t);
				prword(MODE_SYM, t);
				typestr("   ");

				/* show AC or E */
				if((t&0700000000000) == 0700000000000){
					t = t&RT;
				}else{
					t = t>>23 & 017;
				}
				dot = t;
				prword(MODE_SYM, dot);
				typestr("/   ");
				t = examine(t);
				prword(MODE_SYM, t);
				q = t;
				typestr("    ");
				
				started = 0;
			}
			fe_svc();
		//	usleep(1000);
			continue;
		}

		chu = toupper(ch);
		if(ch == erasec || ch == CTL('H') || ch == CTL('?')){
			/* can't backspace in all cases */
			if((flags & CF) || nbuf <= 0)
				err("\r\n");
			typestr("\b \b");
			nbuf--;
		}else if(ch == killc)
			err("");
		else if(ch == intrc)
			break;
		else if(linemode){
			if(ch == '\r' || ch == '\n'){
				typestrnl("\n");
				runline();
				err("");
			}else{
				tyo(ch);
				ins();
			}
		}else{
			tyo(ch);

			if((flags & CF) == 0 &&
			   (ch >= 'A' && ch <= 'Z' ||
			    ch >= 'a' && ch <= 'z')){
				ins();
			}else switch(chu){
			case ':':
				linemode = LM_COLON;
				nbuf = 0;
				break;
			case 'L':
				tyo(' ');
				linemode = LM_LOAD;
				nbuf = 0;
				break;
			case 'Y':
				tyo(' ');
				linemode = LM_DUMP;
				nbuf = 0;
				break;

			case '?':
				typestrnl_(flags & CF ? colhelpstr : helpstr);
				break;

			case CTL('D'):
				err("XXX? ");
				break;

			case '.': case '%': case '$':
				ins();
				break;

			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
			case '8': case '9':
				if(flags & CF){
					num();
					goto keepflags;
				}
				ins();
				break;

			case 'F':
				cpu_printflags();
				break;

			case 'G':
				if(!endword(&t))
					t = starta;
				if(flags & CCF)
					starta = t;
				if(hasnum && number == 0)
					cpu_setpc(t);
				else
					cpu_start(t);
				started = 1;
				break;

			case 'R':
				if(!endword(&t))
					t = starta;
				if(flags & CCF)
					starta = t;
				cpu_readin(t);
				started = 1;
				break;

			case CTL('Z'):
				nbuf = 0;
				if(flags & CF)
					cpu_stopmem();
				else
					cpu_stopinst();
				break;

			case 'P':
				endword(&t);
				cpu_cont();
				started = 1;
				break;

			case CTL('N'):
				endword(&t);
				if(flags & CF)
					cpu_nextmem();
				else
					cpu_nextinst();
				started = 1;
				break;

			case 'X':
				if(!endword(&t))
					t = q;
				cpu_exec(t);
				typestrnl("\n");
				break;

			case 'I':
				nbuf = 0;
				cpu_ioreset();
				break;

			case 'Q':
				combine(q);
				break;

			case 'Z':
				if(flags & CCF)
					zerocore();
				else
					err(" ?? ");
				break;

			case ALT:
				if(flags & CF)
					flags |= CCF;
				flags |= CF;
				goto keepflags;

			/* expressions */
			case '+':
			case '-':
			case '*':
			case '|':
				if(!parse(&t)){
					if(ch == '-')
						as.op = '-';
					break;
				}
				combine(t);
				as.op = ch;
				break;
			case '(':
				if(parse(&t))
					combine(t);
				pushexp();
				break;
			case ')':
				t = 0;
				endword(&t);
				popexp();
				if(as.op == 0){
					t = (t&LT)>>18 | (t&RT)<<18;
					as.fields[0] += t;
					as.gotexp = 1;
				}else{
					combine(t);
				}
				as.op = 0;
				break;

			/* word assembly */
			case ' ':
				if(!parse(&t))
					break;
				combine(t);
				if(as.f & F_FW){
					// add to address field
					// if we have a full word already
					as.fields[3] += as.exp;
					as.f |= F_A;
				}else{
					as.fields[0] += as.exp;
					as.f |= F_FW;
				}
				as.exp = 0;
				break;
			case ',':
				if(!parse(&t))
					break;
				combine(t);
				if(as.f & F_AC){
					// add to address field
					// if we have an AC already
					as.fields[3] += as.exp;
					as.f |= F_A;
				}else{
					as.fields[1] += as.exp;
					as.f |= F_AC;
				}
				as.exp = 0;
				break;
			case '@':
				as.fields[2] += 020000000;
				as.f |= F_I;
				as.gotexp = 1;
				break;

			/* opening */
			case '/':
				aclose(0);
				dot = addr;
				aopen(curmode);
				break;
			case '\\':
				aclose(0);
				aopen(curmode);
				break;
			case '[':
				if(flags & CF){
					tyo(' ');
					linemode = LM_MOUNT;
					nbuf = 0;
					break;
				}
				aclose(0);
				dot = addr;
				aopen(MODE_NUM);
				break;
			case ']':
				if(flags & CF){
					tyo(' ');
					linemode = LM_UNMOUNT;
					nbuf = 0;
					break;
				}
				aclose(0);
				dot = addr;
				aopen(MODE_SYM);
				break;
			case '!':
				aclose(0);
				dot = addr;
				aopen(MODE_NONE);
				break;

			case CTL('I'):
				typestrnl("\n");
				aclose(1);
				dot = addr;
				propen(addr);
				aopen(curmode);
				break;
			case '^':
				typestrnl("\n");
				aclose(1);
				dot = (dot-1) & ADDRMASK;
				addr = dot;
				propen(addr);
				aopen(curmode);
				break;
			case CTL('J'):
				tyo('\r');
				aclose(1);
				dot = (dot+1) & ADDRMASK;
				addr = dot;
				propen(addr);
				aopen(curmode);
				break;

			case CTL('M'):
				tyo('\n');
				aclose(1);
				curmode = permmode;
				break;

			/* typeout */
			case '_':
				typeout(MODE_SYM);
				break;
			case '=':
				typeout(MODE_NUM);
				break;
			case ';':
				typeout(MODE_FLOAT);
				break;
			case '"':
				if(flags & CF)
					modechange(MODE_ASCII);
				else
					typeout(MODE_ASCII);
				break;
			case '\'':
				if(flags & CF)
					modechange(MODE_SIXBIT);
				else
					typeout(MODE_SIXBIT);
				break;
			case '&':
				if(flags & CF)
					modechange(MODE_SQUOZE);
				else
					typeout(MODE_SQUOZE);
				break;
			case 'S':
				modechange(MODE_SYM);
				break;
			case 'C':
				modechange(MODE_NUM);
				break;
			case 'H':
				modechange(MODE_HALF);
				break;

			default:
				err(" ?? ");
				break;
			}
		}
		flags = 0;
		number = 0;
		hasnum = 0;
keepflags:;
	}

	quit();

	return 0;
}
