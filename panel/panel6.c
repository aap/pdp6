#include <stdarg.h>
#include <fcntl.h>
#include <SDL.h>
#include <SDL_image.h>
#include <pthread.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#define nelem(a) (sizeof(a)/sizeof(a[0]))
#define nil NULL

typedef uint64_t word;
typedef uint32_t hword;
typedef uint32_t u32;
typedef uint16_t u16;
typedef int8_t   i8;
typedef uint8_t  u8;
typedef unsigned char uchar;
typedef uchar bool;

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
setlights(uchar b, Element *l, int n)
{
	int i;
	for(i = 0; i < n; i++)
		l[i].state = !!(b & 1 << 7-i);
}

void
set18lights(uchar *b, Element *l)
{
	setlights(b[0], l, 8);
	setlights(b[1], l+8, 8);
	setlights(b[2], l+16, 2);
}

void
set36lights(uchar *b, Element *l)
{
	setlights(b[0], l, 8);
	setlights(b[1], l+8, 8);
	setlights(b[2], l+16, 8);
	setlights(b[3], l+24, 8);
	setlights(b[4], l+32, 4);
}

uchar
getswitches(Element *sw, int n, int state)
{
	uchar b;
	int i;
	b = 0;
	for(i = 0; i < n; i++)
		b |= (uchar)(sw[i].state == state) << 7-i;
	return b;
}

uchar
get18switches(uchar *b, Element *sw)
{
	b[0] = getswitches(sw, 8, 1);
	b[1] = getswitches(sw+8, 8, 1);
	b[2] = getswitches(sw+16, 2, 1);
}

uchar
get36switches(uchar *b, Element *sw)
{
	b[0] = getswitches(sw, 8, 1);
	b[1] = getswitches(sw+8, 8, 1);
	b[2] = getswitches(sw+16, 8, 1);
	b[3] = getswitches(sw+24, 8, 1);
	b[4] = getswitches(sw+32, 4, 1);
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

void*
talki2c(void *x)
{
	int fd;
	int i;
	int n;
	uchar msg[128], *p;

	fd = open("/dev/i2c-1", O_RDWR);
	if(fd < 0)
		err("Couldn't open I2C connection");
	if(ioctl(fd, I2C_SLAVE, 0x26) < 0)
		err("Couldn't select device");

	while(1){
		n = 0;
		msg[n++] = 0;	// address
		get18switches(&msg[n], ma_sw); n += 3;
		get36switches(&msg[n], data_sw); n += 5;
		msg[n++] = getswitches(misc_sw, 4, 1);
		msg[n++] = getswitches(extra_sw, 6, 1);
		msg[n++] = getswitches(keys, 8, 1);
		msg[n++] = getswitches(keys, 8, 2);
		write(fd, msg, n);

		p = msg;
		*p++ = 0;
		write(fd, msg, 1);
		p = msg;
		read(fd, p, 0x12);
		p += 0x12;

		read(fd, p, 0xF);
		p += 0xF;

		read(fd, p, 0xE);
		p += 0xE;

		n = 0;
		set18lights(&msg[n], ir_l); n += 3;
		set18lights(&msg[n], pc_l); n += 3;
		set36lights(&msg[n], mi_l); n += 5;
		set18lights(&msg[n], ma_l); n += 3;
		setlights(msg[n+0]<<1, pih_l, 7);
		setlights(msg[n+1]<<1, pir_l, 7);
		setlights(msg[n+2]<<1, pio_l, 7);
		misc_l[0].state = !!(msg[n+2] & 0x80);
		misc_l[1].state = !!(msg[n+1] & 0x80);
		misc_l[2].state = !!(msg[n+0] & 0x80);
		misc_l[3].state = !!(msg[n+3] & 0x80);
		misc_l[4].state = !!(msg[n+3] & 0x40);
		misc_l[5].state = !!(msg[n+3] & 0x20);
		misc_l[6].state = !!(msg[n+3] & 0x10);
		n += 4;
		set36lights(&msg[n], mb_l); n += 5;
		set36lights(&msg[n], ar_l); n += 5;
		set36lights(&msg[n], mq_l); n += 5;

		setlights(msg[n++], &ff_l[0], 8);
		setlights(msg[n++], &ff_l[8], 8);
		setlights(msg[n++], &ff_l[16], 8);
		setlights(msg[n++], &ff_l[24], 8);
		setlights(msg[n++], &ff_l[32], 8);
		setlights(msg[n++], &ff_l[40], 8);
		setlights(msg[n++], &ff_l[48], 8);
		setlights(msg[n++], &ff_l[56], 8);
		setlights(msg[n++], &ff_l[64], 8);
		setlights(msg[n++], &ff_l[72], 8);
		setlights(msg[n++], &ff_l[80], 8);
		setlights(msg[n++], &ff_l[88], 8);
		setlights(msg[n++], &ff_l[96], 8);
		setlights(msg[n++], &ff_l[104], 8);

		for(i = 0; i < n; i++)
			printf("%02X ", msg[i]);
		printf("\n");
	}

	close(fd);
}

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
	pthread_t comthread;

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

	pthread_create(&comthread, nil, talki2c, nil);

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
				SDL_Quit();
				return 0;
			}

		SDL_FillRect(screen, nil, SDL_MapRGBA(screen->format, 0xe6, 0xe6, 0xe6, 0xff));
		SDL_BlitSurface(indpanel1.surf, nil, screen, &indpanel1.pos);
		SDL_BlitSurface(indpanel2.surf, nil, screen, &indpanel2.pos);
		SDL_BlitSurface(extrapanel.surf, nil, screen, &extrapanel.pos);
		SDL_BlitSurface(iopanel.surf, nil, screen, &iopanel.pos);
		SDL_BlitSurface(oppanel.surf, nil, screen, &oppanel.pos);

		for(i = 0, e = lamps; i < nelem(lamps); i++, e++)
			drawelement(screen, e, 1);
		for(i = 0, e = keys; i < nelem(keys); i++, e++)
			drawelement(screen, e, 1);
		for(i = 0, e = switches; i < nelem(switches); i++, e++)
			drawelement(screen, e, 1);


		SDL_Flip(screen);
	}
	return 0;
}
