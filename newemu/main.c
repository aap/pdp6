#include "common.h"
#include "pdp6.h"
#include "args.h"

#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include <sys/socket.h>

#include <signal.h>

u64 simtime;
u64 realtime;

typedef struct Panel Panel;
void updateswitches(PDP6 *pdp, Panel *panel);
void updatelights(PDP6 *pdp, Panel *panel);
void lightsoff(Panel *panel);
Panel *getpanel(void);


// tmp
extern Word memory[01000000];
extern Word fmem[020];

#define I(op, ac, i, x, y)\
	((Word)(op)<<27 |\
	 (Word)(ac)<<23 |\
	 (Word)(i)<<22 |\
	 (Word)(x)<<18 |\
	 (Word)(y))

#define IO(op, dev, i, x, y)\
	((Word)(7)<<33 |\
	 (Word)(dev)<<24 |\
	 (Word)(op)<<23 |\
	 (Word)(i)<<22 |\
	 (Word)(x)<<18 |\
	 (Word)(y))

Hword
readmemory(const char *path)
{
	char line[128], *p;
	Hword a, pc;
	Word d;
	FILE *f;

	f = fopen(path, "r");
	if(f == nil) {
		printf("couldn't open file %s\n", path);
		return 0;
	}

	pc = 0;
	while(p = fgets(line, sizeof(line), f)) {
		if(*p == 'd') {
			p++;
			sscanf(p, "%o %llo", &a, &d);
			memory[a] = d;
		} else if(*p == 'g') {
			p++;
			sscanf(p, "%o", &pc);
			break;
		}
	}

	fclose(f);
	return pc;
}

void
readmem(const char *file, Word *mem, Hword size)
{
	FILE *f;
	char buf[100], *s;
	Hword a;
	Word w;
	if(f = fopen(file, "r"), f == nil)
		return;
	a = 0;
	while(s = fgets(buf, 100, f)){
		while(*s){
			if(*s == ';')
				break;
			else if('0' <= *s && *s <= '7'){
				w = strtoull(s, &s, 8);
				if(*s == ':' || *s == '/'){
					a = w;
					s++;
				}else if(a < size)
					mem[a++] = w;
				else
					fprintf(stderr, "Address out of range: %o\n", a++);
			}else
				s++;
		}
	}
	fclose(f);
}

void
dumpmem(const char *file, Word *mem, Hword size)
{
	FILE *f;
	Hword a;

	if(f = fopen("coremem", "w"), f == nil)
		return;

	for(a = 0; a < size; a++)
		if(mem[a] != 0){
			fprintf(f, "%06o/	", a);
			fprintf(f, "%012llo\n", mem[a]);
		}
	fclose(f);
}

const bool dothrottle = 1;

// TODO: this sucks
Dis340 *dis;
Ux555 *ux1;
Ux555 *ux2;
Ux555 *ux3;
Ux555 *ux4;

void
configmachine(PDP6 *pdp)
{
//	uxmount(ux1, "t/systemdis.dtr");
	uxmount(ux1, "t/system.dtr");
//	uxmount(ux2, "t/syseng.dtr");
	uxmount(ux2, "t/its138.dtr");
//	uxmount(ux3, "t/foo.dtr");
	uxmount(ux3, "t/music.dtr");

	/* MACDMP RIM loader */
	memory[000] = 0255000000000;
	memory[001] = 0205000255000;
	memory[002] = 0700200635550;
	memory[003] = 0700600011577;
	memory[004] = 0721200223110;
	memory[005] = 0720200004010;
	memory[006] = 0720340001000;
	memory[007] = 0254000000006;
	memory[010] = 0720040000013;
	memory[011] = 0345540000006;
	memory[012] = 0602540777777;
	memory[013] = 0000000000013;
	memory[014] = 0254000000006;

	/* PTR RIM loader */
	memory[020] = 0710600000060;
	memory[021] = 0710740000010;
	memory[022] = 0254000000021;
	memory[023] = 0710440000026;
	memory[024] = 0710740000010;
	memory[025] = 0254000000024;
	memory[027] = 0254000000021;
	pdp->pc = 020;

//	pdp->pc = readmemory("t/pdp6.part3.oct");
	// part3: assumes non-existent memory at 777777 at PC=2004
	//        assumes reader motor will be off
//	memory[0101] |= I(0, 1, 0, 0, 0);	// disable device tests in part3
	// part4: start with 1 in switches, flip back to 0 at some point
	pdp->pc = readmemory("t/ddt.16k.oct");
//	pdp->pc = readmemory("t/ddt.d16k.oct");	// needs moby
//	pdp->pc = readmemory("t/nts.lisp.u16k.oct");
//	pdp->pc = readmemory("t/nts.teco6.u256k.oct");
//	pdp->pc = readmemory("t/spcwar.oct");
//	pdp->pc = readmemory("t/hello.oct");
//	pdp->pc = readmemory("t/its138.oct");
//	pdp->pc = readmemory("t/music.oct");
}

int domusic = 0;

void
initemu(PDP6 *pdp)
{
	memset(pdp, 0, sizeof(*pdp));

	predecode();

	initdevs(pdp);
	attach_ptp(pdp);
	attach_ptr(pdp);
	attach_tty(pdp);
	dis = attach_dis(pdp);
	Dc136 *dc = attach_dc(pdp);
	Ut551 *ut = attach_ut(pdp, dc);
	ux1 = attach_ux(ut, 1);
	ux2 = attach_ux(ut, 2);
	ux3 = attach_ux(ut, 3);
	ux4 = attach_ux(ut, 4);
	// stub for ITS
	attach_ge(pdp);


	srand(time(nil));
	pwrclr(pdp);
	cycle_io(pdp, 0);

if(domusic) initmusic();
}


void prompt(void) { printf("> "); fflush(stdout); }
void
cli(PDP6 *pdp, FD *fd)
{
	char line[1024];
	int n;

	n = read(fd->fd, line, sizeof(line));
	if(n <= 0) {
		printf("\n");
		exit(0);
	}
	if(n > 0 && n < sizeof(line))  {
		line[n] = '\0';
void doline(char *line);
		doline(line);
		prompt();
		waitfd(fd);
	}
}

void
throttle(void)
{
	while(realtime < simtime) {
		usleep(1000);
		realtime = gettime();
	}
}

void
emu(PDP6 *pdp, Panel *panel)
{
	bool key_manual;
	bool key_inst_stop;
	bool power;

	updateswitches(pdp, panel);

	FD clifd;
	clifd.id = -1;
	clifd.fd = 0;
	prompt();
	waitfd(&clifd);

	inittime();
	simtime = 0;	//gettime();
	pdp->clk_timer = 0;
	for(;;) {
		key_manual = pdp->key_manual;
		key_inst_stop = pdp->key_inst_stop;
		power = pdp->sw_power;
		updateswitches(pdp, panel);

		if(pdp->sw_power) {
			if(pdp->key_manual && !key_manual)
				kt0(pdp);
			if(pdp->key_inst_stop && !key_inst_stop) {
				clr_run(pdp);
				pdp->ia_inh = 1;
				pdp->ia_inh_timer = simtime + 100000;
			}

			if(pdp->ia_inh && simtime > pdp->ia_inh_timer)
				pdp->ia_inh = 0;

			if(pdp->cycling) {
				// avg: 1.4 - 1.7μs
				cycle(pdp);
svc_music(pdp);
			} else {
				simtime += 1500;	// keep things running

				pdp->ia_inh = 0;
stopmusic();
			}
			updatelights(pdp, panel);

			cycle_io(pdp, 1);
			handlenetmem(pdp);

			if(dothrottle) throttle();
while(realtime < simtime) realtime = gettime();
		} else {
stopmusic();
			if(power)
				pwrclr(pdp);

			lightsoff(panel);

			if(dothrottle)
				simtime = gettime();
			else
				simtime += 1500;
			cycle_io(pdp, 0);
		}

		if(clifd.ready)
			cli(pdp, &clifd);
	}
}

void 
handledis(int fd, void *arg)
{
//	PDP6 *pdp = (PDP6*)arg;
	nodelay(fd);
	dis_connect(dis, fd);
}

void
handleptr(int fd, void *arg)
{
	PDP6 *pdp = (PDP6*)arg;
	nodelay(fd);
	ptrmount(pdp, fd);
}

void
handleptp(int fd, void *arg)
{
	PDP6 *pdp = (PDP6*)arg;
	nodelay(fd);
	ptpmount(pdp, fd);
}

void*
netthread(void *arg)
{
	struct PortHandler ports[] = {
//		{ 1640, handlenetcmd },
//		// 1041 is teletype
		{ 1642, handleptr },
		{ 1643, handleptp },
		{ 3400, handledis },
	};
	serveN(ports, nelem(ports), arg);
        return nil;
}

/*
char *argv0;
void
usage(void)
{
	fprintf(stderr, "usage: %s [-h host] [-p port]\n", argv0);
	exit(1);
}
*/

static void
cleanup(void *arg)
{
	Panel *panel = (Panel*)arg;
	lightsoff(panel);
	dumpmem("coremem", memory, 01000000);
}

void
inthandler(int sig)
{
	exit(0);
}

int
main(int argc, char *argv[])
{
	pthread_t th;
	Panel *panel;
	PDP6 pdp6, *pdp = &pdp6;

/*
	const char *host;
	int port;

	host = "localhost";
	port = 3400;
	ARGBEGIN {
	case 'h':
		host = EARGF(usage());
		break;
	case 'p':
		port = atoi(EARGF(usage()));
		break;
	default:
		usage();
	} ARGEND;
*/

	panel = getpanel();
	if(panel == nil) {
		fprintf(stderr, "can't find operator panel\n");
		return 1;
	}

	atexit(exitcleanup);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, inthandler);

	addcleanup(cleanup, panel);

	initemu(pdp);
	readmem("coremem", memory, 01000000);

	startpolling();

// now in lua
//	configmachine(pdp);

/*
	int dis_fd = dial(host, port);
	if(dis_fd < 0)
		printf("can't open display\n");
	nodelay(dis_fd);
	dis_connect(dis, dis_fd);
*/

/*
	const char *tape = "t/hello.rim";
//	const char *tape = "t/ptp_test.rim";
//	const char *tape = "t/bla.txt";
	pdp->ptr_fd.fd = open(tape, O_RDONLY);
	waitfd(&pdp->ptr_fd);

	pdp->ptp_fd = open("out.ptp", O_CREAT|O_WRONLY|O_TRUNC, 0644);
*/

/*
	pdp->tty_fd.fd = open("/tmp/tty", O_RDWR);
	if(pdp->tty_fd.fd < 0)
		printf("can't open /tmp/tty\n");
	waitfd(&pdp->tty_fd);
*/



//	pdp->netmem_fd.fd = dial("maya", 10006);
/*
	pdp->netmem_fd.fd = dial("maya", 20006);
	if(pdp->netmem_fd.fd >= 0)
		printf("netmem connected\n");
	waitfd(&pdp->netmem_fd);
*/

void initlua(PDP6 *pdp);
        initlua(pdp);

	pthread_create(&th, nil, netthread, pdp);
	int fd[2];
	socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
	pdp->tty_fd.fd = fd[0];
	waitfd(&pdp->tty_fd);
void ttytelnet(int port, int fd);
	ttytelnet(1641, fd[1]);

	emu(pdp, panel);
	return 0;	// can't happen
}
