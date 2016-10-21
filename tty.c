#include "pdp6.h"
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <pthread.h>    
#include <poll.h>

/*
 * This device is not accurately modeled after the schematics.
 */

#define TTY (0120>>2)

typedef struct Tty Tty;
struct Tty
{
	uchar tto, tti;
	bool tto_busy, tto_flag;
	bool tti_busy, tti_flag;
	int pia;
	int fd;
};
Tty tty;

void
recalc_tty_req(void)
{
	u8 req;
	req = tty.tto_flag || tty.tti_flag ? tty.pia : 0;
	if(req != ioreq[TTY]){
		ioreq[TTY] = req;
		recalc_req();
	}
}

void*
ttythread(void *arg)
{
	int sockfd, newsockfd, portno, clilen;
	char buf;
	struct sockaddr_in serv_addr, cli_addr;
	int n;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("ERROR opening socket");
		exit(1);
	}
	memset(&serv_addr, 0, sizeof(serv_addr));
	portno = 6666;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		perror("ERROR on bind");
		exit(1);
	}
	listen(sockfd,5);
	clilen = sizeof(cli_addr);

	while(newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen)){
		printf("TTY attached\n");
		tty.fd = newsockfd;
		while(n = read(tty.fd, &buf, 1), n > 0){
			tty.tti_busy = 1;
			//fprintf(stderr, "(got.%c)", buf);
			tty.tti = buf|0200;
			tty.tti_busy = 0;
			tty.tti_flag = 1;
			recalc_tty_req();
		}
		tty.fd = -1;
		close(newsockfd);
	}
	if(newsockfd < 0){
		perror("ERROR on accept");
		exit(1);
	}
	return nil;
}

static void
wake_tty(void)
{
	if(IOB_RESET){
		tty.pia = 0;
		tty.tto_busy = 0;
		tty.tto_flag = 0;
		tty.tto = 0;
		tty.tti_busy = 0;
		tty.tti_flag = 0;
		tty.tti = 0;
		ioreq[TTY] = 0;
	}
	if(IOB_STATUS){
		if(tty.tti_busy) iobus0 |= F29;
		if(tty.tti_flag) iobus0 |= F30;
		if(tty.tto_busy) iobus0 |= F31;
		if(tty.tto_flag) iobus0 |= F32;
		iobus0 |= tty.pia & 7;
	}
	if(IOB_DATAI){
		iobus0 |= tty.tti;
		tty.tti_flag = 0;
	}
	if(IOB_CONO_CLEAR)
		tty.pia = 0;
	if(IOB_CONO_SET){
		if(iobus0 & F25) tty.tti_busy = 0;
		if(iobus0 & F26) tty.tti_flag = 0;
		if(iobus0 & F27) tty.tto_busy = 0;
		if(iobus0 & F28) tty.tto_flag = 0;
		if(iobus0 & F29) tty.tti_busy = 1;
		if(iobus0 & F30) tty.tti_flag = 1;
		if(iobus0 & F31) tty.tto_busy = 1;
		if(iobus0 & F32) tty.tto_flag = 1;
		tty.pia |= iobus0 & 7;
	}
	if(IOB_DATAO_CLEAR){
		tty.tto = 0;
		tty.tto_busy = 1;
		tty.tto_flag = 0;
	}
	if(IOB_DATAO_SET){
		tty.tto = iobus0 & 0377;
		if(/*tty.tto & 0200 &&*/ tty.fd >= 0){
			tty.tto &= ~0200;
			write(tty.fd, &tty.tto, 1);
		}
		// TTO DONE
		tty.tto_busy = 0;
		tty.tto_flag = 1;
	}
	recalc_tty_req();
}

void
inittty(void)
{
	pthread_t thread_id;
	tty.fd = -1;
	ioreq[TTY] = 0;
	pthread_create(&thread_id, nil, ttythread, nil);
	iobusmap[TTY] = wake_tty;
}
