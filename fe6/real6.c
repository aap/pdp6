#include "fe6.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
//#include "hps_0.h"

#define H2F_BASE (0xC0000000)

#define PERIPH_BASE (0xFC000000)
#define PERIPH_SPAN (0x04000000)
#define PERIPH_MASK (PERIPH_SPAN - 1)

#define LWH2F_BASE (0xFF200000)

/* Memory mapped PDP-6 interface */
enum
{
	/* The more important keys, switches and lights */
	REG6_CTL1_DN = 0,
	REG6_CTL1_UP = 1,
	 MM6_START	= 1,
	 MM6_READIN	= 2,
	 MM6_INSTCONT	= 4,
	 MM6_MEMCONT	= 010,
	 MM6_INSTSTOP	= 020,
	 MM6_MEMSTOP	= 040,
	  MM6_STOP = MM6_INSTSTOP|MM6_MEMSTOP,
	 MM6_RESET	= 0100,
	 MM6_EXEC	= 0200,
	 MM6_ADRSTOP	= 0400,
	 /* lights - read only */
	 MM6_RUN	= 01000,
	 MM6_MCSTOP	= 02000,
	 MM6_PWR	= 04000,

	/* Less important keys and switches */
	REG6_CTL2_DN = 2,
	REG6_CTL2_UP = 3,
	 MM6_THISDEP	= 1,
	 MM6_NEXTDEP	= 2,
	 MM6_THISEX	= 4,
	 MM6_NEXTEX	= 010,
	 MM6_READEROFF	= 020,
	 MM6_READERON	= 040,
	 MM6_FEEDPUNCH	= 0100,
	 MM6_FEEDREAD	= 0200,
	 MM6_REPEAT	= 0400,
	 MM6_MEMDIS	= 01000,

	/* Maintenance switches */
	REG6_MAINT_DN = 4,
	REG6_MAINT_UP = 5,

	/* switches and knobs */
	REG6_DSLT = 6,
	REG6_DSRT = 7,
	REG6_MAS = 010,
	REG6_REPEAT = 011,

	/* lights */
	REG6_IR = 012,
	REG6_MILT = 013,
	REG6_MIRT = 014,
	REG6_PC = 015,
	REG6_MA = 016,
	REG6_PI = 017,

	REG6_MBLT = 020,
	REG6_MBRT = 021,
	REG6_ARLT = 022,
	REG6_ARRT = 023,
	REG6_MQLT = 024,
	REG6_MQRT = 025,
	REG6_FF1 = 026,
	REG6_FF2 = 027,
	REG6_FF3 = 030,
	REG6_FF4 = 031,
	REG6_MMU = 032,

	REG6_TTY = 033,
	REG6_PTP = 034,
	REG6_PTR = 035,
	REG6_PTR_LT = 036,
	REG6_PTR_RT = 037,
};

enum {
	FEREG_REQ = 0,
	FEREG_PTR,
	FEREG_PTP
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
static volatile u32 *h2f_cmemif, *h2f_cmemif2;
static volatile u32 *h2f_fmemif, *h2f_fmemif2;
static volatile u32 *h2f_apr;
static volatile u32 *h2f_fe;
static volatile u32 *h2f_lw_led_addr;
static volatile u32 *h2f_lw_sw_addr;

void
deposit(hword a, word w)
{
/*
	if(a < 01000000){
		h2f_cmemif[0] = a & RT;
		h2f_cmemif[1] = w & RT;
		h2f_cmemif[2] = (w >> 18) & RT;
	}else if(a < 01000020){
		h2f_fmemif[0] = a & 017;
		h2f_fmemif[1] = w & RT;
		h2f_fmemif[2] = (w >> 18) & RT;
	}else if(a < 01000040){
		h2f_fmemif2[0] = a & 017 | 01000000;
		h2f_fmemif2[1] = w & RT;
		h2f_fmemif2[2] = (w >> 18) & RT;
	}else if(a >= 02000000 && a < 03000000){
		h2f_cmemif2[0] = a & RT;
		h2f_cmemif2[1] = w & RT;
		h2f_cmemif2[2] = (w >> 18) & RT;
*/


	if(a < 020){
		h2f_fmemif[0] = a & 017;
		h2f_fmemif[1] = w & RT;
		h2f_fmemif[2] = (w >> 18) & RT;
	}else if(a < 01000020){
		h2f_cmemif[0] = a & RT;
		h2f_cmemif[1] = w & RT;
		h2f_cmemif[2] = (w >> 18) & RT;
	}else if(a >= 02000000){
		h2f_cmemif2[0] = a & 01777777;
		h2f_cmemif2[1] = w & RT;
		h2f_cmemif2[2] = (w >> 18) & RT;
	}else switch(a){
	case APR_DS:
		h2f_apr[REG6_DSLT] = w>>18 & RT;
		h2f_apr[REG6_DSRT] = w & RT;
		break;
	case APR_MAS:
		h2f_apr[REG6_MAS] = w & RT;
		break;
	case APR_RPT:
		h2f_apr[REG6_REPEAT] = w;
		break;

#ifdef TEST
	case APR_CTL1_DN:
		h2f_apr[REG6_CTL1_DN] = w;
		break;
	case APR_CTL1_UP:
		h2f_apr[REG6_CTL1_UP] = w;
		break;
	case APR_CTL2_DN:
		h2f_apr[REG6_CTL2_DN] = w;
		break;
	case APR_CTL2_UP:
		h2f_apr[REG6_CTL2_UP] = w;
		break;

	case PTR_FE:
		h2f_fe[FEREG_PTR] = w;
		break;
#endif

	case 01001000:
		*h2f_lw_led_addr = w;
		break;
	}
}

word
examine(hword a)
{
	u64 w;
	w = 0;

/*
	if(a < 01000000){
		h2f_cmemif[0] = a & RT;
		w = h2f_cmemif[2] & RT;
		w <<= 18;
		w |= h2f_cmemif[1] & RT;
	}else if(a < 01000020){
		h2f_fmemif[0] = a & 017;
		w = h2f_fmemif[2] & RT;
		w <<= 18;
		w |= h2f_fmemif[1] & RT;
	}else if(a < 01000040){
		h2f_fmemif2[0] = a & 017 | 01000000;
		w = h2f_fmemif2[2] & RT;
		w <<= 18;
		w |= h2f_fmemif2[1] & RT;
	}else if(a >= 02000000 && a < 03000000){
		h2f_cmemif2[0] = a & RT;
		w = h2f_cmemif2[2] & RT;
		w <<= 18;
		w |= h2f_cmemif2[1] & RT;
*/


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
	}else if(a >= 02000000){
		h2f_cmemif2[0] = a & 01777777;
		w = h2f_cmemif2[2] & RT;
		w <<= 18;
		w |= h2f_cmemif2[1] & RT;
	}else switch(a){
	case APR_DS:
		w = h2f_apr[REG6_DSLT];
		w <<= 18;
		w |= h2f_apr[REG6_DSRT] & RT;
		return w;
	case APR_MAS:
		w = h2f_apr[REG6_MAS];
		break;
	case APR_RPT:
		w = h2f_apr[REG6_REPEAT];
		break;

	case APR_IR:
		return h2f_apr[REG6_IR];
	case APR_MI:
		w = h2f_apr[REG6_MILT];
		w <<= 18;
		w |= h2f_apr[REG6_MIRT] & RT;
		return w;
	case APR_PC:
		return h2f_apr[REG6_PC];
	case APR_MA:
		return h2f_apr[REG6_MA];
	case APR_PIO:
		return h2f_apr[REG6_PI]>>1 & 0177;
	case APR_PIR:
		return h2f_apr[REG6_PI]>>8 & 0177;
	case APR_PIH:
		return h2f_apr[REG6_PI]>>15 & 0177;
	case APR_PION:
		return h2f_apr[REG6_PI] & 1;

	case APR_RUN:
		return !!(h2f_apr[REG6_CTL1_DN] & MM6_RUN);
	case APR_STOP:
		return !!(h2f_apr[REG6_CTL1_DN] & MM6_MCSTOP);

#ifdef TEST
	case APR_CTL1_DN:
		return h2f_apr[REG6_CTL1_DN];
	case APR_CTL1_UP:
		return h2f_apr[REG6_CTL1_UP];
	case APR_CTL2_DN:
		return h2f_apr[REG6_CTL2_DN];
	case APR_CTL2_UP:
		return h2f_apr[REG6_CTL2_UP];
	case APR_MB:
		w = h2f_apr[REG6_MBLT];
		w <<= 18;
		w |= h2f_apr[REG6_MBRT] & RT;
		return w;
	case APR_AR:
		w = h2f_apr[REG6_ARLT];
		w <<= 18;
		w |= h2f_apr[REG6_ARRT] & RT;
		return w;
	case APR_MQ:
		w = h2f_apr[REG6_MQLT];
		w <<= 18;
		w |= h2f_apr[REG6_MQRT] & RT;
		return w;

	case TTY_TTI:
		return (h2f_apr[REG6_TTY]>>9) & 0377;
	case TTY_ST:
		return h2f_apr[REG6_TTY] & 0177;
	case PTR_PTR:
		w = h2f_apr[REG6_PTR_LT];
		w <<= 18;
		w |= h2f_apr[REG6_PTR_RT] & RT;
		return w;
	case PTR_ST:
		return h2f_apr[REG6_PTR] & 0177;

	case FE_REQ:
		return h2f_fe[FEREG_REQ];
#endif

	case 01001000:
		w = *h2f_lw_led_addr;
		break;
	case 01001001:
		w = *h2f_lw_sw_addr;
		break;
	}
	return w;
}



static void set_ta(hword a)
{
	h2f_apr[REG6_MAS] = a & RT;
}

static void set_td(word d)
{
	h2f_apr[REG6_DSLT] = d>>18 & RT;
	h2f_apr[REG6_DSRT] = d & RT;
}

static void keydown(u32 k)
{
	h2f_apr[REG6_CTL1_DN] = k;
	if(k & MM6_INSTSTOP)
		usleep(1000);	// wait for AT1 INH to go down
}

static void keyup(u32 k)
{
	h2f_apr[REG6_CTL1_UP] = k;
}

static void keytoggle(u32 k) {
	keydown(k);
	usleep(1000);	// TODO: maybe don't sleep? or different duration?
	keyup(k);
}

int isrunning(void)
{
	return !!(h2f_apr[REG6_CTL1_DN] & MM6_RUN);
}
int isstopped(void)
{
	return !!(h2f_apr[REG6_CTL1_DN] & MM6_MCSTOP);
}

static void waithalt(void)
{
	int i;
	for(i = 0; i < 10; i++){
		if(!isrunning())
			return;
		usleep(100);
	}
	keytoggle(MM6_INSTSTOP);
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
	for(i = 0; i < 10; i++){
		if(isstopped())
			return;
		usleep(100);
	}
	keytoggle(MM6_MEMSTOP);
	for(i = 0; i < 10; i++){
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
	keyup(MM6_STOP | MM6_ADRSTOP);
	set_ta(a);
	keytoggle(MM6_START);
	X	run = 1;
	X	memstop = 0;
}

void
cpu_readin(hword a)
{
X	typestr("<READIN>\r\n");

	cpu_stopinst();
	X	run = 0;
	keyup(MM6_STOP | MM6_ADRSTOP);
	set_ta(a);
	keytoggle(MM6_READIN);
	X	run = 1;
	X	memstop = 0;
}

void
cpu_setpc(hword a)
{
X	typestr("<SETPC>\r\n");

	cpu_stopinst();
	X	run = 0;
	// TODO: maybe INSTSTOP?
	keydown(MM6_MEMSTOP);
	keyup(MM6_ADRSTOP);
	set_ta(a);
	keytoggle(MM6_START);
	X	run = 1;
	X	memstop = 0;
	waitmemstop();
	X	memstop = 1;
	keyup(MM6_MEMSTOP);
	keytoggle(MM6_INSTSTOP);
	X	run = 0;
}

void
cpu_stopinst(void)
{
X	typestr("<STOPINST>\r\n");

	if(!isrunning())
		return;
	keytoggle(MM6_INSTSTOP);
	waithalt();
	X	run = 0;
}

void
cpu_stopmem(void)
{
X	typestr("<STOPMEM>\r\n");

	if(!isrunning() || isstopped())
		return;
	keytoggle(MM6_MEMSTOP);
	waitmemstop();
	X	memstop = 1;
}

static void
togglecont(void)
{
	if(isstopped()){
		keytoggle(MM6_MEMCONT);
		X	memstop = 0;
	}else{
		keytoggle(MM6_INSTCONT);
		X	memstop = 0;
		X	run = 1;
	}
}

void
cpu_cont(void)
{
	int stop;
X	typestr("<CONT>\r\n");

	if(isrunning())
		return;
	stop = isstopped();
	keyup(MM6_STOP);
	togglecont();

	// on stop the machine should halt after one instruction
	// so restart
	// BUG: if next instruction is HALT we'll continue past it
	if(stop){
		waithalt();
		togglecont();
	}
}

void
cpu_nextinst(void)
{
X	typestr("<NEXTINST>\r\n");

	if(isrunning() && !isstopped())
		err("?R? ");
	keydown(MM6_INSTSTOP);
	X	run = 0;
	togglecont();
	waithalt();
	X	run = 0;
	keyup(MM6_INSTSTOP);
}

void
cpu_nextmem(void)
{
X	typestr("<NEXTMEM>\r\n");

	if(isrunning() && !isstopped())
		err("?R? ");
	keydown(MM6_MEMSTOP);
	togglecont();
	waitmemstop();
	X	memstop = 1;
	keyup(MM6_MEMSTOP);
}

void
cpu_exec(word inst)
{
X	typestr("<EXEC>\r\n");

	if(isrunning())
		err("?R? ");
	set_td(inst);
	keytoggle(MM6_EXEC);
}

void
cpu_ioreset(void) 
{
X	typestr("<RESET>\r\n");

	if(isrunning())
		err("?R? ");
	keytoggle(MM6_RESET);
}

#include "flags.inc"

void
prflags(const char *fmt, u8 flags)
{
	static const char *l = ".#";
	printf(fmt,
		l[!!(flags&0200)], l[!!(flags&0100)],
		l[!!(flags&040)], l[!!(flags&020)],
		l[!!(flags&010)], l[!!(flags&04)],
		l[!!(flags&02)], l[!!(flags&01)]);
}

void
cpu_printflags(void)
{
	u32 ff1, ff2, ff3, ff4;
	u32 ctl1, pi;
	ff1 = h2f_apr[REG6_FF1];
	ff2 = h2f_apr[REG6_FF2];
	ff3 = h2f_apr[REG6_FF3];
	ff4 = h2f_apr[REG6_FF4];
	ctl1 = h2f_apr[REG6_CTL1_DN];
	pi = h2f_apr[REG6_PI];

	printf("\r\n");
	prflags(ff0str, ff1>>24);
	prflags(ff1str, ff1>>16);
	prflags(ff2str, ff1>>8);
	prflags(ff3str, ff1);
	prflags(ff4str, ff2>>24);
	prflags(ff5str, ff2>>16);
	prflags(ff6str, ff2>>8);
	prflags(ff7str, ff2);
	prflags(ff8str, ff3>>24);
	prflags(ff9str, ff3>>16);
	prflags(ff10str, ff3>>8);
	prflags(ff11str, ff3);
	prflags(ff12str, ff4>>24);
	prflags(ff13str, ff4>>16);

	printf("PIH/%03o   PIR/%03o   PIO/%03o   PI ACTIVE/%o\r\n",
		pi>>15 & 0177, pi>>8 & 0177, pi>>1 & 0177, !!(pi & 1));
	printf("RUN/%o     MEM STOP/%o\r\n",
		!!(ctl1 & MM6_RUN), !!(ctl1 & MM6_MCSTOP));

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

void
fe_svc(void)
{
	u32 req;

	req = h2f_fe[FEREG_REQ];

	if(req & 1) svc_ptr();
	if(req & 2) svc_ptp();
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
	h2f_cmemif2 = getLWH2Faddr(0x20000);

	h2f_fmemif = getLWH2Faddr(0x10010);
	h2f_fmemif2 = getLWH2Faddr(0x20010);


	h2f_apr = getLWH2Faddr(0x10100);
	h2f_fe = getLWH2Faddr(0x20000);
	h2f_lw_sw_addr = getLWH2Faddr(0x10020);
	h2f_lw_led_addr = getLWH2Faddr(0x10040);
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
