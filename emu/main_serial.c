#include "pdp6.h"
#include <stdarg.h>
#include <pthread.h>
#include "args.h"

char *argv0;

FILE *debugfp;
int dotrace;

void
trace(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	if(dotrace){
		fprintf(debugfp, "  ");
		vfprintf(debugfp, fmt, ap);
	}
	va_end(ap);
}

void
debug(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(debugfp, fmt, ap);
	va_end(ap);
}

void
err(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "error: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(1);
}

// TODO: get rid of this
void updatepanel(Apr *apr) {}

u32
getms(void)
{
	// TODO
	return 0;
}

#define KEYPULSE(k) (apr->k && !oldapr.k)

void
talkserial(int fd, Apr *apr, Ptr *ptr)
{
	Apr oldapr;
	char buf[32];
	int n;

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
		writen(fd, buf, n);
	}
}

Thread *threads;

void
addthread(Thread th)
{
	Thread *p;
	p = malloc(sizeof(Thread));
	*p = th;
	p->next = threads;
	threads = p;
}

void*
simthread(void *p)
{
	Thread *th;

	printf("[simthread] start\n");
	for(;;)
		for(th = threads; th; th = th->next){
			th->cnt++;
			if(th->cnt == th->freq){
				th->cnt = 0;
				th->f(th->arg);
			}
		}
	err("can't happen");
	return nil;
}

Device *devlist;

void
adddevice(Device *dev)
{
	dev->next = devlist;
	devlist = dev;
};

Device*
getdevice(const char *name)
{
	Device *dev;
	for(dev = devlist; dev; dev = dev->next)
		if(strcmp(dev->name, name) == 0)
			return dev;
	return nil;
}

void
showdevices(void)
{
	Device *dev;
	for(dev = devlist; dev; dev = dev->next)
		printf("%s %s\n", dev->name, dev->type);
}


void
dofile(const char *path)
{
	FILE *f;
	if(f = fopen(path, "r"), f == nil){
		printf("Couldn't open file %s\n", path);
		return;
	}
	cli(f);
	fclose(f);
}

void
quit(int code)
{
	int i;
	Device *dev;
	Membus *mbus;

	/* Detach all files */
	for(dev = devlist; dev; dev = dev->next)
		if(dev->detach)
			dev->detach(dev);

	/* Sync memory to disk */
	for(dev = devlist; dev; dev = dev->next){
		if(strcmp(dev->type, apr_ident) != 0)
			continue;

		mbus = &((Apr*)dev)->membus;
		for(i = 0; i < 16; i++)
			if(mbus->cmem[i] && mbus->cmem[i]->sync)
				mbus->cmem[i]->sync(mbus->cmem[i]);
	}

//	putchar('\n');
	printf("\nquitting\n");
	exit(code);
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-t] [-d debugfile]\n", argv0);
	exit(1);
}

int
main(int argc, char *argv[])
{
	pthread_t cmd_thread, sim_thread;
	const char *outfile;
	int fd;

	Apr *apr;
	Ptr *ptr;

	outfile = "/dev/null";
	ARGBEGIN{
	case 't':
		dotrace = 1;
		break;
	case 'd':
		outfile = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;
	if(debugfp = fopen(outfile, "w"), debugfp == nil){
		fprintf(stderr, "Can't open %s\n", outfile);
		exit(1);
	}


	dofile("init.ini");
	apr = (Apr*)getdevice("apr");
	ptr = (Ptr*)getdevice("ptr");

	pthread_create(&sim_thread, nil, simthread, apr);
	pthread_create(&cmd_thread, nil, cmdthread, nil);

	fd = dial("localhost", 2000);
	if(fd < 0)
		err("can't connect to panel");
	talkserial(fd, apr, ptr);

	return 0;
}
