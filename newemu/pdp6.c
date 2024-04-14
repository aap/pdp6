#include "common.h"
#include "pdp6.h"
#include "args.h"

#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include <signal.h>

#if 1
#define P(name) pulse(pdp, #name)
#define DLY(ns) simtime += ns
// can be turned into nop - only for accuracy
#define FF(ff, val) (pdp->ff = val)
#else
#define P(name)
#define DLY(ns) simtime += ns;
#define FF(ff, val) 0	// will be optimized out
#endif

#define SBR(sbr, ff) (FF(ff,1), sbr, FF(ff,0))

enum {
	// UUOs and IOTs are remapped, other opcodes serve as states directly.
	// since there are 64 UUO opcodes we should have enough space
	IOT_BLKI = 0,
	IOT_DATAI,
	IOT_BLKO,
	IOT_DATAO,
	IOT_CONO,
	IOT_CONI,
	IOT_CONSZ,
	IOT_CONSO,

	UUO,

	KT1,
	IT0,
	// for memory returns
	KEY_RDWR_RET,
	IT1A,
	AT0,
	AT3,
	FT1A,
	FT4A,
	FT6A,
	ST3,
	ST5A,
	ST7,
	UUOT1,
	BLTT0A,
	IOTT0A,
	CH_INC_OP,
	CH_NOT_INC_OP,
	CH_LOAD,
	CH_DEP,
	CHT8B,

	NOP
};

#define FCRY 01000000000000
#define NF0  0377777777777
#define NF01 0177777777777
#define H6 0004000
#define H9 0000400
#define H10 0000200
#define H11 0000100
#define H12 0000040
#define EXPBITS 0377000000000
#define MANTBITS 0000777777777

enum {
	CODE_UUO	= 01,
	// these can turn into UUO
	// depending on processor state
	CODE_IOT	= 02,
	CODE_JRST	= 04,
	// need CHF5 and CHF7 for decoding
	CODE_CHAR	= 010,

	// standard bits
	CODE_FAC_INH	= 020,
	CODE_FAC2	= 040,
	CODE_FCCACLT	= 0100,
	CODE_FCCACRT	= 0200,
	CODE_FAC2_ETC	= CODE_FAC2|CODE_FCCACLT|CODE_FCCACRT,
	CODE_FCE	= 0400,
	CODE_FCE_PSE	= 01000,
	CODE_SAC_INH	= 02000,
	CODE_SAC0_INH	= 04000,
	CODE_SAC2	= 010000,
	CODE_SCE	= 020000,
	CODE_PC_INC_INH	= 040000,
	CODE_PI_HOLD	= 0100000,
};
Code codetab[01000];
Code iotcodetab[8];


#define AR pdp->ar
#define MQ pdp->mq
#define MQ36 pdp->mq36
#define MB pdp->mb
#define MI pdp->mi
#define IR pdp->ir
#define MA pdp->ma
#define PC pdp->pc
#define FE pdp->fe
#define SC pdp->sc
#define IOB pdp->iob

#define SWAP(A,B) (t = A, A = B, B = t)
#define SWAPLTRT(a) ((a)<<18 & LT | (a)>>18 & RT)
#define EXP_CLR(reg) (reg = reg&F0 ? reg|EXPBITS : reg&~EXPBITS)

#define AR_ZERO (AR == 0)
#define AR_NEG !!(AR & F0)
#define AR_OV (pdp->ar_cry0 != pdp->ar_cry1)
#define AR_LESS (AR_NEG != AR_OV)
#define SELECTED_FLAGS (IR & 0740 &\
	(pdp->ar_ov_flag<<8 | pdp->ar_cry0_flag<<7 |\
	 pdp->ar_cry1_flag<<6 | pdp->ar_pc_chg_flag<<5))

#define MB_NEG !!(MB & F0)
#define MQ_NEG !!(MQ & F0)
#define SC_NEG !!(SC & 0400)

// AR
#define AR_CLR (AR = 0)
#define ARLT_CLR (AR &= RT)
#define ARLT_COM (AR ^= LT)
#define ARRT_COM (AR ^= RT)
#define AR_COM (AR ^= FW)
#define AR_FM_MB_C (AR ^= MB)
#define AR_FM_MB_0 (AR &= MB)
#define AR_FM_MB_1 (AR |= MB)
#define AR_FM_MBLT_J (AR = MB&LT | AR&RT)
#define AR_FM_MBRT_J (AR = AR&LT | MB&RT)
#define AR_FM_MB_J (AR = MB)
#define AR_FM_SC_J (AR = AR&~EXPBITS | ((Word)SC&0377)<<27)
#define AR_ADD ar_add(pdp)
#define AR_SUB ar_sub(pdp)
#define AR_INC ar_inc(pdp)
#define AR_INC_LTRT ar_inc_ltrt(pdp)
#define AR_DEC ar_dec(pdp)
#define AR_DEC_LTRT ar_dec_ltrt(pdp)
#define AR_NEGATE ar_negate(pdp)
#define AR_SET_OV_CRY ar_set_ov_cry(pdp)

// MQ
#define MQ_CLR (MQ = 0)
#define MQ_FM_MB_J (MQ = MB)

// MB
#define MB_CLR (MB = 0)
#define MBLT_CLR (MB &= RT)
#define MB_FM_AR_0 (MB &= AR)
#define MB_FM_AR_1 (MB |= AR)
#define MB_FM_AR_J (MB = AR)
#define MB_FM_MQ_0 (MB &= MQ)
#define MB_FM_MQ_1 (MB |= MQ)
#define MB_FM_MQ_J (MB = MQ)
#define MB_SWAP (MB = (MB<<18 | MB>>18) & FW)
#define MB_FM_PC_1 (MB |= PC)
#define MB_FM_MISC_BITS mb_fm_misc_bits(pdp)
#define SWAP_MB_AR SWAP(MB,AR)
#define SWAP_MB_MQ SWAP(MB,MQ)

// MA
#define MA_CLR (MA = 0)
#define MA_FM_MBRT_1 (MA |= MB & RT)
#define MA_INC (MA = (MA+1)&0777777)
#define MA_INC4 (MA = (MA+1)&017)	// not quite correct, but good enough, only used for AC2

// IR
#define IR_CLR (IR = 0)

// PC
#define PC_CLR (PC = 0)
#define PC_FM_MA_1 (PC |= MA)
#define PC_INC (PC = (PC+1) & RT)
#define PC_CHG pc_chg(pdp)

// FE
#define FE_CLR (FE = 0)
#define FE_FM_SC_1 (FE |= SC)
#define FE_FM_MB_1 (FE |= (MB>>30)&077)

// SC
#define SC_CLR (SC = 0)
#define SC_COM (SC ^= 0777)
#define SC_INC (SC = (SC+1) & 0777)
#define SC_FM_MB (SC |= (~MB>>9)&0400 | ~MB&0377)
#define SC_PAD(x) (SC ^= x)
#define SC_ADD(x) sc_add(pdp, x)
#define SC_FM_FE_1 (SC |= FE)

#define MR_CLR mr_clr(pdp)
#define FWT_OV if(!pdp->ar_cry0 && pdp->ar_cry1) ar_set_ov(pdp)
#define PDL_OV if(pdp->ar_cry0) cpa_set_pdl_ov(pdp)
#define SKIP (PC_INC, PC_CHG)
#define ACBM_COM (t = MB & AR, AR ^= MB, MB = t)
#define ACBM_SET (t = MB & AR, AR |= MB, MB = t)
#define NR_ROUND (!pdp->nrf3 && IR&H6 && MQ&F1)

#define trace if(pdp->dotrace) printf

void
pulse(PDP6 *pdp, const char *name)
{
	trace("%s", name);
//	trace("\tAR/%012llo MQ/%012llo MB/%012llo", AR, MQ, MB);
//	trace(" SC/%03o FE/%03o", SC, FE);
	trace("\tMB/%012llo AR/%012llo MQ/%012llo", MB, AR, MQ);
	trace(" FE/%03o SC/%03o", FE, SC);
	trace(" IR/%06o PC/%06o MA/%06o", IR, PC, MA);
	trace(" %llu", simtime);
	trace("\n");
}

static IOdev *devs_code[0200];
static IOdev *devs[0200];
static int numdevs;

void handle_cpa(PDP6 *pdp, IOdev *dev, int cmd);
void handle_pi(PDP6 *pdp, IOdev *dev, int cmd);
static IOdev cpa_dev = { 0, 0, nil, handle_cpa, nil };
static IOdev pi_dev = { 0, 4, nil, handle_pi, nil };


// use as template
void handle_null(PDP6 *pdp, IOdev *dev, int cmd) {
printf("IOT with unimplemented device %06o %06o %03o %o\n", PC, IR, (IR>>6)&0774, cmd);
	switch(cmd) {
	case IOB_RESET:
		break;
	case IOB_CONO_CLR:
		break;
	case IOB_CONO_SET:
		break;
	case IOB_DATAO_CLR:
		break;
	case IOB_DATAO_SET:
		break;
	case IOB_DATAI:
		break;
	case IOB_STATUS:
		break;
	}
}
static IOdev null_dev = { 0, 0, nil, handle_null, nil };

void
io_reset(PDP6 *pdp)
{
	for(int i = 0; i < numdevs; i++)
		devs[i]->handler(pdp, devs[i], IOB_RESET);
}

static void
calc_iob_req(PDP6 *pdp)
{
	pdp->iob_req = 0;
	for(int i = 0; i < numdevs; i++)
		pdp->iob_req |= devs[i]->req;
}

// need to call this for all sorts of devices
void
setreq(PDP6 *pdp, IOdev *dev, u8 req)
{
	req &= 0177;
	if(dev->req != req) {
		dev->req = req;
		calc_iob_req(pdp);
	}
}


Hword maxmem = 256*1024;
Word memory[01000000];
Word fmem[020];

static void cpa_set_illeg_op(PDP6 *pdp);
static void cpa_set_nomem(PDP6 *pdp);

static int
reloc(PDP6 *pdp)
{
	if(pdp->ex_user && !pdp->ex_pi_sync && MA >= 020 && !pdp->ex_ill_op) {
		pdp->rla = (MA + pdp->rlr) & RT;
		DLY(100);
		if((MA & 0776000) > pdp->pr) {
			cpa_set_illeg_op(pdp);
			DLY(100);
			pdp->state = ST7;
			return 1;
		}
	} else
		pdp->rla = MA;
	return 0;
}

static int
mc_rq(PDP6 *pdp, int ret)
{
	pdp->state = ret;
	pdp->mc_stop = pdp->key_mem_stop || pdp->sw_addr_stop && MA == pdp->mas;
	return reloc(pdp);
}

static int
mc_nxm(PDP6 *pdp)
{
printf("no mem %06o PC %06o\n", MA, PC);
	if(pdp->mc_stop) {
		pdp->cycling = 0;
		return 0;
	}
	cpa_set_nomem(pdp);
	if(pdp->sw_mem_disable) {
		pdp->cycling = 0;
		return 0;
	}
	// restart
	FF(mc_rq, 0);
	FF(mc_rd, 0);
	return 1;
}

static void
rdrq(PDP6 *pdp, int ret)
{
	FF(mc_rd, 1);
	FF(mc_wr, 0);
	MB = 0;
	if(mc_rq(pdp, ret))
		return;

	FF(mc_rq, 1);
	// timing from F-67_162_161C_Apr66.pdf
	if(MA < 020 && !pdp->key_rim_sbr) {
		DLY(380);
		MB |= fmem[MA];
	} else if(pdp->rla < maxmem) {
		DLY(2750);
		MB |= memory[pdp->rla];
	} else {
		mc_nxm(pdp);
		return;
	}

	// restart
	FF(mc_rq, 0);
	if(MA == pdp->mas)
		MI = MB;
	if(pdp->mc_stop)
		pdp->cycling = 0;
	else
		FF(mc_rd, 0);
trace("RD\t\t%06o  %06o %d -> %012llo\n", MA, pdp->rla, pdp->key_rim_sbr, MB);
}

static void
wrrq(PDP6 *pdp, int ret)
{
	FF(mc_rd, 0);
	FF(mc_wr, 1);
	if(mc_rq(pdp, ret))
		return;

	FF(mc_rq, 1);
	// timing from F-67_162_161C_Apr66.pdf
	if(MA < 020 && !pdp->key_rim_sbr) {
		DLY(420);
		fmem[MA] = MB;
		FF(mc_rq, 0);
	} else if(pdp->rla < maxmem) {
		DLY(750);
		memory[pdp->rla] = MB;
		FF(mc_rq, 0);
	} else {
		if(mc_nxm(pdp)) {
			if(MA == pdp->mas)
				MI = MB;
		}
		return;
	}

	// restart
	FF(mc_rq, 0);
	if(MA == pdp->mas)
		MI = MB;
	if(pdp->mc_stop)
		pdp->cycling = 0;
	else
		FF(mc_rd, 0);
trace("WR\t\t%06o  %06o %d <- %012llo\n", MA, pdp->rla, pdp->key_rim_sbr, MB);
}

// TODO: actually do these properly

void
rdwrrq(PDP6 *pdp, int ret)
{
	rdrq(pdp, ret);
}

void
wrrs(PDP6 *pdp, int ret)
{
	wrrq(pdp, ret);
}




void mr_start(PDP6 *pdp);
void mr_clr(PDP6 *pdp);
void pi_reset(PDP6 *pdp);

void
initdevs(PDP6 *pdp)
{
	for(int i = 0; i < 0200; i++)
		devs_code[i] = &null_dev;
	numdevs = 0;
	installdev(pdp, &cpa_dev);
	installdev(pdp, &pi_dev);
}

void
installdev(PDP6 *pdp, IOdev *dev)
{
	devs_code[dev->devcode >> 2] = devs[numdevs++] = dev;
}

void
cycle_io(PDP6 *pdp, int pwr)
{
	for(int i = 0; i < numdevs; i++)
		if(devs[i]->cycle)
			devs[i]->cycle(pdp, devs[i], pwr);
}

void
pwrclr(PDP6 *pdp)
{
	for(int i = 0; i < numdevs; i++)
		devs[i]->req = 0;
	pdp->iob_req = 0;

	AR = (rand() | (Word)rand()<<18) & FW;
	MQ = (rand() | (Word)rand()<<18) & FW;
	MB = (rand() | (Word)rand()<<18) & FW;
	MI = (rand() | (Word)rand()<<18) & FW;
	PC = rand() & RT;
	MA = rand() & RT;
	IR = rand() & RT;

	pdp->run = 0;
	mr_start(pdp);
	mr_clr(pdp);
}

void
mr_start(PDP6 *pdp)
{
	pdp->pi_cyc = 0;
	pdp->pi_ov = 0;
	pi_reset(pdp);

	pdp->ex_user = 0;
	pdp->ex_ill_op = 0;
	pdp->pr = 0;
	pdp->rlr = 0;

	pdp->ar_pc_chg_flag = 0;
	pdp->ar_ov_flag = 0;
	pdp->ar_cry0_flag = 0;
	pdp->ar_cry1_flag = 0;
	pdp->chf7 = 0;

	pdp->cpa_iot_user = 0;
	pdp->cpa_illeg_op = 0;
	pdp->cpa_non_exist_mem = 0;
	pdp->cpa_clock_enable = 0;
	pdp->cpa_clock_flag = 0;
	pdp->cpa_pc_chg_enable = 0;
	pdp->cpa_pdl_ov = 0;
	pdp->cpa_arov_enable = 0;
	pdp->cpa_pia = 0;
	setreq(pdp, &cpa_dev, 0);

	io_reset(pdp);
}

void
mr_clr(PDP6 *pdp)
{
	MQ_CLR;
	MQ36 = 0;
	IR_CLR;
	FE_CLR;
	SC_CLR;
	if(pdp->ex_mode_sync)
		pdp->ex_user = 1;
	pdp->ex_mode_sync = 0;
	FF(ex_uuo_sync, 0);
	pdp->ex_pi_sync = pdp->pi_cyc;

	FF(iot_go, 0);

	FF(ar_com_cont, 0);

	FF(key_rd_wr, 0);
	FF(if1a, 0);
	FF(af0, 0);
	FF(af3, 0);
	FF(af3a, 0);
	FF(f1a, 0);
	FF(f4a, 0);
	FF(f6a, 0);
	FF(et4_ar_pause, 0);
	FF(sf3, 0);
	FF(sf5a, 0);
	FF(sf7, 0);
	pdp->dsf7 = 0;
	FF(iot_f0a, 0);
	FF(blt_f0a, 0);
	FF(blt_f3a, 0);
	FF(blt_f5a, 0);
	FF(uuo_f1, 0);
	FF(dcf1, 0);

	// MP CLR
	FF(chf1, 0);
	FF(chf2, 0);
	FF(chf3, 0);
	FF(chf4, 0);
	pdp->chf5 = 0;
	FF(chf6, 0);
	FF(lcf1, 0);
	FF(shf1, 0);
	FF(mpf1, 0);
	pdp->mpf2 = 0;
	FF(msf1, 0);
	FF(fmf1, 0);
	FF(fmf2, 0);
	FF(fdf1, 0);
	FF(fdf2, 0);
	FF(faf1, 0);
	FF(faf2, 0);
	FF(faf3, 0);
	FF(faf4, 0);
	FF(fpf1, 0);
	FF(fpf2, 0);
	FF(nrf1, 0);
	FF(nrf2, 0);
	pdp->nrf3 = 0;

	// DS CLR
	FF(dsf1, 0);
	FF(dsf2, 0);
	FF(dsf3, 0);
	FF(dsf4, 0);
	FF(dsf5, 0);
	FF(dsf6, 0);
	FF(dsf8, 0);
	FF(dsf9, 0);
	FF(fsf1, 0);
}

// PDP-1 equivalences:
//	PIH = B4
//	PIR = B3
//	REQ = B2	latched and gated by PIO
//	PIO = B1
static void calc_req(PDP6 *pdp) {
	u8 r;
	if(!pdp->pi_active) {
		pdp->pi_req = 0;
		return;
	}
	for(r = 0100; r; r >>= 1) {
		if(pdp->pih & r)
			r = 0;
		else if(pdp->pir & r)
			break;
	}
	pdp->pi_req = r;
}
static void pir_set(PDP6 *pdp, u8 pir) {
	if(pir == 0) return;	// common case
	pdp->pir |= pir;
	pdp->pir &= ~pdp->pih;
	calc_req(pdp);
}

static void pi_hold(PDP6 *pdp) {
	// accept break
	pdp->pih |= pdp->pi_req;
	pdp->pir &= ~pdp->pih;
	calc_req(pdp);
}

static void pi_rst(PDP6 *pdp) {
	// clear highest break
	if(!pdp->pi_active)
		return;
	for(u8 r = 0100; r; r >>= 1)
		if(pdp->pih & r) {
			pdp->pih &= ~r;
			calc_req(pdp);
			break;
		}
}

// exit PI cycle
static void pi_exit(PDP6 *pdp) {
	pdp->pi_ov = 0;
	pdp->pi_cyc = 0;
}

void
pi_reset(PDP6 *pdp)
{
	pdp->pi_active = 0;
	pdp->pih = 0;
	pdp->pir = 0;
	pdp->pio = 0;
	// therefore:
	pdp->pi_req = 0;
}

void
handle_pi(PDP6 *pdp, IOdev *dev, int cmd)
{
	switch(cmd) {
	case IOB_CONO_CLR:
		if(IOB & F23) pi_reset(pdp);
		break;
	case IOB_CONO_SET:
// TODO: actually set&clear at the same time should complement
		if(IOB & F24) pir_set(pdp, IOB & 0177);
		if(IOB & F25) pdp->pio |=  IOB & 0177;
		if(IOB & F26) pdp->pio &= ~IOB & 0177;
		if(IOB & F27) pdp->pi_active = 0;
		if(IOB & F28) pdp->pi_active = 1;
		calc_req(pdp);
		break;
	case IOB_STATUS:
		if(pdp->pi_active) IOB |= F28;
		IOB |= pdp->pio;
		break;
	}
}

#define PIR_STB pir_set(pdp, pdp->iob_req & pdp->pio)
#define PI_HOLD (pdp->pi_cyc && (!(pdp->code&CODE_IOT) || !pdp->pi_ov && (pdp->code&CODE_PI_HOLD)))




void
calc_cpa_req(PDP6 *pdp)
{
	if(pdp->cpa_pia &&
	   (pdp->cpa_illeg_op ||
	    pdp->cpa_non_exist_mem ||
	    pdp->cpa_pdl_ov ||
	    pdp->cpa_clock_enable && pdp->cpa_clock_flag ||
	    pdp->cpa_pc_chg_enable && pdp->ar_pc_chg_flag ||
	    pdp->cpa_arov_enable && pdp->ar_ov_flag))
		setreq(pdp, &cpa_dev, 0200>>pdp->cpa_pia);
	else
		setreq(pdp, &cpa_dev, 0);
}

void
handle_cpa(PDP6 *pdp, IOdev *dev, int cmd)
{
	switch(cmd) {
	case IOB_RESET:
		pdp->cpa_iot_user = 0;
		break;
	case IOB_CONO_SET:
// TODO: actually set&clear at the same time should complement
		if(IOB & F18) pdp->cpa_pdl_ov = 0;
		if(IOB & F20) pdp->cpa_iot_user = 0;
		if(IOB & F21) pdp->cpa_iot_user = 1;
		if(IOB & F22) pdp->cpa_illeg_op = 0;
		if(IOB & F23) pdp->cpa_non_exist_mem = 0;
		if(IOB & F24) pdp->cpa_clock_enable = 0;
		if(IOB & F25) pdp->cpa_clock_enable = 1;
		if(IOB & F26) pdp->cpa_clock_flag = 0;
		if(IOB & F27) pdp->cpa_pc_chg_enable = 0;
		if(IOB & F28) pdp->cpa_pc_chg_enable = 1;
		if(IOB & F29) pdp->ar_pc_chg_flag = 0;
		if(IOB & F30) pdp->cpa_arov_enable = 0;
		if(IOB & F31) pdp->cpa_arov_enable = 1;
		if(IOB & F32) pdp->ar_ov_flag = 0;
		pdp->cpa_pia = IOB & 7;
		if(IOB & F19) io_reset(pdp);
		calc_cpa_req(pdp);
		break;
	case IOB_DATAO_CLR:
		pdp->pr = 0;
		pdp->rlr = 0;
		break;
	case IOB_DATAO_SET:
		pdp->pr = (IOB>>18) & 0776000;
		pdp->rlr = IOB & 0776000;
		break;
	case IOB_DATAI:
		IOB |= pdp->datasw;
		break;
	case IOB_STATUS:
		if(pdp->cpa_pdl_ov) IOB |= F19;
		if(pdp->cpa_iot_user) IOB |= F20;
		if(pdp->ex_user) IOB |= F21;
		if(pdp->cpa_illeg_op) IOB |= F22;
		if(pdp->cpa_non_exist_mem) IOB |= F23;
		if(pdp->cpa_clock_enable) IOB |= F25;
		if(pdp->cpa_clock_flag) IOB |= F26;
		if(pdp->cpa_pc_chg_enable) IOB |= F28;
		if(pdp->ar_pc_chg_flag) IOB |= F29;
		if(pdp->cpa_arov_enable) IOB |= F31;
		if(pdp->ar_ov_flag) IOB |= F32;
		IOB |= pdp->cpa_pia;
		break;
	}
}


static void add(PDP6 *pdp, Word x) {
	DLY(100);		// just a guess
	if((AR&~F0) + (x&~F0) & F0) pdp->ar_cry1 = 1;
	AR += x;
	if(AR & FCRY) pdp->ar_cry0 = 1;
	AR &= FW;
}
static void ar_inc(PDP6 *pdp) { add(pdp, 1); }
static void ar_inc_ltrt(PDP6 *pdp) { add(pdp, 01000001); }
static void ar_add(PDP6 *pdp) {
	DLY(100);
	add(pdp, MB);
}
static void ar_sub(PDP6 *pdp) {   
	AR ^= FW;
	DLY(100);
	ar_add(pdp);
	AR ^= FW;
	DLY(100);
}       
static void ar_dec(PDP6 *pdp) {
	AR ^= FW;
	DLY(100);
	ar_inc(pdp);
	AR ^= FW;
	DLY(100);
}                       
static void ar_dec_ltrt(PDP6 *pdp) {
	AR ^= FW;
	DLY(100);
	ar_inc_ltrt(pdp);
	AR ^= FW;
	DLY(100);
}
static void ar_negate(PDP6 *pdp) {
	AR ^= FW;
	DLY(100);
	ar_inc(pdp);
}

// TODO: only call if flag changes?
static void pc_chg(PDP6 *pdp) { pdp->ar_pc_chg_flag = 1; calc_cpa_req(pdp); }
static void ar_set_ov(PDP6 *pdp) { pdp->ar_ov_flag = 1; calc_cpa_req(pdp); }
static void cpa_set_pdl_ov(PDP6 *pdp) { pdp->cpa_pdl_ov = 1; calc_cpa_req(pdp); }
static void cpa_set_illeg_op(PDP6 *pdp) { pdp->cpa_illeg_op = 1; calc_cpa_req(pdp); }
static void cpa_set_nomem(PDP6 *pdp) { pdp->cpa_non_exist_mem = 1; calc_cpa_req(pdp); }
static void cpa_set_clock(PDP6 *pdp) { pdp->cpa_clock_flag = 1; calc_cpa_req(pdp); }

static void ar_flag_clr(PDP6 *pdp) {
	pdp->ar_ov_flag = 0;
	pdp->ar_cry0_flag = 0;
	pdp->ar_cry1_flag = 0;
	pdp->ar_pc_chg_flag = 0;
	pdp->chf7 = 0;
	// perhaps unnecessary because flags are changed again
	// by ar_flag_set
	calc_cpa_req(pdp);
}
static void ar_flag_set(PDP6 *pdp) {
	if(MB & F0) pdp->ar_ov_flag = 1;
	if(MB & F1) pdp->ar_cry0_flag = 1;
	if(MB & F2) pdp->ar_cry1_flag = 1;
	if(MB & F3) pdp->ar_pc_chg_flag = 1;
	if(MB & F4) pdp->chf7 = 1;
	if(MB & F5) pdp->ex_mode_sync = 1;
	if(!pdp->ex_user) pdp->cpa_iot_user = !!(MB & F6);
	calc_cpa_req(pdp);
}
static void mb_fm_misc_bits(PDP6 *pdp){
	if(pdp->ar_ov_flag) MB |= F0;
	if(pdp->ar_cry0_flag) MB |= F1;
	if(pdp->ar_cry1_flag) MB |= F2;
	if(pdp->ar_pc_chg_flag) MB |= F3;
	if(pdp->chf7) MB |= F4;
	if(pdp->ex_user) MB |= F5;
	if(pdp->cpa_iot_user) MB |= F6;
}
static void ar_set_ov_cry(PDP6 *pdp) {
	if(AR_OV) ar_set_ov(pdp);
	if(pdp->ar_cry0) pdp->ar_cry0_flag = 1;
	if(pdp->ar_cry1) pdp->ar_cry1_flag = 1;
}
static void ar_jfcl_clr(PDP6 *pdp) {
	if(IR & H9) pdp->ar_ov_flag = 0;
	if(IR & H10) pdp->ar_cry0_flag = 0;
	if(IR & H11) pdp->ar_cry1_flag = 0;
	if(IR & H12) pdp->ar_pc_chg_flag = 0;
	calc_cpa_req(pdp);
}

// TODO: all MQ right shifts shift into MQ36
static void ash_lt_ov(PDP6 *pdp) { if((AR ^ (AR<<1))&F0) ar_set_ov(pdp); }
static void ashrt(PDP6 *pdp) { AR = AR&F0 | (AR>>1); }
static void lshrt(PDP6 *pdp) { AR = AR>>1; }
static void rotrt(PDP6 *pdp) { AR = (AR<<35)&F0 | (AR>>1); }
static void ashlt(PDP6 *pdp) { ash_lt_ov(pdp); AR = AR&F0 | (AR<<1)&NF0; }
static void lshlt(PDP6 *pdp) { AR = (AR<<1)&FW; }
static void rotlt(PDP6 *pdp) { AR = (AR<<1)&FW | (AR>>35)&F35; }
static void ashcrt(PDP6 *pdp) {
	MQ = AR&F0 | (AR<<34)&F1 | (MQ>>1)&NF01;
	AR = AR&F0 | (AR>>1);
}
static void lshcrt(PDP6 *pdp) {
	MQ = (AR<<35)&F0 | (MQ>>1);
	AR = AR>>1;
}
static void rotcrt(PDP6 *pdp) {
	Word t = (AR<<35)&F0 | (MQ>>1);
	AR  = (MQ<<35)&F0 | (AR>>1);
	MQ = t;
}
static void ashclt(PDP6 *pdp) {
	ash_lt_ov(pdp);
	AR = AR&F0 | (AR<<1)&NF0 | (MQ>>34)&F35;
	MQ = AR&F0 | (MQ<<1)&NF0;
}
static void lshclt(PDP6 *pdp) {
	AR = (AR<<1)&FW | (MQ>>35)&F35;
	MQ = (MQ<<1)&FW;
}
static void rotclt(PDP6 *pdp) {
	Word t = (AR<<1)&FW | (MQ>>35)&F35;
	MQ  = (MQ<<1)&FW | (AR>>35)&F35;
	AR = t;
}
static void char_first(PDP6 *pdp) {
	MQ = (MQ<<1)&FW | 1;
}
static void ch_dep(PDP6 *pdp) {
	AR = (AR<<1)&FW | 0;
	MQ = (MQ<<1)&FW | 0;
}


static void
sc_add(PDP6 *pdp, int x)
{
	FF(chf1, 0);
		P(SAT0);
		DLY(150);
		P(SAT1);
	// really SC PAD
	SC = (SC+x)&0777;
		DLY(200);
		P(SAT2);
		DLY(50);
	// SC CRY
		P(SAT21);
		DLY(100);
		P(SAT3);
}

static void
sc_count(PDP6 *pdp, void (*shift)(PDP6*))
{
	P(SCT0);
	DLY(200);

	while(SC != 0777) {
		SC_INC;
		shift(pdp);
		P(SCT1);
		DLY(75);
	}

	P(SCT2);
}
#define SCT_ASHLT sc_count(pdp, ashlt)
#define SCT_ASHRT sc_count(pdp, ashrt)
#define SCT_ROTLT sc_count(pdp, rotlt)
#define SCT_ROTRT sc_count(pdp, rotrt)
#define SCT_LSHLT sc_count(pdp, lshlt)
#define SCT_LSHRT sc_count(pdp, lshrt)
#define SCT_ASHCLT sc_count(pdp, ashclt)
#define SCT_ASHCRT sc_count(pdp, ashcrt)
#define SCT_ROTCLT sc_count(pdp, rotclt)
#define SCT_ROTCRT sc_count(pdp, rotcrt)
#define SCT_LSHCLT sc_count(pdp, lshclt)
#define SCT_LSHCRT sc_count(pdp, lshcrt)

static void
multiply(PDP6 *pdp)
{
	MQ_FM_MB_J;
	MB_FM_AR_J;
					P(MST1);
	AR_CLR;
					DLY(200);
loop:
	if((MQ&1) != MQ36) {
		if(MQ36) {
					P(MST3);
			SBR(AR_ADD, msf1);
		} else {
					P(MST4);
			SBR(AR_SUB, msf1);
		}
					P(MST3A);
	}
	if(SC == 0777) {
		SC_CLR;
		MQ = AR&F0 | MQ>>1;
					P(MST5);
					DLY(100);
					P(MST6);
		return;
	}
	SC_INC;
	MQ36 = MQ&1;
	MQ = (AR<<35)&F0 | (MQ>>1);
	AR = AR&F0 | (AR>>1);
					P(MST2);
					DLY(150);
	goto loop;
}

// return 1 on overflow
static int
divide(PDP6 *pdp)
{
	Word t;
					P(DST10);
					DLY(100);
	if(IR & 0100000) {	// FDV
		MQ = MQ&NF0 | (AR<<35)&F0;
		ashrt(pdp);
					P(DST10A);
					DLY(200);
	} else {		// DIV
		MQ = (MQ<<1)&FW | ((AR>>35)^1);
					P(DST10B);
					DLY(200);
	}
	if(MB_NEG) {
					P(DST11);
		SBR(AR_ADD, dsf4);
	} else {
					P(DST12);
		SBR(AR_SUB, dsf4);
	}
					P(DST11A);
	if(!AR_NEG) {
					P(DST13);
		ar_set_ov(pdp);
		return 1;
	}
loop:
	SC_INC;
	t = (AR<<1)&FW | (MQ>>35)&F35;
	MQ  = (MQ<<1)&FW | (~AR>>35)&F35;
	AR = t;
					P(DST14A);
					P(DST14B);
					DLY(100);
	if(SC != 0777) {
		if((MQ ^ (MB>>35))&1) {
					P(DST14);
			SBR(AR_SUB, dsf5);
		} else {
					P(DST15);
			SBR(AR_ADD, dsf5);
		}
		goto loop;
	}
	if(IR & 0100000)	// FDV
		ashrt(pdp);
	else			// DIV
		AR = (~MQ&1)<<35 | AR>>1;
					P(DST16);
					DLY(100);
	if(AR_NEG) {
		if(!MB_NEG) {
					P(DST17);
			SBR(AR_ADD, dsf6);
		} else {
					P(DST18);
			SBR(AR_SUB, dsf6);
		}
	}
					P(DST17A);
	if(pdp->dsf7) {
					P(DST19);
		SBR(AR_NEGATE, dsf8);
	}
	SWAP_MB_MQ;
					P(DST19A);
					DLY(100);
	SC_CLR;
	SWAP_MB_AR;
					P(DST20);
					DLY(100);
	if(pdp->dsf7 != MQ_NEG) {
					P(DST21);
		SBR(AR_NEGATE, dsf9);
	}
					P(DST21A);
	SWAP_MB_MQ;
	return 0;
}

static void
exp_calc(PDP6 *pdp, int mult)
{
					P(FPT0);
	SC |= 0200;
					P(FPT01);
	if(AR_NEG != mult)
		SC_COM;
					P(FPT1);
					DLY(100);
	SBR(SC_ADD((AR>>27)&0777), fpf1);
	if((AR_NEG == MB_NEG) != mult) {
		SC_COM;
	} else {
		SC_INC;
					P(FPT2);
	}
					P(FPT1A);
					DLY(100);
					P(FPT1AA);
	SBR(SC_ADD((MB>>27)&0777), fpf2);
					P(FPT1B);
	if(MB_NEG == mult)
		SC_COM;
					DLY(100);
	FE_FM_SC_1;
	SC_CLR;
	EXP_CLR(AR);
	EXP_CLR(MB);
					P(FPT3);
					DLY(100);
					P(FPT4);
}

void
decode(PDP6 *pdp)
{
	pdp->estate = IR>>9;
	pdp->code = codetab[pdp->estate];

	// special handling
	if(pdp->code & (CODE_SAC0_INH|CODE_UUO|CODE_IOT|CODE_JRST|CODE_CHAR)) {
		if(pdp->code & CODE_SAC0_INH) {
			if(((IR>>5)&017)==0)
				pdp->code |= CODE_SAC_INH;
		} if(pdp->code & CODE_IOT) {
			if(pdp->ex_user && !pdp->cpa_iot_user && !pdp->ex_pi_sync) {
				pdp->code = codetab[0];
			} else {
				pdp->estate = (IR>>5)&7;
				pdp->code = iotcodetab[pdp->estate];
			}
		} else if(pdp->code & CODE_JRST) {
			if(pdp->ex_user && (IR&(H9|H10)))
				pdp->code = codetab[0];
		} else if(pdp->code & CODE_CHAR) {
			if(!pdp->chf5) {
				// first part
				pdp->code = pdp->estate == 0133 ? 0 : CODE_PC_INC_INH;
				if((pdp->estate&5) != 5 && !pdp->chf7) {
					pdp->estate = CH_INC_OP;
					pdp->code |= CODE_FAC_INH | CODE_FCE_PSE;
				} else {
					pdp->estate = CH_NOT_INC_OP;
					pdp->code |= CODE_FAC_INH | CODE_FCE;
				}
			} else {
				// second part
				if((pdp->estate&6) == 4) {
					pdp->estate = CH_LOAD;
					pdp->code = CODE_FAC_INH | CODE_FCE;
				} else if((pdp->estate&6) == 6) {
					pdp->estate = CH_DEP;
					pdp->code = CODE_FCE_PSE | CODE_SAC_INH;
				} else
					pdp->estate = NOP;	// should be impossible
			}
		}

		if(pdp->code & CODE_UUO)
			pdp->estate = UUO;
	}
}


void
kt0(PDP6 *pdp)
{
	P(KT0);

	pdp->key_ex_st = 0;
	pdp->key_dep_st = 0;
	pdp->key_ex_sync = pdp->key_ex;
	pdp->key_dep_sync = pdp->key_dep;
	P(KT0A);
	if(!pdp->run || pdp->key_mem_cont) {
		if(pdp->key_mem_cont) {
			pdp->mc_stop = 0;
			FF(mc_rd, 0);
		} else
			pdp->state = KT1;	// hack, key cycle not so useful for mem cont
		pdp->cycling = 1;
	}
}

void
start_measure(PDP6 *pdp)
{
	pdp->sim_start = simtime;
	pdp->real_start = gettime();
}

void
end_measure(PDP6 *pdp)
{
	pdp->sim_end = simtime;
	pdp->real_end = gettime();
	u64 sim_diff = pdp->sim_end - pdp->sim_start;
	u64 real_diff = pdp->real_end - pdp->real_start;
	printf("ran for:\n");
	printf("real: %lld\n", real_diff);
	printf("sim : %lld\n", sim_diff);
	printf("%%   : %f\n", (double)sim_diff/real_diff);
}

void
clr_run(PDP6 *pdp)
{
	if(pdp->run) end_measure(pdp);
	pdp->run = 0;
}

void
key_go(PDP6 *pdp)
{
	pdp->run = 1;
start_measure(pdp);
	pdp->key_ex_st = 0;
	pdp->key_dep_st = 0;
	pdp->key_ex_sync = 0;
	pdp->key_dep_sync = 0;
	pdp->cycling = 1;
	pdp->state = IT0;
	P(KEY_GO);
}

#define IO_PULSE(cmd) devs_code[(IR>>8)&0177]->handler(pdp, devs_code[(IR>>8)&0177], cmd)

#define MEM_RQ(type, state, ff) \
	FF(ff, 1); \
	type(pdp, state); \
	break;
#define MEM_RET(state, ff) \
	case state: \
	FF(ff, 0);

#define CLK_INTERVAL 16666667

void
cycle(PDP6 *pdp)
{
	int test;
	Word t;

	while(pdp->clk_timer < simtime) {
		pdp->clk_timer += CLK_INTERVAL;
		cpa_set_clock(pdp);
	}
	switch(pdp->state){
	case KT1:
		DLY(100);
		if(pdp->key_read_in || pdp->key_start || pdp->key_inst_cont ||
		   pdp->key_ex || pdp->key_ex_nxt ||
		   pdp->key_dep || pdp->key_dep_nxt ||
		   pdp->key_execute || pdp->key_io_reset)
			MR_CLR;
// we don't even come here with key_mem_cont
		if(!(pdp->key_read_in || pdp->key_inst_cont || pdp->key_mem_cont))
			pdp->key_rim_sbr = 0;
		if(pdp->key_read_in) {
			MA_CLR;
			PC_CLR;
			pdp->key_rim_sbr = 1;
		}
		if(pdp->key_start) {
			MA_CLR;
			PC_CLR;
		}
		if(pdp->key_execute)
			AR_CLR;
		if(pdp->key_ex_sync)
			MA_CLR;
		if(pdp->key_ex_nxt)
			MA_INC;
		if(pdp->key_dep_sync) {
			MA_CLR;
			AR_CLR;
		}
		if(pdp->key_dep_nxt) {
			MA_INC;
			AR_CLR;
		}
		if(pdp->key_io_reset)
			mr_start(pdp);
		P(KT1);
		DLY(200);


		if(pdp->key_read_in || pdp->key_start)
			MA |= pdp->mas;
		if(pdp->key_execute)
			AR |= pdp->datasw;
		if(pdp->key_ex_sync)
			MA |= pdp->mas;
		if(pdp->key_dep_sync) {
			MA |= pdp->mas;
			AR |= pdp->datasw;
		}
		if(pdp->key_dep_nxt)
			AR |= pdp->datasw;
		P(KT2);
		DLY(200);


		if(pdp->key_read_in || pdp->key_start)
			PC_FM_MA_1;
		if(pdp->key_execute)
			MB_FM_AR_J;
		P(KT3);

		if(pdp->key_ex_sync || pdp->key_ex_nxt) {
			MEM_RQ(rdrq, KEY_RDWR_RET, key_rd_wr);
			// --------------------
		}
		if(pdp->key_dep_sync || pdp->key_dep_nxt) {
			MB_FM_AR_J;
			MEM_RQ(wrrq, KEY_RDWR_RET, key_rd_wr);
			// --------------------
		}

		pdp->cycling = 0;
		if(pdp->key_execute) {
			pdp->cycling = 1;
			DLY(100);
			goto it1a;
		}
		if(pdp->key_start || pdp->key_read_in || pdp->key_inst_cont)
			key_go(pdp);
		break;

		MEM_RET(KEY_RDWR_RET, key_rd_wr);
		pdp->ex_ill_op = 0;
		P(KEY_RDWR_RET);
		if(pdp->key_ex_nxt || pdp->key_dep_nxt)
			MI = MB;
		P(KT4);
		pdp->cycling = 0;
		if(pdp->run && (pdp->key_ex_st || pdp->key_ex_st))
			key_go(pdp);
		break;

	case IT0:
		MA_CLR;
		MR_CLR;
		P(IT0);
		FF(if1a, 1);

		P(PI_SYNC);
		if(!pdp->pi_cyc)
			PIR_STB;

		DLY(200);


		if(pdp->pi_req && !pdp->pi_cyc) {
iat0:			MR_CLR;
			pdp->pi_cyc = 1;
			pdp->ex_pi_sync = 1;
			P(IAT0);

			DLY(200);
		} else if(pdp->ia_inh) {
			pdp->cycling = 0;
			break;
		}

		if(pdp->pi_cyc) {
			MA = 040;
			if(pdp->pi_req & 0017) MA |= 010;
			if(pdp->pi_req & 0063) MA |= 004;
			if(pdp->pi_req & 0125) MA |= 002;
			if(pdp->pi_ov) MA |= 1;
		} else
			MA |= PC;
		P(IT1);

		MEM_RQ(rdrq, IT1A, if1a);
		// --------------------
it1a:		MEM_RET(IT1A, if1a);
		IR |= (MB>>18) & 0777740;
		if((MA & 0777760) != 0) pdp->key_rim_sbr = 0;
		P(IT1A);

at0:		MEM_RET(AT0, af0);
		AR_FM_MBRT_J;
		IR |= (MB>>18) & 037;
		MA_CLR;
		P(AT0);

		P(PI_SYNC);
		if(!pdp->pi_cyc)
			PIR_STB;

		DLY(200);

		if(pdp->pi_req && !pdp->pi_cyc)
			goto iat0;
		else if(pdp->ia_inh) {
			pdp->cycling = 0;
			break;
		}

		FF(ex_uuo_sync, 1);
		P(AT1);

		if(IR & 017) {
			MA |= IR & 017;
			P(AT2);

			MEM_RQ(rdrq, AT3, af3);
			// --------------------
			MEM_RET(AT3, af3);
			MA_CLR;
			P(AT3);

			SBR(AR_ADD, af3a);

			MB_FM_AR_J;
			P(AT3A);

			DLY(200);
		}

		AR &= RT;
		P(AT4);

		if(IR & 020) {
			MA_FM_MBRT_1;
			IR &= 0777740;
			P(AT5);

			MEM_RQ(rdrq, AT0, af0);
			// --------------------
		}

ft0:		P(FT0);
		decode(pdp);
		if(!(pdp->code & CODE_FAC_INH)) {
			MA |= (IR>>5) & 017;
			P(FT1);

			MEM_RQ(rdrq, FT1A, f1a);
			// --------------------
ft1a:			MEM_RET(FT1A, f1a);
			if(pdp->code & CODE_FAC2) MA_INC4;
			else MA_CLR;
			if(pdp->code & CODE_FCCACLT) MB_SWAP;
			else if(pdp->code & CODE_FCCACRT) ;	// nothing
			else SWAP_MB_AR;
			P(FT1A);

			DLY(100);

			if(pdp->code & CODE_FAC2_ETC) {
				if(pdp->code & (CODE_FCCACLT|CODE_FCCACRT)) {
					MA_FM_MBRT_1;
					SWAP_MB_AR;
					P(FT3);

					DLY(100);
				}

				MQ_FM_MB_J;
				P(FT4);

				MEM_RQ(rdrq, FT4A, f4a);
				// --------------------
				MEM_RET(FT4A, f4a);
				MA_CLR;
				SWAP_MB_MQ;
				P(FT4A);

				DLY(100);
			} // FAC2_ETC
		} // FAC_INH

		MA_FM_MBRT_1;
		P(FT5);

		if(pdp->code & CODE_FCE) {
			P(FT6);

			MEM_RQ(rdrq, FT6A, f6a);
			// --------------------
		} else if(pdp->code & CODE_FCE_PSE) {
			P(FT7);

			MEM_RQ(rdwrrq, FT6A, f6a);
			// --------------------
		}
		MEM_RET(FT6A, f6a);
		P(FT6A);

		// ET0A
		if(PI_HOLD)
			pi_hold(pdp);
		if(pdp->key_ex_sync) pdp->key_ex_st = 1;
		if(pdp->key_dep_sync) pdp->key_dep_st = 1;
		if(pdp->key_inst_stop) clr_run(pdp);

		// ET0
		if(!pdp->pi_cyc && !(pdp->code & CODE_PC_INC_INH) && !pdp->key_execute)
			PC_INC;
		pdp->ar_cry0 = 0;
		pdp->ar_cry1 = 0;

		// dispatch to instruction
		pdp->state = pdp->estate;
		break;

	case UUO:
					P(ET0);
					DLY(100);
		MA_CLR;
		MBLT_CLR;
		pdp->ex_ill_op = 1;
					P(ET1);
					DLY(100);
		MA |= 040;
					P(ET3);
		MB |= (Word)(IR&0777740) << 18;
					P(UUO_T0);

		MEM_RQ(wrrq, UUOT1, uuo_f1);
		// --------------------
		MEM_RET(UUOT1, uuo_f1);
		MA_INC;
		MR_CLR;
					P(UUO_T1);
					DLY(100);
					P(UUO_T2);
		MEM_RQ(rdrq, IT1A, if1a);
		// --------------------

	case 0132:	// FSC
		SC_FM_MB;
					P(ET0);
					DLY(100);
		if(!AR_NEG)
			SC_COM;
					P(ET1);
					DLY(100);
					P(ET3);
		if(AR_NEG) {
			SC_INC;
					P(FST1);
		}
					P(FST0);
		SBR(SC_ADD((AR>>27)&0777), fsf1);
		if(AR_NEG  != SC_NEG)
			ar_set_ov(pdp);
		AR_FM_SC_J;
					P(FST0A);
		goto et10;

	// FAD
	case 0140:
	case 0141:
	case 0142:
	case 0143:
	case 0144:
	case 0145:
	case 0146:
	case 0147:
					P(ET0);
fat0:
		if(AR_NEG == MB_NEG)
			SC_COM;
		FF(faf1, 1);
					P(FAT0);
					DLY(100);
		SC_PAD((MB>>27)&0777);
					P(FAT1);
		FF(faf1, 0);
					P(FAT1B);
		SBR(SC_ADD((AR>>27)&0777), faf2);
		if(AR_NEG == SC_NEG)
			SWAP_MB_AR;
					P(FAT1A);
		if(!SC_NEG) {
			SC_INC;
					P(FAT2);
					DLY(150);
			SC_COM;
					P(FAT3);
					DLY(150);
		} else {
					P(FAT4);
					DLY(100);
		}
		if((SC&0700)==0700) {
					P(FAT5);
			EXP_CLR(AR);
			SBR(SCT_ASHCRT, faf3);
		} else {
			AR_CLR;
					P(FAT6);
		}
		SC_CLR;
		FF(faf1, 1);
					P(FAT5A);
					DLY(100);
		if(MB_NEG)
			SC_COM;
					P(FAT7);
					DLY(100);
		SC_PAD((MB>>27)&0777);
					P(FAT8);
					DLY(50);
		EXP_CLR(MB);
					P(FAT8A);
					DLY(100);
					P(FAT9);
		SBR(AR_ADD, faf4);
		FF(faf1, 0);
					P(FAT10);
		goto nrt05;

	// FSB
	case 0150:
	case 0151:
	case 0152:
	case 0153:
	case 0154:
	case 0155:
	case 0156:
	case 0157:
		SWAP_MB_AR;
					P(ET0);
					DLY(100);
					P(ET1);
					DLY(100);
					P(ET3);
		SBR(AR_NEGATE, et4_ar_pause);
					P(ET4);
		goto fat0;

	// FMP
	case 0160:
	case 0161:
	case 0162:
	case 0163:
	case 0164:
	case 0165:
	case 0166:
	case 0167:
					P(ET0);
					P(FMT0);
		SBR(exp_calc(pdp, 1), fmf1);
		SC |= 0744;
					P(FMT0A);
		SBR(multiply(pdp), fmf2);
		SC_FM_FE_1;
		pdp->nrf2 = 1;
					P(FMT0B);
		goto nrt0dly;

	// FDV
	case 0170:
	case 0171:
	case 0172:
	case 0173:
	case 0174:
	case 0175:
	case 0176:
	case 0177:
					P(ET0);
					P(FDT0);
		SBR(exp_calc(pdp, 0), fdf1);
		SC |= 0741;
					P(FDT0A);
		FF(fdf2, 1);
		if(AR_NEG) {
			pdp->dsf7 = 1;
					P(DST0);
			SBR(AR_NEGATE, dsf1);
					P(DST0A);
		}
		if(divide(pdp))
			goto st7;
		FF(fdf2, 0);
		SC_FM_FE_1;
		pdp->nrf2 = 1;
					P(FDT0B);
					DLY(100);
		ashcrt(pdp);
					P(FDT1);
		goto nrt05;


	// Normalize return
nrt05:
		FF(nrf2, 1);
					P(NRT05);
					DLY(100);
		SC_INC;
		ashcrt(pdp);
					P(NRT0);
					P(NRT01);
					DLY(200);
nrt0dly:
// fm entry
		if(AR_ZERO && !(MQ&F1))
			goto nrt6;
		SC_COM;
					P(NRT1);
					DLY(100);
		while((AR&MANTBITS)!=0400000000 &&
		      AR_NEG == !!(AR&F9)) {
			SC_INC;
			ashclt(pdp);
					P(NRT2);
					DLY(150);
		}
		if(!SC_NEG)
			ar_set_ov(pdp);
		if(!AR_NEG || NR_ROUND)
			SC_COM;
					P(NRT3);
					P(NRT31);
					DLY(100);
		if(NR_ROUND) {
					P(NRT5);
			SBR(AR_INC, nrf1);
			pdp->nrf3 = 1;
					P(NRT5A);
			goto nrt05;
		}
		AR_FM_SC_J;
					P(NRT4);
nrt6:
					P(NRT6);
		if(pdp->code & CODE_SCE)
			MB_FM_AR_J;
		goto et10;

	case CH_INC_OP:
					P(ET0);
		AR_FM_MB_J;
		FF(chf1, 1);
					P(CHT1);
					DLY(100);
		SC_PAD((~MB>>30)&077 | 0700);
					P(CHT2);
		for(;;) {
					P(CHT3);
			SBR(SC_ADD((MB>>24)&077), chf2);
					P(CHT3A);
			if(SC_NEG)
				break;
			SC_CLR;
					P(CHT4);
			SBR(AR_INC, chf3);
			SC |= 0433;
					P(CHT4A);
		}
		SC_COM;
					P(CHT5);
					DLY(100);
		AR = (AR&~0770000000000) | (Word)(SC&077)<<30;
		SC_CLR;
		if(0) {
	case CH_NOT_INC_OP:
					P(ET0);
		}
		FF(chf2, 1);
					P(CHT6);
					DLY(150);
		SC_PAD((MB>>24)&077);
		if(pdp->state == CH_INC_OP)
			SWAP_MB_AR;
					P(CHT7);
					DLY(100);
		if(pdp->state == CH_INC_OP) {
					P(CHT8);
			MEM_RQ(wrrs, CHT8B, chf6);
			// --------------------
		}
		MEM_RET(CHT8B, chf6);
		FF(chf2, 0);
		FE_FM_MB_1;
		SC_COM;
					P(CHT8B);
		if((IR&0777000) == 0133000)
			goto st7;
		SBR(sc_count(pdp, char_first), chf4);
		SC_CLR;
		IR &= ~037;
					P(CHT8A);
					DLY(100);
		SC_FM_FE_1;
		pdp->chf5 = 1;
		pdp->chf7 = 1;
					P(CHT9);
		goto at0;

	case CH_LOAD:
					P(ET0);
		AR_FM_MB_J;
		MB_FM_MQ_J;
		SC_COM;
					P(LCT0);
		SBR(sc_count(pdp, lshrt), lcf1);
		AR_FM_MB_0;
		pdp->chf7 = 0;
					P(LCT0A);
		goto et10;

	case CH_DEP:
					P(ET0);
		SC_COM;
					P(DCT0);
		SBR(sc_count(pdp, ch_dep), dcf1);
		t = MB;
		MB_FM_MQ_J;
		MQ |= t;
		AR_COM;
					P(DCT0A); // and B
					DLY(150);
		AR_FM_MB_0;
		MB_FM_MQ_J;
					P(DCT1);
					DLY(100);
		AR_COM;
					P(DCT2);
					DLY(100);
		MB_FM_AR_0;
		pdp->chf7 = 0;
					P(DCT3);
		goto et10;

	// IMUL
	case 0220:
	case 0221:
	case 0222:
	case 0223:
					P(ET0);
		SC |= 0734;
		if(MB & AR & F0)
			pdp->mpf2 = 1;
					P(MPT0);
		SBR(multiply(pdp), mpf1);
		if(AR_NEG) {
			if(pdp->mpf2)
				ar_set_ov(pdp);
			AR_COM;
		}
					P(MPT0A);
		MB_FM_MQ_J;
		if(AR)
			ar_set_ov(pdp);
					P(MPT1);
					DLY(100);
		AR_FM_MB_J;
					P(MPT2);
					DLY(100);
		goto nrt6;

	// MUL
	case 0224:
	case 0225:
	case 0226:
	case 0227:
					P(ET0);
		SC |= 0734;
		if(MB & AR & F0)
			pdp->mpf2 = 1;
					P(MPT0);
		SBR(multiply(pdp), mpf1);
		if(AR_NEG) {
			if(pdp->mpf2)
				ar_set_ov(pdp);
		}
					P(MPT0A);
		goto nrt6;

	// IDIV
	case 0230:
	case 0231:
	case 0232:
	case 0233:
					P(ET0);
		SC |= 0733;
					P(DS_DIV_T0);
		if(AR_NEG) {
			pdp->dsf7 = 1;
					P(DST0);
			SBR(AR_NEGATE, dsf1);
					P(DST0A);
		}
		MQ_FM_MB_J;
		MB_FM_AR_J;
					P(DST1);
					DLY(150);
		SWAP_MB_MQ;
		AR_CLR;
					P(DST2);
		goto div;

	// DIV
	case 0234:
	case 0235:
	case 0236:
	case 0237:
					P(ET0);
		SC |= 0733;
					P(DS_DIV_T0);
		if(AR_NEG) {
			SWAP_MB_MQ;
			pdp->dsf7 = 1;
					P(DST3);
					DLY(100);
			SWAP_MB_AR;
					P(DST4);
					DLY(100);
					P(DST5);
			SBR(AR_NEGATE, dsf2);
					P(DST5A);
			// bug in schematics: have to ignore AR0
			if(AR&NF0) {
				SWAP_MB_AR;
					P(DST6);
					DLY(100);
				SWAP_MB_MQ;
				AR_COM;
					P(DST7);
			} else {
				SWAP_MB_AR;
					P(DST8);
					DLY(100);
				SWAP_MB_MQ;
					P(DST9);
				SBR(AR_NEGATE, dsf3);
			}
		}
div:
		if(divide(pdp))
			goto st7;
					P(ET9);
					DLY(200);
		if(pdp->code & CODE_SCE)
			MB_FM_AR_J;
		goto et10;

#include "instgen.inc"

	case 0250:	// EXCH
		SWAP_MB_AR;
nop:
					P(ET0);
					DLY(100);
					P(ET1);
					DLY(100);
					P(ET3);
					P(ET4);
					DLY(100);
					P(ET5);
		goto et10;

	case 0251:	// BLT
		SWAP_MB_AR;
					P(ET0);
					DLY(100);
		MB_SWAP;
		MA_CLR;
					P(ET1);
					DLY(100);
		MA_FM_MBRT_1;
					P(ET3);
		SWAP_MB_MQ;
					P(BLT_T0);
		MEM_RQ(wrrq, BLTT0A, blt_f0a);
		// --------------------
		MEM_RET(BLTT0A, blt_f0a);
		MB_FM_MQ_J;
					P(BLT_T0A);
					DLY(100);
		SWAP_MB_AR;
					P(BLT_T1);
					DLY(100);
		ARLT_CLR;
					P(BLT_T2);
					DLY(100);
					P(BLT_T3);
		SBR(AR_SUB, blt_f3a);
		SWAP_MB_MQ;
					P(BLT_T3A);
					DLY(100);
		SWAP_MB_AR;
					P(BLT_T4);
		PIR_STB;
					DLY(100);
		SWAP_MB_MQ;
					P(BLT_T5);
		SBR(AR_INC_LTRT, blt_f5a);
		if(!(MQ & F0)) {
			PC_INC;
			pdp->code |= CODE_SAC_INH;
		}
					P(BLT_T5A);
		if(!(MQ & F0) || pdp->pi_req)
			goto et10;
		SWAP_MB_AR;
					P(BLT_T6);
					DLY(100);
		goto ft1a;

	case 0252:	// AOBJP
	case 0253:	// AOBJN
					P(ET0);
					DLY(100);
					P(ET1);
					DLY(100);
					P(ET3);
		SBR(AR_INC_LTRT, et4_ar_pause);
					P(ET4);
					DLY(100);
					P(ET5);
					DLY(100);
		// can't be bothered to split them just for this
		test = AR_NEG == (pdp->state&1);
					P(ET6);
					DLY(100);
		if(test)
			PC_CLR;
					P(ET7);
					DLY(100);
		if(test)
			PC_FM_MA_1;
					P(ET8);
					DLY(200);
		if(test)
			PC_CHG;
					P(ET9);
					DLY(200);
		goto et10;

	case 0254:	// JRST
		if(pdp->ir & H10) clr_run(pdp);
		if(pdp->ir & H11) ar_flag_clr(pdp);
					P(ET0);
					DLY(100);
		if(pdp->ir & H9) pi_rst(pdp);
		if(pdp->ir & H11) ar_flag_set(pdp);
		if(pdp->ir & H12) pdp->ex_mode_sync = 1;
					P(ET1);
					DLY(100);
					P(ET3);
					P(ET4);
					DLY(100);
		MB_CLR;
					P(ET5);
					DLY(100);
		MB_FM_PC_1;
					P(ET6);
					DLY(100);
		PC_CLR;
					P(ET7);
					DLY(100);
		PC_FM_MA_1;
					P(ET8);
					DLY(200);
		if(!(pdp->ir & H11)) PC_CHG;
		MA_CLR;
					P(ET9);
					DLY(200);
		MA_FM_MBRT_1;
		goto et10;

	case 0255:	// JFCL
					P(ET0);
					DLY(100);
					P(ET1);
					DLY(100);
					P(ET3);
					P(ET4);
					DLY(100);
					P(ET5);
					DLY(100);
					P(ET6);
					DLY(100);
		test = SELECTED_FLAGS;
		if(test) PC_CLR;
					P(ET7);
					DLY(100);
		if(test) PC_FM_MA_1;
					P(ET8);
					DLY(200);
		if(test) PC_CHG;
					P(ET9);
					DLY(200);
		ar_jfcl_clr(pdp);
		goto et10;


	case 0256:	// XCT
					P(ET0);
					DLY(100);
					P(ET1);
					DLY(100);
					P(ET3);
		MR_CLR;
					DLY(200);
		goto it1a;

	case 0257:	// NOP
		goto nop;

	case 0260:	// PUSHJ
					P(ET0);
					DLY(100);
					P(ET1);
					DLY(100);
					P(ET3);
		SBR(AR_INC_LTRT, et4_ar_pause);
					P(ET4);
					DLY(100);
		MB_CLR;
					P(ET5);
					DLY(100);
		MB_FM_PC_1;
		MB_FM_MISC_BITS;
					P(ET6);
					DLY(100);
		PC_CLR;
					P(ET7);
					DLY(100);
		PC_FM_MA_1;
					P(ET8);
					DLY(200);
		SWAP_MB_AR;
		PC_CHG;
		MA_CLR;
					P(ET9);
					DLY(200);
		MA_FM_MBRT_1;
		SWAP_MB_AR;
		PDL_OV;
		goto et10;

	case 0261:	// PUSH
					P(ET0);
					DLY(100);
					P(ET1);
					DLY(100);
					P(ET3);
		SBR(AR_INC_LTRT, et4_ar_pause);
					P(ET4);
					DLY(100);
					P(ET5);
					DLY(100);
					P(ET6);
					DLY(100);
					P(ET7);
					DLY(100);
					P(ET8);
					DLY(200);
		SWAP_MB_AR;
		MA_CLR;
					P(ET9);
					DLY(200);
		MA_FM_MBRT_1;
		SWAP_MB_AR;
		PDL_OV;
		goto et10;

	case 0262:	// POP
		MB_FM_MQ_J;
					P(ET0);
					DLY(100);
					P(ET1);
					DLY(100);
					P(ET3);
		SBR(AR_DEC_LTRT, et4_ar_pause);
					P(ET4);
					DLY(100);
					P(ET5);
					DLY(100);
					P(ET6);
					DLY(100);
					P(ET7);
					DLY(100);
					P(ET8);
					DLY(200);
		SWAP_MB_AR;
					P(ET9);
					DLY(200);
		SWAP_MB_AR;
		PDL_OV;
		goto et10;

	case 0263:	// POPJ
		MB_FM_MQ_J;
					P(ET0);
					DLY(100);
		MA_CLR;
					P(ET1);
					DLY(100);
		MA_FM_MBRT_1;
					P(ET3);
		SBR(AR_DEC_LTRT, et4_ar_pause);
					P(ET4);
					DLY(100);
					P(ET5);
					DLY(100);
					P(ET6);
					DLY(100);
		PC_CLR;
					P(ET7);
					DLY(100);
		PC_FM_MA_1;
					P(ET8);
					DLY(200);
		SWAP_MB_AR;
		PC_CHG;
					P(ET9);
					DLY(200);
		SWAP_MB_AR;
		PDL_OV;
		goto et10;

	case 0264:	// JSR
					P(ET0);
					DLY(100);
					P(ET1);
					DLY(100);
					P(ET3);
					P(ET4);
					DLY(100);
		MB_CLR;
					P(ET5);
					DLY(100);
		MB_FM_PC_1;
		MB_FM_MISC_BITS;
					P(ET6);
					DLY(100);
		PC_CLR;
		if(pdp->ex_pi_sync || pdp->ex_ill_op)
			pdp->ex_user = 0;
					P(ET7);
					DLY(100);
		PC_FM_MA_1;
		pdp->ex_ill_op = 0;
					P(ET8);
					DLY(200);
		SKIP;
		pdp->chf7 = 0;
					P(ET9);
					DLY(200);
		goto et10;

	case 0265:	// JSP
		SWAP_MB_AR;
					P(ET0);
					DLY(100);
					P(ET1);
					DLY(100);
					P(ET3);
					P(ET4);
					DLY(100);
		MB_CLR;
					P(ET5);
					DLY(100);
		MB_FM_PC_1;
		MB_FM_MISC_BITS;
					P(ET6);
					DLY(100);
		PC_CLR;
					P(ET7);
					DLY(100);
		PC_FM_MA_1;
					P(ET8);
					DLY(200);
		SWAP_MB_AR;
		PC_CHG;
					P(ET9);
					DLY(200);
		goto et10;

	case 0266:	// JSA
		MB_SWAP;
					P(ET0);
					DLY(100);
					P(ET1);
					DLY(100);
					P(ET3);
					P(ET4);
					DLY(100);
					P(ET5);
					DLY(100);
		MB_FM_PC_1;
					P(ET6);
					DLY(100);
		PC_CLR;
					P(ET7);
					DLY(100);
		PC_FM_MA_1;
					P(ET8);
					DLY(200);
		SWAP_MB_AR;
		SKIP;
					P(ET9);
					DLY(200);
		goto et10;

	case 0267:	// JRA
		MB_FM_MQ_J;
					P(ET0);
					DLY(100);
					P(ET1);
					DLY(100);
					P(ET3);
					P(ET4);
					DLY(100);
					P(ET5);
					DLY(100);
					P(ET6);
					DLY(100);
		PC_CLR;
					P(ET7);
					DLY(100);
		PC_FM_MA_1;
					P(ET8);
					DLY(200);
		SWAP_MB_AR;
		PC_CHG;
					P(ET9);
					DLY(200);
		goto et10;



et10:
		if(PI_HOLD)
			pi_exit(pdp);
		P(ET10);

		if(pdp->code & CODE_SCE) {
			P(ST1);

			MEM_RQ(wrrq, ST3, sf3);
			// --------------------
		} else if(pdp->code & CODE_FCE_PSE) {
			P(ST2);

			MEM_RQ(wrrs, ST3, sf3);
			// --------------------
		}

		MEM_RET(ST3, sf3);
		if(!(pdp->code & CODE_SAC_INH))
			MA_CLR;
		P(ST3);

		if(!(pdp->code & CODE_SAC_INH)) {
			P(ST3A);

			DLY(100);

			MA |= (IR>>5) & 017;
			MB_FM_AR_J;
			P(ST5);

			MEM_RQ(wrrq, ST5A, sf5a);
			// --------------------
			MEM_RET(ST5A, sf5a);
			P(ST5A);

			if(pdp->code & CODE_SAC2) {
				MA_INC4;
				MB = MQ;
				P(ST6);

				P(ST6A);

				MEM_RQ(wrrq, ST7, sf7);
				// --------------------
			}
		} // SAC_INH

st7:		MEM_RET(ST7, sf7);
		P(ST7);

		if(pdp->run) {
			if(pdp->key_ex_st || pdp->key_dep_st)
				pdp->state = KT1;
			else
				pdp->state = IT0;
		} else
		// TODO: KT4 to repeat
			pdp->cycling = 0;
		break;

	case IOT_BLKI:
	case IOT_BLKO:
		AR_FM_MB_J;
					P(ET0);
					DLY(100);
					P(ET1);
					DLY(100);
					P(ET3);
		SBR(AR_INC_LTRT, et4_ar_pause);
		SWAP_MB_AR;
		pdp->ex_ill_op = 0;
					P(ET4);
					P(IOT_T0);
		MEM_RQ(wrrs, IOTT0A, iot_f0a);
		// --------------------
		MEM_RET(IOTT0A, iot_f0a);
		IR |= H12;
		MA_CLR;
		if(pdp->pi_cyc) {
			if(pdp->ar_cry0)
				pdp->pi_ov = 1;
		} else {
			if(!pdp->ar_cry0)
				PC_INC;
		}
					P(IOT_T0A);
					DLY(200);
		goto ft0;

	/* In the following instructions we're ignoring
	 * the FINAL SETUP and IOT RESET delays.
	 * for more timing accuracy we will need IOT RESET eventually. */
	case IOT_DATAI:
					P(ET0);
					DLY(100);
		AR_CLR;
		if(pdp->pi_cyc && !pdp->pi_ov)
			pi_rst(pdp);
					P(ET1);
					DLY(100);
					P(ET3);
		FF(iot_go, 1);
					P(ET4);
		IO_PULSE(IOB_DATAI);
		DLY(1000);	// initial setup
		FF(iot_go, 0);
					P(IOT_T2);
		DLY(1000);	// restart
		AR |= IOB;
					P(IOT_T3);
					DLY(200);
		IOB = 0;	 // approx
					P(ET5);
		MB_FM_AR_J;
		goto et10;

	case IOT_CONI:
	case IOT_CONSZ:
	case IOT_CONSO:
		AR_FM_MB_J;
					P(ET0);
					DLY(100);
		AR_CLR;
					P(ET1);
					DLY(100);
					P(ET3);
		FF(iot_go, 1);
					P(ET4);
		IOB = 0;
		IO_PULSE(IOB_STATUS);
		DLY(1000);	// initial setup
		FF(iot_go, 0);
					P(IOT_T2);
		DLY(1000);	// restart
		AR |= IOB;
					P(IOT_T3);
					DLY(200);
		IOB = 0;	 // approx
					P(ET5);
		if(pdp->state == IOT_CONI)
			MB_FM_AR_J;
		else {
					DLY(100);
			AR_FM_MB_0;
					P(ET6);
					DLY(100);
					P(ET7);
					P(ET8);
					DLY(200);
			if(pdp->state == IOT_CONSZ && AR_ZERO ||
			   pdp->state == IOT_CONSO && !AR_ZERO)
				SKIP;
					P(ET9);
					DLY(200);
		}
		goto et10;

	case IOT_DATAO:
		AR_FM_MB_J;
					P(ET0);
					DLY(100);
		if(pdp->pi_cyc && !pdp->pi_ov)
			pi_rst(pdp);
					P(ET1);
					DLY(100);
					P(ET3);
		FF(iot_go, 1);
					P(ET4);
		IOB |= AR;
		DLY(1000);	// initial setup
		FF(iot_go, 0);
					P(IOT_T2);
		IO_PULSE(IOB_DATAO_CLR);
		DLY(1000);	// restart
					P(IOT_T3);
		IO_PULSE(IOB_DATAO_SET);
					DLY(200);
		IOB = 0;	 // approx
					P(ET5);
		goto et10;

	case IOT_CONO:
		MB_SWAP;
					P(ET0);
					DLY(100);
					P(ET1);
					DLY(100);
					P(ET3);
		AR_FM_MBLT_J;
		FF(iot_go, 1);
					P(ET4);
		IOB |= AR;
		DLY(1000);	// initial setup
		FF(iot_go, 0);
					P(IOT_T2);
		IO_PULSE(IOB_CONO_CLR);
		DLY(1000);	// restart
					P(IOT_T3);
		IO_PULSE(IOB_CONO_SET);
					DLY(200);
		IOB = 0;	 // approx
					P(ET5);
		goto et10;

	default:
		printf("unknown state %o\n", pdp->state);
		pdp->cycling = 0;
		break;
	}
}

enum {
	IR0 = 0400,
	IR1 = 0200,
	IR2 = 0100,
	IR3 = 0040,
	IR4 = 0020,
	IR5 = 0010,
	IR6 = 0004,
	IR7 = 0002,
	IR8 = 0001
};

void
predecode(void)
{
	Code code, sac2;
	int ir;

	for(ir = 0; ir < 01000; ir++) {
		code = 0;
		switch(ir&0700) {
		case 0000:
			code |= CODE_UUO | CODE_FAC_INH | CODE_PC_INC_INH;
			break;

		case 0100:
			switch(ir&0770) {
			case 0130:
				switch(ir) {
				case 0132:	// FSC
					break;

				case 0133:	// IBP
				case 0134:	// ILDB
				case 0135:	// LDB
				case 0136:	// IDPB
				case 0137:	// DPB
					code |= CODE_CHAR;
					break;
				}
				break;

			case 0140:	// FAD
			case 0150:	// FSB
			case 0160:	// FMP
			case 0170:	// FDV
				code |= CODE_FCE;
				switch(ir&3) {
				case 0: break;
				case 1: code |= CODE_SAC2; break;
				case 2: code |= CODE_SCE | CODE_SAC_INH; break;
				case 3: code |= CODE_SCE; break;
				}
				break;
			}
			break;

		case 0200:
			switch(ir&0770) {
			case 0200:		// FWT
			case 0210:
				switch(ir&3) {
				case 0: code |= CODE_FAC_INH | CODE_FCE; break;
				case 1: code |= CODE_FAC_INH; break;
				case 2: code |= CODE_SCE | CODE_SAC_INH; break;
				case 3: code |= CODE_FAC_INH | CODE_FCE_PSE | CODE_SAC0_INH; break;
				}
				break;

			case 0220:		// IMUL/MUL
				sac2 = ir&IR6 ? CODE_SAC2 : 0;
				goto md;
			case 0230:		// IDIV/DIV
				if(ir&IR6) code |= CODE_FAC2;
				sac2 = CODE_SAC2;
			md:
				switch(ir&3) {
				case 0: code |= CODE_FCE | sac2; break;
				case 1: code |= sac2; break;
				case 2: code |= CODE_FCE | CODE_SCE | CODE_SAC_INH; break;
				case 3: code |= CODE_FCE | CODE_SCE | sac2; break;
				}
				break;

			case 0240:		// Shift/Rot
				switch(ir) case 0244: case 0245: case 0246:
					code |= CODE_FAC2 | CODE_SAC2;
				break;
			case 0250:		// Misc
			case 0260:
				switch(ir) {
				case 0250:	// EXCH
					code |= CODE_FCE_PSE;
					break;
				case 0251:	// BLT
					code |= CODE_FCCACLT | CODE_PC_INC_INH;
					break;
				case 0252:	// AOBJP
				case 0253:	// AOBJN
					break;
				case 0254:	// JRST
					code |= CODE_JRST | CODE_FAC_INH | CODE_SAC_INH;
					break;
				case 0255:	// JFCL
					code |= CODE_FAC_INH | CODE_SAC_INH;
					break;
				case 0256:	// XCT
					code |= CODE_FAC_INH | CODE_SAC_INH | CODE_FCE | CODE_PC_INC_INH;
					break;
				case 0257:
					code |= CODE_FAC_INH | CODE_SAC_INH;
					break;
				case 0260:	// PUSHJ
					code |= CODE_SCE;
					break;
				case 0261:	// PUSH
					code |= CODE_FCE | CODE_SCE;
					break;
				case 0262:	// POP
					code |= CODE_FCCACRT | CODE_SCE;
					break;
				case 0263:	// POPJ
					code |= CODE_FCCACRT;
					break;
				case 0264:	// JSR
					code |= CODE_FAC_INH | CODE_SCE | CODE_SAC_INH;
					break;
				case 0265:	// JSP
					code |= CODE_FAC_INH;
					break;
				case 0266:	// JSA
					code |= CODE_SCE;
					break;
				case 0267:	// JRA
					code |= CODE_FCCACLT;
					break;
				}
				break;

			case 0270:		// ADD/SUB
				goto boole_as;
			}
			break;

		case 0300:	// ACCP, MEMAC
			switch(ir&0760) {
			case 0300:	// ACCP
				if(ir&IR5)
					code |= CODE_FCE;
				code |= CODE_SAC_INH;
				break;
			case 0320:	// MEMAC TST
			case 0340:	// MEMAC +1
			case 0360:	// MEMAC -1
				if(ir&IR5)
					code |= CODE_FAC_INH | CODE_FCE_PSE | CODE_SAC0_INH;
				break;
			}
			break;

		case 0400:	// BOOLE
		boole_as:
			switch(ir&3) {
			case 0: code |= CODE_FCE; break;
			case 1: break;
			case 2: code |= CODE_FCE_PSE | CODE_SAC_INH; break;
			case 3: code |= CODE_FCE_PSE; break;
			}
			break;

		case 0500:	// HWT
			switch(ir&3) {
			case 0: code |= CODE_FCE; break;
			case 1: break;
			case 2: code |= CODE_FCE_PSE | CODE_SAC_INH; break;
			case 3: code |= CODE_FAC_INH | CODE_FCE_PSE | CODE_SAC0_INH; break;
			}
			break;

		case 0600:	// ACBM
			if(ir&IR5)
				code |= CODE_FCE;
			if((ir&060) == 0)
				code |= CODE_SAC_INH;
			break;

		case 0700:
			code |= CODE_IOT;
			break;
		}
		codetab[ir] = code;
	}

	for(ir = 0; ir < 8; ir++) {
		code = CODE_IOT | CODE_FAC_INH | CODE_SAC_INH;
		switch(ir) {
		case IOT_BLKI:
			code |= CODE_FCE_PSE | CODE_PC_INC_INH;
			break;

		case IOT_DATAI:
			code |= CODE_SCE | CODE_PI_HOLD;
			break;

		case IOT_BLKO:
			code |= CODE_FCE_PSE | CODE_PC_INC_INH;
			break;

		case IOT_DATAO:
			code |= CODE_FCE | CODE_PI_HOLD;
			break;

		case IOT_CONO:
			break;

		case IOT_CONI:
			code |= CODE_SCE;
			break;

		case IOT_CONSZ:
			break;

		case IOT_CONSO:
			break;

		}
		iotcodetab[ir] = code;
	}
/*
#define X(bit) if(code & bit) printf("\t%s", #bit); else printf("\t\t");
	for(ir = 0; ir < 01000; ir++) {
		code = codetab[ir];
		printf("%03o", ir);
	X(CODE_UUO)
	X(CODE_IOT)
	X(CODE_JRST)
	X(CODE_CHAR)
	X(CODE_FAC_INH)
	X(CODE_FAC2)
	X(CODE_FCCACLT)
	X(CODE_FCCACRT)
	X(CODE_FCE)
	X(CODE_FCE_PSE)
	X(CODE_SAC_INH)
	X(CODE_SAC0_INH)
	X(CODE_SAC2)
	X(CODE_SCE)
		printf("\n");
	}
*/
}


/* Not sure where to put this atm */

int ncleanups;
struct {
	void (*func)(void *arg);
	void *arg;
} cleanups[1000];

void
exitcleanup(void)
{
	for(int i = 0; i < ncleanups; i++)
		cleanups[i].func(cleanups[i].arg);
}

void
addcleanup(void (*func)(void*), void *arg)
{
	cleanups[ncleanups].func = func;
	cleanups[ncleanups].arg = arg;
	ncleanups++;
}
