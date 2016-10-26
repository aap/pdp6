#include "pdp6.h"
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <pthread.h>    
#include <poll.h>

/* TODO? implement motor delays */

void
recalc_ptp_req(Ptp *ptp)
{
	u8 req;
	req = ptp->flag ? ptp->pia : 0;
	if(req != iobus[PTP].pireq){
		iobus[PTP].pireq = req;
		recalc_req();
	}
}

void
recalc_ptr_req(Ptr *ptr)
{
	u8 req;
	req = ptr->flag ? ptr->pia : 0;
	if(req != iobus[PTR].pireq){
		iobus[PTR].pireq = req;
		recalc_req();
	}
}

/* We have to punch after DATAO SET has happened. But BUSY is set by
 * DATAO CLEAR. So we use waitdatao to record when SET has happened */

void*
ptpthread(void *dev)
{
	Ptp *ptp;

	ptp = dev;
	for(;;){
		if(ptp->busy && ptp->waitdatao){
			if(ptp->fp){
				if(ptp->b)
					putc((ptp->ptp & 077) | 0200, ptp->fp);
				else
					putc(ptp->ptp, ptp->fp);
				fflush(ptp->fp);
			}
			ptp->busy = 0;
			ptp->flag = 1;
			recalc_ptp_req(ptp);
		}
	}
	return nil;
}

void*
ptrthread(void *dev)
{
	Ptr *ptr;
	int c;

	ptr = dev;
	for(;;){
		if(ptr->busy && ptr->motor_on){
			// PTR CLR
			ptr->sr = 0;
			ptr->ptr = 0;
	next:
			if(ptr->fp)
				c = getc(ptr->fp);
			else
				c = 0;
			if(c == EOF)
				c = 0;
			if(!ptr->b || c & 0200){
				// PTR STROBE
				ptr->sr <<= 1;
				ptr->ptr <<= 6;
				ptr->sr |= 1;
				ptr->ptr |= c & 077;
				if(!ptr->b)
					ptr->ptr |= c & 0300;
			}
			if(!ptr->b || ptr->sr & 040){
				ptr->busy = 0;
				ptr->flag = 1;
			}else
				goto next;
			recalc_ptr_req(ptr);
		}
	}
	return nil;
}

static void
wake_ptp(void *dev)
{
	Ptp *ptp;

	ptp = dev;
	if(IOB_RESET){
		ptp->pia = 0;
		ptp->busy = 0;
		ptp->flag = 0;
		ptp->b = 0;
	}
	if(iodev == PTP){
		if(IOB_STATUS){
			if(ptp->b) iobus0 |= F30;
			if(ptp->busy) iobus0 |= F31;
			if(ptp->flag) iobus0 |= F32;
			iobus0 |= ptp->pia & 7;
		}
		if(IOB_CONO_CLEAR){
			ptp->pia = 0;
			ptp->busy = 0;
			ptp->flag = 0;
			ptp->b = 0;
		}
		if(IOB_CONO_SET){
			if(iobus0 & F30) ptp->b = 1;
			if(iobus0 & F31) ptp->busy = 1;
			if(iobus0 & F32) ptp->flag = 1;
			ptp->pia |= iobus0 & 7;
		}
		if(IOB_DATAO_CLEAR){
			ptp->ptp = 0;
			ptp->busy = 1;
			ptp->flag = 0;
			ptp->waitdatao = 0;
		}
		if(IOB_DATAO_SET){
			ptp->ptp = iobus0 & 0377;
			ptp->waitdatao = 1;
		}
	}
	recalc_ptp_req(ptp);
}

static void
wake_ptr(void *dev)
{
	Ptr *ptr;

	ptr = dev;
	if(IOB_RESET){
		ptr->pia = 0;
		ptr->busy = 0;
		ptr->flag = 0;
		ptr->b = 0;
	}

	if(iodev == PTR){
		if(IOB_STATUS){
			if(ptr->motor_on) iobus0 |= F27;
			if(ptr->b) iobus0 |= F30;
			if(ptr->busy) iobus0 |= F31;
			if(ptr->flag) iobus0 |= F32;
			iobus0 |= ptr->pia & 7;
		}
		if(IOB_DATAI){
			iobus0 |= ptr->ptr;
			ptr->flag = 0;
			// actually when DATAI is negated again
			ptr->busy = 1;
		}
		if(IOB_CONO_CLEAR){
			ptr->pia = 0;
			ptr->busy = 0;
			ptr->flag = 0;
			ptr->b = 0;
		}
		if(IOB_CONO_SET){
			if(iobus0 & F30) ptr->b = 1;
			if(iobus0 & F31) ptr->busy = 1;
			if(iobus0 & F32) ptr->flag = 1;
			ptr->pia |= iobus0 & 7;
		}
	}
	recalc_ptr_req(ptr);
}

void
ptr_setmotor(Ptr *ptr, int m)
{
	if(ptr->motor_on == m)
		return;
	ptr->motor_on = m;
	if(ptr->motor_on)
		ptr->busy = 0;
	ptr->flag = 1;
	recalc_ptr_req(ptr);
}

void
initptp(Ptp *ptp)
{
	pthread_t thread_id;
	iobus[PTP] = (Busdev){ ptp, wake_ptp, 0 };
	pthread_create(&thread_id, nil, ptpthread, ptp);

	ptp->fp = fopen("../code/ptp.out", "wb");
}

void
initptr(Ptr *ptr)
{
	pthread_t thread_id;
	iobus[PTR] = (Busdev){ ptr, wake_ptr, 0 };
	pthread_create(&thread_id, nil, ptrthread, ptr);

	ptr->fp = fopen("../code/test.rim", "rb");
}
