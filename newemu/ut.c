#include "common.h"
#include "pdp6.h"

#include <fcntl.h>
#include <unistd.h>

#define MOVEDLY 33333
// very approximate, unfortunately no hard data for this
#define STARTDLY 200000000
#define STOPDLY 100000000	// not sure how to use this one here
#define TURNDLY 250000000
// ~350 lines per inch, ~4.56in per block
// again very approximate
#define STARTDIST 7*350
#define STOPDIST 8*350
// after turnaround we will have passed the location that we turned at
// TENDMP assumes one block of turnaround space
// MACDMP assumes two
#define TURNDIST 2*350

struct Ux555
{
	int fd;

	u8 *buf;
	int size, pos;
	bool written;
	bool flapping;
	u64 timer;
	bool tp;

	bool wrlock;
	int num;	// 0-7
	// from controller:
	int go;
	int rev;
};

struct Ut551
{
	Dc136 *dc;
	int dcdev;

	Ux555 *transports[8];
	Ux555 *sel;

	bool btm_sw;

	bool units_select;
	bool tape_end_en;
	bool jb_done_en;
	bool go;
	bool rev;
	bool time_en;
	int time;
	int fcn;
	int units;
	int pia;

	bool incomp_block;
	bool wren;
	bool time_flag;
	bool info_error;
	bool illegal_op;
	bool tape_end_flag;
	bool jb_done_flag;

	int utek;
	bool uteck;

	int lb;
	int rwb;
	int wb;

	int tbm;
	int tdata;
	int tmk;
	int tct;

	int ramp;	// read amplifiers
	int state;
	bool start_dly;
	u64 dlyend;
};



Ux555*
attach_ux(Ut551 *ut, int num)
{
	int i;
	Ux555 *ux = malloc(sizeof(Ux555));

	ux->fd = -1;
	ux->buf = nil;
	ux->size = 0;
	ux->pos = 0;
	ux->tp = 0;

	ux->wrlock = 0;
	ux->num = num & 7;
	ux->go = 0;
	ux->rev = 0;

	for(i = 0; i < 8; i++)
		if(ut->transports[i] == nil) {
			ut->transports[i] = ux;
			break;
		}

	addcleanup((void (*)(void*))uxunmount, ux);

	return ux;
}

void
uxunmount(Ux555 *ux)
{
	if(ux->fd < 0)
		return;

	if(ux->written) {
		lseek(ux->fd, 0, SEEK_SET);
		write(ux->fd, ux->buf, ux->size);
	}

	close(ux->fd);
	ux->fd = -1;

	free(ux->buf);
	ux->buf = nil;
}

void
uxmount(Ux555 *ux, const char *path)
{
	uxunmount(ux);

	ux->fd = open(path, O_RDWR);
	if(ux->fd < 0) {
		ux->fd = open(path, O_RDONLY);
		if(ux->fd < 0) {
			fprintf(stderr, "can't open file <%s>\n", path);
			return;
		}
		ux->wrlock = 1;
	}
	ux->size = lseek(ux->fd, 0, SEEK_END);
	ux->buf = malloc(ux->size);
	lseek(ux->fd, 0, SEEK_SET);
	read(ux->fd, ux->buf, ux->size);
	ux->written = 0;
	ux->pos = 100;
	ux->flapping = ux->pos >= ux->size;	// hopefully always true
	ux->tp = 0;
	ux->go = 0;
	ux->rev = 0;
printf("μt unit %d file <%s>, len %d lock? %d\n", ux->num, path, ux->size, ux->wrlock);
}

static void
uxmove(Ux555 *ux)
{
	if(!ux->go || ux->timer >= simtime)
		return;
	ux->timer += MOVEDLY;
	if(ux->rev) {
		if(--ux->pos < 0)
//printf("flap back\n"),
			ux->flapping = 1;
	} else {
		if(++ux->pos >= ux->size)
//printf("flap fwd\n"),
			ux->flapping = 1;
	}
	if(!ux->flapping)
		ux->tp = 1;
}

static void
uxsetmotion(Ux555 *ux, int go, int rev)
{
	if(ux->go != go) {
		if(!ux->go) {
//printf("start transport\n");
			// start transport
			ux->timer = simtime + STARTDLY;
			ux->pos += rev ? -STARTDIST : STARTDIST;
		} else {
			// stop transport
//printf("stop transport\n");
			ux->pos += ux->rev ? -STOPDIST : STOPDIST;
		}
		ux->go = go;
	} else if(ux->go && ux->rev != rev) {
//printf("turn transport\n");
		// turn around transport
		ux->timer = simtime + TURNDLY;
		ux->pos += rev ? -TURNDIST : TURNDIST;
	}
	ux->rev = rev;
}




#define IOB pdp->iob

enum {
	RW_NULL,
	RW_RQ,
	RW_ACTIVE,
};

#define UT_DN (ut->fcn == 0)
#define UT_RDA (ut->fcn == 1)
#define UT_RDBM (ut->fcn == 2)
#define UT_RDD (ut->fcn == 3)
#define UT_WRTM (ut->fcn == 4)
#define UT_WRA (ut->fcn == 5)
#define UT_WRBM (ut->fcn == 6)
#define UT_WRD (ut->fcn == 7)
#define UT_WRITE (ut->fcn & 4)
#define UT_READ !UT_WRITE
#define UT_ALL (UT_RDA || UT_WRA)
#define UT_BM (UT_RDBM || UT_WRBM)
#define UT_DATA (UT_RDD || UT_WRD)

#define MK_BM_SPACE ((ut->tmk & 0477) == 0425)
#define MK_BM_SYNC (ut->tmk == 0751)
#define MK_DATA_SYNC (ut->tmk == 0632)
#define MK_BM_END (ut->tmk == 0526)
#define MK_FWD_DATA_END (ut->tmk == 0773 || ut->tmk == 0473)
#define MK_REV_DATA_END (ut->tmk == 0610 || ut->tmk == 0410)
#define MK_END (ut->tmk == 0622)
#define MK_DATA (ut->tmk == 0470)
#define MK_DATA_END (MK_FWD_DATA_END || MK_REV_DATA_END)

#define UTE_DC_DISCONNECT (ut->dc->darq || ut->dc->device != ut->dcdev)
#define UTE_MK (MK_BM_END || MK_BM_SYNC || MK_DATA_END || MK_DATA_SYNC || MK_DATA)

static void calc_ut_req(PDP6 *pdp, Ut551 *ut);

static int
utsel(Ut551 *ut)
{
	int i, num, nsel;

	num = ut->units;
	// weirdness: ~UNITS SELECT only disables 0 and 1
	if(!ut->units_select && num < 2)
		num = -1;

	nsel = 0;
	ut->sel = nil;
	for(i = 0; i < 8; i++)
		if(ut->transports[i] && ut->transports[i]->num == num) {
			ut->sel = ut->transports[i];
			nsel++;
		}
	if(nsel != 1)
		ut->sel = nil;
	return nsel;
}

static void
tp0(Ut551 *ut)
{
	Ux555 *ux = ut->sel;

	if(ut->state == RW_ACTIVE) {
		ut->wren = 1;

		if(UT_WRITE) {
			// RWB(J)->WB
			ut->wb = (ut->rwb >> (ut->tct?0:3))&7;
			if(ut->rev) ut->wb ^= 7;
			if(ux->wrlock)
				ut->illegal_op = 1;
		}
	}
}

static void
tp1(Ut551 *ut)
{
	// write complement - rather useless for us here
	if(UT_WRITE)
		ut->wb ^= 7;

	// advance TMK
	if(!ut->start_dly && !UT_WRTM)
		ut->tmk = ((ut->tmk<<1) | (ut->ramp>>3)&1)&0777 | ut->tmk&0400;

	// strobe data into RWB
	if(ut->state == RW_ACTIVE && UT_READ) {
		int c = ut->rev && !UT_RDBM ? 7 : 0;
		if(ut->tct)
			ut->rwb |= ut->ramp&7 ^ c;
		else
			ut->rwb |= (ut->ramp&7 ^ c) << 3;
	}
}

static void
tp2(Ut551 *ut)
{
	bool wasdone = ut->jb_done_flag;

	if(ut->state == RW_ACTIVE) {
		if(ut->tct) {
			// RW EVEN
			ut->lb ^= ~ut->rwb & 077;
			if(UT_READ && (ut->tdata&0102)==0) {
				// RWB<->DC
				ut->rwb |= dctkgv(ut->dc, ut->dcdev, ut->rwb, ut->rev && !UT_RDBM);
				if(UTE_DC_DISCONNECT)
					ut->incomp_block = 1;
			}
		}
		ut->tct ^= 1;

		if(UT_BM && MK_BM_END)
			ut->jb_done_flag = 1;

		if(!UT_ALL && (MK_BM_SYNC || MK_DATA_SYNC)) {
			ut->info_error = 1;
			ut->state = RW_NULL;
		}
	} else
		ut->tct = 0;

	if(ut->uteck && UTE_MK != (ut->utek==040))
		ut->info_error = 1;

	if(!MK_BM_SPACE)
		ut->utek = (ut->utek>>1) | (ut->utek&1)<<5;


	// only interested in rising edges here
	int tbmedge = ~ut->tbm;
	if(MK_BM_SYNC) ut->tbm = 010 | ut->tbm>>1;
	else if(MK_BM_SPACE) ut->tbm >>= 1;
	tbmedge &= ut->tbm;

	// need both rising and falling edges
	int tdedge = ut->tdata;
	if(MK_DATA_SYNC) ut->tdata = 0200 | ut->tdata>>1;
	else if(MK_DATA_END) ut->tdata >>= 1;
	tdedge ^= ut->tdata;

	if(tdedge & ut->tdata & 0100)
		ut->lb = 0;

	if(ut->state == RW_RQ) {
		if(UT_BM && tbmedge & 1 ||
		   UT_ALL && tbmedge & 010 ||
		   UT_DATA && tdedge & ut->tdata & 0100)
			ut->state = RW_ACTIVE;
	} else if(ut->state == RW_ACTIVE && UT_DATA && (tdedge & ~ut->tdata & 2)) {
		if(UTE_DC_DISCONNECT)
			ut->jb_done_flag = 1;
		else
			ut->state = RW_RQ;
	}

	if(ut->jb_done_flag && !wasdone)
		ut->state = RW_NULL;
}

static void
tp3(Ut551 *ut)
{
	if(ut->state == RW_ACTIVE && !ut->tct)
		ut->rwb = 0;

	if(UT_READ && (UT_ALL | UT_DATA) && ut->tdata & 1 &&
	   ut->lb != 077)
		ut->info_error = 1;

	if(ut->tbm & 1) {
		if(MK_BM_SPACE)
			ut->utek = 040;
		else
			ut->uteck = 1;
	}
	if(MK_BM_SYNC)
		ut->uteck = 0;

	if(MK_END) {
		ut->tape_end_flag = 1;
		ut->state = RW_NULL;
	}
}

static void
tp4(Ut551 *ut)
{
	if(ut->state != RW_ACTIVE)
		ut->wren = 0;
	else if(!ut->tct) {
		if(UT_WRITE && UT_DATA && ut->tdata & 2)
			ut->rwb |= ut->lb;
		if(UT_WRITE && (ut->tdata&0102)==0 || UT_WRTM) {
			// RWB<->DC
			// writing - so no need to check RDBM
			ut->rwb |= dctkgv(ut->dc, ut->dcdev, ut->rwb, ut->rev);
			if(UTE_DC_DISCONNECT)
				ut->incomp_block = 1;
		}
	}
}

static void
cycle_ut(PDP6 *pdp, IOdev *dev, int pwr)
{
	int i;
	Ut551 *ut = (Ut551*)dev->dev;
	Ux555 *ux;

	if(!pwr) {
		ut->dlyend = NEVER;
		return;
	}

	int doreq = 0;
	if(ut->start_dly && ut->dlyend < simtime) {
//printf("delay done\n");
		ut->dlyend = NEVER;
		ut->start_dly = 0;
		ut->time_flag = 1;
		if(utsel(ut) != 1) {
			ut->go = 0;
			ut->illegal_op = 1;
		}
		if(UT_WRTM != ut->btm_sw)
			ut->illegal_op = 1;
		if(UT_WRTM)
			ut->state = RW_ACTIVE;
		else if(!UT_DN)
			ut->state = RW_RQ;
		doreq = 1;
	}

	if(ut->tape_end_flag)
		ut->go = 0;

	// move transports
	ux = ut->sel;
	if(ux)
		uxsetmotion(ux, ut->go, ut->rev);
	for(i = 0; i < 8; i++)
		if(ut->transports[i])
			uxmove(ut->transports[i]);

	if(ux && ux->tp) {
		ux->tp = 0;
		doreq = 1;

		// rising edge of time track
		int mask = ux->rev ? 017 : 0;
		ut->ramp = ux->buf[ux->pos] ^ mask;

		// which fires TP0 - write load
		tp0(ut);

		// write back at some point
		if(UT_WRITE && ut->wren && !ux->wrlock) {
			if(UT_WRTM && ut->btm_sw)
				ux->buf[ux->pos] = ((ut->wb<<1)&010 | ut->wb) ^ mask;
			else
				ux->buf[ux->pos] = (ut->ramp&010 | ut->wb) ^ mask;
			ux->written = 1;
		}

		// ca 16.6μs later

		// falling edge of time track
		// write complement - read strobe
		tp1(ut);
		// NB we don't also write the complement
		tp2(ut);
		tp3(ut);
		tp4(ut);
	}

	if(doreq) calc_ut_req(pdp, ut);
}

static void
handle_ut(PDP6 *pdp, IOdev *dev, int cmd)
{
	Ut551 *ut = (Ut551*)dev->dev;

	switch(cmd) {
	case IOB_RESET:
	case IOB_CONO_CLR:
		ut->incomp_block = 0;
		ut->wren = 0;
		ut->time_flag = 0;
		ut->info_error = 0;
		ut->illegal_op = 0;
		ut->tape_end_flag = 0;
		ut->jb_done_flag = 0;

		ut->units_select = 0;
		ut->tape_end_en = 0;
		ut->jb_done_en = 0;
		ut->go = 0;
		ut->rev = 0;
		ut->time_en = 0;
		ut->time = 0;
		ut->fcn = 0;
		ut->units = 0;
		ut->pia = 0;

		ut->state = RW_NULL;
		break;

	case IOB_CONO_SET:
//printf("CONO UT %o (PC %06o)\n", (int)IOB&0777777, pdp->pc);
		if(IOB & F1) ut->units_select = 1;
		if(IOB & F2) ut->tape_end_en = 1;
		if(IOB & F3) ut->jb_done_en = 1;
		if(IOB & F4) ut->go = 1;
		if(IOB & F5) ut->rev = 1;
		if(IOB & F6) ut->time_en = 1;
		ut->time |= (IOB>>27) & 3;
		ut->fcn |= (IOB>>24) & 7;
		ut->units |= (IOB>>21) & 7;
		ut->pia |= (IOB>>18) & 7;

		// command is effective before selection changes
		if(ut->sel) uxsetmotion(ut->sel, ut->go, ut->rev);

		utsel(ut);

		if(ut->time == 0) {
			// bit strange for DN and WRTM
			ut->state = RW_RQ;
		} else {
			static int dlytab[4] = { 0, 35000000, 225000000, 300000000 };
			ut->dlyend = simtime + dlytab[ut->time];
			ut->start_dly = 1;
			// T CLEAR
			ut->tbm = 0;
			ut->tdata = 0;
			ut->tmk = 0;
			ut->uteck = 0;
		}
		break;

	case IOB_STATUS:
		if(ut->units_select) IOB |= F19;
		if(ut->tape_end_en) IOB |= F20;
		if(ut->jb_done_en) IOB |= F21;
		if(ut->go) IOB |= F22;
		if(ut->rev) IOB |= F23;
		if(ut->time_en) IOB |= F24;
		IOB |= ut->time << 9;
		IOB |= ut->fcn << 6;
		IOB |= ut->units << 3;
		IOB |= ut->pia;
		break;
	}
	calc_ut_req(pdp, ut);
}

static void
handle_uts(PDP6 *pdp, IOdev *dev, int cmd)
{
	Ut551 *ut = (Ut551*)dev->dev;

	switch(cmd) {
	case IOB_STATUS:
		if(ut->start_dly) IOB |= F25;
		if(ut->state == RW_RQ) IOB |= F26;
		if(ut->state == RW_ACTIVE) IOB |= F27;
		if(ut->state == RW_NULL) IOB |= F28;
		if(ut->incomp_block) IOB |= F29;
		if(ut->wren) IOB |= F30;
		if(ut->time_flag) IOB |= F31;
		if(ut->info_error) IOB |= F32;
		if(ut->illegal_op) IOB |= F33;
		if(ut->tape_end_flag) IOB |= F34;
		if(ut->jb_done_flag) IOB |= F35;
		break;
	}
}

static Ut551 ut;
static IOdev ut_dev = { 0, 0210, &ut, handle_ut, cycle_ut };
static IOdev uts_dev = { 0, 0214, &ut, handle_uts, nil };

static void
calc_ut_req(PDP6 *pdp, Ut551 *ut)
{
	int req = 0;
	if(ut->pia &&
	   (ut->time_flag && ut->time_en ||
	    ut->jb_done_flag && ut->jb_done_en ||
	    ut->tape_end_flag && ut->tape_end_en ||
	    ut->illegal_op || ut->info_error))
		req = 0200>>ut->pia;
	setreq(pdp, &ut_dev, req);
}

Ut551*
attach_ut(PDP6 *pdp, Dc136 *dc)
{
	ut.dc = dc;
	ut.dcdev = 1;
	installdev(pdp, &ut_dev);
	installdev(pdp, &uts_dev);
	return &ut;
}
