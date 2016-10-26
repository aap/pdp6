#include "pdp6.h"
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <pthread.h>    
#include <poll.h>

static void
recalc_tty_req(Tty *tty)
{
	u8 req;
	IOBus *bus;
	bus = tty->bus;
	req = tty->tto_flag || tty->tti_flag ? tty->pia : 0;
	if(req != bus->dev[TTY].req){
		bus->dev[TTY].req = req;
		recalc_req(bus);
	}
}

static void*
ttythread(void *dev)
{
	int sockfd, newsockfd, portno, clilen;
	char buf;
	struct sockaddr_in serv_addr, cli_addr;
	int n;
	Tty *tty;

	tty = dev;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("error: socket");
		exit(1);
	}
	memset(&serv_addr, 0, sizeof(serv_addr));
	portno = 6666;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		perror("error: bind");
		exit(1);
	}
	listen(sockfd,5);
	clilen = sizeof(cli_addr);

	while(newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen)){
		printf("TTY attached\n");
		tty->fd = newsockfd;
		while(n = read(tty->fd, &buf, 1), n > 0){
			tty->tti_busy = 1;
			//fprintf(stderr, "(got.%c)", buf);
			tty->tti = buf|0200;
			tty->tti_busy = 0;
			tty->tti_flag = 1;
			recalc_tty_req(tty);
		}
		tty->fd = -1;
		close(newsockfd);
	}
	if(newsockfd < 0){
		perror("error: accept");
		exit(1);
	}
	return nil;
}

static void
wake_tty(void *dev)
{
	Tty *tty;
	IOBus *bus;

	tty = dev;
	bus = tty->bus;
	if(IOB_RESET){
		tty->pia = 0;
		tty->tto_busy = 0;
		tty->tto_flag = 0;
		tty->tto = 0;
		tty->tti_busy = 0;
		tty->tti_flag = 0;
		tty->tti = 0;
	}
	if(bus->devcode == TTY){
		if(IOB_STATUS){
			if(tty->tti_busy) bus->c12 |= F29;
			if(tty->tti_flag) bus->c12 |= F30;
			if(tty->tto_busy) bus->c12 |= F31;
			if(tty->tto_flag) bus->c12 |= F32;
			bus->c12 |= tty->pia & 7;
		}
		if(IOB_DATAI){
			bus->c12 |= tty->tti;
			tty->tti_flag = 0;
		}
		if(IOB_CONO_CLEAR)
			tty->pia = 0;
		if(IOB_CONO_SET){
			if(bus->c12 & F25) tty->tti_busy = 0;
			if(bus->c12 & F26) tty->tti_flag = 0;
			if(bus->c12 & F27) tty->tto_busy = 0;
			if(bus->c12 & F28) tty->tto_flag = 0;
			if(bus->c12 & F29) tty->tti_busy = 1;
			if(bus->c12 & F30) tty->tti_flag = 1;
			if(bus->c12 & F31) tty->tto_busy = 1;
			if(bus->c12 & F32) tty->tto_flag = 1;
			tty->pia |= bus->c12 & 7;
		}
		if(IOB_DATAO_CLEAR){
			tty->tto = 0;
			tty->tto_busy = 1;
			tty->tto_flag = 0;
		}
		if(IOB_DATAO_SET){
			tty->tto = bus->c12 & 0377;
			if(tty->fd >= 0){
				tty->tto &= ~0200;
				write(tty->fd, &tty->tto, 1);
			}
			// TTO DONE
			tty->tto_busy = 0;
			tty->tto_flag = 1;
		}
	}
	recalc_tty_req(tty);
}

void
inittty(Tty *tty, IOBus *bus)
{
	pthread_t thread_id;
	tty->fd = -1;
	tty->bus = bus;
	bus->dev[TTY] = (Busdev){ tty, wake_tty, 0 };
	pthread_create(&thread_id, nil, ttythread, tty);
}
