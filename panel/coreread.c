#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>

typedef uint64_t word;
typedef unsigned char uchar;

void
err(char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        fprintf(stderr, "error: ");
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
        va_end(ap);
        exit(1);
}

uchar*
puthalfword(uchar *p, word w)
{
	p[2] = w & 077;
	p[1] = (w >>  6) & 077;
	p[0] = (w >> 12) & 077;
	return p+3;
}

int
main()
{
	int i;
	uchar msg[3+8*6], *p;
	int fd;
	if(fd = open("/dev/i2c-1", O_RDWR), fd < 0)
		err("couldn't open file");
	if(ioctl(fd, I2C_SLAVE, 0x21) < 0)
		err("couldn't select device");

	puthalfword(msg, 0000000);
	write(fd, msg, 3);

	read(fd, msg, 48+2);

	for(i = 0; i < 48; i++)
		printf("%03o ", msg[i+2]);
	printf("\n");

	close(fd);
	
	return 0;
}
