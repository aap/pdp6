#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "../tools/pdp6common.h"
#include "threading.h"

#define nelem(a) (sizeof(a)/sizeof(a[0]))
#define nil NULL
#define print printf
#define fprint fprintf

typedef uint32_t u32;
typedef uint16_t u16;
typedef int8_t   i8;
typedef uint8_t  u8;
typedef unsigned char uchar;
typedef uchar bool;

extern FILE *debugfp;
extern int dotrace;
void trace(char *fmt, ...);
void debug(char *fmt, ...);
void err(char *fmt, ...);
u32 getms(void);

void strtolower(char *s);
int hasinput(int fd);
int writen(int fd, void *data, int n);
int readn(int fd, void *data, int n);
int dial(const char *host, int port);

void quit(int code);
void commandline(char *line);
void cli(FILE *f, Channel **c);
void readcmdchan(void *p);
void *cmdthread(void *);
void *simthread(void *p);
void dofile(const char *path);

enum {
	MAXPULSE = 20
};

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

typedef struct Mem Mem;
typedef struct Membus Membus;
typedef struct FMem FMem;
typedef struct CMem CMem;
typedef struct Busdev Busdev;
typedef struct IOBus IOBus;
typedef struct Apr Apr;


typedef struct Task Task;
struct Task
{
	Task *next;	/* link to next task */
	void (*f)(void*);
	void *arg;
	int freq;	/* how often the task is serviced */
	int cnt;
};

extern Task *tasks;
void addtask(Task t);

typedef struct Device Device;
struct Device
{
	Device *next;
	char *type;
	char *name;
	/* attach file to device */
	void (*attach)(Device *dev, const char *path);
	void (*detach)(Device *dev);
	/* connect device to iobus */
	void (*ioconnect)(Device *dev, IOBus *bus);
	/* manipulate device registers */
	word (*examine)(Device *dev, const char *reg);
	int  (*deposit)(Device *dev, const char *reg, word data);
};

extern Device *devlist;
void adddevice(Device *dev);
Device *getdevice(const char *name);
void showdevices(void);


/*
 * Memory
 */

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

/* A memory module connected to up to 4 Membuses */
struct Mem
{
	Device dev;
	void *module;			/* CMem, FMem */
	Membus *bus[4];
	int (*wake)(Mem *mem, Membus *bus);
	void (*poweron)(Mem *mem);
	void (*sync)(Mem *mem);		/* sync file to disk */
};

struct Membus
{
	/* c12 are cables 1 and 2, c34 3 and 4.
	 * c??_prev records the previous state of the bus
	 * and c??_pulse is used to detect leading edges. */
	word c12, c12_prev, c12_pulse;
	word c34, c34_prev, c34_pulse;

	Mem *fmem;	/* fast memory */
	Mem *cmem[16];	/* 16 16k core modules */
};
extern Membus memterm;	/* terminator */
void wakemem(Membus *bus);

/* Fast memory 162, 16 words */
struct FMem
{
	word ff[16];
	i8 fmc_p_sel;
	bool fmc_act;
	bool fmc_wr;
};
#define FMEM_IDENT "fmem162"
extern char *fmem_ident;
Mem *makefastmem(int p);
Device *makefmem(int argc, char *argv[]);

/* Core memory 161C, 16k words */
struct CMem
{
	const char *filename;

	word core[040000];
	word cmb;
	hword cma;
	bool cma_rd_rq, cma_wr_rq;
	bool cmc_aw_rq;
	i8 cmc_p_act;
	u8 cmc_last_proc;
	bool cmc_pse_sync;
};
#define CMEM_IDENT "cmem161C"
extern char *cmem_ident;
Mem *makecoremem(const char *file);
Device *makecmem(int argc, char *argv[]);

void attachmem(Mem *mem, int p, Membus *bus, int n);
void readmem(const char *file, word *mem, word size);
void showmem(Membus *bus);
word *getmemref(Membus *bus, hword addr, int fastmem);

/*
 * IO bus
 */

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

#define IOB_RESET       (bus->c34 & IOBUS_IOB_RESET)
#define IOB_DATAO_CLEAR (bus->c34 & IOBUS_DATAO_CLEAR)
#define IOB_DATAO_SET   (bus->c34 & IOBUS_DATAO_SET)
#define IOB_CONO_CLEAR  (bus->c34 & IOBUS_CONO_CLEAR)
#define IOB_CONO_SET    (bus->c34 & IOBUS_CONO_SET)
#define IOB_STATUS      (bus->c34 & IOBUS_IOB_STATUS)
#define IOB_DATAI       (bus->c34 & IOBUS_IOB_DATAI)

/* A peripheral device connected to an IO bus */
struct Busdev
{
	void *dev;
	void (*wake)(void *dev);
	u8 req;
};

struct IOBus
{
	/* c12 are cables 1 and 2, c34 3 and 4.
	 * c??_prev records the previous state of the bus
	 * and c??_pulse is used to detect leading edges. */
	word c12, c12_prev, c12_pulse;
	word c34, c34_prev, c34_pulse;
	/* device code decoded from the cables */
	int devcode;
	/* All IO devices connected to this bus */
	Busdev dev[128];
};
void setreq(IOBus *bus, int dev, u8 pia);


/* A pulse modeled as a function, and a name for debugging */
typedef struct Pulse Pulse;
struct Pulse
{
	void (*f)(void *arg);
	char *n;
};

/* A timed pulse in a linked list.
 * t is time difference since last pulse */
typedef struct TPulse TPulse;
struct TPulse
{
	TPulse *next;
	Pulse *p;
	int t;
};

/*
 * Devices
 */

/* Arithmetic Processor 166 */
#define CPA (0000>>2)
#define PI (0004>>2)

/* external pulses, bits of Apr.extpulse */
enum Extpulse {
	EXT_KEY_MANUAL       = 1,
	EXT_KEY_INST_STOP    = 2,
};

/* everything that is read AND written at the same time */
struct AprState
{
	word ar;
	word mb;
	word mq;
};

struct Apr
{
	Device dev;
	IOBus iobus;
	Membus membus;
	int powered;

	struct AprState c, n;
	hword ir;
	word mi;
	word data;
	hword pc;
	hword ma;
	hword mas;
	bool mq36;
	u16 sc, fe;
	u8 pr, rlr, rla;
	bool run;
	bool sw_addr_stop, sw_repeat, sw_mem_disable, sw_power;
	/* maint switches */
	bool sw_rim_maint, sw_rpt_bypass, sw_art3_maint, sw_sct_maint, sw_spltcyc_override;
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
	bool iot_init_setup, iot_final_setup;	// not used
	bool iot_reset;
	bool iot_go_pulse;	// for edge detection

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
	hword inst_ma;		/* for debug, memory address of instruction */
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
	double lasttick, tick;
	int extpulse;
	bool ia_inh;	// this is asserted for some time
	int pulsestepping;

	/* This could be abstracted away */
	TPulse pulses[MAXPULSE];
	TPulse *pfree;
	TPulse *pulse;
};
#define APR_IDENT "apr166"
extern char *apr_ident;
Device *makeapr(int argc, char *argv[]);


/* Paper tape reader 760 */
#define PTR (0104>>2)
typedef struct Ptr Ptr;
struct Ptr
{
	Device dev;
	IOBus *bus;
	int motor_on;
	word sr;
	word ptr;
	bool busy, flag, b;
	int pia;

	int fd;
};
#define PTR_IDENT "ptr760"
extern char *ptr_ident;
Device *makeptr(int argc, char *argv[]);
void ptr_setmotor(Ptr *ptr, int m);


/* Paper tape punch 761 */
#define PTP (0100>>2)
typedef struct Ptp Ptp;
struct Ptp
{
	Device dev;
	IOBus *bus;
	uchar ptp;
	bool busy, flag, b;
	int pia;

	int fd;
	int waitdatao;
};
#define PTP_IDENT "ptp761"
extern char *ptp_ident;
Device *makeptp(int argc, char *argv[]);


/* Teletype 626 */
#define TTY (0120>>2)
typedef struct Tty Tty;
struct Tty
{
	Device dev;
	IOBus *bus;
	uchar tto, tti;
	bool tto_busy, tto_flag;
	bool tti_busy, tti_flag;
	int pia;
	int fd;
};
#define TTY_IDENT "tty626"
extern char *tty_ident;
Device *maketty(int argc, char *argv[]);

/* Data Control 136 */
#define DC (0200>>2)
typedef struct Dc136 Dc136;
struct Dc136
{
	Device dev;
	IOBus *bus;
	int pia;
	int device;
	int ch_mode;
	void *devp[8];

	word da;
	word db;
	int cct;
	int sct;

	int data_clbd;	/* data clobbered */
	int dbda_move;
	int da_rq;
	int db_rq;
	int inout;

	/* lines between dc and devices 1/2:
	 * tk/gv rt
	 * tk/gv lt
	 * clr cct 1
	 * darq(1)
	 * sel 1
	 */
};
#define DC_IDENT "dc136"
extern char *dc_ident;
Device *makedc(int argc, char *argv[]);
void dcgv(Dc136 *dc, int n, word c, int rev);
word dctk(Dc136 *dc, int n, int rev);

/* Microtape/DECtape */

/* 555 has:
 *  move tape fwd, rev, auto
 *  unit selection (1-8)
 *  motor off, write lock on/off
*/

typedef struct Dx555 Dx555;
typedef struct Dt551 Dt551;

/* Transport 555,
 * a half of it really */
struct Dx555
{
	Device dev;

	Dt551 *dt;

	int unit;
	int wrlock;

	int fd;
	uchar *start;
	uchar *cur;
	uchar *end;
	uint size;
};
#define DX_IDENT "dx555"
extern char *dx_ident;
Device *makedx(int argc, char *argv[]);
void dxconn(Dt551 *dt, Dx555 *dx, int n);
/*
void dxatt(Dx555 *dx, uchar *buf, int size);
void dxff(Dx555 *dx);
void dxrew(Dx555 *dx);
*/

/* Control 551 */
#define UTC (0210>>2)
#define UTS (0214>>2)
struct Dt551
{
	Device dev;
	IOBus *bus;

	Dc136 *dc;
	Dx555 *dx[8];
	Dx555 *seldx;

	/* enable time and mark track writing */
	int ut_btm_switch;

	int ut_pia;
	int ut_units;
	int ut_units_select;
	int ut_fcn;
	int ut_time;
	int ut_wren;
	int ut_go;
	int ut_rev;
	int ut_tape_end_flag;
	int ut_tape_end_enable;
	int ut_jb_done_flag;
	int ut_jb_done_enable;
	int time_enable;
	int time_flag;
	int ut_illegal_op;
	int ut_info_error;
	int ut_incomp_block;

	int tmk;	/* mark track window, 9b */
	int rwb;	/* read write buffer, 6b */
	int lb;		/* longitudinal buffer, computes check sum, 6b */
	int tbm;	/* block mark timing, 4b */
	int tdata;	/* data timing, 8b */
	int tct;	/* selects half of 6 bit rwb, 1b */
	/* error check */
	int utek;	/* 6 bits */
	int uteck;

	int rw_state;	/* null, rq, active */

	int delay;
	int sense;
	int wb;

//	int req;
};
#define DT_IDENT "dt551"
extern char *dt_ident;
Device *makedt(int argc, char *argv[]);
void dtconn(Dc136 *dc, Dt551 *dt);

typedef struct Netmem Netmem;
struct Netmem
{
	Device dev;
	int fd;
	// tmp?
	Apr *apr;
};
#define NETMEM_IDENT "netmem"
extern char *netmem_ident;
Device *makenetmem(int argc, char *argv[]);
