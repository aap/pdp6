#include "common.h"
#include "panel_pidp10.h"
#include "pdp6.h"

void
updateswitches(PDP6 *pdp, Panel *panel)
{
	int sw3 = panel->sw3;
	int sw4 = panel->sw4;

	// TODO: make sure "opposite" keys are not down at the same time
	pdp->key_start = !!(sw3 & KEY_START);
	pdp->key_read_in = !!(sw3 & KEY_READIN);
	pdp->key_dep = !!(sw3 & KEY_DEP);
	pdp->key_dep_nxt = !!(sw3 & KEY_DEP_NEXT);
	pdp->key_ex = !!(sw3 & KEY_EX);
	pdp->key_ex_nxt = !!(sw3 & KEY_EX_NEXT);
	pdp->key_io_reset = !!(sw3 & KEY_RESET);
	pdp->key_execute = !!(sw3 & KEY_XCT) && !pdp->run;
	pdp->key_mem_stop = !!(sw4 & SW_SING_CYC);
	pdp->key_inst_stop = !!(sw4 & SW_SING_INST) || !!(sw3 & KEY_STOP);

	pdp->sw_power = 1;
	pdp->sw_mem_disable = !!(sw4 & SW_NXM_STOP);
	pdp->sw_addr_stop = !!(sw4 & SW_ADR_STOP);
	pdp->sw_repeat = !!(sw4 & SW_REPT);

	pdp->datasw = panel->sw0 | (Word)panel->sw1<<18;
	pdp->mas = panel->sw2;

	if(pdp->mc_stop) {
		pdp->key_inst_cont = 0;
		pdp->key_mem_cont = !!(sw3 & KEY_CONT);
	} else {
		pdp->key_inst_cont = !!(sw3 & KEY_CONT);
		pdp->key_mem_cont = 0;
	}

	// this sucks a bit
	pdp->ptr_tape_feed = 0;
	pdp->ptp_tape_feed = 0;
	pdp->sw_ptr_motor_on = 0;
	pdp->sw_ptr_motor_off = 0;

	pdp->key_manual =
		pdp->key_start ||
		pdp->key_read_in ||
		pdp->key_inst_cont ||
		pdp->key_io_reset ||
		pdp->key_execute ||
		pdp->key_ex || pdp->key_ex_nxt ||
		pdp->key_dep || pdp->key_dep_nxt;

	pdp->dotrace = !!(sw4 & SW_PAR_STOP);
}

void
updatelights(PDP6 *pdp, Panel *panel)
{
	int l;

	panel->lights0 = pdp->mi & 0777777;
	panel->lights1 = (pdp->mi>>18) & 0777777;
	panel->lights2 = pdp->ma & 0777777;
	panel->lights3 = pdp->ir & 0777777;
	panel->lights4 = pdp->pc & 0777777;

	l = pdp->pio;
	l |= pdp->iob_req << 7;
	l |= L5_POWER;
	if(pdp->mc_stop)
		l |= L5_MC_STOP;
	if(pdp->ex_user)
		l |= L5_USER_MODE;
	panel->lights5 = l;

	l = pdp->pir;
	l |= pdp->pih << 7;
	if(pdp->run)
		l |= L6_RUN;
	if(pdp->pi_on)
		l |= L6_PI_ON;
	panel->lights6 = l;
}

void
lightsoff(Panel *panel)
{
	panel->lights0 = 0;
	panel->lights1 = 0;
	panel->lights2 = 0;
	panel->lights3 = 0;
	panel->lights4 = 0;
	panel->lights5 = 0;
	panel->lights6 = 0;
}

Panel*
getpanel(void)
{
	return attachseg("/tmp/pdp10_panel", sizeof(Panel));
}
