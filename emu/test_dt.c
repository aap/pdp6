#include "pdp6.h"

Dc136 *dc;
Dt551 *dt;
Dx555 *dx;
IOBus iobus;
IOBus *bus = &iobus;
int pireq;
Task dttask;

void
setreq(IOBus *bus, int dev, u8 pia)
{
	if(dev == DC)
		pireq = pia;
}

void
addtask(Task t)
{
	dttask = t;
}


void
wakedev(IOBus *bus, int d)
{
	Busdev *dev;
	bus->devcode = d;
	dev = &bus->dev[d];
	if(dev->wake)
		dev->wake(dev->dev);
}

void
bus_reset(IOBus *bus)
{
	int d;
	bus->c34 |= IOBUS_IOB_RESET;
	for(d = 0; d < 128; d++)
		wakedev(bus, d);
	bus->c34 &= ~IOBUS_IOB_RESET;
}

void
cono(IOBus *bus, int d, hword w)
{
	bus->c34 |= IOBUS_CONO_CLEAR;
	wakedev(bus, d);
	bus->c34 &= ~IOBUS_CONO_CLEAR;

	bus->c34 |= IOBUS_CONO_SET;
	bus->c12 = w;
	wakedev(bus, d);
	bus->c34 &= ~IOBUS_CONO_SET;
	bus->c12 = 0;
}

void
datao(IOBus *bus, int d, word w)
{
	bus->c34 |= IOBUS_DATAO_CLEAR;
	wakedev(bus, d);
	bus->c34 &= ~IOBUS_DATAO_CLEAR;

	bus->c34 |= IOBUS_DATAO_SET;
	bus->c12 = w;
	wakedev(bus, d);
	bus->c34 &= ~IOBUS_DATAO_SET;
	bus->c12 = 0;
}

word
coni(IOBus *bus, int d)
{
	word w;
	bus->c34 |= IOBUS_IOB_STATUS;
	wakedev(bus, d);
	bus->c34 &= ~IOBUS_IOB_STATUS;
	w = bus->c12;
	bus->c12 = 0;
	return w;
}

word
datai(IOBus *bus, int d)
{
	word w;
	bus->c34 |= IOBUS_IOB_DATAI;
	wakedev(bus, d);
	bus->c34 &= ~IOBUS_IOB_DATAI;
	w = bus->c12;
	bus->c12 = 0;
	return w;
}

void
printdc(Dc136 *dc)
{
	printf("DB/%012lo DA/%012lo CCT/%o SCT/%o CHMOD/%o\n",
		dc->db, dc->da, dc->cct, dc->sct, dc->ch_mode);
	printf(" CLBD/%o DBDAMOVE/%o DARQ/%o DBRQ/%o INOUT/%o DEV/%o PIA/%o\n",
		dc->data_clbd, dc->dbda_move, dc->da_rq, dc->db_rq,
		dc->inout, dc->device, dc->pia);
}

// DC controls
enum {
	CLBD = 010000,

	DBDAMOVE = 04000,
	DARQ = 02000,
	DBRQ = 01000,

	IN = 0000,
	OUT = 0400,

	CHMOD_6 = 0000,
	CHMOD_36 = 0100,
	CHMOD_12 = 0200,
	CHMOD_18 = 0300,

	DEV1 = 010,
	DEV2 = 020,
	DEV3 = 030,
	DEV4 = 040,
	DEV5 = 050,
	DEV6 = 060,
};

enum {
	SEL    = 0200000,
	GO     = 0020000,
	FWD    = 0000000,
	REV    = 0010000,
	DLY0   = 0000000,
	DLY1   = 0001000,
	DLY2   = 0002000,
	DLY3   = 0003000,
	FN_DN  = 0000000,
	FN_RA  = 0000100,
	FN_RBN = 0000200,
	FN_RD  = 0000300,
	FN_WTM = 0000400,
	FN_WA  = 0000500,
	FN_WBN = 0000600,
	FN_WD  = 0000700
};

word writebuf[133*01102 + 6000];
word *bufp;
int bufsz;

void
fillbuf(int n)
{
	bufp = writebuf;
	bufsz = n;
	while(n--)
		bufp[n] = n | 0112200000000;
}

void
fillbuf_fmt(void)
{
	int i, j;
	bufp = writebuf;

	for(i = 0; i < 6; i++)
		*bufp++ = 0707707707707;	// 55 55

	for(i = 0; i < 01102; i++){
		*bufp++ = 0070707070770;	// 25 26
		*bufp++ = 0077070007000;	// 32 10
		*bufp++ = 0007000007000;	// 10 10
		*bufp++ = 0007000777000;	// 10 70

		for(j = 0; j < 125; j++)
			*bufp++ = 0777000777000;	// 70 70

		*bufp++ = 0777000777077;	// 70 73
		*bufp++ = 0777077777077;	// 73 73
		*bufp++ = 0777077707007;	// 73 51
		*bufp++ = 0700707070707;	// 45 25
	}

	for(i = 0; i < 6; i++)
		*bufp++ = 0070070070070;	// 22 22

	bufsz = bufp - writebuf;
	bufp = writebuf;
}

void
fwdtest_w(void)
{
	word w;
	printf("forward test:\n\n");
	dx->cur = dx->start;

	fillbuf(128);

	cono(bus, DC, DARQ|DBRQ|OUT|CHMOD_6|DEV1|7);
	cono(bus, UTC, SEL|GO|FWD|DLY3|FN_WBN|DEV1|0);
	while(dt->ut_go){
		dttask.f(dt);
		if(pireq == 7){
			if(bufsz > 0){
				w = bufp[--bufsz];
				datao(bus, DC, w);
				printf("DC send: %012lo\n", w);
			}
//			break;
		}
	}
//	printdc(dc);
}

void
revtest_w(void)
{
	word w;
	printf("reverse test:\n\n");
	dx->cur = dx->end-1;

	fillbuf(128);

	cono(bus, DC, DARQ|DBRQ|OUT|CHMOD_6|DEV1|7);
	cono(bus, UTC, SEL|GO|REV|DLY3|FN_WD|DEV1|0);
	while(dt->ut_go){
		dttask.f(dt);
		if(pireq == 7){
			if(bufsz > 0){
				w = bufp[--bufsz];
				datao(bus, DC, w);
				printf("DC send: %012lo\n", w);
			}
//			break;
		}
	}
//	printdc(dc);
}

void
fwdtest_r(void)
{
	word w;
	printf("forward test:\n\n");
	dx->cur = dx->start;

	cono(bus, DC, DBDAMOVE|IN|CHMOD_6|DEV1|7);
	cono(bus, UTC, SEL|GO|FWD|DLY3|FN_RD|DEV1|0);
	while(dt->ut_go){
		dttask.f(dt);
		if(pireq == 7){
			w = datai(bus, DC);
			printf("DC got: %012lo\n", w);
//			break;
		}
	}
//	printdc(dc);
}

void
revtest_r(void)
{
	word w;
	printf("reverse test:\n\n");
	dx->cur = dx->end-1;

	cono(bus, DC, DBDAMOVE|IN|CHMOD_6|DEV1|7);
	cono(bus, UTC, SEL|GO|REV|DLY3|FN_RD|DEV1|0);
	while(dt->ut_go){
		dttask.f(dt);
		if(pireq == 7){
			w = datai(bus, DC);
			printf("DC got: %012lo\n", w);
//			break;
		}
	}
//	printdc(dc);
}

void
fmttest(void)
{
	word w;
	printf("format test:\n\n");
	dx->cur = dx->start;

	fillbuf_fmt();

	cono(bus, DC, DARQ|DBRQ|OUT|CHMOD_6|DEV1|7);
	cono(bus, UTC, SEL|GO|FWD|DLY3|FN_WTM|DEV1|0);
	while(dt->ut_go){
		dttask.f(dt);
		if(pireq == 7){
			if(bufsz > 0){
				--bufsz;
				w = *bufp++;
				datao(bus, DC, w);
//				printf("DC send: %012lo\n", w);
			}else
				cono(bus, UTC, SEL|DLY2|DEV1|0);
//			break;
		}
	}
//	printdc(dc);
}

void
dxstore(Dx555 *dx, const char *file)
{
	FILE *f;
	f = fopen(file, "wb");
	if(f == nil)
		return;
	fwrite(dx->start, 1, dx->size, f);
	fclose(f);
}

int
main()
{
	dc = (Dc136*)makedc(0, nil);
	dc->dev.ioconnect((Device*)dc, bus);
	dt = (Dt551*)makedt(0, nil);
	dt->dev.ioconnect((Device*)dt, bus);
	dtconn(dc, dt);
	dx = (Dx555*)makedx(0, nil);
	dxconn(dt, dx, 1);
	dx->dev.attach((Device*)dx, "../test/out.dt6");

	bus_reset(bus);
	assert(coni(bus, DC) == 0);

	printdc(dc);

//	fmttest();
//	fwdtest_r();

	dxstore(dx, "foo.dt6");

	return 0;
}
