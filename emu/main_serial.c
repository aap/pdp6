#include "pdp6.h"
#include <stdarg.h>
#include "args.h"

#include <fcntl.h>
#include <termios.h>


// TODO: get rid of this
void updatepanel(Apr *apr) {}
void writeconsreg(Apr *apr, u32 addr, u32 data)
{ // stub
}
u32
readconsreg(Apr *apr, u32 addr)
{
	// stub
	return 0;
}

#define KEYPULSE(k) (apr->k && !oldapr.k)

void
talkserial(int fd, Apr *apr, Ptr *ptr)
{
	struct termios ios;
	Apr oldapr;
	char c, buf[32];
	int n, sw;

	if(tcgetattr(fd, &ios) == 0){
		cfsetispeed(&ios, B38400);
		cfsetospeed(&ios, B38400);
		cfmakeraw(&ios);
		tcsetattr(fd, TCSANOW, &ios);
	}

	/* Synch to CR */
	while(readn(fd, &c, 1) == 0, c != 015);

	sw = 0;
	while(1){
		if(apr == nil)
			return;
		oldapr = *apr;

		/* Read switch and key state */
		if(readn(fd, buf, 15))
			break;
		apr->data = 0;
		apr->data |= ((word)buf[0]&077) << 30;
		apr->data |= ((word)buf[1]&077) << 24;
		apr->data |= ((word)buf[2]&077) << 18;
		apr->data |= ((word)buf[3]&077) << 12;
		apr->data |= ((word)buf[4]&077) << 6;
		apr->data |= ((word)buf[5]&077) << 0;
		apr->mas = 0;
		apr->mas |= ((word)buf[6]&077) << 12;
		apr->mas |= ((word)buf[7]&077) << 6;
		apr->mas |= ((word)buf[8]&077) << 0;

		apr->key_start     = !!(buf[10] & 040);
		apr->key_readin    = !!(buf[9]  & 040);
		apr->key_inst_cont = !!(buf[10] & 020);
		apr->key_mem_cont  = !!(buf[9]  & 020);
		apr->key_inst_stop = !!(buf[10] & 010);
		apr->key_mem_stop  = !!(buf[9]  & 010);
		apr->key_io_reset  = !!(buf[10] & 4);
		apr->key_exec      = !!(buf[9]  & 4);
		apr->key_dep       = !!(buf[10] & 2);
		apr->key_dep_nxt   = !!(buf[9]  & 2);
		apr->key_ex        = !!(buf[10] & 1);
		apr->key_ex_nxt    = !!(buf[9]  & 1);
		apr->key_rd_off    = !!(buf[11] & 010);
		apr->key_rd_on     = !!(buf[11] & 4);
		apr->key_pt_rd     = !!(buf[11] & 2);
		apr->key_pt_wr     = !!(buf[11] & 1);

		apr->sw_addr_stop   = !!(buf[12] & 010);
		apr->sw_repeat      = !!(buf[12] & 4);
		apr->sw_mem_disable = !!(buf[12] & 2);
		apr->sw_power       = !!(buf[12] & 1);

		// TODO: buf[13] is a speed knob
		// buf[14] is CR

		if(apr->sw_power){
			if(KEYPULSE(key_start) || KEYPULSE(key_readin) ||
			   KEYPULSE(key_inst_cont) || KEYPULSE(key_mem_cont) ||
			   KEYPULSE(key_io_reset) || KEYPULSE(key_exec) ||
			   KEYPULSE(key_dep) || KEYPULSE(key_dep_nxt) ||
			   KEYPULSE(key_ex) || KEYPULSE(key_ex_nxt))
				apr->extpulse |= EXT_KEY_MANUAL;
			if(KEYPULSE(key_inst_stop))
				apr->extpulse |= EXT_KEY_INST_STOP;
			if(ptr){
				if(KEYPULSE(key_rd_on))
					ptr_setmotor(ptr, 1);
				if(KEYPULSE(key_rd_off))
					ptr_setmotor(ptr, 0);
			}
		}

		/* Send lights */
		// TODO: only send changes
		n = 0;
		switch(sw){
		case 0:
			buf[n++] = '0';
			buf[n++] = apr->ir>>12 & 077 | 0100;
			buf[n++] = apr->ir>>6 & 077 | 0100;
			buf[n++] = apr->ir>>0 & 077 | 0100;
			buf[n++] = '1';
			buf[n++] = apr->mi>>30 & 077 | 0100;
			buf[n++] = apr->mi>>24 & 077 | 0100;
			buf[n++] = apr->mi>>18 & 077 | 0100;
			buf[n++] = '2';
			buf[n++] = apr->mi>>12 & 077 | 0100;
			buf[n++] = apr->mi>>6 & 077 | 0100;
			buf[n++] = apr->mi>>0 & 077 | 0100;
			buf[n++] = '3';
			buf[n++] = apr->pc>>12 & 077 | 0100;
			buf[n++] = apr->pc>>6 & 077 | 0100;
			buf[n++] = apr->pc>>0 & 077 | 0100;
			break;
		case 1:
			buf[n++] = '4';
			buf[n++] = apr->ma>>12 & 077 | 0100;
			buf[n++] = apr->ma>>6 & 077 | 0100;
			buf[n++] = apr->ma>>0 & 077 | 0100;
			buf[n++] = '5';
			buf[n++] = apr->run<<5 | apr->pih>>3 | 0100;
			buf[n++] = (apr->pih&7)<<3 | 0100;
			buf[n++] = 0100;
			buf[n++] = '6';
			buf[n++] = apr->mc_stop<<5 | apr->pir>>3 | 0100;
			buf[n++] = (apr->pir&7)<<3 | 0100;
			buf[n++] = 0100;
			buf[n++] = '7';
			buf[n++] = apr->pi_active<<5 | apr->pio>>3 | 0100;
			buf[n++] = (apr->pio&7)<<3 | 0100;
			buf[n++] = 0100;
			break;
		}
		if(apr->sw_power == 0){
			buf[1] = 0;
			buf[2] = 0;
			buf[3] = 0;
			buf[5] = 0;
			buf[6] = 0;
			buf[7] = 0;
			buf[9] = 0;
			buf[10] = 0;
			buf[11] = 0;
			buf[13] = 0;
			buf[14] = 0;
			buf[15] = 0;
		}
		sw = !sw;
		writen(fd, buf, n);
	}
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-t] [-d debugfile] [-p paneltty]\n", argv0);
	exit(1);
}

int
threadmain(int argc, char *argv[])
{
	const char *outfile;
	const char *panelfile;
	int fd;
	Channel *cmdchans[2];
	Task t;

	Apr *apr;
	Ptr *ptr;

	outfile = "/dev/null";
	panelfile = nil;
	ARGBEGIN{
	case 't':
		dotrace = 1;
		break;
	case 'd':
		outfile = EARGF(usage());
		break;
	case 'p':
		panelfile = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;

	if(debugfp = fopen(outfile, "w"), debugfp == nil){
		fprintf(stderr, "Can't open %s\n", outfile);
		exit(1);
	}

	rtcchan = chancreate(sizeof(RtcMsg), 20);

	if(dofile("init.ini"))
		defaultconfig();
	apr = (Apr*)getdevice("apr");
	ptr = (Ptr*)getdevice("ptr");

	if(apr == nil || ptr == nil)
		err("need APR, PTR");

	cmdchans[0] = chancreate(sizeof(char*), 1);
	cmdchans[1] = chancreate(sizeof(void*), 1);
	t = (Task){ nil, readcmdchan, cmdchans, 10, 0 };
	addtask(t);
	threadcreate(simthread, nil);
	threadcreate(cmdthread, cmdchans);
	threadcreate(rtcthread, nil);

	if(panelfile){
		fd = open(panelfile, O_RDWR);
		if(fd < 0)
			err("Couldn't open %s", panelfile);
	}else{
		/* Just testing... */
		fd = dial("localhost", 2000);
		if(fd < 0)
			err("can't connect to panel");
	}

	talkserial(fd, apr, ptr);

	return 0;
}
