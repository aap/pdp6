#include "common.h"
#include "pdp6.h"

#include <unistd.h>

#define IOB pdp->iob

typedef struct GEcon GEcon;
struct GEcon {
	int out_pia;
	bool out_ready;
	bool out_busy;
	u8 out_chr;
	u8 out_lp;
	int out_port;
	int out_state;
	u64 out_timer;

	int in_pia;
	bool in_ready;
	u8 in_chr;
};

static void calc_ge_req(PDP6 *pdp, GEcon *ge);

static void
handle_gtyi(PDP6 *pdp, IOdev *dev, int cmd)
{
	GEcon *ge = (GEcon*)dev->dev;

	switch(cmd) {
	case IOB_RESET:
	case IOB_CONO_CLR:
		ge->in_pia = 0;
		ge->in_ready = 0;
		break;
	case IOB_CONO_SET:
		ge->in_pia |= IOB & 7;
		break;
	case IOB_STATUS:
		IOB |= ge->in_pia;
		if(ge->in_ready) IOB |= F32;
		return;
	case IOB_DATAI:
		IOB |= ge->in_chr;
		ge->in_ready = 0;
		break;
	}
	calc_ge_req(pdp, ge);
}

static void
handle_gtyo(PDP6 *pdp, IOdev *dev, int cmd)
{
	GEcon *ge = (GEcon*)dev->dev;

	switch(cmd) {
	case IOB_RESET:
	case IOB_CONO_CLR:
		ge->out_pia = 0;
		ge->out_ready = 0;
		ge->out_busy = 0;
		ge->out_chr = 0;
		ge->out_state = 0;
		break;
	case IOB_CONO_SET:
		ge->out_pia |= IOB & 7;
		// init somehow
		if(IOB & F28) {
			ge->out_busy = 0;
			ge->out_ready = 1;
		}
		break;
	case IOB_DATAO_CLR:
		ge->out_chr = 0;
		ge->out_ready = 0;
		ge->out_busy = 1;
		break;
	case IOB_DATAO_SET:
		ge->out_chr |= (IOB & 0177)^0177;
		ge->out_chr = (ge->out_chr<<1 | ge->out_chr>>6) & 0177;
		ge->out_lp ^= ge->out_chr;

		ge->out_timer = simtime + 1000000;
		break;
	case IOB_STATUS:
		IOB |= ge->out_pia;
		if(ge->out_ready) IOB |= F29;
		return;
	}
	calc_ge_req(pdp, ge);
}

static void
cycle_ge(PDP6 *pdp, IOdev *dev, int pwr)
{
	GEcon *ge = (GEcon*)dev->dev;

	if(!pwr) {
		return;
	}

	// TODO: actually connect this to consoles

	if(ge->out_busy && ge->out_timer < simtime) {
		switch(ge->out_state) {
		case 0:	// wait for msg start
			if(ge->out_chr == 1) {
				ge->out_state = 1;
				ge->out_lp = 0;
			}
			break;
		case 1:	// read port to write to
			if((ge->out_chr & 0147) == 0140) {
				ge->out_state = 2;
				ge->out_port = (ge->out_chr>>3) & 7;
			} else
				ge->out_state = 0;
			break;
		case 2:	// read status code (always 0 for us)
			if(ge->out_chr == 0)
				ge->out_state = 3;
			else
				ge->out_state = 0;
			break;
		case 3:	// read start of message
			if(ge->out_chr == 2)
				ge->out_state = 4;
			break;
		case 4:	// the actual message
			if(ge->out_chr == 3)
				ge->out_state = 5;
			else
				printf("GE %d: %o\n", ge->out_port, ge->out_chr);
			break;
		case 5:	// checksum
			if(ge->out_lp != 0)
				printf("GE checksum error %o\n", ge->out_lp);
			ge->out_state = 0;
			break;
		}
		ge->out_ready = 1;
		ge->out_busy = 0;
		calc_ge_req(pdp, ge);
	}
}

static GEcon gecon;
static IOdev gtyi_dev = { 0, 0070, &gecon, handle_gtyi, nil };
static IOdev gtyo_dev = { 0, 0750, &gecon, handle_gtyo, cycle_ge };

static void
calc_ge_req(PDP6 *pdp, GEcon *ge)
{
	int ireq = 0;
	if(ge->in_pia && ge->in_ready)
		ireq = 0200>>ge->in_pia;
	setreq(pdp, &gtyi_dev, ireq);

	int oreq = 0;
	if(ge->out_pia && ge->out_ready)
		oreq = 0200>>ge->out_pia;
	setreq(pdp, &gtyo_dev, oreq);

}

void
attach_ge(PDP6 *pdp)
{
	installdev(pdp, &gtyi_dev);
	installdev(pdp, &gtyo_dev);
}
