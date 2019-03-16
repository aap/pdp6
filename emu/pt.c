#include "pdp6.h"
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

/* TODO:
 * implement motor delays
 * get rid of waitdatao and use a delay
 */

char *ptr_ident = PTR_IDENT;
char *ptp_ident = PTP_IDENT;

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

static void
ptpcycle(void *p)
{
	Ptp *ptp;
	uchar c;

	ptp = p;
	if(ptp->busy && ptp->waitdatao){
		if(ptp->fd >= 0){
			if(ptp->b)
				c = (ptp->ptp & 077) | 0200;
			else
				c = ptp->ptp;
			write(ptp->fd, &c, 1);
		}
		ptp->busy = 0;
		ptp->flag = 1;
		recalc_ptp_req(ptp);
	}
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
ptr_setbusy(Ptr *ptr, int busy)
{
	if(ptr->busy == busy)
		return;
	ptr->busy = busy;
	if(ptr->busy){
		// PTR CLR
		ptr->ptr = 0;
		ptr->sr = 0;
	}
}

static void
ptr_ic_clr(Ptr *ptr)
{
	ptr->pia = 0;
	ptr_setbusy(ptr, 0);
	ptr->flag = 0;
	ptr->b = 0;
}

void
ptr_setmotor(Ptr *ptr, int m)
{
	if(ptr->motor_on == m)
		return;
	ptr->motor_on = m;
	if(ptr->motor_on)
		ptr_setbusy(ptr, 0);
	ptr->flag = 1;
	recalc_ptr_req(ptr);
}

static void
ptrcycle(void *p)
{
	Ptr *ptr;
	uchar c;

	ptr = p;

	// TODO: reader feed key
	if(!(ptr->motor_on && ptr->busy))
		return;

	/* PTR LEAD */
	if(!hasinput(ptr->fd))
		return;		/* no feed so no pulses */

	read(ptr->fd, &c, 1);

	if(!ptr->busy)
		return;

	/* skip invalid chars */
	if(ptr->b && (c & 0200) == 0)
		return;

	/* PTR STROBE */
	ptr->sr <<= 1;
	ptr->ptr <<= 6;
	ptr->sr |= 1;
	if(ptr->b)
		ptr->ptr |= c & 077;
	else
		ptr->ptr |= c;

	/* TRAIL */
	assert(ptr->busy);
	if(!ptr->b || ptr->sr & 040){
		ptr->flag = 1;
		ptr_setbusy(ptr, 0);
	}

	recalc_ptr_req(ptr);
}

static void
wake_ptr(void *dev)
{
	Ptr *ptr;
	IOBus *bus;

	ptr = dev;
	bus = ptr->bus;
	if(IOB_RESET)
		ptr_ic_clr(ptr);

	if(bus->devcode == PTR){
		if(IOB_STATUS){
//printf("PTR STATUS\n");
			if(ptr->motor_on) bus->c12 |= F27;
			if(ptr->b) bus->c12 |= F30;
			if(ptr->busy) bus->c12 |= F31;
			if(ptr->flag) bus->c12 |= F32;
			bus->c12 |= ptr->pia & 7;
		}
		if(IOB_DATAI){
//printf("PTR DATAI\n");
			bus->c12 |= ptr->ptr;
			ptr->flag = 0;
			// actually when DATAI is negated again
			ptr_setbusy(ptr, 1);
		}
		if(IOB_CONO_CLEAR)
			ptr_ic_clr(ptr);
		if(IOB_CONO_SET){
//printf("PTR CONO %012lo\n", bus->c12);
			/* TODO: schematics don't have this, but code uses it */
			if(bus->c12 & F27) ptr_setmotor(ptr, 1);
			if(bus->c12 & F30) ptr->b = 1;
			if(bus->c12 & F31) ptr_setbusy(ptr, 1);
			if(bus->c12 & F32) ptr->flag = 1;
			ptr->pia |= bus->c12 & 7;
		}
	}
	recalc_ptr_req(ptr);
}

static void
ptpunmount(Device *dev)
{
	Ptp *ptp;

	ptp = (Ptp*)dev;
	if(ptp->fd >= 0){
		close(ptp->fd);
		ptp->fd = -1;
	}
}

static void
ptpmount(Device *dev, const char *path)
{
	Ptp *ptp;

	ptp = (Ptp*)dev;
	ptpunmount(dev);
	ptp->fd = open(path, O_WRONLY | O_CREAT, 0666);
	if(ptp->fd < 0)
		fprintf(stderr, "couldn't open file %s\n", path);
}

static void
ptrunmount(Device *dev)
{
	Ptr *ptr;

	ptr = (Ptr*)dev;
	if(ptr->fd >= 0){
		close(ptr->fd);
		ptr->fd = -1;
	}
}

static void
ptrmount(Device *dev, const char *path)
{
	Ptr *ptr;

	ptr = (Ptr*)dev;
	ptrunmount(dev);
	ptr->fd = open(path, O_RDONLY);
	if(ptr->fd < 0)
		fprintf(stderr, "couldn't open file %s\n", path);
}

static void
ptrioconnect(Device *dev, IOBus *bus)
{
	Ptr *ptr;
	ptr = (Ptr*)dev;
	ptr->bus = bus;
	bus->dev[PTR] = (Busdev){ ptr, wake_ptr, 0 };
}

static void
ptpioconnect(Device *dev, IOBus *bus)
{
	Ptp *ptp;
	ptp = (Ptp*)dev;
	ptp->bus = bus;
	bus->dev[PTP] = (Busdev){ ptp, wake_ptp, 0 };
}

static Device ptpproto = {
	nil, nil, "",
	ptpmount, ptpunmount,
	ptpioconnect,
	nil, nil
};

static Device ptrproto = {
	nil, nil, "",
	ptrmount, ptrunmount,
	ptrioconnect,
	nil, nil
};


Device*
makeptp(int argc, char *argv[])
{
	Ptp *ptp;
	Task t;

	ptp = malloc(sizeof(Ptp));
	memset(ptp, 0, sizeof(Ptp));

	ptp->dev = ptpproto;
	ptp->dev.type = ptp_ident;
	ptp->fd = -1;

	// 63.3 chars per second, value around 60000?
	t = (Task){ nil, ptpcycle, ptp, 1000, 0 };
	addtask(t);
	return &ptp->dev;
}

Device*
makeptr(int argc, char *argv[])
{
	Ptr *ptr;
	Task t;

	ptr = malloc(sizeof(Ptr));
	memset(ptr, 0, sizeof(Ptr));

	ptr->dev = ptrproto;
	ptr->dev.type = ptr_ident;
	ptr->fd = -1;

	// 400 chars per second, value around 10000?
	t = (Task){ nil, ptrcycle, ptr, 1000, 0 };
	addtask(t);
	return &ptr->dev;
}
