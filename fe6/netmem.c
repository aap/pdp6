#include "fe.h"
#include <unistd.h>

int netmemfd = -1;

enum
{
	WRRQ = 1,
	RDRQ = 2,
	ACK  = 3,
	ERR  = 4,
};

void*
netmemthread(void *arg)
{
	u16 len;
	word a, d;
	u8 buf[9];

	while(readn(netmemfd, buf, 2) == 0){
		len = buf[0]<<8 | buf[1];
		if(len > 9){
			fprintf(stderr, "netmem botch(%d), closing\n", len);
			close(netmemfd);
			netmemfd = -1;
			return nil;
		}
		memset(buf, 0, sizeof(buf));
		readn(netmemfd, buf, len);

		a = buf[1] | buf[2]<<8 | buf[3]<<16;
		d = buf[4] | buf[5]<<8 | buf[6]<<16 |
			(word)buf[7]<<24 | (word)buf[8]<<32;
		a &= 0777777;
		d &= 0777777777777;

		switch(buf[0]){
		case WRRQ:
			deposit(a, d);
	printf("write %06lo %012lo\r\n", a, d);
	fflush(stdout);
			buf[0] = 0;
			buf[1] = 1;
			buf[2] = ACK;
			writen(netmemfd, buf, buf[1]+2);
			break;
		case RDRQ:
			d = examine(a);
	printf("read %06lo %012lo\r\n", a, d);
	fflush(stdout);
			buf[0] = 0;
			buf[1] = 6;
			buf[2] = ACK;
			buf[3] = d;
			buf[4] = d>>8;
			buf[5] = d>>16;
			buf[6] = d>>24;
			buf[7] = d>>32;
			writen(netmemfd, buf, buf[1]+2);
			break;
		default:
			fprintf(stderr, "unknown netmem message %d\n", buf[0]);
			break;
		}
	}
	fprintf(stderr, "netmem fd closed\n");
	netmemfd = -1;
	return nil;
}

void
initnetmem(const char *host, int port)
{
	netmemfd = dial(host, port);
	if(netmemfd >= 0){
		printf("netmem connected\n");
		threadcreate(netmemthread, nil);
	}
}

