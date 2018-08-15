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
	u8 buf[9], len;
	word a, d, *p;

	if(nm->fd < 0)
		return;

	if(!hasinput(nm->fd))
		return;

	if(readn(nm->fd, &len, 1)){
		fprintf(stderr, "netmem fd closed\n");
		nm->fd = -1;
		return;
	}
	if(len > 9){
		fprintf(stderr, "netmem botch, closing\n");
		close(nm->fd);
		nm->fd = -1;
	}
	memset(buf, 0, sizeof(buf));
	readn(nm->fd, buf, len);

	a = buf[1] | buf[2]<<8 | buf[3]<<16;
	d = buf[4] | buf[5]<<8 | buf[6]<<16 |
		(word)buf[7]<<24 | (word)buf[8]<<32;
	a &= 0777777;
	d &= 0777777777777;

	switch(buf[0]){
	case WRRQ:
		p = getmemref(&nm->apr->membus, a, 0);
		if(p == nil) goto err;
		*p = d;
		printf("write %06lo %012lo\n", a, d);
		buf[0] = 1;
		buf[1] = ACK;
		writen(nm->fd, buf, buf[0]+1);
		break;
	case RDRQ:
		p = getmemref(&nm->apr->membus, a, 0);
		if(p == nil) goto err;
		d = *p;
		printf("read %06lo %012lo\n", a, d);
		buf[0] = 6;
		buf[1] = ACK;
		buf[2] = d;
		buf[3] = d>>8;
		buf[4] = d>>16;
		buf[5] = d>>24;
		buf[6] = d>>32;
		writen(nm->fd, buf, buf[0]+1);
		break;
	default:
		fprintf(stderr, "unknown netmem message %d\n", buf[0]);
		break;
	}
	return;
err:
	printf("error address %06lo\n", a);
	buf[0] = 1;
	buf[1] = ERR;
	writen(nm->fd, buf, buf[0]+1);
}

Device*
makenetmem(int argc, char *argv[])
{
	const char *host;
	int port;
	Netmem *nm;
	Device *apr;
	Thread th;

	nm = malloc(sizeof(Netmem));
	memset(nm, 0, sizeof(Netmem));
	nm->dev.type = netmem_ident;
	nm->dev.name = "";
	nm->dev.attach = nil;
	nm->dev.ioconnect = nil;
	nm->dev.next = nil;

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

	th = (Thread){ nil, netmemcycle, nm, 50, 0 };
	addthread(th);

	return &nm->dev;
}
