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

void
dofile(const char *path)
{
	FILE *f;
	if(f = fopen(path, "r"), f == nil){
		printf("Couldn't open file %s\n", path);
		return;
	}
	cli(f, nil);
	fclose(f);
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
