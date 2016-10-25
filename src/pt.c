#include "pdp6.h"
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <pthread.h>    
#include <poll.h>

/* TODO? implement motor delays */

Ptp ptp;
Ptr ptr;

void
recalc_ptp_req(void)
{
	u8 req;
	req = ptp.flag ? ptp.pia : 0;
	if(req != ioreq[PTP]){
		ioreq[PTP] = req;
		recalc_req();
	}
}

void
recalc_ptr_req(void)
{
	u8 req;
	req = ptr.flag ? ptr.pia : 0;
	if(req != ioreq[PTR]){
		ioreq[PTR] = req;
		recalc_req();
	}
}

/* We have to punch after DATAO SET has happened. But BUSY is set by
 * DATAO CLEAR. So we use this to record when SET has happened */
int waitdatao;

void*
ptthread(void *arg)
{
	int c;
	for(;;){
		if(ptp.busy && waitdatao){
			if(ptp.fp){
				if(ptp.b)
					putc((ptp.ptp & 077) | 0200, ptp.fp);
				else
					putc(ptp.ptp, ptp.fp);
				fflush(ptp.fp);
			}
			ptp.busy = 0;
			ptp.flag = 1;
			recalc_ptp_req();
		}

		if(ptr.busy && ptr.motor_on){
			// PTR CLR
			ptr.sr = 0;
			ptr.ptr = 0;
	next:
			if(ptr.fp)
				c = getc(ptr.fp);
			else
				c = 0;
			if(c == EOF)
				c = 0;
			if(!ptr.b || c & 0200){
				// PTR STROBE
				ptr.sr <<= 1;
				ptr.ptr <<= 6;
				ptr.sr |= 1;
				ptr.ptr |= c & 077;
				if(!ptr.b)
					ptr.ptr |= c & 0300;
			}
			if(!ptr.b || ptr.sr & 040){
				ptr.busy = 0;
				ptr.flag = 1;
			}else
				goto next;
			recalc_ptr_req();
		}
	}
	return nil;
}

static void
wake_ptp(void)
{
	if(IOB_RESET){
		ptp.pia = 0;
		ptp.busy = 0;
		ptp.flag = 0;
		ptp.b = 0;
	}
	if(iodev == PTP){
		if(IOB_STATUS){
			if(ptp.b) iobus0 |= F30;
			if(ptp.busy) iobus0 |= F31;
			if(ptp.flag) iobus0 |= F32;
			iobus0 |= ptp.pia & 7;
		}
		if(IOB_CONO_CLEAR){
			ptp.pia = 0;
			ptp.busy = 0;
			ptp.flag = 0;
			ptp.b = 0;
		}
		if(IOB_CONO_SET){
			if(iobus0 & F30) ptp.b = 1;
			if(iobus0 & F31) ptp.busy = 1;
			if(iobus0 & F32) ptp.flag = 1;
			ptp.pia |= iobus0 & 7;
		}
		if(IOB_DATAO_CLEAR){
			ptp.ptp = 0;
			ptp.busy = 1;
			ptp.flag = 0;
			waitdatao = 0;
		}
		if(IOB_DATAO_SET){
			ptp.ptp = iobus0 & 0377;
			waitdatao = 1;
		}
	}
	recalc_ptp_req();
}

static void
wake_ptr(void)
{
	if(IOB_RESET){
		ptr.pia = 0;
		ptr.busy = 0;
		ptr.flag = 0;
		ptr.b = 0;
	}

	if(iodev == PTR){
		if(IOB_STATUS){
			if(ptr.motor_on) iobus0 |= F27;
			if(ptr.b) iobus0 |= F30;
			if(ptr.busy) iobus0 |= F31;
			if(ptr.flag) iobus0 |= F32;
			iobus0 |= ptr.pia & 7;
		}
		if(IOB_DATAI){
			iobus0 |= ptr.ptr;
			ptr.flag = 0;
			// actually when DATAI is negated again
			ptr.busy = 1;
		}
		if(IOB_CONO_CLEAR){
			ptr.pia = 0;
			ptr.busy = 0;
			ptr.flag = 0;
			ptr.b = 0;
		}
		if(IOB_CONO_SET){
			if(iobus0 & F30) ptr.b = 1;
			if(iobus0 & F31) ptr.busy = 1;
			if(iobus0 & F32) ptr.flag = 1;
			ptr.pia |= iobus0 & 7;
		}
	}
	recalc_ptr_req();
}

void
ptr_setmotor(int m)
{
	if(ptr.motor_on == m)
		return;
	ptr.motor_on = m;
	if(ptr.motor_on)
		ptr.busy = 0;
	ptr.flag = 1;
	recalc_ptr_req();
}

void
initpt(void)
{
	pthread_t thread_id;
	ioreq[PTP] = 0;
	iobusmap[PTP] = wake_ptp;
	ioreq[PTR] = 0;
	iobusmap[PTR] = wake_ptr;
	pthread_create(&thread_id, nil, ptthread, nil);

	ptr.fp = fopen("../code/test.rim", "rb");
	ptp.fp = fopen("../code/ptp.out", "wb");
}
