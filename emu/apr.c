#include "pdp6.h"
#include <unistd.h>

// Schematics don't have USER IOT implemented fully
#define FIX_USER_IOT
// Schematics have a bug in the divide subroutine
#define FIX_DS

enum Opcode {
	FSC    = 0132,
	IBP    = 0133,
	CAO    = 0133,
	LDCI   = 0134,
	LDC    = 0135,
	DPCI   = 0136,
	DPC    = 0137,
	ASH    = 0240,
	ROT    = 0241,
        LSH    = 0242,
	ASHC   = 0244,
	ROTC   = 0245,
	LSHC   = 0246,
	EXCH   = 0250,
	BLT    = 0251,
	AOBJP  = 0252,
	AOBJN  = 0253,
	JRST   = 0254,
	JFCL   = 0255,
	XCT    = 0256,
	PUSHJ  = 0260,
	PUSH   = 0261,
	POP    = 0262,
	POPJ   = 0263,
	JSR    = 0264,
	JSP    = 0265,
	JSA    = 0266,
	JRA    = 0267,

	BLKI   = 0700000,
	DATAI  = 0700040,
	BLKO   = 0700100,
	DATAO  = 0700140,
	CONO   = 0700200,
	CONI   = 0700240,
	CONSZ  = 0700300,
	CONSO  = 0700340

};

#define declpulse(p) \
	static void p##_p(Apr *apr); \
	static Pulse p = { (void (*)(void*))p##_p, #p }

#define defpulse_(p) \
	static void p##_p(Apr *apr)

#define defpulse(p) \
	declpulse(p); \
	defpulse_(p)

static void aprcycle(void *p);
static void wake_cpa(void *dev);
static void wake_pi(void *dev);

char *apr_ident = APR_IDENT;

/*
 * Pulse system
 */

static void
pfree(Apr *apr, TPulse *p)
{
	p->next = apr->pfree;
	apr->pfree = p;
}

/* Add a pulse to the list.
 * NB: does *not* check if pulse is already queued. */
static void
pulse(Apr *apr, Pulse *p, int t)
{
	TPulse *tp, **pp;

	assert(apr->pfree);
	tp = apr->pfree;
	apr->pfree = tp->next;
	tp->p = p;
	tp->t = t;
	for(pp = &apr->pulse; *pp; pp = &(*pp)->next){
		if(tp->t < (*pp)->t){
			(*pp)->t -= tp->t;
			break;
		}
		tp->t -= (*pp)->t;
	}
	tp->next = *pp;
	*pp = tp;
}

/* Remove pulse from list. Only first. */
static void
rempulse(Apr *apr, Pulse *p)
{
	TPulse *tp, **pp;

	for(pp = &apr->pulse; *pp; pp = &(*pp)->next)
		if((*pp)->p == p){
			tp = *pp;
			*pp = tp->next;
			pfree(apr, tp);
			break;
		}
}

static void
pprint(Apr *apr)
{
	int t;
	TPulse *p;

	t = 0;
	for(p = apr->pulse; p; p = p->next){
		t += p->t;
		trace("%s %d %d\n", p->p->n, p->t, t);
	}
}

static void
tracechange(Apr *apr)
{
	static char buf[256], *p;

	p = buf;
	if(apr->c.ar != apr->n.ar)
		p += sprintf(p, " AR/%012lo", apr->n.ar);
	if(apr->c.mb != apr->n.mb)
		p += sprintf(p, " MB/%012lo", apr->n.mb);
	if(apr->c.mq != apr->n.mq)
		p += sprintf(p, " MQ/%012lo", apr->n.mq);
	if(p != buf)
		trace("%s\n", buf);
}

static void
tracestate(Apr *apr)
{
	trace("AR/%012lo MQ/%012lo.%d MB/%012lo MA/%06o SC/%03o FE/%03o\n",
		apr->c.ar, apr->c.mq, apr->mq36, apr->c.mb,
		apr->ma, apr->sc, apr->fe);
}

static int
stepone(Apr *apr)
{
	int i;
	TPulse *p;

	p = apr->pulse;
	if(p == nil) return 0;
	apr->pulse = p->next;
	trace("pulse %s\n", p->p->n);
	p->p->f(apr);
	pfree(apr, p);

	i = 1;
	while(apr->pulse && apr->pulse->t == 0){
		p = apr->pulse;
		apr->pulse = p->next;
		trace("pulse %s\n", p->p->n);
		pfree(apr, p);
		p->p->f(apr);
		i++;
	}
//	tracechange(apr);
	apr->c = apr->n;
	tracestate(apr);
	return i;
}



// TODO This is WIP
// TODO: can't set switches with real panel connected

#define EXDEPREGS \
	X(ir)\
	X(mi)\
	X(data)\
	X(pc)\
	X(ma)\
	X(mas)
#define EXDEPSREGS \
	X(mb)\
	X(ar)\
	X(mq)

static word
ex_apr(Device *dev, const char *reg)
{
	Apr *apr = (Apr*)dev;
#define X(name) if(strcmp(#name, reg) == 0) return apr->name;
EXDEPREGS
#undef X
#define X(name) if(strcmp(#name, reg) == 0) return apr->c.name;
EXDEPSREGS
#undef X
	return ~0;
}
static int
dep_apr(Device *dev, const char *reg, word data)
{
	Apr *apr = (Apr*)dev;
#define X(name) if(strcmp(#name, reg) == 0) { apr->name = data; return 0; }
EXDEPREGS
#undef X
#define X(name) if(strcmp(#name, reg) == 0) { apr->n.name = data; return 0; }
EXDEPSREGS
#undef X
	return 1;
}

Device*
makeapr(int argc, char *argv[])
{
	Apr *apr;
	Thread th;

	apr = malloc(sizeof(Apr));
	memset(apr, 0, sizeof(Apr));

	apr->dev.type = apr_ident;
	apr->dev.name = "";
	apr->dev.examine = ex_apr;
	apr->dev.deposit = dep_apr;

	apr->iobus.dev[CPA] = (Busdev){ apr, wake_cpa, 0 };
	apr->iobus.dev[PI] = (Busdev){ apr, wake_pi, 0 };

	th = (Thread){ nil, aprcycle, apr, 1, 0 };
	addthread(th);

	return &apr->dev;
}



#define DBG_AR debug("AR: %012llo\n", apr->ar)
#define DBG_MB debug("MB: %012llo\n", apr->mb)
#define DBG_MQ debug("MQ: %012llo\n", apr->mq)
#define DBG_MA debug("MA: %06o\n", apr->ma)
#define DBG_IR debug("IR: %06o\n", apr->ir)

#define SWAP(a, b) apr->n.a = apr->c.b, apr->n.b = apr->c.a
#define SWAPLTRT(a) ((a)<<18 & LT | (a)>>18 & RT)
#define CONS(a, b) ((a)&LT | (b)&RT)

// 6-10
#define AR_OV_SET (apr->ar_cry0 != apr->ar_cry1)
#define AR0_XOR_AROV (!!(apr->c.ar & F0) != apr->ar_cry0_xor_cry1)
#define AR0_XOR_AR1 (!!(apr->c.ar & F0) != !!(apr->c.ar & F1))
#define AR0_XOR_MB0 (!!(apr->c.ar & F0) != !!(apr->c.mb & F0))
#define AR0_EQ_SC0 (!!(apr->c.ar & F0) == !!(apr->sc & 0400))
#define AR_EQ_FP_HALF ((apr->c.ar & 0377777777) == 0 && apr->c.ar & F9)
#define SET_OVERFLOW apr->ar_ov_flag = 1, recalc_cpa_req(apr)

// 6-13
#define MQ35_XOR_MB0 (!!(apr->c.mq & F35) != !!(apr->c.mb & F0))

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
#define FWT_NEGATE (FWT_MOVNM && ((apr->inst & 04) == 0 || apr->c.ar & SGN))

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
#define BLT_DONE (apr->pi_req || !(apr->c.mq & F0))
#define BLT_LAST (apr->inst == BLT && !(apr->c.mq & F0))

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
                       (apr->ir & H7) && apr->c.ar == 0)
#define ACCP_ET_AL_TEST (ACCP_ETC_COND != !!(apr->ir & H6))

// 5-9
#define HWT_LT (apr->ir_hwt && !(apr->inst & 040))
#define HWT_RT (apr->ir_hwt && apr->inst & 040)
#define HWT_SWAP (apr->ir_hwt && apr->inst & 04)
#define HWT_AR_0 (apr->ir_hwt && apr->inst & 030)
#define HWT_LT_SET (HWT_RT && apr->inst & 020 &&\
                    (!(apr->inst & 010) || apr->c.mb & RSGN))
#define HWT_RT_SET (HWT_LT && apr->inst & 020 &&\
                    (!(apr->inst & 010) || apr->c.mb & SGN))

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
	apr->iobus.c34 &= ~037777000000LL;
	apr->iobus.c34 |= apr->ir & H3 ? IOBUS_IOS3_1 : IOBUS_IOS3_0;
	apr->iobus.c34 |= apr->ir & H4 ? IOBUS_IOS4_1 : IOBUS_IOS4_0;
	apr->iobus.c34 |= apr->ir & H5 ? IOBUS_IOS5_1 : IOBUS_IOS5_0;
	apr->iobus.c34 |= apr->ir & H6 ? IOBUS_IOS6_1 : IOBUS_IOS6_0;
	apr->iobus.c34 |= apr->ir & H7 ? IOBUS_IOS7_1 : IOBUS_IOS7_0;
	apr->iobus.c34 |= apr->ir & H8 ? IOBUS_IOS8_1 : IOBUS_IOS8_0;
	apr->iobus.c34 |= apr->ir & H9 ? IOBUS_IOS9_1 : IOBUS_IOS9_0;

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

/*
 * AR control signals
 */

#define ARLT_CLEAR apr->n.ar &= RT
#define ARRT_CLEAR apr->n.ar &= LT
#define AR_CLEAR apr->n.ar = 0
#define ARLT_COM apr->n.ar ^= LT
#define ARRT_COM apr->n.ar ^= RT
#define AR_COM apr->n.ar ^= FW
#define ARLT_FM_MB_J apr->n.ar = CONS(apr->c.mb, apr->n.ar)
#define ARRT_FM_MB_J apr->n.ar = CONS(apr->n.ar, apr->c.mb)

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
	apr->ar_cry0_xor_cry1 = AR_OV_SET && !MEMAC;
}

void
ar_cry_in(Apr *apr, word c)
{
	word a;
	a = (apr->n.ar & ~F0) + c;
	apr->n.ar += c;
	if(apr->n.ar & FCRY) apr->ar_cry0 = 1;
	if(a         &   F0) apr->ar_cry1 = 1;
	apr->n.ar &= FW;
	ar_cry(apr);
}

/*
 * MB
 */

#define MBLT_CLEAR apr->n.mb &= RT
#define MB_CLEAR apr->n.mb = 0

/*
 * PI control signals and helpers
 */

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

#define PIH_CLEAR apr->pih = 0

/* clear highest held break */
void
pih0_fm_pi_ok1(Apr *apr)
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
pih_fm_pi_ch_rq(Apr *apr)
{
	// 8-3
	apr->pih |= apr->pi_req;
	apr->pir &= ~apr->pih;
	apr->pi_req = get_pi_req(apr);
}

#define PIR_CLEAR apr->pir = 0
#define PIR_FM_IOB1 set_pir(apr, apr->iobus.c12 & 0177)
#define PIR_STB set_pir(apr, apr->iobus.c34 & apr->pio)

void
set_pir(Apr *apr, int pir)
{
	// 8-3
	apr->pir |= pir;
	/* held breaks aren't retriggered */
	apr->pir &= ~apr->pih;
	apr->pi_req = get_pi_req(apr);
}

#define PIO_FM_IOB1 apr->pio |= apr->iobus.c12&0177
#define PIO0_FM_IOB1 apr->pio &= ~(apr->iobus.c12&0177)

/* Recalculate the PI requests on the IO bus from each device */
void
recalc_req(IOBus *bus)
{
	int i;
	int req;
	req = 0;
	for(i = 0; i < 128; i++)
		req |= bus->dev[i].req;
	bus->c34 = bus->c34&~0177LL | req&0177;
}

/* Decode pia into req and place it onto the bus */
void
setreq(IOBus *bus, int dev, u8 pia)
{
	u8 req;
	if(bus){
		req = (0200>>pia) & 0177;
		if(bus->dev[dev].req != req){
			bus->dev[dev].req = req;
			recalc_req(bus);
		}
	}
}

void
recalc_cpa_req(Apr *apr)
{
	u8 pia = 0;
	// 8-5
	if(apr->cpa_illeg_op || apr->cpa_non_exist_mem || apr->cpa_pdl_ov ||
	   apr->cpa_clock_flag && apr->cpa_clock_enable ||
	   apr->ar_pc_chg_flag && apr->cpa_pc_chg_enable ||
	   apr->ar_ov_flag && apr->cpa_arov_enable)
		pia = apr->cpa_pia;
	setreq(&apr->iobus, CPA, pia);
}

void
set_mc_rq(Apr *apr, bool value)
{
	apr->mc_rq = value;		// 7-9
	if(value && (apr->mc_rd || apr->mc_wr))
		apr->membus.c12 |= MEMBUS_RQ_CYC;
	else
		apr->membus.c12 &= ~MEMBUS_RQ_CYC;
}

void
set_mc_wr(Apr *apr, bool value)
{
	apr->mc_wr = value;		// 7-9
	if(value)
		apr->membus.c12 |= MEMBUS_WR_RQ;
	else
		apr->membus.c12 &= ~MEMBUS_WR_RQ;
	set_mc_rq(apr, apr->mc_rq);	// 7-9
}

void
set_mc_rd(Apr *apr, bool value)
{
	apr->mc_rd = value;		// 7-9
	if(value)
		apr->membus.c12 |= MEMBUS_RD_RQ;
	else
		apr->membus.c12 &= ~MEMBUS_RD_RQ;
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
	apr->membus.c12 &= ~0007777777761LL;
	apr->membus.c12 |= ma_fmc_select ? MEMBUS_MA_FMC_SEL1 : MEMBUS_MA_FMC_SEL0;
	apr->membus.c12 |= (apr->ma&01777) << 4;
	apr->membus.c12 |= ((word)apr->rla&017) << 14;
	apr->membus.c12 |= apr->rla & 0020 ? MEMBUS_MA21_1|MEMBUS_MA21 : MEMBUS_MA21_0;
	apr->membus.c12 |= apr->rla & 0040 ? MEMBUS_MA20_1 : MEMBUS_MA20_0;
	apr->membus.c12 |= apr->rla & 0100 ? MEMBUS_MA19_1 : MEMBUS_MA19_0;
	apr->membus.c12 |= apr->rla & 0200 ? MEMBUS_MA18_1 : MEMBUS_MA18_0;
	apr->membus.c12 |= apr->ma & 01 ? MEMBUS_MA35_1 : MEMBUS_MA35_0;
	return ma_ok;
}

// 6-17, 6-9
#define cfac_ar_add ar_ast1
#define cfac_ar_sub ar_ast0
#define cfac_ar_negate ar_negate_t0

declpulse(kt1);
declpulse(kt4);
declpulse(key_rd_wr_ret);
declpulse(mc_rs_t0);
declpulse(mc_addr_ack);
declpulse(mc_wr_rs);
declpulse(mc_rd_rq_pulse);
declpulse(mc_split_rd_rq);
declpulse(mc_wr_rq_pulse);
declpulse(mc_rdwr_rq_pulse);
declpulse(mc_rd_wr_rs_pulse);
declpulse(it0);
declpulse(iat0);
declpulse(it1);
declpulse(it1a);
declpulse(at0);
declpulse(at1);
declpulse(at3a);
declpulse(ft0);
declpulse(ft1a);
declpulse(et4);
declpulse(et5);
declpulse(et9);
declpulse(et10);
declpulse(st7);
declpulse(ar_negate_t0);
declpulse(ar_pm1_t1);
declpulse(ar_ast0);
declpulse(ar_ast1);
declpulse(sht1a);
declpulse(cht3);
declpulse(cht3a);
declpulse(cht8a);
declpulse(lct0a);
declpulse(dst14);
declpulse(dct0a);
declpulse(mst2);
declpulse(mpt0a);
declpulse(nrt2);
declpulse(fst0a);
declpulse(fmt0a);
declpulse(fmt0b);
declpulse(fdt0a);
declpulse(fdt0b);
declpulse(fpt1a);
declpulse(fpt1b);
declpulse(nrt0_5);
declpulse(fat1a);
declpulse(fat5a);
declpulse(fat10);

// TODO: find A LONG, it probably doesn't exist

defpulse(pi_reset)
{
	apr->pi_active = 0;	// 8-4
	PIH_CLEAR;		// 8-4
	PIR_CLEAR;		// 8-4
	apr->pi_req = get_pi_req(apr);
	apr->pio = 0;		// 8-3
}

static void
ar_flag_clr(Apr *apr)
{
	apr->ar_ov_flag = 0;		// 6-10
	apr->ar_cry0_flag = 0;		// 6-10
	apr->ar_cry1_flag = 0;		// 6-10
	apr->ar_pc_chg_flag = 0;	// 6-10
	apr->chf7 = 0;			// 6-19
	recalc_cpa_req(apr);
}

static void
ar_flag_set(Apr *apr)
{
	// 6-10
	if(apr->c.mb & F0) apr->ar_ov_flag = 1;
	if(apr->c.mb & F1) apr->ar_cry0_flag = 1;
	if(apr->c.mb & F2) apr->ar_cry1_flag = 1;
	if(apr->c.mb & F3) apr->ar_pc_chg_flag = 1;
	if(apr->c.mb & F4) apr->chf7 = 1;		// 6-19
	if(apr->c.mb & F5) apr->ex_mode_sync = 1;	// 5-13
#ifdef FIX_USER_IOT
	if(!apr->ex_user) apr->cpa_iot_user = !!(apr->c.mb & F6);
#endif
	recalc_cpa_req(apr);
}

static void
mp_clr(Apr *apr)
{
	// 6-19
	apr->chf1 = 0;
	apr->chf2 = 0;
	apr->chf3 = 0;
	apr->chf4 = 0;
	apr->chf5 = 0;
	apr->chf6 = 0;
	// 6-20
	apr->lcf1 = 0;
	apr->shf1 = 0;
	// 6-21
	apr->mpf1 = 0;
	apr->mpf2 = 0;
	apr->msf1 = 0;		// 6-24
	// 6-22
	apr->fmf1 = 0;
	apr->fmf2 = 0;
	apr->fdf1 = 0;
	apr->fdf2 = 0;
	apr->faf1 = 0;
	apr->faf2 = 0;
	apr->faf3 = 0;
	apr->faf4 = 0;
	// 6-23
	apr->fpf1 = 0;
	apr->fpf2 = 0;
	// 6-27
	apr->nrf1 = 0;
	apr->nrf2 = 0;
	apr->nrf3 = 0;
}

static void
ds_clr(Apr *apr)
{
	// 6-25, 6-26
	apr->dsf1 = 0;
	apr->dsf2 = 0;
	apr->dsf3 = 0;
	apr->dsf4 = 0;
	apr->dsf5 = 0;
	apr->dsf6 = 0;
	apr->dsf8 = 0;
	apr->dsf9 = 0;
	apr->fsf1 = 0;	// 6-19
}

defpulse(mr_clr)
{
	apr->ir = 0;	// 5-7
	apr->n.mq = 0;	// 6-13
	apr->mq36 = 0;	// 6-13
	apr->sc = 0;	// 6-15
	apr->fe = 0;	// 6-15

	apr->mc_rd = 0;		// 7-9
	apr->mc_wr = 0;		// 7-9
	apr->mc_rq = 0;		// 7-9
	apr->mc_stop = 0;	// 7-9
	apr->mc_stop_sync = 0;	// 7-9
	apr->mc_split_cyc_sync = 0;	// 7-9

	if(apr->ex_mode_sync)
		apr->ex_user = 1;	// 5-13
	apr->ex_mode_sync = 0;
	apr->ex_uuo_sync = 0;	// 5-13
	apr->ex_pi_sync = apr->pi_cyc;	// 5-13

	apr->a_long = 0;	// ?? nowhere to be found
	apr->ar_com_cont = 0;	// 6-9
	mp_clr(apr);		// 6-21
	ds_clr(apr);		// 6-26

	apr->iot_go = 0;	// 8-1
	apr->iot_init_setup = 0;
	apr->iot_final_setup = 0;
	apr->iot_reset = 0;
	apr->iot_go_pulse = 0;

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
	apr->dsf7 = 0;		// 6-25
	apr->iot_f0a = 0;	// 8-1
	apr->blt_f0a = 0;	// 6-18
	apr->blt_f3a = 0;	// 6-18
	apr->blt_f5a = 0;	// 6-18
	apr->uuo_f1 = 0;	// 5-10
	apr->dcf1 = 0;		// 6-20

	// EX UUO SYNC
	decodeir(apr);
}

defpulse(ex_clr)
{
	apr->pr = 0;		// 7-4
	apr->rlr = 0;		// 7-5
}

static void
ex_set(Apr *apr)
{
	apr->pr  = apr->iobus.c12>>28 & 0377;	// 7-4
	apr->rlr = apr->iobus.c12>>10 & 0377;	// 7-5
}

defpulse(mr_start)
{
	// 8-1
	apr->iobus.c34 |= IOBUS_IOB_RESET;

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
	apr->iobus.dev[CPA].req = 0;

	// PI
	apr->pi_ov = 0;		// 8-4
	set_pi_cyc(apr, 0);	// 8-4
	pulse(apr, &pi_reset, 0);	// 8-4
	ar_flag_clr(apr);	// 6-10

	pulse(apr, &ex_clr, 0);
	apr->ex_user = 0;	// 5-13
	apr->ex_ill_op = 0;	// 5-13
	apr->rla = 0;
}

defpulse(mr_pwr_clr)
{
	apr->run = 0;	// 5-1
	/* order seems to matter.
	 * better call directly before external pulses can trigger stuff */
	mr_start_p(apr);	// 5-2
	mr_clr_p(apr);	// 5-2
}

/* CPA and PI devices */

static void
wake_cpa(void *dev)
{
	Apr *apr;
	IOBus *bus;

	apr = dev;
	bus = &apr->iobus;
	if(apr->iobus.devcode != CPA)
		return;
	// 8-5
	if(IOB_STATUS){
		if(apr->cpa_pdl_ov)        bus->c12 |= F19;
		if(apr->cpa_iot_user)      bus->c12 |= F20;
		if(apr->ex_user)           bus->c12 |= F21;
		if(apr->cpa_illeg_op)      bus->c12 |= F22;
		if(apr->cpa_non_exist_mem) bus->c12 |= F23;
		if(apr->cpa_clock_enable)  bus->c12 |= F25;
		if(apr->cpa_clock_flag)    bus->c12 |= F26;
		if(apr->cpa_pc_chg_enable) bus->c12 |= F28;
		if(apr->ar_pc_chg_flag)    bus->c12 |= F29;
		if(apr->cpa_arov_enable)   bus->c12 |= F31;
		if(apr->ar_ov_flag)        bus->c12 |= F32;
		bus->c12 |= apr->cpa_pia & 7;
	}
	if(IOB_CONO_SET){
		if(bus->c12 & F18) apr->cpa_pdl_ov = 0;
		if(bus->c12 & F19) bus->c34 |= IOBUS_IOB_RESET;	// 8-1
#ifdef FIX_USER_IOT
		// IO reset seems to reset this too?
		if(bus->c12 & F19) apr->cpa_iot_user = 0;
		// these bits seem to be flipped in the schematics
		if(bus->c12 & F20) apr->cpa_iot_user = 0;
		if(bus->c12 & F21) apr->cpa_iot_user = 1;
#else
		if(bus->c12 & F20) apr->cpa_iot_user = 1;
		if(bus->c12 & F21) apr->cpa_iot_user = 0;
#endif
		if(bus->c12 & F22) apr->cpa_illeg_op = 0;
		if(bus->c12 & F23) apr->cpa_non_exist_mem = 0;
		if(bus->c12 & F24) apr->cpa_clock_enable = 0;
		if(bus->c12 & F25) apr->cpa_clock_enable = 1;
		if(bus->c12 & F26) apr->cpa_clock_flag = 0;
		if(bus->c12 & F27) apr->cpa_pc_chg_enable = 0;
		if(bus->c12 & F28) apr->cpa_pc_chg_enable = 1;
		if(bus->c12 & F29) apr->ar_pc_chg_flag = 0;	// 6-10
		if(bus->c12 & F30) apr->cpa_arov_enable = 0;
		if(bus->c12 & F31) apr->cpa_arov_enable = 1;
		if(bus->c12 & F32) apr->ar_ov_flag = 0;		// 6-10
		apr->cpa_pia = bus->c12 & 7;
		recalc_cpa_req(apr);
	}

	// 5-2
	if(IOB_DATAI)
		apr->n.ar = apr->data;
	// 5-13
	if(IOB_DATAO_CLEAR)
		ex_clr_p(apr);
	if(IOB_DATAO_SET)
		ex_set(apr);
}

static void
wake_pi(void *dev)
{
	Apr *apr;
	IOBus *bus;

	apr = dev;
	bus = &apr->iobus;
	if(bus->devcode != PI)
		return;
	// 8-4, 8-5
	if(IOB_STATUS){
		trace("PI STATUS %llo\n", bus->c12);
		if(apr->pi_active) bus->c12 |= F28;
		bus->c12 |= apr->pio;
	}

	// 8-4, 8-3
	if(IOB_CONO_CLEAR){
		trace("PI CONO CLEAR %llo\n", bus->c12);
		if(bus->c12 & F23)
			// can call directly
			pi_reset_p(apr);
	}
	if(IOB_CONO_SET){
		trace("PI CONO SET %llo\n", bus->c12);
		if(bus->c12 & F24) PIR_FM_IOB1;
		if(bus->c12 & F25) PIO_FM_IOB1;
		if(bus->c12 & F26) PIO0_FM_IOB1;
		if(bus->c12 & F27) apr->pi_active = 0;
		if(bus->c12 & F28) apr->pi_active = 1;
	}
}

/*
 * IOT
 */

defpulse(iot_t4)
{
	apr->iot_reset = 0;	// 8-1
}

defpulse(iot_t3a)
{
	apr->iot_reset = 1;	// 8-1
	/* Clear what was set in IOT T2 */
	apr->iobus.c34 &= ~(IOBUS_IOB_STATUS | IOBUS_IOB_DATAI);
	/* and do something like IOB BUS RESET */
	apr->iobus.c12 = 0;
	pulse(apr, &iot_t4, 2000);
}

defpulse(iot_t3)
{
	// 8-1
	/* Pulses, cleared in the main loop. */
	if(IOT_DATAO)	apr->iobus.c34 |= IOBUS_DATAO_SET;
	if(IOT_CONO)	apr->iobus.c34 |= IOBUS_CONO_SET;
	apr->n.ar |= apr->iobus.c12;	// 6-8
	pulse(apr, &et5, 200);		// 5-5
}

defpulse(iot_t2)
{
	// 8-1
	apr->iot_go = 0;
	apr->iot_init_setup = 0;
	apr->iot_final_setup = 1;
	/* Pulses, cleared in the main loop. */
	if(IOT_DATAO)	apr->iobus.c34 |= IOBUS_DATAO_CLEAR;
	if(IOT_CONO)	apr->iobus.c34 |= IOBUS_CONO_CLEAR;
	pulse(apr, &iot_t3, 1000);
	pulse(apr, &iot_t3a, 1500);
}

defpulse(iot_t0a)
{
	apr->iot_f0a = 0;			// 8-1
	apr->ir |= H12;				// 5-8
	decodeir(apr);
	apr->ma = 0;				// 7-3
	if(apr->pi_cyc && apr->ar_cry0)
		apr->pi_ov = 1;			// 8-4
	if(!apr->pi_cyc && !apr->ar_cry0)
		apr->pc = apr->pc+1 & RT;	// 5-12
	pulse(apr, &ft0, 200);			// 5-4
}

defpulse(iot_t0)
{
	apr->iot_f0a = 1;			// 8-1
	pulse(apr, &mc_rd_wr_rs_pulse, 0);	// 7-8
}

/*
 * UUO subroutine
 */

defpulse(uuo_t2)
{
	apr->inst_ma = apr->ma;		// remember where we got the instruction from
	apr->if1a = 1;			// 5-3
	pulse(apr, &mc_rd_rq_pulse, 0);	// 7-8
}

defpulse(uuo_t1)
{
	apr->uuo_f1 = 0;		// 5-10
	apr->ma = apr->ma+1 & RT;	// 7-3
	pulse(apr, &mr_clr, 0);		// 5-2
	pulse(apr, &uuo_t2, 100);	// 5-10
}

/*
 * BLT subroutine
 */

defpulse(blt_t6)
{
	SWAP(mb, ar);			// 6-3
	pulse(apr, &ft1a, 100);		// 5-4
}

defpulse(blt_t5a)
{
	apr->blt_f5a = 0;			// 6-18
	if(!(apr->c.mq & F0))
		apr->pc = apr->pc+1 & RT;	// 5-12
	pulse(apr, BLT_DONE ? &et10 : &blt_t6, 0);	// 5-5, 6-18
}

defpulse(blt_t5)
{
	SWAP(mb, mq);			// 6-17
	apr->blt_f5a = 1;		// 6-18
	pulse(apr, &ar_pm1_t1, 0);	// 6-9
}

defpulse(blt_t4)
{
	SWAP(mb, ar);			// 6-3
	PIR_STB;			// 8-4
	pulse(apr, &blt_t5, 100);	// 6-18
}

defpulse(blt_t3a)
{
	apr->blt_f3a = 0;		// 6-18
	SWAP(mb, mq);			// 6-17
	pulse(apr, &blt_t4, 100);	// 6-18
}

defpulse(blt_t3)
{
	apr->blt_f3a = 1;		// 6-18
	pulse(apr, &ar_ast0, 0);	// 6-9
}

defpulse(blt_t2)
{
	ARLT_CLEAR;			// 6-8
	pulse(apr, &blt_t3, 100);	// 6-18
}

defpulse(blt_t1)
{
	SWAP(mb, ar);			// 6-3
	pulse(apr, &blt_t2, 100);	// 6-18
}

defpulse(blt_t0a)
{
	apr->blt_f0a = 0;		// 6-18
	apr->n.mb = apr->c.mq;		// 6-3
	pulse(apr, &blt_t1, 100);	// 6-18
}

defpulse(blt_t0)
{
	SWAP(mb, mq);			// 6-17
	apr->blt_f0a = 1;		// 6-18
	pulse(apr, &mc_wr_rq_pulse, 0);	// 7-8
}

/*
 * Shift subroutines
 */

// 6-14
#define SC_COM apr->sc = ~apr->sc & 0777
#define SC_INC apr->sc = apr->sc+1 & 0777
#define SC_DATA (apr->chf1 ? ~apr->c.mb>>30 & 077 | 0700 : \
                 apr->chf2 ? apr->c.mb>>24 & 077 : \
                 apr->fsf1 || apr->fpf1 || apr->faf2 ? apr->c.ar>>27 & 0777 : \
                 apr->fpf2 || apr->faf1 ? apr->c.mb>>27 & 0777 : 0)
#define SC_PAD apr->sc ^= SC_DATA
#define SC_CRY apr->sc += (~apr->sc & SC_DATA) << 1
// 6-7
#define SHC_ASHC (apr->inst == ASHC || apr->nrf2 || apr->faf3)
#define SHC_DIV ((IR_DIV || IR_FDV) && !apr->nrf2)

#define MS_MULT (apr->mpf1 || apr->fmf2) 	// 6-24

/* Shift counter */

// 6-7, 6-17, 6-13
#define AR_SH_LT apr->n.ar = apr->c.ar<<1 & 0377777777776 | ar0_shl_inp | ar35_shl_inp
#define MQ_SH_LT apr->n.mq = apr->c.mq<<1 & 0377777777776 | mq0_shl_inp | mq35_shl_inp
#define AR_SH_RT apr->n.ar = apr->c.ar>>1 & 0377777777777 | ar0_shr_inp
#define MQ_SH_RT apr->mq36 = apr->c.mq&F35, \
                 apr->n.mq = apr->c.mq>>1 & 0177777777777 | mq0_shr_inp | mq1_shr_inp

defpulse(sct2)
{
	if(apr->shf1) pulse(apr, &sht1a, 0);	// 6-20
	if(apr->chf4) pulse(apr, &cht8a, 0);	// 6-19
	if(apr->lcf1) pulse(apr, &lct0a, 0);	// 6-20
	if(apr->dcf1) pulse(apr, &dct0a, 0);	// 6-20
	if(apr->faf3) pulse(apr, &fat5a, 0);	// 6-22
}

defpulse(sct1)
{
	word ar0_shl_inp, ar0_shr_inp, ar35_shl_inp;
	word mq0_shl_inp, mq0_shr_inp, mq1_shr_inp, mq35_shl_inp;

	SC_INC;		// 6-16

	// 6-7 What a mess, and many things aren't even used here
	if(apr->inst != ASH && !SHC_ASHC)
		ar0_shl_inp = (apr->c.ar & F1) << 1;
	else
		ar0_shl_inp = apr->c.ar & F0;

	if(apr->inst == ROTC)
		ar0_shr_inp = (apr->c.mq & F35) << 35;
	else if(apr->inst == ROT)
		ar0_shr_inp = (apr->c.ar & F35) << 35;
	else if(IR_DIV)
		ar0_shr_inp = (~apr->c.mq & F35) << 35;
	else if(apr->inst == LSH || apr->inst == LSHC || CH_LOAD)
		ar0_shr_inp = 0;
	else if(apr->inst == ASH || SHC_ASHC || MS_MULT || IR_FDV)
		ar0_shr_inp = apr->c.ar & F0;

	if(apr->inst == ROT)
		ar35_shl_inp = (apr->c.ar & F0) >> 35;
	else if(apr->inst == ASHC)
		ar35_shl_inp = (apr->c.mq & F1) >> 34;
	else if(apr->inst == ROTC || apr->inst == LSHC || SHC_DIV)
		ar35_shl_inp = (apr->c.mq & F0) >> 35;
	else if(CH_DEP || apr->inst == LSH || apr->inst == ASH)
		ar35_shl_inp = 0;

	if(SHC_ASHC){
		mq0_shl_inp = apr->c.ar & F0;
		mq1_shr_inp = (apr->c.ar & F35) << 34;
	}else{
		mq0_shl_inp = (apr->c.mq & F1) << 1;
		mq1_shr_inp = (apr->c.mq & F0) >> 1;
	}

	if(MS_MULT && apr->sc == 0777 || SHC_ASHC)
		mq0_shr_inp = apr->c.ar & F0;
	else
		mq0_shr_inp = (apr->c.ar & F35) << 35;

	if(apr->inst == ROTC)
		mq35_shl_inp = (apr->c.ar & F0) >> 35;
	else if(SHC_DIV)
		mq35_shl_inp = (~apr->c.ar & F0) >> 35;
	else if(CH_N_INC_OP || CH_INC_OP)
		mq35_shl_inp = 1;
	else if(apr->inst == LSHC || SHC_ASHC || CH_DEP)
		mq35_shl_inp = 0;

	// 6-17
	if(apr->shift_op && !(apr->c.mb & F18)){
		AR_SH_LT;
		MQ_SH_LT;
	}
	if(apr->shift_op && (apr->c.mb & F18) || apr->faf3){
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

	if(!(apr->c.mb & F18) && (apr->inst == ASH || apr->inst == ASHC) && AR0_XOR_AR1){
		apr->ar_ov_flag = 1;			// 6-10
		recalc_cpa_req(apr);
	}
	pulse(apr, apr->sc == 0777 ? &sct2 : &sct1, 75);	// 6-15, 6-16
}

defpulse(sct0)
{
	pulse(apr, apr->sc == 0777 ? &sct2 : &sct1, 200);	// 6-15, 6-16
}

/* Shift adder */

defpulse(sat3)
{
	if(apr->chf2) pulse(apr, &cht3a, 0);	// 6-19
	if(apr->fsf1) pulse(apr, &fst0a, 0);	// 6-19
	if(apr->fpf1) pulse(apr, &fpt1a, 0);	// 6-23
	if(apr->fpf2) pulse(apr, &fpt1b, 0);	// 6-23
	if(apr->faf2) pulse(apr, &fat1a, 0);	// 6-22
}

defpulse(sat2_1)
{
	SC_CRY;			// 6-15
	pulse(apr, &sat3, 100);	// 6-16
}

defpulse(sat2)
{
	pulse(apr, &sat2_1, 50);	// 6-16
}

defpulse(sat1)
{
	SC_PAD;			// 6-15
	pulse(apr, &sat2, 200);	// 6-16
}

defpulse(sat0)
{
	apr->chf1 = 0;		// 6-19
	pulse(apr, &sat1, 150);	// 6-16
}

/*
 * Shift operations subroutine
 */

defpulse_(sht1a)
{
	apr->shf1 = 0;			// 6-20
	pulse(apr, &et10, 0);		// 5-5
}

defpulse(sht1)
{
	if(apr->c.mb & F18)
		SC_COM;			// 6-15
	apr->shf1 = 1;			// 6-20
	pulse(apr, &sct0, 0);		// 6-16
}

defpulse(sht0)
{
	SC_INC;				// 6-16
}

/*
 * Character subroutines
 */

defpulse(dct3)
{
	apr->n.mb &= apr->c.ar;		// 6-3
	apr->chf7 = 0;			// 6-19
	pulse(apr, &et10, 0);		// 5-5
}

defpulse(dct2)
{
	AR_COM;				// 6-17
	pulse(apr, &dct3, 100);		// 6-20
}

defpulse(dct1)
{
	apr->n.ar &= apr->c.mb;		// 6-8
	apr->n.mb = apr->c.mq;		// 6-17
	pulse(apr, &dct2, 100);		// 6-20
}

defpulse_(dct0a)
{
	apr->dcf1 = 0;			// 6-20
	apr->n.mq |= apr->c.mb;		// 6-13 dct0b
	apr->n.mb = apr->c.mq;		// 6-17
	AR_COM;				// 6-17
	pulse(apr, &dct1, 150);		// 6-20
}

defpulse(dct0)
{
	apr->dcf1 = 1;			// 6-20
	SC_COM;				// 6-15
	pulse(apr, &sct0, 0);		// 6-16
}

defpulse_(lct0a)
{
	apr->lcf1 = 0;			// 6-20
	apr->n.ar &= apr->c.mb;		// 6-8
	apr->chf7 = 0;			// 6-19
	pulse(apr, &et10, 0);		// 5-5
}

defpulse(lct0)
{
	apr->n.ar = apr->c.mb;		// 6-8
	apr->n.mb = apr->c.mq;		// 6-17
	SC_COM;				// 6-15
	apr->lcf1 = 1;			// 6-20
	pulse(apr, &sct0, 0);		// 6-16
}

defpulse(cht9)
{
	apr->sc = apr->fe;		// 6-15
	apr->chf5 = 1;			// 6-19
	apr->chf7 = 1;			// 6-19
	pulse(apr, &at0, 0);		// 5-3
}

defpulse_(cht8a)
{
	apr->chf4 = 0;			// 6-19
	apr->sc = 0;			// 6-15
	apr->ir &= ~037;		// 5-7
	pulse(apr, &cht9, 100);		// 6-19
}

defpulse(cht8b)
{
	apr->chf2 = 0;			// 6-19
	apr->chf6 = 0;			// 6-19
	apr->fe = apr->c.mb>>30 & 077;	// 6-14, 6-15
	SC_COM;				// 6-15
	if(apr->inst == CAO)
		pulse(apr, &st7, 0);	// 5-6
	else{
		apr->chf4 = 1;		// 6-19
		pulse(apr, &sct0, 0);	// 6-16
	}
}

defpulse(cht8)
{
	apr->chf6 = 1;		// 6-19
	pulse(apr, &mc_rd_wr_rs_pulse, 0);	// 7-8
}

defpulse(cht7)
{
	SC_PAD;				// 6-15
	if(CH_INC_OP){
		SWAP(mb, ar);			// 6-17
		pulse(apr, &cht8, 100);		// 6-19
	}
	if(CH_N_INC_OP)
		pulse(apr, &cht8b, 100);	// 6-19
}

defpulse(cht6)
{
	apr->n.ar = apr->n.ar & 0007777777777 |
		(((word)apr->sc & 077) << 30);	// 6-9, 6-4
	if(CH_INC_OP)
		apr->sc = 0;		// 6-15
	apr->chf2 = 1;			// 6-19
	pulse(apr, &cht7, 150);		// 6-19
}

defpulse(cht5)
{
	SC_COM;				// 6-15
	pulse(apr, &cht6, 100);		// 6-19
}

defpulse(cht4a)
{
	apr->chf3 = 0;			// 6-19
	apr->sc |= 0433;
	pulse(apr, &cht3, 0);		// 6-19
}

defpulse(cht4)
{
	apr->sc = 0;			// 6-15
	apr->chf3 = 1;			// 6-19
	pulse(apr, &ar_pm1_t1, 0);	// 6-9
}

defpulse_(cht3a)
{
	apr->chf2 = 0;			// 6-19
	pulse(apr, apr->sc & 0400 ? &cht5 : &cht4, 0);	// 6-19
}

defpulse_(cht3)
{
	apr->chf2 = 1;			// 6-19
	pulse(apr, &sat0, 0);		// 6-16
}

defpulse(cht2)
{
	SC_PAD;				// 6-15
	pulse(apr, &cht3, 0);		// 6-19
}

defpulse(cht1)
{
	apr->n.ar = apr->c.mb;		// 6-8
	apr->chf1 = 1;			// 6-19
	pulse(apr, &cht2, 100);		// 6-19
}

/*
 * Multiply subroutine
 */

// 6-13
#define MQ35_EQ_MQ36 ((apr->c.mq&F35) == apr->mq36)

defpulse(mst6)
{
	if(apr->mpf1) pulse(apr, &mpt0a, 0);	// 6-21
	if(apr->fmf2) pulse(apr, &fmt0b, 0);	// 6-22
}

defpulse(mst5)
{
	word mq0_shr_inp, mq1_shr_inp;

	mq0_shr_inp = apr->c.ar & F0;	// 6-17
	mq1_shr_inp = (apr->c.mq & F0) >> 1;	// 6-17
	MQ_SH_RT;			// 6-17
	apr->sc = 0;			// 6-15
	pulse(apr, &mst6, 100);		// 6-24
}

defpulse(mst4)
{
	apr->msf1 = 1;			// 6-24
	pulse(apr, &cfac_ar_sub, 0);	// 6-17
}

defpulse(mst3a)
{
	apr->msf1 = 0;			// 6-24
	pulse(apr, apr->sc == 0777 ? &mst5 : &mst2, 0);	// 6-24
}

defpulse(mst3)
{
	apr->msf1 = 1;			// 6-24
	pulse(apr, &cfac_ar_add, 0);	// 6-17
}

defpulse_(mst2)
{
	word ar0_shr_inp, mq0_shr_inp, mq1_shr_inp;

	ar0_shr_inp = apr->c.ar & F0;		// 6-7
	mq0_shr_inp = (apr->c.ar & F35) << 35;	// 6-7
	mq1_shr_inp = (apr->c.mq & F0) >> 1;	// 6-7
	AR_SH_RT;	// 6-17
	MQ_SH_RT;	// 6-17
	SC_INC;		// 6-16
	if(MQ35_EQ_MQ36)
		pulse(apr, apr->sc == 0777 ? &mst5 : &mst2, 150);	// 6-24
	if(!(apr->c.mq&F35) && apr->mq36)
		pulse(apr, &mst3, 150);	// 6-24
	if(apr->c.mq&F35 && !apr->mq36)
		pulse(apr, &mst4, 150);	// 6-24
}

defpulse(mst1)
{
	apr->n.mq = apr->c.mb;		// 6-13
	apr->n.mb = apr->c.ar;		// 6-3
	AR_CLEAR;			// 6-8
	if(MQ35_EQ_MQ36 && apr->sc != 0777)
		pulse(apr, &mst2, 200);	// 6-24
	if(!(apr->c.mq&F35) && apr->mq36)
		pulse(apr, &mst3, 200);	// 6-24
	if(apr->c.mq&F35 && !apr->mq36)
		pulse(apr, &mst4, 200);	// 6-24
}

/*
 * Divide subroutine
 */

#define DS_DIV (IR_DIV & apr->ir & H6)
#define DS_DIVI (IR_DIV & !(apr->ir & H6))

#define DSF7_XOR_MQ0 (apr->dsf7 != !!(apr->c.mq & F0))

defpulse(dst21a)
{
	apr->dsf9 = 0;			// 6-26
	SWAP(mb, mq);			// 6-17
	if(IR_DIV)
		pulse(apr, &et9, 0);	// 5-5
	if(apr->fdf2) pulse(apr, &fdt0b, 0);	// 6-22
}

defpulse(dst21)
{
	apr->dsf9 = 1;			// 6-26
	pulse(apr, &cfac_ar_negate, 0);	// 6-17
}

defpulse(dst20)
{
	apr->sc = 0;			// 6-15
	SWAP(mb, ar);			// 6-17
	pulse(apr, DSF7_XOR_MQ0 ? &dst21 : &dst21a, 100);	// 6-26
}

defpulse(dst19a)
{
	apr->dsf8 = 0;			// 6-26
	SWAP(mb, mq);			// 6-17
	pulse(apr, &dst20, 100);	// 6-26
}

defpulse(dst19)
{
	apr->dsf8 = 1;			// 6-26 DST19B
	pulse(apr, &cfac_ar_negate, 0);	// 6-17
}

defpulse(dst18)
{
	apr->dsf6 = 1;			// 6-26
	pulse(apr, &cfac_ar_sub, 0);	// 6-17
}

defpulse(dst17a)
{
	apr->dsf6 = 0;		// 6-26
	pulse(apr, apr->dsf7 ? &dst19 : &dst19a, 0);	// 6-26
}

defpulse(dst17)
{
	apr->dsf6 = 1;		// 6-26
	pulse(apr, &cfac_ar_add, 0);	// 6-17
}

defpulse(dst16)
{
	word ar0_shr_inp;

	// 6-7
	if(IR_FDV)
		ar0_shr_inp = apr->c.ar & F0;
	if(IR_DIV)
		ar0_shr_inp = (~apr->c.mq & F35) << 35;
	AR_SH_RT;	// 6-17
	if(apr->c.ar & F0)
		pulse(apr, apr->c.mb & F0 ? &dst18 : &dst17, 100);	// 6-26
	else
		pulse(apr, &dst17a, 0);				// 6-26
}

defpulse(dst15)
{
	apr->dsf5 = 1;			// 6-26
	pulse(apr, &cfac_ar_add, 0);	// 6-17
}

defpulse(dst14b_dly)
{
	if(apr->sc == 0777)
		pulse(apr, &dst16, 0);	// 6-26
	else
		pulse(apr, MQ35_XOR_MB0 ? &dst14 : &dst15, 0);	// 6-26
}

defpulse(dst14b)
{
	// TODO: remove this perhaps?
	pulse(apr, &dst14b_dly, 100);	// 6-26
}

defpulse(dst14a)
{
	word ar0_shl_inp, ar35_shl_inp;
	word mq0_shl_inp, mq35_shl_inp;

	apr->dsf5 = 0;		// 6-26
	SC_INC;			// 6-16
	// 6-7
	ar0_shl_inp = (apr->c.ar & F1) << 1;
	ar35_shl_inp = (apr->c.mq & F0) >> 35;
	mq0_shl_inp = (apr->c.mq & F1) << 1;
	mq35_shl_inp = (~apr->c.ar & F0) >> 35;
	AR_SH_LT;	// 6-17
	MQ_SH_LT;	// 6-17
	pulse(apr, &dst14b, 0);	// 6-26
}

defpulse_(dst14)
{
	apr->dsf5 = 1;			// 6-26
	pulse(apr, &cfac_ar_sub, 0);	// 6-17
}

defpulse(dst13)
{
	SET_OVERFLOW;		// 6-17
	pulse(apr, &st7, 0);	// 5-6
}

defpulse(dst12)
{
	apr->dsf4 = 1;			// 6-25
	pulse(apr, &cfac_ar_sub, 0);	// 6-17
}

defpulse(dst11a)
{
	apr->dsf4 = 0;		// 6-25
	pulse(apr, apr->c.ar & F0 ? &dst14a : &dst13, 0);	// 6-25, 6-26
}

defpulse(dst11)
{
	apr->dsf4 = 1;			// 6-25
	pulse(apr, &cfac_ar_add, 0);	// 6-17
}

defpulse(dst10b)
{
	word mq0_shl_inp, mq35_shl_inp;

	mq0_shl_inp = (apr->c.mq & F1) << 1;	// 6-7
	mq35_shl_inp = (~apr->c.ar & F0) >> 35;	// 6-7
	MQ_SH_LT;				// 6-17
	pulse(apr, apr->c.mb & F0 ? &dst11 : &dst12, 200);	// 6-15
}

defpulse(dst10a)
{
	word ar0_shr_inp;

	apr->n.mq = apr->c.mq & ~F0 | ((apr->c.ar & F35) << 35);	// 6-13
	ar0_shr_inp = apr->c.ar & F0;	// 6-7
	AR_SH_RT;			// 6-17
	pulse(apr, apr->c.mb & F0 ? &dst11 : &dst12, 200);	// 6-15
}

defpulse(dst10)
{
	apr->dsf3 = 0;			// 6-25
	if(IR_FDV)
		pulse(apr, &dst10a, 100);	// 6-25
	if(IR_DIV)
		pulse(apr, &dst10b, 100);	// 6-25
}

defpulse(dst9)
{
	SWAP(mb, mq);			// 6-17
	apr->dsf3 = 1;			// 6-25
	pulse(apr, &cfac_ar_negate, 0);	// 6-17
}

defpulse(dst8)
{
	SWAP(mb, ar);			// 6-17
	pulse(apr, &dst9, 100);		// 6-25
}

defpulse(dst7)
{
	SWAP(mb, mq);			// 6-17
	AR_COM;				// 6-17
	pulse(apr, &dst10, 0);		// 6-25
}

defpulse(dst6)
{
	SWAP(mb, ar);			// 6-17
	pulse(apr, &dst7, 100);		// 6-25
}

defpulse(dst5a)
{
	apr->dsf2 = 0;				// 6-25
#ifdef FIX_DS
	/* we have to ignore the lower sign bit
	 * for carry propagation */
	pulse(apr, apr->c.ar & 0377777777777 ?
		&dst6 : &dst8, 100);	// 6-25
#else
	pulse(apr, apr->c.ar ? &dst6 : &dst8, 100);	// 6-25
#endif
}

defpulse(dst5)
{
	apr->dsf2 = 1;			// 6-25
	pulse(apr, &cfac_ar_negate, 0);	// 6-17
}

defpulse(dst4)
{
	SWAP(mb, ar);			// 6-17
	pulse(apr, &dst5, 100);		// 6-25
}

defpulse(dst3)
{
	SWAP(mb, mq);			// 6-17
	apr->dsf7 = 1;			// 6-25
	pulse(apr, &dst4, 100);		// 6-25
}

defpulse(dst2)
{
	SWAP(mb, mq);			// 6-17
	AR_CLEAR;			// 6-8
	pulse(apr, &dst10, 0);		// 6-25
}

defpulse(dst1)
{
	apr->n.mq = apr->c.mb;		// 6-13
	apr->n.mb = apr->c.ar;		// 6-3
	pulse(apr, &dst2, 150);		// 6-25
}

defpulse(dst0a)
{
	apr->dsf1 = 0;				// 6-25
	pulse(apr, DS_DIVI ? &dst1 : &dst10, 0);	// 6-25
}

defpulse(dst0)
{
	apr->dsf7 = 1;			// 6-25
	apr->dsf1 = 1;			// 6-25
	pulse(apr, &cfac_ar_negate, 0);	// 6-17
}

defpulse(ds_div_t0)
{
	apr->sc = 0733;			// 6-14
}

/*
 * Floating point subroutines
 */

/* Normalize return */

// 6-27
#define AR_0_AND_MQ1_0 (apr->c.ar == 0 && !(apr->c.mq & F1))
#define AR9_EQ_AR0 (!!(apr->c.ar & F9) == !!(apr->c.ar & F0))
#define NR_ROUND (apr->ir & H6 && apr->c.mq & F1 && !apr->nrf3)

defpulse(nrt6)
{
	pulse(apr, &et10, 0);	// 5-5
}

defpulse(nrt5a)
{
	apr->nrf1 = 0;		// 6-27
	apr->nrf3 = 1;		// 6-27
	pulse(apr, &nrt0_5, 0);	// 6-27
}

defpulse(nrt5)
{
	apr->nrf1 = 1;			// 6-27
	pulse(apr, &ar_pm1_t1, 0);	// 6-9
}

defpulse(nrt4)
{
	apr->n.ar |= apr->n.ar&0400777777777 | ((word)apr->sc&0377)<<27;	 // 6-4, 6-9
	pulse(apr, &nrt6, 0);		// 6-27
}

defpulse(nrt31_dly)
{
	pulse(apr, NR_ROUND ? &nrt5 : &nrt4, 0);	// 6-27
}

defpulse(nrt3)
{
	if(!(apr->sc & 0400))
		SET_OVERFLOW;		// 6-17
	if(!(apr->c.ar & F0) || NR_ROUND)
		SC_COM;			// 6-15
	pulse(apr, &nrt31_dly, 100);	// 6-27
}

defpulse(nrt1_dly)
{
	pulse(apr, AR_EQ_FP_HALF || !AR9_EQ_AR0 ? &nrt3 : &nrt2, 0);	// 6-27
}

defpulse_(nrt2)
{
	word ar0_shl_inp, ar35_shl_inp;
	word mq0_shl_inp, mq35_shl_inp;

	SC_INC;		// 6-16
	// 6-7
	ar0_shl_inp = apr->c.ar & F0;
	mq0_shl_inp = apr->c.ar & F0;
	ar35_shl_inp = (apr->c.mq & F1) >> 34;
	mq35_shl_inp = 0;
	AR_SH_LT;	// 6-17
	MQ_SH_LT;	// 6-17
	pulse(apr, &nrt1_dly, 150);	// 6-17
}

defpulse(nrt1)
{
	SC_COM;			// 6-15
	pulse(apr, &nrt1_dly, 100);	// 6-17
}

defpulse(nrt01_dly)
{
	pulse(apr, AR_0_AND_MQ1_0 ? &nrt6 : &nrt1, 0);	// 6-27
}

defpulse(nrt0)
{
	word ar0_shr_inp;
	word mq0_shr_inp, mq1_shr_inp;

	SC_INC;			// 6-16
	// 6-7
	ar0_shr_inp = apr->c.ar & F0;
	mq0_shr_inp = apr->c.ar & F0;
	mq1_shr_inp = (apr->c.ar & F35) << 34;
	AR_SH_RT;	// 6-17
	MQ_SH_RT;	// 6-17
	pulse(apr, &nrt01_dly, 200);	// 6-27
}

defpulse_(nrt0_5)
{
	apr->nrf2 = 1;		// 6-27
	pulse(apr, &nrt0, 100);	// 6-27
}

/* Scale */

defpulse(fst1)
{
	SC_INC;		// 6-16
}

defpulse_(fst0a)
{
	apr->fsf1 = 0;	// 6-19
	if(!AR0_EQ_SC0)
		SET_OVERFLOW;	// 6-17
	apr->n.ar |= apr->n.ar&0400777777777 | ((word)apr->sc&0377)<<27;	 // 6-4, 6-9
	pulse(apr, &et10, 0);	// 5-5
}

defpulse(fst0)
{
	apr->fsf1 = 1;		// 6-19
	pulse(apr, &sat0, 0);	// 6-16
}

/* Exponent calculate */

// 6-23
#define AR0_XOR_FMF1 (!!(apr->c.ar & F0) != !!apr->fmf1)
#define AR0_XOR_MB0_XOR_FMF1 (AR0_XOR_FMF1 != !!(apr->c.mb & F0))
#define MB0_EQ_FMF1 (!!(apr->c.mb & F0) == !!apr->fmf1)

defpulse(fpt4)
{
	// 6-22
	if(apr->fmf1) pulse(apr, &fmt0a, 0);
	if(apr->fdf1) pulse(apr, &fdt0a, 0);
}

defpulse(fpt3)
{
	apr->fe |= apr->sc;	// 6-15
	apr->sc = 0;		// 6-15
	// 6-3, 6-4
	if(apr->c.mb & F0) apr->n.mb |= 0377000000000LL;
	else               apr->n.mb &= ~0377000000000LL;
	// 6-9, 6-4
	if(apr->c.ar & F0) apr->n.ar |= 0377000000000LL;
	else               apr->n.ar &= ~0377000000000LL;
	pulse(apr, &fpt4, 100);	// 6-23
}

defpulse(fpt2)
{
	SC_INC;			// 6-17
}

defpulse_(fpt1b)
{
	apr->fpf2 = 0;		// 6-23
	if(MB0_EQ_FMF1)
		SC_COM;		// 6-15
	pulse(apr, &fpt3, 100);	// 6-23
}

defpulse(fpt1aa)
{
	apr->fpf2 = 1;		// 6-23
	pulse(apr, &sat0, 0);	// 6-15
}

defpulse_(fpt1a)
{
	apr->fpf1 = 0;		// 6-23
	if(AR0_XOR_MB0_XOR_FMF1)
		pulse(apr, &fpt2, 0);	// 6-23
	else
		SC_COM;		// 6-15
	pulse(apr, &fpt1aa, 100);	// 6-23
}

defpulse(fpt1)
{
	apr->fpf1 = 1;		// 6-23
	if(AR0_XOR_FMF1)
		SC_COM;		// 6-15
	pulse(apr, &sat0, 0);	// 6-16
}

defpulse(fpt0)
{
	apr->sc |= 0200;	// 6-14
	pulse(apr, &fpt1, 100);	// 6-23
}

/* Multiply */

defpulse_(fmt0b)
{
	apr->fmf2 = 0;		// 6-22
	apr->sc |= apr->fe;	// 6-15
	apr->nrf2 = 1;		// 6-27
	pulse(apr, &nrt01_dly, 100);	// 6-27
}

defpulse_(fmt0a)
{
	apr->fmf1 = 0;		// 6-22
	apr->fmf2 = 1;		// 6-22
	apr->sc |= 0744;	// 6-14
	pulse(apr, &mst1, 0);	// 6-24
}

defpulse(fmt0)
{
	apr->fmf1 = 1;		// 6-22
	pulse(apr, &fpt0, 0);	// 6-23
}

/* Divide */

defpulse(fdt1)
{
	word ar0_shr_inp;
	word mq0_shr_inp, mq1_shr_inp;

	// 6-7
	ar0_shr_inp = apr->c.ar & F0;
	mq0_shr_inp = apr->c.ar & F0;
	mq1_shr_inp = (apr->c.ar & F35) << 34;
	AR_SH_RT;	// 6-17
	MQ_SH_RT;	// 6-17
	pulse(apr, &nrt0_5, 150);	// 6-27
}

defpulse_(fdt0b)
{
	apr->fdf2 = 0;		// 6-22
	apr->sc = apr->fe;	// 6-15
	apr->nrf2 = 1;		// 6-27
	pulse(apr, &fdt1, 100);	// 6-22
}

defpulse_(fdt0a)
{
	apr->fdf1 = 0;		// 6-22
	apr->fdf2 = 1;		// 6-22
	apr->sc = 0741;		// 6-14
	pulse(apr, apr->c.ar & F0 ? &dst0 : &dst10, 0);	// 6-25
}

defpulse(fdt0)
{
	apr->fdf1 = 1;		// 6-22
	pulse(apr, &fpt0, 0);	// 6-23
}

/* Add/Subtract */

defpulse_(fat10)
{
	apr->faf4 = 0;		// 6-22
	apr->faf1 = 0;		// 6-22
	pulse(apr, &nrt0_5, 0);	// 6-27
}

defpulse(fat9)
{
	apr->faf4 = 1;		// 6-22
	pulse(apr, &cfac_ar_add, 0);	// 6-17
}

defpulse(fat8a)
{
	// 6-3, 6-4
	if(apr->c.mb & F0) apr->n.mb |= 0377000000000;
	else               apr->n.mb &= ~0377000000000;
	pulse(apr, &fat9, 100);	// 6-22
}

defpulse(fat8)
{
	SC_PAD;			// 6-15
	pulse(apr, &fat8a, 50);	// 6-22
}

defpulse(fat7)
{
	if(apr->c.mb & F0)
		SC_COM;		// 6-15
	pulse(apr, &fat8, 100);	// 6-22
}

defpulse(fat6)
{
	AR_CLEAR;		// 6-8
	pulse(apr, &fat5a, 0);	// 6-22
}

defpulse_(fat5a)
{
	apr->faf3 = 0;		// 6-22
	apr->sc = 0;		// 6-15
	apr->faf1 = 1;		// 6-22
	pulse(apr, &fat7, 100);	// 6-22
}

defpulse(fat5)
{
	// 6-9, 6-4
	if(apr->c.ar & F0) apr->n.ar |= 0377000000000;
	else               apr->n.ar &= ~0377000000000;
	apr->faf3 = 1;		// 6-22
	pulse(apr, &sct0, 0);	// 6-16
}

defpulse(fat4)
{
	pulse(apr, (apr->sc & 0700) == 0700 ? &fat5 : &fat6, 100);	// 6-22
}

defpulse(fat3)
{
	SC_COM;			// 6-15
	pulse(apr, (apr->sc & 0700) == 0700 ? &fat5 : &fat6, 150);	// 6-22
}

defpulse(fat2)
{
	SC_INC;			// 6-16
	pulse(apr, &fat3, 150);	// 6-22
}

defpulse(fat1b)
{
	apr->faf1 = 0;		// 6-22
	apr->faf2 = 1;		// 6-22
}

defpulse_(fat1a)
{
	apr->faf2 = 0;		// 6-22
	if(!!(apr->c.ar & F0) == !!(apr->sc & 0400))
		SWAP(mb, ar);	// 6-17
	pulse(apr, apr->sc & 0400 ? &fat4 : &fat2, 0);	// 6-22
}

defpulse(fat1)
{
	SC_PAD;			// 6-15
	pulse(apr, &fat1b, 50);	// 6-22
	pulse(apr, &sat0, 0);	// 6-16
}

defpulse(fat0)
{
	if(!AR0_XOR_MB0)
		SC_COM;		// 6-15
	apr->faf1 = 1;		// 6-22
	pulse(apr, &fat1, 100);	// 6-22
}

/*
 * Fixed point multiply
 */

defpulse(mpt2)
{
	apr->n.ar = apr->c.mb;		// 6-8
	pulse(apr, &nrt6, 100);		// 6-27
}

defpulse(mpt1)
{
	apr->n.mb = apr->c.mq;		// 6-17
	if(apr->c.ar != 0)
		SET_OVERFLOW;		// 6-17
	pulse(apr, &mpt2, 100);		// 6-21
}

defpulse_(mpt0a)
{
	apr->mpf1 = 0;				// 6-21
	if(!(apr->ir & H6) && apr->c.ar & F0)
		AR_COM;				// 6-17
	if(apr->c.ar & F0 && apr->mpf2)
		SET_OVERFLOW;			// 6-17
	if(apr->ir & H6)
		pulse(apr, &nrt6, 0);	// 6-27
	else
		pulse(apr, &mpt1, 200);	// 6-21
}

defpulse(mpt0)
{
	apr->sc |= 0734;		// 6-14
	apr->mpf1 = 1;			// 6-21
	if(apr->c.ar & F0 && apr->c.mb & F0)
		apr->mpf2 = 1;		// 6-21
	pulse(apr, &mst1, 0);		// 6-24
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

defpulse(art3)
{
	apr->ar_com_cont = 0;		// 6-9

	if(apr->af3a) pulse(apr, &at3a, 0);		// 5-3
	if(apr->et4_ar_pse) pulse(apr, &et4, 0);	// 5-5
	if(apr->blt_f3a) pulse(apr, &blt_t3a, 0);	// 6-18
	if(apr->blt_f5a) pulse(apr, &blt_t5a, 0);	// 6-18
	if(apr->chf3) pulse(apr, &cht4a, 0);		// 6-19
	if(apr->msf1) pulse(apr, &mst3a, 0);		// 6-24
	if(apr->dsf1) pulse(apr, &dst0a, 0);		// 6-25
	if(apr->dsf2) pulse(apr, &dst5a, 0);		// 6-25
	if(apr->dsf3) pulse(apr, &dst10, 0);		// 6-25
	if(apr->dsf4) pulse(apr, &dst11a, 0);		// 6-25
	if(apr->dsf5) pulse(apr, &dst14a, 0);		// 6-26
	if(apr->dsf6) pulse(apr, &dst17a, 0);		// 6-26
	if(apr->dsf8) pulse(apr, &dst19a, 0);		// 6-26
	if(apr->dsf9) pulse(apr, &dst21a, 0);		// 6-26
	if(apr->nrf1) pulse(apr, &nrt5a, 0);		// 6-27
	if(apr->faf4) pulse(apr, &fat10, 0);		// 6-22
}

defpulse(ar_cry_comp)
{
	if(apr->ar_com_cont){
		AR_COM;				// 6-8
		pulse(apr, &art3, 100);		// 6-9
	}
}

defpulse_(ar_pm1_t1)
{
	if(apr->inst == BLT || AR_PM1_LTRT)
		ar_cry_in(apr, 01000001);	// 6-9
	else
		ar_cry_in(apr, 1);		// 6-6
	// There must be some delay after the carry is done
	// but I don't quite know how this works,
	// so we just use 50ns a an imaginary value
	if(!apr->ar_com_cont)
		pulse(apr, &art3, 50);		// 6-9
	pulse(apr, &ar_cry_comp, 50);		// 6-9
}

defpulse(ar_pm1_t0)
{
	AR_COM;				// 6-8
	apr->ar_com_cont = 1;		// 6-9
	pulse(apr, &ar_pm1_t1, 100);	// 6-9
}

defpulse_(ar_negate_t0)
{
	AR_COM;				// 6-8
	pulse(apr, &ar_pm1_t1, 100);	// 6-9
}

defpulse(ar_ast2)
{
	ar_cry_in(apr, (~apr->c.ar & apr->c.mb) << 1);	// 6-8
	// see comment in ar_pm1_t1
	if(!apr->ar_com_cont)
		pulse(apr, &art3, 50);			// 6-9
	pulse(apr, &ar_cry_comp, 50);			// 6-9
}

defpulse_(ar_ast1)
{
	apr->n.ar ^= apr->c.mb;		// 6-8
	pulse(apr, &ar_ast2, 100);	// 6-9
}

defpulse_(ar_ast0)
{
	AR_COM;				// 6-8
	apr->ar_com_cont = 1;		// 6-9
	pulse(apr, &ar_ast1, 100);	// 6-9
}


defpulse(xct_t0)
{
	pulse(apr, &mr_clr, 0);		// 5-2
	pulse(apr, &it1a, 200);		// 5-3
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

defpulse(pi_sync)
{
	if(!apr->pi_cyc)
		PIR_STB;			// 8-4
	if(apr->pi_req && !apr->pi_cyc)
		pulse(apr, &iat0, 200);		// 5-3
	if(IA_NOT_INT)
		pulse(apr, apr->if1a ? &it1 : &at1, 200);	// 5-3
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

defpulse_(st7)
{
	apr->sf7 = 0;		// 5-6
	if(apr->run)
		pulse(apr, apr->key_ex_st || apr->key_dep_st ? &kt1 : &it0, 0);	// 5-1, 5-3
	if(apr->key_start || apr->key_readin || apr->key_inst_cont)
		pulse(apr, &kt4, 0);	// 5-2
}

defpulse(st6a)
{
	apr->sf7 = 1;		// 5-6
}

defpulse(st6)
{
	/* We know SAC2 is asserted
	 * so clamp to fast memory addresses */
	apr->ma = apr->ma+1 & 017;	// 7-3
	apr->n.mb = apr->c.mq;		// 6-3
	pulse(apr, &st6a, 50);		// 5-6
	pulse(apr, &mc_wr_rq_pulse, 0);	// 7-8
}

defpulse(st5a)
{
	apr->sf5a = 0;				// 5-6
	pulse(apr, SAC2 ? &st6 : &st7, 0);	// 5-6
}

defpulse(st5)
{
	apr->sf5a = 1;			// 5-6
	apr->ma |= apr->ir>>5 & 017;	// 7-3
	apr->n.mb = apr->c.ar;		// 6-3
	pulse(apr, &mc_wr_rq_pulse, 0);	// 7-8
}

defpulse(st3a)
{
	pulse(apr, &st5, 100);
}

defpulse(st3)
{
	apr->sf3 = 0;			// 5-6
	if(SAC_INH)
		pulse(apr, &st7, 0);	// 5-6
	else{
		apr->ma = 0;		// 7-3
		pulse(apr, &st3a, 0);	// 5-6
	}
}

defpulse(st2)
{
	apr->sf3 = 1;		// 5-6
	pulse(apr, &mc_rd_wr_rs_pulse, 0);	// 7-8
}

defpulse(st1)
{
	apr->sf3 = 1;		// 5-6
	pulse(apr, &mc_wr_rq_pulse, 0);	// 7-8
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
	               apr->inst == AOBJN && apr->c.ar & SGN || \
	               apr->inst == AOBJP && !(apr->c.ar & SGN) || \
	               apr->inst == JFCL && JFCL_FLAGS)
#define PC_INC_ENABLE ((MEMAC_MEM || ACCP || apr->ir_acbm) && ACCP_ET_AL_TEST || \
	               IOT_CONSO && apr->c.ar != 0 || IOT_CONSZ && apr->c.ar == 0)
#define PC_SET (PC_SET_ENABLE || JP_JMP || apr->ir_jrst)
#define PC_INC_ET9 (apr->inst == JSR || apr->inst == JSA || PC_INC_ENABLE)
// 5-5
#define E_LONG (IOT_CONSZ || apr->ir_jp || apr->ir_acbm || apr->pc_set || \
                MB_PC_STO || PC_INC_ET9 || IOT_CONSO || IR_ACCP_MEMAC)

defpulse_(et10)
{
	bool sc_e = SC_E;

	if(PI_HOLD){
		// 8-4
		apr->pi_ov = 0;
		apr->pi_cyc = 0;
	}
	if(apr->inst == PUSH || apr->inst == PUSHJ || apr->ir_jrst)
		apr->ma |= apr->c.mb & RT;		// 7-3
	if((apr->fc_e_pse || sc_e) &&
	   !(apr->ir_jp || apr->inst == EXCH || CH_DEP))
		apr->n.mb = apr->c.ar;			// 6-3
	if(apr->ir_jp && !(apr->ir & H6))
		SWAP(mb, ar);				// 6-3
	if(MEMAC || apr->ir_as){
		// 6-10
		if(apr->ar_cry0) apr->ar_cry0_flag = 1;
		if(apr->ar_cry1) apr->ar_cry1_flag = 1;
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
	pulse(apr, sc_e          ? &st1 :		// 5-6
                   apr->fc_e_pse ? &st2 :		// 5-6
                                   &st3, 0);		// 5-6
}

defpulse_(et9)
{
	bool pc_inc;

	pc_inc = PC_INC_ET9;
	if(pc_inc)
		apr->pc = apr->pc+1 & RT;	// 5-12
	if((apr->pc_set || pc_inc) && !(apr->ir_jrst && apr->ir & H11)){
		apr->ar_pc_chg_flag = 1;	// 6-10
		recalc_cpa_req(apr);
	}

	if(apr->ir_acbm || apr->ir_jp && apr->inst != JSR)
		SWAP(mb, ar);			// 6-3
	if(apr->inst == PUSH || apr->inst == PUSHJ || apr->ir_jrst)
		apr->ma = 0;			// 7-3
	if(apr->inst == JSR)
		apr->chf7 = 0;			// 6-19
	pulse(apr, &et10, 200);			// 5-5
}

defpulse(et8)
{
	if(apr->pc_set)
		apr->pc |= apr->ma;	// 5-12
	if(apr->inst == JSR)
		apr->ex_ill_op = 0;	// 5-13
	pulse(apr, &et9, 200);		// 5-5
}

defpulse(et7)
{
	if(apr->pc_set)
		apr->pc = 0;		// 5-12
	if(apr->inst == JSR && (apr->ex_pi_sync || apr->ex_ill_op))
		apr->ex_user = 0;	// 5-13
	if(apr->ir_acbm)
		AR_COM;				// 6-8
	pulse(apr, &et8, 100);			// 5-5
}

defpulse(et6)
{
	if(MB_PC_STO || apr->inst == JSA)
		apr->n.mb |= apr->pc;	// 6-3
	if(ACBM_CL)
		apr->n.mb &= apr->c.ar;	// 6-3
	if(JP_FLAG_STOR){		// 6-4
		// 6-4
		if(apr->ar_ov_flag)     apr->n.mb |= F0;
		if(apr->ar_cry0_flag)   apr->n.mb |= F1;
		if(apr->ar_cry1_flag)   apr->n.mb |= F2;
		if(apr->ar_pc_chg_flag) apr->n.mb |= F3;
		if(apr->chf7)           apr->n.mb |= F4;
		if(apr->ex_user)        apr->n.mb |= F5;
#ifdef FIX_USER_IOT
		if(apr->cpa_iot_user)   apr->n.mb |= F6;
#endif
	}
	if(IOT_CONSZ || IOT_CONSO)
		apr->n.ar &= apr->c.mb;	// 6-8
	pulse(apr, &et7, 100);		// 5-5
}

defpulse_(et5)
{
	if(MB_PC_STO)
		MB_CLEAR;			// 6-3
	if(apr->ir_acbm)
		AR_COM;				// 6-8
	apr->pc_set = PC_SET;
	if(E_LONG)				// 5-5
		pulse(apr, &et6, 100);
	else
		pulse(apr, &et10, 0);
}

defpulse_(et4)
{
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
		AR_COM;
	if(HWT_LT || IOT_CONO)
		ARLT_FM_MB_J;
	if(HWT_RT)
		ARRT_FM_MB_J;
	if(HWT_LT_SET)
		ARLT_COM;
	if(HWT_RT_SET)
		ARRT_COM;

	if(FWT_SWAP || IOT_BLK || apr->ir_acbm)
		SWAP(mb, ar);			// 6-3

	if(IOT_BLK)
		pulse(apr, &iot_t0, 0);		// 8-1
	else if(apr->ir_iot)
		apr->iot_go = 1;		// 8-1
	if(IR_FSB)
		pulse(apr, &fat0, 0);		// 6-22
	if(!ET5_INH)
		pulse(apr, &et5, 100);		// 5-5
}

defpulse(et3)
{
	if(apr->ex_ir_uuo){
		// MBLT <- IR(1) (UUO T0) on 6-3
		apr->n.mb |= ((word)apr->ir&0777740) << 18;	// 6-1
		apr->ma |= F30;				// 7-3
		apr->uuo_f1 = 1;			// 5-10
		pulse(apr, &mc_wr_rq_pulse, 0);		// 7-8
	}

	if(apr->inst == POPJ || apr->inst == BLT)
		apr->ma = apr->c.mb & RT;		// 7-3

	// AR SBR, 6-9
	if(FWT_NEGATE || IR_FSB)
		pulse(apr, &ar_negate_t0, 0);
	if(AR_SUB)
		pulse(apr, &ar_ast0, 0);
	if(AR_ADD)
		pulse(apr, &ar_ast1, 0);
	if(AR_M1)
		pulse(apr, &ar_pm1_t0, 0);
	if(AR_P1)
		pulse(apr, &ar_pm1_t1, 0);

	if(IR_FPCH && !(apr->ir & H3) &&
	   (apr->inst == 0130 || apr->inst == 0131 || !(apr->ir & H4) || !(apr->ir & H5)))
		pulse(apr, &st7, 0);		// 5-6
	if(apr->inst == BLT)
		pulse(apr, &blt_t0, 0);		// 6-18
	if(apr->shift_op)
		pulse(apr, &sht1, 100);		// 6-20
	if(apr->inst == FSC){
		if(apr->c.ar & F0)
			pulse(apr, &fst1, 0);	// 6-19
		pulse(apr, &fst0, 0);		// 6-19
	}
	if(apr->inst == XCT)
		pulse(apr, &xct_t0, 0);		// 5-10
	if(AR_SBR)
		apr->et4_ar_pse = 1;		// 5-5
	if(!ET4_INH)
		pulse(apr, &et4, 0);		// 5-5
};

defpulse(et1)
{
	if(apr->ex_ir_uuo){
		apr->ex_ill_op = 1;		// 5-13
		MBLT_CLEAR;		// 6-3
	}
	if(apr->ir_jrst && apr->ir & H12)
		apr->ex_mode_sync = 1;		// 5-13
	if(apr->ir_jrst && apr->ir & H11)
		ar_flag_set(apr);		// 6-10
	if(PI_RST)
		pih0_fm_pi_ok1(apr);			// 8-4

	// 6-3
	if(apr->ir_acbm)
		apr->n.mb &= apr->c.ar;
	// 6-8
	if(apr->ir_boole && (apr->ir_boole_op == 06 ||
	                     apr->ir_boole_op == 011 ||
	                     apr->ir_boole_op == 014) ||
	   ACBM_COM)
		apr->n.ar ^= apr->c.mb;
	if(apr->ir_boole && (apr->ir_boole_op == 01 ||
	                     apr->ir_boole_op == 02 ||
	                     apr->ir_boole_op == 015 ||
	                     apr->ir_boole_op == 016))
		apr->n.ar &= apr->c.mb;
	if(apr->ir_boole && (apr->ir_boole_op == 03 ||
	                     apr->ir_boole_op == 04 ||
	                     apr->ir_boole_op == 07 ||
	                     apr->ir_boole_op == 010 ||
	                     apr->ir_boole_op == 013) ||
	   ACBM_SET)
		apr->n.ar |= apr->c.mb;
	if(HWT_AR_0 || IOT_STATUS || IOT_DATAI)
		AR_CLEAR;

	if(HWT_SWAP || FWT_SWAP || apr->inst == BLT)
		apr->n.mb = SWAPLTRT(apr->c.mb);

	if(apr->inst == POPJ || apr->ex_ir_uuo || apr->inst == BLT)
		apr->ma = 0;			// 7-3
	if(apr->shift_op && apr->c.mb & F18)
		pulse(apr, &sht0, 0);		// 6-20
	if(apr->inst == FSC && !(apr->c.ar & F0))
		SC_COM;				// 6-15
	pulse(apr, &et3, 100);
}

defpulse(et0)
{
	if(!PC_P1_INH)
		apr->pc = apr->pc+1 & RT;	// 5-12
	apr->ar_cry0 = 0;	// 6-10
	apr->ar_cry1 = 0;	// 6-10
	ar_cry(apr);
	if(apr->ir_jrst && apr->ir & H11)
		ar_flag_clr(apr);		// 6-10
	if(CH_INC_OP)
		pulse(apr, &cht1, 0);		// 6-19
	if(CH_N_INC_OP)
		pulse(apr, &cht6, 0);		// 6-19
	if(CH_LOAD)
		pulse(apr, &lct0, 0);		// 6-20
	if(CH_DEP)
		pulse(apr, &dct0, 0);		// 6-20
	if(IR_MUL)
		pulse(apr, &mpt0, 0);		// 6-21
	if(IR_FAD)
		pulse(apr, &fat0, 0);		// 6-22
	if(IR_FMP)
		pulse(apr, &fmt0, 0);		// 6-22
	if(IR_FDV)
		pulse(apr, &fdt0, 0);		// 6-22
	if(IR_DIV){
		pulse(apr, &ds_div_t0, 0);	// 6-25
		if(apr->ir & H6)	// DIV
			pulse(apr, apr->c.ar & F0 ? &dst3 : &dst10, 0);	// 6-25
		else			// IDIV
			pulse(apr, apr->c.ar & F0 ? &dst0 : &dst1, 0);	// 6-25
	}
}

defpulse(et0a)
{
	debug("%06o: ", apr->inst_ma);
	if((apr->inst & 0700) != 0700)
		debug("%s\n", mnemonics[apr->inst]);
	else
		debug("%s\n", iomnemonics[apr->io_inst>>5 & 7]);

	if(PI_HOLD)
		pih_fm_pi_ch_rq(apr);	// 8-3, 8-4
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
		AR_CLEAR;			// 6-8
	if(apr->ir_boole && (apr->ir_boole_op == 02 ||
	                     apr->ir_boole_op == 04 ||
	                     apr->ir_boole_op == 012 ||
	                     apr->ir_boole_op == 013 ||
	                     apr->ir_boole_op == 015))
		AR_COM;				// 6-8
	if(apr->fwt_00 || apr->fwt_11 || apr->hwt_11 || MEMAC_MEM ||
	   IOT_BLK || IOT_DATAO)
		apr->n.ar = apr->c.mb;		// 6-8
	if(apr->fwt_01 || apr->fwt_10 || IOT_STATUS)
		apr->n.mb = apr->c.ar;		// 6-3
	if(apr->hwt_10 || apr->inst == JSP || apr->inst == EXCH ||
	   apr->inst == BLT || IR_FSB)
		SWAP(mb, ar);			// 6-3
	if(apr->inst == POP || apr->inst == POPJ || apr->inst == JRA)
		apr->n.mb = apr->c.mq;		// 6-3
	if(ACBM_SWAP || IOT_CONO || apr->inst == JSA)
		apr->n.mb = SWAPLTRT(apr->c.mb);		// 6-3
	if(apr->inst == FSC || apr->shift_op)
		apr->sc |= ~apr->c.mb & 0377 | ~apr->c.mb>>9 & 0400;	// 6-15
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

defpulse(ft7)
{
	apr->f6a = 1;			// 5-4
	pulse(apr, apr->mc_split_cyc_sync ? &mc_split_rd_rq :	// 7-8
	                                    &mc_rdwr_rq_pulse, 0);	// 7-8
}

defpulse(ft6a)
{
	apr->f6a = 0;			// 5-4
	// HACK: if we fetch nothing at all we get to ET0(A)
	// from AT4, where we clear ARLT, without simulated delay.
	// In the real machines the values must have settled by then,
	// so introduce a fake delay to update our state.
	pulse(apr, &et0a, 1);		// 5-5
	pulse(apr, &et0, 1);		// 5-5
	pulse(apr, &et1, 100);		// 5-5
}

defpulse(ft6)
{
	apr->f6a = 1;			// 5-4
	pulse(apr, &mc_rd_rq_pulse, 0);	// 7-8
}

defpulse(ft5)
{
	// cache this because we need it again in ET10
	apr->fc_e_pse = FC_E_PSE;
	apr->ma = apr->c.mb & RT;		// 7-3
	pulse(apr, FC_E          ? &ft6 :	// 5-4
                   apr->fc_e_pse ? &ft7 :	// 5-4
                                   &ft6a, 0);	// 5-4
}

defpulse(ft4a)
{
	apr->f4a = 0;			// 5-4
	apr->ma = 0;			// 7-3
	SWAP(mb, mq);			// 6-3, 6-13
	pulse(apr, &ft5, 100);		// 5-4
}

defpulse(ft4)
{
	apr->n.mq = apr->c.mb;		// 6-13
	apr->f4a = 1;			// 5-4
	pulse(apr, &mc_rd_rq_pulse, 0);	// 7-8
}

defpulse(ft3)
{
	apr->ma = apr->c.mb & RT;	// 7-3
	SWAP(mb, ar);			// 6-3
	pulse(apr, &ft4, 100);		// 5-4
}

defpulse_(ft1a)
{
	bool acltrt = FC_C_ACLT || FC_C_ACRT;
	bool fac2 = FAC2;
	apr->f1a = 0;				// 5-4
	if(fac2)
		apr->ma = apr->ma+1 & 017;	// 7-1, 7-3
	else
		apr->ma = 0;			// 7-3
	if(!acltrt)
		SWAP(mb, ar);			// 6-3
	if(FC_C_ACLT)
		apr->n.mb = SWAPLTRT(apr->c.mb);	// 6-3
	pulse(apr, fac2   ? &ft4 :	// 5-4
                   acltrt ? &ft3 :	// 5-4
                            &ft5, 100);	// 5-4
}

defpulse(ft1)
{
	apr->ma |= apr->ir>>5 & 017;	// 7-3
	apr->f1a = 1;			// 5-4
	pulse(apr, &mc_rd_rq_pulse, 0);	// 7-8
}

defpulse_(ft0)
{
	pulse(apr, FAC_INH ? &ft5 : &ft1, 0);	// 5-4
}

/*
 * Address
 */

defpulse(at5)
{
	apr->a_long = 1;		// ?? nowhere to be found
	apr->af0 = 1;			// 5-3
	apr->ma |= apr->c.mb & RT;	// 7-3
	apr->ir &= ~037;		// 5-7
	pulse(apr, &mc_rd_rq_pulse, 0);	// 7-8
}

defpulse(at4)
{
	ARLT_CLEAR;		// 6-8
	// TODO: what is MC DR SPLIT? what happens here anyway?
	if(apr->sw_addr_stop || apr->key_mem_stop)
		apr->mc_split_cyc_sync = 1;	// 7-9
	pulse(apr, apr->ir & 020 ? &at5 : &ft0, 0);	// 5-3, 5-4
}

defpulse_(at3a)
{
	apr->af3a = 0;			// 5-3
	apr->n.mb = apr->c.ar;		// 6-3
	pulse(apr, &at4, 100);		// 5-3
}

defpulse(at3)
{
	apr->af3 = 0;			// 5-3
	apr->ma = 0;			// 7-3
	apr->af3a = 1;			// 5-3
	pulse(apr, &ar_ast1, 0);	// 6-9
}

defpulse(at2)
{
	apr->a_long = 1;		// ?? nowhere to be found
	apr->ma |= apr->ir & 017;	// 7-3
	apr->af3 = 1;			// 5-3
	pulse(apr, &mc_rd_rq_pulse, 0);	// 7-8
}

defpulse_(at1)
{
	apr->ex_uuo_sync = 1;		// 5-13
	// decode here because of EX UUO SYNC
	decodeir(apr);
	pulse(apr, (apr->ir & 017) == 0 ? &at4 : &at2, 0);	// 5-3
}

defpulse_(at0)
{
	ARRT_FM_MB_J;			// 6-8
	apr->ir |= apr->c.mb>>18 & 037;	// 5-7
	apr->ma = 0;			// 7-3
	apr->af0 = 0;			// 5-3
	pulse(apr, &pi_sync, 0);	// 8-4
}

/*
 * Instruction
 */

defpulse_(it1a)
{
	apr->if1a = 0;				// 5-3
	apr->ir |= apr->c.mb>>18 & 0777740;	// 5-7
	if(apr->ma & 0777760)
		set_key_rim_sbr(apr, 0);	// 5-2
	pulse(apr, &at0, 0);			// 5-3
}

defpulse_(it1)
{
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
	apr->inst_ma = apr->ma;			// remember where we got the instruction from
	apr->if1a = 1;				// 5-3
	pulse(apr, &mc_rd_rq_pulse, 0);		// 7-8
}

defpulse_(iat0)
{
	// have to call directly because PI CYC sets EX PI SYNC
	mr_clr_p(apr);			// 5-2
	set_pi_cyc(apr, 1);		// 8-4
	pulse(apr, &it1, 200);		// 5-3
}

defpulse_(it0)
{
	apr->ma = 0;			// 7-3
	// have to call directly because IF1A is set with a delay
	mr_clr_p(apr);			// 5-2
	apr->if1a = 1;			// 5-3
	pulse(apr, &pi_sync, 0);	// 8-4
}

/*
 * Memory Control
 */

defpulse(mi_fm_mb)
{
	// 7-7
	apr->mi |= apr->c.mb;
}

defpulse(mi_clr)
{
	// 7-7
	apr->mi = 0;
	pulse(apr, &mi_fm_mb, 100);
}

defpulse(mai_addr_ack)
{
	pulse(apr, &mc_addr_ack, 0);	// 7-8
}

defpulse(mai_rd_rs)
{
	if(apr->ma == apr->mas)
		pulse(apr, &mi_clr, 0);		// 7-7
	if(!apr->mc_stop)
		pulse(apr, &mc_rs_t0, 0);	// 7-8
}

defpulse(mc_rs_t1)
{
	set_mc_rd(apr, 0);			// 7-9
	if(apr->key_ex_nxt || apr->key_dep_nxt)
		pulse(apr, &mi_clr, 0);		// 7-7

	if(apr->key_rd_wr) pulse(apr, &key_rd_wr_ret, 0);	// 5-2
	if(apr->sf7) pulse(apr, &st7, 0);			// 5-6
	if(apr->sf5a) pulse(apr, &st5a, 0);			// 5-6
	if(apr->sf3) pulse(apr, &st3, 0);			// 5-6
	if(apr->f6a) pulse(apr, &ft6a, 0);			// 5-4
	if(apr->f4a) pulse(apr, &ft4a, 0);			// 5-4
	if(apr->f1a) pulse(apr, &ft1a, 0);			// 5-4
	if(apr->af0) pulse(apr, &at0, 0);			// 5-3
	if(apr->af3) pulse(apr, &at3, 0);			// 5-3
	if(apr->if1a) pulse(apr, &it1a, 0);			// 5-3
	if(apr->iot_f0a) pulse(apr, &iot_t0a, 0);		// 8-1
	if(apr->blt_f0a) pulse(apr, &blt_t0a, 0);		// 6-18
	if(apr->blt_f3a) pulse(apr, &blt_t3a, 0);		// 6-18
	if(apr->blt_f5a) pulse(apr, &blt_t5a, 0);		// 6-18
	if(apr->uuo_f1) pulse(apr, &uuo_t1, 0);			// 5-10
	if(apr->chf6) pulse(apr, &cht8b, 0);			// 6-19
}

defpulse_(mc_rs_t0)
{
//	apr->mc_stop = 0;		// ?? not found on 7-9
	pulse(apr, &mc_rs_t1, 50);	// 7-8
}

defpulse_(mc_wr_rs)
{
	if(apr->ma == apr->mas)
		pulse(apr, &mi_clr, 0);		// 7-7
	apr->membus.c34 |= apr->c.mb;		// 7-8
	apr->membus.c12 |= MEMBUS_WR_RS;	// 7-8
	if(!apr->mc_stop)
		pulse(apr, &mc_rs_t0, 0);	// 7-8
}

defpulse_(mc_addr_ack)
{
	set_mc_rq(apr, 0);			// 7-9
	if(!apr->mc_rd && apr->mc_wr)
		pulse(apr, &mc_wr_rs, 0);	// 7-8
}

defpulse(mc_non_exist_rd)
{
	if(!apr->mc_stop)
		pulse(apr, &mc_rs_t0, 0);		// 7-8
}

defpulse(mc_non_exist_mem_rst)
{
	pulse(apr, &mc_addr_ack, 0);			// 7-8
	if(apr->mc_rd)
		pulse(apr, &mc_non_exist_rd, 0);	// 7-9
}

defpulse(mc_non_exist_mem)
{
	apr->cpa_non_exist_mem = 1;			// 8-5
	recalc_cpa_req(apr);
	if(!apr->sw_mem_disable)
		pulse(apr, &mc_non_exist_mem_rst, 0);	// 7-9
}

defpulse(mc_error_dly)
{
	if(apr->mc_rq && !apr->mc_stop)
		pulse(apr, &mc_non_exist_mem, 0);	// 7-9
}

defpulse(mc_illeg_address)
{
	apr->cpa_illeg_op = 1;				// 8-5
	recalc_cpa_req(apr);
	pulse(apr, &st7, 100);				// 5-6
}

defpulse(mc_rq_1)
{
	set_mc_rq(apr, 1);	// 7-9
}

defpulse(mc_stop_1)
{
	// NOTE: it's important this pulse come *before* MC RS T0
	apr->mc_stop = 1;		// 7-9
	if(apr->key_mem_cont)
		pulse(apr, &kt4, 0);	// 5-2
}

defpulse(mc_rq_pulse)
{
	bool ma_ok;

	/* have to call this to set flags, do relocation and set address */
	ma_ok = relocate(apr);
	apr->mc_stop = 0;		// 7-9

	/* this pulse is likely to be still queued. so remove first it. */
	rempulse(apr, &mc_error_dly);
	pulse(apr, &mc_error_dly, 100000);	// 7-9

	if(ma_ok || apr->ex_inh_rel)
		pulse(apr, &mc_rq_1, 50);
	else
		pulse(apr, &mc_illeg_address, 150);	// 7-9
	if(apr->key_mem_stop || apr->ma == apr->mas && apr->sw_addr_stop)
		pulse(apr, &mc_stop_1, 200);	// 7-9
}

defpulse_(mc_rdwr_rq_pulse)
{
	set_mc_rd(apr, 1);		// 7-9
	set_mc_wr(apr, 1);		// 7-9
	MB_CLEAR;			// 7-8
	apr->mc_stop_sync = 1;		// 7-9
	pulse(apr, &mc_rq_pulse, 0);	// 7-8
}

defpulse_(mc_rd_rq_pulse)
{
	set_mc_rd(apr, 1);		// 7-9
	set_mc_wr(apr, 0);		// 7-9
	MB_CLEAR;			// 7-8
	pulse(apr, &mc_rq_pulse, 0);	// 7-8
}

defpulse_(mc_split_rd_rq)
{
	pulse(apr, &mc_rd_rq_pulse, 0);	// 7-8
}

defpulse_(mc_wr_rq_pulse)
{
	set_mc_rd(apr, 0);		// 7-9
	set_mc_wr(apr, 1);		// 7-9
	pulse(apr, &mc_rq_pulse, 0);	// 7-8
}

defpulse(mc_split_wr_rq)
{
	pulse(apr, &mc_wr_rq_pulse, 0);	// 7-8
}

defpulse_(mc_rd_wr_rs_pulse)
{
	if(apr->mc_split_cyc_sync)
		pulse(apr, &mc_split_wr_rq, 0);	// 7-8
	else
		pulse(apr, &mc_wr_rs, 100);	// 7-8
}

/*
 * Keys
 */

defpulse_(key_rd_wr_ret)
{
	apr->key_rd_wr = 0;	// 5-2
//	apr->ex_ill_op = 0;	// ?? not found on 5-13
	pulse(apr, &kt4, 0);	// 5-2
}

defpulse(key_rd)
{
	apr->key_rd_wr = 1;		// 5-2
	pulse(apr, &mc_rd_rq_pulse, 0);	// 7-8
}

defpulse(key_wr)
{
	apr->key_rd_wr = 1;		// 5-2
	apr->n.mb = apr->c.ar;		// 6-3
	pulse(apr, &mc_wr_rq_pulse, 0);	// 7-8
}

defpulse(key_go)
{
	apr->run = 1;		// 5-1
	apr->key_ex_st = 0;	// 5-1
	apr->key_dep_st = 0;	// 5-1
	apr->key_ex_sync = 0;	// 5-1
	apr->key_dep_sync = 0;	// 5-1
	pulse(apr, &it0, 0);	// 5-3
}

defpulse_(kt4)
{
	if(apr->run && (apr->key_ex_st || apr->key_dep_st))
		pulse(apr, &key_go, 0); 	// 5-2
	// TODO check repeat switch
}

defpulse(kt3)
{
	if(apr->key_readin || apr->key_start)
		apr->pc |= apr->ma;		// 5-12
	if(KEY_EXECUTE){
		apr->n.mb = apr->c.ar;		// 6-3
		pulse(apr, &it1a, 100);		// 5-3
		pulse(apr, &kt4, 0);		// 5-2
	}
	if(KEY_EX_EX_NXT)
		pulse(apr, &key_rd, 0);		// 5-2
	if(KEY_DP_DP_NXT)
		pulse(apr, &key_wr, 0);		// 5-2
	if(apr->key_start || apr->key_readin || apr->key_inst_cont)
		pulse(apr, &key_go, 0);		// 5-2
}

defpulse(kt2)
{
	if(KEY_MA_MAS)
		apr->ma |= apr->mas;		// 7-1
	if(KEY_EXECUTE_DP_DPNXT)
		apr->n.ar |= apr->data;		// 5-2
	pulse(apr, &kt3, 200);	// 5-2
}

defpulse_(kt1)
{
	if(apr->key_io_reset)
		pulse(apr, &mr_start, 0);	// 5-2
	if(KEY_MANUAL && !apr->key_mem_cont)
		pulse(apr, &mr_clr, 0);		// 5-2
	if(KEY_CLR_RIM)
		set_key_rim_sbr(apr, 0);	// 5-2
	if(apr->key_mem_cont && apr->mc_stop)
		pulse(apr, &mc_rs_t0, 0);	// 7-8
	if(KEY_MANUAL && apr->mc_stop && apr->mc_stop_sync && !apr->key_mem_cont)
		pulse(apr, &mc_wr_rs, 0);	// 7-8

	if(apr->key_readin)
		set_key_rim_sbr(apr, 1);	// 5-2
	if(apr->key_readin || apr->key_start)
		apr->pc = 0;			// 5-12
	if(KEY_MA_MAS)
		apr->ma = 0;			// 5-2
	if(apr->key_ex_nxt  || apr->key_dep_nxt)
		apr->ma = (apr->ma+1)&RT;	// 5-2
	if(KEY_EXECUTE_DP_DPNXT)
		AR_CLEAR;			// 5-2

	pulse(apr, &kt2, 200);	// 5-2
}

defpulse(kt0a)
{
	apr->key_ex_st = 0;	// 5-1
	apr->key_dep_st = 0;	// 5-1
	apr->key_ex_sync = apr->key_ex;		// 5-1
	apr->key_dep_sync = apr->key_dep;	// 5-1
	if(!apr->run || apr->key_mem_cont)
		// TODO: actually 0 or 100, but couldn't this
		// cause kt1 to be executed multiple times?
		pulse(apr, &kt1, 100);	// 5-2
}

defpulse(kt0)
{
	pulse(apr, &kt0a, 0);		// 5-2
}

defpulse(key_manual)
{
	pulse(apr, &kt0, 0);		// 5-2
}


/* find out which bits were turned on */
void
updatebus(void *bus)
{
	IOBus *b;
	b = bus;
	b->c12_pulse = (b->c12_prev ^ b->c12) & b->c12;
	b->c34_pulse = (b->c34_prev ^ b->c34) & b->c34;
}

#define TIMESTEP (1000.0/60.0)

void
aprstart(Apr *apr)
{
	int i;
	printf("[aprstart]\n");

	apr->pfree = nil;
	apr->pulse = nil;
	for(i = 0; i < MAXPULSE; i++)
		pfree(apr, &apr->pulses[i]);
	apr->ia_inh = 0;

	// must clear lest EX USER be set by MR CLR after MR START
	apr->ex_mode_sync = 0;

	apr->membus.c12 = 0;
	apr->membus.c34 = 0;
	apr->iobus.c12 = 0;
	apr->iobus.c34 = 0;

	pulse(apr, &mr_pwr_clr, 100);	// random value
	apr->lasttick = getms();
	apr->powered = 1;
}

static void
aprcycle(void *p)
{
	Apr *apr;
	Busdev *dev;
	int i, devcode;
	bool iot_go_pulse;

	apr = p;
	if(!apr->sw_power){
		apr->powered = 0;
		return;
	}else if(!apr->powered)
		aprstart(apr);

/*
	if(apr->pulsestepping){
		int c;
		while(c = getchar(), c != EOF && c != '\n')
			if(c == 'x')
				apr->pulsestepping = 0;
	}
*/

	apr->tick = getms();
	if(apr->tick-apr->lasttick >= TIMESTEP){
		apr->lasttick = apr->lasttick+TIMESTEP;
		apr->cpa_clock_flag = 1;
		recalc_cpa_req(apr);
	}

	apr->iobus.c12_prev = apr->iobus.c12;
	apr->iobus.c34_prev = apr->iobus.c34;
	apr->membus.c12_prev = apr->membus.c12;
	apr->membus.c34_prev = apr->membus.c34;

	i = stepone(apr);

	iot_go_pulse = apr->iot_go && !apr->iot_reset;
	if(iot_go_pulse && !apr->iot_go_pulse){
		apr->iot_init_setup = 1;
		/* These are asserted during INIT SETUP, IOT T2 and FINAL SETUP.
		 * We clear them in IOT T3A which happens after FINAL SETUP */
		if(IOT_OUTGOING)	apr->iobus.c12 |= apr->c.ar;
		if(IOT_STATUS)	apr->iobus.c34 |= IOBUS_IOB_STATUS;
		if(IOT_DATAI)	apr->iobus.c34 |= IOBUS_IOB_DATAI;
		pulse(apr, &iot_t2, 1000);
	}
	apr->iot_go_pulse = iot_go_pulse;

	updatebus(&apr->iobus);
	updatebus(&apr->membus);


	/* Key pulses */
	if(apr->extpulse & EXT_KEY_MANUAL){
		apr->extpulse &= ~EXT_KEY_MANUAL;
		pulse(apr, &key_manual, 1);
	}
	if(apr->extpulse & EXT_KEY_INST_STOP){
		apr->extpulse &= ~EXT_KEY_INST_STOP;
		apr->run = 0;
		// hack: cleared when the pulse list was empty
		apr->ia_inh = 1;
	}


	/* Pulses and signals through IO bus.
	 * NB: we interpret level signals as pulses here too. */
	apr->iobus.devcode = -1;
	if(apr->iobus.c34_pulse & (IOBUS_PULSES | IOBUS_IOB_STATUS | IOBUS_IOB_DATAI)){
		devcode = 0;
		if(apr->iobus.c34 & IOBUS_IOS3_1) devcode |= 0100;
		if(apr->iobus.c34 & IOBUS_IOS4_1) devcode |= 0040;
		if(apr->iobus.c34 & IOBUS_IOS5_1) devcode |= 0020;
		if(apr->iobus.c34 & IOBUS_IOS6_1) devcode |= 0010;
		if(apr->iobus.c34 & IOBUS_IOS7_1) devcode |= 0004;
		if(apr->iobus.c34 & IOBUS_IOS8_1) devcode |= 0002;
		if(apr->iobus.c34 & IOBUS_IOS9_1) devcode |= 0001;
		apr->iobus.devcode = devcode;
		dev = &apr->iobus.dev[devcode];
		if(dev->wake)
			dev->wake(dev->dev);
	}
	if(apr->iobus.c34_pulse & IOBUS_IOB_RESET){
		int d;
		for(d = 0; d < 128; d++){
			dev = &apr->iobus.dev[d];
			if(dev->wake)
				dev->wake(dev->dev);
		}
	}
	apr->iobus.c34 &= ~(IOBUS_PULSES | IOBUS_IOB_RESET);

	/* Pulses to memory */
	if(apr->membus.c12_pulse & (MEMBUS_WR_RS | MEMBUS_RQ_CYC)){
		wakemem(&apr->membus);
		apr->membus.c12 &= ~MEMBUS_WR_RS;
	}

	/* Pulses from memory  */
	if(apr->membus.c12 & MEMBUS_MAI_ADDR_ACK){
		apr->membus.c12 &= ~MEMBUS_MAI_ADDR_ACK;
		// TODO: figure out a better delay
		// NB: must be at least 200 because
		//     otherwise MC STOP won't be set in time
		pulse(apr, &mai_addr_ack, 200);
	}
	if(apr->membus.c12 & MEMBUS_MAI_RD_RS){
		apr->membus.c12 &= ~MEMBUS_MAI_RD_RS;
		// TODO: as above
		pulse(apr, &mai_rd_rs, 200);
	}
	if(apr->mc_rd && apr->membus.c34){
		/* 7-6, 7-9 */
		apr->n.mb |= apr->membus.c34;
		apr->membus.c34 = 0;
	}

	if(i)
		trace("--------------\n");
	else
		/* no longer needed */
		apr->ia_inh = 0;
}






void
testinst(Apr *apr)
{
	int inst;

	for(inst = 0; inst < 0700; inst++){
//	for(inst = 0140; inst < 0141; inst++){
		apr->ir = inst << 9 | 1 << 5;
		decodeir(apr);
		debug("%06o %6s ", apr->ir, mnemonics[inst]);
/*
		debug("%s ", FAC_INH ? "FAC_INH" : "       ");
		debug("%s ", FAC2 ? "FAC2" : "    ");
		debug("%s ", FC_C_ACRT ? "FC_C_ACRT" : "         ");
		debug("%s ", FC_C_ACLT ? "FC_C_ACLT" : "         ");
		debug("%s ", FC_E ? "FC_E" : "    ");
*/
		debug("%s ", FC_E_PSE ? "FC_E_PSE" : "        ");
		debug("%s ", SC_E ? "SC_E" : "    ");
		debug("%s ", SAC_INH ? "SAC_INH" : "       ");
		debug("%s ", SAC2 ? "SAC2" : "    ");
		debug("\n");
// FC_E_PSE
//debug("FC_E_PSE: %d %d %d %d %d %d %d %d %d %d\n", apr->hwt_10 , apr->hwt_11 , apr->fwt_11 ,
//                  IOT_BLK , apr->inst == EXCH , CH_DEP , CH_INC_OP ,
//                  MEMAC_MEM , apr->boole_as_10 , apr->boole_as_11);
//debug("CH: %d %d %d %d %d\n", CH_INC, CH_INC_OP, CH_N_INC_OP, CH_LOAD, CH_DEP);
//debug("FAC_INH: %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
//	apr->hwt_11 , apr->fwt_00 , apr->fwt_01 , apr->fwt_11 ,
//	apr->inst == XCT , apr->ex_ir_uuo ,
//	apr->inst == JSP , apr->inst == JSR ,
//	apr->ir_iot , IR_254_7 , MEMAC_MEM ,
//	CH_LOAD , CH_INC_OP , CH_N_INC_OP);
	}
}
