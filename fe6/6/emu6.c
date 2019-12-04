#include "../fe.h"
#include <unistd.h>

/* Memory mapped PDP-6 interface */
enum
{
	MEMIF_BASE = 0,
	APR_BASE = 010,
	

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
	REG6_MAINT_UP = 4,
	REG6_MAINT_DN = 5,

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

	/* TODO: more */
};

static int fd6;

enum
{
	WRRQ = 1,
	RDRQ = 2,
	ACK  = 3,
	ERR  = 4,
};

void
writereg(u32 addr, u32 data)
{
	u8 msg[11];
	u16 len;

	msg[0] = 0;
	msg[1] = 9;
	msg[2] = WRRQ;
	msg[3] = addr;
	msg[4] = addr>>8;
	msg[5] = addr>>16;
	msg[6] = addr>>24;
	msg[7] = data;
	msg[8] = data>>8;
	msg[9] = data>>16;
	msg[10] = data>>24;
	write(fd6, msg, msg[1]+2);

	// expecting ACK message
	if(readn(fd6, msg, 2)){
		printf("CLOSE!\r\n");
		quit();
	}
	len = msg[0]<<8 | msg[1];
	if(len != 1){
		printf("BOTCH! %X %X\r\n", msg[0], msg[1]);
		quit();
	}
	if(readn(fd6, msg, len)){
		printf("CLOSE!\r\n");
		quit();
	}
	if(msg[0] != ACK){
		printf("BOTCH! type %d\r\n", msg[0]);
		quit();
	}
}

u32
readreg(u32 addr)
{
	u8 msg[11];
	u16 len;
	u32 data;

	msg[0] = 0;
	msg[1] = 5;
	msg[2] = RDRQ;
	msg[3] = addr;
	msg[4] = addr>>8;
	msg[5] = addr>>16;
	msg[6] = addr>>24;
	write(fd6, msg, msg[1]+2);

	if(readn(fd6, msg, 2)){
		printf("CLOSE!\r\n");
		quit();
	}
	len = msg[0]<<8 | msg[1];
	if(len != 5){
		printf("BOTCH! len %d\r\n", len);
		quit();
	}
	if(readn(fd6, msg, len)){
		printf("CLOSE!\r\n");
		quit();
	}
	if(msg[0] != ACK){
		printf("BOTCH! %d\r\n", msg[0]);
		quit();
	}
	data = msg[1] | msg[2]<<8 | msg[3]<<16 | msg[4]<<24;
	return data;
}

void
deposit(hword a, word w)
{
	if(a <= 01000017){
		writereg(MEMIF_BASE, a);
		writereg(MEMIF_BASE+1, w & 0777777);
		writereg(MEMIF_BASE+2, (w>>18) & 0777777);
	}else if(a >= APR_DS && a <= APR_END){
		switch(a){
		case APR_DS:
			writereg(APR_BASE+REG6_DSLT, w>>18 & RT);
			writereg(APR_BASE+REG6_DSRT, w & RT);
			break;
		case APR_MAS:
			writereg(APR_BASE+REG6_MAS, w & RT);
			break;
		case APR_RPT:
			writereg(APR_BASE+REG6_REPEAT, w);
			break;

#ifdef TEST
		case APR_CTL1_DN:
			writereg(APR_BASE+REG6_CTL1_DN, w);
			break;
		case APR_CTL1_UP:
			writereg(APR_BASE+REG6_CTL1_UP, w);
			break;
		case APR_CTL2_DN:
			writereg(APR_BASE+REG6_CTL2_DN, w);
			break;
		case APR_CTL2_UP:
			writereg(APR_BASE+REG6_CTL2_UP, w);
			break;
#endif
		}
	}
}

word
examine(hword a)
{
	word w;
	if(a <= 01000017){
		writereg(MEMIF_BASE, a);
		w = readreg(MEMIF_BASE+2) & 0777777;
		w <<= 18;
		w |= readreg(MEMIF_BASE+1) & 0777777;
	}else if(a >= APR_DS && a < APR_END){
		switch(a){
		case APR_DS:
			w = readreg(APR_BASE+REG6_DSLT);
			w <<= 18;
			w |= readreg(APR_BASE+REG6_DSRT) & RT;
			return w;
		case APR_MAS:
			w = readreg(APR_BASE+REG6_MAS);
			break;
		case APR_RPT:
			w = readreg(APR_BASE+REG6_REPEAT);
			break;

		case APR_IR:
			return readreg(APR_BASE+REG6_IR);
		case APR_MI:
			w = readreg(APR_BASE+REG6_MILT);
			w <<= 18;
			w |= readreg(APR_BASE+REG6_MIRT) & RT;
			return w;
		case APR_PC:
			return readreg(APR_BASE+REG6_PC);
		case APR_MA:
			return readreg(APR_BASE+REG6_MA);
		case APR_PIO:
			return readreg(APR_BASE+REG6_PI)>>1 & 0177;
		case APR_PIR:
			return readreg(APR_BASE+REG6_PI)>>8 & 0177;
		case APR_PIH:
			return readreg(APR_BASE+REG6_PI)>>15 & 0177;
		case APR_PION:
			return readreg(APR_BASE+REG6_PI) & 1;

		case APR_RUN:
			return !!(readreg(APR_BASE+REG6_CTL1_DN) & MM6_RUN);
		case APR_STOP:
			return !!(readreg(APR_BASE+REG6_CTL1_DN) & MM6_MCSTOP);

#ifdef TEST
		case APR_CTL1_DN:
			return readreg(APR_BASE+REG6_CTL1_DN);
		case APR_CTL1_UP:
			return readreg(APR_BASE+REG6_CTL1_UP);
		case APR_CTL2_DN:
			return readreg(APR_BASE+REG6_CTL2_DN);
		case APR_CTL2_UP:
			return readreg(APR_BASE+REG6_CTL2_UP);
#endif

		default:
			w = 0;
		}
	}else
		w = 0;
	return w;
}

static void set_ta(hword a)
{
	writereg(APR_BASE+REG6_MAS, a & RT);
}

static void set_td(word d)
{
	writereg(APR_BASE+REG6_DSLT, d>>18 & RT);
	writereg(APR_BASE+REG6_DSRT, d & RT);
}

static void keydown(u32 k)
{
	writereg(APR_BASE+REG6_CTL1_DN, k);
}

static void keyup(u32 k)
{
	writereg(APR_BASE+REG6_CTL1_UP, k);
}

static void keytoggle(u32 k) { keydown(k); keyup(k); }

int isrunning(void)
{
	return !!(readreg(APR_BASE+REG6_CTL1_DN) & MM6_RUN);
}
int isstopped(void)
{
	return !!(readreg(APR_BASE+REG6_CTL1_DN) & MM6_MCSTOP);
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
X	typestr("<CONT>\r\n");

	if(isrunning())
		return;
	keyup(MM6_STOP);
	togglecont();
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

void
cpu_printflags(void)
{
}

void fe_svc(void) {}

void initcrt(const char *host) {}

void
init6(void)
{
	printf("waiting for connection\n");
	fd6 = serve1(10007);
	printf("fd = %d\n", fd6);
	if(fd6 < 0)
		exit(0);

//	deposit(0, 0777000777000);
//	deposit(1, 0000777000777);
//	deposit(010, 01234);
}

void
deinit6(void)
{
}
