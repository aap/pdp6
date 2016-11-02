/*
 * Establish a connection with a TCP socket and set terminal into raw mode.
 * Exit with ^].
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include "../src/args.h"

char *argv0;
struct termios tiosaved;

int
raw(void)
{
	struct termios tio;
	if(tcgetattr(0, &tio) < 0) return -1;
	tiosaved = tio;
	cfmakeraw(&tio);
	if(tcsetattr(0, TCSAFLUSH, &tio) < 0) return -1;
	return 0;
}

int
reset(void)
{
	if(tcsetattr(0, TCSAFLUSH, &tiosaved) < 0) return -1;
	return 0;
}

int
opentcp(const char *host, int port)
{
	int sockfd;
	struct sockaddr_in serv_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("ERROR opening socket");
		exit(1);
	}
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(host);
	serv_addr.sin_port = htons(port);
	if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		perror("ERROR on connect");
		exit(1);
	}
	return sockfd;
}

void
readwrite(int fd)
{
	int n;
	struct pollfd pfd[2];
	char c;

	pfd[0].fd = fd;
	pfd[0].events = POLLIN;
	pfd[1].fd = 0;
	pfd[1].events = POLLIN;
	while(pfd[0].fd != -1){
		n = poll(pfd, 2, -1);
		if(n < 0){
			perror("error poll");
			goto out;
		}
		if(n == 0)
			goto out;
		/* read from socket, write to stdout */
		if(pfd[0].revents & POLLIN){
			if(n = read(fd, &c, 1), n < 0)
				goto out;
			if(n == 0){
				shutdown(fd, SHUT_RD);
				pfd[0].fd = -1;
				pfd[0].events = 0;
			}else
				write(1, &c, 1);
		}
		/* read from stdin, write to socket */
		if(pfd[1].revents & POLLIN){
			if(n = read(0, &c, 1), n < 0)
				goto out;
			if(n == 0){
				shutdown(fd, SHUT_WR);
				pfd[1].fd = -1;
				pfd[1].events = 0;
			}else{
				if(c == 035)
					goto out;
				write(fd, &c, 1);
			}
		}
	}
out:
	close(fd);
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-h host] [-p port]\n", argv0);
	exit(1);
}

int clear;
int port = 6666;
char const *host = "127.0.0.1";

int
main(int argc, char *argv[])
{
	int fd;

	ARGBEGIN{
	case 'c':
		clear = 1;
		break;
	case 'p':
		port = atoi(EARGF(usage()));
		break;
	case 'h':
		host = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;

	fd = opentcp(host, port);

	if(clear)
		printf("\033[H\033[J");	// clear screen

	fflush(stdout);
	if(raw())
		return 1;

	readwrite(fd);

	reset();
	putchar('\n');
	return 0;
}
