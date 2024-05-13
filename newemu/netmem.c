#include "common.h"
#include "pdp6.h"

#include <unistd.h>

extern Word memory[01000000];

static u8*
putu16(u8 *p, u16 n)
{
	*p++ = n>>8;
	*p++ = n;
	return p;
}

static u8*
getu16(u8 *p, u16 *wp)
{
	u16 w;
	w = p[0];
	w <<= 8; w |= p[1];
	*wp = w;
	return p+2;
}

static u8*
putword(u8 *p, u64 w)
{
	*p++ = w;
	*p++ = w>>8;
	*p++ = w>>16;
	*p++ = w>>24;
	*p++ = w>>32;
	return p;
}

static u8*
getword(u8 *p, u64 *wp)
{
	u64 w;
	w = p[4];
	w <<= 8; w |= p[3];
	w <<= 8; w |= p[2];
	w <<= 8; w |= p[1];
	w <<= 8; w |= p[0];
	*wp = w & 0777777777777;
	return p+5;
}

static u8*
getaddr(u8 *p, u32 *wp)
{
	u32 w;
	w = p[2];
	w <<= 8; w |= p[1];
	w <<= 8; w |= p[0];
	*wp = w & 0777777;
	return p+3;
}

#define log(...)

void
handlenetmem(PDP6 *pdp)
{
	static u8 buf[6 + 512*5];
	u8 *p;
	u16 len, n;
	u32 a;
	u64 d;
	enum {
		WRRQ = 1,
		RDRQ,
		ACK,
		ERR,
		RDNRQ,
		WRNRQ,
	};

	if(pdp->mem_busy || pdp->netmem_fd.fd < 0 || !pdp->netmem_fd.ready)
		return;

	if(readn(pdp->netmem_fd.fd, buf, 2))
		return;
	getu16(buf, &len);
	if(len > sizeof(buf)) {
		printf("netmem botch, closing\n");
		closefd(&pdp->netmem_fd);
		return;
	}

	memset(buf, 0, sizeof(buf));
	readn(pdp->netmem_fd.fd, buf, len);

	p = getaddr(&buf[1], &a);

	switch(buf[0]) {
	case WRRQ:
		p = getword(p, &d);
		memory[a] = d;
		log("write %06o %012llo\n", a, d);
		len = 1;
		p = putu16(buf, len);
		*p++ = ACK;
		write(pdp->netmem_fd.fd, buf, len+2);
		break;

	case RDRQ:
		d = memory[a];
		log("read %06o %012llo\n", a, d);
		len = 6;
		p = putu16(buf, len);
		*p++ = ACK;
		p = putword(p, d);
		write(pdp->netmem_fd.fd, buf, len+2);
		break;

	case RDNRQ:
		p = getu16(p, &n);
		assert(n <= 512);

		log("read N %d %06o\n", n, a);
		len = 1 + n*5;
		p = putu16(buf, len);
		*p++ = ACK;
		while(n--) {
			d = memory[a];
//			log("read %06o %012llo\n", a, d);
			p = putword(p, d);
			a = (a+1) & 0777777;
		}

		write(pdp->netmem_fd.fd, buf, len+2);
		break;

	case WRNRQ:
		p = getu16(p, &n);
		assert(n <= 512);

		log("write N %d %06o\n", n, a);
		while(n--) {
			p = getword(p, &d);
			memory[a] = d;
//			log("write %06o %012llo\n", a, d);
			a = (a+1) & 0777777;
		}

		len = 1;
		p = putu16(buf, len);
		*p++ = ACK;
		write(pdp->netmem_fd.fd, buf, len+2);
		break;

	default:
		printf("unknown msg %d\n", buf[0]);
	}

	waitfd(&pdp->netmem_fd);
}
