#include "pdp6.h"
#include "inst.h"

#define DBG_AR print("AR: %012llo\n", apr->ar)
#define DBG_MB print("MB: %012llo\n", apr->mb)
#define DBG_MQ print("MQ: %012llo\n", apr->mq)
#define DBG_MA print("MA: %06o\n", apr->ma)
#define DBG_IR print("IR: %06o\n", apr->ir)

int dotrace = 1;
Apr apr;

void
trace(char *fmt, ...)
{
	va_list ap;
	if(!dotrace)
		return;
	va_start(ap, fmt);
	print("  ");
	vfprintf(stdout, fmt, ap);
	va_end(ap);
}

#define SWAPLTRT(a) (a = (a)<<18 & LT | (a)>>18 & RT)
#define CONS(a, b) ((a)&LT | (b)&RT)

void
swap(word *a, word *b)
{
	word tmp;
	tmp = *a;
	*a = *b;
	*b = tmp;
}

// 6-10
#define AR_OV_SET (apr->ar_cry0 != apr->ar_cry1)
#define AR0_XOR_AROV (!!(apr->ar & F0) != apr->ar_cry0_xor_cry1)
#define AR0_XOR_AR1 (!!(apr->ar & F0) != !!(apr->ar & F1))

/*
 * I'm a bit inconsistent with the decoding.
 * Some things are cached in variables,
 * some are macros...or not even that.
 */

// 5-8
#define IR_FPCH ((apr->inst & 0700) == 0100)

// 6-19
#define CH_INC ((apr->inst == CAO || apr->inst == LDCI || apr->inst == DPCI) && !apr->chf5)
#define CH_INC_OP (CH_INC && !apr->chf7)
#define CH_N_INC_OP ((apr->inst == LDC || apr->inst == DPC) && !apr->chf5 ||\
              CH_INC && apr->chf7)
// 6-20
#define CH_LOAD ((apr->inst == LDCI || apr->inst == LDC) && apr->chf5)
#define CH_DEP ((apr->inst == DPCI || apr->inst == DPC) && apr->chf5)

// 5-8
#define IR_FP ((apr->inst & 0740) == 0140)
#define IR_FAD ((apr->inst & 0770) == 0140)
#define IR_FSB ((apr->inst & 0770) == 0150)
#define IR_FMP ((apr->inst & 0770) == 0160)
#define IR_FDV ((apr->inst & 0770) == 0170)
#define IR_FP_DIR ((apr->inst & 0743) == 0140)
#define IR_FP_REM ((apr->inst & 0743) == 0141)
#define IR_FP_MEM ((apr->inst & 0743) == 0142)
#define IR_FP_BOTH ((apr->inst & 0743) == 0143)

// 5-8
#define FWT_MOVS ((apr->inst & 0770) == 0200)
#define FWT_MOVNM ((apr->inst & 0770) == 0210)
// 5-9
#define FWT_SWAP (apr->ir_fwt && (apr->inst & 014) == 004)
#define FWT_NEGATE (FWT_MOVNM && ((apr->inst & 04) == 0 || apr->ar & SGN))

// 5-8
#define IR_MUL ((apr->inst & 0770) == 0220)
#define IR_DIV ((apr->inst & 0770) == 0230)
#define IR_MD_FC_E (apr->ir_md && (apr->inst & 03) != 1)
#define IR_MD_FAC2 (IR_DIV && apr->ir & H6)
#define IR_MD_SC_E (apr->ir_md && apr->ir & H7)
#define IR_MD_SAC_INH (apr->ir_md && (apr->ir & (H7|H8)) == H7)
#define IR_MD_SAC2 ((IR_DIV || IR_MUL && apr->ir & H6) && !IR_MD_SAC_INH)

// 6-20
#define SH_AC2 (apr->inst == ASHC || apr->inst == ROTC || apr->inst == LSHC)

// 5-10
#define AS_ADD (apr->ir_as && !(apr->ir & H6))
#define AS_SUB (apr->ir_as && apr->ir & H6)

// 5-8
#define IR_254_7 ((apr->inst & 0774) == 0254)
// 6-18
#define BLT_DONE (apr->pi_req || !(apr->mq & F0))
#define BLT_LAST (apr->inst == BLT && !(apr->mq & F0))

// 5-10
#define JP_JMP (apr->ir_jp && apr->inst != PUSH && apr->inst != POP)
#define JP_FLAG_STOR (apr->inst == PUSHJ || apr->inst == JSR || apr->inst == JSP)
// 6-3
#define MB_PC_STO (apr->inst == PUSHJ || apr->inst == JSR || \
                   apr->inst == JSP || apr->ir_jrst)

// 5-9
#define IR_ACCP_MEMAC ((apr->inst & 0700) == 0300)
#define ACCP ((apr->inst & 0760) == 0300)
#define ACCP_DIR (ACCP && apr->inst & 010)
#define MEMAC_TST ((apr->inst & 0760) == 0320)
#define MEMAC_P1 ((apr->inst & 0760) == 0340)
#define MEMAC_M1 ((apr->inst & 0760) == 0360)
#define MEMAC (MEMAC_TST || MEMAC_P1 || MEMAC_M1)
#define MEMAC_MEM (MEMAC && apr->inst & 010)
#define MEMAC_AC (MEMAC && (apr->inst & 010) == 0)
#define ACCP_ETC_COND (IR_ACCP_MEMAC && apr->ir & H8 && AR0_XOR_AROV || \
                       (apr->ir & H7) && apr->ar == 0)
#define ACCP_ET_AL_TEST (ACCP_ETC_COND != !!(apr->ir & H6))

// 5-9
#define HWT_LT (apr->ir_hwt && !(apr->inst & 040))
#define HWT_RT (apr->ir_hwt && apr->inst & 040)
#define HWT_SWAP (apr->ir_hwt && apr->inst & 04)
#define HWT_AR_0 (apr->ir_hwt && apr->inst & 030)
#define HWT_LT_SET (HWT_RT && apr->inst & 020 &&\
                    (!(apr->inst & 010) || apr->mb & RSGN))
#define HWT_RT_SET (HWT_LT && apr->inst & 020 &&\
                    (!(apr->inst & 010) || apr->mb & SGN))

// 5-9
#define ACBM_DIR (apr->ir_acbm && apr->inst & 010)
#define ACBM_SWAP (apr->ir_acbm && apr->inst & 01)
#define ACBM_DN (apr->ir_acbm && (apr->inst & 060) == 000)
#define ACBM_CL (apr->ir_acbm && (apr->inst & 060) == 020)
#define ACBM_COM (apr->ir_acbm && (apr->inst & 060) == 040)
#define ACBM_SET (apr->ir_acbm && (apr->inst & 060) == 060)

// 5-8
#define UUO_A ((apr->inst & 0700) == 0)
#define IOT_A ((apr->inst & 0700) == 0700)
#define JRST_A (apr->inst == JRST)

// 8-1
#define IOT_BLKI (apr->ir_iot && apr->io_inst == BLKI)
#define IOT_DATAI (apr->ir_iot && apr->io_inst == DATAI)
#define IOT_BLKO (apr->ir_iot && apr->io_inst == BLKO)
#define IOT_DATAO (apr->ir_iot && apr->io_inst == DATAO)
#define IOT_CONO (apr->ir_iot && apr->io_inst == CONO)
#define IOT_CONI (apr->ir_iot && apr->io_inst == CONI)
#define IOT_CONSZ (apr->ir_iot && apr->io_inst == CONSZ)
#define IOT_CONSO (apr->ir_iot && apr->io_inst == CONSO)
#define IOT_CONI (apr->ir_iot && apr->io_inst == CONI)
#define IOT_BLK (apr->ir_iot && (apr->io_inst == BLKI || apr->io_inst == BLKO))
#define IOT_OUTGOING (apr->ir_iot && (apr->io_inst == DATAO || apr->io_inst == CONO))
#define IOT_STATUS (apr->ir_iot && \
	(apr->io_inst == CONI || apr->io_inst == CONSZ || apr->io_inst == CONSO))
#define IOT_DATAIO (apr->ir_iot && (apr->io_inst == DATAI || apr->io_inst == DATAO))

void
decodeir(Apr *apr)
{
	apr->inst = apr->ir>>9 & 0777;
	apr->io_inst = apr->ir & 0700340;

	// 5-7
	iobus1 &= ~037777000000;
	iobus1 |= apr->ir & H3 ? IOBUS_IOS3_1 : IOBUS_IOS3_0;
	iobus1 |= apr->ir & H4 ? IOBUS_IOS4_1 : IOBUS_IOS4_0;
	iobus1 |= apr->ir & H5 ? IOBUS_IOS5_1 : IOBUS_IOS5_0;
	iobus1 |= apr->ir & H6 ? IOBUS_IOS6_1 : IOBUS_IOS6_0;
	iobus1 |= apr->ir & H7 ? IOBUS_IOS7_1 : IOBUS_IOS7_0;
	iobus1 |= apr->ir & H8 ? IOBUS_IOS8_1 : IOBUS_IOS8_0;
	iobus1 |= apr->ir & H9 ? IOBUS_IOS9_1 : IOBUS_IOS9_0;

	apr->ir_fp = (apr->inst & 0740) == 0140;

	/* 2xx */
	apr->ir_fwt = FWT_MOVS || FWT_MOVNM;
	apr->fwt_00 = apr->fwt_01 = apr->fwt_10 = apr->fwt_11 = 0;
	if(apr->ir_fwt){
		// 5-9
		apr->fwt_00 = (apr->inst & 03) == 0;
		apr->fwt_01 = (apr->inst & 03) == 1;
		apr->fwt_10 = (apr->inst & 03) == 2;
		apr->fwt_11 = (apr->inst & 03) == 3;
	}
	apr->ir_md = (apr->inst & 0760) == 0220;
	apr->shift_op = (apr->inst & 0770) == 0240 &&
	               (apr->inst & 03) != 3;		// 6-20
	apr->ir_jp = (apr->inst & 0770) == 0260;
	apr->ir_as = (apr->inst & 0770) == 0270;

	/* BOOLE */
	apr->boole_as_00 = apr->boole_as_01 = 0;
	apr->boole_as_10 = apr->boole_as_11 = 0;
	apr->ir_boole = (apr->inst & 0700) == 0400;		// 5-8
	if(apr->ir_boole)	
		apr->ir_boole_op = apr->inst>>2 & 017;

	/* HWT */
	apr->hwt_00 = apr->hwt_01 = apr->hwt_10 = apr->hwt_11 = 0;
	apr->ir_hwt = (apr->inst & 0700) == 0500;	// 5-8
	if(apr->ir_hwt){
		// 5-9
		apr->hwt_00 = (apr->inst & 03) == 0;
		apr->hwt_01 = (apr->inst & 03) == 1;
		apr->hwt_10 = (apr->inst & 03) == 2;
		apr->hwt_11 = (apr->inst & 03) == 3;
	}

	if(apr->ir_boole || apr->ir_as){
		apr->boole_as_00 = (apr->inst & 03) == 0;
		apr->boole_as_01 = (apr->inst & 03) == 1;
		apr->boole_as_10 = (apr->inst & 03) == 2;
		apr->boole_as_11 = (apr->inst & 03) == 3;
	}

	/* ACBM */
	apr->ir_acbm = (apr->inst & 0700) == 0600;	// 5-8

	// 5-13
	apr->ex_ir_uuo =
		UUO_A && apr->ex_uuo_sync ||
		IOT_A && !apr->ex_pi_sync && apr->ex_user && !apr->cpa_iot_user ||
		JRST_A && (apr->ir & 0000600) && apr->ex_user;
	apr->ir_jrst = !apr->ex_ir_uuo && JRST_A;		// 5-8
	apr->ir_iot = !apr->ex_ir_uuo && IOT_A;			// 5-8
}

void recalc_cpa_req(Apr *apr);

void
ar_jfcl_clr(Apr *apr)
{
	// 6-10
	if(apr->ir & H9) apr->ar_ov_flag = 0;
	if(apr->ir & H10) apr->ar_cry0_flag = 0;
	if(apr->ir & H11) apr->ar_cry1_flag = 0;
	if(apr->ir & H12) apr->ar_pc_chg_flag = 0;
	recalc_cpa_req(apr);
}

void
ar_cry(Apr *apr)
{
	// 6-10
	apr->ar_cry0_xor_cry1 = AR_OV_SET && ~MEMAC;
}

void
ar_cry_in(Apr *apr, word c)
{
	word a;
	a = (apr->ar & ~F0) + c;
	apr->ar += c;
	apr->ar_cry0 = !!(apr->ar & FCRY);
	apr->ar_cry1 = !!(a       &  F0);
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
	if(apr->pi_cyc)
		apr->ex_pi_sync = 1;	// 5-13
}

/* get highest priority request above highest currently held */
int
get_pi_req(Apr *apr)
{
	// 8-3
	int chan;
	if(apr->pi_active)
		for(chan = 0100; chan; chan >>= 1)
			if(apr->pih & chan)
				return 0;
			else if(apr->pir & chan)
				return chan;
	return 0;
}

/* clear highest held break */
void
clear_pih(Apr *apr)
{
	// 8-3
	int chan;
	if(apr->pi_active)
		for(chan = 0100; chan; chan >>= 1)
			if(apr->pih & chan){
				apr->pih &= ~chan;
				apr->pi_req = get_pi_req(apr);
				return;
			}
}

void
set_pir(Apr *apr, int pir)
{
	// 8-3
	/* held breaks aren't retriggered */
	apr->pir = pir & ~apr->pih;
	apr->pi_req = get_pi_req(apr);
}

void
set_pih(Apr *apr, int pih)
{
	// 8-3
	apr->pih = pih;
	apr->pir &= ~apr->pih;
	apr->pi_req = get_pi_req(apr);
}

/* Recalculate the PI requests on the IO bus from each device */
void
recalc_req(void)
{
	int i;
	int req;
	req = 0;
	for(i = 0; i < 128; i++)
		req |= ioreq[i];
	iobus1 = iobus1&~0177 | req&0177;
}

void
recalc_cpa_req(Apr *apr)
{
	u8 req = 0;
	// 8-5
	if(apr->cpa_illeg_op || apr->cpa_non_exist_mem || apr->cpa_pdl_ov ||
	   apr->cpa_clock_flag && apr->cpa_clock_enable ||
	   apr->ar_pc_chg_flag && apr->cpa_pc_chg_enable ||
	   apr->ar_ov_flag && apr->cpa_arov_enable)
		req = apr->cpa_pia;
	if(ioreq[0] != req){
		ioreq[0] = req;
		recalc_req();
	}
}

void
set_mc_rq(Apr *apr, bool value)
{
	apr->mc_rq = value;		// 7-9
	if(value && (apr->mc_rd || apr->mc_wr))
		membus0 |= MEMBUS_RQ_CYC;
	else
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
	set_mc_rq(apr, apr->mc_rq);	// 7-9
}

void
set_mc_rd(Apr *apr, bool value)
{
	apr->mc_rd = value;		// 7-9
	if(value)
		membus0 |= MEMBUS_RD_RQ;
	else
		membus0 &= ~MEMBUS_RD_RQ;
	set_mc_rq(apr, apr->mc_rq);	// 7-9
}

void
set_key_rim_sbr(Apr *apr, bool value)
{
	// not sure if this is correct
	apr->key_rim_sbr = value | apr->sw_rim_maint;	// 5-2
}

/* Relocation
 * MA is divided into 8:10 bits, the upper 8 bits (18-25) are relocated in RLA
 * by adding RLR. If MA18-25 is above PR we have a relocation error. */
bool
relocate(Apr *apr)
{
	u8 ma18_25;
	bool ma_ok, ma_fmc_select;

	ma18_25 = apr->ma>>10 & 0377;
	apr->ex_inh_rel = !apr->ex_user ||	// 5-13
	                   apr->ex_pi_sync ||
	                  (apr->ma & 0777760) == 0 ||
                           apr->ex_ill_op;
	ma_ok = ma18_25 <= apr->pr;	// 7-4, PR18 OK
	ma_fmc_select = !apr->key_rim_sbr && (apr->ma & 0777760) == 0;	// 7-2
	// 7-5
	apr->rla = ma18_25;
	if(!apr->ex_inh_rel)
		apr->rla += apr->rlr;

	// 7-2, 7-10
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

pulse(kt1);
pulse(kt4);
pulse(mc_rs_t0);
pulse(mc_addr_ack);
pulse(key_rd_wr_ret);
pulse(it0);
pulse(it1);
pulse(it1a);
pulse(ft0);
pulse(ft1a);
pulse(at0);
pulse(at1);
pulse(at3a);
pulse(iat0);
pulse(et4);
pulse(et5);
pulse(et10);
pulse(st7);
pulse(mc_wr_rs);
pulse(mc_rd_rq_pulse);
pulse(mc_split_rd_rq);
pulse(mc_wr_rq_pulse);
pulse(mc_rdwr_rq_pulse);
pulse(mc_rd_wr_rs_pulse);
pulse(ar_pm1_t1);
pulse(ar_ast0);
pulse(sht1a);
pulse(cht3);
pulse(cht3a);
pulse(cht8a);
pulse(lct0a);
pulse(dct0a);
pulse(pir_stb);

// TODO: find A LONG, it probably doesn't exist

pulse(pi_reset){
	trace("PI RESET\n");
	apr->pi_active = 0;	// 8-4
	apr->pih = 0;		// 8-4
	apr->pir = 0;		// 8-4
	apr->pi_req = get_pi_req(apr);
	apr->pio = 0;		// 8-3
}

pulse(ar_flag_clr){
	trace("AR FLAG CLR\n");
	apr->ar_ov_flag = 0;		// 6-10
	apr->ar_cry0_flag = 0;		// 6-10
	apr->ar_cry1_flag = 0;		// 6-10
	apr->ar_pc_chg_flag = 0;	// 6-10
	apr->chf7 = 0;			// 6-19
	recalc_cpa_req(apr);
}

pulse(ar_flag_set){
	trace("AR FLAG SET\n");
	apr->ar_ov_flag     = !!(apr->mb & F0); // 6-10
	apr->ar_cry0_flag   = !!(apr->mb & F1); // 6-10
	apr->ar_cry1_flag   = !!(apr->mb & F2); // 6-10
	apr->ar_pc_chg_flag = !!(apr->mb & F3); // 6-10
	apr->chf7           = !!(apr->mb & F4); // 6-19
	if(apr->mb & F5)
		set_ex_mode_sync(apr, 1);	// 5-13
	recalc_cpa_req(apr);
}

pulse(mp_clr){
	// 6-19
	apr->chf1 = 0;
	apr->chf2 = 0;
	apr->chf3 = 0;
	apr->chf4 = 0;
	apr->chf5 = 0;
	apr->chf6 = 0;
	apr->lcf1 = 0;		// 6-20
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

	apr->a_long = 0;	// ?? nowhere to be found
	apr->ar_com_cont = 0;	// 6-9
	mp_clr(apr);
	// TODO: DS CLR

	apr->iot_go = 0;	// 8-1

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
	apr->iot_f0a = 0;	// 8-1
	apr->blt_f0a = 0;	// 6-18
	apr->blt_f3a = 0;	// 6-18
	apr->blt_f5a = 0;	// 6-18
	apr->uuo_f1 = 0;	// 5-10
	apr->dcf1 = 0;		// 6-20

	// EX UUO SYNC
	decodeir(apr);
}

pulse(ex_clr){
	apr->pr = 0;		// 7-4
	apr->rlr = 0;		// 7-5
}

pulse(ex_set){
	apr->pr  = iobus0>>28 & 0377;	// 7-4
	apr->rlr = iobus0>>10 & 0377;	// 7-5
}

pulse(mr_start){
	trace("MR START\n");

	// 8-1
	iobus1 |= IOBUS_IOB_RESET;

	// 8-5
	apr->cpa_iot_user = 0;
	apr->cpa_illeg_op = 0;
	apr->cpa_non_exist_mem = 0;
	apr->cpa_clock_enable = 0;
	apr->cpa_clock_flag = 0;
	apr->cpa_pc_chg_enable = 0;
	apr->cpa_pdl_ov = 0;
	apr->cpa_arov_enable = 0;
	apr->cpa_pia = 0;
	ioreq[0] = 0;

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
	/* order matters because of EX PI SYNC,
	 * better call directly before external pulses can trigger stuff */
	// TODO: is this correct then?
	mr_start(apr);	// 5-2
	mr_clr(apr);	// 5-2
}

/* CPA and PI devices */

void
wake_cpa(void)
{
	// 8-5
	if(IOB_STATUS){
		if(apr.cpa_pdl_ov)        iobus0 |= F19;
		if(apr.cpa_iot_user)      iobus0 |= F20;
		if(apr.ex_user)           iobus0 |= F21;
		if(apr.cpa_illeg_op)      iobus0 |= F22;
		if(apr.cpa_non_exist_mem) iobus0 |= F23;
		if(apr.cpa_clock_enable)  iobus0 |= F25;
		if(apr.cpa_clock_flag)    iobus0 |= F26;
		if(apr.cpa_pc_chg_enable) iobus0 |= F28;
		if(apr.ar_pc_chg_flag)    iobus0 |= F29;
		if(apr.cpa_arov_enable)   iobus0 |= F31;
		if(apr.ar_ov_flag)        iobus0 |= F32;
		iobus0 |= apr.cpa_pia & 7;
	}
	if(IOB_CONO_SET){
		if(iobus0 & F18) apr.cpa_pdl_ov = 0;
		if(iobus0 & F19) iobus1 |= IOBUS_IOB_RESET;	// 8-1
		if(iobus0 & F22) apr.cpa_illeg_op = 0;
		if(iobus0 & F23) apr.cpa_non_exist_mem = 0;
		if(iobus0 & F24) apr.cpa_clock_enable = 0;
		if(iobus0 & F25) apr.cpa_clock_enable = 1;
		if(iobus0 & F26) apr.cpa_clock_flag = 0;
		if(iobus0 & F27) apr.cpa_pc_chg_enable = 0;
		if(iobus0 & F28) apr.cpa_pc_chg_enable = 1;
		if(iobus0 & F29) apr.ar_pc_chg_flag = 0;	// 6-10
		if(iobus0 & F30) apr.cpa_arov_enable = 0;
		if(iobus0 & F31) apr.cpa_arov_enable = 1;
		if(iobus0 & F32) apr.ar_ov_flag = 0;		// 6-10
		apr.cpa_pia = iobus0 & 7;
		recalc_cpa_req(&apr);
	}

	// 5-2
	if(IOB_DATAI)
		apr.ar = apr.data;
	// 5-13
	if(IOB_DATAO_CLEAR)
		ex_clr(&apr);
	if(IOB_DATAO_SET)
		ex_set(&apr);
}

void
wake_pi(void)
{
	// 8-4, 8-5
	if(IOB_STATUS){
		trace("PI STATUS %llo\n", iobus0);
		if(apr.pi_active) iobus0 |= F28;
		iobus0 |= apr.pio;
	}

	// 8-4, 8-3
	if(IOB_CONO_CLEAR){
		trace("PI CONO CLEAR %llo\n", iobus0);
		if(iobus0 & F23)
			// can call directly
			pi_reset(&apr);
	}
	if(IOB_CONO_SET){
		trace("PI CONO SET %llo\n", iobus0);
		if(iobus0 & F24)
			set_pir(&apr, apr.pir | iobus0&0177);
		if(iobus0 & F25)
			apr.pio |= iobus0&0177;
		if(iobus0 & F26)
			apr.pio &= ~(iobus0&0177);
		if(iobus0 & F27)
			apr.pi_active = 0;
		if(iobus0 & F28)
			apr.pi_active = 1;
	}
}

/*
 * IOT
 * Here be dragons.
 */

pulse(iot_t4){
	trace("IOT T3A\n");
	/* Clear what was set in IOT T2 */
	iobus1 &= ~(IOBUS_IOB_STATUS | IOBUS_IOB_DATAI);
	iobus0 = 0;
}

pulse(iot_t3a){
	trace("IOT T3A\n");
	/* delay before reset in IOT T4. Otherwise bus will be
	 * reset before bus-pulses triggered by IOT T3 are executed. */
	nextpulse(apr, iot_t4);
}

pulse(iot_t3){
	trace("IOT T3\n");
	// 8-1
	/* Pulses, cleared in the main loop. */
	if(IOT_DATAO)
		iobus1 |= IOBUS_DATAO_SET;
	if(IOT_CONO)
		iobus1 |= IOBUS_CONO_SET;
	apr->ar |= iobus0;		// 6-8
	nextpulse(apr, et5);		// 5-5
}

pulse(iot_t2){
	trace("IOT T2\n");
	// 8-1
	apr->iot_go = 0;
	/* These are asserted during INIT SETUP, IOT T2 and FINAL SETUP.
	 * We clear them in IOT T3A which happens after FINAL SETUP */
	if(IOT_OUTGOING)
		iobus0 |= apr->ar;
	if(IOT_STATUS)
		iobus1 |= IOBUS_IOB_STATUS;
	if(IOT_DATAI)
		iobus1 |= IOBUS_IOB_DATAI;
	/* Pulses, cleared in the main loop. */
	if(IOT_DATAO)
		iobus1 |= IOBUS_DATAO_CLEAR;
	if(IOT_CONO)
		iobus1 |= IOBUS_CONO_CLEAR;
	nextpulse(apr, iot_t3);
	nextpulse(apr, iot_t3a);
}

pulse(iot_t0a){
	trace("IOT T0A\n");
	apr->iot_f0a = 0;			// 8-1
	apr->ir |= H12;				// 5-8
	decodeir(apr);
	apr->ma = 0;				// 7-3
	if(apr->pi_cyc && apr->ar_cry0)
		apr->pi_ov = 1;			// 8-4
	if(!apr->pi_cyc && !apr->ar_cry0)
		apr->pc = apr->pc+1 & RT;	// 5-12
	nextpulse(apr, ft0);			// 5-4
}

pulse(iot_t0){
	trace("IOT T0\n");
	apr->iot_f0a = 1;			// 8-1
	nextpulse(apr, mc_rd_wr_rs_pulse);	// 7-8
}

/*
 * UUO subroutine
 */

pulse(uuo_t2){
	trace("UUO T2\n");
	apr->if1a = 1;			// 5-3
	nextpulse(apr, mc_rd_rq_pulse);	// 7-8
}

pulse(uuo_t1){
	trace("UUO T1\n");
	apr->uuo_f1 = 0;		// 5-10
	apr->ma = apr->ma+1 & RT;	// 7-3
	nextpulse(apr, mr_clr);		// 5-2
	nextpulse(apr, uuo_t2);		// 5-10
}

/*
 * BLT subroutine
 */

pulse(blt_t6){
	trace("BLT T6\n");
	swap(&apr->mb, &apr->ar);	// 6-3
	nextpulse(apr, ft1a);		// 5-4
}

pulse(blt_t5a){
	trace("BLT T5A\n");
	apr->blt_f5a = 0;			// 6-18
	if(!(apr->mq & F0))
		apr->pc = apr->pc+1 & RT;	// 5-12
	nextpulse(apr, BLT_DONE ? et10 : blt_t6);	// 5-5, 6-18
}

pulse(blt_t5){
	trace("BLT T5\n");
	swap(&apr->mb, &apr->mq);	// 6-17
	apr->blt_f5a = 1;		// 6-18
	nextpulse(apr, ar_pm1_t1);	// 6-9
}

pulse(blt_t4){
	trace("BLT T4\n");
	swap(&apr->mb, &apr->ar);	// 6-3
	nextpulse(apr, pir_stb);	// 8-4
	nextpulse(apr, blt_t5);		// 6-18
}

pulse(blt_t3a){
	trace("BLT T3A\n");
	apr->blt_f3a = 0;		// 6-18
	swap(&apr->mb, &apr->mq);	// 6-17
	nextpulse(apr, blt_t4);		// 6-18
}

pulse(blt_t3){
	trace("BLT T3\n");
	apr->blt_f3a = 1;		// 6-18
	nextpulse(apr, ar_ast0);	// 6-9
}

pulse(blt_t2){
	trace("BLT T2\n");
	apr->ar &= RT;			// 6-8
	nextpulse(apr, blt_t3);		// 6-18
}

pulse(blt_t1){
	trace("BLT T1\n");
	swap(&apr->mb, &apr->ar);	// 6-3
	nextpulse(apr, blt_t2);		// 6-18
}

pulse(blt_t0a){
	trace("BLT T0A\n");
	apr->blt_f0a = 0;		// 6-18
	apr->mb = apr->mq;		// 6-3
	nextpulse(apr, blt_t1);		// 6-18
}

pulse(blt_t0){
	trace("BLT T0\n");
	swap(&apr->mb, &apr->mq);	// 6-17
	apr->blt_f0a = 1;		// 6-18
	nextpulse(apr, mc_wr_rq_pulse);	// 7-8
}

/*
 * Shift subroutines
 */

// 6-14
#define SC_COM apr->sc = ~apr->sc & 0777
#define SC_INC apr->sc = apr->sc+1 & 0777
#define SC_DATA (apr->chf1 ? ~apr->mb>>30 & 077 | 0700 : \
                 apr->chf2 ? apr->mb>>24 & 077 : \
                 0)	// TODO
#define SC_PAD apr->sc ^= SC_DATA
#define SC_CRY apr->sc += (~apr->sc & SC_DATA) << 1
// 6-7
#define SHC_ASHC (apr->inst == ASHC || 0) // TODO
#define SHC_DIV (IR_DIV || IR_FDV || 0) // TODO

// TODO
#define MS_MULT 0

/* Shift counter */

// 6-7, 6-17
#define AR_SH_LT apr->ar = apr->ar<<1 & 0377777777776 | ar0_shl_inp | ar35_shl_inp
#define MQ_SH_LT apr->mq = apr->mq<<1 & 0377777777776 | mq0_shl_inp | mq35_shl_inp
#define AR_SH_RT apr->ar = apr->ar>>1 & 0377777777777 | ar0_shr_inp
#define MQ_SH_RT apr->mq = apr->mq>>1 & 0177777777777 | mq0_shr_inp | mq1_shr_inp

pulse(sct2){
	trace("SCT2\n");
	if(apr->shf1) nextpulse(apr, sht1a);	// 6-20
	if(apr->chf4) nextpulse(apr, cht8a);	// 6-19
	if(apr->lcf1) nextpulse(apr, lct0a);	// 6-20
	if(apr->dcf1) nextpulse(apr, dct0a);	// 6-20
}

pulse(sct1){
	word ar0_shl_inp, ar0_shr_inp, ar35_shl_inp;
	word mq0_shl_inp, mq0_shr_inp, mq1_shr_inp, mq35_shl_inp;
	trace("SCT1\n");
	SC_INC;		// 6-16

	// 6-7 What a mess, and I don't really understand the schematics
	if(apr->inst != ASH && !SHC_ASHC)
		ar0_shl_inp = (apr->ar & F1) << 1;
	else
		ar0_shl_inp = apr->ar & F0;

	if(apr->inst == ROTC)
		ar0_shr_inp = (apr->mq & F35) << 35;
	else if(apr->inst == ROT)
		ar0_shr_inp = (apr->ar & F35) << 35;
	else if(IR_DIV)
		ar0_shr_inp = (~apr->mq & F35) << 35;
	else if(apr->inst == LSH || apr->inst == LSHC || CH_LOAD)
		ar0_shr_inp = 0;
	else if(apr->inst == ASH || SHC_ASHC || MS_MULT || IR_FDV)
		ar0_shr_inp = apr->ar & F0;

	if(apr->inst == ROT)
		ar35_shl_inp = (apr->ar & F0) >> 35;
	else if(apr->inst == ASHC)
		ar35_shl_inp = (apr->mq & F1) >> 34;
	else if(apr->inst == ROTC || apr->inst == LSHC || SHC_DIV)
		ar35_shl_inp = (apr->mq & F0) >> 35;
	else if(CH_DEP || apr->inst == LSH || apr->inst == ASH)
		ar35_shl_inp = 0;

	if(SHC_ASHC){
		mq0_shl_inp = apr->ar & F0;
		mq1_shr_inp = (apr->ar & F35) << 34;
	}else{
		mq0_shl_inp = (apr->mq & F1) << 1;
		mq1_shr_inp = (apr->mq & F0) >> 1;
	}

	if(MS_MULT && apr->sc == 0777 || SHC_ASHC)
		mq0_shr_inp = apr->ar & F0;
	else
		mq0_shr_inp = (apr->ar & F35) << 35;

	if(apr->inst == ROTC)
		mq35_shl_inp = (apr->ar & F0) >> 35;
	else if(SHC_DIV)
		mq35_shl_inp = (~apr->ar & F0) >> 35;
	else if(CH_N_INC_OP || CH_INC_OP)
		mq35_shl_inp = 1;
	else if(apr->inst == LSHC || SHC_ASHC || CH_DEP)
		mq35_shl_inp = 0;

	// TODO
	// 6-17
	if(apr->shift_op && !(apr->mb & F18)){
		AR_SH_LT;
		MQ_SH_LT;
	}
	if(apr->shift_op && (apr->mb & F18)){
		AR_SH_RT;
		MQ_SH_RT;
	}
	if(apr->chf4)
		MQ_SH_LT;
	if(apr->lcf1)
		AR_SH_RT;
	if(apr->dcf1){
		AR_SH_LT;
		MQ_SH_LT;
	}

	if(!(apr->mb & F18) && (apr->inst == ASH || apr->inst == ASHC) && AR0_XOR_AR1){
		apr->ar_ov_flag = 1;			// 6-10
		recalc_cpa_req(apr);
	}
	nextpulse(apr, apr->sc == 0777 ? sct2 : sct1);	// 6-15, 6-16
}

pulse(sct0){
	trace("SCT0\n");
	nextpulse(apr, apr->sc == 0777 ? sct2 : sct1);	// 6-15, 6-16
}

/* Shift adder */

pulse(sat3){
	trace("SAT3\n");
	if(apr->chf2) nextpulse(apr, cht3a);	// 6-19
}

pulse(sat2_1){
	trace("SAT2,1\n");
	SC_CRY;			// 6-15
	nextpulse(apr, sat3);	// 6-16
}

pulse(sat2){
	trace("SAT2\n");
	nextpulse(apr, sat2_1);	// 6-16
}

pulse(sat1){
	trace("SAT1\n");
	SC_PAD;			// 6-15
	nextpulse(apr, sat2);	// 6-16
}

pulse(sat0){
	trace("SAT0\n");
	apr->chf1 = 0;		// 6-19
	nextpulse(apr, sat1);	// 6-16
}

/*
 * Shift operations subroutine
 */

pulse(sht1a){
	trace("SHT1A\n");
	apr->shf1 = 0;			// 6-20
	nextpulse(apr, et10);		// 5-5
}

pulse(sht1){
	trace("SHT1\n");
	if(apr->mb & F18)
		SC_COM;			// 6-15
	apr->shf1 = 1;			// 6-20
	nextpulse(apr, sct0);		// 6-16
}

pulse(sht0){
	trace("SHT0\n");
	SC_INC;	// 6-16
}

/*
 * Character subroutines
 */

pulse(dct3){
	trace("DCT3\n");
	apr->mb &= apr->ar;		// 6-3
	apr->chf7 = 0;			// 6-19
	nextpulse(apr, et10);		// 5-5
}

pulse(dct2){
	trace("DCT2\n");
	apr->ar = ~apr->ar & FW;	// 6-17
	nextpulse(apr, dct3);		// 6-20
}

pulse(dct1){
	trace("DCT1\n");
	apr->ar &= apr->mb;		// 6-8
	apr->mb = apr->mq;		// 6-17
	nextpulse(apr, dct2);		// 6-20
}

pulse(dct0a){
	trace("DCT0A\n");
	apr->dcf1 = 0;			// 6-20
	swap(&apr->mb, &apr->mq);	// 6-17, 6-13 (dct0b)
	apr->ar = ~apr->ar & FW;	// 6-17
	nextpulse(apr, dct1);		// 6-20
}

pulse(dct0){
	trace("DCT0\n");
	apr->dcf1 = 1;			// 6-20
	SC_COM;				// 6-15
	nextpulse(apr, sct0);		// 6-16
}

pulse(lct0a){
	trace("LCT0A\n");
	apr->lcf1 = 0;			// 6-20
	apr->ar &= apr->mb;		// 6-8
	apr->chf7 = 0;			// 6-19
	nextpulse(apr, et10);		// 5-5
}

pulse(lct0){
	trace("LCT0\n");
	apr->ar = apr->mb;		// 6-8
	apr->mb = apr->mq;		// 6-17
	SC_COM;				// 6-15
	apr->lcf1 = 1;			// 6-20
	nextpulse(apr, sct0);		// 6-16
}

pulse(cht9){
	trace("CHT9\n");
	apr->sc = apr->fe;		// 6-15
	apr->chf5 = 1;			// 6-19
	apr->chf7 = 1;			// 6-19
	nextpulse(apr, at0);		// 5-3
}

pulse(cht8a){
	trace("CHT8A\n");
	apr->chf4 = 0;			// 6-19
	apr->sc = 0;			// 6-15
	apr->ir &= ~037;		// 5-7
	nextpulse(apr, cht9);
}

pulse(cht8b){
	trace("CHT8B\n");
	apr->chf2 = 0;			// 6-19
	apr->chf6 = 0;			// 6-19
	apr->fe = apr->mb>>30 & 077;	// 6-14, 6-15
	SC_COM;				// 6-15
	if(apr->inst == CAO)
		nextpulse(apr, st7);	// 5-6
	else{
		apr->chf4 = 1;		// 6-19
		nextpulse(apr, sct0);	// 6-16
	}
}

pulse(cht8){
	trace("CHT8\n");
	apr->chf6 = 1;		// 6-19
	nextpulse(apr, mc_rd_wr_rs_pulse);	// 7-8
}

pulse(cht7){
	trace("CHT7\n");
	SC_PAD;				// 6-15
	if(CH_INC_OP){
		swap(&apr->mb, &apr->ar);	// 6-17
		nextpulse(apr, cht8);		// 6-19
	}
	if(CH_N_INC_OP)
		nextpulse(apr, cht8b);		// 6-19
}

pulse(cht6){
	trace("CHT6\n");
	apr->ar = apr->ar & 0007777777777 |
		(((word)apr->sc & 077) << 30);	// 6-9, 6-4
	if(CH_INC_OP)
		apr->sc = 0;		// 6-15
	apr->chf2 = 1;			// 6-19
	nextpulse(apr, cht7);		// 6-19
}

pulse(cht5){
	trace("CHT5\n");
	SC_COM;				// 6-15
	nextpulse(apr, cht6);		// 6-19
}

pulse(cht4a){
	trace("CHT4A\n");
	apr->chf3 = 0;			// 6-19
	apr->sc |= 0433;
	nextpulse(apr, cht3);		// 6-19
}

pulse(cht4){
	trace("CHT4\n");
	apr->sc = 0;			// 6-15
	apr->chf3 = 1;			// 6-19
	nextpulse(apr, ar_pm1_t1);	// 6-9
}

pulse(cht3a){
	trace("CHT3A\n");
	apr->chf2 = 0;			// 6-19
	nextpulse(apr, apr->sc & 0400 ? cht5 : cht4);	// 6-19
}

pulse(cht3){
	trace("CHT3\n");
	apr->chf2 = 1;			// 6-19
	nextpulse(apr, sat0);		// 6-16
}

pulse(cht2){
	trace("CHT2\n");
	SC_PAD;				// 6-15
	nextpulse(apr, cht3);		// 6-19
}

pulse(cht1){
	trace("CHT1\n");
	apr->ar = apr->mb;		// 6-8
	apr->chf1 = 1;			// 6-19
	nextpulse(apr, cht2);		// 6-19
}

/*
 * AR subroutines
 */

// 6-9
#define AR_SUB (AS_SUB || ACCP)
#define AR_ADD (AS_ADD)
#define AR_P1 (MEMAC_P1 || apr->inst == PUSH || apr->inst == PUSHJ || \
               IOT_BLK || apr->inst == AOBJP || apr->inst == AOBJN)
#define AR_M1 (MEMAC_M1 || apr->inst == POP || apr->inst == POPJ)
#define AR_PM1_LTRT (IOT_BLK || apr->inst == AOBJP || apr->inst == AOBJN || \
	            apr->ir_jp && !(apr->inst & 0004))
#define AR_SBR (FWT_NEGATE || AR_ADD || AR_SUB || AR_P1 || AR_M1 || IR_FSB)

pulse(art3){
	trace("ART3\n");
	apr->ar_com_cont = 0;		// 6-9

	if(apr->af3a) nextpulse(apr, at3a);		// 5-3
	if(apr->et4_ar_pse) nextpulse(apr, et4);	// 5-5
	if(apr->blt_f3a) nextpulse(apr, blt_t3a);	// 6-18
	if(apr->blt_f5a) nextpulse(apr, blt_t5a);	// 6-18
	if(apr->chf3) nextpulse(apr, cht4a);		// 6-19
}

pulse(ar_cry_comp){
	trace("AR CRY COMP\n");
	if(apr->ar_com_cont){
		apr->ar = ~apr->ar & FW;	// 6-8
		nextpulse(apr, art3);		// 6-9
	}
}

pulse(ar_pm1_t1){
	trace("AR AR+-1 T1\n");
	ar_cry_in(apr, 1);			// 6-6
	if(apr->inst == BLT || AR_PM1_LTRT)
		ar_cry_in(apr, 01000000);	// 6-9
	if(!apr->ar_com_cont)
		nextpulse(apr, art3);			// 6-9
	nextpulse(apr, ar_cry_comp);			// 6-9
}

pulse(ar_pm1_t0){
	trace("AR AR+-1 T0\n");
	apr->ar = ~apr->ar & FW;	// 6-8
	apr->ar_com_cont = 1;		// 6-9
	nextpulse(apr, ar_pm1_t1);	// 6-9
}

pulse(ar_negate_t0){
	trace("AR NEGATE T0\n");
	apr->ar = ~apr->ar & FW;	// 6-8
	nextpulse(apr, ar_pm1_t1);	// 6-9
}

pulse(ar_ast2){
	trace("AR2\n");
	ar_cry_in(apr, (~apr->ar & apr->mb) << 1);	// 6-8
	if(!apr->ar_com_cont)
		nextpulse(apr, art3);			// 6-9
	nextpulse(apr, ar_cry_comp);			// 6-9
}

pulse(ar_ast1){
	trace("AR AST1\n");
	apr->ar ^= apr->mb;		// 6-8
	nextpulse(apr, ar_ast2);	// 6-9
}

pulse(ar_ast0){
	trace("AR AST0\n");
	apr->ar = ~apr->ar & FW;	// 6-8
	apr->ar_com_cont = 1;		// 6-9
	nextpulse(apr, ar_ast1);	// 6-9
}


pulse(xct_t0){
	trace("XCT T0\n");
	nextpulse(apr, mr_clr);		// 5-2
	nextpulse(apr, it1a);		// 5-3
}

/*
 * Priority Interrupt
 */

// 5-3
#define IA_NOT_INT ((!apr->pi_req || apr->pi_cyc) && !apr->ia_inh)
// 8-4
#define PI_BLK_RST (!apr->pi_ov && IOT_DATAIO)
#define PI_RST (apr->ir_jrst && apr->ir & H9 || PI_BLK_RST && apr->pi_cyc)
#define PI_HOLD ((!apr->ir_iot || PI_BLK_RST) && apr->pi_cyc)

pulse(pir_stb){
	trace("PIR STB\n");
	set_pir(apr, apr->pir | apr->pio & iobus1);	// 8-3
}

pulse(pi_sync){
	trace("PI SYNC\n");
	/* Call directly, we need the result in this pulse */
	if(!apr->pi_cyc)
		pir_stb(apr);			// 8-4
	if(apr->pi_req && !apr->pi_cyc)
		nextpulse(apr, iat0);		// 5-3
	if(IA_NOT_INT)
		nextpulse(apr, apr->if1a ? it1 : at1);	// 5-3
	/* no longer needed */
	apr->ia_inh = 0;
}

// 5-1
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

/*
 * Store
 */

// 5-6
#define SC_E (apr->inst == PUSHJ || apr->inst == PUSH || apr->inst == POP || \
              apr->inst == JSR || apr->inst == JSA || IOT_DATAI || IOT_CONI || \
              apr->fwt_10 || IR_MD_SC_E || IR_FP_MEM || IR_FP_BOTH)
#define SAC0 ((apr->ir & 0740) == 0)
#define SAC_INH_IF_AC_0 (SAC0 && (apr->fwt_11 || apr->hwt_11 || MEMAC_MEM))
#define SAC_INH (apr->boole_as_10 || apr->hwt_10 || BLT_LAST || ACCP || \
                 apr->inst == JSR || apr->fwt_10 || ACBM_DN || apr->ir_iot || \
                 IR_MD_SAC_INH || CH_DEP || IR_FP_MEM || IR_254_7 || \
                 SAC_INH_IF_AC_0)
#define SAC2 (SH_AC2 || IR_FP_REM || IR_MD_SAC2)

pulse(st7){
	trace("ST7\n");
	trace("\n");
	apr->sf7 = 0;		// 5-6
	if(apr->run)
		nextpulse(apr, apr->key_ex_st || apr->key_dep_st ? kt1 : it0);	// 5-1, 5-3
	if(apr->key_start || apr->key_readin || apr->key_inst_cont)
		nextpulse(apr, kt4);	// 5-2
}

pulse(st6a){
	trace("ST6A\n");
	apr->sf7 = 1;		// 5-6
}

pulse(st6){
	trace("ST6\n");
	/* We know SAC2 is asserted
	 * so clamp to fast memory addresses */
	apr->ma = apr->ma+1 & 017;	// 7-3
	apr->mb = apr->mq;		// 6-3
	nextpulse(apr, st6a);		// 5-6
	nextpulse(apr, mc_wr_rq_pulse);	// 7-8
}

pulse(st5a){
	trace("ST5A\n");
	apr->sf5a = 0;				// 5-6
	nextpulse(apr, SAC2 ? st6 : st7);	// 5-6
}

pulse(st5){
	trace("ST5\n");
	apr->sf5a = 1;			// 5-6
	apr->ma |= apr->ir>>5 & 017;	// 7-3
	apr->mb = apr->ar;		// 6-3
	nextpulse(apr, mc_wr_rq_pulse);	// 7-8
}

pulse(st3a){
	trace("ST3A\n");
	nextpulse(apr, st5);
}

pulse(st3){
	trace("ST3\n");
	apr->sf3 = 0;			// 5-6
	if(SAC_INH)
		nextpulse(apr, st7);	// 5-6
	else{
		apr->ma = 0;		// 7-3
		nextpulse(apr, st3a);	// 5-6
	}
}

pulse(st2){
	trace("ST2\n");
	apr->sf3 = 1;		// 5-6
	nextpulse(apr, mc_rd_wr_rs_pulse);	// 7-8
}

pulse(st1){
	trace("ST1\n");
	apr->sf3 = 1;		// 5-6
	nextpulse(apr, mc_wr_rq_pulse);	// 7-8
}

/*
 * Execute
 */

// 5-5
#define ET4_INH (apr->inst == BLT || apr->inst == XCT || apr->ex_ir_uuo || \
                 apr->shift_op || AR_SBR || apr->ir_md || IR_FPCH)
#define ET5_INH (apr->ir_iot || IR_FSB)
// 5-12
#define PC_P1_INH (KEY_EXECUTE || (CH_INC_OP || CH_N_INC_OP) && apr->inst != CAO || \
                   apr->inst == XCT || apr->ex_ir_uuo || apr->pi_cyc || \
                   IOT_BLK || apr->inst == BLT)
// 5-12
#define JFCL_FLAGS (apr->ir & 0400 && apr->ar_ov_flag || \
                    apr->ir & 0200 && apr->ar_cry0_flag || \
                    apr->ir & 0100 && apr->ar_cry1_flag || \
                    apr->ir & 0040 && apr->ar_pc_chg_flag)
#define PC_SET_ENABLE (MEMAC_AC && ACCP_ET_AL_TEST || \
	               apr->inst == AOBJN && apr->ar & SGN || \
	               apr->inst == AOBJP && !(apr->ar & SGN) || \
	               apr->inst == JFCL && JFCL_FLAGS)
#define PC_INC_ENABLE ((MEMAC_MEM || ACCP || apr->ir_acbm) && ACCP_ET_AL_TEST || \
	               IOT_CONSO && apr->ar != 0 || IOT_CONSZ && apr->ar == 0)
#define PC_SET (PC_SET_ENABLE || JP_JMP || apr->ir_jrst)
#define PC_INC_ET9 (apr->inst == JSR || apr->inst == JSA || PC_INC_ENABLE)
// 5-5
#define E_LONG (IOT_CONSZ || apr->ir_jp || apr->ir_acbm || apr->pc_set || \
                MB_PC_STO || PC_INC_ET9 || IOT_CONSO || IR_ACCP_MEMAC)

pulse(et10){
	bool sc_e = SC_E;
	trace("ET10\n");

	if(PI_HOLD){
		// 8-4
		apr->pi_ov = 0;
		apr->pi_cyc = 0;
	}
	if(apr->inst == PUSH || apr->inst == PUSHJ || apr->ir_jrst)
		apr->ma |= apr->mb & RT;		// 7-3
	if((apr->fc_e_pse || sc_e) &&
	   !(apr->ir_jp || apr->inst == EXCH || CH_DEP))
		apr->mb = apr->ar;			// 6-3
	if(apr->ir_jp && !(apr->ir & H6))
		swap(&apr->mb, &apr->ar);		// 6-3
	if(MEMAC || apr->ir_as){
		// 6-10
		apr->ar_cry0_flag = apr->ar_cry0;
		apr->ar_cry1_flag = apr->ar_cry1;
	}

	if(apr->ir_fwt && !apr->ar_cry0 && apr->ar_cry1 ||
	   (MEMAC || apr->ir_as) && AR_OV_SET){
		apr->ar_ov_flag = 1;			// 6-10
		recalc_cpa_req(apr);
	}
	if(apr->ir_jp && !(apr->ir & H6) && apr->ar_cry0){
		apr->cpa_pdl_ov = 1;			// 8-5
		recalc_cpa_req(apr);
	}
	if(apr->inst == JFCL)
		ar_jfcl_clr(apr);			// 6-10
	nextpulse(apr, sc_e          ? st1 :		// 5-6
                       apr->fc_e_pse ? st2 :		// 5-6
                                       st3);		// 5-6
}

pulse(et9){
	bool pc_inc;

	trace("ET9\n");
	pc_inc = PC_INC_ET9;
	if(pc_inc)
		apr->pc = apr->pc+1 & RT;	// 5-12
	if((apr->pc_set || pc_inc) && !(apr->ir_jrst && apr->ir & H11)){
		apr->ar_pc_chg_flag = 1;	// 6-10
		recalc_cpa_req(apr);
	}

	if(apr->ir_acbm || apr->ir_jp && apr->inst != JSR)
		swap(&apr->mb, &apr->ar);	// 6-3
	if(apr->inst == PUSH || apr->inst == PUSHJ || apr->ir_jrst)
		apr->ma = 0;			// 7-3
	if(apr->inst == JSR)
		apr->chf7 = 0;			// 6-19
	nextpulse(apr, et10);			// 5-5
}

pulse(et8){
	trace("ET8\n");
	if(apr->pc_set)
		apr->pc |= apr->ma;	// 5-12
	if(apr->inst == JSR)
		apr->ex_ill_op = 0;	// 5-13
	nextpulse(apr, et9);		// 5-5
}

pulse(et7){
	trace("ET7\n");
	if(apr->pc_set)
		apr->pc = 0;		// 5-12
	if(apr->inst == JSR && (apr->ex_pi_sync || apr->ex_ill_op))
		apr->ex_user = 0;	// 5-13
	if(apr->ir_acbm)
		apr->ar = ~apr->ar & FW;	// 6-8
	nextpulse(apr, et8);			// 5-5
}

pulse(et6){
	trace("ET6\n");
	if(MB_PC_STO || apr->inst == JSA)
		apr->mb |= apr->pc;	// 6-3
	if(ACBM_CL)
		apr->mb &= apr->ar;	// 6-3
	if(JP_FLAG_STOR){		// 6-4
		// 6-4
		if(apr->ar_ov_flag)     apr->mb |= F0;
		if(apr->ar_cry0_flag)   apr->mb |= F1;
		if(apr->ar_cry1_flag)   apr->mb |= F2;
		if(apr->ar_pc_chg_flag) apr->mb |= F3;
		if(apr->chf7)           apr->mb |= F4;
		if(apr->ex_user)        apr->mb |= F5;
	}
	if(IOT_CONSZ || IOT_CONSO)
		apr->ar &= apr->mb;	// 6-8
	nextpulse(apr, et7);		// 5-5
}

pulse(et5){
	trace("ET5\n");
	if(MB_PC_STO)
		apr->mb = 0;			// 6-3
	if(apr->ir_acbm)
		apr->ar = ~apr->ar & FW;	// 6-8
	apr->pc_set = PC_SET;
	nextpulse(apr, E_LONG ? et6 : et10);	// 5-5
}

pulse(et4){
	trace("ET4\n");
	apr->et4_ar_pse = 0;		// 5-5
	if(IOT_BLK)
		apr->ex_ill_op = 0;	// 5-13

	// 6-8
	if(apr->ir_boole && (apr->ir_boole_op == 04 ||
	                     apr->ir_boole_op == 010 ||
	                     apr->ir_boole_op == 011 ||
	                     apr->ir_boole_op == 014 ||
	                     apr->ir_boole_op == 015 ||
	                     apr->ir_boole_op == 016 ||
	                     apr->ir_boole_op == 017))
		apr->ar = ~apr->ar & FW;
	if(HWT_LT || IOT_CONO)
		apr->ar = CONS(apr->mb, apr->ar);
	if(HWT_RT)
		apr->ar = CONS(apr->ar, apr->mb);
	if(HWT_LT_SET)
		apr->ar = CONS(~apr->ar, apr->ar);
	if(HWT_RT_SET)
		apr->ar = CONS(apr->ar, ~apr->ar);

	if(FWT_SWAP || IOT_BLK || apr->ir_acbm)
		swap(&apr->mb, &apr->ar);	// 6-3

	if(IOT_BLK)
		nextpulse(apr, iot_t0);		// 8-1
	else if(apr->ir_iot)
		apr->iot_go = 1;		// 8-1

	if(!ET5_INH)
		nextpulse(apr, et5);		// 5-5
}

pulse(et3){
	trace("ET3\n");

	if(apr->ex_ir_uuo){
		// MBLT <- IR(1) (UUO T0) on 6-3
		apr->mb |= (apr->ir&0777740) << 18;	// 6-1
		apr->ma |= F30;				// 7-3
		apr->uuo_f1 = 1;			// 5-10
		nextpulse(apr, mc_wr_rq_pulse);		// 7-8
	}

	if(apr->inst == POPJ || apr->inst == BLT)
		apr->ma = apr->mb & RT;		// 7-3

	// AR SBR, 6-9
	if(FWT_NEGATE || IR_FSB)
		nextpulse(apr, ar_negate_t0);
	if(AR_SUB)
		nextpulse(apr, ar_ast0);
	if(AR_ADD)
		nextpulse(apr, ar_ast1);
	if(AR_M1)
		nextpulse(apr, ar_pm1_t0);
	if(AR_P1)
		nextpulse(apr, ar_pm1_t1);

	if(apr->inst == BLT)
		nextpulse(apr, blt_t0);		// 6-18
	if(apr->shift_op)
		nextpulse(apr, sht1);		// 6-20
	if(apr->inst == XCT)
		nextpulse(apr, xct_t0);		// 5-10
	if(AR_SBR)
		apr->et4_ar_pse = 1;		// 5-5
	if(!ET4_INH)
		nextpulse(apr, et4);		// 5-5
};

pulse(et1){
	trace("ET1\n");

	if(apr->ex_ir_uuo){
		apr->ex_ill_op = 1;		// 5-13
		apr->mb &= RT;			// 6-3
	}
	if(apr->ir_jrst && apr->ir & H12)
		apr->ex_mode_sync = 1;		// 5-13
	if(apr->ir_jrst && apr->ir & H11)
		ar_flag_set(apr);		// 6-10
	if(PI_RST)
		clear_pih(apr);			// 8-4

	// 6-8
	if(apr->ir_boole && (apr->ir_boole_op == 06 ||
	                     apr->ir_boole_op == 011 ||
	                     apr->ir_boole_op == 014) ||
	   ACBM_COM)
		apr->ar ^= apr->mb;
	if(apr->ir_boole && (apr->ir_boole_op == 01 ||
	                     apr->ir_boole_op == 02 ||
	                     apr->ir_boole_op == 015 ||
	                     apr->ir_boole_op == 016))
		apr->ar &= apr->mb;
	if(apr->ir_boole && (apr->ir_boole_op == 03 ||
	                     apr->ir_boole_op == 04 ||
	                     apr->ir_boole_op == 07 ||
	                     apr->ir_boole_op == 010 ||
	                     apr->ir_boole_op == 013) ||
	   ACBM_SET)
		apr->ar |= apr->mb;
	if(HWT_AR_0 || IOT_STATUS || IOT_DATAI)
		apr->ar = 0;

	// 6-3
	if(apr->ir_acbm)
		apr->mb &= ~apr->ar;
	if(HWT_SWAP || FWT_SWAP || apr->inst == BLT)
		SWAPLTRT(apr->mb);

	if(apr->inst == POPJ || apr->ex_ir_uuo || apr->inst == BLT)
		apr->ma = 0;			// 7-3
	if(apr->shift_op && apr->mb & F18)
		nextpulse(apr, sht0);		// 6-20
	if(apr->inst == FSC && !(apr->ar & F0))
		SC_COM;				// 6-15
	nextpulse(apr, et3);
}

pulse(et0){
	trace("ET0\n");

	if(!PC_P1_INH)
		apr->pc = apr->pc+1 & RT;	// 5-12
	apr->ar_cry0 = 0;	// 6-10
	apr->ar_cry1 = 0;	// 6-10
	ar_cry(apr);
	if(apr->ir_jrst && apr->ir & H11)
		ar_flag_clr(apr);		// 6-10
	if(CH_INC_OP)
		nextpulse(apr, cht1);		// 6-19
	if(CH_N_INC_OP)
		nextpulse(apr, cht6);		// 6-19
	if(CH_LOAD)
		nextpulse(apr, lct0);		// 6-20
	if(CH_DEP)
		nextpulse(apr, dct0);		// 6-20
	// TODO: subroutines
}

pulse(et0a){
	static int gen = 0;
	trace("ET0A\n");
	print("%o: ", apr->pc);
	if((apr->inst & 0700) != 0700)
		print("%d %s\n", gen++, names[apr->inst]);
	else
		print("%d %s\n", gen++, ionames[apr->io_inst>>5 & 7]);

	if(PI_HOLD)
		set_pih(apr, apr->pi_req);	// 8-3, 8-4
	if(apr->key_ex_sync)
		apr->key_ex_st = 1;		// 5-1
	if(apr->key_dep_sync)
		apr->key_dep_st = 1;		// 5-1
	if(apr->key_inst_stop ||
	   apr->ir_jrst && apr->ir & H10 && !apr->ex_user)
		apr->run = 0;			// 5-1

	if(apr->ir_boole && (apr->ir_boole_op == 00 ||
	                     apr->ir_boole_op == 03 ||
	                     apr->ir_boole_op == 014 ||
	                     apr->ir_boole_op == 017))
		apr->ar = 0;			// 6-8
	if(apr->ir_boole && (apr->ir_boole_op == 02 ||
	                     apr->ir_boole_op == 04 ||
	                     apr->ir_boole_op == 012 ||
	                     apr->ir_boole_op == 013 ||
	                     apr->ir_boole_op == 015))
		apr->ar = ~apr->ar & FW;	// 6-8
	if(apr->fwt_00 || apr->fwt_11 || apr->hwt_11 || MEMAC_MEM ||
	   IOT_BLK || IOT_DATAO)
		apr->ar = apr->mb;		// 6-8
	if(apr->fwt_01 || apr->fwt_10 || IOT_STATUS)
		apr->mb = apr->ar;		// 6-3
	if(apr->hwt_10 || apr->inst == JSP || apr->inst == EXCH || 
	   apr->inst == BLT || IR_FSB)
		swap(&apr->mb, &apr->ar);	// 6-3
	if(apr->inst == POP || apr->inst == POPJ || apr->inst == JRA)
		apr->mb = apr->mq;		// 6-3
	if(ACBM_SWAP || IOT_CONO || apr->inst == JSA)
		SWAPLTRT(apr->mb);		// 6-3
	if(apr->inst == FSC || apr->shift_op)
		apr->sc |= ~apr->mb & 0377 | ~apr->mb>>9 & 0400;	// 6-15
}

/*
 * Fetch
 *
 * After this stage we have:
 * AR = 0,E or (AC)
 * MQ = 0; (AC+1) or ((AC)LT|RT) if fetched
 * MB = [0?],E or (E)
 */

// 5-4
#define FAC_INH (apr->hwt_11 || apr->fwt_00 || apr->fwt_01 || apr->fwt_11 || \
                 apr->inst == XCT || apr->ex_ir_uuo || \
                 apr->inst == JSP || apr->inst == JSR || \
                 apr->ir_iot || IR_254_7 || MEMAC_MEM || \
                 CH_LOAD || CH_INC_OP || CH_N_INC_OP)
#define FAC2 (SH_AC2 || IR_MD_FAC2)
#define FC_C_ACRT (apr->inst == POP || apr->inst == POPJ)
#define FC_C_ACLT (apr->inst == JRA || apr->inst == BLT)
#define FC_E (apr->hwt_00 || apr->fwt_00 || apr->inst == XCT || \
              apr->inst == PUSH || IOT_DATAO || apr->ir_fp || \
              IR_MD_FC_E || CH_LOAD || CH_N_INC_OP || \
              ACCP_DIR || ACBM_DIR || apr->boole_as_00)
#define FC_E_PSE (apr->hwt_10 || apr->hwt_11 || apr->fwt_11 || \
                  IOT_BLK || apr->inst == EXCH || CH_DEP || CH_INC_OP || \
                  MEMAC_MEM || apr->boole_as_10 || apr->boole_as_11)

pulse(ft7){
	trace("FT7\n");
	apr->f6a = 1;			// 5-4
	nextpulse(apr, apr->mc_split_cyc_sync ? mc_split_rd_rq :	// 7-8
	                                        mc_rdwr_rq_pulse);	// 7-8
}

pulse(ft6a){
	trace("FT6A\n");
	apr->f6a = 0;			// 5-4
	nextpulse(apr, et0a);		// 5-5
	nextpulse(apr, et0);		// 5-5
	nextpulse(apr, et1);		// 5-5
}

pulse(ft6){
	trace("FT6\n");
	apr->f6a = 1;			// 5-4
	nextpulse(apr, mc_rd_rq_pulse);	// 7-8
}

pulse(ft5){
	trace("FT5\n");
	// cache this because we need it again in ET10
	apr->fc_e_pse = FC_E_PSE;
	apr->ma = apr->mb & RT;		// 7-3
	nextpulse(apr, FC_E          ? ft6 :	// 5-4
                       apr->fc_e_pse ? ft7 :	// 5-4
                                       ft6a);	// 5-4
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
	nextpulse(apr, mc_rd_rq_pulse);	// 7-8
}

pulse(ft3){
	trace("FT3\n");
	apr->ma = apr->mb & RT;		// 7-3
	swap(&apr->mb, &apr->ar);	// 6-3
	nextpulse(apr, ft4);		// 5-4
}

pulse(ft1a){
	trace("FT1A\n");
	bool acltrt = FC_C_ACLT || FC_C_ACRT;
	bool fac2 = FAC2;
	apr->f1a = 0;				// 5-4
	if(fac2)
		apr->ma = apr->ma+1 & 017;	// 7-1, 7-3
	else
		apr->ma = 0;			// 7-3
	if(!acltrt)
		swap(&apr->mb, &apr->ar);	// 6-3
	if(FC_C_ACLT)
		SWAPLTRT(apr->mb);		// 6-3
	nextpulse(apr, fac2   ? ft4 :	// 5-4
                       acltrt ? ft3 :	// 5-4
                              ft5);	// 5-4
}

pulse(ft1){
	trace("FT1\n");
	apr->ma |= apr->ir>>5 & 017;	// 7-3
	apr->f1a = 1;			// 5-4
	nextpulse(apr, mc_rd_rq_pulse);	// 7-8
}

pulse(ft0){
	trace("FT0\n");
	nextpulse(apr, FAC_INH ? ft5 : ft1);	// 5-4
}

/*
 * Address
 */

pulse(at5){
	trace("AT5\n");
//	apr->a_long = 1;		// ?? nowhere to be found
	apr->af0 = 1;			// 5-3
	apr->ma |= apr->mb & RT;	// 7-3
	apr->ir &= ~037;		// 5-7
	nextpulse(apr, mc_rd_rq_pulse);	// 7-8
}

pulse(at4){
	trace("AT4\n");
	apr->ar &= ~LT;			// 6-8
	// TODO: what is MC DR SPLIT? what happens here anyway?
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
	nextpulse(apr, ar_ast1);	// 6-9
}

pulse(at2){
	trace("AT2\n");
//	apr->a_long = 1;		// ?? nowhere to be found
	apr->ma |= apr->ir & 017;	// 7-3
	apr->af3 = 1;			// 5-3
	nextpulse(apr, mc_rd_rq_pulse);	// 7-8
}

pulse(at1){
	trace("AT1\n");
	apr->ex_uuo_sync = 1;		// 5-13
	// decode here because of EX UUO SYNC
	decodeir(apr);
	nextpulse(apr, (apr->ir & 017) == 0 ? at4 : at2);	// 5-3
}

pulse(at0){
	trace("AT0\n");
	apr->ar &= ~RT;			// 6-8
	apr->ar |= apr->mb & RT;	// 6-8
	apr->ir |= apr->mb>>18 & 037;	// 5-7
	apr->ma = 0;			// 7-3
	apr->af0 = 0;			// 5-3
	nextpulse(apr, pi_sync);	// 8-4
}

/*
 * Instruction
 */

pulse(it1a){
	trace("IT1A\n");
	apr->if1a = 0;				// 5-3
	apr->ir |= apr->mb>>18 & 0777740;	// 5-7
	if(apr->ma & 0777760)
		set_key_rim_sbr(apr, 0);	// 5-2
	nextpulse(apr, at0);			// 5-3
}

pulse(it1){
	trace("IT1\n");
	if(apr->pi_cyc){
		// 7-3, 8-4
		apr->ma |= 040;
		if(apr->pi_req & 0017) apr->ma |= 010;
		if(apr->pi_req & 0063) apr->ma |= 004;
		if(apr->pi_req & 0125) apr->ma |= 002;
	}else
		apr->ma |= apr->pc;		// 7-3
	if(apr->pi_ov)
		apr->ma = (apr->ma+1)&RT;	// 7-3
	apr->if1a = 1;				// 5-3
	nextpulse(apr, mc_rd_rq_pulse);		// 7-8
}

pulse(iat0){
	trace("IAT0\n");
	// have to call directly because PI CYC sets EX PI SYNC
	mr_clr(apr);			// 5-2
	set_pi_cyc(apr, 1);		// 8-4
	nextpulse(apr, it1);		// 5-3
}

pulse(it0){
	trace("IT0\n");
	apr->ma = 0;			// 7-3
	// have to call directly because IF1A is set with a delay
	mr_clr(apr);			// 5-2
	apr->if1a = 1;			// 5-3
	nextpulse(apr, pi_sync);	// 8-4
}

/*
 * Memory Control
 */

pulse(mai_addr_ack){
	trace("MAI ADDR ACK\n");
	nextpulse(apr, mc_addr_ack);	// 7-8
}

pulse(mai_rd_rs){
	trace("MAI RD RS\n");
	/* we do this here instead of whenever MC RD is set; 7-6, 7-9 */
	apr->mb = membus1;
	if(apr->ma == apr->mas)
		apr->mi = apr->mb;		// 7-7
	if(!apr->mc_stop)
		nextpulse(apr, mc_rs_t0);	// 7-8
}

pulse(mc_rs_t1){
	trace("MC RS T1\n");
	set_mc_rd(apr, 0);			// 7-9
	if(apr->key_ex_nxt || apr->key_dep_nxt)
		apr->mi = apr->mb;		// 7-7

	if(apr->key_rd_wr) nextpulse(apr, key_rd_wr_ret);	// 5-2
	if(apr->sf7) nextpulse(apr, st7);			// 5-6
	if(apr->sf5a) nextpulse(apr, st5a);			// 5-6
	if(apr->sf3) nextpulse(apr, st3);			// 5-6
	if(apr->f6a) nextpulse(apr, ft6a);			// 5-4
	if(apr->f4a) nextpulse(apr, ft4a);			// 5-4
	if(apr->f1a) nextpulse(apr, ft1a);			// 5-4
	if(apr->af0) nextpulse(apr, at0);			// 5-3
	if(apr->af3) nextpulse(apr, at3);			// 5-3
	if(apr->if1a) nextpulse(apr, it1a);			// 5-3
	if(apr->iot_f0a) nextpulse(apr, iot_t0a);		// 8-1
	if(apr->blt_f0a) nextpulse(apr, blt_t0a);		// 6-18
	if(apr->blt_f3a) nextpulse(apr, blt_t3a);		// 6-18
	if(apr->blt_f5a) nextpulse(apr, blt_t5a);		// 6-18
	if(apr->uuo_f1) nextpulse(apr, uuo_t1);			// 5-10
	if(apr->chf6) nextpulse(apr, cht8b);			// 6-19
}

pulse(mc_rs_t0){
	trace("MC RS T0\n");
//	apr->mc_stop = 0;		// ?? not found on 7-9
	nextpulse(apr, mc_rs_t1);	// 7-8
}

pulse(mc_wr_rs){
	trace("MC WR RS\n");
	if(apr->ma == apr->mas)
		apr->mi = apr->mb;	// 7-7
	membus1 = apr->mb;		// 7-8
	membus0 |= MEMBUS_WR_RS;	// 7-8
	if(!apr->mc_stop)
		nextpulse(apr, mc_rs_t0);	// 7-8
}

pulse(mc_addr_ack){
	trace("MC ADDR ACK\n");
	set_mc_rq(apr, 0);			// 7-9
	if(!apr->mc_rd && apr->mc_wr)
		nextpulse(apr, mc_wr_rs);	// 7-8
}

pulse(mc_non_exist_rd){
	trace("MC NON EXIST RD\n");
	if(!apr->mc_stop)
		nextpulse(apr, mc_rs_t0);		// 7-8
}

pulse(mc_non_exist_mem_rst){
	trace("MC NON EXIST MEM RST\n");
	nextpulse(apr, mc_addr_ack);			// 7-8
	if(apr->mc_rd)
		nextpulse(apr, mc_non_exist_rd);	// 7-9
}

pulse(mc_non_exist_mem){
	trace("MC NON EXIST MEM\n");
	apr->cpa_non_exist_mem = 1;			// 8-5
	recalc_cpa_req(apr);
	if(!apr->sw_mem_disable)
		nextpulse(apr, mc_non_exist_mem_rst);	// 7-9
}

pulse(mc_illeg_address){
	trace("MC ILLEG ADDRESS\n");
	apr->cpa_illeg_op = 1;				// 8-5
	recalc_cpa_req(apr);
	nextpulse(apr, st7);				// 5-6
}

pulse(mc_stop_1){
	trace("MC STOP <- (1)\n");
	apr->mc_stop = 1;		// 7-9
	if(apr->key_mem_cont)
		nextpulse(apr, kt4);	// 5-2
}

pulse(mc_rq_pulse){
	bool ma_ok;
	trace("MC RQ PULSE\n");
	/* have to call this to set flags, do relocation and set address */
	ma_ok = relocate(apr);
	apr->mc_stop = 0;		// 7-9
	/* hack to catch non-existent memory */
	apr->extpulse |= 4;
	if(ma_ok || apr->ex_inh_rel)
		set_mc_rq(apr, 1);	// 7-9
	else
		nextpulse(apr, mc_illeg_address);	// 7-9
	if(apr->key_mem_stop || apr->ma == apr->mas && apr->sw_addr_stop)
		nextpulse(apr, mc_stop_1);	// 7-9
}

pulse(mc_rdwr_rq_pulse){
	trace("MC RD/RW RQ PULSE\n");
	set_mc_rd(apr, 1);		// 7-9
	set_mc_wr(apr, 1);		// 7-9
	apr->mb = 0;			// 7-8
	apr->mc_stop_sync = 1;		// 7-9
	nextpulse(apr, mc_rq_pulse);	// 7-8
}

pulse(mc_rd_rq_pulse){
	trace("MC RD RQ PULSE\n");
	set_mc_rd(apr, 1);		// 7-9
	set_mc_wr(apr, 0);		// 7-9
	apr->mb = 0;			// 7-8
	nextpulse(apr, mc_rq_pulse);	// 7-8
}

pulse(mc_split_rd_rq){
	trace("MC SPLIT RD RQ\n");
	nextpulse(apr, mc_rd_rq_pulse);	// 7-8
}

pulse(mc_wr_rq_pulse){
	trace("MC WR RQ PULSE\n");
	set_mc_rd(apr, 0);		// 7-9
	set_mc_wr(apr, 1);		// 7-9
	nextpulse(apr, mc_rq_pulse);	// 7-8
}

pulse(mc_split_wr_rq){
	trace("MC SPLIT WR RQ\n");
	nextpulse(apr, mc_wr_rq_pulse);	// 7-8
}

pulse(mc_rd_wr_rs_pulse){
	trace("MC RD/WR RS PULSE\n");
	nextpulse(apr, apr->mc_split_cyc_sync ? mc_split_wr_rq : mc_wr_rs);	// 7-8
}

/*
 * Keys
 */

pulse(key_rd_wr_ret){
	trace("KEY RD WR RET\n");
	apr->key_rd_wr = 0;	// 5-2
//	apr->ex_ill_op = 0;	// ?? not found on 5-13
	nextpulse(apr, kt4);	// 5-2
}

pulse(key_rd){
	trace("KEY RD\n");
	apr->key_rd_wr = 1;		// 5-2
	nextpulse(apr, mc_rd_rq_pulse);	// 7-8
}

pulse(key_wr){
	trace("KEY WR\n");
	apr->key_rd_wr = 1;		// 5-2
	apr->mb = apr->ar;		// 6-3
	nextpulse(apr, mc_wr_rq_pulse);	// 7-8
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

void
nextpulse(Apr *apr, Pulse *p)
{
	if(apr->nnextpulses >= MAXPULSE){
		fprint(stderr, "error: too many next pulses\n");
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
	apr->ia_inh = 0;

	// TODO: move this somewhere else
	iobusmap[0] = wake_cpa;
	iobusmap[1] = wake_pi;

	nextpulse(apr, mr_pwr_clr);
	while(apr->sw_power){
		apr->ncurpulses = apr->nnextpulses;
		apr->nnextpulses = 0;
		tmp = apr->clist; apr->clist = apr->nlist; apr->nlist = tmp;

		for(i = 0; i < apr->ncurpulses; i++)
			apr->clist[i](apr);

		/* KEY MANUAL */
		if(apr->extpulse & 1){
			apr->extpulse &= ~1;
			// BUG: without a print somewhere the thing doesn't work :(
			print("KEY MANUAL\n");
			nextpulse(apr, key_manual);
		}
		/* KEY INST STOP */
		if(apr->extpulse & 2){
			apr->extpulse &= ~2;
			apr->run = 0;
			// cleared after PI SYNC
			apr->ia_inh = 1;
		}


		/* This is simplified, we have no IOT RESET,
		 * IOT INIT SET UP or IOT FINAL SETUP really */
		if(apr->iot_go)
			nextpulse(apr, iot_t2);
		/* pulse and signals through IO bus */
		if(iobus1 & (IOBUS_PULSES | IOBUS_IOB_STATUS | IOBUS_IOB_DATAI)){
			int dev = 0;
			if(iobus1 & IOBUS_IOS3_1) dev |= 0100;
			if(iobus1 & IOBUS_IOS4_1) dev |= 0040;
			if(iobus1 & IOBUS_IOS5_1) dev |= 0020;
			if(iobus1 & IOBUS_IOS6_1) dev |= 0010;
			if(iobus1 & IOBUS_IOS7_1) dev |= 0004;
			if(iobus1 & IOBUS_IOS8_1) dev |= 0002;
			if(iobus1 & IOBUS_IOS9_1) dev |= 0001;
			//print("bus active for %o\n", dev<<2);
			if(iobusmap[dev])
				iobusmap[dev]();
			// TODO: clear IOB STATUS and IOB DATAI too?
			iobus1 &= ~IOBUS_PULSES;
		}
		if(iobus1 & IOBUS_IOB_RESET){
			iobus1 &= ~IOBUS_IOB_RESET;
		}

		/* Pulses to memory */
		if(membus0 & (MEMBUS_WR_RS | MEMBUS_RQ_CYC)){
			wakemem();
			/* Normally this should still be asserted but it
			 * is interpreted as a pulse every loop iteration here.
			 * Clearing it is a hack */
			membus0 &= ~MEMBUS_RQ_CYC;
		}

		/* Pulses from memory */
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
				nextpulse(apr, mc_non_exist_mem);	// 7-9
		}
		if(i)
			print("--------------\n");
	}
	print("power off\n");
	return NULL;
}


void
testinst(Apr *apr)
{
	int inst;

	for(inst = 0; inst < 0700; inst++){
//	for(inst = 0140; inst < 0141; inst++){
		apr->ir = inst << 9 | 1 << 5;
		decodeir(apr);
		print("%06o %6s ", apr->ir, names[inst]);
/*
		print("%s ", FAC_INH ? "FAC_INH" : "       ");
		print("%s ", FAC2 ? "FAC2" : "    ");
		print("%s ", FC_C_ACRT ? "FC_C_ACRT" : "         ");
		print("%s ", FC_C_ACLT ? "FC_C_ACLT" : "         ");
		print("%s ", FC_E ? "FC_E" : "    ");
*/
		print("%s ", FC_E_PSE ? "FC_E_PSE" : "        ");
		print("%s ", SC_E ? "SC_E" : "    ");
		print("%s ", SAC_INH ? "SAC_INH" : "       ");
		print("%s ", SAC2 ? "SAC2" : "    ");
		print("\n");
// FC_E_PSE
//print("FC_E_PSE: %d %d %d %d %d %d %d %d %d %d\n", apr->hwt_10 , apr->hwt_11 , apr->fwt_11 , \
//                  IOT_BLK , apr->inst == EXCH , CH_DEP , CH_INC_OP , \
//                  MEMAC_MEM , apr->boole_as_10 , apr->boole_as_11);
//print("CH: %d %d %d %d %d\n", CH_INC, CH_INC_OP, CH_N_INC_OP, CH_LOAD, CH_DEP);
//print("FAC_INH: %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
//	apr->hwt_11 , apr->fwt_00 , apr->fwt_01 , apr->fwt_11 ,
//	apr->inst == XCT , apr->ex_ir_uuo ,
//	apr->inst == JSP , apr->inst == JSR ,
//	apr->ir_iot , IR_254_7 , MEMAC_MEM ,
//	CH_LOAD , CH_INC_OP , CH_N_INC_OP);
	}
}
