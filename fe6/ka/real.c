#include "../fe.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define H2F_BASE (0xC0000000)

#define PERIPH_BASE (0xFC000000)
#define PERIPH_SPAN (0x04000000)
#define PERIPH_MASK (PERIPH_SPAN - 1)

#define LWH2F_BASE (0xFF200000)

/* Memory mapped KA10 interface */
enum
{
	/* keys, switches and some lights */
	REG_SW_DN = 0,
	REG_SW_UP = 1,
	 // keys
	 MMKA_DEP_NXT	= 1,
	 MMKA_DEP	= 2,
	 MMKA_EX_NXT	= 4,
	 MMKA_EX	= 010,
	 MMKA_XCT	= 020,
	 MMKA_RESET	= 040,
	 MMKA_STOP	= 0100,
	 MMKA_CONT	= 0200,
	 MMKA_START	= 0400,
	 MMKA_READIN	= 01000,
	 // switches
	 MMKA_ADR_BRK	= 02000,
	 MMKA_ADR_STOP	= 04000,
	 MMKA_ADR_WR	= 010000,
	 MMKA_ADR_RD	= 020000,
	 MMKA_ADR_INST	= 040000,
	 MMKA_RPT	= 0100000,
	 MMKA_NXM_STOP	= 0200000,
	 MMKA_PAR_STOP	= 0400000,
	 MMKA_SING_CYC	= 01000000,
	 MMKA_SING_INST	= 02000000,
	 /* lights */
	 MMKA_MEM_STOP	= 04000000,
	 MMKA_USER	= 010000000,
	 MMKA_PROG_STOP	= 020000000,
	 MMKA_PWR_ON	= 040000000,
	 MMKA_RUN	= 0100000000,

	/* Maintenance switches */
	REG_MAINT_DN = 2,
	REG_MAINT_UP = 3,
	 MMKA_RDI_SEL	= 0177,
	 MMKA_MI_PROG_DIS	= 0200,
	 MMKA_RPT_BYPASS	= 0400,
	 MMKA_FM_EN	= 01000,
	 MMKA_SC_STOP	= 02000,

	/* switches and knobs */
	REG_DSLT = 4,
	REG_DSRT = 5,
	REG_AS = 6,
	REG_REPEAT = 7,

	/* lights */
	REG_IR = 010,
	REG_MILT = 011,
	REG_MIRT = 012,
	REG_PC = 013,
	REG_MA = 014,
	REG_PI = 015,

	REG_ARLT = 016,
	REG_ARRT = 017,
	REG_BRLT = 020,
	REG_BRRT = 021,
	REG_MQLT = 022,
	REG_MQRT = 023,
	REG_ADLT = 024,
	REG_ADRT = 025,
	REG_SC_FE = 026,
	REG_SCAD = 027,
	REG_KEY_OPR = 030,
	REG_F_S_FMA = 031,
	REG_PR_RL = 032,
	REG_RLA_MEM = 033,
	REG_CPA_MISC = 034,
	REG_REST = 035,

	REG_TTY = 040,
	REG_PTP = 041,
	REG_PTR = 042,
	REG_PTR_LT = 043,
	REG_PTR_RT = 044,
};

enum {
	FEREG_REQ = 0,
	FEREG_PTR,
	FEREG_PTP,
	FEREG_DIS
};


static u64 *h2f_base;
static u32 *virtual_base;
static u32 *getLWH2Faddr(u32 offset)
{
	return (u32*)((u32)virtual_base - PERIPH_BASE + (LWH2F_BASE+offset));
}
static u64 *getH2Faddr(u32 offset)
{
	return (u64*)((u32)h2f_base + offset);
}

static int memfd;
static volatile u32 *h2f_cmemif;
static volatile u32 *h2f_fmemif;
static volatile u32 *h2f_apr;
static volatile u32 *h2f_fe;
static volatile u32 *h2f_csl;

void
deposit(hword a, word w)
{
	if(a < 020){
		h2f_fmemif[0] = a & 017;
		h2f_fmemif[1] = w & RT;
		h2f_fmemif[2] = (w >> 18) & RT;
	}else if(a < 01000020){
		h2f_cmemif[0] = a & RT;
		h2f_cmemif[1] = w & RT;
		h2f_cmemif[2] = (w >> 18) & RT;
	}else switch(a){
	case APR_DS:
		h2f_apr[REG_DSLT] = w>>18 & RT;
		h2f_apr[REG_DSRT] = w & RT;
		break;
	case APR_AS:
		h2f_apr[REG_AS] = w & RT;
		break;
	case APR_RPT:
		h2f_apr[REG_REPEAT] = w;
		break;

#ifdef TEST
	case APR_SW_DN:
		h2f_apr[REG_SW_DN] = w;
		break;
	case APR_SW_UP:
		h2f_apr[REG_SW_UP] = w;
		break;
	case APR_MAINT_DN:
		h2f_apr[REG_MAINT_DN] = w;
		break;
	case APR_MAINT_UP:
		h2f_apr[REG_MAINT_UP] = w;
		break;

	case PTR_FE:
		h2f_fe[FEREG_PTR] = w;
		break;
#endif
	}
}

word
examine(hword a)
{
	u64 w;
	w = 0;

	if(a < 020){
		h2f_fmemif[0] = a & 017;
		w = h2f_fmemif[2] & RT;
		w <<= 18;
		w |= h2f_fmemif[1] & RT;
	}else if(a < 01000020){
		h2f_cmemif[0] = a & RT;
		w = h2f_cmemif[2] & RT;
		w <<= 18;
		w |= h2f_cmemif[1] & RT;
	}else switch(a){
	case APR_DS:
		w = h2f_apr[REG_DSLT] & RT;
		w <<= 18;
		w |= h2f_apr[REG_DSRT] & RT;
		return w;
	case APR_AS:
		w = h2f_apr[REG_AS];
		break;
	case APR_RPT:
		w = h2f_apr[REG_REPEAT];
		break;

	case APR_IR:
		return h2f_apr[REG_IR] & RT;
	case APR_MI:
		w = h2f_apr[REG_MILT] & RT;
		w <<= 18;
		w |= h2f_apr[REG_MIRT] & RT;
		return w;
	case APR_PC:
		return h2f_apr[REG_PC];
	case APR_MA:
		return h2f_apr[REG_MA];
	case APR_PIO:
		return h2f_apr[REG_PI]>>1 & 0177;
	case APR_PIR:
		return h2f_apr[REG_PI]>>8 & 0177;
	case APR_PIH:
		return h2f_apr[REG_PI]>>15 & 0177;
	case APR_PION:
		return h2f_apr[REG_PI] & 1;

	case APR_RUN:
		return !!(h2f_apr[REG_SW_DN] & MMKA_RUN);
	case APR_STOP:
		return !!(h2f_apr[REG_SW_DN] & MMKA_MEM_STOP);

#ifdef TEST
	case APR_SW_DN:
		return h2f_apr[REG_SW_DN];
	case APR_SW_UP:
		return h2f_apr[REG_SW_UP];
	case APR_MAINT_DN:
		return h2f_apr[REG_MAINT_DN];
	case APR_MAINT_UP:
		return h2f_apr[REG_MAINT_UP];
	case APR_AR:
		w = h2f_apr[REG_ARLT] & RT;
		w <<= 18;
		w |= h2f_apr[REG_ARRT] & RT;
		return w;
	case APR_BR:
		w = h2f_apr[REG_BRLT] & RT;
		w <<= 18;
		w |= h2f_apr[REG_BRRT] & RT;
		return w;
	case APR_MQ:
		w = h2f_apr[REG_MQLT] & RT;
		w <<= 18;
		w |= h2f_apr[REG_MQRT] & RT;
		return w;
	case APR_AD:
		w = h2f_apr[REG_ADLT] & RT;
		w <<= 18;
		w |= h2f_apr[REG_ADRT] & RT;
		return w;

	case TTY_TTI:
		return (h2f_apr[REG_TTY]>>9) & 0377;
	case TTY_ST:
		return h2f_apr[REG_TTY] & 0177;
	case PTR_PTR:
		w = h2f_apr[REG_PTR_LT];
		w <<= 18;
		w |= h2f_apr[REG_PTR_RT] & RT;
		return w;
	case PTR_ST:
		return h2f_apr[REG_PTR] & 0177;

	case FE_REQ:
		return h2f_fe[FEREG_REQ];
#endif
	}
	return w;
}



static void set_ta(hword a)
{
	h2f_apr[REG_AS] = a & RT;
}

static void set_td(word d)
{
	h2f_apr[REG_DSLT] = d>>18 & RT;
	h2f_apr[REG_DSRT] = d & RT;
}

static void keydown(u32 k)
{
	h2f_apr[REG_SW_DN] = k;
	// TODO: still needed with KA?
	if(k & MMKA_STOP)
		usleep(1000);	// wait for AT INH to go down
}

static void keyup(u32 k)
{
	h2f_apr[REG_SW_UP] = k;
}

static void keytoggle(u32 k) {
	keydown(k);
	usleep(1000);	// TODO: maybe don't sleep? or different duration?
	keyup(k);
}

int isrunning(void)
{
	return !!(h2f_apr[REG_SW_DN] & MMKA_RUN);
}
int isstopped(void)
{
	return !!(h2f_apr[REG_SW_DN] & MMKA_MEM_STOP);
}

static void waithalt(void)
{
	int i;
	for(i = 0; i < 10; i++){
		if(!isrunning())
			return;
		usleep(100);
	}
	keytoggle(MMKA_STOP);
	for(i = 0; i < 10; i++){
		if(!isrunning())
			return;
		usleep(100);
	}
	typestr("not halted!!!\r\n");
}

static void waitmemstop(void)
{
	int i;
	if(!isrunning())
		return;
	for(i = 0; i < 20; i++){
		if(isstopped())
			return;
		usleep(100);
	}
	typestr("not stopped!!!\r\n");
}

static int run;
static int memstop;
#define X if(0)

void
cpu_start(hword a)
{
X	typestr("<GO>\r\n");

	cpu_stopinst();
	X	run = 0;
	keyup(MMKA_SING_INST | MMKA_ADR_STOP);
	set_ta(a);
	keytoggle(MMKA_START);
	X	run = 1;
	X	memstop = 0;
}

void
cpu_readin(hword a)
{
X	typestr("<READIN>\r\n");

	cpu_stopinst();
	X	run = 0;
	keyup(MMKA_SING_INST | MMKA_ADR_STOP);
	h2f_apr[REG_MAINT_UP] = 0177;
	h2f_apr[REG_MAINT_DN] = a & 0177;
	keytoggle(MMKA_READIN);
	X	run = 1;
	X	memstop = 0;
}

void
cpu_setpc(hword a)
{
X	typestr("<SETPC>\r\n");

	cpu_stopinst();
	X	run = 0;
	keydown(MMKA_SING_CYC);
	keyup(MMKA_ADR_STOP);
	set_ta(a);
	keytoggle(MMKA_START);
	X	run = 1;
	X	memstop = 0;
	waitmemstop();
	X	memstop = 1;
	keyup(MMKA_SING_CYC);
	keytoggle(MMKA_STOP);
	X	run = 0;
	keytoggle(MMKA_EX);
	usleep(1000);
	X	memstop = 0;
}

void
cpu_stopinst(void)
{
X	typestr("<STOPINST>\r\n");

	if(!isrunning())
		return;
	// TODO: what if memory stop?
	keytoggle(MMKA_STOP);
	keydown(MMKA_SING_INST);
	waithalt();
	X	run = 0;
	keyup(MMKA_SING_INST);
}

void
cpu_stopmem(void)
{
X	typestr("<STOPMEM>\r\n");

	if(!isrunning() || isstopped())
		return;
	keydown(MMKA_SING_CYC);
	waitmemstop();
	X	memstop = 1;
	keyup(MMKA_SING_CYC);
}

static void
togglecont(void)
{
	keytoggle(MMKA_CONT);
}

void
cpu_cont(void)
{
X	typestr("<CONT>\r\n");

	if(isrunning())
		return;
	keyup(MMKA_SING_CYC | MMKA_SING_INST | MMKA_ADR_STOP);
	togglecont();
}

void
cpu_nextinst(void)
{
X	typestr("<NEXTINST>\r\n");

	if(isrunning() && !isstopped())
		err("?R? ");
	keydown(MMKA_SING_INST);
	X	run = 0;
	togglecont();
	waithalt();
	X	run = 0;
	keyup(MMKA_SING_INST);
}

void
cpu_nextmem(void)
{
X	typestr("<NEXTMEM>\r\n");

	if(isrunning() && !isstopped())
		err("?R? ");
	keydown(MMKA_SING_CYC);
	togglecont();
	waitmemstop();
	X	memstop = 1;
	keyup(MMKA_SING_CYC);
}

void
cpu_exec(word inst)
{
X	typestr("<EXEC>\r\n");

	if(isrunning())
		err("?R? ");
	set_td(inst);
	keytoggle(MMKA_XCT);
}

void
cpu_ioreset(void) 
{
X	typestr("<RESET>\r\n");

	if(isrunning())
		err("?R? ");
	keytoggle(MMKA_RESET);
	typestr("\r\n");
}

void
cpu_printflags(void)
{
	static const char *l = ".#";
	u32 ctl, pi;
	u32 sc_fe, scad, key_opr, f_s_fma, pr_rl, rla_mem, cpa_misc, rest;

	ctl = h2f_apr[REG_SW_DN];
	pi = h2f_apr[REG_PI];
	sc_fe = h2f_apr[REG_SC_FE];
	scad = h2f_apr[REG_SCAD];
	key_opr = h2f_apr[REG_KEY_OPR];
	f_s_fma = h2f_apr[REG_F_S_FMA];
	pr_rl = h2f_apr[REG_PR_RL];
	rla_mem = h2f_apr[REG_RLA_MEM];
	cpa_misc = h2f_apr[REG_CPA_MISC];
	rest = h2f_apr[REG_REST];

	printf("\r\n");
	printf("FE %03o  SC %03o STOP %c\r\n", sc_fe&0777, sc_fe>>9&0777,
		l[!!(sc_fe & 01000000)]);
	printf("SCAD %03o\r\n", scad&0777);

	printf("KEY F1 %c  SYNC RQ %c  SYNC %c  RSET %c\r\n"
		"    EXAM %c  EX NXT %c  DEP %c  DEP NXT %c\r\n"
		"    RDI %c  STRT %c  EXE %c  CONT %c\r\n",
		l[!!(key_opr&040000000)],
		l[!!(key_opr&020000000)],
		l[!!(key_opr&010000000)],
		l[!!(key_opr&04000000)],
		l[!!(key_opr&02000000)],
		l[!!(key_opr&01000000)],
		l[!!(key_opr&0400000)],
		l[!!(key_opr&0200000)],
		l[!!(key_opr&0100000)],
		l[!!(key_opr&040000)],
		l[!!(key_opr&020000)],
		l[!!(key_opr&010000)]);

	printf("OPR IF0 %c  AF2 %c  FF1 %c  FF2 %c  FF4 %c\r\n"
		"    E UUOF %c  E XCTF %c  E LONG %c  EF0 LONG %c\r\n"
		"    SF1 %c  SF6 %c  SF8 %c\r\n",
		l[!!(key_opr&04000)],
		l[!!(key_opr&02000)],
		l[!!(key_opr&01000)],
		l[!!(key_opr&0400)],
		l[!!(key_opr&0200)],
		l[!!(key_opr&0100)],
		l[!!(key_opr&040)],
		l[!!(key_opr&020)],
		l[!!(key_opr&010)],
		l[!!(key_opr&04)],
		l[!!(key_opr&02)],
		l[!!(key_opr&01)]);

	printf("FETCH FCE %c  FCE PSE %c  FAC INH %c  FAC2 %c"
		"  FCC ACLT %c  FCC ACRT\r\n",
		l[!!(f_s_fma&040000)],
		l[!!(f_s_fma&020000)],
		l[!!(f_s_fma&010000)],
		l[!!(f_s_fma&04000)],
		l[!!(f_s_fma&02000)],
		l[!!(f_s_fma&01000)]);

	printf("STORE SCE %c  ST INH %c  SAC2 %c  SAC INH %c  SAC=0 %c\r\n",
		l[!!(f_s_fma&0400)],
		l[!!(f_s_fma&0200)],
		l[!!(f_s_fma&0100)],
		l[!!(f_s_fma&040)],
		l[!!(f_s_fma&020)]);

	printf("FMA %02o\r\n", f_s_fma & 017);

	printf("PR RL %o\r\n", pr_rl);

	printf("RLA %03o  RLC %03o\r\n", rla_mem<<1 & 0776, rla_mem>>7 & 0776);
	printf("MEM MC RQ %c  MC RD %c  MC WR %c  REQ CYC %c  SPLIT SYNC %c\r\n"
		"    FM EN %c  FMA SEL %c  FMA AC %c  FMA AC2 %c  FMA XR %c\r\n",
		l[!!(rla_mem&0200000000)],
		l[!!(rla_mem&0100000000)],
		l[!!(rla_mem&040000000)],
		l[!!(rla_mem&020000000)],
		l[!!(rla_mem&010000000)],
		l[!!(rla_mem&04000000)],
		l[!!(rla_mem&02000000)],
		l[!!(rla_mem&01000000)],
		l[!!(rla_mem&0400000)],
		l[!!(rla_mem&0200000)]);
		
	printf("CPA PWR %c  ADR BRK %c  PAR ERR %c  PAR EN %c  PDL OV %c  MEM PROT %c\r\n"
		"    NXM FLAG %c  CLK EN %c  CLK FLAG %c  FOV EN %c  AROV EN %c\r\n"
		"    PIA %o\r\n",
		l[!!(cpa_misc&020000)],
		l[!!(cpa_misc&010000)],
		l[!!(cpa_misc&04000)],
		l[!!(cpa_misc&02000)],
		l[!!(cpa_misc&01000)],
		l[!!(cpa_misc&0400)],
		l[!!(cpa_misc&0200)],
		l[!!(cpa_misc&0100)],
		l[!!(cpa_misc&040)],
		l[!!(cpa_misc&020)],
		l[!!(cpa_misc&010)],
		cpa_misc&07);

	printf("MISC %o\r\n", cpa_misc>>14);

	printf("EX ILL OP %c  PI SYNC %c  MODE SYNC %c  IOT USER %c  REL %c\r\n",
		l[!!(rest&020000)],
		l[!!(rest&010000)],
		l[!!(rest&04000)],
		l[!!(rest&02000)],
		l[!!(rest&01000)]);
	printf("PI OV %c  CYC %c\r\n",
		l[!!(rest&0400)],
		l[!!(rest&0200)]);
	printf("BYTE LOAD %c  DEP %c\r\n",
		l[!!(rest&0100)],
		l[!!(rest&040)]);
	printf("NR SHRT COND %c  NOR %c  RND %c\r\n",
		l[!!(rest&020)],
		l[!!(rest&010)],
		l[!!(rest&04)]);
	printf("AS= RLA %c  FMA %c\r\n",
		l[!!(rest&02)],
		l[!!(rest&01)]);

	printf("PIH/%03o   PIR/%03o   PIO/%03o   PI ACTIVE/%o\r\n",
		pi>>15 & 0177, pi>>8 & 0177, pi>>1 & 0177, !!(pi & 1));
	printf("PWR %c   RUN %c   MCSTOP %c   PROGSTOP %c   USER %c\r\n",
		l[!!(ctl&MMKA_PWR_ON)],
		l[!!(ctl&MMKA_RUN)],
		l[!!(ctl&MMKA_MEM_STOP)],
		l[!!(ctl&MMKA_PROG_STOP)],
		l[!!(ctl&MMKA_USER)]);
	fflush(stdout);
}

static void
svc_ptr(void)
{
	int fd;
	u8 c;

	fd = devtab[DEV_PTR].fd;
	if(fd < 0)
		return;
	if(read(fd, &c, 1) == 1){
printf("%d%d%d%d%d%d%d%d -> PTR\r\n",
	!!(c&0200), !!(c&0100), !!(c&040), !!(c&020), !!(c&010),
	!!(c&04), !!(c&02), !!(c&01));
fflush(stdout);
		h2f_fe[FEREG_PTR] = c;
	}
}

static void
svc_ptp(void)
{
	int fd;
	u8 c;

	c = h2f_fe[FEREG_PTP];
printf("PTP <- %d%d%d%d%d%d%d%d\r\n",
	!!(c&0200), !!(c&0100), !!(c&040), !!(c&020), !!(c&010),
	!!(c&04), !!(c&02), !!(c&01));
fflush(stdout);

	fd = devtab[DEV_PTP].fd;
	if(fd < 0)
		return;
	write(fd, &c, 1);
}

int dis_fd;

static void
svc_dis(void)
{
	u32 pnt;
	pnt = h2f_fe[FEREG_DIS];
	if((pnt & 0x80000000) == 0)
		return;
	if(dis_fd >= 0)
		write(dis_fd, &pnt, 4);
	else{
		printf("%X\r\n", pnt);
		fflush(stdout);
	}
}

void
fe_svc(void)
{
	u32 req;

	req = h2f_fe[FEREG_REQ];

	if(req & 1) svc_ptr();
	if(req & 2) svc_ptp();
//	if(req & 4) svc_dis();
	svc_dis();
}

void*
wcsl_thread(void *arg)
{
	u32 ctl;
	while(readn(dis_fd, &ctl, 4) == 0){
//		printf("%o\r\n", ctl);
//		fflush(stdout);
		h2f_csl[ctl>>24] = ctl;
	}
}

void
initcrt(const char *host)
{
	dis_fd = dial(host, 3400);
	if(dis_fd >= 0){
		printf("display connected\n");
		threadcreate(wcsl_thread, nil);
	}
}

void
init6(void)
{
	if((memfd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1) {
		fprintf(stderr, "ERROR: could not open /dev/mem...\n");
		exit(1);
	}
	virtual_base = (u32*)mmap(nil, PERIPH_SPAN,
		(PROT_READ | PROT_WRITE), MAP_SHARED, memfd, PERIPH_BASE);
	if(virtual_base == MAP_FAILED) {
		fprintf(stderr, "ERROR: mmap() failed...\n");
		close(memfd);
		exit(1);
	}
	h2f_base = (u64*)mmap(nil, 0x100000,
		(PROT_READ | PROT_WRITE), MAP_SHARED, memfd, H2F_BASE);
	if(h2f_base == MAP_FAILED) {
		fprintf(stderr, "ERROR: mmap() failed...\n");
		close(memfd);
		exit(1);
	}

	h2f_cmemif = getLWH2Faddr(0x10000);
	h2f_fmemif = getLWH2Faddr(0x10010);
	h2f_apr = getLWH2Faddr(0x10100);
	h2f_fe = getLWH2Faddr(0x20000);
	h2f_csl = getLWH2Faddr(0x30000);
}

void
deinit6(void)
{
	if(munmap(virtual_base, PERIPH_SPAN) != 0) {
		fprintf(stderr, "ERROR: munmap() failed...\n");
		close(memfd);
		exit(1);
	}
	close(memfd);
}
