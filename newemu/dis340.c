#include "common.h"
#include "pdp6.h"

#include <unistd.h>

enum Modes
{
	PM,
	XYM,
	SM,
	CM,
	VM,
	VCM,
	IM,
	SBM
};

struct Dis340
{
	int fd;

	/* 344 interface for PDP-6.
	 * no schematics unfortunately */
	int pia_data;
	int pia_spec;
	Word ib;	/* interface buffer */
	int ibc;	/* number of words in IB */

	/* 340 display and 342 char gen */
	Hword br;	/* 18 bits */
	int mode;	/* 3 bits */
	int i;		/* intensity - 3 bits */
	int sz;		/* scale - 2 bits */
	int brm;	/* 7 bits */
	int s;		/* shift - 4 bits */
	int x, y;	/* 10 bits */
	/* flip flops */
	int stop;
	int halt;
	int move;
	int lp_find;
	int lp_flag;
	int lp_enable;
	int rfd;
	int hef, vef;
	/* br shifted for IM and CM */
	int pnts;
	int chrs;
	/* values for BRM counting */
	int brm_add;
	int brm_comp;

	int penx, peny;
	int pen;

	/* 342 char gen */
	int shift;
	int *cp;	/* current char pointer */

	u64 simtime, lasttime;
	u64 inputtimer;
	int state;
	int pnt;
	u32 cmdbuf[128];
	u32 ncmds;
};

#define LDB(p, s, w) ((w)>>(p) & (1<<(s))-1)
#define IOB pdp->iob
static void calc_dis_req(PDP6 *pdp, Dis340 *dis);


static void
addcmd(Dis340 *dis, u32 cmd)
{
	dis->cmdbuf[dis->ncmds++] = cmd;
	if(dis->ncmds == nelem(dis->cmdbuf)) {
		if(write(dis->fd, dis->cmdbuf, sizeof(dis->cmdbuf)) < sizeof(dis->cmdbuf))
			dis->fd = -1;
		dis->ncmds = 0;
	}
}

static void
agedisplay(Dis340 *dis)
{
	if(dis->fd < 0)
		return;
	u32 cmd = 511<<23;
	assert(dis->lasttime <= dis->simtime);
	u64 dt = (dis->simtime - dis->lasttime)/1000;
	while(dt >= 511) {
		dis->lasttime += 511*1000;
		addcmd(dis, cmd);
		dt = (dis->simtime - dis->lasttime)/1000;
	}
}

static void
intensify(Dis340 *dis)
{
	if(dis->pen && dis->lp_enable) {
		int dx = dis->penx - dis->x;
		int dy = dis->peny - dis->y;
		if(dx*dx + dy*dy <= 4)
			dis->lp_find = 1;
	}
	if(dis->fd >= 0){
		agedisplay(dis);
		u32 cmd;
		cmd = dis->x;
		cmd |= dis->y<<10;
		cmd |= dis->i<<20;
		int dt = (dis->simtime - dis->lasttime)/1000;
		cmd |= dt<<23;
		dis->lasttime = dis->simtime;
		addcmd(dis, cmd);
	}
}

enum {
	ST_IDLE,
	ST_DATA_SYNC,
	ST_READ_TO_S,
	ST_DLY_IDP,
	ST_IDP,
	ST_XY_INTENSIFY,
	ST_SEQ_DLY,
	ST_CHAR,
	ST_CHAR_INT,
	ST_RFD,
};


static void
rfd(Dis340 *dis)
{
	dis->rfd = 1;
	dis->hef = 0;
	dis->vef = 0;
}

static void
escape(Dis340 *dis)
{
	dis->mode = 0;
	dis->lp_find = 0;
}

static void
initiate(Dis340 *dis)
{
	dis->shift = 0;

	dis->lp_enable = 0;	// TODO: not in schematics?
	dis->lp_find = 0;
	dis->lp_flag = 0;
	dis->stop = 0;
	escape(dis);

	dis->state = ST_RFD;
}

static void
resume(Dis340 *dis)
{
	dis->lp_enable = 0;
	dis->lp_find = 0;
	dis->lp_flag = 0;
	if(dis->mode != CM) {
		dis->simtime += 1000;
		dis->state = ST_SEQ_DLY;
	}
}

/* figure out add and complement values for COUNT BRM */
static void
initbrm(Dis340 *dis)
{
	int m;
	int dx, dy, d;

	dis->brm_add = 1;

	dx = LDB(0, 7, dis->br);
	dy = LDB(8, 7, dis->br);
	d = dx > dy ? dx : dy;
	for(m = 0100; m > 040; m >>= 1)
		if(d & m)
			break;
		else
			dis->brm_add *= 2;
	dis->brm_comp = dis->brm_add - 1;
}

static void
read_to_mode(Dis340 *dis)
{
	dis->mode = LDB(13, 3, dis->br);
	if(dis->br & 0010000)
		dis->lp_enable = !!(dis->br & 04000);
}

static void
shift_s(Dis340 *dis)
{
	dis->s >>= 1;
	dis->pnts <<= 4;
	dis->chrs <<= 6;
}

static void
count_brm(Dis340 *dis)
{
	dis->brm = ((dis->brm+dis->brm_add) ^ dis->brm_comp) & 0177;
	shift_s(dis);
}

static void
count_x(Dis340 *dis, int s, int r, int l)
{
	if(r) dis->x += s;
	if(l) dis->x -= s;
	if(r || l) dis->move = dis->mode != CM;
	if(dis->x >= 1024 || dis->x < 0) dis->hef = 1;
	dis->x &= 01777;
	if(dis->mode == IM && dis->s&1)
		dis->halt = 1;
}

static void
count_y(Dis340 *dis, int s, int u, int d)
{
	if(u) dis->y += s;
	if(d) dis->y -= s;
	if(u || d) dis->move = dis->mode != CM;	// actually in count_x
	if(dis->y >= 1024 || dis->y < 0) dis->vef = 1;
	dis->y &= 01777;
	dis->lp_flag |= dis->lp_find && dis->lp_enable;
}

static void
getdir(int brm, int xy, int c, int *pos, int *neg)
{
	int dir;
	int foo;

	foo = ~brm & c;
	dir =	foo&0001 && xy&0100 ||
		foo&0002 && xy&0040 ||
		foo&0004 && xy&0020 ||
		foo&0010 && xy&0010 ||
		foo&0020 && xy&0004 ||
		foo&0040 && xy&0002 ||
		foo&0100 && xy&0001;
	*pos = 0;
	*neg = 0;
	if(dir){
		if(xy & 0200)
			*neg = 1;
		else
			*pos = 1;
	}
}

#include "chargen.inc"

static void
cycle_dis(PDP6 *pdp, IOdev *dev, int pwr)
{
	Dis340 *dis = (Dis340*)dev->dev;
	int halt, pnt, r, l, u, d, c, brm;

	if(!pwr) {
		dis->simtime = simtime;
		agedisplay(dis);
		return;
	}

	agedisplay(dis);
	if(dis->inputtimer < simtime) {
		dis->inputtimer = simtime + 30000000;
		if(hasinput(dis->fd)) {
			u32 cmds[512];
			int n = read(dis->fd, cmds, sizeof(cmds));
			n /= 4;
			for(int i = 0; i < n; i++) {
				u32 cmd = cmds[i];
				dis->peny = cmd & 01777;
				dis->penx = (cmd>>10) & 01777;
				dis->pen = (cmd>>20) & 1;
			}
		}
	}

	int doreq = 0;
	// catch up with CPU
	while(dis->simtime < simtime) {
	doreq = 1;
	switch(dis->state) {
	case ST_IDLE:
		if(dis->rfd && dis->ibc > 0)
			dis->state = ST_DATA_SYNC;
		else
			dis->simtime = simtime;
		break;

	case ST_DATA_SYNC:
		// CLR BR
		dis->br = 0;
		dis->rfd = 0;

		// CLR BRM
		dis->brm = 0;
		dis->halt = 0;
		dis->move = 0;
		dis->s = 0;	// CLEAR S
		dis->hef = 0;	// CLR CF
		dis->vef = 0;	// CLR CF

		dis->simtime += 2800;
		dis->state = ST_READ_TO_S;
		break;

	case ST_READ_TO_S:
		// READ TO S
		dis->lp_flag |= dis->lp_find && dis->lp_enable;
		if(dis->mode == IM) dis->s |= 010;
		if(dis->mode == CM) dis->s |= 004;

		// LOAD BR
		dis->br |= dis->ib>>18 & RT;
		dis->pnts = dis->br;
		dis->chrs = dis->br;
		initbrm(dis);

		// SHIFT IB
		dis->ib = dis->ib<<18 & LT;
		dis->ibc--;

		dis->state = ST_DLY_IDP;
		break;

	case ST_DLY_IDP:
		if(dis->move && dis->br&0200000)
			intensify(dis);
		dis->simtime += 500;
		dis->state = ST_IDP;
		break;
	case ST_IDP:
		dis->state = ST_IDLE;
		switch(dis->mode) {
		case PM:
			// TODO: what is RI?
			read_to_mode(dis);
			/* PM PULSE */
			if(dis->br & 010) dis->i = LDB(0, 3, dis->br);
			if(dis->br & 0100) dis->sz = LDB(4, 2, dis->br);
			if(dis->br & 02000) dis->stop = 1;
			else dis->state = ST_RFD;
			break;

		case XYM:
			if(dis->lp_flag)
				break;
			if(dis->br & 0200000){
				// Y START
				read_to_mode(dis);
				dis->y = 0;	// CLEAR Y
				dis->simtime += 200;
				dis->y |= dis->br & 01777;	// LOAD Y
				if(dis->br & 0002000)
					dis->simtime += 35000;
			}else{
				// X START
				read_to_mode(dis);
				dis->simtime += 35000;
				dis->x = 0;	// CLEAR X
				// 200ns parallel to above dly
				dis->x |= dis->br & 01777;	// LOAD X
			}
			dis->state = ST_XY_INTENSIFY;
			break;

		case IM:
			halt = dis->halt;
			count_brm(dis);
			pnt = LDB(16, 4, dis->pnts);
			r = (pnt & 014) == 010;
			l = (pnt & 014) == 014;
			u = (pnt & 003) == 002;
			d = (pnt & 003) == 003;
			goto count_xy;

		case VM:
		case VCM:
			halt = dis->halt;
			brm = dis->brm;
			count_brm(dis);
			/* actually the complement inputs to COUNT BRM
			 * but it's easier to find them out after the addition. */
			c = dis->brm^brm;
			getdir(brm, LDB(0, 8, dis->br), c, &r, &l);
			getdir(brm, LDB(8, 8, dis->br), c, &u, &d);

count_xy:
			count_y(dis, 1<<dis->sz, u, d);
			count_x(dis, 1<<dis->sz, r, l);
			if(halt) {
				/* at COUNT X */
				if(dis->br & 0400000)
					escape(dis);
				dis->state = ST_RFD;
			} else {
				// dly started by COUNT BRM
				dis->simtime += 1000;
				dis->state = ST_SEQ_DLY;
			}
			break;

		case CM:
			/* Tell CG to start */
			dis->cp = chars[LDB(12, 6, dis->chrs) + dis->shift];
			dis->state = ST_CHAR;
			break;
		}
		break;

	case ST_CHAR:
		pnt = dis->pnt = *dis->cp++;
		if(pnt == 0) {
end:
			/* end of character */
			if(dis->s & 1)
				dis->state = ST_RFD;
			else {
				shift_s(dis);
				dis->state = ST_DLY_IDP;
			}
			break;
		}
		if(pnt & 040) {
			/* escape from char */
			escape(dis);
			dis->state = ST_RFD;
			break;
		}
		if(pnt & 0100) {
			/* carriage return */
			dis->x = 0;
			goto end;	// maybe?
		}
		if(pnt & 0200) {
			/* shift in */
			dis->shift = 0;
			goto end;
		}
		if(pnt & 0400) {
			/* shift out */
			dis->shift = 64;
			goto end;
		}
		/* cg count */
		r = (pnt & 014) == 010;
		l = (pnt & 014) == 014;
		u = (pnt & 003) == 002;
		d = (pnt & 003) == 003;
		count_y(dis, 1<<dis->sz, u, d);
		count_x(dis, 1<<dis->sz, r, l);
		dis->simtime += 1000;
		dis->state = ST_CHAR_INT;
		break;

	case ST_CHAR_INT:
		if(dis->pnt & 020)
			intensify(dis);
		dis->simtime += 500;
		dis->state = ST_CHAR;
		break;

	case ST_SEQ_DLY:
		dis->state = ST_IDLE;
		if(dis->mode == VM && dis->brm == 0177)
			dis->halt = 1;
		if(dis->hef || dis->vef){
			escape(dis);
			if(dis->mode == VCM)
				dis->state = ST_RFD;
		}else if(!dis->rfd && !dis->lp_flag)
			dis->state = ST_DLY_IDP;
		break;

	case ST_XY_INTENSIFY:
		if(dis->br & 0002000)
			intensify(dis);
		dis->simtime += 500;
		dis->state = ST_RFD;
		break;

	case ST_RFD:
		rfd(dis);
		dis->state = ST_IDLE;
		break;
	}
	}

	// could be more efficient with this
	if(doreq) calc_dis_req(pdp, dis);
}

static void
handle_dis(PDP6 *pdp, IOdev *dev, int cmd)
{
	Dis340 *dis = (Dis340*)dev->dev;

	switch(cmd) {
	case IOB_RESET:
		dis->pia_data = 0;
		dis->pia_spec = 0;

		dis->ib = 0;
		dis->ibc = 0;

		initiate(dis);
		break;

	case IOB_CONO_CLR:
		dis->pia_data = 0;
		dis->pia_spec = 0;
		return;
	case IOB_CONO_SET:
// TODO: according to .INFO.;340 INFO more stuff:
//	24	clear no ink mode
//	25	set no ink mode
//	26	clear half word mode
//	27	set half word mode
//	28	resume display
		dis->pia_data |= IOB & 7;
		dis->pia_spec |= IOB>>3 & 7;

		if(IOB & F29)
			initiate(dis);
		else
			resume(dis);	// not sure about this
		break;

	case IOB_DATAO_CLR:
		dis->ib = 0;
		dis->ibc = 0;
		return;
	case IOB_DATAO_SET:
		dis->ib |= IOB;
		dis->ibc = 2;
		break;

	case IOB_STATUS:
#if 0
// like this in the manual
		if(dis->lp_flag) IOB |= F25;
		if(dis->hef || dis->vef) IOB |= F26;
#else
// according to .INFO.;340 INFO
		IOB |= dis->mode<<15;
		// VECT CONT LP??
		if(dis->vef) IOB |= F24;
		if(dis->lp_flag) IOB |= F25;
		if(dis->hef) IOB |= F26;
#endif
		if(dis->stop && dis->br&01000) IOB |= F27;
		if(dis->ibc == 0 && dis->rfd) IOB |= F28;
		IOB |= (dis->pia_spec & 7) << 3;
		IOB |= dis->pia_data & 7;
		return;

	case IOB_DATAI:
		IOB |= dis->x;
		IOB |= dis->y << 18;
		return;

	}
	calc_dis_req(pdp, dis);
}

static Dis340 dis;
static IOdev dis_dev = { 0, 0130, &dis, handle_dis, cycle_dis };

static void
calc_dis_req(PDP6 *pdp, Dis340 *dis)
{
	int data, spec;
	u8 req_data, req_spec;

	data = dis->ibc == 0 && dis->rfd;
	spec = dis->lp_flag ||
		dis->hef || dis->vef ||
		dis->stop && dis->br&01000;
	req_data = data ? 0200 >> dis->pia_data : 0;
	req_spec = spec ? 0200 >> dis->pia_spec : 0;

	setreq(pdp, &dis_dev, req_data | req_spec);
}

// stub
static void
handle_joy(PDP6 *pdp, IOdev *dev, int cmd)
{
	switch(cmd) {
	case IOB_DATAI:
		IOB |= FW;
		break;
	}
}
static IOdev joy_dev = { 0, 0420, nil, handle_joy, nil };

Dis340*
attach_dis(PDP6 *pdp)
{
	dis.fd = -1;
	installdev(pdp, &dis_dev);

	installdev(pdp, &joy_dev);

	return &dis;
}

void
dis_connect(Dis340 *dis, int fd)
{
	if(dis->fd >= 0)
		close(dis->fd);
	dis->fd = fd;
}
