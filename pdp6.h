#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define nelem(a) (sizeof(a)/sizeof(a[0]))

typedef uint64_t word;
typedef uint32_t hword;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef char bool;

typedef struct Apr Apr;
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
	u8 pio, pir, pih;
	u16 sc, fe;
	u8 pr, rlr, rla;
	bool run, pi_active;
	bool sw_addr_stop, sw_repeat, sw_mem_disable, sw_power;
	bool sw_rim_maint;
	/* keys */
	bool key_start, key_readin;
	bool key_mem_cont, key_inst_cont;
	bool key_mem_stop, key_inst_stop;
	bool key_io_reset, key_execute;
	bool key_dep, key_dep_next;
	bool key_ex, key_ex_next;
	bool key_rd_off, key_rd_on;
	bool key_pt_rd, key_pt_wr;

	/* flip-flops */
	bool ex_mode_sync, ex_uuo_sync, ex_pi_sync, ex_ill_op, ex_user;
	bool pc_chg_flag, ar_ov_flag, ar_cry0_flag, ar_cry1_flag;
	bool pi_ov, pi_cyc, pi_req;

	bool key_ex_st, key_ex_sync;
	bool key_dep_st, key_dep_sync;
	bool key_rd_wr, key_rim_sbr;

	bool mc_rd, mc_wr, mc_rq, mc_stop, mc_stop_sync, mc_split_cyc_sync;

	bool cpa_iot_user, cpa_illeg_op, cpa_non_exist_mem,
	     cpa_clock_en, cpa_clock_flag, cpa_pc_chg_en, cpa_pdl_ov,
	     cpa_arov_en, cpa_pia33, cpa_pia34, cpa_pia35;

	bool cry0_cry1, ar_cry0, ar_cry1;
	/* ?? */
	bool mq36;
	bool iot_go, a_long, uuo_f1;

	/* sbr flip-flops */
	bool chf7;

	/* temporaries */
	bool ex_inh_rel;
};
extern Apr apr;
void *aprmain(void *p);
extern int extpulse;

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
