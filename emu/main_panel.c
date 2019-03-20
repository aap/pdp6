#include "pdp6.h"
#include <stdarg.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "args.h"

#include "../art/panelart.inc"

typedef struct Point Point;
struct Point
{
	float x, y;
};

typedef struct Image Image;
struct Image
{
	SDL_Texture *tex;
	int w, h;
};

typedef struct Panel Panel;
struct Panel
{
	Image *surf;
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
	Image **surf;
	Grid *grid;
	Point pos;
	int state;
	int active;
};

typedef struct SwDigit SwDigit;
struct SwDigit
{
	Element *sw;	// these 3 switch
	Element *first;	// the first one
	SwDigit *prev;
	SwDigit *next;
};

static SDL_Window *window;
static SDL_Renderer *renderer;

Image*
mustloadimg(const char *path)
{
	Image *img;
	SDL_Texture *s;

	s = IMG_LoadTexture(renderer, path);
	if(s == nil)
		err("Couldn't load %s", path);
	img = malloc(sizeof(Image));
	img->tex = s;
	SDL_QueryTexture(img->tex, nil, nil, &img->w, &img->h);
	return img;
}

Image*
loadresimg(u8 *data, int sz)
{
	SDL_RWops *f;
	Image *img;
	SDL_Texture *s;

	f = SDL_RWFromConstMem(data, sz);
	if(f == nil)
		err("Couldn't load resource");
	s = IMG_LoadTexture_RW(renderer, f, 0);
	if(s == nil)
		err("Couldn't load image");
	img = malloc(sizeof(Image));
	img->tex = s;
	SDL_QueryTexture(img->tex, nil, nil, &img->w, &img->h);
	return img;
}

Image *lampsurf[2];
Image *switchsurf[2];
Image *keysurf[3];

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

SwDigit data_digits[12];
SwDigit addr_digits[6];
SwDigit *curdigit;
SDL_Rect digitindline;

#include "elements.inc"

void
setelements(word w, Element *l, int n)
{
	int i;
	for(i = 0; i < n; i++)
		l[n-i-1].state = !!(w & 1L<<i);
}

word
getelements(Element *sw, int n)
{
	word w;
	int i;
	w = 0;
	for(i = 0; i < n; i++)
		w |= (word)sw[n-i-1].state << i;
	return w;
}

#define KEYPULSE(k) (apr->k && !oldapr.k)

/* Set panel from internal APR state */
void
updatepanel(Apr *apr)
{
	setelements(apr->data, data_sw, 36);
	setelements(apr->mas, ma_sw, 18);

	extra_sw[0].state = apr->sw_rim_maint;
	extra_sw[1].state = apr->sw_rpt_bypass;
	extra_sw[2].state = apr->sw_art3_maint;
	extra_sw[3].state = apr->sw_sct_maint;
	extra_sw[4].state = apr->sw_spltcyc_override;
}

static void
updateapr(Apr *apr, Ptr *ptr)
{
	Apr oldapr;

	if(apr == nil)
		return;
	oldapr = *apr;
	setelements(apr->ir, ir_l, 18);
	setelements(apr->mi, mi_l, 36);
	setelements(apr->pc, pc_l, 18);
	setelements(apr->ma, ma_l, 18);
	setelements(apr->pih, pih_l, 7);
	setelements(apr->pir, pir_l, 7);
	setelements(apr->pio, pio_l, 7);
	misc_l[0].state = apr->pi_active;
	misc_l[1].state = apr->mc_stop;
	misc_l[2].state = apr->run;
	misc_l[3].state = apr->sw_repeat = misc_sw[0].state;
	misc_l[4].state = apr->sw_addr_stop = misc_sw[1].state;
	misc_l[5].state = apr->sw_power = misc_sw[2].state;
	misc_l[6].state = apr->sw_mem_disable = misc_sw[3].state;
	apr->data = getelements(data_sw, 36);
	apr->mas = getelements(ma_sw, 18);

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

	setelements(apr->c.mb, mb_l, 36);
	setelements(apr->c.ar, ar_l, 36);
	setelements(apr->c.mq, mq_l, 36);

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
	ff_l[34].state = apr->ir & H6 && apr->c.mq & F1 && !apr->nrf3;
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

	setelements(apr->fe, &ff_l[72], 8);

	setelements(apr->sc, &ff_l[80], 8);

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

	setelements(apr->pr, pr_l, 8);
	setelements(apr->rlr, rlr_l, 8);
	setelements(apr->rla, rla_l, 8);
	setelements(apr->iobus.c12, iobus_l, 36);
}

static void
updatetty(Tty *tty)
{
	if(tty){
		tty_l[0].state = tty->tti_busy;
		tty_l[1].state = tty->tti_flag;
		tty_l[2].state = tty->tto_busy;
		tty_l[3].state = tty->tto_flag;
		setelements(tty->pia, &tty_l[4], 3);
		setelements(tty->tti, ttibuf_l, 8);
	}
}

static void
updatept(Ptp *ptp, Ptr *ptr)
{
	if(ptp){
		ptp_l[0].state = ptp->b;
		ptp_l[1].state = ptp->busy;
		ptp_l[2].state = ptp->flag;
		setelements(ptp->pia, &ptp_l[3], 3);
		setelements(ptp->ptp, ptpbuf_l, 8);
	}

	if(ptr){
		ptr_l[0].state = ptr->b;
		ptr_l[1].state = ptr->busy;
		ptr_l[2].state = ptr->flag;
		setelements(ptr->pia, &ptr_l[3], 3);
		setelements(ptr->ptr, ptrbuf_l, 36);

		extra_l[0].state = ptr->motor_on;
	}
}

static void
putpixel(SDL_Texture *screen, int x, int y, Uint32 col)
{
	// TODO: rewrite for SDL2
/*
	Uint32 *p = (Uint32*)screen->pixels;
	if(x < 0 || x >= screen->w ||
	   y < 0 || y >= screen->h)
		return;
	p += y*screen->w+x;
	*p = SDL_MapRGBA(screen->format,
		col&0xFF, (col>>8)&0xFF, (col>>16)&0xFF, (col>>24)&0xFF);
*/
}

static void
drawhline(SDL_Texture *screen, int y, int x1, int x2, Uint32 col)
{
	for(; x1 < x2; x1++)
		putpixel(screen, x1, y, col);
}

static void
drawvline(SDL_Texture *screen, int x, int y1, int y2, Uint32 col)
{
	for(; y1 < y2; y1++)
		putpixel(screen, x, y1, col);
}

static Point
xform(Grid *g, Point p)
{
	p.x = g->panel->pos.x + g->xoff + p.x*g->xscl;
	p.y = g->panel->pos.y + (g->panel->surf->h - (g->yoff + p.y*g->yscl));
	return p;
}

static int
ismouseover(Element *e, int x, int y)
{
	Point p;

	p = xform(e->grid, e->pos);
	return x >= p.x && x <= p.x + e->surf[e->state]->w &&
	       y >= p.y && y <= p.y + e->surf[e->state]->h;
}

static void
drawgrid(Grid *g, SDL_Texture *s, Uint32 col)
{
	Image *pi;
	int x, y;
	int xmax, ymax;
	Point p;

	pi = g->panel->surf;
	xmax = pi->w/g->xscl;
	ymax = pi->h/g->yscl;
	for(x = 0; x < xmax; x++){
		p = xform(g, (Point){ x, 0 });
		drawvline(s, p.x,
		          p.y - pi->h, p.y, col);
	}
	for(y = 0; y < ymax; y++){
		p = xform(g, (Point){ 0, y });
		drawhline(s, p.y,
		          p.x, p.x + pi->w, col);
	}
}

static void
drawpanel(SDL_Renderer *renderer, Panel *p)
{
	if(p->pos.x < 0)
		return;

	p->pos.w = p->surf->w;
	p->pos.h = p->surf->h;
	SDL_RenderCopy(renderer, p->surf->tex, nil, &p->pos);
}

static void
drawelement(SDL_Renderer *renderer, Element *elt, int power)
{
	Image *img;
	SDL_Rect r;
	Point p;
	int s;

	if(elt->grid->panel == nil)
		return;

	p = xform(elt->grid, elt->pos);
	r.x = p.x+0.5;
	r.y = p.y+0.5;
	if(elt->surf == lampsurf)
		s = elt->state && power;
	else
		s = elt->state;
	img = elt->surf[s];
	r.w = img->w;
	r.h = img->h;
	SDL_RenderCopy(renderer, img->tex, nil, &r);
}

static void
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

static void
setcurdigit(SwDigit *d)
{
	Point p1, p2;

	curdigit = d;
	p1 = xform(d->sw[0].grid, d->sw[0].pos);
	p2 = xform(d->sw[2].grid, d->sw[2].pos);
	p2.x += d->sw[2].surf[0]->w;
	p2.y += d->sw[2].surf[0]->h;
	digitindline.x = p1.x;
	digitindline.y = p1.y;
	digitindline.w = p2.x - p1.x;
	digitindline.h = p2.y - p1.y;
}

static void
setdigit(int n)
{
	curdigit->sw[0].state = !!(n & 4);
	curdigit->sw[1].state = !!(n & 2);
	curdigit->sw[2].state = !!(n & 1);
	setcurdigit(curdigit->next);
}

/* This is in some ways horrible, but oh well */
static void
keydown(SDL_Keysym keysym)
{
	int i;
	static Element *dsrc, *asrc, **srcp;
	static int dlen, alen, *lenp;
	word mask, s, d;
	int ctrl, len;
	const u8 *keystate;

	if(dsrc == nil){
		dsrc = mi_l;
		dlen = 36;
	}
	if(asrc == nil){
		asrc = ma_l;
		alen = 18;
	}
	if(srcp == nil){
		srcp = &dsrc;
		lenp = &dlen;
	}

	keystate = SDL_GetKeyboardState(nil);
	ctrl = keystate[SDL_SCANCODE_CAPSLOCK] ||
		keystate[SDL_SCANCODE_LCTRL] || keystate[SDL_SCANCODE_RCTRL];

	switch(keysym.scancode){
	case SDL_SCANCODE_0: setdigit(0); break;
	case SDL_SCANCODE_1: setdigit(1); break;
	case SDL_SCANCODE_2: setdigit(2); break;
	case SDL_SCANCODE_3: setdigit(3); break;
	case SDL_SCANCODE_4: setdigit(4); break;
	case SDL_SCANCODE_5: setdigit(5); break;
	case SDL_SCANCODE_6: setdigit(6); break;
	case SDL_SCANCODE_7: setdigit(7); break;
	case SDL_SCANCODE_LEFT: setcurdigit(curdigit->prev); break;
	case SDL_SCANCODE_RIGHT: setcurdigit(curdigit->next); break;

	/* Set destination switches */
	case SDL_SCANCODE_A:
		setcurdigit(addr_digits);
		srcp = &asrc;
		lenp = &alen;
		break;
	case SDL_SCANCODE_S:
		setcurdigit(data_digits);
		srcp = &dsrc;
		lenp = &dlen;
		break;

	/* Set source lights */
	case SDL_SCANCODE_M:
		*srcp = ma_l;
		*lenp = 18;
		break;
	case SDL_SCANCODE_P:
		*srcp = pc_l;
		*lenp = 18;
		break;
	case SDL_SCANCODE_I:
		*srcp = ir_l;
		*lenp = 18;
		break;
	case SDL_SCANCODE_D:
		*srcp = mi_l;
		*lenp = 36;
		break;

	/* Set MA from IR AC */
	case SDL_SCANCODE_R:
		d = getelements(ir_l, 18);
		d = d>>5 & 017;
		setelements(d, ma_sw, 18);
		break;
	/* Set MA from IR index */
	case SDL_SCANCODE_X:
		d = getelements(ir_l, 18);
		d &= 017;
		setelements(d, ma_sw, 18);
		break;

	/* Clear */
	case SDL_SCANCODE_C:
		for(i = 0; i < 12; i++)
			setdigit(0);
		break;

	case SDL_SCANCODE_J:
		mask = LT;
		goto load;
	case SDL_SCANCODE_K:
		mask = FW;
		goto load;
	case SDL_SCANCODE_L:
		mask = RT;
		goto load;

	load:
		if(curdigit->first == data_sw)
			len = 36;
		else
			len = 18;
		d = getelements(curdigit->first, len);
		s = getelements(*srcp, *lenp);
		if(ctrl) s = (s&RT)<<18 | (s&LT)>>18;
		d = s&mask | d&~mask;
		setelements(d, curdigit->first, len);
		break;
	default: break;
	}
}

static void
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

static void
initpanel(void)
{
	Element *e;

#ifdef LOADFILES
	oppanel.surf = mustloadimg("../art/op_panel.png");
	iopanel.surf = mustloadimg("../art/io_panel.png");
	indpanel1.surf = mustloadimg("../art/ind_panel1.png");
	indpanel2.surf = mustloadimg("../art/ind_panel2.png");
	extrapanel.surf = mustloadimg("../art/extra_panel.png");

	lampsurf[0] = mustloadimg("../art/lamp_off.png");
	lampsurf[1] = mustloadimg("../art/lamp_on.png");

	switchsurf[0] = mustloadimg("../art/switch_d.png");
	switchsurf[1] = mustloadimg("../art/switch_u.png");

	keysurf[0] = mustloadimg("../art/key_n.png");
	keysurf[1] = mustloadimg("../art/key_d.png");
	keysurf[2] = mustloadimg("../art/key_u.png");
#else
	oppanel.surf = loadresimg(op_panel_png, op_panel_png_len);
	iopanel.surf = loadresimg(io_panel_png, io_panel_png_len);
	indpanel1.surf = loadresimg(ind_panel1_png, ind_panel1_png_len);
	indpanel2.surf = loadresimg(ind_panel2_png, ind_panel2_png_len);
	extrapanel.surf = loadresimg(extra_panel_png, extra_panel_png_len);

	lampsurf[0] = loadresimg(lamp_off_png, lamp_off_png_len);
	lampsurf[1] = loadresimg(lamp_on_png, lamp_on_png_len);

	switchsurf[0] = loadresimg(switch_d_png, switch_d_png_len);
	switchsurf[1] = loadresimg(switch_u_png, switch_u_png_len);

	keysurf[0] = loadresimg(key_n_png, key_n_png_len);
	keysurf[1] = loadresimg(key_d_png, key_d_png_len);
	keysurf[2] = loadresimg(key_u_png, key_u_png_len);
#endif


	opgrid1.panel = &oppanel;
	opgrid1.xscl = opgrid1.panel->surf->w/90.0f;
	opgrid1.yscl = opgrid1.panel->surf->h/11.0f;
	opgrid1.yoff = opgrid1.yscl/2.0f;

	opgrid2.panel = &oppanel;
	opgrid2.xscl = opgrid1.xscl*1.76f;
	opgrid2.yscl = opgrid2.xscl;
	opgrid2.xoff = opgrid1.xscl*44.5f;
	opgrid2.yoff = opgrid1.panel->surf->h*2.4f/143.0f;

	iogrid.panel = &iopanel;
	iogrid.xscl = iogrid.panel->surf->w/44.0f;
	iogrid.yscl = iogrid.panel->surf->h/28.0f;

	indgrid1.panel = &indpanel1;
	indgrid1.xscl = indgrid1.panel->surf->w*5.0f/77.0f;
	indgrid1.yscl = indgrid1.panel->surf->h/12.0f;

	indgrid2.panel = &indpanel2;
	indgrid2.xscl = indgrid2.panel->surf->w/44.0f;
	indgrid2.yscl = indgrid2.panel->surf->h/11.0f;

	extragrid = indgrid1;
	extragrid.panel = &extrapanel;

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
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-t] [-d debugfile]\n", argv0);
	exit(1);
}

int
threadmain(int argc, char *argv[])
{
	SDL_Event ev;
	SDL_MouseButtonEvent *mbev;
	SDL_MouseMotionEvent *mmev;
	Element *e;
	int i;
	int w, h;
	const char *outfile;
	Channel *cmdchans[2];
	Task t;

	Apr *apr;
	Ptr *ptr;
	Ptp *ptp;
	Tty *tty;

	outfile = "/dev/null";
	ARGBEGIN{
	case 't':
		dotrace = 1;
		break;
	case 'd':
		outfile = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;

	if(debugfp = fopen(outfile, "w"), debugfp == nil){
		fprintf(stderr, "Can't open %s\n", outfile);
		exit(1);
	}

	SDL_Init(SDL_INIT_EVERYTHING);
	IMG_Init(IMG_INIT_PNG);

void main340(void);
//	main340();
//	return 0;

	if(SDL_CreateWindowAndRenderer(1399, 740, 0, &window, &renderer) < 0)
		err("SDL_CreateWindowAndRenderer() failed: %s\n", SDL_GetError());

	initpanel();

	findlayout(&w, &h);

	SDL_SetWindowSize(window, w, h);

	for(i = 0; i < 12; i++){
		data_digits[i].first = data_sw;
		data_digits[i].sw = &data_sw[i*3];
		data_digits[i].prev = &data_digits[(i+11) % 12];
		data_digits[i].next = &data_digits[(i+1) % 12];
	}
	for(i = 0; i < 6; i++){
		addr_digits[i].first = ma_sw;
		addr_digits[i].sw = &ma_sw[i*3];
		addr_digits[i].prev = &addr_digits[(i+5) % 6];
		addr_digits[i].next = &addr_digits[(i+1) % 6];
	}

	setcurdigit(data_digits);

	rtcchan = chancreate(sizeof(RtcMsg), 20);

	if(dofile("init.ini"))
		defaultconfig();
	apr = (Apr*)getdevice("apr");
	tty = (Tty*)getdevice("tty");
	ptr = (Ptr*)getdevice("ptr");
	ptp = (Ptp*)getdevice("ptp");

	if(apr == nil || tty == nil || ptr == nil || ptp == nil)
		err("need APR, TTY, PTR and PTP");

	cmdchans[0] = chancreate(sizeof(char*), 1);
	cmdchans[1] = chancreate(sizeof(void*), 1);
	t = (Task){ nil, readcmdchan, cmdchans, 10, 0 };
	addtask(t);
	threadcreate(simthread, nil);
	threadcreate(cmdthread, cmdchans);
	threadcreate(rtcthread, nil);

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
			case SDL_KEYDOWN:
				keydown(ev.key.keysym);
				break;
			case SDL_QUIT:
				quit(0);
			}

		updateapr(apr, ptr);
		updatetty(tty);
		updatept(ptp, ptr);

		SDL_SetRenderDrawColor(renderer, 0xe6, 0xe6, 0xe6, 0xff);
		SDL_RenderClear(renderer);

		drawpanel(renderer, &indpanel1);
		drawpanel(renderer, &indpanel2);
		drawpanel(renderer, &extrapanel);
		drawpanel(renderer, &iopanel);
		drawpanel(renderer, &oppanel);

		for(i = 0, e = lamps; i < nelem(lamps); i++, e++)
			drawelement(renderer, e, apr->sw_power);
		for(i = 0, e = keys; i < nelem(keys); i++, e++)
			drawelement(renderer, e, apr->sw_power);
		for(i = 0, e = switches; i < nelem(switches); i++, e++)
			drawelement(renderer, e, apr->sw_power);

		SDL_SetRenderDrawColor(renderer, 0xFF, 0, 0, 0xff);
		SDL_RenderDrawRect(renderer, &digitindline);

//		SDL_LockSurface(screen);
//		drawgrid(&opgrid1, screen, 0xFFFFFF00);
//		drawgrid(&opgrid2, screen, 0xFF0000FF);
//		drawgrid(&iogrid, screen, 0xFFFFFF00);
//		drawgrid(&indgrid1, screen, 0xFFFFFF00);
//		drawgrid(&indgrid2, screen, 0xFFFFFF00);
//		drawgrid(&extragrid, screen, 0xFFFFFF00);
//		SDL_UnlockSurface(screen);

		SDL_RenderPresent(renderer);
	}
	return 0;
}
