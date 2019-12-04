#include "fe.h"
#include <unistd.h>
#include <fcntl.h>

#define MAXOPS 20

static char *lp;
static int numops;
static char *ops[MAXOPS];

static void
skipwhite(void)
{
	while(isspace(*lp))
		lp++;
}

static int
isdelim(char c)
{
	return c == '\0' || strchr(" \t\n;'\"", c) != nil;
}

/* tokenize lp into ops[numops] */
static void
splitops(void)
{
	char delim;
	char *p;

	numops = 0;
	for(; *lp; lp++){
		skipwhite();
		if(*lp == ';' || *lp == '\0')
			return;
		if(numops == MAXOPS){
			fprintf(stderr, "Too many arguments, ignored <%s>\n", lp);
			return;
		}
		if(*lp == '"' || *lp == '\''){
			delim = *lp++;
			ops[numops++] = p = lp;
			while(*lp && *lp != delim){
				if(*lp == '\\')
					lp++;
				*p++ = *lp++;
			}
			if(*lp == '\0') lp--;
			*p = '\0';
		}else{
			ops[numops++] = p = lp;
			while(!isdelim(*lp)){
				if(*lp == '\\')
					lp++;
				*p++ = *lp++;
			}
			if(*lp == '\0') lp--;
			*p = '\0';
		}
	}
}

void
loadsav(FILE *fp)
{
	word iowd;
	word w;
	while(w = readwbak(fp), w != ~0){
		if(w >> 27 == 0254){
		printf("PC: %o\r\n", right(w));
		fflush(stdout);
			cpu_setpc(right(w));
			started = 1;
			return;
		}
		iowd = w;
		while(left(iowd) != 0){
			iowd += 01000001;
			w = readwbak(fp);
			if(w == ~0)
				goto format;
			deposit(right(iowd), w);
		}
	}
format:
	printf("\r\nSAV format botch\r\n");
	fflush(stdout);
}

void
loadsblk(FILE *fp)
{
	word iowd, w, chk;
	int d;

	/* Skip RIM loader */
	while(w = readwits(fp), w != ~0)
		if(w == 0254000000001)
			goto sblk;
	goto format;
sblk:
	/* Read a simple block */
	while(w = readwits(fp), w != ~0){
		/* We expect an AOBJN word here */
		if((w & F0) == 0)
			goto end;
		iowd = w;
		chk = iowd;
		d = right(iowd) != 0;   /* 0 is symbol table */
		while(left(iowd) != 0){
			w = readwits(fp);
			if(w == ~0)
				goto format;

			chk = (chk<<1 | chk>>35) + w & FW;

			if(d){
				deposit(right(iowd), w);
			}
			iowd += 01000001;
		}
		if(readwits(fp) != chk)
			goto format;
	}
	goto format;
end:
	/* use the first JRST, not the second */
	//      w = readwits(fp);
	if(left(w) != 0324000 && left(w) != 0254000)
		goto format;
printf("PC: %o\r\n", right(w));
fflush(stdout);
	cpu_setpc(right(w));
	started = 1;
	return;

format:
	printf("\r\nSBLK format botch\r\n");
	fflush(stdout);
}

#define MAXBLK 01000

void
dumpsblk(FILE *fp, hword start, hword end)
{
	hword a;
	word w, iowd, chk;
	int last0;

	writewits(0254000000001, fp);
	for(a = start; a <= end; /*a++*/){
		/* look for beginning of block */
		w = examine(a);
		if(w == 0){
			a++;
			continue;
		}

		/* look for end of block */
		iowd = a;
		last0 = 0;
		for(; a <= end; a++){
			if(a - iowd >= MAXBLK)
				break;

			w = examine(a);
			if(w == 0){
				if(last0){
					a--;
					break;
				}
				last0 = 1;
			}else{
				last0 = 0;
			}
		}
//printf("%lo - %o\r\n", iowd, a);

		/* write block */
		iowd |= (01000000 - (a-iowd)) << 18;
		writewits(iowd, fp);
		chk = iowd;
//printf("%lo\r\n", iowd);

		while(left(iowd) != 0){
			w = examine(right(iowd));
			writewits(w, fp);
			chk = (chk<<1 | chk>>35) + w & FW;
//printf(" %lo\r\n", w);

			iowd += 01000001;
		}
		writewits(chk, fp);
	}
	w = 0324000000000 | starta;
	writewits(w, fp);
	writewits(w, fp);

	writewits(~0, fp);	// flush
//fflush(stdout);
}

static void
printops(void)
{
	int i;
	for(i = 0; i < numops; i++)
		printf("<%s> ", ops[i]);
	printf("\r\n");
	fflush(stdout);
}

void
c_dump(int argc, char *argv[])
{
	FILE *f;
	if(argc < 2)
		return;
	f = fopen(ops[1], "wb");
	if(f == nil)
		err("?F? ");
	dumpsblk(f, 020, MAXMEM-1);
	fclose(f);
}

void
c_load(int argc, char *argv[])
{
	FILE *f;
	if(argc < 2)
		return;
	f = fopen(ops[1], "rb");
	if(f == nil)
		err("?F? ");
	loadsblk(f);
	fclose(f);
}

void
c_loadsav(int argc, char *argv[])
{
	FILE *f;
	if(argc < 2)
		return;
	f = fopen(ops[1], "rb");
	if(f == nil)
		err("?F? ");
	loadsav(f);
	fclose(f);
}

struct dev devtab[] = {
	{ "ptr", O_RDONLY, -1, nil },
	{ "ptp", O_WRONLY | O_CREAT | O_TRUNC, -1, nil },
	nil, 0, 0
};

struct dev*
finddev(char *str)
{
	struct dev *d;
	for(d = devtab; d->devname; d++)
		if(strcasecmp(d->devname, str) == 0)
			return d;
	return nil;
}

void
c_mount(int argc, char *argv[])
{
	char *devstr, *path;
	struct dev *dev;

	if(argc == 1){
		/* with no arguments, print mounts */
		for(dev = devtab; dev->devname; dev++)
			if(dev->fd >= 0)
				printf("%s: %s\r\n", dev->devname, dev->path);
		return;
	}

	if(argc < 3)
		err("?A? ");
	devstr = argv[1];
	path = argv[2];

	dev = finddev(devstr);
	if(dev == nil)
		err("?D? ");
	if(dev->fd >= 0){
		close(dev->fd);
		dev->fd = -1;
		free(dev->path);
		dev->path = nil;
	}
	dev->fd = open(path, dev->mode);
	if(dev->fd < 0)
		err("?F? ");
	dev->path = strdup(path);
}

void
c_unmount(int argc, char *argv[])
{
	char *devstr;
	struct dev *dev;

	if(argc < 2)
		err("?A? ");
	devstr = argv[1];

	dev = finddev(devstr);
	if(dev == nil)
		err("?D? ");
	if(dev->fd >= 0){
		close(dev->fd);
		dev->fd = -1;
		free(dev->path);
		dev->path = nil;
	}
}

struct {
	char *cmd;
	void (*f)(int, char **);
} cmdtab[] = {
	{ "load", c_load },
	{ "loadsav", c_loadsav },
	{ "dump", c_dump },
	{ "mount", c_mount },
	{ "unmount", c_unmount },
	{ nil, nil }
};

void
coloncmd(char *line)
{
	int i;

	lp = line;
	splitops();
	if(numops < 1)
		return;

	for(i = 0; cmdtab[i].cmd; i++)
		if(strcasecmp(ops[0], cmdtab[i].cmd) == 0){
			cmdtab[i].f(numops, ops);
			return;
		}
	err("?? ");
}

void
docmd(char *cmd, char *line)
{
	char buf[256];
	sprintf(buf, "%s %s", cmd, line);
	coloncmd(buf);
}

char *helpstr =
"\n\
?         print help\n\
◊?        print colon command help\n\
◊D        clear input\n\
◊G        start at start address\n\
<loc>◊G   start at <loc>\n\
<loc>◊◊G  set start address to <loc> and start\n\
◊R\n\
<loc>◊R\n\
<loc>◊◊R  like their G counterparts but read-in instead of start\n\
↑Z        instruction stop\n\
◊↑Z       memory stop\n\
◊P        continue\n\
↑N        instruction step\n\
◊↑N       memory step\n\
<inst>◊X  execute instruction inst\n\
◊I        IO reset\n\
◊Q        last quantity\n\
+         addition operator\n\
-         subtraction operator\n\
*         multiplication operator\n\
|         division operator\n\
SPACE     assembly field separator\n\
,         assembly field separator\n\
(word)    swap word and add to current full word\n\
X(word)   where X is an operator. parenthesize expression\n\
/         open\n\
\\         open, don't set .\n\
[         open, constant typeout\n\
]         open, symbolic typeout\n\
!         open, no typeout\n\
TAB       deposit, open\n\
^         deposit, open previous\n\
↑J        deposit, open next\n\
↑M        deposit, close\n\
_         typeout symbolic\n\
=         typeout constant\n\
\"         typeout ASCII\n\
'         typeout SIXBIT\n\
&         typeout SQUOZE\n\
◊S ◊◊S    symbolic mode\n\
◊C ◊◊C    constant mode\n\
◊\" ◊◊\"    ASCII mode\n\
◊' ◊◊'    SIXBIT mode\n\
◊& ◊◊&    SQUOZE mode\n\
◊H ◊◊H    half word mode\n\
◊L <file> load binary file\n\
◊Y <file> dump to binary file\n\
";

char *colhelpstr =
"\n\
:load <file>           load binary file\n\
:dump <file>           dump to binary file\n\
:mount <dev> <file>    mount file on device\n\
:unmount <dev>         unmount file on device\n\
";
