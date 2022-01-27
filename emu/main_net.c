#include "pdp6.h"
#include "util.h"
#include <stdarg.h>
#include "args.h"

#include <fcntl.h>
#include <termios.h>


// TODO: get rid of this
void updatepanel(Apr *apr) {}
void writeconsreg(Apr *apr, u32 addr, u32 data)
{ // stub
}
u32
readconsreg(Apr *apr, u32 addr)
{
	// stub
	return 0;
}

#define KEYPULSE(k) (apr->k && !oldapr.k)

struct Foo
{
	Apr *apr;
	Ptr *ptr;
	Dx555 *dt[4];
};

#define PACKBYTE(b7, b6, b5, b4, b3, b2, b1, b0) \
	((b7)<<7 | (b6)<<6 | (b5)<<5 | (b4)<<4 | (b3)<<3 | (b2)<<2 | (b1)<<1 | (b0))

void
talknetwork(int fd, void *foo)
{
	Apr *apr;
	Ptr *ptr;
	Dx555 **dx;
	Apr oldapr;
	char c, buf[32];
	int n, sw;

	apr = ((struct Foo*)foo)->apr;
	ptr = ((struct Foo*)foo)->ptr;
	dx = ((struct Foo*)foo)->dt;

	printf("connected to network panel\n");

	sw = 0;
	while(1){
		if(apr == nil)
			return;
		oldapr = *apr;

		/* Read switch and key state */
		if(readn(fd, buf, 15))
			break;
		apr->data = 0;
		apr->data |= ((word)buf[0]&077) << 30;
		apr->data |= ((word)buf[1]&077) << 24;
		apr->data |= ((word)buf[2]&077) << 18;
		apr->data |= ((word)buf[3]&077) << 12;
		apr->data |= ((word)buf[4]&077) << 6;
		apr->data |= ((word)buf[5]&077) << 0;
		apr->mas = 0;
		apr->mas |= ((word)buf[6]&077) << 12;
		apr->mas |= ((word)buf[7]&077) << 6;
		apr->mas |= ((word)buf[8]&077) << 0;

		apr->key_start     = !!(buf[10] & 040);
		apr->key_readin    = !!(buf[9]  & 040);
		apr->key_inst_cont = !!(buf[10] & 020);
		apr->key_mem_cont  = !!(buf[9]  & 020);
		apr->key_inst_stop = !!(buf[10] & 010);
		apr->key_mem_stop  = !!(buf[9]  & 010);
		apr->key_io_reset  = !!(buf[10] & 4);
		apr->key_exec      = !!(buf[9]  & 4);
		apr->key_dep       = !!(buf[10] & 2);
		apr->key_dep_nxt   = !!(buf[9]  & 2);
		apr->key_ex        = !!(buf[10] & 1);
		apr->key_ex_nxt    = !!(buf[9]  & 1);
		apr->key_rd_off    = !!(buf[11] & 010);
		apr->key_rd_on     = !!(buf[11] & 4);
		apr->key_pt_rd     = !!(buf[11] & 2);
		apr->key_pt_wr     = !!(buf[11] & 1);

		apr->sw_addr_stop   = !!(buf[12] & 010);
		apr->sw_repeat      = !!(buf[12] & 4);
		apr->sw_mem_disable = !!(buf[12] & 2);
		apr->sw_power       = !!(buf[12] & 1);

		// TODO: buf[13] is a speed knob


		// buf[14] is CR

		if(apr->sw_power){
			if(KEYPULSE(key_start) || KEYPULSE(key_readin) ||
			   KEYPULSE(key_inst_cont) || KEYPULSE(key_mem_cont) ||
			   KEYPULSE(key_io_reset) || KEYPULSE(key_exec) ||
			   KEYPULSE(key_dep) || KEYPULSE(key_dep_nxt) ||
			   KEYPULSE(key_ex) || KEYPULSE(key_ex_nxt))
				apr->extpulse |= EXT_KEY_MANUAL;
			if(KEYPULSE(key_inst_stop))
				apr->extpulse |= EXT_KEY_INST_STOP;
			if(ptr){
				if(KEYPULSE(key_rd_on))
					ptr_setmotor(ptr, 1);
				if(KEYPULSE(key_rd_off))
					ptr_setmotor(ptr, 0);
			}
		}

		/* DECtape */
		int dir[4];
		int i;
		for(i = 0; i < 4; i++) dir[i] = 0;
		for(i = 0; i < 4; i++){
			if(dx[i] == nil) continue;
			int u = dx[i]->unit;
			if(u < 1 || u > 4) continue;
			if(dx[i]->motion < 0) dir[u-1] = 3;
			if(dx[i]->motion > 0) dir[u-1] = 1;
		}

		char enb6 = apr->sw_power ? 0177 : 0100;
		char enb8 = apr->sw_power ? 0377 : 0;

		/* Send lights */
		// TODO: only send changes
		n = 0;
		switch(sw){
		case 0:
			buf[n++] = '0';
			buf[n++] = enb6 & (apr->ir>>12 & 077 | 0100);
			buf[n++] = enb6 & (apr->ir>>6 & 077 | 0100);
			buf[n++] = enb6 & (apr->ir>>0 & 077 | 0100);
			buf[n++] = '1';
			buf[n++] = enb6 & (apr->mi>>30 & 077 | 0100);
			buf[n++] = enb6 & (apr->mi>>24 & 077 | 0100);
			buf[n++] = enb6 & (apr->mi>>18 & 077 | 0100);
			buf[n++] = '2';
			buf[n++] = enb6 & (apr->mi>>12 & 077 | 0100);
			buf[n++] = enb6 & (apr->mi>>6 & 077 | 0100);
			buf[n++] = enb6 & (apr->mi>>0 & 077 | 0100);
			buf[n++] = '3';
			buf[n++] = enb6 & (apr->pc>>12 & 077 | 0100);
			buf[n++] = enb6 & (apr->pc>>6 & 077 | 0100);
			buf[n++] = enb6 & (apr->pc>>0 & 077 | 0100);
			break;
		case 1:
			buf[n++] = '4';
			buf[n++] = enb6 & (apr->ma>>12 & 077 | 0100);
			buf[n++] = enb6 & (apr->ma>>6 & 077 | 0100);
			buf[n++] = enb6 & (apr->ma>>0 & 077 | 0100);
			buf[n++] = '5';
			buf[n++] = enb6 & (apr->run<<5 | apr->pih>>3 | 0100);
			buf[n++] = enb6 & ((apr->pih&7)<<3 | 0100);
			buf[n++] = 0100;
			buf[n++] = '6';
			buf[n++] = enb6 & (apr->mc_stop<<5 | apr->pir>>3 | 0100);
			buf[n++] = enb6 & ((apr->pir&7)<<3 | 0100);
			buf[n++] = 0100;
			buf[n++] = '7';
			buf[n++] = enb6 & (apr->pi_active<<5 | apr->pio>>3 | 0100);
			buf[n++] = enb6 & ((apr->pio&7)<<3 | 0100);
			buf[n++] = 0100;
			break;
		case 2:
			buf[n++]  = '8';
			buf[n++] = enb6 & (dir[0] | 0100);
			buf[n++] = enb6 & (dir[1] | 0100);
			buf[n++] = enb6 & (dir[2] | 0100);
			buf[n++] = enb6 & (dir[3] | 0100);
			break;
		case 3:
			buf[n++] = '!';
			buf[n++] = enb6 & (apr->c.mb>>30 & 077 | 0100);
			buf[n++] = enb6 & (apr->c.mb>>24 & 077 | 0100);
			buf[n++] = enb6 & (apr->c.mb>>18 & 077 | 0100);
			buf[n++] = enb6 & (apr->c.mb>>12 & 077 | 0100);
			buf[n++] = enb6 & (apr->c.mb>>6 & 077 | 0100);
			buf[n++] = enb6 & (apr->c.mb>>0 & 077 | 0100);
			buf[n++] = '"';
			buf[n++] = enb6 & (apr->c.ar>>30 & 077 | 0100);
			buf[n++] = enb6 & (apr->c.ar>>24 & 077 | 0100);
			buf[n++] = enb6 & (apr->c.ar>>18 & 077 | 0100);
			buf[n++] = enb6 & (apr->c.ar>>12 & 077 | 0100);
			buf[n++] = enb6 & (apr->c.ar>>6 & 077 | 0100);
			buf[n++] = enb6 & (apr->c.ar>>0 & 077 | 0100);
			break;
		case 4:
			buf[n++] = '#';
			buf[n++] = enb6 & (apr->c.mq>>30 & 077 | 0100);
			buf[n++] = enb6 & (apr->c.mq>>24 & 077 | 0100);
			buf[n++] = enb6 & (apr->c.mq>>18 & 077 | 0100);
			buf[n++] = enb6 & (apr->c.mq>>12 & 077 | 0100);
			buf[n++] = enb6 & (apr->c.mq>>6 & 077 | 0100);
			buf[n++] = enb6 & (apr->c.mq>>0 & 077 | 0100);
			break;
		case 5:
			buf[n++] = '$';
			buf[n++] = enb8 & PACKBYTE(apr->key_ex_st,
				apr->key_ex_sync, apr->key_dep_st, apr->key_dep_sync,
				apr->key_rd_wr, apr->mc_rd, apr->mc_wr, apr->mc_rq);
			buf[n++] = enb8 & PACKBYTE(apr->if1a, apr->af0,
				apr->af3, apr->af3a, apr->et4_ar_pse,
				apr->f1a, apr->f4a, apr->f6a);
			buf[n++] = enb8 & PACKBYTE(apr->sf3, apr->sf5a,
				apr->sf7, apr->ar_com_cont, apr->blt_f0a,
				apr->blt_f3a, apr->blt_f5a, apr->iot_f0a);
			buf[n++] = enb8 & PACKBYTE(apr->fpf1, apr->fpf2,
				apr->faf1, apr->faf2, apr->faf3,
				apr->faf4, apr->fmf1, apr->fmf2);
			buf[n++] = enb8 & PACKBYTE( apr->fdf1, apr->fdf2,
				apr->ir & H6 && apr->c.mq & F1 && !apr->nrf3, apr->nrf1, apr->nrf2,
				apr->nrf3, apr->fsf1, apr->chf7);
			buf[n++] = enb8 & PACKBYTE(apr->dsf1, apr->dsf2,
				apr->dsf3, apr->dsf4, apr->dsf5,
				apr->dsf6, apr->dsf7, apr->dsf8);
			buf[n++] = enb8 & PACKBYTE(apr->dsf9, apr->msf1,
				apr->mpf1, apr->mpf2, apr->mc_split_cyc_sync,
				apr->mc_stop_sync, apr->shf1, apr->sc == 0777);
			buf[n++] = enb8 & PACKBYTE(apr->chf1, apr->chf2,
				apr->chf3, apr->chf4, apr->chf5,
				apr->chf6, apr->lcf1, apr->dcf1);
			buf[n++] = enb8 & PACKBYTE(apr->pi_ov, apr->pi_cyc,
				!!apr->pi_req, apr->iot_go, apr->a_long,
				apr->ma == apr->mas, apr->uuo_f1, apr->cpa_pdl_ov);
			buf[n++] = enb8 & apr->fe;
			buf[n++] = enb8 & apr->sc;
			buf[n++] = enb8 & PACKBYTE(!apr->ex_user, apr->cpa_illeg_op,
				apr->ex_ill_op, apr->ex_uuo_sync, apr->ex_pi_sync,
				apr->mq36, !!(apr->sc & 0400), !!(apr->fe & 0400));
			buf[n++] = enb8 & PACKBYTE(apr->key_rim_sbr, apr->ar_cry0_xor_cry1,
				apr->ar_cry0, apr->ar_cry1, apr->ar_ov_flag,
				apr->ar_cry0_flag, apr->ar_cry1_flag, apr->ar_pc_chg_flag);
			buf[n++] = enb8 & PACKBYTE(apr->cpa_non_exist_mem, apr->cpa_clock_enable,
				apr->cpa_clock_flag, apr->cpa_pc_chg_enable, apr->cpa_arov_enable,
				!!(apr->cpa_pia&4), !!(apr->cpa_pia&2), !!(apr->cpa_pia&1));
			break;
		}
		// always want to read in 16 byte chunks!!!
		while(n < 16) buf[n++] = ' ';
		sw = (sw+1)%6;
		writen(fd, buf, n);
	}
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-t] [-d debugfile] [-p paneltty]\n", argv0);
	exit(1);
}

int
threadmain(int argc, char *argv[])
{
	const char *outfile;
	const char *panelfile;
	int fd;
	Channel *cmdchans[2];
	Task t;

	Apr *apr;
	Ptr *ptr;

	outfile = "/dev/null";
	panelfile = nil;
	ARGBEGIN{
	case 't':
		dotrace = 1;
		break;
	case 'd':
		outfile = EARGF(usage());
		break;
	case 'p':
		panelfile = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;

	if(debugfp = fopen(outfile, "w"), debugfp == nil){
		fprintf(stderr, "Can't open %s\n", outfile);
		exit(1);
	}

	rtcchan = chancreate(sizeof(RtcMsg), 20);

	if(dofile("init.ini"))
		defaultconfig();
	apr = (Apr*)getdevice("apr");
	ptr = (Ptr*)getdevice("ptr");

	if(apr == nil || ptr == nil)
		err("need APR, PTR");

	cmdchans[0] = chancreate(sizeof(char*), 1);
	cmdchans[1] = chancreate(sizeof(void*), 1);
	t = (Task){ nil, readcmdchan, cmdchans, 10, 0 };
	addtask(t);
	threadcreate(simthread, nil);
	threadcreate(cmdthread, cmdchans);
	threadcreate(rtcthread, nil);

	struct Foo foo = { apr, ptr};
	foo.dt[0] = (Dx555*)getdevice("dx1");
	foo.dt[1] = (Dx555*)getdevice("dx2");
	foo.dt[2] = nil;
	foo.dt[3] = nil;
	serve(2000, talknetwork, &foo);

	return 0;
}
