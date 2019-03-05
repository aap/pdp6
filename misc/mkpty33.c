/* Creates a pty and connects it to the process tty.
 * As an optional argument it takes a filename and symlinks /dev/pts/N. */ 

/* Pretend to be an ASR33 of sorts */

#define _XOPEN_SOURCE 600	/* for ptys */
#define _DEFAULT_SOURCE		/* for cfmakeraw */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <time.h>

struct termios tiosaved;

int
raw(int fd)
{
	struct termios tio;
	if(tcgetattr(fd, &tio) < 0) return -1;
	tiosaved = tio;
	cfmakeraw(&tio);
	if(tcsetattr(fd, TCSAFLUSH, &tio) < 0) return -1;
	return 0;
}

int
reset(int fd)
{
	if(tcsetattr(0, TCSAFLUSH, &tiosaved) < 0) return -1;
	return 0;
}

#define BAUD 30

struct timespec slp = { 0, 1000*1000*1000 / BAUD };
struct timespec hupslp = { 0, 100*1000*1000 };

//#define SLEEP nanosleep(&slp, NULL)
#define SLEEP

void
readwrite(int ttyin, int ttyout, int ptyin, int ptyout)
{
	int n;
	struct pollfd pfd[2];
	char c;

	pfd[0].fd = ptyin;
	pfd[0].events = POLLIN;
	pfd[1].fd = ttyin;
	pfd[1].events = POLLIN;
	while(pfd[0].fd != -1){
		n = poll(pfd, 2, -1);
		if(n < 0){
			perror("error poll");
			return;
		}
		if(n == 0)
			return;
		if(pfd[0].revents & POLLHUP)
			nanosleep(&hupslp, NULL);
		/* read from pty, write to tty */
		if(pfd[0].revents & POLLIN){
			if(n = read(ptyin, &c, 1), n <= 0)
				return;
			else{
				c &= 0177;	// is this needed?
//if(c == '\a') c = '.';
				if(c < 0140)
					write(ttyout, &c, 1);
				SLEEP;
			}
		}
		/* read from tty, write to pty */
		if(pfd[1].revents & POLLIN){
			if(n = read(ttyin, &c, 1), n <= 0)
				return;
			else{
				c &= 0177;	// needed?
				if(c == 035)
					return;
				/* map character reasonably */
				if(c == 033) c = 0175;	// esc to altmode
				if(c >= 'a' && c <= 'z')	// to upper
					c &= ~040;
				c |= 0200;	// needed?
				write(ptyout, &c, 1);
				SLEEP;
			}
		}
	}
}

int
main(int argc, char *argv[])
{
	int fd;
	int slv;
	const char *ptylink;

	ptylink = NULL;
	if(argc > 1)
		ptylink = argv[1];

	fd = posix_openpt(O_RDWR);
	if(fd < 0)
		return 1;
	if(grantpt(fd) < 0)
		return 2;
	if(unlockpt(fd) < 0)
		return 3;

	if(ptylink){
		unlink(ptylink);
		if(symlink(ptsname(fd), ptylink) < 0)
			fprintf(stderr, "Error: %s\n", strerror(errno));
	}

	printf("%s\n", ptsname(fd));

	slv = open(ptsname(fd), O_RDWR);
	if(slv < 0)
		return 4;
	raw(slv);
	close(slv);

	raw(0);

	readwrite(0, 1, fd, fd);
	close(fd);

	reset(0);

	if(ptylink)
		unlink(ptylink);

	return 0;
}
