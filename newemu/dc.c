#include "common.h"
#include "pdp6.h"

#define IOB pdp->iob

static void calc_dc_req(PDP6 *pdp, Dc136 *dc);

static void
dbda_swap(Dc136 *dc)
{
	dc->dbda_move = 0;
	dc->darq = 0;
	dc->dbrq = 1;
	if(dc->inout)	// out
		dc->da = dc->db;
	else {
		dc->db = dc->da;
		dc->da = 0;
	}
//printf("DB req %012llo\n", dc->db);
}

/*
 * bits per char  ch mode  shifts per char  chars per word
 *             6      0 0                0               6
 *            12      1 0                2               3
 *            18      1 1                3               2
 *            36      0 1                0               1
 */
#define NSHIFT ((0xE0 >> (dc->ch_mode*2)) & 3)
#define NCHARS ((02316 >> (dc->ch_mode*3)) & 7)
#define DC_SCT_DONE (dc->sct == NSHIFT)
#define DC_CCT_DONE (dc->cct == NCHARS-1)

static void
dashcp0(Dc136 *dc, Word inp)
{
cp0:
	// DASH CP0
	if(dc->darq)
		dc->data_clbd = 1;

	// 1μs dly

	// DASH CP1
	if(DC_CCT_DONE) {
		dc->cct = 0;
		if(dc->dbda_move && !dc->darq)
			dbda_swap(dc);
		else
			dc->darq = 1;
	} else if(DC_SCT_DONE) {
		// CCT CONT
		dc->sct = 0;
		dc->cct = (dc->cct+1) & 7;
	} else {
		// SCT CONT
		dc->sct = (dc->sct+1) & 3;

		// DASH LT
		dc->da = dc->da<<6 | inp;
		goto cp0;
	}
}

int
dctkgv(Dc136 *dc, int dev, int ch, int rt)
{
	int ret;
	if(dc->device != dev)
		return 0;

	switch(dev) {
	case 1: case 2:
		if(rt) {
			ret = dc->da & 077;
			ret = (ret<<3 | ret>>3) & 077;
			ch = (ch<<3 | ch>>3) & 077;
			dc->da = dc->da>>6 | (Word)ch<<30;
		} else {
	case 3: case 4:
			ret = (dc->da>>30) & 077;
			ch &= 077;
			dc->da = dc->da<<6 | ch;
		}
		break;
	default:	// shouldn't happen
		return 0;
	}	
	dc->da &= FW;
	dashcp0(dc, ch);
	calc_dc_req(dc->pdp, dc);
	return ret;
}

static void
handle_dc(PDP6 *pdp, IOdev *dev, int cmd)
{
	Dc136 *dc = (Dc136*)dev->dev;

	bool s0 = dc->darq && dc->dbda_move;
	switch(cmd) {
	case IOB_RESET:
		dc->db = 0;
		dc->da = 0;	// hack so DB isn't overwritten again
	case IOB_CONO_CLR:
		dc->sct = 0;
		dc->cct = 0;
		dc->data_clbd = 0;
		dc->dbda_move = 0;
		dc->darq = 0;
		dc->dbrq = 0;
		dc->inout = 0;
		dc->ch_mode = 0;
		dc->device = 0;
		dc->pia = 0;

		dc->db = dc->da;
		dc->da = 0;
		break;

	case IOB_CONO_SET:
		if(IOB & F23) dc->data_clbd = 1;
		if(IOB & F24) dc->dbda_move = 1;
		if(IOB & F25) dc->darq = 1;
		if(IOB & F26) dc->dbrq = 1;
		if(IOB & F27) dc->inout = 1;
		dc->ch_mode |= (IOB>>6)&3;
		dc->device |= (IOB>>3)&7;
		dc->pia |= IOB&7;
		break;

	case IOB_DATAO_CLR:
		dc->db = 0;
		dc->dbrq = 0;
		break;
	case IOB_DATAO_SET:
		dc->db |= IOB;
		dc->dbda_move = 1;
		break;

	case IOB_DATAI:
		IOB |= dc->db;
		dc->dbrq = 0;
		dc->dbda_move = 1;
		break;

	case IOB_STATUS:
		IOB |= dc->cct<<13;
		if(dc->data_clbd) IOB |= F23;
		if(dc->dbda_move) IOB |= F24;
		if(dc->darq) IOB |= F25;
		if(dc->dbrq) IOB |= F26;
		if(dc->inout) IOB |= F27;
		IOB |= dc->ch_mode<<6;
		IOB |= dc->device<<3;
		IOB |= dc->pia;
		break;
	}
	bool s1 = dc->darq && dc->dbda_move;
	if(s1 && !s0)
		dbda_swap(dc);
	calc_dc_req(pdp, dc);
}

static Dc136 dc;
static IOdev dc_dev = { 0, 0200, &dc, handle_dc, nil };

static void
calc_dc_req(PDP6 *pdp, Dc136 *dc)
{
	setreq(pdp, &dc_dev, dc->pia && dc->dbrq ? 0200>>dc->pia : 0);
}

Dc136*
attach_dc(PDP6 *pdp)
{
	dc.pdp = pdp;
	installdev(pdp, &dc_dev);

	return &dc;
}
