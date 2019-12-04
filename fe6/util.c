#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <netdb.h>


void
strtolower(char *s)
{
	for(; *s != '\0'; s++)
		*s = tolower(*s);
}

int
hasinput(int fd)
{
	fd_set fds;
	struct timeval timeout;

	if(fd < 0) return 0;

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	return select(fd+1, &fds, NULL, NULL, &timeout) > 0;
}

int
writen(int fd, void *data, int n)
{
	int m;

	while(n > 0){
		m = write(fd, data, n);
		if(m == -1)
			return -1;
		data += m;
		n -= m;
	}
	return 0;
}

int
readn(int fd, void *data, int n)
{
	int m;

	while(n > 0){
		m = read(fd, data, n);
		if(m <= 0)
			return -1;
		data += m;
		n -= m;
	}
	return 0;
}

int
dial(const char *host, int port)
{
	char portstr[32];
	int sockfd;
	struct addrinfo *result, *rp, hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	snprintf(portstr, 32, "%d", port);
	if(getaddrinfo(host, portstr, &hints, &result)){
		perror("error: getaddrinfo");
		return -1;
	}

	for(rp = result; rp; rp = rp->ai_next){
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(sockfd < 0)
			continue;
		if(connect(sockfd, rp->ai_addr, rp->ai_addrlen) >= 0)
			goto win;
		close(sockfd);
	}
	freeaddrinfo(result);
	perror("error");
	return -1;

win:
	freeaddrinfo(result);
	return sockfd;
}

void
serve(int port, void (*handlecon)(int, void*), void *arg)
{
	int sockfd, confd;
	socklen_t len;
	struct sockaddr_in server, client;
	int x;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("error: socket");
		return;
	}

	x = 1;
	setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&x, sizeof x);
	
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);
	if(bind(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0){
		perror("error: bind");
		return;
	}
	listen(sockfd, 5);
	len = sizeof(client);
	while(confd = accept(sockfd, (struct sockaddr*)&client, &len),
	      confd >= 0)
		handlecon(confd, arg);
	perror("error: accept");
	return;
}

int
serve1(int port)
{
	int sockfd, confd;
	socklen_t len;
	struct sockaddr_in server, client;
	int x;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("error: socket");
		return -1;
	}

	x = 1;
	setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&x, sizeof x);
	
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);
	if(bind(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0){
		perror("error: bind");
		return -1;
	}
	listen(sockfd, 5);
	len = sizeof(client);
	while(confd = accept(sockfd, (struct sockaddr*)&client, &len),
	      confd >= 0)
		return confd;
	perror("error: accept");
	return -1;
}

void
nodelay(int fd)
{
	int flag = 1;
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
}
