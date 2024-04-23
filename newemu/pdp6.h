#include <stdbool.h>

typedef u64 Word;
typedef u32 Hword;

typedef struct PDP6 PDP6;


void addcleanup(void (*func)(void*), void *arg);
void exitcleanup(void);


#define F0  0400000000000
#define F1  0200000000000
#define F2  0100000000000
#define F3  0040000000000
#define F4  0020000000000
#define F5  0010000000000
#define F6  0004000000000
#define F7  0002000000000
#define F8  0001000000000
#define F9  0000400000000
#define F10 0000200000000
#define F11 0000100000000
#define F12 0000040000000
#define F13 0000020000000
#define F14 0000010000000
#define F15 0000004000000
#define F16 0000002000000
#define F17 0000001000000
#define F18 0000000400000
#define F19 0000000200000
#define F20 0000000100000
#define F21 0000000040000
#define F22 0000000020000
#define F23 0000000010000
#define F24 0000000004000
#define F25 0000000002000
#define F26 0000000001000
#define F27 0000000000400
#define F28 0000000000200
#define F29 0000000000100
#define F30 0000000000040
#define F31 0000000000020
#define F32 0000000000010
#define F33 0000000000004
#define F34 0000000000002
#define F35 0000000000001
#define LT 0777777000000
#define RT 0777777
#define FW 0777777777777

typedef u32 Code;
struct PDP6
{
	int cycling;
	int state, estate;
	Code code;

	bool key_start;
	bool key_read_in;
	bool key_mem_stop;
	bool key_mem_cont;
	bool key_inst_stop;
	bool key_inst_cont;
	bool key_ex;
	bool key_ex_nxt;
	bool key_dep;
	bool key_dep_nxt;
	bool key_io_reset;
	bool key_execute;

	bool key_manual;	// for convenience

	bool sw_power;
	bool sw_mem_disable;
	bool sw_addr_stop;
	bool sw_repeat;

	bool run;
	bool key_rim_sbr;
	bool mc_stop;

	bool key_ex_st;
	bool key_dep_st;
	bool key_ex_sync;
	bool key_dep_sync;

	Word ar;
	Word mq;
	u8 mq36;
	Word mb;
	Word mi;
	Word datasw;
	Hword pc;
	Hword ma;
	Hword mas;
	Hword ir;

	Hword fe, sc;	// 9 bits

	Word iob;
	u8 iob_req;

	// SBR flip flops
	// memory
	bool key_rd_wr;
	bool if1a;
	bool af0, af3;
	bool f1a, f4a, f6a;
	bool sf3, sf5a, sf7;
	bool uuo_f1;
	bool blt_f0a;
	bool iot_f0a;
	bool chf6;
	// AR
	bool ar_com_cont;
	bool af3a;
	bool et4_ar_pause;
	bool chf3;
	bool blt_f3a, blt_f5a;
	bool faf4;
	bool msf1;
	bool nrf1;
	bool dsf1, dsf2, dsf3, dsf4, dsf5, dsf6, dsf8, dsf9;
	// shift count
	bool shf1;
	bool chf4, lcf1, dcf1, faf3;
	// SC add
	bool chf2;
	bool fsf1;
	bool faf2;
	bool fpf1, fpf2;
	// exponent
	bool fmf1;
	bool fdf1;
	// multiply
	bool mpf1;
	bool fmf2;
	// divide
	bool fdf2;

	// misc FFs
	bool faf1;	// SC PAD enable
	bool chf1;	// SC PAD enable
	bool nrf2;	// ASHC enable
	bool nrf3;	// NR ROUND disable
	bool dsf7;	// division sign flag
	bool mpf2;	// multiply sign flag
	bool iot_go;

	// char state
	bool chf5, chf7;

	bool pi_active;
	bool pi_cyc, pi_ov;
	bool pi_on;
	u8 pio, pir, pih, pi_req;

	bool ex_user;
	bool ex_mode_sync;
	bool ex_pi_sync;
	bool ex_uuo_sync;
	bool ex_ill_op;
	Hword pr, rlr, rla;

	bool mem_busy;
	bool mc_rq, mc_rd, mc_wr;

	bool cpa_iot_user;
	bool cpa_illeg_op;
	bool cpa_non_exist_mem;
	bool cpa_clock_enable;
	bool cpa_clock_flag;
	bool cpa_pc_chg_enable;
	bool cpa_pdl_ov;
	bool cpa_arov_enable;
	u8 cpa_pia;


	bool ar_cry0, ar_cry1;
	bool ar_cry0_flag, ar_cry1_flag;
	bool ar_ov_flag;
	bool ar_pc_chg_flag;


	/* PTP */
	bool sw_ptr_motor_on;
	bool sw_ptr_motor_off;
	bool ptp_tape_feed;
	bool ptp_motor_on;
	bool ptp_busy;
	bool ptp_flag;
	bool ptp_b;
	u8 ptp;
	int ptp_pia;
	int ptp_fd;
	u64 ptp_punch_timer;
	u64 ptp_motor_off_timer;
	u64 ptp_motor_on_timer;


	/* PTR */
	bool ptr_tape_feed;
	bool ptr_motor_on;
	bool ptr_clutch;
	u8 ptr_sr;
	Word ptr;
	bool ptr_busy;
	bool ptr_flag;
	bool ptr_b;
	int ptr_pia;
	FD ptr_fd;
	u64 ptr_timer;


	/* TTY */
	u8 tty_pia;
	bool tto_busy;
	bool tto_flag;
	bool tto_active;
	u8 tto;
	bool tti_busy;
	bool tti_flag;
	u8 tti;
	//
	int tty_baud, tty_dly;
	FD tty_fd;
	int tti_state;
	u64 tti_timer;
	u64 tto_timer;


	FD netmem_fd;

	bool dotrace;
	u64 clk_timer;

	/* temp - for time measurement */
	u64 sim_start, sim_end;
	u64 real_start, real_end;

	bool ia_inh;
	u64 ia_inh_timer;
};

void predecode(void);
void pwrclr(PDP6 *pdp);
void kt0(PDP6 *pdp);
void cycle(PDP6 *pdp);
void clr_run(PDP6 *pdp);
void handlenetmem(PDP6 *pdp);

extern u64 simtime;
extern u64 realtime;


enum {
	IOB_CONO_CLR,
	IOB_CONO_SET,
	IOB_DATAO_CLR,
	IOB_DATAO_SET,
	IOB_DATAI,
	IOB_STATUS,
	IOB_RESET
};

typedef struct IOdev IOdev;
struct IOdev {
	u8 req;
	int devcode;
	void *dev;
	void (*handler)(PDP6 *pdp, IOdev *dev, int cmd);
	void (*cycle)(PDP6 *pdp, IOdev *dev, int pwr);
};
void initdevs(PDP6 *pdp);
void installdev(PDP6 *pdp, IOdev *dev);
void cycle_io(PDP6 *pdp, int pwr);
void setreq(PDP6 *pdp, IOdev *dev, u8 req);

void attach_ptp(PDP6 *pdp);
void ptr_set_motor(PDP6 *pdp, int state);
void attach_ptr(PDP6 *pdp);
void attach_tty(PDP6 *pdp);


typedef struct Dis340 Dis340;
Dis340 *attach_dis(PDP6 *pdp);
void dis_connect(Dis340 *dis, int fd);


typedef struct Dc136 Dc136;
struct Dc136
{
	PDP6 *pdp;

	int pia;
	int device;
	int ch_mode;

	Word da;
	Word db;
	int cct;
	int sct;

	bool data_clbd;
	bool dbda_move;
	bool darq;
	bool dbrq;
	bool inout;
};
Dc136 *attach_dc(PDP6 *pdp);
int dctkgv(Dc136 *dc, int dev, int ch, int rt);

typedef struct Ux555 Ux555;
typedef struct Ut551 Ut551;
Ut551 *attach_ut(PDP6 *pdp, Dc136 *dc);
Ux555 *attach_ux(Ut551 *ut, int num);
void uxmount(Ux555 *ux, const char *path);
void uxunmount(Ux555 *ux);

void attach_ge(PDP6 *pdp);


void initmusic(void);
void stopmusic(void);
void svc_music(PDP6 *pdp);
