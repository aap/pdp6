#include "pdp6.h"

char *dc_ident = DC_IDENT;

/* table from DC1:
 * bits per char  ch mode  shifts per char  chars per word
 *             6      0 0                0               6
 *            12      1 0                2               3
 *            18      1 1                3               2
 *            36      0 1                0               1
 */
#define NSHIFT ((0x70 >> (dc->ch_mode*2)) & 03)
#define NCHARS ((02316 >> (dc->ch_mode*3)) & 07)
#define DC_SCT_DONE (dc->sct == NSHIFT)
#define DC_CCT_DONE (dc->cct == NCHARS-1)
//#define DC_CCT_CONT (DC_SCT_DONE && !(DC_CCT_DONE))
//#define DC_SCT_CONT (!(DC_SCT_DONE) && !(DC_CCT_DONE))

static void
recalc_dc_req(Dc136 *dc)
{
	setreq(dc->bus, DC, dc->db_rq ? dc->pia : 0);
}

static void
dbdamove(Dc136 *dc)
{
	dc->da_rq = 0;
	dc->dbda_move = 0;
	dc->db_rq = 1;

	if(dc->inout)
		dc->da = dc->db;	// out
	else{
		dc->db = dc->da;	// in
		dc->da = 0;
	}
}

static void
dash_cp0(Dc136 *dc)
{
cp0:
	// DASH CP0
	if(dc->da_rq)
		dc->data_clbd = 1;

	// DASH CP1
	if(DC_CCT_DONE){
//printf("Full DA: %012lo\n", dc->da);
		dc->cct = 0;
		if(dc->da_rq == 0){
			dc->da_rq = 1;
			if(dc->dbda_move)
				dbdamove(dc);
		}else
			dc->da_rq = 1;
	}else{
		if(DC_SCT_DONE){
			// CCT CONT
			dc->cct++;
			// this shouldn't be under the !CCT_DONE branch
			// but the flow diagram FD1 does it too
			dc->sct = 0;
		}else{
			// SCT CONT
			dc->sct++;

			// DASH LT
			dc->da <<= 6;
			goto cp0;
		}
	}
}

void
dcgv(Dc136 *dc, int n, word c, int rev)
{
//printf("DC Taking %06lo from dev %d\n", c, n);
	switch(n){
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

word
dctk(Dc136 *dc, int n, int rev)
{
	word c;
	switch(n){
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
wake_dc(void *dev)
{
	Dc136 *dc;
	IOBus *bus;
	int dbda_move_old, da_rq_old;

	dc = dev;
	bus = dc->bus;

	if(IOB_RESET){
		dc->db = 0;
		dc->da = 0;

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

	if(bus->devcode == DC){
		dbda_move_old = dc->dbda_move;
		da_rq_old = dc->da_rq;

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
			dc->db_rq = 0;
			dc->dbda_move = 1;
		}
		if(IOB_CONO_CLEAR){
			dc->db = dc->da;
			dc->da = 0;

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
		if(IOB_CONO_SET){
			if(bus->c12 & F23) dc->data_clbd = 1;
			if(bus->c12 & F24) dc->dbda_move = 1;
			if(bus->c12 & F25) dc->da_rq = 1;
			if(bus->c12 & F26) dc->db_rq = 1;
			if(bus->c12 & F27) dc->inout = 1;
			dc->ch_mode |= bus->c12>>6 & 03;
			dc->device |= bus->c12>>3 & 07;
			dc->pia |= bus->c12 & 07;
		}
		if(IOB_DATAO_CLEAR){
			dc->db = 0;
			dc->db_rq = 0;
		}
		if(IOB_DATAO_SET){
			dc->db |= bus->c12;
			dc->dbda_move = 1;
		}

		if(dc->dbda_move && dc->da_rq &&
		   !(dbda_move_old && da_rq_old))
			dbdamove(dc);

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
	dc->dev.attach = nil;
	dc->dev.ioconnect = dcioconnect;

	return &dc->dev;
}
