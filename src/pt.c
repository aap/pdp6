#include "pdp6.h"
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <pthread.h>    
#include <poll.h>

/* TODO? implement motor delays */

static void
recalc_ptp_req(Ptp *ptp)
{
	setreq(ptp->bus, PTP, ptp->flag ? ptp->pia : 0);
}

static void
recalc_ptr_req(Ptr *ptr)
{
	setreq(ptr->bus, PTR, ptr->flag ? ptr->pia : 0);
}

/* We have to punch after DATAO SET has happened. But BUSY is set by
 * DATAO CLEAR. So we use waitdatao to record when SET has happened */

static void*
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

static void*
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
	IOBus *bus;

	ptp = dev;
	bus = ptp->bus;
	if(IOB_RESET){
		ptp->pia = 0;
		ptp->busy = 0;
		ptp->flag = 0;
		ptp->b = 0;
	}
	if(bus->devcode == PTP){
		if(IOB_STATUS){
			if(ptp->b) bus->c12 |= F30;
			if(ptp->busy) bus->c12 |= F31;
			if(ptp->flag) bus->c12 |= F32;
			bus->c12 |= ptp->pia & 7;
		}
		if(IOB_CONO_CLEAR){
			ptp->pia = 0;
			ptp->busy = 0;
			ptp->flag = 0;
			ptp->b = 0;
		}
		if(IOB_CONO_SET){
			if(bus->c12 & F30) ptp->b = 1;
			if(bus->c12 & F31) ptp->busy = 1;
			if(bus->c12 & F32) ptp->flag = 1;
			ptp->pia |= bus->c12 & 7;
		}
		if(IOB_DATAO_CLEAR){
			ptp->ptp = 0;
			ptp->busy = 1;
			ptp->flag = 0;
			ptp->waitdatao = 0;
		}
		if(IOB_DATAO_SET){
			ptp->ptp = bus->c12 & 0377;
			ptp->waitdatao = 1;
		}
	}
	recalc_ptp_req(ptp);
}

static void
wake_ptr(void *dev)
{
	Ptr *ptr;
	IOBus *bus;

	ptr = dev;
	bus = ptr->bus;
	if(IOB_RESET){
		ptr->pia = 0;
		ptr->busy = 0;
		ptr->flag = 0;
		ptr->b = 0;
	}

	if(bus->devcode == PTR){
		if(IOB_STATUS){
			if(ptr->motor_on) bus->c12 |= F27;
			if(ptr->b) bus->c12 |= F30;
			if(ptr->busy) bus->c12 |= F31;
			if(ptr->flag) bus->c12 |= F32;
			bus->c12 |= ptr->pia & 7;
		}
		if(IOB_DATAI){
			bus->c12 |= ptr->ptr;
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
			if(bus->c12 & F30) ptr->b = 1;
			if(bus->c12 & F31) ptr->busy = 1;
			if(bus->c12 & F32) ptr->flag = 1;
			ptr->pia |= bus->c12 & 7;
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
initptp(Ptp *ptp, IOBus *bus)
{
	pthread_t thread_id;
	ptp->bus = bus;
	bus->dev[PTP] = (Busdev){ ptp, wake_ptp, 0 };
	pthread_create(&thread_id, nil, ptpthread, ptp);

	ptp->fp = fopen("../code/ptp.out", "wb");
}

void
initptr(Ptr *ptr, IOBus *bus)
{
	pthread_t thread_id;
	ptr->bus = bus;
	bus->dev[PTR] = (Busdev){ ptr, wake_ptr, 0 };
	pthread_create(&thread_id, nil, ptrthread, ptr);

	ptr->fp = fopen("../code/test.rim", "rb");
}
