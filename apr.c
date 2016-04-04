#include "pdp6.h"

int dotrace = 1;
Apr apr;

void
trace(char *fmt, ...)
{
	va_list ap;
	if(!dotrace)
		return;
	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	va_end(ap);
}

void
decode_ch(Apr *apr)
{
	int ir6_8;
	ir6_8 = apr->ir>>9 & 7;
	apr->ch_inc = apr->ch_inc_op = apr->ch_n_inc_op = 0;
	apr->ch_load = apr->ch_dep = 0;
	if((apr->ir & 0770000) == 0130000){
		apr->ch_inc = ((ir6_8 & 5) == 4 || ir6_8 == 3) && !apr->chf5;
		apr->ch_inc_op = apr->ch_inc && !apr->chf7;
		apr->ch_n_inc_op = (ir6_8 & 5) == 5 && !apr->chf5 ||
		                  apr->ch_inc && apr->chf7;
		apr->ch_load = (ir6_8 & 6) == 4 && apr->chf5;
		apr->ch_dep = (ir6_8 & 6) == 6 && apr->chf5;
	}
	if(apr->ch_inc_op || apr->ch_n_inc_op || apr->ch_load)
		apr->fac_inh = 1;
	if(apr->ch_n_inc_op || apr->ch_load)
		apr->fc_e = 1;
	if(apr->ch_dep || apr->ch_inc_op)
		apr->fc_e_pse = 1;
	if(apr->ch_dep)
		apr->sac_inh = 1;
}

void
decode_2xx(Apr *apr)
{
	apr->ir_fwt = (apr->inst & 0760) == 0200;
	apr->ir_fwt_swap = 0;
	apr->fwt_00 = apr->fwt_01 = apr->fwt_10 = apr->fwt_11 = 0;
	if(apr->ir_fwt){
		apr->ir_fwt_swap = (apr->inst & 0774) == 0204;
		apr->fwt_00 = (apr->inst & 03) == 0;
		apr->fwt_01 = (apr->inst & 03) == 1;
		apr->fwt_10 = (apr->inst & 03) == 2;
		apr->fwt_11 = (apr->inst & 03) == 3;
		if(apr->fwt_00 || apr->fwt_01 || apr->fwt_11)
			apr->fac_inh = 1;
		if(apr->fwt_00)
			apr->fc_e = 1;
		if(apr->fwt_11)
			apr->fc_e_pse = 1;
		if(apr->fwt_10)
			apr->sac_inh = apr->sc_e = 1;
	}
	apr->fc_c_acrt = (apr->inst & 0776) == 0262;
	apr->fc_c_aclt = apr->inst == 0251 || apr->inst == 0267;

	apr->ir_md = (apr->inst & 0760) == 0220;
	if(apr->ir_md){
		if((apr->inst & 03) != 1)
			apr->fc_e = 1;
		if((apr->inst & 03) == 2)
			apr->sac_inh = 1;
		if(apr->inst & 02)
			apr->sc_e = 1;
		if((apr->inst & 0774) == 0224)
			apr->fac2 = 1;
		if(((apr->inst & 0774) == 0224 || (apr->inst & 0770) == 0230) &&
		   !apr->sac_inh)
			apr->sac2 = 1;
	}

	apr->shift_op = (apr->inst & 0770) == 0240 &&
	               (apr->inst & 03) != 3;
	if(apr->shift_op && apr->inst & 04)
		apr->fac2 = apr->sac2 = 1;

	if(apr->inst == EXCH)
		apr->fc_e_pse = 1;
	if((apr->inst & 0774) == 0254)
		apr->fac_inh = 1;
	if(apr->inst == XCT)
		apr->fac_inh = apr->fc_e = 1;

	apr->ir_jp = (apr->inst & 0770) == 0260;
	if(apr->ir_jp){
		apr->e_long = 1;
		if(apr->inst == PUSHJ)
			apr->sc_e = apr->mb_pc_sto = 1;
		if(apr->inst == PUSH)
			apr->sc_e = apr->fc_e = 1;
		if((apr->inst & 0776) == 0264)	/* JSR, JSP */
			apr->mb_pc_sto = apr->fac_inh = 1;
		if(apr->inst == POP || apr->inst == JSR || apr->inst == JSA)
			apr->sc_e = 1;
	}

	apr->ir_as = (apr->inst & 0770) == 0270;
}

void
decodeir(Apr *apr)
{
	bool iot_a, jrst_a, uuo_a;

	apr->inst = apr->ir>>9 & 0777;
	apr->fac_inh = 0;
	apr->fc_e = 0;
	apr->fc_e_pse = 0;
	apr->e_long = 0;
	apr->mb_pc_sto = 0;
	apr->sc_e = 0;
	apr->sac_inh = 0;
	apr->sac2 = 0;

	decode_ch(apr);

	apr->ir_fp = (apr->inst & 0740) == 0140;
	if(apr->ir_fp){
		apr->fc_e = 1;
		if(apr->inst & 02)	/* M, B */
			apr->sc_e = 1;
		if((apr->inst & 03) == 2)
			apr->sac_inh = 1;
		if((apr->inst & 03) == 1)
			apr->sac2 = 1;
	}

	decode_2xx(apr);

	/* ACCP v MEMAC */
	apr->ir_accp = apr->ir_memac = apr->ir_memac_mem = apr->ir_memac_ac = 0;
	if((apr->inst & 0700) == 0300){
		apr->ir_memac = apr->inst & 060;
		apr->ir_accp = !apr->ir_memac;
		apr->ir_memac_mem = apr->ir_memac && apr->inst & 010;
		apr->ir_memac_ac = apr->ir_memac && !apr->ir_memac_mem;
		if(apr->ir_memac_mem)
			apr->fac_inh = apr->fc_e_pse = 1;
		if(apr->ir_accp && apr->inst & 010)
			apr->fc_e = 1;
		apr->e_long = 1;
		if(apr->ir_accp)
			apr->sac_inh = 1;
	}

	/* BOOLE */
	apr->boole_as_00 = apr->boole_as_01 = 0;
	apr->boole_as_10 = apr->boole_as_11 = 0;
	apr->ir_boole = (apr->inst & 0700) == 0400;
	if(apr->ir_boole)
		apr->ir_boole_op = apr->inst>>2 & 017;

	/* HWT */
	apr->hwt_00 = apr->hwt_01 = apr->hwt_10 = apr->hwt_11 = 0;
	apr->ir_hwt = (apr->inst & 0700) == 0500;
	if(apr->ir_hwt){
		apr->hwt_00 = (apr->inst & 03) == 0;
		apr->hwt_01 = (apr->inst & 03) == 1;
		apr->hwt_10 = (apr->inst & 03) == 2;
		apr->hwt_11 = (apr->inst & 03) == 3;
		if(apr->hwt_00)
			apr->fc_e = 1;
		if(apr->hwt_11)
			apr->fac_inh = 1;
		if(apr->hwt_10 || apr->hwt_11)
			apr->fc_e_pse = 1;
		if(apr->hwt_10)
			apr->sac_inh = 1;
	}

	if(apr->ir_boole || apr->ir_as){
		apr->boole_as_00 = (apr->inst & 03) == 0;
		apr->boole_as_01 = (apr->inst & 03) == 1;
		apr->boole_as_10 = (apr->inst & 03) == 2;
		apr->boole_as_11 = (apr->inst & 03) == 3;
		if(apr->boole_as_00)
			apr->fc_e = 1;
		if(apr->boole_as_10 || apr->boole_as_11)
			apr->fc_e_pse = 1;
		if(apr->boole_as_10)
			apr->sac_inh = 1;
	}

	/* ACBM */
	apr->ir_acbm = (apr->inst & 0700) == 0600;
	if(apr->ir_acbm){
		if(apr->inst & 010)
			apr->fc_e = 1;
		apr->e_long = 1;
		if(!(apr->inst & 60))
			apr->sac_inh = 1;
	}

	if(!(apr->ir & 0740) && (apr->fwt_11 || apr->hwt_11 || apr->ir_memac_mem))
		apr->sac_inh = 1;

	uuo_a = (apr->inst & 0700) == 0;
	iot_a = (apr->inst & 0700) == 0700;
	jrst_a = apr->inst == 0254;
	// 5-13
	apr->ex_ir_uuo =
		uuo_a && apr->ex_uuo_sync ||
		iot_a && !apr->ex_pi_sync && apr->ex_user && !apr->cpa_iot_user ||
		jrst_a && (apr->ir & 0000600) && apr->ex_user;
	apr->ir_jrst = !apr->ex_ir_uuo && jrst_a;
	apr->ir_iot = !apr->ex_ir_uuo && iot_a;
	apr->iot_blk = apr->ir_iot && (apr->ir & 0240) == 0;
	apr->iot_dataio = apr->ir_iot && (apr->ir & 0240) == 0040;

	if(apr->ir_jrst)
		apr->mb_pc_sto = 1;

	if(apr->ex_ir_uuo || apr->ir_iot)
		apr->fac_inh = 1;
	if(apr->iot_blk)
		apr->fc_e_pse = 1;
	if(apr->iot_dataio && apr->ir & 0100)		/* DATAO */
		apr->fc_e = 1;
	/* No need to check PC +1 for E LONG, it's already implied.
	 * We have to wait until ET5 for PC SET */
	if(apr->ir_iot && (apr->inst & 0300) == 0300 ||	/* CONSZ, CONSO */
	   apr->mb_pc_sto)
		apr->e_long = 1;
	if(apr->ir_iot && (apr->ir & 0140) == 040)	/* DATAI, CONI */
		apr->sc_e = 1;
}

void
swap(word *a, word *b)
{
	word tmp;
	tmp = *a;
	*a = *b;
	*b = tmp;
}

bool
accp_et_al_test(Apr *apr)
{
	bool lt, cond;
	// 5-9
	lt = !!(apr->ar & SGN) != apr->ar_cry0_xor_cry1;
	cond = (apr->inst & 2) && apr->ar == 0 ||
               (apr->ir_accp || apr->ir_memac) && (apr->inst & 1) && lt;
	return cond != !!(apr->inst & 040);
}

bool
selected_flag(Apr *apr)
{
	// 5-12
	return apr->ir & 0400 && apr->ar_ov_flag ||
	       apr->ir & 0200 && apr->ar_cry0_flag ||
	       apr->ir & 0100 && apr->ar_cry1_flag ||
	       apr->ir & 0040 && apr->ar_pc_chg_flag;
}

void
selected_flags_clr(Apr *apr)
{
	// 6-10
	if(apr->ir & 0400) apr->ar_ov_flag = 0;
	if(apr->ir & 0200) apr->ar_cry0_flag = 0;
	if(apr->ir & 0100) apr->ar_cry1_flag = 0;
	if(apr->ir & 0040) apr->ar_pc_chg_flag = 0;
}

bool
pc_set_enable(Apr *apr)
{
	// 5-12
	return apr->inst == AOBJP && !(apr->ar & SGN) ||
	       apr->inst == AOBJN && apr->ar & SGN ||
	       apr->ir_memac_ac && accp_et_al_test(apr) ||
	       apr->inst == JFCL && selected_flag(apr);
}

bool
pc_inc_enable(Apr *apr)
{
	// 5-12
	return (apr->ir_accp || apr->ir_acbm || apr->ir_memac_mem) && accp_et_al_test(apr) ||
	        apr->ir_iot && ((apr->inst & 0340) == 0300 && apr->ar == 0 ||
	                      (apr->inst & 0340) == 0340 && apr->ar != 0);
}

void
ar_cry(Apr *apr)
{
	// 6-10
	apr->ar_cry0_xor_cry1 = apr->ar_cry0 != apr->ar_cry1 &&
	                       !((apr->inst & 0700) == 0300 &&
	                         (apr->inst & 0060) != 0020);
}

void
ar_cry_in(Apr *apr, word c)
{
	word a;
	a = (apr->ar & 0377777777777) + c;
	apr->ar += c;
	apr->ar_cry0 = !!(apr->ar & 01000000000000);
	apr->ar_cry1 = !!(a      &  0400000000000);
	apr->ar &= FW;
	ar_cry(apr);
}

void
set_ex_mode_sync(Apr *apr, bool value)
{
	apr->ex_mode_sync = value;
	if(apr->ex_mode_sync)
		apr->ex_user = 1;	// 5-13
}

void
set_pi_cyc(Apr *apr, bool value)
{
	apr->pi_cyc = value;
	if(!apr->pi_cyc)
		apr->ex_pi_sync = 1;	// 5-13
}

void
recalc_pi_req(Apr *apr)
{
	int chan;
	u8 req;
	// 8-3
	apr->pi_req = 0;
	if(apr->pi_active){
		req = apr->pir & ~apr->pih;
		for(chan = 0100; chan; chan >>= 1)
			if(req & chan){
				apr->pi_req = chan;
				break;
			}
	}
}

void
set_mc_rq(Apr *apr, bool value)
{
	apr->mc_rq = value;		// 7-9
	if(value && (apr->mc_rd || apr->mc_wr)){
		membus0 |= MEMBUS_RQ_CYC;
		wakemem();
	}else
		membus0 &= ~MEMBUS_RQ_CYC;
}

void
set_mc_wr(Apr *apr, bool value)
{
	apr->mc_wr = value;		// 7-9
	if(value)
		membus0 |= MEMBUS_WR_RQ;
	else
		membus0 &= ~MEMBUS_WR_RQ;
	set_mc_rq(apr, apr->mc_rq);
}

void
set_mc_rd(Apr *apr, bool value)
{
	apr->mc_rd = value;		// 7-9
	if(value)
		membus0 |= MEMBUS_RD_RQ;
	else
		membus0 &= ~MEMBUS_RD_RQ;
	set_mc_rq(apr, apr->mc_rq);
}

void
set_key_rim_sbr(Apr *apr, bool value)
{
	// not sure if this is correct
	apr->key_rim_sbr = value | apr->sw_rim_maint;	// 5-2
}

bool
calcaddr(Apr *apr)
{
	u8 ma18_25;
	bool ma_ok, ma_fmc_select;

	// 5-13
	apr->ex_inh_rel = !apr->ex_user || apr->ex_pi_sync ||
	                 (apr->ma & 0777760) == 0 || apr->ex_ill_op;
	// 7-4
	ma18_25 = apr->ma>>10 & 0377;
	ma_ok = ma18_25 <= apr->pr;
	// 7-2
	ma_fmc_select = !apr->key_rim_sbr && (apr->ma & 0777760) == 0;
	// 7-5
	apr->rla = ma18_25;
	if(!apr->ex_inh_rel)
		apr->rla += apr->rlr;

	membus0 &= ~0007777777761;
	membus0 |= ma_fmc_select ? MEMBUS_MA_FMC_SEL1 : MEMBUS_MA_FMC_SEL0;
	membus0 |= (apr->ma&01777) << 4;
	membus0 |= (apr->rla&017) << 14;
	membus0 |= apr->rla & 0020 ? MEMBUS_MA21_1|MEMBUS_MA21 : MEMBUS_MA21_0;
	membus0 |= apr->rla & 0040 ? MEMBUS_MA20_1 : MEMBUS_MA20_0;
	membus0 |= apr->rla & 0100 ? MEMBUS_MA19_1 : MEMBUS_MA19_0;
	membus0 |= apr->rla & 0200 ? MEMBUS_MA18_1 : MEMBUS_MA18_0;
	membus0 |= apr->ma & 01 ? MEMBUS_MA35_1 : MEMBUS_MA35_0;
	return ma_ok;
}

pulse(kt4);
pulse(key_rd_wr_ret);
pulse(it1);
pulse(at1);
pulse(at0);
pulse(iat0);
pulse(et10);
pulse(mc_wr_rs);
pulse(mc_rd_rq_pulse);
pulse(mc_wr_rq_pulse);
pulse(mc_rdwr_rq_pulse);
pulse(mc_rd_wr_rs_pulse);

// TODO: find A LONG

pulse(pi_reset){
	trace("PI RESET\n");
	apr->pi_active = 0;	// 8-4
	apr->pih = 0;		// 8-4
	apr->pir = 0;		// 8-4
	recalc_pi_req(apr);	// 8-4
	apr->pio = 0;		// 8-3
}

pulse(ar_flag_clr){
	trace("AR FLAG CLR\n");
	apr->ar_ov_flag = 0;	// 6-10
	apr->ar_cry0_flag = 0;	// 6-10
	apr->ar_cry1_flag = 0;	// 6-10
	apr->ar_pc_chg_flag = 0;	// 6-10
	ar_cry(apr);
	apr->chf7 = 0;		// 6-19
}

pulse(ar_flag_set){
	trace("AR FLAG SET\n");
	apr->ar_ov_flag     = !!(apr->mb & 0400000000000); // 6-10
	apr->ar_cry0_flag   = !!(apr->mb & 0200000000000); // 6-10
	apr->ar_cry1_flag   = !!(apr->mb & 0100000000000); // 6-10
	apr->ar_pc_chg_flag = !!(apr->mb & 0040000000000); // 6-10
	ar_cry(apr);
	apr->chf7           = !!(apr->mb & 0020000000000); // 6-19
	if(apr->mb & 0010000000000)
		set_ex_mode_sync(apr, 1);	// 5-13
}

// TODO
pulse(mp_clr){
	apr->chf5 = 0;		// 6-19
	apr->shf1 = 0;		// 6-20
}

pulse(mr_clr){
	trace("MR CLR\n");
	apr->ir = 0;	// 5-7
	apr->mq = 0;	// 6-13
	apr->sc = 0;	// 6-15
	apr->fe = 0;	// 6-15

	apr->mc_rd = 0;		// 7-9
	apr->mc_wr = 0;		// 7-9
	apr->mc_rq = 0;		// 7-9
	apr->mc_stop = 0;	// 7-9
	apr->mc_stop_sync = 0;	// 7-9
	apr->mc_split_cyc_sync = 0;	// 7-9

	set_ex_mode_sync(apr, 0);	// 5-13
	apr->ex_uuo_sync = 0;	// 5-13
	apr->ex_pi_sync = 0;	// 5-13

	apr->a_long = 0;		// nowhere to be found :(
	apr->ar_com_cont = 0;	// 6-9
	mp_clr(apr);
	// TODO: DS CLR

	/* sbr flip-flops */
	apr->key_rd_wr = 0;	// 5-2
	apr->if1a = 0;		// 5-3
	apr->af0 = 0;		// 5-3
	apr->af3 = 0;		// 5-3
	apr->af3a = 0;		// 5-3
	apr->f1a = 0;		// 5-4
	apr->f4a = 0;		// 5-4
	apr->f6a = 0;		// 5-4
	apr->et4_ar_pse = 0;	// 5-5
	apr->sf3 = 0;		// 5-6
	apr->sf5a = 0;		// 5-6
	apr->sf7 = 0;		// 5-6
	apr->art3_ret = NULL;
	apr->sct2_ret = NULL;
}

pulse(ex_clr){
	apr->pr = 0;		// 5-13, 7-4
	apr->rlr = 0;		// 5-13, 7-5
}

pulse(mr_start){
	trace("MR START\n");
	// IOB RESET
	apr->cpa_iot_user = 0;	// 8-5
	apr->cpa_illeg_op = 0;	// 8-5
	apr->cpa_non_exist_mem = 0;	// 8-5
	apr->cpa_clock_en = 0;	// 8-5
	apr->cpa_clock_flag = 0;	// 8-5
	apr->cpa_pc_chg_en = 0;	// 8-5
	apr->cpa_pdl_ov = 0;	// 8-5
	apr->cpa_arov_en = 0;	// 8-5
	apr->cpa_pia33 = 0;	// 8-5
	apr->cpa_pia34 = 0;	// 8-5
	apr->cpa_pia35 = 0;	// 8-5

	// PI
	apr->pi_ov = 0;		// 8-4
	set_pi_cyc(apr, 0);	// 8-4
	nextpulse(apr, pi_reset);	// 8-4
	ar_flag_clr(apr);	// 6-10

	nextpulse(apr, ex_clr); 
	apr->ex_user = 0;	// 5-13
	apr->ex_ill_op = 0;	// 5-13
	apr->rla = 0;
}

pulse(mr_pwr_clr){
	trace("MR PWR CLR\n");
	apr->run = 0;	// 5-1
	/* order matters because of EX PI SYNC */
	// TODO: is this correct then?
	nextpulse(apr, mr_start);	// 5-2
	nextpulse(apr, mr_clr);		// 5-2
}

pulse(st7){
	trace("ST7\n");
}

/*
 * Shift subroutines
 */

pulse(ar_sh_lt){
}

pulse(mq_sh_lt){
}

pulse(ar_sh_rt){
}

pulse(mq_sh_rt){
}

pulse(sct2){
	trace("SCT2\n");
	nextpulse(apr, apr->sct2_ret);
}

pulse(sct1){
	trace("SCT1\n");
	apr->sc = apr->sc+1 & 0777;	// 6-16
	// TODO: implement shifts
	if(apr->shift_op && !(apr->mb & RSGN)){
		ar_sh_lt(apr);
		mq_sh_lt(apr);
	}
	if(apr->shift_op && (apr->mb & RSGN)){
		ar_sh_rt(apr);
		mq_sh_rt(apr);
	}
	nextpulse(apr, apr->sc == 0777 ? sct2 : sct1);
}

pulse(sct0){
	trace("SCT0\n");
	nextpulse(apr, apr->sc == 0777 ? sct2 : sct1);
}

pulse(sht1a){
	trace("SHT1A\n");
	apr->shf1 = 0;			// 6-20
	nextpulse(apr, et10);
}

pulse(sht1){
	trace("SHT1\n");
	if(apr->mb & 0400000)
		apr->sc = ~apr->sc & 0777;
	apr->shf1 = 1;			// 6-20
	apr->sct2_ret = sht1a;
	nextpulse(apr, sct0);
}

pulse(sht0){
	trace("SHT0\n");
	apr->sc = apr->sc+1 & 0777;	// 6-16
}

/*
 * AR subroutines
 */

pulse(art3){
	trace("ART3\n");
	apr->ar_com_cont = 0;	// 6-9
	nextpulse(apr, apr->art3_ret);
}

pulse(ar_cry_comp){
	trace("AR CRY COMP\n");
	apr->ar = ~apr->ar & FW;	// 6-8
	nextpulse(apr, art3);		// 6-9
}

pulse(ar_pm1_t1){
	trace("AR AR+-1 T1\n");
	ar_cry_in(apr, 1);
	if(apr->iot_blk || apr->inst == 0252 || apr->inst == 0253 ||
	   apr->ir_jp && !(apr->inst & 0004))
		ar_cry_in(apr, 01000000);
	nextpulse(apr, apr->ar_com_cont ? ar_cry_comp : art3);
}

pulse(ar_pm1_t0){
	trace("AR AR+-1 T0\n");
	apr->ar = ~apr->ar & FW;	// 6-8
	apr->ar_com_cont = 1;	// 6-9
	nextpulse(apr, ar_pm1_t1);
}

pulse(ar_negate_t0){
	trace("AR NEGATE T0\n");
	apr->ar = ~apr->ar & FW;	// 6-8
	nextpulse(apr, ar_pm1_t1);
}

pulse(ar_ast2){
	trace("AR2\n");
	ar_cry_in(apr, (~apr->ar & apr->mb) << 1);
	nextpulse(apr, apr->ar_com_cont ? ar_cry_comp : art3);
}

pulse(ar_ast1){
	trace("AR AST1\n");
	apr->ar ^= apr->mb;
	nextpulse(apr, ar_ast2);
}

pulse(ar_ast0){
	trace("AR AST0\n");
	apr->ar = ~apr->ar & FW;	// 6-8
	apr->ar_com_cont = 1;	// 6-9
	nextpulse(apr, ar_ast1);
}

/*
 * Priority Interrupt
 */

pulse(pir_stb){
	trace("PIR STB\n");
	apr->pir |= apr->pio & iobus1;
	recalc_pi_req(apr);			// 8-3
}

pulse(pi_sync){
	trace("PI SYNC\n");
	if(!apr->pi_cyc)
		pir_stb(apr);
	// 5-3
	if(apr->pi_req && !apr->pi_cyc){
		nextpulse(apr, iat0);
		return;
	}
	// TODO: IA INH/AT INH
	nextpulse(apr, apr->if1a ? it1 : at1);
}

/*
 * Store
 */

pulse(st3){
	trace("ST3\n");
	apr->sf3 = 0;
}

pulse(st2){
	trace("ST2\n");
	apr->sf3 = 1;
	nextpulse(apr, mc_rd_wr_rs_pulse);
}

pulse(st1){
	trace("ST1\n");
	apr->sf3 = 1;
	nextpulse(apr, mc_wr_rq_pulse);
}

/*
 * Execute
 */

pulse(et10){
	trace("ET10\n");

	if(apr->pi_hold){
		apr->pi_ov = 0;
		apr->pi_cyc = 0;
	}
	if(apr->ir_jrst || apr->inst == PUSHJ || apr->inst == PUSH)
		apr->ma |= apr->mb & RT;
	if((apr->fc_e_pse || apr->sc_e) &&
	   !(apr->ir_jp || apr->inst == EXCH || apr->ch_dep))
		apr->mb = apr->ar;
	if(apr->ir_jp && !(apr->inst & 04))
		swap(&apr->mb, &apr->ar);
	if(apr->ir_memac || apr->ir_as){
		apr->ar_cry0_flag = apr->ar_cry0;
		apr->ar_cry1_flag = apr->ar_cry1;
	}

	if(apr->hwt_10 || apr->hwt_11 || apr->fwt_10 || apr->fwt_11 ||
	   apr->ir_memac_mem)
		apr->mb = apr->ar;
	if(apr->ir_fwt && !apr->ar_cry0 && apr->ar_cry1 ||
	   (apr->ir_memac || apr->ir_as) && apr->ar_cry0 != apr->ar_cry1){
		apr->ar_ov_flag = 1;			// 6-10
		if(apr->cpa_arov_en)
			;
	}
	if(apr->ir_jp && !(apr->inst & 04) && apr->ar_cry0){
		apr->cpa_pdl_ov = 1;
		;
	}
	if(apr->inst == JFCL)
		selected_flags_clr(apr);
	nextpulse(apr, apr->sc_e     ? st1 :
                       apr->fc_e_pse ? st2 :
                                       st3);
}

pulse(et9){
	bool pc_inc;

	trace("ET9\n");
	pc_inc = apr->inst == JSR || apr->inst == JSA || pc_inc_enable(apr);
	if(pc_inc)
		apr->pc = apr->pc+1 & FW;
	if((apr->pc_set || pc_inc) && !(apr->ir_jrst && apr->ir & 0100)){
		apr->ar_pc_chg_flag = 1;
		if(apr->cpa_pc_chg_en)
			;
	}

	if(apr->ir_acbm || apr->ir_jp && apr->inst != JSR)
		swap(&apr->mb, &apr->ar);
	if(apr->ir_jrst || apr->inst == PUSHJ || apr->inst == PUSH)
		apr->ma = 0;
	if(apr->inst == JSR)
		apr->chf7 = 0;
	nextpulse(apr, et10);
}

pulse(et8){
	trace("ET8\n");
	if(apr->pc_set)
		apr->pc |= apr->ma;	// 5-12
	if(apr->inst == JSR)
		apr->ex_ill_op = 0;
	nextpulse(apr, et9);
}

pulse(et7){
	trace("ET7\n");
	if(apr->pc_set)
		apr->pc = 0;		// 5-12
	if(apr->inst == JSR && (apr->ex_pi_sync || apr->ex_ill_op))
		apr->ex_user = 0;	// 5-13

	if(apr->ir_acbm)
		apr->ar = ~apr->ar;
	nextpulse(apr, et8);
}

pulse(et6){
	trace("ET6\n");
	if(apr->mb_pc_sto || apr->inst == JSA)
		apr->mb |= apr->pc;
	if(apr->inst == PUSHJ || apr->inst == JSR || apr->inst == JSP){
		// 6-4
		if(apr->ar_ov_flag)     apr->mb |= 0400000000000;
		if(apr->ar_cry0_flag)   apr->mb |= 0200000000000;
		if(apr->ar_cry1_flag)   apr->mb |= 0100000000000;
		if(apr->ar_pc_chg_flag) apr->mb |= 0040000000000;
		if(apr->chf7)           apr->mb |= 0020000000000;
		if(apr->ex_user)        apr->mb |= 0010000000000;
	}

	if(apr->ir_acbm && (apr->inst & 060) == 020)	/* Z */
		apr->mb &= apr->ar;
	nextpulse(apr, et7);
}

pulse(et5){
	trace("ET5\n");
	if(apr->mb_pc_sto)
		apr->mb = 0;

	if(apr->ir_acbm)
		apr->ar = ~apr->ar;

	// 5-12
	apr->pc_set = apr->ir_jp && !(apr->inst == PUSH || apr->inst == POP) ||
	             apr->ir_jrst ||
	             pc_set_enable(apr);
	nextpulse(apr, apr->e_long || apr->pc_set ? et6 : et10);
}

pulse(et4){
	bool hwt_lt, hwt_rt;

	trace("ET4\n");
	apr->et4_ar_pse = 0;		// 5-5

	if(apr->ir_boole && (apr->ir_boole_op == 04 ||
	                    apr->ir_boole_op == 010 ||
	                    apr->ir_boole_op == 011 ||
	                    apr->ir_boole_op == 014 ||
	                    apr->ir_boole_op == 015 ||
	                    apr->ir_boole_op == 016 ||
	                    apr->ir_boole_op == 017))
		apr->ar = ~apr->ar & FW;	// 6-8

	hwt_lt = apr->ir_hwt && !(apr->inst & 040);
	hwt_rt = apr->ir_hwt && apr->inst & 040;
	if(hwt_rt && apr->inst & 020 && (!(apr->inst & 010) || apr->mb & RSGN))
		apr->ar = apr->ar & ~LT | ~apr->ar & LT;
	if(hwt_lt && apr->inst & 020 && (!(apr->inst & 010) || apr->mb & SGN))
		apr->ar = apr->ar & ~RT | ~apr->ar & RT;
	if(hwt_lt)
		apr->ar = apr->ar & ~LT | apr->mb & LT;
	if(hwt_rt)
		apr->ar = apr->ar & ~RT | apr->mb & RT;

	if(apr->ir_fwt_swap)
		swap(&apr->mb, &apr->ar);	// 6-3

	if(apr->ir_acbm)
		swap(&apr->mb, &apr->ar);
	nextpulse(apr, et5);
}

pulse(et3){
	trace("ET3\n");

	if(apr->inst == POPJ || apr->inst == BLT)
		apr->ma = apr->mb & RT;		// 7-3

	// 5-9
	if(apr->ir_fwt && ((apr->inst & 0774) == 0210 ||
	                  (apr->inst & 0774) == 0214 && apr->ar & SGN)){
		apr->et4_ar_pse = 1;		// 5-5
		apr->art3_ret = et4;
		nextpulse(apr, ar_negate_t0);
		return;
	}

	if(apr->ir_as){
		apr->et4_ar_pse = 1;		// 5-5
		apr->art3_ret = et4;
		nextpulse(apr, apr->inst & 4 ? ar_ast0 : ar_ast1);
		return;
	}

	if(apr->ir_accp){
		apr->et4_ar_pse = 1;		// 5-5
		apr->art3_ret = et4;
		nextpulse(apr, ar_ast0);
		return;
	}
	if(apr->ir_memac){
		apr->et4_ar_pse = 1;		// 5-5
		apr->art3_ret = et4;
		nextpulse(apr, (apr->inst & 070) == 040 ? ar_pm1_t1 : // +1
                               (apr->inst & 070) == 060 ? ar_pm1_t0 : // -1
                                                          et4);
		return;
	}
	if(apr->inst == AOBJP || apr->inst == AOBJN ||
	   apr->inst == PUSHJ || apr->inst == PUSH){
		apr->et4_ar_pse = 1;		// 5-5
		apr->art3_ret = et4;
		nextpulse(apr, ar_pm1_t1);
		return;
	}
	if(apr->inst == POP || apr->inst == POPJ){
		apr->et4_ar_pse = 1;		// 5-5
		apr->art3_ret = et4;
		nextpulse(apr, ar_pm1_t0);
		return;
	}
	nextpulse(apr, apr->shift_op ? sht1 : et4);
};

pulse(et1){
	trace("ET1\n");

	if(apr->ex_ir_uuo)
		apr->ex_ill_op = 1;		// 5-13
	if(apr->ir_jrst && apr->ir & 040)
		apr->ex_mode_sync = 1;		// 5-13
	if(apr->ir_jrst && apr->ir & 0100)
		ar_flag_set(apr);
	if(apr->ir_jrst && apr->ir & 0400 ||
	   apr->iot_dataio && !apr->pi_ov && apr->pi_cyc)
		if(apr->pi_active){
			// TODO: check if this correct
			apr->pih &= apr->pi_req-1;	// 8-3
			recalc_pi_req(apr);
		}

	if(apr->ir_boole && (apr->ir_boole_op == 06 ||
	                    apr->ir_boole_op == 011 ||
	                    apr->ir_boole_op == 014))
		apr->ar ^= apr->mb;		// 6-8
	if(apr->ir_boole && (apr->ir_boole_op == 01 ||
	                    apr->ir_boole_op == 02 ||
	                    apr->ir_boole_op == 015 ||
	                    apr->ir_boole_op == 016))
		apr->ar &= apr->mb;		// 6-8
	if(apr->ir_boole && (apr->ir_boole_op == 03 ||
	                    apr->ir_boole_op == 04 ||
	                    apr->ir_boole_op == 07 ||
	                    apr->ir_boole_op == 010 ||
	                    apr->ir_boole_op == 013))
		apr->ar |= apr->mb;		// 6-8
	if(apr->ir_fwt_swap ||
	   apr->ir_hwt && apr->inst & 04)
		apr->mb = apr->mb<<18 & LT | apr->mb>>18 & RT;	// 6-3
	if(apr->ir_hwt && apr->inst & 030)
		apr->ar = 0;			// 6-8
	if(apr->inst == POPJ)
		apr->ma = 0;
	if(apr->shift_op && apr->mb & 0400000)
		sht0(apr);			// 6-20

	if(apr->ir_acbm){
		apr->mb &= apr->ar;
		if((apr->inst & 060) == 040)	/* C */
			apr->ar ^= apr->mb;
		if((apr->inst & 060) == 060)	/* O */
			apr->ar |= apr->mb;
	}
	nextpulse(apr, et3);
}

pulse(et0){
	trace("ET0\n");

	if(!((apr->ch_inc_op || apr->ch_n_inc_op) && apr->inst != IBP ||
	     apr->ex_ir_uuo || apr->iot_blk || apr->inst == BLT || apr->inst == XCT ||
	     apr->key_exec && !apr->run ||
	     apr->pi_cyc))
		apr->pc = apr->pc+1 & RT;	// 5-12
	apr->ar_cry0 = 0;	// 6-10
	apr->ar_cry1 = 0;	// 6-10
	ar_cry(apr);
	if(apr->ir_jrst && apr->ir & 0100)
		ar_flag_clr(apr);
	// TODO: subroutines
	nextpulse(apr, et1);
}

pulse(et0a){
	trace("ET0A\n");

	// 8-4
	apr->pi_hold = (!apr->ir_iot || !apr->pi_ov && apr->iot_dataio) &&
	              apr->pi_cyc;
	if(apr->pi_hold){
		apr->pih |= apr->pi_req;	// 8-3
		recalc_pi_req(apr);
	}
	// 5-1
	if(apr->key_ex_sync)
		apr->key_ex_st = 1;
	if(apr->key_dep_sync)
		apr->key_dep_st = 1;
	if(apr->key_inst_stop ||
	   apr->ir_jrst && apr->ir & 0200 && !apr->ex_user)
		apr->run = 0;

	if(apr->ir_boole && (apr->ir_boole_op == 00 ||
	                    apr->ir_boole_op == 03 ||
	                    apr->ir_boole_op == 014 ||
	                    apr->ir_boole_op == 017))
		apr->ar = 0;		// 6-8
	if(apr->ir_boole && (apr->ir_boole_op == 02 ||
	                    apr->ir_boole_op == 04 ||
	                    apr->ir_boole_op == 012 ||
	                    apr->ir_boole_op == 013 ||
	                    apr->ir_boole_op == 015))
		apr->ar = ~apr->ar & FW;	// 6-8
	if(apr->fwt_00 || apr->fwt_11 || apr->hwt_11 || apr->ir_memac_mem)
		apr->ar = apr->mb;	// 6-8
	if(apr->fwt_01 || apr->fwt_10)
		apr->mb = apr->ar;	// 6-3
	if(apr->inst == EXCH || apr->inst == JSP ||
	   apr->hwt_10)
		swap(&apr->mb, &apr->ar);	// 6-3
	if(apr->inst == POP || apr->inst == POPJ || apr->inst == JRA)
		apr->mb = apr->mq;	// 6-3
	if(apr->inst == FSC || apr->shift_op)
		apr->sc |= ~apr->mb & 0377 | ~apr->mb>>9 & 0400;	// 6-15

	if(apr->ir_acbm && apr->inst & 1 ||
	   apr->inst == JSA)
		apr->mb = apr->mb<<18 & LT | apr->mb>>18 & RT;	// 6-3
}

/*
 * Fetch
 */

pulse(ft6a){
	trace("FT6A\n");
	apr->f6a = 0;			// 5-4
	et0a(apr);			// 5-5
	nextpulse(apr, et0);		// 5-5
}

pulse(ft7){
	trace("FT7\n");
	apr->f6a = 1;			// 5-4
	nextpulse(apr, apr->mc_split_cyc_sync ? mc_rd_rq_pulse :
	                                        mc_rdwr_rq_pulse);
}

pulse(ft6){
	trace("FT6\n");
	apr->f6a = 1;			// 5-4
	nextpulse(apr, mc_rd_rq_pulse);
}

pulse(ft5){
	trace("FT5\n");
	apr->ma = apr->mb & RT;		// 7-3
	nextpulse(apr, apr->fc_e ?     ft6 :
                       apr->fc_e_pse ? ft7 :
                                       ft6a);
}

pulse(ft4a){
	trace("FT4A\n");
	apr->f4a = 0;			// 5-4
	apr->ma = 0;			// 7-3
	swap(&apr->mb, &apr->mq);	// 6-3, 6-13
	nextpulse(apr, ft5);		// 5-4
}

pulse(ft4){
	trace("FT4\n");
	apr->mq = apr->mb;		// 6-13
	apr->f4a = 1;			// 5-4
	nextpulse(apr, mc_rd_rq_pulse);
}

pulse(ft3){
	trace("FT3\n");
	apr->ma = apr->mb & RT;		// 7-3
	swap(&apr->mb, &apr->ar);	// 6-3
	nextpulse(apr, ft4);		// 5-4
}

pulse(ft1a){
	trace("FT1A\n");
	apr->f1a = 0;			// 5-4
	if(apr->fac2)
		apr->ma = apr->ma+1 & 017;	// 7-1, 7-5
	else
		apr->ma = 0;		// 7-3
	if(!(apr->fc_c_aclt || apr->fc_c_acrt))
		swap(&apr->mb, &apr->ar);	// 6-3
	if(apr->fc_c_aclt)
		apr->mb = apr->mb<<18 & LT | apr->mb>>18 & RT;	// 6-3
	nextpulse(apr, apr->fac2                        ? ft4 :	// 5-4
                       apr->fc_c_aclt || apr->fc_c_acrt ? ft3 :	// 5-4
                                                          ft5);	// 5-4
}

pulse(ft1){
	trace("FT1\n");
	apr->ma = apr->ir>>5 & 017;	// 7-3
	apr->f1a = 1;			// 5-4
	nextpulse(apr, mc_rd_rq_pulse);
}

pulse(ft0){
	trace("FT0\n");
	nextpulse(apr, apr->fac_inh ? ft5 : ft1);	// 5-4
}

/*
 * Address
 */

pulse(at5){
	trace("AT5\n");
	apr->a_long = 1;			// nowhere to be found :(
	apr->af0 = 1;			// 5-3
	apr->ma = apr->mb & RT;		// 7-3
	apr->ir &= ~037;			// 5-7
	nextpulse(apr, mc_rd_rq_pulse);
}

pulse(at4){
	trace("AT4\n");
	apr->ar &= ~LT;			// 6-8
	// TODO: what is MC DR SPLIT?
	if(apr->sw_addr_stop || apr->key_mem_stop)
		apr->mc_split_cyc_sync = 1;	// 7-9
	nextpulse(apr, apr->ir & 020 ? at5 : ft0);	// 5-3, 5-4
}

pulse(at3a){
	trace("AT3A\n");
	apr->af3a = 0;			// 5-3
	apr->mb = apr->ar;		// 6-3
	nextpulse(apr, at4);		// 5-3
}

pulse(at3){
	trace("AT3\n");
	apr->af3 = 0;			// 5-3
	apr->ma = 0;			// 7-3
	apr->af3a = 1;			// 5-3
	apr->art3_ret = at3a;
	nextpulse(apr, ar_ast1);
}

pulse(at2){
	trace("AT2\n");
	apr->a_long = 1;			// nowhere to be found :(
	apr->ma = apr->ir & 017;		// 7-3
	apr->af3 = 1;			// 5-3
	nextpulse(apr, mc_rd_rq_pulse);
}

pulse(at1){
	trace("AT1\n");
	apr->ex_uuo_sync = 1;		// 5-13
	nextpulse(apr, (apr->ir & 017) == 0 ? at4 : at2);	// 5-3
}

pulse(at0){
	trace("AT0\n");
	apr->ar &= ~RT;			// 6-8
	apr->ar |= apr->mb & RT;		// 6-8
	apr->ir |= apr->mb>>18 & 037;	// 5-7
	decodeir(apr);
	apr->ma = 0;			// 7-3
	apr->af0 = 0;			// 5-3
	nextpulse(apr, pi_sync);	// 8-4
}

/*
 * Instruction
 */

pulse(iat0){
	trace("IAT0\n");
	mr_clr(apr);
	set_pi_cyc(apr, 1);
	nextpulse(apr, it1);
}

pulse(it1a){
	trace("IT1A\n");
	apr->if1a = 0;
	apr->ir |= apr->mb>>18 & 0777740;	// 5-7
	if(apr->ma & 0777760)
		set_key_rim_sbr(apr, 0);	// 5-2
	nextpulse(apr, at0);
}

pulse(it1){
	trace("IT1\n");
	hword n;
	u8 r;
	if(apr->pi_cyc){
		// 7-3
		r = apr->pi_req;
		for(n = 7; !(r & 1); n--, r >>= 1);
		apr->ma = 040 | n<<1;
	}else
		apr->ma = apr->pc;	// 7-3
	if(apr->pi_ov)
		apr->ma = (apr->ma+1)&RT;	// 7-3
	apr->if1a = 1;
	nextpulse(apr, mc_rd_rq_pulse);
}

pulse(it0){
	trace("IT0\n");
	apr->ma = 0;
	mr_clr(apr);
	apr->if1a = 1;			// 5-3
	nextpulse(apr, pi_sync);	// 8-4
}

/*
 * Memory Control
 */

pulse(mc_rs_t1){
	trace("MC RS T1\n");
	set_mc_rd(apr, 0);
	if(apr->key_ex_nxt || apr->key_dep_nxt)
		apr->mi = apr->mb;	// 7-7

	if(apr->key_rd_wr) nextpulse(apr, key_rd_wr_ret);	// 5-2
	if(apr->sf3) nextpulse(apr, st3);
	if(apr->f6a) nextpulse(apr, ft6a);
	if(apr->f4a) nextpulse(apr, ft4a);
	if(apr->f1a) nextpulse(apr, ft1a);
	if(apr->af0) nextpulse(apr, at0);
	if(apr->af3) nextpulse(apr, at3);
	if(apr->if1a) nextpulse(apr, it1a);
}

pulse(mc_rs_t0){
	trace("MC RS T0\n");
	apr->mc_stop = 0;
	nextpulse(apr, mc_rs_t1);
}

pulse(mc_wr_rs){
	trace("MC WR RS\n");
	if(apr->ma == apr->mas)
		apr->mi = apr->mb;	// 7-7
	membus1 = apr->mb;
	membus0 |= MEMBUS_MAI_WR_RS;
	wakemem();
	if(!apr->mc_stop)
		nextpulse(apr, mc_rs_t0);
}

pulse(mc_addr_ack){
	trace("MC ADDR ACK\n");
	set_mc_rq(apr, 0);
	if(!apr->mc_rd && apr->mc_wr)
		nextpulse(apr, mc_wr_rs);
}

pulse(mc_non_exist_rd){
	trace("MC NON EXIST RD\n");
	if(!apr->mc_stop)
		nextpulse(apr, mc_rs_t0);
}

pulse(mc_non_exist_mem_rst){
	trace("MC NON EXIST MEM RST\n");
	nextpulse(apr, mc_addr_ack);
	if(apr->mc_rd)
		nextpulse(apr, mc_non_exist_rd);
}

pulse(mc_non_exist_mem){
	trace("MC NON EXIST MEM\n");
	apr->cpa_non_exist_mem = 1;
	// TODO: IOB PI REQ CPA PIA
	if(!apr->sw_mem_disable)
		nextpulse(apr, mc_non_exist_mem_rst);
}

pulse(mc_illeg_address){
	trace("MC ILLEG ADDRESS\n");
	apr->cpa_illeg_op = 1;
	// TODO: IOB PI REQ CPA PIA
	nextpulse(apr, st7);
}

pulse(mc_rq_pulse){
	trace("MC RQ PULSE\n");
	apr->mc_stop = 0;		// 7-9
	/* catch non-existent memory */
	apr->extpulse |= 4;
	if(calcaddr(apr) == 0 && !apr->ex_inh_rel){
		nextpulse(apr, mc_illeg_address);
		return;
	}
	set_mc_rq(apr, 1);
	if(apr->key_mem_stop || apr->ma == apr->mas && apr->sw_addr_stop){
		apr->mc_stop = 1;	// 7-9
		// TODO: what is this? does it make any sense?
		if(apr->key_mem_cont)
			nextpulse(apr, kt4);
	}
}

pulse(mc_rdwr_rq_pulse){
	trace("MC RD/RW RQ PULSE\n");
	set_mc_rd(apr, 1);		// 7-9
	set_mc_wr(apr, 1);		// 7-9
	apr->mb = 0;
	apr->mc_stop_sync = 1;		// 7-9
	nextpulse(apr, mc_rq_pulse);
}

pulse(mc_rd_rq_pulse){
	trace("MC RD RQ PULSE\n");
	set_mc_rd(apr, 1);		// 7-9
	set_mc_wr(apr, 0);		// 7-9
	apr->mb = 0;
	nextpulse(apr, mc_rq_pulse);
}

pulse(mc_split_rd_rq){
	trace("MC SPLIT RD RQ\n");
	nextpulse(apr, mc_rd_rq_pulse);
}

pulse(mc_wr_rq_pulse){
	trace("MC WR RQ PULSE\n");
	set_mc_rd(apr, 0);		// 7-9
	set_mc_wr(apr, 1);		// 7-9
	nextpulse(apr, mc_rq_pulse);
}

pulse(mc_split_wr_rq){
	trace("MC SPLIT WR RQ\n");
	nextpulse(apr, mc_wr_rq_pulse);
}

pulse(mc_rd_wr_rs_pulse){
	trace("MC RD/WR RS PULSE\n");
	nextpulse(apr, apr->mc_split_cyc_sync ? mc_split_wr_rq : mc_wr_rs);
}

/*
 * Keys
 */

// some helpers, all on 5-1
#define KEY_MANUAL (apr->key_readin || apr->key_start || apr->key_inst_cont ||\
                    apr->key_mem_cont || apr->key_ex || apr->key_dep ||\
                    apr->key_ex_nxt || apr->key_dep_nxt || apr->key_exec ||\
                    apr->key_io_reset)
#define KEY_MA_MAS (apr->key_readin || apr->key_start ||\
                    apr->key_ex_sync || apr->key_dep_sync)
#define KEY_CLR_RIM (!(apr->key_readin || apr->key_inst_cont || apr->key_mem_cont))
#define KEY_EXECUTE (apr->key_exec && !apr->run)
#define KEY_EX_EX_NXT (apr->key_ex_sync || apr->key_ex_nxt)
#define KEY_DP_DP_NXT (apr->key_dep_sync || apr->key_dep_nxt)
#define KEY_EXECUTE_DP_DPNXT (KEY_EXECUTE || KEY_DP_DP_NXT)

pulse(key_rd_wr_ret){
	trace("KEY RD WR RET\n");
	apr->key_rd_wr = 0;	// 5-2
//	apr->ex_ill_op = 0;	// ?? not found on 5-13
	nextpulse(apr, kt4);	// 5-2
}

pulse(key_rd){
	trace("KEY RD\n");
	apr->key_rd_wr = 1;	// 5-2
	nextpulse(apr, mc_rd_rq_pulse);
}

pulse(key_wr){
	trace("KEY WR\n");
	apr->key_rd_wr = 1;	// 5-2
	apr->mb = apr->ar;	// 6-3
	nextpulse(apr, mc_wr_rq_pulse);
}

pulse(key_go){
	trace("KEY GO\n");
	apr->run = 1;		// 5-1
	apr->key_ex_st = 0;	// 5-1
	apr->key_dep_st = 0;	// 5-1
	apr->key_ex_sync = 0;	// 5-1
	apr->key_dep_sync = 0;	// 5-1
	nextpulse(apr, it0);	// 5-3
}

pulse(kt4){
	trace("KT4\n");
	if(apr->run && (apr->key_ex_st || apr->key_dep_st))
		nextpulse(apr, key_go); 	// 5-2
	// TODO check repeat switch
}

pulse(kt3){
	trace("KT3\n");
	if(apr->key_readin || apr->key_start)
		apr->pc |= apr->ma;		// 5-12
	if(KEY_EXECUTE){
		apr->mb = apr->ar;		// 6-3
		nextpulse(apr, it1a);		// 5-3
		nextpulse(apr, kt4);		// 5-2
	}
	if(KEY_EX_EX_NXT)
		nextpulse(apr, key_rd);		// 5-2
	if(KEY_DP_DP_NXT)
		nextpulse(apr, key_wr);		// 5-2
	if(apr->key_start || apr->key_readin || apr->key_inst_cont)
		nextpulse(apr, key_go);		// 5-2
}

pulse(kt2){
	trace("KT2\n");
	if(KEY_MA_MAS)
		apr->ma |= apr->mas;		// 7-1
	if(KEY_EXECUTE_DP_DPNXT)
		apr->ar |= apr->data;		// 5-2
	nextpulse(apr, kt3);	// 5-2
}

pulse(kt1){
	trace("KT1\n");
	if(apr->key_io_reset)
		nextpulse(apr, mr_start);	// 5-2
	if(KEY_MANUAL && !apr->key_mem_cont)
		nextpulse(apr, mr_clr);		// 5-2
	if(KEY_CLR_RIM)
		set_key_rim_sbr(apr, 0);	// 5-2
	if(apr->key_mem_cont && apr->mc_stop)
		nextpulse(apr, mc_rs_t0);	// 7-8
	if(KEY_MANUAL && apr->mc_stop && apr->mc_stop_sync && !apr->key_mem_cont)
		nextpulse(apr, mc_wr_rs);	// 7-8

	if(apr->key_readin)
		set_key_rim_sbr(apr, 1);	// 5-2
	if(apr->key_readin || apr->key_start)
		apr->pc = 0;			// 5-12
	if(KEY_MA_MAS)
		apr->ma = 0;			// 5-2
	if(apr->key_ex_nxt  || apr->key_dep_nxt)
		apr->ma = (apr->ma+1)&RT;	// 5-2
	if(KEY_EXECUTE_DP_DPNXT)
		apr->ar = 0;			// 5-2

	nextpulse(apr, kt2);	// 5-2
}

pulse(kt0a){
	trace("KT0A\n");
	apr->key_ex_st = 0;	// 5-1
	apr->key_dep_st = 0;	// 5-1
	apr->key_ex_sync = apr->key_ex;		// 5-1
	apr->key_dep_sync = apr->key_dep;	// 5-1
	if(!apr->run || apr->key_mem_cont)
		nextpulse(apr, kt1);	// 5-2
}

pulse(kt0){
	trace("KT0\n");
	nextpulse(apr, kt0a);		// 5-2
}

pulse(key_manual){
	trace("KEY MANUAL\n");
	nextpulse(apr, kt0);		// 5-2
}

pulse(mai_addr_ack){
	trace("MAI ADDR ACK\n");
	nextpulse(apr, mc_addr_ack);
}

pulse(mai_rd_rs){
	trace("MAI RD RS\n");
	apr->mb = membus1;
	if(apr->ma == apr->mas)
		apr->mi = apr->mb;	// 7-7
	if(!apr->mc_stop)
		nextpulse(apr, mc_rs_t0);
}

void
nextpulse(Apr *apr, Pulse *p)
{
	if(apr->nnextpulses >= MAXPULSE){
		fprintf(stderr, "error: too many next pulses\n");
		exit(1);
	}
	apr->nlist[apr->nnextpulses++] = p;
}

void*
aprmain(void *p)
{
	Apr *apr;
	Pulse **tmp;
	int i;

	apr = (Apr*)p;
	apr->clist = apr->pulses1;
	apr->nlist = apr->pulses2;
	apr->ncurpulses = 0;
	apr->nnextpulses = 0;

	nextpulse(apr, mr_pwr_clr);
	while(apr->sw_power){
		apr->ncurpulses = apr->nnextpulses;
		apr->nnextpulses = 0;
		tmp = apr->clist; apr->clist = apr->nlist; apr->nlist = tmp;

		for(i = 0; i < apr->ncurpulses; i++)
			apr->clist[i](apr);

		if(apr->extpulse & 1){
			apr->extpulse &= ~1;
			nextpulse(apr, key_manual);
		}
		if(membus0 & MEMBUS_MAI_ADDR_ACK){
			membus0 &= ~MEMBUS_MAI_ADDR_ACK;
			apr->extpulse &= ~4;
			nextpulse(apr, mai_addr_ack);
		}
		if(membus0 & MEMBUS_MAI_RD_RS){
			membus0 &= ~MEMBUS_MAI_RD_RS;
			nextpulse(apr, mai_rd_rs);
		}
		if(apr->extpulse & 4){
			apr->extpulse &= ~4;
			if(apr->mc_rq && !apr->mc_stop)
				nextpulse(apr, mc_non_exist_mem);
		}
	}
	printf("power off\n");
	return NULL;
}
