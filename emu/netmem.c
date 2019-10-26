#include "pdp6.h"
#include <unistd.h>

char *netmem_ident = NETMEM_IDENT;

enum
{
	WRRQ = 1,
	RDRQ = 2,
	ACK  = 3,
	ERR  = 4,
};

static void
netmemcycle(void *dev)
{
	Netmem *nm;
	nm = (Netmem*)dev;
	u16 len;
	word a, d, *p;
	int busy;

	if(nm->fd < 0)
		return;

	if(!nm->waiting){
		if(!hasinput(nm->fd))
			return;

		if(readn(nm->fd, nm->buf, 2)){
			fprintf(stderr, "netmem fd closed\n");
			nm->fd = -1;
			return;
		}
		len = nm->buf[0]<<8 | nm->buf[1];
		if(len > 9){
			fprintf(stderr, "netmem botch(%d), closing\n", len);
			close(nm->fd);
			nm->fd = -1;
			return;
		}
		memset(nm->buf, 0, sizeof(nm->buf));
		readn(nm->fd, nm->buf, len);
	}
	nm->waiting = 0;

	a = nm->buf[1] | nm->buf[2]<<8 | nm->buf[3]<<16;
	d = nm->buf[4] | nm->buf[5]<<8 | nm->buf[6]<<16 |
		(word)nm->buf[7]<<24 | (word)nm->buf[8]<<32;
	a &= 0777777;
	d &= 0777777777777;

	switch(nm->buf[0]){
	case WRRQ:
		p = getmemref(&nm->apr->membus, a, 0, &busy);
		if(p == nil) goto err;
		if(busy) goto wait;
		*p = d;
		printf("write %06lo %012lo\n", a, d);
		nm->buf[0] = 0;
		nm->buf[1] = 1;
		nm->buf[2] = ACK;
		writen(nm->fd, nm->buf, nm->buf[1]+2);
		break;
	case RDRQ:
		p = getmemref(&nm->apr->membus, a, 0, &busy);
		if(p == nil) goto err;
		if(busy) goto wait;
		d = *p;
		printf("read %06lo %012lo\n", a, d);
		nm->buf[0] = 0;
		nm->buf[1] = 6;
		nm->buf[2] = ACK;
		nm->buf[3] = d;
		nm->buf[4] = d>>8;
		nm->buf[5] = d>>16;
		nm->buf[6] = d>>24;
		nm->buf[7] = d>>32;
		writen(nm->fd, nm->buf, nm->buf[1]+2);
		break;
	default:
		fprintf(stderr, "unknown netmem message %d\n", nm->buf[0]);
		break;
	}
	return;
err:
	printf("error address %06lo\n", a);
	nm->buf[0] = 1;
	nm->buf[1] = 1;
	nm->buf[2] = ERR;
	writen(nm->fd, nm->buf, nm->buf[0]+2);
	return;
wait:
	nm->waiting = 1;
}

Device*
makenetmem(int argc, char *argv[])
{
	const char *host;
	int port;
	Netmem *nm;
	Device *apr;
	Task t;

	nm = malloc(sizeof(Netmem));
	memset(nm, 0, sizeof(Netmem));
	nm->dev.type = netmem_ident;
	nm->dev.name = "";

	// TODO: don't hardcode;
	apr = getdevice("apr");
	assert(apr);
	assert(apr->type == apr_ident);
	nm->apr = (Apr*)apr;

	if(argc > 0)
		host = argv[0];
	else
		host = "localhost";
	if(argc > 1)
		port = atoi(argv[1]);
	else
		port = 10006;

	printf("connecting to %s %d\n", host, port);

	nm->fd = dial(host, port);
	if(nm->fd < 0)
		printf("couldn't connect\n");
	printf("netmem fd: %d\n", nm->fd);

	t = (Task){ nil, netmemcycle, nm, 50, 0 };
	addtask(t);

	return &nm->dev;
}
