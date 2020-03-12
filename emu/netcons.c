#include "pdp6.h"
#include <unistd.h>

char *netcons_ident = NETCONS_IDENT;

enum
{
	WRRQ = 1,
	RDRQ = 2,
	ACK  = 3,
	ERR  = 4,
};

#include "mmregs.h"

static word
memread(Apr *apr, u32 addr)
{
	word *p;
	if(addr < 020)
		p = getmemref(&apr->membus, addr, 1, nil);
	else
		p = getmemref(&apr->membus, addr&RT, 0, nil);
	if(p == nil) return 0;
	return *p;
}

static void
memwrite(Apr *apr, u32 addr, word w)
{
	word *p;
	if(addr < 020)
		p = getmemref(&apr->membus, addr, 1, nil);
	else
		p = getmemref(&apr->membus, addr&RT, 0, nil);
	if(p == nil) return;
	*p = w;
}

static u32 memif_addr;
static word memif_word;

static void
writereg(Apr *apr, u32 addr, u32 data)
{
//	printf("writing %o to %o\n", data, addr);
	if(addr < MM_APR_BASE){
		switch(addr){
		case MM_MEMIF_ADDR:
			memif_addr = data;
			break;
		case MM_MEMIF_LOW:
			memif_word = memif_word&LT | data&RT;
			break;
		case MM_MEMIF_HIGH:
			memif_word = ((word)data<<18)&LT | memif_word&RT;
			memwrite(apr, memif_addr, memif_word);
			break;
		}
	}else if(addr <= MM_APR_PI)
		writeconsreg(apr, addr, data);
}

static u32
readreg(Apr *apr, u32 addr)
{
//	printf("reading from %o\n", addr);
	if(addr < MM_APR_BASE){
		switch(addr){
		case MM_MEMIF_LOW:
			return memif_word&RT;
			break;
		case MM_MEMIF_HIGH:
			memif_word = memread(apr, memif_addr);
			return (memif_word>>18)&RT;
			break;
		}
	}else if(addr <= MM_APR_PI)
		return readconsreg(apr, addr);
	return 0;
}

static void
netconscycle(void *dev)
{
	Netcons *nc;
	nc = (Netcons*)dev;
	u16 len;
	u32 a, d;

	if(nc->fd < 0)
		return;


	if(!hasinput(nc->fd))
		return;

	if(readn(nc->fd, nc->buf, 2)){
		fprintf(stderr, "netcons fd closed\n");
		nc->fd = -1;
		return;
	}
	len = nc->buf[0]<<8 | nc->buf[1];
	if(len > 9){
		fprintf(stderr, "netcons botch(%d), closing\n", len);
		close(nc->fd);
		nc->fd = -1;
		return;
	}
	memset(nc->buf, 0, sizeof(nc->buf));
	readn(nc->fd, nc->buf, len);


	a = nc->buf[1] | nc->buf[2]<<8 | nc->buf[3]<<16 | nc->buf[4] << 24;
	d = nc->buf[5] | nc->buf[6]<<8 | nc->buf[7]<<16 | nc->buf[8] << 24;

	switch(nc->buf[0]){
	case WRRQ:
		writereg(nc->apr, a, d);
		nc->buf[0] = 0;
		nc->buf[1] = 1;
		nc->buf[2] = ACK;
		writen(nc->fd, nc->buf, nc->buf[1]+2);
		break;
	case RDRQ:
		d = readreg(nc->apr, a);
	//	printf("got %lo\n", d);
		nc->buf[0] = 0;
		nc->buf[1] = 5;
		nc->buf[2] = ACK;
		nc->buf[3] = d;
		nc->buf[4] = d>>8;
		nc->buf[5] = d>>16;
		nc->buf[6] = d>>24;
		writen(nc->fd, nc->buf, nc->buf[1]+2);
		break;
	default:
		fprintf(stderr, "unknown netcons message %d\n", nc->buf[0]);
		break;
	}
	return;
}

Device*
makenetcons(int argc, char *argv[])
{
	const char *host;
	int port;
	Netcons *nc;
	Device *apr;
	Task t;

	nc = malloc(sizeof(Netcons));
	memset(nc, 0, sizeof(Netcons));
	nc->dev.type = netcons_ident;
	nc->dev.name = "";

	// TODO: don't hardcode;
	apr = getdevice("apr");
	assert(apr);
	assert(apr->type == apr_ident);
	nc->apr = (Apr*)apr;

	if(argc > 0)
		host = argv[0];
	else
		host = "localhost";
	if(argc > 1)
		port = atoi(argv[1]);
	else
		port = 10007;

	printf("connecting to %s %d\n", host, port);

	nc->fd = dial(host, port);
	if(nc->fd < 0)
		printf("couldn't connect\n");
	printf("netcons fd: %d\n", nc->fd);

	t = (Task){ nil, netconscycle, nc, 50, 0 };
	addtask(t);

	return &nc->dev;
}
