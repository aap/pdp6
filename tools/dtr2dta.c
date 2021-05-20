#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define nil NULL
typedef unsigned char uchar;
typedef unsigned int uint;
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

	NUMBLOCKS = 01102,

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

/* Read raw tape data (forward) */
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

/* Write raw tape data (forwad */
void
writef(uchar **p, int mark, int *ck, int data)
{
	int i;
	uchar d;
	if(ck){
		*ck ^= ~data     & 077;
		*ck ^= ~data>>6  & 077;
		*ck ^= ~data>>12 & 077;
	}
	for(i = 0; i < 6; i++){
		if(mark)
			d = !!(mark & 040) << 3;
		else
			d = !!(**p & 010) << 3;
		if(data & 01000000)
			d |= **p & 07;
		else{
			d |= (data & 0700000) >> 15;
			data = (data << 3) & 0777777;
		}
		mark <<= 1;
		**p = d;
		(*p)++;
	}
}

#define LDB(p, s, w) ((w)>>(p) & (1<<(s))-1)
#define XLDB(ppss, w) LDB((ppss)>>6 & 077, (ppss)&077, w)
#define MASK(p, s) ((1<<(s))-1 << (p))
#define DPB(b, p, s, w) ((w)&~MASK(p,s) | (b)<<(p) & MASK(p,s))
#define XDPB(b, ppss, w) DPB(b, (ppss)>>6 & 077, (ppss)&077, w)

void
writesimh(FILE *f, word w)
{
	uchar c[8];
	uint l, r;

	r = LDB(0, 18, w);
	l = LDB(18, 18, w);
	c[0] = LDB(0, 8, l);
	c[1] = LDB(8, 8, l);
	c[2] = LDB(16, 8, l);
	c[3] = LDB(24, 8, l);
	c[4] = LDB(0, 8, r);
	c[5] = LDB(8, 8, r);
	c[6] = LDB(16, 8, r);
	c[7] = LDB(24, 8, r);
	fwrite(c, 1, 8, f);
}

word
readsimh(FILE *f)
{
	uchar c[8];
	word w[2];
	if(fread(c, 1, 8, f) != 8)
		return ~0;
	w[0] = c[3]<<24 | c[2]<<16 | c[1]<<8 | c[0];
	w[1] = c[7]<<24 | c[6]<<16 | c[5]<<8 | c[4];
	return (w[0]<<18 | w[1]) & 0777777777777;
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
	for(i = 0; i < NUMBLOCKS; i++){
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
dumpdta(uchar *dtp, FILE *f)
{
	uchar mark;
	word w, w2;
	int i, j;

	readf(&dtp, &mark, &w);
	readf(&dtp, &mark, &w);
	for(i = 0; i < NUMBLOCKS; i++){
		readf(&dtp, &mark, &w);
		readf(&dtp, &mark, &w2);
		w = w << 18 | w2;

		// data unused
		readf(&dtp, &mark, &w);
		readf(&dtp, &mark, &w2);
		w = w << 18 | w2;

		readf(&dtp, &mark, &w);
		for(j = 0; j < 128; j++){
			readf(&dtp, &mark, &w);
			readf(&dtp, &mark, &w2);
			writesimh(f, w<<18 | w2);
		}
		readf(&dtp, &mark, &w);

		// data unused
		readf(&dtp, &mark, &w);
		readf(&dtp, &mark, &w2);
		w = w << 18 | w2;

		// reverse block mark
		readf(&dtp, &mark, &w);
		readf(&dtp, &mark, &w2);
		w = w << 18 | w2;
	}
	readf(&dtp, &mark, &w);
	readf(&dtp, &mark, &w);
}

void
wrf(FILE *f, int mark, int *ck, int data)
{
	uchar buf[6], *p;
	p = buf;
	writef(&p, mark, ck, data);
	fwrite(buf, 1, 6, f);
};

void
dumpdtr(word *wp, word *bmp, word *ibp, FILE *f)
{
	word w;
	int i, j;
	int ck;

	for(i = 0; i < 2700; i++)
	{
		wrf(f, TapeEndR, nil, 0);
		wrf(f, TapeEndR, nil, 0);
	}

	for(i = 0; i < NUMBLOCKS; i++){
		w = *bmp++;
		wrf(f, BlockSpace, nil, LDB(18, 18, w));	// block mark
		wrf(f, BlockEndF, nil, LDB(0, 18, w));	// block mark

		w = *ibp++;
		wrf(f, DataSync, nil, LDB(18, 18, w));
		wrf(f, DataEndR, nil, LDB(0, 18, w));

		ck = 077;
//		wrf(f, DataEndR, nil, 0007777);	// rev check
		// this seems what 551 actually does
		wrf(f, DataEndR, nil, 0000000);	// rev check

		/* the data */
		w = *wp++;
		wrf(f, DataEndR, &ck, LDB(18, 18, w));
		wrf(f, DataEndR, &ck, LDB(0, 18, w));
		for(j = 1; j < 127; j++){
			w = *wp++;
			wrf(f, Data, &ck, LDB(18, 18, w));
			wrf(f, Data, &ck, LDB(0, 18, w));
		}
		w = *wp++;
		wrf(f, DataEndF, &ck, LDB(18, 18, w));
		wrf(f, DataEndF, &ck, LDB(0, 18, w));

		wrf(f, DataEndF, nil, (ck & 077) << 12 | 07777);	// check

		w = *ibp++;
		wrf(f, DataEndF, nil, LDB(18, 18, w));
		wrf(f, BlockSync, nil, LDB(0, 18, w));

		w = *bmp++;
		wrf(f, BlockEndR, nil, LDB(18, 18, w));	// rev block mark
		wrf(f, BlockSpace, nil, LDB(0, 18, w));	// rev block mark
	}

	for(i = 0; i < 2700; i++)
	{
		wrf(f, TapeEndF, nil, 0);
		wrf(f, TapeEndF, nil, 0);
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

uchar dtbuf[DTSIZE], *dtp;
word blockbuf[NUMBLOCKS*0200];
word bmbuf[NUMBLOCKS*2];
word ibbuf[NUMBLOCKS*2];

/* Fill block mark and interblock word buffers */
void
makefmt(void)
{
	int i, j;
	word co;

	for(i = 0; i < NUMBLOCKS; i++){
		/* forward and reverse block number */
		bmbuf[i*2] = i;
		// last one is 700707070707 really because it can't be written easily
		co = 0;
		for(j = 0; j < 12; j++)
			co |= ((bmbuf[i*2]>>j*3) & 07) << (11-j)*3;
		co ^= 0777777777777;
		bmbuf[i*2+1] = co;

		/* interblock words - used to find block 75 which contains the loader */

		/* forward word */
		if(i < 075)
			ibbuf[i*2] = 0721200220107;	// move forward
		else if(i == 075)
			ibbuf[i*2] = 0577777777777;	// nop
		else
			ibbuf[i*2] = 0721200233107;	// move backward

		/* reverse word */
		// last one is 777077707007 really because it can't be written easily
		if(i < 073)
			ibbuf[i*2+1] = 0721200223107;	// move forward
		else
			ibbuf[i*2+1] = 0721200230107;	// move backward
	}

	/* Last block isn't written quite correctly. To simulate: */
	// ibbuf[01101*2 + 0] = 0077070007000;
	// ibbuf[01101*2 + 1] = 0777077707007;
	// bmbuf[01101*2 + 1] = 0700707070707;
}

int
readdtr(FILE *f)
{
	fread(dtbuf, 1, DTSIZE, f);
	fclose(f);
	dtp = findstart(dtbuf);
	if(dtp == nil){
		fprintf(stderr, "no start\n");
		return 1;
	}
	if(!validate(dtp)){
		fprintf(stderr, "invalid file format\n");
		return 1;
	}
	return 0;
}

int
readdta(FILE *f)
{
	word w;
	int n;

	n = 0;
	while(w = readsimh(f), w != ~0 && n < NUMBLOCKS*0200)
		blockbuf[n++] = w;
	return n != NUMBLOCKS*0200;
}

int
dtr2dta(void)
{
	if(readdtr(stdin))
		return 1;
	dumpdta(dtp, stdout);
	return 0;
}

int
dta2dtr(void)
{
	if(readdta(stdin))
		return 1;
	makefmt();
	dumpdtr(blockbuf, bmbuf, ibbuf, stdout);
	return 0;
}

int
main(int argc, char *argv[])
{

	if(strstr(argv[0], "dtr2dta")) return dtr2dta();
	else if(strstr(argv[0], "dta2dtr")) return dta2dtr();
	else fprintf(stderr, "Unknown %s\n", argv[0]);

	return 0;
}
