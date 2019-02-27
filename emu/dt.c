#include "pdp6.h"
#include <fcntl.h>
#include <unistd.h>

char *dt_ident = DT_IDENT;
char *dx_ident = DX_IDENT;

// TODO: find out how this should be controlled
#define DEVNO 1

enum
{
//	DTSIZE = 922512
	// 01102 blocks, 2700 word for start/end each + safe zone
	DTSIZE = 987288 + 1000
};

/* Transport */

#if 0
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
#endif

static uchar
dxread_(Dx555 *dx)
{
	if(dx->dt->ut_rev)
		return *dx->cur ^ 017;
	else
		return *dx->cur;
}

static void
dxwrite_(Dx555 *dx, uchar d)
{
	if(dx->dt->ut_rev)
		*dx->cur = d ^ 017;
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
dxdetach(Device *dev)
{
	Dx555 *dx;

	dx = (Dx555*)dev;
	if(dx->fd < 0)
		return;

	lseek(dx->fd, 0, SEEK_SET);
	write(dx->fd, dx->start, dx->size);
	close(dx->fd);
	dx->fd = -1;
	memset(dx->start, 0, dx->size);
}

static void
dxattach(Device *dev, const char *path)
{
	Dx555 *dx;

	dx = (Dx555*)dev;
	dxdetach(dev);
	memset(dx->start, 0, dx->size);
	// TODO: resize
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
	dx->dev.detach = dxdetach;

	dx->fd = -1;
	dx->size = DTSIZE;
	dx->start = malloc(dx->size);
	dx->cur = dx->start;
	dx->end = dx->start + dx->size;
	memset(dx->start, 0, dx->size);

	return &dx->dev;
}

/* Control */

enum
{
	TapeEndF  = 022,
	TapeEndR  = 055,

	/* block mark high half */
	BlockSpace = 025,

	/* block mark low half */
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
#define BM_SPACE ((dt->tmk&0477) == (0400|BlockSpace))
#define BM_END (dt->tmk == (0500|BlockEndF))
#define BM_SYNC (dt->tmk == (0700|BlockSync))
#define REV_BM (dt->tmk == (0500|BlockEndR))
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
#define UT_WRITE_PREVENT (UT_WRITE && dt->seldx->wrlock)
#define UT_WREN_DATA (UT_WRITE && dt->ut_wren && !UT_WRITE_PREVENT)
#define UT_WREN_BTM (UT_WREN_DATA && dt->ut_btm_switch && UT_WRTM)	// 3-14
#define RW_ODD (!dt->tct && dt->rw_state == RW_ACTIVE)	// 3-6
#define RW_EVEN (dt->tct && dt->rw_state == RW_ACTIVE)
#define RW_BM_DONE (UT_BM && BM_END)
#define RW_DATA_CONT (DC_SELECT1 && !dt->dc->da_rq && UT_DATA && prev.rw_state == RW_ACTIVE)
// TODO: what is RW_DATA_WR_CONT?
// TODO: this is really RW_DATA_WR_STOP
#define RW_DATA_STOP (UTE_DC_DISCONNECT && UT_DATA && prev.rw_state == RW_ACTIVE)

#define TBM0 (dt->tbm == 10)
#define TBM3 (dt->tbm == 1)

#define UTE_UNIT_OK (nunits == 1)
#define UTE_DC_DISCONNECT (!(DC_SELECT1 && !dt->dc->da_rq))
// all but block space and rev block mark
#define UTE_MK (DATA_SYNC || END_ZONE || BM_END ||\
	BM_SYNC || FWD_DATA_END || REV_DATA_END || DATA)
#define UTE_ERROR (dt->uteck && (prev.utek == 040) != UTE_MK)
// TODO: not sure if BM SYNC || DATA SYNC is correct
#define UTE_ACTIVE_ERROR (!UT_ALL && prev.rw_state == RW_ACTIVE && (BM_SYNC || DATA_SYNC))

//#define debug(...) printf(__VA_ARGS__)
#define debug(...)

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
		debug("%c%d %02o TapeEndF ", state, dt->tct, dt->lb); \
	else if(BM_SPACE) \
		debug("%c%d %02o BlockSpace %o ", state, dt->tct, dt->lb, dt->tbm); \
	else if(BM_END) \
		debug("%c%d %02o BlockEndF %o ", state, dt->tct, dt->lb, dt->tbm); \
	else if(REV_BM) \
		debug("%c%d %02o BlockEndR %o ", state, dt->tct, dt->lb, dt->tbm); \
	else if(DATA_SYNC) \
		debug("%c%d %02o DataSync %o ", state, dt->tct, dt->lb, dt->tdata); \
	else if(BM_SYNC) \
		debug("%c%d %02o BlockSync %o ", state, dt->tct, dt->lb, dt->tbm); \
	else if(FWD_DATA_END) \
		debug("%c%d %02o DataEndF %o ", state, dt->tct, dt->lb, dt->tdata); \
	else if(REV_DATA_END) \
		debug("%c%d %02o DataEndR %o ", state, dt->tct, dt->lb, dt->tdata); \
	else if(DATA) \
		debug("%c%d %02o Data %o ", state, dt->tct, dt->lb, dt->tdata);

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

#define LDB(p, s, w) ((w)>>(p) & (1<<(s))-1)
#define XLDB(ppss, w) LDB((ppss)>>6 & 077, (ppss)&077, w)
#define MASK(p, s) ((1<<(s))-1 << (p))
#define DPB(b, p, s, w) ((w)&~MASK(p,s) | (b)<<(p) & MASK(p,s))
#define XDPB(b, ppss, w) DPB(b, (ppss)>>6 & 077, (ppss)&077, w)

word dtbuf;
char state = 'n';

#define REV (dt->ut_rev && !UT_BM)

static void
dt_tp0(Dt551 *dt)
{
	dt->sense = dxread_(dt->seldx);

	// 3-7
	if(UT_WRITE && dt->rw_state == RW_ACTIVE)
		dt->ut_wren = 1;

	// 3-7
	if(UT_WRITE){	// TODO
		int c;
		c = REV ? 7 : 0;
		if(dt->tct)
			dt->wb = dt->rwb&7 ^ c;
		else
			dt->wb = dt->rwb>>3&7 ^ c;
		/* copy to mark track; 3-13/14 */
		if(UT_WREN_BTM)
			dt->wb |= dt->wb<<1 & 010;
		else
			dt->wb = dt->wb&7 | dt->sense&010;
		if(UT_WRITE_PREVENT)
			dt->ut_illegal_op = 1;		// 3-16
		if(UT_WREN_DATA){
			dxwrite_(dt->seldx, dt->wb);
debug("writing %02o\n", dt->wb);
		}
	}
}

static void
dt_tp1(Dt551 *dt)
{
	/* complement WB here, totally useless */
	//dt->wb = ~dt->wb & 07;

	/* advance mark track window */
	if(!UT_WRTM)
		dt->tmk = ((dt->tmk << 1) | LDB(3, 1, dt->sense)) & 0777 |
			dt->tmk&0400;

	/* Strobe sensors into RWB */
	// 3-12
	if(dt->rw_state == RW_ACTIVE && UT_READ){
		int c = REV ? 7 : 0;
		if(dt->tct)
			dt->rwb |= (dt->sense^c)&07;
		else
			dt->rwb |= ((dt->sense^c)&07) << 3;
	}
}

static void
dt_tp2(Dt551 *dt)
{
	Dt551 prev;
	int tbedge, tdedge;

	prev = *dt;

	if(END_ZONE){
		dt->ut_go = 0;
		dt->ut_tape_end_flag = 1;
		/* TODO: is this right? */
		dt->rw_state = RW_NULL;
	}

	/* Block mark timing.
	 * Shift down starting at block sync,
	 * shifts to 10 before rev block number,
	 * shifts to 01 before fwd block number. */
	// 3-5
	if(BM_SYNC)
		dt->tbm |= 020;
	if(BM_SYNC || BM_SPACE)
		dt->tbm >>= 1;
	tbedge = ~prev.tbm & dt->tbm;

	/* Data timing.
	 * Shift down starting at data sync.
	 * shifts to 100 before reverse check sum,
	 * shifts to 001 after forward check sum */
	// 3-8, 3-9
	if(DATA_SYNC)
		dt->tdata = 0400;
	if(FWD_DATA_END || REV_DATA_END || DATA_SYNC)
		dt->tdata >>= 1;
	tdedge = ~prev.tdata & dt->tdata;


	/* Send 6 bit byte to DC every two steps when reading */
	if(UT_READ && RW_EVEN)
		/* except checksums */
		if((prev.tdata & 0102) == 0){
			int rwb = dt->rwb;
			if(REV)
				rwb = rwb>>3 & 07 | rwb<<3 & 070;
			dcgv(dt->dc, DEVNO, rwb, REV);
			if(UTE_DC_DISCONNECT)
				dt->ut_incomp_block = 1;	// 3-17
		}

	/* Clear LB before reading the reverse checksum */
	if(tdedge & 0100)
		dt->lb = 0;	// 3-9

	/* Keep track of checksum */
	if(dt->tct)
		// Manual says XOR, but also talks of parity of zeros
//		dt->lb ^= dt->rwb;
		dt->lb ^= ~dt->rwb & 077;

	if(prev.rw_state != RW_ACTIVE)
		dt->tct = 0;
	else
		dt->tct ^= 1;

	/* Ring count UTEK */
	if(!BM_SPACE){
		if(dt->utek == 1)
			dt->utek = 0100;
		dt->utek >>= 1;
	}

	/* Go into active state */
	if(prev.rw_state == RW_RQ)
		if(UT_BM && tbedge & 1 ||	// 3-6
		   UT_DATA && tdedge & 0100 ||	// 3-9
		   UT_ALL && tbedge & 010)	// 3-11
			dt->rw_state = RW_ACTIVE;

	/* What to do after block */
	if(tdedge & 1){
		if(RW_DATA_CONT)		// 3-10
			dt->rw_state = RW_RQ;
		if(RW_DATA_STOP)		// 3-11
			dt->ut_jb_done_flag = 1;
		/* test checksum. NB: manual says should be 0
		 * TODO: not sure about ACTIVE */
		if(prev.rw_state == RW_ACTIVE && dt->lb != 077){
debug("CHECKSUM ERR\n");
			dt->ut_info_error = 1;	// 3-13
		}
	}

	/* Job done after block mark */
	if(prev.rw_state == RW_ACTIVE && RW_BM_DONE)	// 3-8
		dt->ut_jb_done_flag = 1;

	/* Catch wrongly set active state; 3-16 */
	if(UTE_ACTIVE_ERROR){
		dt->ut_info_error = 1;
		dt->rw_state = RW_NULL;
	}
	/* Catch invalid mark code */
	if(UTE_ERROR){
debug("TRACK ERROR\n");
		dt->ut_info_error = 1;		// 3-15
	}

	/* Stop activity once job gets done */
	if(!prev.ut_jb_done_flag && dt->ut_jb_done_flag)	// 3-8
		dt->rw_state = RW_NULL;
}

static void
dt_tp3(Dt551 *dt)
{
	// 3-6
	if(RW_ODD)
		dt->rwb = 0;

	// 3-15
	/* before fwd block mark */
	if(dt->tbm & 1){
		if(BM_SPACE)
			dt->utek = 040;		// UTE PRESET 100000
		else
			dt->uteck = 1;
	}
	if(BM_SYNC)
		dt->uteck = 0;
}

static void
dt_tp4(Dt551 *dt)
{
	/* stop writing WB here */
	if(dt->rw_state != RW_ACTIVE)
		dt->ut_wren = 0;

	/* get 6 bit byte from DC every two steps when writing */
	if(UT_WRITE && RW_ODD){
		/* except checksums */
		if((dt->tdata & 0102) == 0){
			dt->rwb |= dctk(dt->dc, DEVNO, REV);
			if(UTE_DC_DISCONNECT)
				dt->ut_incomp_block = 1;	// 3-17
		}else{
debug("Getting LB\n");
			dt->rwb |= dt->lb;
		}
		// 3-6
		if(REV)
			dt->rwb = dt->rwb>>3 & 07 | dt->rwb<<3 & 070;
	}
}

/* count dialled transports and select last */
static int
selectdx(Dt551 *dt)
{
	int i, n, nunits;
	n = dt->ut_units == 0 ? 8 : dt->ut_units;
	nunits = 0;
	dt->seldx = nil;
	if(dt->ut_units_select)
		for(i = 0; i < 8; i++)
			if(dt->dx[i] && dt->dx[i]->unit == n){
				dt->seldx = dt->dx[i];
				nunits++;
			}
	return nunits;
}

/* Read one line of bits and move the tape.
 * This should occur once every 33.33μs */
static void
dtcycle(void *p)
{
	Dt551 *dt;
	dt = (Dt551*)p;

	if(dt->delay){
		if(--dt->delay == 0){
			// UT_START goes to 0 here

			int nunits;

			dt->time_flag = 1;

			/* check for legal selection */
			nunits = selectdx(dt);

			// UTE_SW_ERROR; 3-16
			if(!UTE_UNIT_OK ||
			   dt->ut_btm_switch != UT_WRTM){
debug("ILL op %d %d %d\n", nunits, dt->ut_btm_switch, UT_WRTM);
				dt->ut_illegal_op = 1;
			}
			if(!UTE_UNIT_OK){
				dt->ut_go = 0;		// 3-3
				return;
			}

			if(UT_WRTM)		// 3-14
				dt->rw_state = RW_ACTIVE;
			else if(!UT_DN)		// TODO: is this right?
				dt->rw_state = RW_RQ;
		}else
			/* Don't move during delay.
			 * TODO: is this right? */
			return;
	}

	// TODO: maybe do something else here?
	if(dt->seldx == nil)
		return;

	if(!dt->ut_go)
		return;

	// sense; start writing
	dt_tp0(dt);

dtbuf = dtbuf<<3 & FW;
dtbuf |= dt->sense & 07;

	// read strobe
	dt_tp1(dt);

	// interpret mark code
	dt_tp2(dt);

	// clear rwb; mark track error sync
	dt_tp3(dt);

	// get next rwb from DC to write
	dt_tp4(dt);

	state = dt->rw_state == RW_NULL ? 'n' :
	        dt->rw_state == RW_RQ ? 'r' :
	        dt->rw_state == RW_ACTIVE ? 'a' : 'x';
	PRINT2
	else
		debug("%c%d -- ", state, dt->tct);
	debug(" %012lo %02o %d.%02o\n", dtbuf, dt->rwb, dt->uteck, dt->utek);

	dxmove(dt->seldx);
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
			if(dt->delay) bus->c12 |= F25;
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
printf("UTC STATUS\n");
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

			// 3-5
			dt->rw_state = RW_NULL;
		}
		if(IOB_CONO_SET){
//printf("UTC CONO SET\n");
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

			// UT CONO SET LONG, 1ms after UT CONO SET
			//  triggers UT START level until end of delay

			/* update selection, also done after delay */
			selectdx(dt);

			dt->delay = 0;
			if(dt->ut_time){
				switch(dt->ut_time){
				case 1:	// 20ms
					dt->delay = 600;
					break;
				case 2:	// 160ms
					dt->delay = 4800;
					break;
				case 3: // 300ms
					dt->delay = 9000;
					break;
				}
				// T CLEAR; 3-4
				dt->tmk = 0;
				dt->uteck = 0;	// 3-15

				dt->time_flag = 0;
				// now wait
			}else
				if(!UT_DN &&		// TODO: is this right?
				   !UT_WRTM)		// 3-14
					dt->rw_state = RW_RQ;
		}
	}
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

int
dtdep(Device *dev, const char *reg, word data)
{
	Dt551 *dt;

	dt = (Dt551*)dev;
	if(strcmp(reg, "btm_wr") == 0) dt->ut_btm_switch = data&1;
	else return 1;
	return 0;
}

word
dtex(Device *dev, const char *reg)
{
	Dt551 *dt;

	dt = (Dt551*)dev;
	if(strcmp(reg, "btm_wr") == 0) return dt->ut_btm_switch;
	return ~0;
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
	dt->dev.ioconnect = dtioconnect;
	dt->dev.deposit = dtdep;
	dt->dev.examine = dtex;

	// should have 30000 cycles per second, so one every 33μs
	// APR at 1 has an approximate cycle time of 200-300ns
	th = (Thread){ nil, dtcycle, dt, 150, 0 };
	addthread(th);

	return &dt->dev;
}
