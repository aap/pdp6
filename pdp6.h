#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#define nelem(a) (sizeof(a)/sizeof(a[0]))
#define nil NULL
#define print printf
#define fprint fprintf

typedef uint64_t word;
typedef uint32_t hword;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef unsigned char uchar;
typedef uchar bool;

enum Mask {
	FW   = 0777777777777,
	RT   = 0000000777777,
	LT   = 0777777000000,
	SGN  = 0400000000000,
	RSGN = 0000000400000,
};

enum HalfwordBits {
	H0  = 0400000, H1  = 0200000, H2  = 0100000,
	H3  = 0040000, H4  = 0020000, H5  = 0010000,
	H6  = 0004000, H7  = 0002000, H8  = 0001000,
	H9  = 0000400, H10 = 0000200, H11 = 0000100,
	H12 = 0000040, H13 = 0000020, H14 = 0000010,
	H15 = 0000004, H16 = 0000002, H17 = 0000001
};

enum FullwordBits {
	FCRY = 01000000000000,
	F0  = 0400000000000, F1  = 0200000000000, F2  = 0100000000000,
	F3  = 0040000000000, F4  = 0020000000000, F5  = 0010000000000,
	F6  = 0004000000000, F7  = 0002000000000, F8  = 0001000000000,
	F9  = 0000400000000, F10 = 0000200000000, F11 = 0000100000000,
	F12 = 0000040000000, F13 = 0000020000000, F14 = 0000010000000,
	F15 = 0000004000000, F16 = 0000002000000, F17 = 0000001000000,
	F18 = 0000000400000, F19 = 0000000200000, F20 = 0000000100000,
	F21 = 0000000040000, F22 = 0000000020000, F23 = 0000000010000,
	F24 = 0000000004000, F25 = 0000000002000, F26 = 0000000001000,
	F27 = 0000000000400, F28 = 0000000000200, F29 = 0000000000100,
	F30 = 0000000000040, F31 = 0000000000020, F32 = 0000000000010,
	F33 = 0000000000004, F34 = 0000000000002, F35 = 0000000000001
};

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

enum {
	MAXPULSE = 5
};

typedef struct Apr Apr;

typedef void Pulse(Apr *apr);
#define pulse(p) static void p(Apr *apr)

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
	bool mq36;
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
	     cpa_clock_enable, cpa_clock_flag, cpa_pc_chg_enable, cpa_pdl_ov,
	     cpa_arov_enable;
	int cpa_pia;

	bool iot_go;

	/* ?? */
	bool a_long;

	/* sbr flip-flops */
	bool if1a;
	bool af0, af3, af3a;
	bool f1a, f4a, f6a;
	bool et4_ar_pse;
	bool chf1, chf2, chf3, chf4, chf5, chf6, chf7;
	bool lcf1, dcf1;
	bool sf3, sf5a, sf7;
	bool shf1;
	bool mpf1, mpf2;
	bool msf1;
	bool dsf1, dsf2, dsf3, dsf4, dsf5, dsf6, dsf7, dsf8, dsf9;
	bool fsf1;
	bool fmf1, fmf2;
	bool fdf1, fdf2;
	bool faf1, faf2, faf3, faf4;
	bool fpf1, fpf2;
	bool nrf1, nrf2, nrf3;
	bool iot_f0a;
	bool blt_f0a, blt_f3a, blt_f5a;
	bool uuo_f1;

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

	bool fc_e_pse;
	bool pc_set;

	/* needed for the emulation */
	int extpulse;
	bool ia_inh;	// this is asserted for some time

	Pulse *pulses1[MAXPULSE], *pulses2[MAXPULSE];
	Pulse **clist, **nlist;
	int ncurpulses, nnextpulses;
};
extern Apr apr;
void nextpulse(Apr *apr, Pulse *p);
void *aprmain(void *p);

void initmem(void);
void dumpmem(void);
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
	MEMBUS_WR_RS        = 0100000000000,
	MEMBUS_MAI_RD_RS    = 0200000000000,
	MEMBUS_MAI_ADDR_ACK = 0400000000000,
};
/* 0 is cable 1 & 2 (above bits); 1 is cable 3 & 4 (data) */
extern word membus0, membus1;

// 7-10
enum {
	IOBUS_PI_REQ_7    = F35,
	IOBUS_PI_REQ_6    = F34,
	IOBUS_PI_REQ_5    = F33,
	IOBUS_PI_REQ_4    = F32,
	IOBUS_PI_REQ_3    = F31,
	IOBUS_PI_REQ_2    = F30,
	IOBUS_PI_REQ_1    = F29,
	IOBUS_IOB_STATUS  = F23,
	IOBUS_IOB_DATAI   = F22,
	IOBUS_CONO_SET    = F21,
	IOBUS_CONO_CLEAR  = F20,
	IOBUS_DATAO_SET   = F19,
	IOBUS_DATAO_CLEAR = F18,
	IOBUS_IOS9_1      = F17,
	IOBUS_IOS9_0      = F16,
	IOBUS_IOS8_1      = F15,
	IOBUS_IOS8_0      = F14,
	IOBUS_IOS7_1      = F13,
	IOBUS_IOS7_0      = F12,
	IOBUS_IOS6_1      = F11,
	IOBUS_IOS6_0      = F10,
	IOBUS_IOS5_1      = F9,
	IOBUS_IOS5_0      = F8,
	IOBUS_IOS4_1      = F7,
	IOBUS_IOS4_0      = F6,
	IOBUS_IOS3_1      = F5,
	IOBUS_IOS3_0      = F4,
	IOBUS_MC_DR_SPLIT = F3,
	IOBUS_POWER_ON    = F1,
	IOBUS_IOB_RESET   = F0,
	IOBUS_PULSES = IOBUS_CONO_SET | IOBUS_CONO_CLEAR |
	               IOBUS_DATAO_SET | IOBUS_DATAO_CLEAR
};
/* 0 is cable 1 & 2 (data); 1 is cable 3 & 4 (above bits) */
extern word iobus0, iobus1;

#define IOB_RESET       (iobus1 & IOBUS_IOB_RESET)
#define IOB_DATAO_CLEAR (iobus1 & IOBUS_DATAO_CLEAR)
#define IOB_DATAO_SET   (iobus1 & IOBUS_DATAO_SET)
#define IOB_CONO_CLEAR  (iobus1 & IOBUS_CONO_CLEAR)
#define IOB_CONO_SET    (iobus1 & IOBUS_CONO_SET)
#define IOB_STATUS      (iobus1 & IOBUS_IOB_STATUS)
#define IOB_DATAI       (iobus1 & IOBUS_IOB_DATAI)

/* every entry is a function to wake up the device */
/* TODO: how to handle multiple APRs? */
extern void (*iobusmap[128])(void);
/* current PI req for each device */
extern u8 ioreq[128];
void recalc_req(void);

void inittty(void);

//void wakepanel(void);

// for debugging
char *names[0700];
char *ionames[010];
