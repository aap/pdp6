/* ASR33 "emulator" */

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
	cfsetispeed(&tio, B110);
	cfsetospeed(&tio, B110);
	cfmakeraw(&tio);
	tio.c_cflag |= CSTOPB;
	if(tcsetattr(fd, TCSAFLUSH, &tio) < 0) return -1;
	return 0;
}

int
reset(int fd)
{
	if(tcsetattr(0, TCSAFLUSH, &tiosaved) < 0) return -1;
	return 0;
}

/* tty1: unix tty
 * tty2: serial port */
void
readwrite(int tty1in, int tty1out, int tty2in, int tty2out)
{
	int n;
	struct pollfd pfd[2];
	char c;

	pfd[0].fd = tty2in;
	pfd[0].events = POLLIN;
	pfd[1].fd = tty1in;
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
			return;
		/* read from tty2, write to tty1 */
		if(pfd[0].revents & POLLIN){
			if(n = read(tty2in, &c, 1), n <= 0)
				return;
			else{
				c &= 0177;
//if(c == '\a') c = '.';
				if(c < 0140)
					write(tty1out, &c, 1);
			}
		}
		/* read from tty1, write to tty2 */
		if(pfd[1].revents & POLLIN){
			if(n = read(tty1in, &c, 1), n <= 0)
				return;
			else{
				if(c == 035)
					return;
				/* map character reasonably */
				if(c == 033) c = 0175;	// esc to altmode
				if(c >= 'a' && c <= 'z')	// to upper
					c &= ~040;
				c |= 0200;	// needed?
				write(tty2out, &c, 1);
			}
		}
	}
}

int
main(int argc, char *argv[])
{
	int fd;

	if(argc < 2)
		return 1;

	fd = open(argv[1], O_RDWR);
	if(fd < 0)
		return 1;

	raw(fd);
	raw(0);

	readwrite(0, 1, fd, fd);
	printf("\r\n\r\n");
	close(fd);

	reset(0);

	return 0;
}
