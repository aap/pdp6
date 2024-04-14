#include "common.h"
#include "pdp6.h"

#include <unistd.h>

#define IOB pdp->iob
#define PTP_DLY 15797788 // 63.3 chars per second
#define PTR_DLY 2500000	// 400 chars per second



/* PTP */

static void calc_ptp_req(PDP6 *pdp);

static void
handle_ptp(PDP6 *pdp, IOdev *dev, int cmd)
{
	switch(cmd) {
	case IOB_RESET:
	case IOB_CONO_CLR:
		pdp->ptp_b = 0;
		pdp->ptp_busy = 0;
		pdp->ptp_flag = 0;
		pdp->ptp_pia = 0;
		break;

	case IOB_CONO_SET:
		if(IOB & F30) pdp->ptp_b = 1;
		if(IOB & F31) pdp->ptp_busy = 1;
		if(IOB & F32) pdp->ptp_flag = 1;
		pdp->ptp_pia |= IOB & 7;
		break;

	case IOB_STATUS:
		if(pdp->ptp_b) IOB |= F30;
		if(pdp->ptp_busy) IOB |= F31;
		if(pdp->ptp_flag) IOB |= F32;
		IOB |= pdp->ptp_pia;
		return;

	case IOB_DATAO_CLR:
		pdp->ptp_busy = 1;
		pdp->ptp_flag = 0;
		pdp->ptp = 0;
		return;

	case IOB_DATAO_SET:
		pdp->ptp |= IOB & 0377;
		break;
	}
	calc_ptp_req(pdp);
}

static void
ptp_set_motor(PDP6 *pdp, int state)
{
	if(pdp->ptp_motor_on == state)
		return;
	pdp->ptp_motor_on = state;
	if(!pdp->ptp_motor_on)
		pdp->ptp_motor_on_timer = NEVER;
	else
		pdp->ptp_punch_timer = simtime + PTP_DLY;
}

static void
cycle_ptp(PDP6 *pdp, IOdev *dev, int pwr)
{
	u8 c;
	bool go;

	if(!pwr) {
		pdp->ptp_motor_on = 0;
		pdp->ptp_motor_on_timer = NEVER;
		return;
	}

	go = pdp->ptp_busy || pdp->ptp_tape_feed;
	if(go) {
		pdp->ptp_motor_off_timer = simtime + 5000000000;
		if(pdp->ptp_motor_on_timer == NEVER)
			pdp->ptp_motor_on_timer = simtime + 1000000000;
	}

	if(!pdp->ptp_motor_on && pdp->ptp_motor_on_timer < simtime)
		ptp_set_motor(pdp, 1);
	if(pdp->ptp_motor_off_timer < simtime)
		ptp_set_motor(pdp, 0);

	if(!pdp->ptp_motor_on)
		return;
	if(pdp->ptp_punch_timer < simtime) {
		// reluctance pickup
		pdp->ptp_punch_timer = simtime + PTP_DLY;
		if(pdp->ptp_busy)	// PTP READY
			c = (pdp->ptp_b ? pdp->ptp&077|0200 : pdp->ptp);
		else if(pdp->ptp_tape_feed)
			c = 0;
		else
			return;

		if(pdp->ptp_fd >= 0)
			if(write(pdp->ptp_fd, &c, 1) <= 0) {
				close(pdp->ptp_fd);
				pdp->ptp_fd = -1;
			}

		// PTP DONE
		if(pdp->ptp_busy) {
			pdp->ptp_busy = 0;
			pdp->ptp_flag = 1;
			calc_ptp_req(pdp);
		}
	}
}

static IOdev ptp_dev = { 0, 0100, handle_ptp, cycle_ptp };

static void
calc_ptp_req(PDP6 *pdp)
{
	if(pdp->ptp_pia && pdp->ptp_flag)
		setreq(pdp, &ptp_dev, 0200>>pdp->ptp_pia);
	else
		setreq(pdp, &ptp_dev, 0);
}

void
attach_ptp(PDP6 *pdp)
{
	installdev(pdp, &ptp_dev);
}

/* PTR */

static void calc_ptr_req(PDP6 *pdp);

void
ptr_set_motor(PDP6 *pdp, int state)
{
	if(pdp->ptr_motor_on == state)
		return;
	pdp->ptr_motor_on = state;

	if(pdp->ptr_motor_on)
		pdp->ptr_busy = 0;
	pdp->ptr_flag = 1;
	calc_ptr_req(pdp);
}

static void
ptr_set_busy(PDP6 *pdp)
{
	if(!pdp->ptr_busy) {
		// PTR CLR
		pdp->ptr_sr = 0;
		pdp->ptr = 0;
	}
	pdp->ptr_busy = 1;
}

static void
handle_ptr(PDP6 *pdp, IOdev *dev, int cmd)
{
	switch(cmd) {
	case IOB_RESET:
		// hack for easier use. NB: this breaks diagnostics
		ptr_set_motor(pdp, 1);
	case IOB_CONO_CLR:
		pdp->ptr_b = 0;
		pdp->ptr_busy = 0;
		pdp->ptr_flag = 0;
		pdp->ptr_pia = 0;
		break;

	case IOB_CONO_SET:
		if(IOB & F27) ptr_set_motor(pdp, 1);	// not in schematics, can we turn it off too?
		if(IOB & F30) pdp->ptr_b = 1;
		if(IOB & F31) ptr_set_busy(pdp);
		if(IOB & F32) pdp->ptr_flag = 1;
		pdp->ptr_pia |= IOB & 7;
		break;

	case IOB_STATUS:
		if(pdp->ptr_motor_on) IOB |= F27;
		if(pdp->ptr_b) IOB |= F30;
		if(pdp->ptr_busy) IOB |= F31;
		if(pdp->ptr_flag) IOB |= F32;
		IOB |= pdp->ptr_pia;
		return;

	case IOB_DATAI:
		IOB |= pdp->ptr;
		pdp->ptr_flag = 0;
		// actually after DATAI negated
		ptr_set_busy(pdp);
		break;
	}
	calc_ptr_req(pdp);
}

static void
cycle_ptr(PDP6 *pdp, IOdev *dev, int pwr)
{
	bool clutch;
	u8 c;

	if(!pwr) return;

	if(pdp->sw_ptr_motor_on) ptr_set_motor(pdp, 1);
	if(pdp->sw_ptr_motor_off) ptr_set_motor(pdp, 0);

	clutch = pdp->ptr_motor_on && (pdp->ptr_busy | pdp->ptr_tape_feed);
	if(clutch && !pdp->ptr_clutch) {
		// start motion
		pdp->ptr_timer = simtime + PTR_DLY;
	}
	pdp->ptr_clutch = clutch;
	if(!pdp->ptr_clutch || pdp->ptr_timer >= simtime)
		return;
	pdp->ptr_timer = simtime + PTR_DLY;

	if(!hasinput(pdp->ptr_fd))
		return;
	if(read(pdp->ptr_fd, &c, 1) <= 0) {
		close(pdp->ptr_fd);
		pdp->ptr_fd = -1;
		return;
	}
	if(pdp->ptr_busy && (c & 0200 || !pdp->ptr_b)) {
		// PTR STROBE
		// actually 400μs after feed hole edge
		pdp->ptr_sr = (pdp->ptr_sr<<1) | 1;
		pdp->ptr <<= 6;
		if(pdp->ptr_b)
			pdp->ptr |= c & 077;
		else
			pdp->ptr |= c;

		// PTR TRAIL
		// only if busy but that's guaranteed here
		if(!pdp->ptr_b || pdp->ptr_sr & 040) {
			pdp->ptr_busy = 0;
			pdp->ptr_flag = 1;
			calc_ptr_req(pdp);
		}
	}
}

static IOdev ptr_dev = { 0, 0104, nil, handle_ptr, cycle_ptr };

static void
calc_ptr_req(PDP6 *pdp)
{
	if(pdp->ptr_pia && pdp->ptr_flag)
		setreq(pdp, &ptr_dev, 0200>>pdp->ptr_pia);
	else
		setreq(pdp, &ptr_dev, 0);
}

void
attach_ptr(PDP6 *pdp)
{
	installdev(pdp, &ptr_dev);
}
