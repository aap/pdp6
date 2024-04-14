#include "common.h"
#include "../panel/panel6.h"
#include "pdp6.h"

void
updateswitches(PDP6 *pdp, Panel6 *panel)
{
	int sw3 = panel->sw3;

	pdp->key_start = !!(sw3 & KEY_START);
	pdp->key_read_in = !!(sw3 & KEY_READIN);
	pdp->key_inst_cont = !!(sw3 & KEY_INST_CONT);
	pdp->key_mem_cont = !!(sw3 & KEY_MEM_CONT);
	pdp->key_inst_stop = !!(sw3 & KEY_INST_STOP);
	pdp->key_mem_stop = !!(sw3 & KEY_MEM_STOP);
	pdp->key_io_reset = !!(sw3 & KEY_IO_RESET);
	pdp->key_execute = !!(sw3 & KEY_EXEC) && !pdp->run;
	pdp->key_dep = !!(sw3 & KEY_DEP);
	pdp->key_dep_nxt = !!(sw3 & KEY_DEP_NXT);
	pdp->key_ex = !!(sw3 & KEY_EX);
	pdp->key_ex_nxt = !!(sw3 & KEY_EX_NXT);

	pdp->sw_power = !!(panel->sw3 & SW_POWER);
	pdp->sw_mem_disable = !!(sw3 & SW_MEM_DISABLE);
	pdp->sw_addr_stop = !!(sw3 & SW_ADDR_STOP);
	pdp->sw_repeat = !!(sw3 & SW_REPEAT);

	pdp->datasw = panel->sw0 | (Word)panel->sw1<<18;
	pdp->mas = panel->sw2;

	pdp->ptr_tape_feed = !!(panel->sw4 & KEY_PTR_FEED);
	pdp->ptp_tape_feed = !!(panel->sw4 & KEY_PTP_FEED);
	pdp->sw_ptr_motor_on = !!(panel->sw4 & KEY_MOTOR_ON);
	pdp->sw_ptr_motor_off = !!(panel->sw4 & KEY_MOTOR_OFF);

	pdp->key_manual =
		pdp->key_start ||
		pdp->key_read_in ||
		pdp->key_inst_cont ||
		pdp->key_io_reset ||
		pdp->key_execute ||
		pdp->key_ex || pdp->key_ex_nxt ||
		pdp->key_dep || pdp->key_dep_nxt;


	pdp->dotrace = 0;
}

void
updatelights(PDP6 *pdp, Panel6 *panel)
{
	int l;

	panel->lights0 = pdp->mi & 0777777;
	panel->lights1 = (pdp->mi>>18) & 0777777;
	panel->lights2 = pdp->ma & 0777777;
	panel->lights3 = pdp->ir & 0777777;
	panel->lights4 = pdp->pc & 0777777;

	l = pdp->pio;
	if(pdp->run) l |= L5_RUN;
	if(pdp->pi_on) l |= L5_PI_ON;
	if(pdp->mc_stop) l |= L5_MC_STOP;
	if(pdp->sw_mem_disable) l |= L5_MEM_DISABLE;
	if(pdp->sw_addr_stop) l |= L5_ADDR_STOP;
	if(pdp->sw_repeat) l |= L5_REPEAT;
	l |= L5_POWER;
	panel->lights5 = l;

	l = pdp->pir;
	l |= pdp->pih<<7;
	panel->lights6 = l;
}

void
lightsoff(Panel6 *panel)
{
	panel->lights0 = 0;
	panel->lights1 = 0;
	panel->lights2 = 0;
	panel->lights3 = 0;
	panel->lights4 = 0;
	panel->lights5 = 0;
	panel->lights6 = 0;
}

Panel6*
getpanel(void)
{
	return attachseg("/tmp/pdp6_panel", sizeof(Panel6));
}
