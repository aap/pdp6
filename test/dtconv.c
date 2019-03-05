#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "../tools/pdp6common.h"

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

	DTSIZE = 922512
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

word datbuf[0200*01102];

void
prerecord(uchar *dtp)
{
	word i, n;
	int j;
	int ck;
	word *datp;

	writef(&dtp, TapeEndR, nil, 0);
	writef(&dtp, TapeEndR, nil, 0);

	datp = datbuf;
	for(i = 0; i < 01102; i++){
		n = 0;
		for(j = 0; j < 12; j++)
			n |= ((i>>j*3) & 07) << (11-j)*3;
		n ^= 0777777777777;

		writef(&dtp, BlockSpace, nil, (i >> 18) & 0777777);	// block mark
		writef(&dtp, BlockEndF, nil, i & 0777777);	// block mark
		writef(&dtp, DataSync, nil, 0);
		writef(&dtp, DataEndR, nil, 0);
		ck = 0;
		writef(&dtp, DataEndR, &ck, 0007777);	// rev check

		/* the data */
		writef(&dtp, DataEndR, &ck, left(*datp));
		writef(&dtp, DataEndR, &ck, right(*datp));
		datp++;
		for(j = 1; j < 127; j++){
			writef(&dtp, Data, &ck, left(*datp));
			writef(&dtp, Data, &ck, right(*datp));
			datp++;
		}
		writef(&dtp, DataEndF, &ck, left(*datp));
		writef(&dtp, DataEndF, &ck, right(*datp));
		datp++;
		writef(&dtp, DataEndF, nil, (ck & 077) << 12 | 07777);	// check
		writef(&dtp, DataEndF, nil, 0);
		writef(&dtp, BlockSync, nil, 0);
		writef(&dtp, BlockEndR, nil, (n >> 18) & 0777777);	// rev block mark
		writef(&dtp, BlockSpace, nil, n & 0777777);	// rev block mark
	}

	writef(&dtp, TapeEndF, nil, 0);
	writef(&dtp, TapeEndF, nil, 0);
}

int
main()
{
	FILE *dtf;
	uchar dtbuf[DTSIZE];
	int i, j;;
	word *datp;

	datp = datbuf;
	for(i = 0; i < 01102; i++)
		for(j = 0; j < 0200; j++)
			*datp++ = fw(i, j);

	prerecord(dtbuf);

	dtf = mustopen("out.dt6", "w");
	fwrite(dtbuf, 1, DTSIZE, dtf);
	fclose(dtf);
	return 0;
}
