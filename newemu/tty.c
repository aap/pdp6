#include "common.h"
#include "pdp6.h"

#include <unistd.h>

#define IOB pdp->iob

static void calc_tty_req(PDP6 *pdp);

static void
handle_tty(PDP6 *pdp, IOdev *dev, int cmd)
{
	switch(cmd) {
	case IOB_RESET:
		pdp->tty_pia = 0;
		pdp->tto_busy = 0;
		pdp->tto_flag = 0;
		pdp->tti_busy = 0;
		pdp->tti_flag = 0;
		pdp->tti_timer = 0;
		break;
	case IOB_CONO_CLR:
		pdp->tty_pia = 0;
		return;
	case IOB_CONO_SET:
		pdp->tty_pia |= IOB & 7;
		if(IOB & F32) pdp->tto_flag = 1;
		if(IOB & F31) pdp->tto_busy = 1;
		if(IOB & F30) pdp->tti_flag = 1;
		if(IOB & F29) pdp->tti_busy = 1;
		if(IOB & F28) pdp->tto_flag = 0;
		if(IOB & F27) pdp->tto_busy = 0;
		if(IOB & F26) pdp->tti_flag = 0;
		if(IOB & F25) pdp->tti_busy = 0;
		break;
	case IOB_DATAO_CLR:
		pdp->tto_busy = 1;
		pdp->tto_flag = 0;
		break;
	case IOB_DATAO_SET:
		pdp->tto |= IOB & 0377;
		pdp->tto_active = 1;	// not really, but close enough
		pdp->tto_timer = simtime + pdp->tty_dly*11;
		break;
	case IOB_DATAI:
		pdp->tti_flag = 0;
		IOB |= pdp->tti;
		break;
	case IOB_STATUS:
		IOB |= pdp->tty_pia;
		if(pdp->tto_flag) IOB |= F32;
		if(pdp->tto_busy) IOB |= F31;
		if(pdp->tti_flag) IOB |= F30;
		if(pdp->tti_busy) IOB |= F29;
		return;
	}
	calc_tty_req(pdp);
}

static void
cycle_tty(PDP6 *pdp, IOdev *dev, int pwr)
{
	if(!pwr) {
		pdp->tto_active = 0;
		pdp->tti_state = 0;
		return;
	}

	if(pdp->tto_active && pdp->tto_timer < simtime) {
		pdp->tto_active = 0;
		if(pdp->tty_fd >= 0) {
			char c = pdp->tto & 0177;
			write(pdp->tty_fd, &c, 1);
		}
		pdp->tto = 0;
		pdp->tto_busy = 0;
		pdp->tto_flag = 1;
		calc_tty_req(pdp);
	}

	// timing is awkward right now, we think in bit-times
	// 11 bits per char (start, 8 bit chars, 2 stop)
	// t=0	read char
	// t=9	set flag (simulate reading done)
	// t=11	ready to accept next char
	if(pdp->tti_timer < simtime) {
		pdp->tti_timer = simtime + pdp->tty_dly;
		if(pdp->tti_state == 0) {
			if(hasinput(pdp->tty_fd)) {
				char c;
				if(read(pdp->tty_fd, &c, 1) <= 0) {
					close(pdp->tty_fd);
					pdp->tty_fd = -1;
					return;
				}
				pdp->tti = c;
				pdp->tti_busy = 1;
				pdp->tti_flag = 0;
				calc_tty_req(pdp);

				pdp->tti_timer += pdp->tty_dly*8;
				pdp->tti_state = 1;
			}
		} else {
			if(pdp->tti_busy) {
				pdp->tti_busy = 0;
				pdp->tti_flag = 1;
				calc_tty_req(pdp);

				pdp->tti_timer += pdp->tty_dly;
				pdp->tti_state = 0;
			}
		}
	}
}

static IOdev tty_dev = { 0, 0120, nil, handle_tty, cycle_tty };

static void
calc_tty_req(PDP6 *pdp)
{
	if(pdp->tty_pia && (pdp->tti_flag || pdp->tto_flag))
		setreq(pdp, &tty_dev, 0200>>pdp->tty_pia);
	else
		setreq(pdp, &tty_dev, 0);
}

void
attach_tty(PDP6 *pdp)
{
	pdp->tty_baud = 110;
	pdp->tty_dly = 1000000000 / pdp->tty_baud;
	installdev(pdp, &tty_dev);
}
