#include "pdp6.h"

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


Task *tasks;

void
addtask(Task t)
{
	Task *p;
	p = malloc(sizeof(Task));
	*p = t;
	p->next = tasks;
	tasks = p;
}

void*
simthread(void *p)
{
	Task *t;

	threadname("sim");

	printf("[simthread] start\n");
	for(;;)
		for(t = tasks; t; t = t->next){
			t->cnt++;
			if(t->cnt == t->freq){
				t->cnt = 0;
				t->f(t->arg);
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

int
dofile(const char *path)
{
	FILE *f;
	if(f = fopen(path, "r"), f == nil){
		printf("Couldn't open file %s\n", path);
		return 1;
	}
	cli(f, nil);
	fclose(f);
	return 0;
}

/* Wrapper around non-const */
static void
line(const char *cln)
{
	static char ln[160];
	strncpy(ln, cln, 159);
	ln[159] = 0;
	commandline(ln);
}

void
defaultconfig(void)
{
	line("mkdev apr apr166");
	line("mkdev tty tty626");
	line("mkdev ptr ptr760");
	line("mkdev ptp ptp761");
	line("mkdev dc dc136");
	line("mkdev dt0 dt551");
	line("mkdev dx0 dx555");
	line("mkdev fmem fmem162 0");
	line("mkdev mem0 moby");

	line("connectdev dc dt0");
	line("connectdev dt0 dx0 1");
	line("connectio tty apr");
	line("connectio ptr apr");
	line("connectio ptp apr");
	line("connectio dc apr");
	line("connectio dt0 apr");
	line("connectmem fmem 0 apr -1");
	line("connectmem mem0 0 apr 0");
}

/* Task for simulation.
 * Execute commands synchronously. */
void
readcmdchan(void *p)
{
	Channel **c;
	char *line;

	c = p;
	if(channbrecv(c[0], &line) == 1){
		commandline(line);
		free(line);
		chansend(c[1], &line);
	}
}

void
quit(int code)
{
	int i;
	Device *dev;
	Membus *mbus;

	/* Detach all files */
	for(dev = devlist; dev; dev = dev->next)
		if(dev->unmount)
			dev->unmount(dev);

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
