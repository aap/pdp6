#include "pdp6.h"

char *dc_ident = DC_IDENT;

/* table from DC1:
 * bits per char  ch mode  shifts per char  chars per word
 *             6      0 0                0               6
 *            12      1 0                2               3
 *            18      1 1                3               2
 *            36      0 1                0               1
 */
#define NSHIFT ((0xE0 >> (dc->ch_mode*2)) & 03)
#define NCHARS ((02316 >> (dc->ch_mode*3)) & 07)
#define DC_SCT_DONE (dc->sct == NSHIFT)
#define DC_CCT_DONE (dc->cct == NCHARS-1)
//#define DC_CCT_CONT (DC_SCT_DONE && !(DC_CCT_DONE))
//#define DC_SCT_CONT (!(DC_SCT_DONE) && !(DC_CCT_DONE))

#define DB_JM_DA (dc->db = dc->da), dc->da = 0
#define DA_JM_DB (dc->da = dc->db)
#define DB_CLR dc->db = dc->db_rq = 0

static void
recalc_dc_req(Dc136 *dc)
{
	setreq(dc->bus, DC, dc->db_rq ? dc->pia : 0);
}

static void
db_da_swap(Dc136 *dc)
{
	dc->da_rq = 0;
	dc->dbda_move = 0;
	dc->db_rq = 1;

	if(dc->inout)
		DA_JM_DB;	// out
	else
		DB_JM_DA;	// in
}

/* TODO: maybe make these two a bit nicer?? */
static void
set_da_rq(Dc136 *dc)
{
	int prev;

	prev = dc->da_rq && dc->dbda_move;
	dc->da_rq = 1;
	if(!prev && dc->da_rq && dc->dbda_move)
		db_da_swap(dc);
}
static void
set_dbda_move(Dc136 *dc)
{
	int prev;

	prev = dc->da_rq && dc->dbda_move;
	dc->dbda_move = 1;
	if(!prev && dc->da_rq && dc->dbda_move)
		db_da_swap(dc);
}

static void
dash_cp0(Dc136 *dc)
{
cp0:
	// DASH CP0
	if(dc->da_rq)
		dc->data_clbd = 1;

	// 1ms delay

	// DASH CP1
	if(DC_CCT_DONE){
//printf("Full DA: %012lo\n", dc->da);
		dc->cct = 0;
		set_da_rq(dc);
	}else if(DC_SCT_DONE){
		// CCT CONT
		dc->sct = 0;
		dc->cct = (dc->cct+1) & 7;
	}else{
		// SCT CONT
		dc->sct = (dc->sct+1) & 3;

		// DASH LT
		dc->da = (dc->da << 6) & FW;
		goto cp0;
	}
}

/* Give TO DC, this is the TK signal in the DC.
 * NB: for dev 5/6 give only the lower N bits. */
void
dcgv(Dc136 *dc, int n, word c, int rev)
{
//printf("DC Taking %06lo from dev %d\n", c, n);

	if(n != dc->device)
		return;
	switch(dc->device){
	case 1: case 2:
		if(rev)
			dc->da = (dc->da >> 6 | (c&077)<<30) & FW;
		else
			dc->da = (dc->da << 6 | c&077) & FW;
		break;
	case 3: case 4:
		dc->da = (dc->da << 6 | c&077) & FW;
		break;
	case 5: case 6:
		dc->da |= c & FW;
		break;
	default:
		return;
	}
	dash_cp0(dc);
	recalc_dc_req(dc);
}

/* Take FROM DC, this is the GV signal in the DC.
 * NB: for dev 5/6 read only the upper N bits. */
word
dctk(Dc136 *dc, int n, int rev)
{
	word c;

	if(n != dc->device)
		return 0;
	switch(dc->device){
	case 1: case 2:
		if(rev){
			c = dc->da & 077;
			dc->da = (dc->da >> 6) & FW;
		}else{
			c = (dc->da >> 30) & 077;
			dc->da = (dc->da << 6) & FW;
		}
		break;
	case 3: case 4:
		c = (dc->da >> 30) & 077;
		dc->da = (dc->da << 6) & FW;
		break;
	case 5: case 6:
		c = dc->da & FW;
		break;
	default:
		return 0;
	}
	dash_cp0(dc);
	recalc_dc_req(dc);
//printf("DC Giving %06lo to dev %d\n", c, n);
	return c;
}

static void
ic_clr(Dc136 *dc)
{
	DB_JM_DA;

	dc->cct = 0;
	dc->sct = 0;
	dc->data_clbd = 0;
	dc->dbda_move = 0;
	dc->da_rq = 0;
	dc->db_rq = 0;
	dc->inout = 0;
	dc->ch_mode = 0;
	dc->device = 0;
	dc->pia = 0;
}

static void
wake_dc(void *dev)
{
	Dc136 *dc;
	IOBus *bus;

	dc = dev;
	bus = dc->bus;

	if(IOB_RESET){
		ic_clr(dc);
		DB_CLR;
	}

	if(bus->devcode == DC){
		if(IOB_STATUS){
			bus->c12 |= (dc->cct&7) << 13;
			if(dc->data_clbd) bus->c12 |= F23;
			if(dc->dbda_move) bus->c12 |= F24;
			if(dc->da_rq) bus->c12 |= F25;
			if(dc->db_rq) bus->c12 |= F26;
			if(dc->inout) bus->c12 |= F27;
			bus->c12 |= (dc->ch_mode & 3) << 6;
			bus->c12 |= (dc->device & 7) << 3;
			bus->c12 |= dc->pia & 7;
		}
		if(IOB_DATAI){
			bus->c12 |= dc->db;
//printf("Sending DB to APR: %012lo\n", dc->db);
			dc->db_rq = 0;
			set_dbda_move(dc);
		}
		if(IOB_CONO_CLEAR)
			ic_clr(dc);
		if(IOB_CONO_SET){
			if(bus->c12 & F23) dc->data_clbd = 1;
			if(bus->c12 & F24) set_dbda_move(dc);
			if(bus->c12 & F25) set_da_rq(dc);
			if(bus->c12 & F26) dc->db_rq = 1;
			if(bus->c12 & F27) dc->inout = 1;
			dc->ch_mode |= bus->c12>>6 & 03;
			dc->device |= bus->c12>>3 & 07;
			dc->pia |= bus->c12 & 07;
		}
		if(IOB_DATAO_CLEAR)
			DB_CLR;
		if(IOB_DATAO_SET){
			dc->db |= bus->c12;
//printf("Got DB from APR: %012lo\n", dc->db);
			set_dbda_move(dc);
		}

		recalc_dc_req(dc);
	}
}

static void
dcioconnect(Device *dev, IOBus *bus)
{
	Dc136 *dc;
	dc = (Dc136*)dev;
	dc->bus = bus;
	bus->dev[DC] = (Busdev){ dc, wake_dc, 0 };
}

Device*
makedc(int argc, char *argv[])
{
	Dc136 *dc;

	dc = malloc(sizeof(Dc136));
	memset(dc, 0, sizeof(Dc136));

	dc->dev.type = dc_ident;
	dc->dev.name = "";
	dc->dev.ioconnect = dcioconnect;

	return &dc->dev;
}
