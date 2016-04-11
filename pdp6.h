#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#define nelem(a) (sizeof(a)/sizeof(a[0]))

typedef uint64_t word;
typedef uint32_t hword;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef char bool;

enum Mask {
	FW   = 0777777777777,
	RT   = 0000000777777,
	LT   = 0777777000000,
	SGN  = 0400000000000,
	RSGN = 0000000400000,
};

enum Opcode {
	FSC    = 0132,
	IBP    = 0133,
	CAO    = 0133,
	LDCI   = 0134,
	LDC    = 0135,
	DPCI   = 0136,
	DPC    = 0137,
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

enum {
	MAXPULSE = 5
};

typedef struct Apr Apr;

typedef void Pulse(Apr *apr);
#define pulse(p) void p(Apr *apr)

struct Apr {
	hword ir;
	word mi;
	word data;
	hword pc;
	hword ma;
	hword mas;
	word mb;
	word ar;
	word mq;
	u16 sc, fe;
	u8 pr, rlr, rla;
	bool run;
	bool sw_addr_stop, sw_repeat, sw_mem_disable, sw_power;
	bool sw_rim_maint;
	/* keys */
	bool key_start, key_readin;
	bool key_mem_cont, key_inst_cont;
	bool key_mem_stop, key_inst_stop;
	bool key_io_reset, key_exec;
	bool key_dep, key_dep_nxt;
	bool key_ex, key_ex_nxt;
	bool key_rd_off, key_rd_on;
	bool key_pt_rd, key_pt_wr;

	/* PI */
	u8 pio, pir, pih, pi_req;
	bool pi_active;
	bool pi_ov, pi_cyc;

	/* flip-flops */
	bool ex_mode_sync, ex_uuo_sync, ex_pi_sync, ex_ill_op, ex_user;
	bool ar_pc_chg_flag, ar_ov_flag, ar_cry0_flag, ar_cry1_flag;
	bool ar_cry0, ar_cry1, ar_com_cont;
	bool ar_cry0_xor_cry1;

	bool key_ex_st, key_ex_sync;
	bool key_dep_st, key_dep_sync;
	bool key_rd_wr, key_rim_sbr;

	bool mc_rd, mc_wr, mc_rq, mc_stop, mc_stop_sync, mc_split_cyc_sync;

	bool cpa_iot_user, cpa_illeg_op, cpa_non_exist_mem,
	     cpa_clock_en, cpa_clock_flag, cpa_pc_chg_en, cpa_pdl_ov,
	     cpa_arov_en, cpa_pia33, cpa_pia34, cpa_pia35;

	/* ?? */
	bool mq36;
	bool iot_go, a_long, uuo_f1;

	/* sbr flip-flops */
	bool if1a;
	bool af0, af3, af3a;
	bool f1a, f4a, f6a;
	bool et4_ar_pse;
	bool chf5, chf7;
	bool sf3, sf5a, sf7;
	bool shf1;

	/* temporaries */
	bool ex_inh_rel;

	/* decoded instructions */
	int inst, io_inst;
	bool ir_fp;
	bool ir_fwt;
	bool fwt_00, fwt_01, fwt_10, fwt_11;
	bool shift_op, ir_md, ir_jp, ir_as;
	bool ir_boole;
	bool boole_as_00, boole_as_01, boole_as_10, boole_as_11;
	int ir_boole_op;
	bool ir_hwt;
	bool hwt_00, hwt_01, hwt_10, hwt_11;
	bool ir_acbm;
	bool ex_ir_uuo, ir_iot, ir_jrst;

	bool fac2;
	bool fac_inh, fc_e, fc_e_pse;
	bool e_long, mb_pc_sto, pc_set;
	bool sc_e, sac_inh, sac2;

	/* needed for the emulation */
	int extpulse;
	bool ia_inh;	// this is asserted for some time
	// want to get rid of this
	Pulse *sct2_ret;

	Pulse *pulses1[MAXPULSE], *pulses2[MAXPULSE];
	Pulse **clist, **nlist;
	int ncurpulses, nnextpulses;
};
extern Apr apr;
void nextpulse(Apr *apr, Pulse *p);
void *aprmain(void *p);

void initmem(void);
void wakemem(void);
// 7-2, 7-10
enum {
	MEMBUS_MA21         = 0000000000001,
	MEMBUS_WR_RQ        = 0000000000004,
	MEMBUS_RD_RQ        = 0000000000010,
	MEMBUS_MA_FMC_SEL0  = 0000001000000,
	MEMBUS_MA_FMC_SEL1  = 0000002000000,
	MEMBUS_MA35_0       = 0000004000000,
	MEMBUS_MA35_1       = 0000010000000,
	MEMBUS_MA21_0       = 0000020000000,
	MEMBUS_MA21_1       = 0000040000000,
	MEMBUS_MA20_0       = 0000100000000,
	MEMBUS_MA20_1       = 0000200000000,
	MEMBUS_MA19_0       = 0000400000000,
	MEMBUS_MA19_1       = 0001000000000,
	MEMBUS_MA18_0       = 0002000000000,
	MEMBUS_MA18_1       = 0004000000000,
	MEMBUS_RQ_CYC       = 0020000000000,
	MEMBUS_MAI_WR_RS    = 0100000000000,
	MEMBUS_MAI_RD_RS    = 0200000000000,
	MEMBUS_MAI_ADDR_ACK = 0400000000000,
};
extern word membus0, membus1;

extern word iobus0, iobus1;
