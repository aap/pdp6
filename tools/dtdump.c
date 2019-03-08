#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define nil NULL
typedef unsigned char uchar;
typedef uint64_t word;

enum
{
	TapeEndF  = 022,
	TapeEndR  = 055,

	BlockSpace = 025,

	BlockEndF = 026,
	BlockEndR = 045,

	DataSync  = 032,	/* rev guard */
	BlockSync = 051,	/* guard */

	DataEndF  = 073,	/* pre-final, final, ck, rev lock */
	DataEndR  = 010,	/* lock, rev ck, rev final, rev pre-final */

	Data = 070,

//	DTSIZE = 922512
	// 01102 blocks, 2700 word for start/end each + safe zone
	DTSIZE = 987288 + 1000
};

FILE*
mustopen(const char *name, const char *mode)
{
	FILE *f;
	if(f = fopen(name, mode), f == nil){
		fprintf(stderr, "couldn't open file: %s\n", name);
		exit(1);
	}
	return f;
}

void
readf(uchar **dtp, uchar *mark, word *wrd)
{
	int i;
	uchar l;
	uchar m;
	word w;

	m = 0;
	w = 0;
	for(i = 0; i < 6; i++){
		l = *(*dtp)++;
		m = (m << 1) | !!(l&010);
		w = (w << 3) | l&7;
	}
	*mark = m;
	*wrd = w;
}

char*
tostr(int m)
{
	return m == TapeEndF ? "End Zone Fwd" :
		m == TapeEndR ? "End Zone Rev" :
		m == BlockSpace ? "Block Space" :
		m == BlockEndF ? "Block End Fwd" :
		m == BlockEndR ? "Block End Rev" :
		m == DataSync ? "Data Sync" :
		m == BlockSync ? "Block Sync" :
		m == DataEndF ? "Data End Fwd" :
		m == DataEndR ? "Data End Rev" :
		m == Data ? "Data" :
			"Unknown";
}

void
rawdump(uchar *dtp)
{
	uchar mark;
	word w;
	uchar *end;
	end = dtp + DTSIZE;
	while(dtp < end){
		readf(&dtp, &mark, &w);
		printf("%02o[%-13s] %06o\n", mark, tostr(mark), w);
	}
}

#define CHKWORD(exp) \
	readf(&dtp, &mark, &w); \
	if(mark != exp){ \
		fprintf(stderr, "expected %s(%02o) got %s(%02o)\n", \
			tostr(exp), exp, tostr(mark), mark); \
		return 0; \
	} \

int
validate(uchar *dtp)
{
	uchar mark;
	word w;
	int i, j;
	
	CHKWORD(TapeEndR);
	CHKWORD(TapeEndR);
	for(i = 0; i < 01102; i++){
		CHKWORD(BlockSpace);
		CHKWORD(BlockEndF);
		CHKWORD(DataSync);
		CHKWORD(DataEndR);
		CHKWORD(DataEndR);
		CHKWORD(DataEndR);
		CHKWORD(DataEndR);
		for(j = 1; j < 127; j++){
			CHKWORD(Data);
			CHKWORD(Data);
		}
		CHKWORD(DataEndF);
		CHKWORD(DataEndF);
		CHKWORD(DataEndF);
		CHKWORD(DataEndF);
		CHKWORD(BlockSync);
		CHKWORD(BlockEndR);
		CHKWORD(BlockSpace);
	}
	CHKWORD(TapeEndF);
	CHKWORD(TapeEndF);
	return 1;
}

void
dumpf(uchar *dtp)
{
	uchar mark;
	word w, w2;
	int i, j;

	readf(&dtp, &mark, &w);
	readf(&dtp, &mark, &w);
	for(i = 0; i < 01102; i++){
		readf(&dtp, &mark, &w);
		readf(&dtp, &mark, &w2);
		w = w << 18 | w2;
		printf("Block %012lo\n", w);

		// data unused
		readf(&dtp, &mark, &w);
		readf(&dtp, &mark, &w2);
		w = w << 18 | w2;
		printf("InterBlock1 %012lo\n", w);

		readf(&dtp, &mark, &w);
		printf("	Rev Check: %06lo\n", w);
		for(j = 0; j < 128; j++){
			readf(&dtp, &mark, &w);
			readf(&dtp, &mark, &w2);
			printf("	%o/%o %012lo\n", i, j, w << 18 | w2);
		}
		readf(&dtp, &mark, &w);
		printf("	Check: %06lo\n", w);

		// data unused
		readf(&dtp, &mark, &w);
		readf(&dtp, &mark, &w2);
		w = w << 18 | w2;
		printf("InterBlock1 %012lo\n", w);

		// reverse block mark
		readf(&dtp, &mark, &w);
		readf(&dtp, &mark, &w2);
		w = w << 18 | w2;
		printf("RevBlock %012lo\n", w);
	}
	readf(&dtp, &mark, &w);
	readf(&dtp, &mark, &w);
}

void
markdump(uchar *buf)
{
	word w;
	uchar m;
	int i;

	i = 1000;
	while(i--){	
		readf(&buf, &m, &w);
		printf("%02o\n", m);
	}
}

uchar*
findstart(uchar *buf)
{
	uchar *t;
	int i;
	word w;
	uchar m;

	for(i = 0; i < 1000; i++){
		t = buf;
		readf(&t, &m, &w);
		if(m == TapeEndR)
			goto found;
		buf++;
	}
	return nil;
found:
	while(readf(&buf, &m, &w), m == TapeEndR);
	return buf-18;
}

int
main(int argc, char *argv[])
{
	char *filename = "out.dt6";
	FILE *dtf;
	uchar dtbuf[DTSIZE], *dtp;

	if(argc > 1)
		filename = argv[1];
	dtf = mustopen(filename, "r");
	fread(dtbuf, 1, DTSIZE, dtf);
	fclose(dtf);

	dtp = findstart(dtbuf);
	if(dtp == nil){
		printf("no start\n");
		return 1;
	}

//	markdump(dtp);

	if(!validate(dtp))
		return 1;
	dumpf(dtp);
//	rawdump(dtp);

	return 0;
}
