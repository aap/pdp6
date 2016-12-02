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

uchar*
putword(uchar *p, word w)
{
	p[5] = w & 077;
	p[4] = (w >>  6) & 077;
	p[3] = (w >> 12) & 077;
	p[2] = (w >> 18) & 077;
	p[1] = (w >> 24) & 077;
	p[0] = (w >> 30) & 077;
	return p+6;
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

	p = msg;
	p = puthalfword(p, 0000000);
	p = putword(p, 0111222333444);
	p = putword(p, 0111111111111);
	p = putword(p, 0222222222222);
	p = putword(p, 0333333333333);
	p = putword(p, 0444444444444);
	p = putword(p, 0555555555555);
	p = putword(p, 0666666666666);
	p = putword(p, 0777777777777);
printf("%d\n", p-msg);
	write(fd, msg, p-msg);

	for(i = 0; i < p-msg; i++)
		printf("%03o ", msg[i]);
	printf("\n");

	close(fd);
	
	return 0;
}
