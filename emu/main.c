#include "pdp6.h"
#include <stdarg.h>
#include <SDL.h>
#include <SDL_image.h>
#include <pthread.h>
#include "args.h"

typedef struct Point Point;
struct Point
{
	float x, y;
};

typedef struct Panel Panel;
struct Panel
{
	SDL_Surface *surf;
	SDL_Rect pos;
};

typedef struct Grid Grid;
struct Grid
{
	float xoff, yoff;
	float xscl, yscl;
	Panel *panel;
};

typedef struct Element Element;
struct Element
{
	SDL_Surface **surf;
	Grid *grid;
	Point pos;
	int state;
	int active;
};

char *argv0;

FILE *debugfp;
int dotrace;

void
trace(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	if(dotrace){
		fprintf(debugfp, "  ");
		vfprintf(debugfp, fmt, ap);
	}
	va_end(ap);
}

void
debug(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(debugfp, fmt, ap);
	va_end(ap);
}

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

u32
getms(void)
{
	return SDL_GetTicks();
}

int
hasinput(int fd)
{
	fd_set fds;
	struct timeval timeout;

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	return select(fd+1, &fds, NULL, NULL, &timeout) > 0;
}


SDL_Surface*
mustloadimg(const char *path)
{
	SDL_Surface *s;
	s = IMG_Load(path);
	if(s == nil)
		err("Couldn't load %s", path);
	return s;
}

SDL_Surface *lampsurf[2];
SDL_Surface *switchsurf[2];
SDL_Surface *keysurf[3];

Panel oppanel;
Grid opgrid1;	/* the smaller base grid */
Grid opgrid2;	/* the key grid */

Panel iopanel;
Grid iogrid;

Panel indpanel1, indpanel2;
Grid indgrid1;
Grid indgrid2;

Panel extrapanel;
Grid extragrid;

/* operator panel */
Element *data_sw, *ma_sw, *misc_sw;
Element *ir_l, *mi_l, *pc_l, *ma_l,
        *pih_l, *pir_l, *pio_l, *misc_l;

/* bay indicator panel */
Element *mb_l, *ar_l, *mq_l;
Element *ff_l;	/* flip flops */

/* io panel */
Element *iobus_l, *cr_l, *crbuf_l, *ptr_l, *ptrbuf_l,
        *ptp_l, *ptpbuf_l, *tty_l, *ttibuf_l,
	*pr_l, *rlr_l, *rla_l;

/* extra panel */
Element *extra_sw;
Element *extra_l;

#include "elements.inc"

void
setlights(word w, Element *l, int n)
{
	int i;
	for(i = 0; i < n; i++)
		l[n-i-1].state = !!(w & 1L<<i);
}

word
getswitches(Element *sw, int n)
{
	word w;
	int i;
	w = 0;
	for(i = 0; i < n; i++)
		w |= (word)sw[n-i-1].state << i;
	return w;
}

#define KEYPULSE(k) (apr->k && !oldapr.k)

void
updateapr(Apr *apr, Ptr *ptr)
{
	Apr oldapr;

	oldapr = *apr;
	setlights(apr->ir, ir_l, 18);
	setlights(apr->mi, mi_l, 36);
	setlights(apr->pc, pc_l, 18);
	setlights(apr->ma, ma_l, 18);
	setlights(apr->pih, pih_l, 7);
	setlights(apr->pir, pir_l, 7);
	setlights(apr->pio, pio_l, 7);
	misc_l[0].state = apr->pi_active;
	misc_l[1].state = apr->mc_stop;
	misc_l[2].state = apr->run;
	misc_l[3].state = apr->sw_repeat = misc_sw[0].state;
	misc_l[4].state = apr->sw_addr_stop = misc_sw[1].state;
	misc_l[5].state = apr->sw_power = misc_sw[2].state;
	misc_l[6].state = apr->sw_mem_disable = misc_sw[3].state;
	apr->data = getswitches(data_sw, 36);
	apr->mas = getswitches(ma_sw, 18);

	apr->sw_rim_maint = extra_sw[0].state;
	if(apr->sw_rim_maint)
		apr->key_rim_sbr = 1;
	apr->sw_rpt_bypass = extra_sw[1].state;
	apr->sw_art3_maint = extra_sw[2].state;
	apr->sw_sct_maint = extra_sw[3].state;
	apr->sw_spltcyc_override  = extra_sw[4].state;

	apr->key_start     = keys[0].state == 1;
	apr->key_readin    = keys[0].state == 2;
	apr->key_inst_cont = keys[1].state == 1;
	apr->key_mem_cont  = keys[1].state == 2;
	apr->key_inst_stop = keys[2].state == 1;
	apr->key_mem_stop  = keys[2].state == 2;
	apr->key_io_reset  = keys[3].state == 1;
	apr->key_exec      = keys[3].state == 2;
	apr->key_dep       = keys[4].state == 1;
	apr->key_dep_nxt   = keys[4].state == 2;
	apr->key_ex        = keys[5].state == 1;
	apr->key_ex_nxt    = keys[5].state == 2;
	apr->key_rd_off    = keys[6].state == 1;
	apr->key_rd_on     = keys[6].state == 2;
	apr->key_pt_rd     = keys[7].state == 1;
	apr->key_pt_wr     = keys[7].state == 2;
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

	setlights(apr->mb, mb_l, 36);
	setlights(apr->ar, ar_l, 36);
	setlights(apr->mq, mq_l, 36);

	ff_l[0].state = apr->key_ex_st;
	ff_l[1].state = apr->key_ex_sync;
	ff_l[2].state = apr->key_dep_st;
	ff_l[3].state = apr->key_dep_sync;
	ff_l[4].state = apr->key_rd_wr;
	ff_l[5].state = apr->mc_rd;
	ff_l[6].state = apr->mc_wr;
	ff_l[7].state = apr->mc_rq;

	ff_l[8].state = apr->if1a;
	ff_l[9].state = apr->af0;
	ff_l[10].state = apr->af3;
	ff_l[11].state = apr->af3a;
	ff_l[12].state = apr->et4_ar_pse;
	ff_l[13].state = apr->f1a;
	ff_l[14].state = apr->f4a;
	ff_l[15].state = apr->f6a;

	ff_l[16].state = apr->sf3;
	ff_l[17].state = apr->sf5a;
	ff_l[18].state = apr->sf7;
	ff_l[19].state = apr->ar_com_cont;
	ff_l[20].state = apr->blt_f0a;
	ff_l[21].state = apr->blt_f3a;
	ff_l[22].state = apr->blt_f5a;
	ff_l[23].state = apr->iot_f0a;

	ff_l[24].state = apr->fpf1;
	ff_l[25].state = apr->fpf2;
	ff_l[26].state = apr->faf1;
	ff_l[27].state = apr->faf2;
	ff_l[28].state = apr->faf3;
	ff_l[29].state = apr->faf4;
	ff_l[30].state = apr->fmf1;
	ff_l[31].state = apr->fmf2;

	ff_l[32].state = apr->fdf1;
	ff_l[33].state = apr->fdf2;
	ff_l[34].state = apr->ir & H6 && apr->mq & F1 && !apr->nrf3;
	ff_l[35].state = apr->nrf1;
	ff_l[36].state = apr->nrf2;
	ff_l[37].state = apr->nrf3;
	ff_l[38].state = apr->fsf1;
	ff_l[39].state = apr->chf7;

	ff_l[40].state = apr->dsf1;
	ff_l[41].state = apr->dsf2;
	ff_l[42].state = apr->dsf3;
	ff_l[43].state = apr->dsf4;
	ff_l[44].state = apr->dsf5;
	ff_l[45].state = apr->dsf6;
	ff_l[46].state = apr->dsf7;
	ff_l[47].state = apr->dsf8;

	ff_l[48].state = apr->dsf9;
	ff_l[49].state = apr->msf1;
	ff_l[50].state = apr->mpf1;
	ff_l[51].state = apr->mpf2;
	ff_l[52].state = apr->mc_split_cyc_sync;
	ff_l[53].state = apr->mc_stop_sync;
	ff_l[54].state = apr->shf1;
	ff_l[55].state = apr->sc == 0777;

	ff_l[56].state = apr->chf1;
	ff_l[57].state = apr->chf2;
	ff_l[58].state = apr->chf3;
	ff_l[59].state = apr->chf4;
	ff_l[60].state = apr->chf5;
	ff_l[61].state = apr->chf6;
	ff_l[62].state = apr->lcf1;
	ff_l[63].state = apr->dcf1;

	ff_l[64].state = apr->pi_ov;
	ff_l[65].state = apr->pi_cyc;
	ff_l[66].state = !!apr->pi_req;
	ff_l[67].state = apr->iot_go;
	ff_l[68].state = apr->a_long;
	ff_l[69].state = apr->ma == apr->mas;
	ff_l[70].state = apr->uuo_f1;
	ff_l[71].state = apr->cpa_pdl_ov;

	setlights(apr->fe, &ff_l[72], 8);

	setlights(apr->sc, &ff_l[80], 8);

	ff_l[88].state = !apr->ex_user;
	ff_l[89].state = apr->cpa_illeg_op;
	ff_l[90].state = apr->ex_ill_op;
	ff_l[91].state = apr->ex_uuo_sync;
	ff_l[92].state = apr->ex_pi_sync;
	ff_l[93].state = apr->mq36;
	ff_l[94].state = !!(apr->sc & 0400);
	ff_l[95].state = !!(apr->fe & 0400);

	ff_l[96].state = apr->key_rim_sbr;
	ff_l[97].state = apr->ar_cry0_xor_cry1;
	ff_l[98].state = apr->ar_cry0;
	ff_l[99].state = apr->ar_cry1;
	ff_l[100].state = apr->ar_ov_flag;
	ff_l[101].state = apr->ar_cry0_flag;
	ff_l[102].state = apr->ar_cry1_flag;
	ff_l[103].state = apr->ar_pc_chg_flag;

	ff_l[104].state = apr->cpa_non_exist_mem;
	ff_l[105].state = apr->cpa_clock_enable;
	ff_l[106].state = apr->cpa_clock_flag;
	ff_l[107].state = apr->cpa_pc_chg_enable;
	ff_l[108].state = apr->cpa_arov_enable;
	ff_l[109].state = !!(apr->cpa_pia&4);
	ff_l[110].state = !!(apr->cpa_pia&2);
	ff_l[111].state = !!(apr->cpa_pia&1);

	setlights(apr->pr, pr_l, 8);
	setlights(apr->rlr, rlr_l, 8);
	setlights(apr->rla, rla_l, 8);
	setlights(apr->iobus.c12, iobus_l, 36);
}

void
updatetty(Tty *tty)
{
	tty_l[0].state = tty->tti_busy;
	tty_l[1].state = tty->tti_flag;
	tty_l[2].state = tty->tto_busy;
	tty_l[3].state = tty->tto_flag;
	setlights(tty->pia, &tty_l[4], 3);
	setlights(tty->tti, ttibuf_l, 8);
}

void
updatept(Ptp *ptp, Ptr *ptr)
{
	ptp_l[0].state = ptp->b;
	ptp_l[1].state = ptp->busy;
	ptp_l[2].state = ptp->flag;
	setlights(ptp->pia, &ptp_l[3], 3);
	setlights(ptp->ptp, ptpbuf_l, 8);

	ptr_l[0].state = ptr->b;
	ptr_l[1].state = ptr->busy;
	ptr_l[2].state = ptr->flag;
	setlights(ptr->pia, &ptr_l[3], 3);
	setlights(ptr->ptr, ptrbuf_l, 36);

	extra_l[0].state = ptr->motor_on;
}

void
putpixel(SDL_Surface *screen, int x, int y, Uint32 col)
{
	Uint32 *p = (Uint32*)screen->pixels;
	if(x < 0 || x >= screen->w ||
	   y < 0 || y >= screen->h)
		return;
	p += y*screen->w+x;
	*p = SDL_MapRGBA(screen->format,
		col&0xFF, (col>>8)&0xFF, (col>>16)&0xFF, (col>>24)&0xFF);
}

void
drawhline(SDL_Surface *screen, int y, int x1, int x2, Uint32 col)
{
	for(; x1 < x2; x1++)
		putpixel(screen, x1, y, col);
}

void
drawvline(SDL_Surface *screen, int x, int y1, int y2, Uint32 col)
{
	for(; y1 < y2; y1++)
		putpixel(screen, x, y1, col);
}

Point
xform(Grid *g, Point p)
{
	p.x = g->panel->pos.x + g->xoff + p.x*g->xscl;
	p.y = g->panel->pos.y + (g->panel->surf->h - (g->yoff + p.y*g->yscl));
	return p;
}

int
ismouseover(Element *e, int x, int y)
{
	Point p;

	p = xform(e->grid, e->pos);
	return x >= p.x && x <= p.x + e->surf[e->state]->w &&
	       y >= p.y && y <= p.y + e->surf[e->state]->h;
}

void
drawgrid(Grid *g, SDL_Surface *s, Uint32 col)
{
	SDL_Surface *ps;
	int x, y;
	int xmax, ymax;
	Point p;

	ps = g->panel->surf;
	xmax = ps->w/g->xscl;
	ymax = ps->h/g->yscl;
	for(x = 0; x < xmax; x++){
		p = xform(g, (Point){ x, 0 });
		drawvline(s, p.x,
		          p.y - ps->h, p.y, col);
	}
	for(y = 0; y < ymax; y++){
		p = xform(g, (Point){ 0, y });
		drawhline(s, p.y,
		          p.x, p.x + ps->w, col);
	}
}

void
drawelement(SDL_Surface *screen, Element *elt, int power)
{
	SDL_Rect r;
	Point p;
	int s;

	p = xform(elt->grid, elt->pos);
	r.x = p.x+0.5;
	r.y = p.y+0.5;
	if(elt->surf == lampsurf)
		s = elt->state && power;
	else
		s = elt->state;
	SDL_BlitSurface(elt->surf[s], nil, screen, &r);
}

void
mouse(int button, int state, int x, int y)
{
	static int buttonstate;
	Element *e;
	int i;

	if(button){
		if(state == 1)
			buttonstate |= 1 << button-1;
		else
			buttonstate &= ~(1 << button-1);
	}

	/* keys */
	for(i = 0; i < nelem(keys); i++){
		e = &keys[i];
		/* e->active means latched on/off for keys */
		if(buttonstate == 0 || !ismouseover(e, x, y)){
			if(!e->active)
				e->state = 0;
			continue;
		}
		if((buttonstate & 5) == 5)	/* left and right -> latched on/off */
			e->active = !e->active;
		else if(buttonstate & 1)	/* left button -> down */
			e->state = 1;
		else if(buttonstate & 4)	/* right button -> up */
			e->state = 2;
	}

	/* switches */
	for(i = 0; i < nelem(switches); i++){
		e = &switches[i];
		if(buttonstate == 0 || !ismouseover(e, x, y)){
			e->active = 0;
			continue;
		}
		if(!e->active){
			e->active = 1;
			if(buttonstate & 1)	/* left button, toggle */
				e->state = !e->state;
			else if(buttonstate & 2)	/* middle button, on */
				e->state = 1;
			else if(buttonstate & 4)	/* right button, off */
				e->state = 0;
		}
	}
}

void
findlayout(int *w, int *h)
{
	float gap;

	gap = (oppanel.surf->w - indpanel1.surf->w - indpanel2.surf->w)/4.0f;

	indpanel1.pos = (SDL_Rect){ 0, 0, 0, 0 };
	indpanel1.pos.x += gap;

	indpanel2.pos = indpanel1.pos;
	indpanel2.pos.x += indpanel1.surf->w + 2*gap;

	iopanel.pos = indpanel2.pos;
	iopanel.pos.y += indpanel2.surf->h;

	oppanel.pos = (SDL_Rect){ 0, 0, 0, 0 };
	oppanel.pos.y += indpanel1.surf->h*2.7;

	extrapanel.pos = indpanel1.pos;
	extrapanel.pos.y = oppanel.pos.y - extrapanel.surf->h;

	*w = oppanel.surf->w;
	*h = oppanel.pos.y + oppanel.surf->h;
}


Thread *threads;

void
addthread(Thread th)
{
	Thread *p;
	p = malloc(sizeof(Thread));
	*p = th;
	p->next = threads;
	threads = p;
}

void*
simthread(void *p)
{
	Thread *th;

	printf("[simthread] start\n");
	for(;;)
		for(th = threads; th; th = th->next){
			th->cnt++;
			if(th->cnt == th->freq){
				th->cnt = 0;
				th->f(th->arg);
			}
		}
	err("can't happen");
	return nil;
}


void
quit(int code)
{
	//SDL_Quit();
	putchar('\n');
	exit(code);
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-t] [-d debugfile] [-c ttyfile]\n", argv0);
	exit(1);
}

Apr *aprs[4];

int
main(int argc, char *argv[])
{
	SDL_Surface *screen;
	SDL_Event ev;
	SDL_MouseButtonEvent *mbev;
	SDL_MouseMotionEvent *mmev;
	Element *e;
	int i;
	int w, h;
	pthread_t cmd_thread, sim_thread;
	const char *outfile, *ttyfile;

	Apr *apr;
	Ptr *ptr;
	Ptp *ptp;
	Tty *tty;
	Mem *coremems[4];
	Mem *fastmem;

	outfile = "/dev/null";
	ttyfile = "/tmp/6tty";
	ARGBEGIN{
	case 't':
		dotrace = 1;
		break;
	case 'd':
		outfile = EARGF(usage());
		break;
	case 'c':
		ttyfile = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;
	if(debugfp = fopen(outfile, "w"), debugfp == nil){
		fprintf(stderr, "Can't open %s\n", outfile);
		exit(1);
	}


	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		err("%s", SDL_GetError());

	lampsurf[0] = mustloadimg("../art/lamp_off.png");
	lampsurf[1] = mustloadimg("../art/lamp_on.png");

	switchsurf[0] = mustloadimg("../art/switch_d.png");
	switchsurf[1] = mustloadimg("../art/switch_u.png");

	keysurf[0] = mustloadimg("../art/key_n.png");
	keysurf[1] = mustloadimg("../art/key_d.png");
	keysurf[2] = mustloadimg("../art/key_u.png");

	oppanel.surf = mustloadimg("../art/op_panel.png");

	opgrid1.panel = &oppanel;
	opgrid1.xscl = opgrid1.panel->surf->w/90.0f;
	opgrid1.yscl = opgrid1.panel->surf->h/11.0f;
	opgrid1.yoff = opgrid1.yscl/2.0f;

	opgrid2.panel = &oppanel;
	opgrid2.xscl = opgrid1.xscl*1.76f;
	opgrid2.yscl = opgrid2.xscl;
	opgrid2.xoff = opgrid1.xscl*44.5f;
	opgrid2.yoff = opgrid1.panel->surf->h*2.4f/143.0f;

	iopanel.surf = mustloadimg("../art/io_panel.png");
	iogrid.panel = &iopanel;
	iogrid.xscl = iogrid.panel->surf->w/44.0f;
	iogrid.yscl = iogrid.panel->surf->h/28.0f;

	indpanel1.surf = mustloadimg("../art/ind_panel1.png");
	indgrid1.panel = &indpanel1;
	indgrid1.xscl = indgrid1.panel->surf->w*5.0f/77.0f;
	indgrid1.yscl = indgrid1.panel->surf->h/12.0f;

	indpanel2.surf = mustloadimg("../art/ind_panel2.png");
	indgrid2.panel = &indpanel2;
	indgrid2.xscl = indgrid2.panel->surf->w/44.0f;
	indgrid2.yscl = indgrid2.panel->surf->h/11.0f;

	extrapanel.surf = mustloadimg("../art/extra_panel.png");
	extragrid = indgrid1;
	extragrid.panel = &extrapanel;

	findlayout(&w, &h);

	screen = SDL_SetVideoMode(w, h, 32, SDL_DOUBLEBUF);
	if(screen == nil)
		err("%s", SDL_GetError());

	e = switches;
	data_sw = e; e += 36;
	ma_sw = e; e += 18;
	misc_sw = e; e += 4;

	extra_sw = e; e += 5;

	e = lamps;
	mi_l = e; e += 36;
	ir_l = e; e += 18;
	ma_l = e; e += 18;
	pc_l = e; e += 18;
	pio_l = e; e += 7;
	pir_l = e; e += 7;
	pih_l = e; e += 7;
	misc_l = e; e += 7;

	mb_l = e; e += 36;
	ar_l = e; e += 36;
	mq_l = e; e += 36;
	ff_l = e; e += 14*8;

	iobus_l = e; e += 36;
	cr_l = e; e += 9;
	crbuf_l = e; e += 36;
	ptr_l = e; e += 6;
	ptrbuf_l = e; e += 36;
	ptp_l = e; e += 6;
	ptpbuf_l = e; e += 8;
	tty_l = e; e += 7;
	ttibuf_l = e; e += 8;
	pr_l = e; e += 8;
	rlr_l = e; e += 8;
	rla_l = e; e += 8;

	extra_l = e; e += 1;

	fastmem = makefastmem(0);
	coremems[0] = makecoremem("mem_0");
	coremems[1] = makecoremem("mem_1");
	coremems[2] = makecoremem("mem_2");
	coremems[3] = makecoremem("mem_3");

	aprs[0] = apr = makeapr();
	attachmem(fastmem, 0, &apr->membus, -1);
	attachmem(coremems[0], 0, &apr->membus, 0);
	attachmem(coremems[1], 0, &apr->membus, 1);
	attachmem(coremems[2], 0, &apr->membus, 2);
	attachmem(coremems[3], 0, &apr->membus, 3);
	tty = maketty(&apr->iobus);
	ptr = makeptr(&apr->iobus);
	ptp = makeptp(&apr->iobus);
	if(ttyfile)
		attachdevtty(tty, ttyfile);

	/* Power on memory */
	// TODO: maybe do that somewhere else?
	fastmem->poweron(fastmem);
	coremems[0]->poweron(coremems[0]);
	coremems[1]->poweron(coremems[1]);
	coremems[2]->poweron(coremems[2]);
	coremems[3]->poweron(coremems[3]);

	pthread_create(&sim_thread, nil, simthread, apr);
	pthread_create(&cmd_thread, nil, cmdthread, nil);

	for(;;){
		while(SDL_PollEvent(&ev))
			switch(ev.type){
			case SDL_MOUSEMOTION:
				mmev = (SDL_MouseMotionEvent*)&ev;
				mouse(0, mmev->state, mmev->x, mmev->y);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				mbev = (SDL_MouseButtonEvent*)&ev;
				mouse(mbev->button, mbev->state, mbev->x, mbev->y);
				break;
			case SDL_QUIT:
				quit(0);
			}

		updateapr(apr, ptr);
		updatetty(tty);
		updatept(ptp, ptr);

		SDL_FillRect(screen, nil, SDL_MapRGBA(screen->format, 0xe6, 0xe6, 0xe6, 0xff));
		SDL_BlitSurface(indpanel1.surf, nil, screen, &indpanel1.pos);
		SDL_BlitSurface(indpanel2.surf, nil, screen, &indpanel2.pos);
		SDL_BlitSurface(extrapanel.surf, nil, screen, &extrapanel.pos);
		SDL_BlitSurface(iopanel.surf, nil, screen, &iopanel.pos);
		SDL_BlitSurface(oppanel.surf, nil, screen, &oppanel.pos);

		for(i = 0, e = lamps; i < nelem(lamps); i++, e++)
			drawelement(screen, e, apr->sw_power);
		for(i = 0, e = keys; i < nelem(keys); i++, e++)
			drawelement(screen, e, apr->sw_power);
		for(i = 0, e = switches; i < nelem(switches); i++, e++)
			drawelement(screen, e, apr->sw_power);

//		SDL_LockSurface(screen);
//		drawgrid(&opgrid1, screen, 0xFFFFFF00);
//		drawgrid(&opgrid2, screen, 0xFF0000FF);
//		drawgrid(&iogrid, screen, 0xFFFFFF00);
//		drawgrid(&indgrid1, screen, 0xFFFFFF00);
//		drawgrid(&indgrid2, screen, 0xFFFFFF00);
//		drawgrid(&extragrid, screen, 0xFFFFFF00);
//		SDL_UnlockSurface(screen);

		SDL_Flip(screen);
	}
	return 0;
}
