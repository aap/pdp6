#include "pdp6.h"

Dc136 *dc;
IOBus iobus;
IOBus *bus = &iobus;
int pireq;

void
setreq(IOBus *bus, int dev, u8 pia)
{
	pireq = pia;
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

/*
 * output:
 *  DARQ:	DA empty, need new data
 *  DBRQ:	DB empty, need new data
 *  DBDA MOVE:	DB waiting to send data to DA
 * input:
 *  DARQ:	DA full, need to empty
 *  DBRQ:	DB full, need to empty
 *  DBDA MOVE:	DB waiting to get data from DA
 */

int
main()
{
	dc = (Dc136*)makedc(0, nil);
	dc->dev.ioconnect((Device*)dc, bus);

	bus_reset(bus);
	assert(coni(bus, DC) == 0);

	// output test
	cono(bus, DC, DARQ|DBRQ|OUT|CHMOD_6|DEV1|7);	// 3417
	assert(pireq == 7);
	printdc(dc);
	datao(bus, DC, 0123456000001);
	assert(pireq == 7);
	printdc(dc);
	datao(bus, DC, 0123456000002);
	assert(pireq == 0);
	printdc(dc);
	assert(dctk(dc, 1, 0) == 012);
	assert(pireq == 0);
	printdc(dc);
	assert(dctk(dc, 1, 0) == 034);
	assert(pireq == 0);
	printdc(dc);
	assert(dctk(dc, 1, 0) == 056);
	assert(pireq == 0);
	printdc(dc);
	assert(dctk(dc, 1, 0) == 000);
	assert(pireq == 0);
	printdc(dc);
	assert(dctk(dc, 1, 0) == 000);
	assert(pireq == 0);
	printdc(dc);
	assert(dctk(dc, 1, 0) == 001);
	assert(pireq == 7);
	printdc(dc);

	printf("\n\n");

	cono(bus, DC, DARQ|DBRQ|OUT|CHMOD_12|DEV5|7);	// 3657
	assert(pireq == 7);
	printdc(dc);
	datao(bus, DC, 0123456112233);
	assert(pireq == 7);
	printdc(dc);
	datao(bus, DC, 0123456445566);
	assert(pireq == 0);
	printdc(dc);
	assert((dctk(dc, 5, 0)&0777700000000) == 0123400000000);
	assert(pireq == 0);
	printdc(dc);
	assert((dctk(dc, 5, 0)&0777700000000) == 0561100000000);
	assert(pireq == 0);
	printdc(dc);
	assert((dctk(dc, 5, 0)&0777700000000) == 0223300000000);
	assert(pireq == 7);
	printdc(dc);

	printf("\n\n");

	// input test
	cono(bus, DC, DBDAMOVE|IN|CHMOD_6|DEV1|7);	// 4017
	assert(pireq == 0);
	printdc(dc);
	dcgv(dc, 1, 012, 0);
	assert(pireq == 0);
	printdc(dc);
	dcgv(dc, 1, 034, 0);
	assert(pireq == 0);
	printdc(dc);
	dcgv(dc, 1, 056, 0);
	assert(pireq == 0);
	printdc(dc);
	dcgv(dc, 1, 011, 0);
	assert(pireq == 0);
	printdc(dc);
	dcgv(dc, 1, 022, 0);
	assert(pireq == 0);
	printdc(dc);
	dcgv(dc, 1, 033, 0);
	assert(pireq == 7);
	printdc(dc);
	assert(datai(bus, DC) == 0123456112233);
	printdc(dc);


	printf("\n\n");


	cono(bus, DC, DBDAMOVE|IN|CHMOD_12|DEV5|7);	// 4017
	assert(pireq == 0);
	printdc(dc);
	dcgv(dc, 5, 01234, 0);
	assert(pireq == 0);
	printdc(dc);
	dcgv(dc, 5, 05611, 0);
	assert(pireq == 0);
	printdc(dc);
	dcgv(dc, 5, 02233, 0);
	assert(pireq == 7);
	printdc(dc);

	return 0;
}
