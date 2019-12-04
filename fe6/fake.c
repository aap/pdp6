#include "fe.h"
#include <unistd.h>

static word memory[01000000];

void
deposit(hword a, word w)
{
	if((a & LT) == 0)
		memory[a&0777777] = w;
}

word
examine(hword a)
{
	if((a & LT) == 0)
		return memory[a&0777777];
	return 0;
}

static int run;
static int memstop;

static void set_ta(hword a) { printf("  TA<-%o\r\n", a); fflush(stdout); }
static void set_td(word d) { printf("  TD<-%llo\r\n", d); fflush(stdout); }
static void keydown(char *k) { printf("  %s down\r\n", k); fflush(stdout); }
static void keyup(char *k) { printf("  %s up\r\n", k); fflush(stdout); }
static void keytoggle(char *k) { keydown(k); keyup(k); }
static void waithalt(void) { printf("  WAIT HALT\r\n"); fflush(stdout); }
static void waitmemstop(void) { printf("  WAIT MEMSTOP\r\n"); fflush(stdout); }
int isrunning(void ) { return run; }
int isstopped(void ) { return memstop; }

#define X if(1)

void
cpu_start(hword a)
{
	typestr("<GO>\r\n");

	cpu_stopinst();
	X	run = 0;
	keyup("STOP");
	keyup("ADR_STOP");
	set_ta(a);
	keytoggle("START");
	X	run = 1;
	X	memstop = 0;
}

void
cpu_readin(hword a)
{
	typestr("<READIN>\r\n");

	cpu_stopinst();
	X	run = 0;
	keyup("STOP");
	keyup("ADR_STOP");
	set_ta(a);
	keytoggle("READIN");
	X	run = 1;
	X	memstop = 0;
}

void
cpu_setpc(hword a)
{
	typestr("<SETPC>\r\n");

	cpu_stopinst();
	X	run = 0;
	keydown("MEM_STOP");
	keyup("ADR_STOP");
	set_ta(a);
	keytoggle("START");
	X	run = 1;
	X	memstop = 0;
	waitmemstop();
	X	memstop = 1;
	keyup("MEM_STOP");
	keytoggle("INST_STOP");
	X	run = 0;
}

void
cpu_stopinst(void)
{
	typestr("<STOPINST>\r\n");

	if(!isrunning())
		return;
	keytoggle("INST_STOP");
	waithalt();
	X	run = 0;
}

void
cpu_stopmem(void)
{
	typestr("<STOPMEM>\r\n");

	if(!isrunning() || isstopped())
		return;
	keytoggle("MEM_STOP");
	waitmemstop();
	X	memstop = 1;
}

static void
togglecont(void)
{
	if(isstopped()){
		keytoggle("MEM_CONT");
		X	memstop = 0;
	}else{
		keytoggle("INST_CONT");
		X	memstop = 0;
		X	run = 1;
	}
}

void
cpu_cont(void)
{
	typestr("<CONT>\r\n");

	if(isrunning())
		return;
	keyup("STOP");
	togglecont();
}

void
cpu_nextinst(void)
{
	typestr("<NEXTINST>\r\n");

	if(isrunning() && !isstopped())
		err("?R? ");
	keydown("INST_STOP");
	X	run = 0;
	togglecont();
	waithalt();
	X	run = 0;
	keyup("INST_STOP");
}

void
cpu_nextmem(void)
{
	typestr("<NEXTMEM>\r\n");

	if(isrunning() && !isstopped())
		err("?R? ");
	keydown("MEM_STOP");
	togglecont();
	waitmemstop();
	X	memstop = 1;
	keyup("MEM_STOP");
}

void
cpu_exec(word inst)
{
	typestr("<EXEC>\r\n");

	if(isrunning())
		err("?R? ");
	set_td(inst);
	keytoggle("EXEC");
}

void
cpu_ioreset(void)
{
	typestr("<RESET>\r\n");

	if(isrunning())
		err("?R? ");
	keytoggle("RESET");
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
//return;
	deposit(0, 0777000777000);
	deposit(1, 0000777000777);
	deposit(010, 01234);

//	deposit(020, 01324);


	deposit(021, 02345);
	deposit(022, 03456);
	deposit(023, 01);
	deposit(024, 0666666555555);
	deposit(025, 0111111222222);

	deposit(030, 012341234);

	deposit(034, 01111111);
	deposit(035, 02222222);
	deposit(036, 03333333);
	deposit(037, 04444444);
	deposit(040, 05555555);
}

void
deinit6(void)
{
}
