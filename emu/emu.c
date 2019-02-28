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
