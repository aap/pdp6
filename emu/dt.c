#include "pdp6.h"
#include <fcntl.h>
#include <unistd.h>

char *dt_ident = DT_IDENT;
char *dx_ident = DX_IDENT;

// TODO: find out how this should be controlled
#define DEVNO 1

enum
{
	DTSIZE = 922512
};

/* Transport */

static uchar
dxread(Dx555 *dx)
{
	if(dx->dt->ut_rev)
		return *dx->cur ^ 010;
	else
		return *dx->cur;
}

static void
dxwrite(Dx555 *dx, uchar d)
{
	if(dx->dt->ut_rev)
		*dx->cur = d ^ 010;
	else
		*dx->cur = d;
}

static void
dxmove(Dx555 *dx)
{
	// At end of tape read the last word forever
	if(dx->dt->ut_rev){
		dx->cur--;
		if(dx->cur < dx->start)
			dx->cur = dx->start+5;
	}else{
		dx->cur++;
		if(dx->cur >= dx->end)
			dx->cur = dx->end-6;
	}
}

static void
dxattach(Device *dev, const char *path)
{
	Dx555 *dx;
	int fd;

	dx = (Dx555*)dev;
	fd = dx->fd;
	if(fd >= 0){
		dx->fd = -1;
		close(dx->fd);
	}
	memset(dx->start, 0, DTSIZE);
	dx->fd = open(path, O_RDWR | O_CREAT, 0666);
	if(dx->fd < 0)
		fprintf(stderr, "couldn't open file %s\n", path);
	else
		read(dx->fd, dx->start, DTSIZE);
}

/* Connect a transport to a control */
void
dxconn(Dt551 *dt, Dx555 *dx, int n)
{
	dt->dx[n%8] = dx;
	dx->dt = dt;
	dx->unit = n;
}

Device*
makedx(int argc, char *argv[])
{
	Dx555 *dx;

	dx = malloc(sizeof(Dx555));
	memset(dx, 0, sizeof(Dx555));

	dx->dev.type = dx_ident;
	dx->dev.name = "";
	dx->dev.attach = dxattach;
	dx->dev.ioconnect = nil;

	dx->start = malloc(DTSIZE);
	dx->cur = dx->start;
	dx->end = dx->start + DTSIZE;
	memset(dx->start, 0, DTSIZE);

	return &dx->dev;
}

/* Control */

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


	RW_NULL   = 0,
	RW_RQ     = 1,
	RW_ACTIVE = 2,
};

#define DATA_SYNC (dt->tmk == (0600|DataSync))
#define END_ZONE (dt->tmk == (0600|TapeEndF))
#define BM_SPACE (dt->tmk == (0400|BlockSpace) ||\
		dt->tmk == (0500|BlockSpace) ||\
		dt->tmk == (0600|BlockSpace) ||\
		dt->tmk == (0700|BlockSpace))
#define BM_END (dt->tmk == (0500|BlockEndF))
#define DATA_SYNC (dt->tmk == (0600|DataSync))
#define BM_SYNC (dt->tmk == (0700|BlockSync))
#define FWD_DATA_END (dt->tmk == (0400|DataEndF) ||\
		dt->tmk == (0700|DataEndF))
#define REV_DATA_END (dt->tmk == (0400|DataEndR) ||\
		dt->tmk == (0600|DataEndR))
#define DATA (dt->tmk == (0400|Data))

#define STOP (dt->tdata == 0 || dt->tdata == 1)
#define SYNC (dt->tdata == 0200)
#define REV_CKS (dt->tdata == 0100)
#define BLOCK (dt->tdata == 040 || dt->tdata == 020 || dt->tdata == 010)
#define PRE_FINAL (dt->tdata == 004)
#define FWD_CKS (dt->tdata == 002)

#define UT_WRITE ((dt->ut_fcn & 04) == 04)
#define UT_READ ((dt->ut_fcn & 04) == 0)
#define UT_ALL ((dt->ut_fcn & 03) == 01)
#define UT_BM ((dt->ut_fcn & 03) == 02)
#define UT_DATA ((dt->ut_fcn & 03) == 03)
#define UT_DN (dt->ut_fcn == 0)
#define UT_WRTM (dt->ut_fcn == 04)

#define DC_SELECT1 (dt->dc->devp[dt->dc->device] == dt)
#define UTE_DC_DISCONNECT (!(DC_SELECT1 && !dt->dc->da_rq))
#define UT_WRITE_PREVENT (dt->seldx->wrlock)
#define UT_WREN_DATA (UT_WRITE && dt->ut_wren && !UT_WRITE_PREVENT)
#define RW_ODD (!dt->tct && dt->rw_state == RW_ACTIVE)
#define RW_BM_DONE (UT_BM && BM_END)
#define RW_DATA_CONT (DC_SELECT1 && !dt->dc->da_rq && UT_DATA && dt->rw_state == RW_ACTIVE)
#define RW_DATA_STOP (UTE_DC_DISCONNECT && UT_DATA && dt->rw_state == RW_ACTIVE)

#define TBM0 (dt->tbm == 10)
#define TBM3 (dt->tbm == 1)

// all but block space
#define UTE_MK (DATA_SYNC || END_ZONE || BM_END ||\
	DATA_SYNC || BM_SYNC || FWD_DATA_END || REV_DATA_END || DATA)
#define UTE_ERROR (dt->uteck && (dt->utek == 040) != UTE_MK)
// not sure if BM SYNC || DATA SYNC is correct
#define UTE_ACTIVE_ERROR (!UT_ALL && dt->rw_state == RW_ACTIVE && (BM_SYNC || DATA_SYNC))



#define PRINT1 \
	printf("%c%02o(%o,%02o,%d)", dt->rw_state == RW_ACTIVE ? '+' : dt->rw_state == RW_RQ ? '-' : ' ', \
		dt->tmk&077, l&07, dt->rwb, dt->tct); \
	putchar(' '); \
	if(STOP) \
		printf(" [STOP] "); \
	else if(SYNC) \
		printf(" [SYNC] "); \
	else if(REV_CKS) \
		printf(" [REV_CKS] "); \
	else if(BLOCK) \
		printf(" [BLOCK] "); \
	else if(PRE_FINAL) \
		printf(" [PRE FINAL] "); \
	else if(FWD_CKS) \
		printf(" [FWD CKS %02o] ", dt->lb);

#define PRINT2 \
	if(END_ZONE) \
		printf("TapeEndF "); \
	else if(BM_SPACE) \
		printf("BlockSpace %o ", dt->tbm); \
	else if(BM_END) \
		printf("BlockEndF "); \
	else if(DATA_SYNC) \
		printf("DataSync "); \
	else if(BM_SYNC) \
		printf("BlockSync "); \
	else if(FWD_DATA_END) \
		printf("DataEndF "); \
	else if(REV_DATA_END) \
		printf("DataEndR "); \
	else if(DATA) \
		printf("Data ");

static void
recalc_dt_req(Dt551 *dt)
{
	int req;
	if(dt->ut_jb_done_flag && dt->ut_jb_done_enable ||
	   dt->ut_tape_end_flag && dt->ut_tape_end_enable ||
	   dt->time_flag && dt->time_enable ||
	   dt->ut_info_error ||
	   dt->ut_illegal_op)
		req = dt->ut_pia;
	else
		req = 0;
	setreq(dt->bus, UTC, req);
}

static void
dtcycle(void *p)
{
	Dt551 *dt;
	dt = (Dt551*)p;
	uchar l, wb;
	int data1edge, data7edge;
	int bm0edge, bm3edge;

	// TODO: maybe do something else here?
	if(dt->seldx == nil)
		return;

	if(!dt->ut_go)
		return;


	if(dt->rw_state != RW_ACTIVE)
		dt->tct = 0;
	l = dxread(dt->seldx);

/* TP0 */

	if(dt->rw_state == RW_ACTIVE)
		dt->ut_wren = 1;

	if(UT_WRITE){
		if(UT_WRITE_PREVENT)
			dt->ut_illegal_op = 1;
		if(dt->tct)
			wb = l&010 | dt->rwb&07;
		else
			wb = l&010 | dt->rwb>>3 & 07;
	}
	if(UT_WREN_DATA)
		dxwrite(dt->seldx, wb);

/* TP1 */

//**/				PRINT1
//**/				PRINT2

	dt->tmk = ((dt->tmk << 1) | !!(l&010)) & 0377 |
		dt->tmk&0400 | (dt->tmk<<1)&0400;
	if(UT_READ){
		if(dt->tct != dt->ut_rev)
			dt->rwb |= l&07;
		else
			dt->rwb |= (l&07) << 3;
	}

/* TP2 */
///**/				printf("%2o%c%c%c  ", dt->utek, UTE_MK ? 'M' : ' ', UTE_ERROR ? 'E' : ' ', dt->uteck ?  'C' : ' ');

	if(UTE_ERROR || UTE_ACTIVE_ERROR)
printf("UT INFO ERROR\n"),
		dt->ut_info_error = 1;

	if(!BM_SPACE){
		if(dt->utek == 1)
			dt->utek = 040;
		else
			dt->utek >>= 1;
	}

	if(dt->tct)
		dt->lb ^= ~dt->rwb & 077;

//**/			if(dt->rw_state == RW_ACTIVE && dt->tct)
//**/				printf("(%02o, %02o) ", dt->rwb, dt->lb);

	if(UT_READ && dt->rw_state == RW_ACTIVE && dt->tct){
		// before tct complement
		if(!(UT_DATA && dt->tdata & 0102)){
			dcgv(dt->dc, DEVNO, dt->rwb, dt->ut_rev);
			if(UTE_DC_DISCONNECT)
				dt->ut_incomp_block = 1;
		}
	}


	if(END_ZONE){
		dt->ut_tape_end_flag = 1;
		dt->ut_go = 0;
	}

	bm0edge = 0;
	bm3edge = 0;
	if(BM_SYNC)
		dt->tbm |= 020;	// shifted to 010 (TBM0) now
	if(BM_SPACE || BM_SYNC){
		dt->tbm >>= 1;
		if(TBM0)
			bm0edge = 1;
		else if(TBM3)
			bm3edge = 1;
	}

	data1edge = 0;	// goes up before rev checksum is expected
	data7edge = 0;	// goes up when checksum is seen
	if(DATA_SYNC)
		dt->tdata |= 0400;	// shifted to 0200 now
	if(FWD_DATA_END || REV_DATA_END || DATA_SYNC){
		dt->tdata >>= 1;
		if(dt->tdata == 0100)
			data1edge = 1;
		else if(dt->tdata == 1)
			data7edge = 1;
	}

	if(data1edge)
		dt->lb = 0;

	// TODO: not sure about rw_active
	if(data7edge && dt->rw_state == RW_ACTIVE)
		if(dt->lb != 077)
printf("UT INFO ERROR\n"),
			dt->ut_info_error = 1;

	// before active is set below
	if(dt->rw_state == RW_ACTIVE)
		dt->tct = !dt->tct;

	// manual mentions tdata6(0) going to 0???
	if(dt->rw_state == RW_ACTIVE){
		if(data7edge && RW_DATA_STOP ||
		   RW_BM_DONE){
printf("FINISHING JOB\n");
			dt->ut_jb_done_flag = 1;
			dt->rw_state = RW_NULL;
		}
		if(UTE_ACTIVE_ERROR)
			dt->rw_state = RW_NULL;
		if(data7edge && RW_DATA_CONT)
			dt->rw_state = RW_RQ;
	}else if(dt->rw_state == RW_RQ){
		if(UT_BM && bm3edge ||
		   UT_DATA && data1edge ||
		   UT_ALL && bm0edge)
			dt->rw_state = RW_ACTIVE;
	}

/* TP3 */

	if(RW_ODD)
		dt->rwb = 0;

	if(TBM3 && BM_SPACE)
		dt->utek = 040;
	if(BM_SYNC)
		dt->uteck = 0;
	if(TBM3 && !BM_SPACE)
		dt->uteck = 1;

/* TP4 */

	if(UT_WRITE && RW_ODD){
		/* Take checksum data from LB */
		if(UT_DATA && dt->tdata & 0102)
			dt->rwb = dt->lb;
		else{
			dt->rwb |= dctk(dt->dc, DEVNO, dt->ut_rev);
			if(UTE_DC_DISCONNECT)
				dt->ut_incomp_block = 1;
		}
		if(dt->ut_rev)
			dt->rwb = dt->rwb>>3 & 07 | dt->rwb<<3 & 070;
	}
	if(dt->rw_state == RW_ACTIVE)
		dt->ut_wren = 0;

//**/				printf("\n");

	dxmove(dt->seldx);

	recalc_dt_req(dt);
}

static void
wake_dt(void *dev)
{
	Dt551 *dt;
	IOBus *bus;

	dt = (Dt551*)dev;
	bus = dt->bus;
	if(IOB_RESET){
		dt->ut_incomp_block = 0;

		dt->ut_pia = 0;
		dt->ut_units = 0;
		dt->ut_units_select = 0;
		dt->ut_fcn = 0;
		dt->ut_time = 0;
		dt->ut_wren = 0;
		dt->ut_go = 0;
		dt->ut_rev = 0;
		dt->ut_tape_end_flag = 0;
		dt->ut_tape_end_enable = 0;
		dt->ut_jb_done_flag = 0;
		dt->ut_jb_done_enable = 0;
		dt->time_enable = 0;
		dt->time_flag = 0;
		dt->ut_illegal_op = 0;
		dt->ut_info_error = 0;

		dt->tmk = 0;
		dt->rwb = 0;
		dt->tbm = 0;
		dt->tdata = 0;
		dt->utek = 0;
		dt->uteck = 0;

		dt->rw_state = RW_NULL;
	}

	if(bus->devcode == UTS){
		if(IOB_STATUS){
//printf("UTS STATUS\n");
			// we have no delays (yet?) F25
			if(dt->rw_state == RW_RQ) bus->c12 |= F26;
			if(dt->rw_state == RW_ACTIVE) bus->c12 |= F27;
			if(dt->rw_state == RW_NULL) bus->c12 |= F28;
			if(dt->ut_incomp_block) bus->c12 |= F29;
			if(dt->ut_wren) bus->c12 |= F30;
			if(dt->time_flag) bus->c12 |= F31;
			if(dt->ut_info_error) bus->c12 |= F32;
			if(dt->ut_illegal_op) bus->c12 |= F33;
			if(dt->ut_tape_end_flag) bus->c12 |= F34;
			if(dt->ut_jb_done_flag) bus->c12 |= F35;
		}
	}
	if(bus->devcode == UTC){
		if(IOB_STATUS){
//printf("UTC STATUS\n");
			if(dt->ut_units_select) bus->c12 |= F19;
			if(dt->ut_tape_end_enable) bus->c12 |= F20;
			if(dt->ut_jb_done_enable) bus->c12 |= F21;
			if(dt->ut_go) bus->c12 |= F22;
			if(dt->ut_rev) bus->c12 |= F23;
			if(dt->time_enable) bus->c12 |= F24;
			bus->c12 |= (dt->ut_time & 03) << 9;
			bus->c12 |= (dt->ut_fcn & 07) << 6;
			bus->c12 |= (dt->ut_units & 07) << 3;
			bus->c12 |= dt->ut_pia & 07;
		}
		if(IOB_CONO_CLEAR){
//printf("UTC CONO CLEAR\n");
			dt->ut_incomp_block = 0;
			dt->ut_wren = 0;
			dt->time_flag = 0;
			dt->ut_info_error = 0;
			dt->ut_illegal_op = 0;
			dt->ut_tape_end_flag = 0;
			dt->ut_jb_done_flag = 0;

			dt->rw_state = RW_NULL;
		}
		if(IOB_CONO_SET){
//printf("UTC CONO SET\n");
			int i, n, nunits;

			dt->ut_pia = bus->c12 & 07;
			dt->ut_units = bus->c12>>3 & 07;
			dt->ut_fcn = bus->c12>>6 & 07;
			dt->ut_time = bus->c12>>9 & 03;
			dt->time_enable = bus->c12>>11 & 1;
			dt->ut_rev = bus->c12>>12 & 1;
			dt->ut_go = bus->c12>>13 & 1;
			dt->ut_jb_done_enable = bus->c12>>14 & 1;
			dt->ut_tape_end_enable = bus->c12>>15 & 1;
			dt->ut_units_select = bus->c12>>16 & 1;

			// UT CONO SET LONG
			//  triggers UT START level until end of delay

			// find selected transport unit
			n = dt->ut_units == 0 ? 8 : dt->ut_units;
			dt->seldx = nil;
			nunits = 0;
			if(dt->ut_units_select)
				for(i = 0; i < 8; i++)
					if(dt->dx[i] && dt->dx[i]->unit == n){
						dt->seldx = dt->dx[i];
						nunits++;
					}
			if(dt->ut_time){
				// wait delay
				dt->time_flag = 1;
				dt->uteck = 0;

				if(nunits != 1){
					dt->ut_go = 0;
					dt->ut_illegal_op = 1;
					goto ret;
				}
				if(dt->ut_btm_switch != UT_WRTM)
					dt->ut_illegal_op = 1;
			}

			if(!UT_DN){
				if(UT_WRTM)
					dt->rw_state = RW_ACTIVE;
				else
					dt->rw_state = RW_RQ;
			}
		}
	}
ret:
	recalc_dt_req(dt);
}

static void
dtioconnect(Device *dev, IOBus *bus)
{
	Dt551 *dt;
	dt = (Dt551*)dev;
	dt->bus = bus;
	bus->dev[UTC] = (Busdev){ dt, wake_dt, 0 };
	bus->dev[UTS] = (Busdev){ dt, wake_dt, 0 };
}

/* Connect a dt control to a data control */
void
dtconn(Dc136 *dc, Dt551 *dt)
{
	dt->dc = dc;
	dc->devp[DEVNO] = dt;
}

Device*
makedt(int argc, char *argv[])
{
	Dt551 *dt;
	Thread th;

	dt = malloc(sizeof(Dt551));
	memset(dt, 0, sizeof(Dt551));

	dt->dev.type = dt_ident;
	dt->dev.name = "";
	dt->dev.attach = nil;
	dt->dev.ioconnect = dtioconnect;

	// should have 30000 cycles per second
	th = (Thread){ nil, dtcycle, dt, 1000, 0 };
	addthread(th);

	return &dt->dev;
}
