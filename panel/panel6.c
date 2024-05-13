#include <stdarg.h>
#include <fcntl.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <pthread.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "../emu/util.h"

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

SDL_Window *window;
SDL_Renderer *renderer;

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

#include "elements.inc"

void
setlights(uchar b, Element *l, int n)
{
	int i;
	for(i = 0; i < n; i++)
		l[i].state = !!(b & 1 << 7-i);
}

void
setlights_(uchar b, Element *l, int n)
{
	int i;
	for(i = 0; i < n; i++)
		l[i].state = !!(b & 1 << 5-i);
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
getswitches_(Element *sw, int n, int state)
{
	uchar b;
	int i;
	b = 0;
	for(i = 0; i < n; i++)
		b |= (uchar)(sw[i].state == state) << 5-i;
	return b;
}

void
setnlights(int b, Element *l, int n, int bit)
{
	int i;
	for(i = 0; i < n; i++, bit >>= 1)
		l[i].state = !!(b & bit);
}

int
getnswitches(Element *sw, int bit, int n, int state)
{
	int b, i;
	b = 0;
	for(i = 0; i < n; i++, bit >>= 1)
		if(sw[i].state == state)
			b |= bit;
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

	if(e->grid->panel == nil)
		return 0;

	p = xform(e->grid, e->pos);
	return x >= p.x && x <= p.x + e->surf[e->state]->w &&
	       y >= p.y && y <= p.y + e->surf[e->state]->h;
}

void
drawpanel(SDL_Renderer *renderer, Panel *p)
{
	if(p->pos.x < 0)
		return;

	p->pos.w = p->surf->w;
	p->pos.h = p->surf->h;
	SDL_RenderCopy(renderer, p->surf->tex, nil, &p->pos);
}


void
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

/* Show only operator console */
void
findlayout_small(int *w, int *h)
{
	SDL_Rect nopos = { -1, -1, -1, -1 };

	indpanel1.pos = nopos;
	indpanel2.pos = nopos;
	iopanel.pos = nopos;
	extrapanel.pos = nopos;

	iogrid.panel = nil;
	indgrid1.panel = nil;
	indgrid2.panel = nil;
	extragrid.panel = nil;

	oppanel.pos = (SDL_Rect){ 0, 0, 0, 0 };

	*w = oppanel.surf->w;
	*h = oppanel.surf->h;
}

void
talkserial(int fd)
{
	int i, n;
	char c, buf[32];

	while(1){
		misc_l[3].state = misc_sw[0].state;
		misc_l[4].state = misc_sw[1].state;
		misc_l[5].state = misc_sw[2].state;
		misc_l[6].state = misc_sw[3].state;

		/* Send switches to CPU */
		n = 0;
		buf[n++] = getswitches_(data_sw, 6, 1);
		buf[n++] = getswitches_(data_sw+6, 6, 1);
		buf[n++] = getswitches_(data_sw+12, 6, 1);
		buf[n++] = getswitches_(data_sw+18, 6, 1);
		buf[n++] = getswitches_(data_sw+24, 6, 1);
		buf[n++] = getswitches_(data_sw+30, 6, 1);
		buf[n++] = getswitches_(ma_sw, 6, 1);
		buf[n++] = getswitches_(ma_sw+6, 6, 1);
		buf[n++] = getswitches_(ma_sw+12, 6, 1);
		buf[n++] = getswitches_(keys, 6, 2);
		buf[n++] = getswitches_(keys, 6, 1);
		c = 0;
		c |= (keys[6].state == 2)<<3;
		c |= (keys[6].state == 1)<<2;
		c |= (keys[7].state == 2)<<1;
		c |= (keys[7].state == 1)<<0;
		buf[n++] = c;
		c = 0;
		c |= (misc_sw[1].state == 1)<<3;
		c |= (misc_sw[0].state == 1)<<2;
		c |= (misc_sw[3].state == 1)<<1;
		c |= (misc_sw[2].state == 1)<<0;
		buf[n++] = c;
		// TODO: speed knobs
		buf[n++] = 0;
		for(i = 0; i < n; i++)
			if(buf[i] != 077)
				buf[i] |= 0100;
		buf[n++] = '\r';
		write(fd, buf, n);


		/* Read lights from CPU if we get any */
		while(hasinput(fd)){
			if(read(fd, &c, 1) != 1)
				break;
			if(c >= '0' && c <= '7'){
				if(readn(fd, buf, 3))
					break;

				switch(c){
				case '0':
					setlights_(buf[0], ir_l, 6);
					setlights_(buf[1], ir_l+6, 6);
					setlights_(buf[2], ir_l+12, 6);
					break;
				case '1':
					setlights_(buf[0], mi_l, 6);
					setlights_(buf[1], mi_l+6, 6);
					setlights_(buf[2], mi_l+12, 6);
					break;
				case '2':
					setlights_(buf[0], mi_l+18, 6);
					setlights_(buf[1], mi_l+24, 6);
					setlights_(buf[2], mi_l+30, 6);
					break;
				case '3':
					setlights_(buf[0], pc_l, 6);
					setlights_(buf[1], pc_l+6, 6);
					setlights_(buf[2], pc_l+12, 6);
					break;
				case '4':
					setlights_(buf[0], ma_l, 6);
					setlights_(buf[1], ma_l+6, 6);
					setlights_(buf[2], ma_l+12, 6);
					break;
				case '5':
					misc_l[2].state = !!(buf[0] & 040);	// run
					setlights_(buf[0]<<2, pih_l, 4);
					setlights_(buf[1], pih_l+4, 3);
					break;
				case '6':
					misc_l[1].state = !!(buf[0] & 040);	// stop
					setlights_(buf[0]<<2, pir_l, 4);
					setlights_(buf[1], pir_l+4, 3);
					break;
				case '7':
					misc_l[0].state = !!(buf[0] & 040);	// pi
					setlights_(buf[0]<<2, pio_l, 4);
					setlights_(buf[1], pio_l+4, 3);
					break;
				}
			}
		}
	}
}

void
handlenetwork(int fd, void *x)
{
	nodelay(fd);
	talkserial(fd);
	close(fd);
}

void*
servethread(void *x)
{
	serve(2000, handlenetwork, nil);
}

#include "panel6.h"

void*
shmthread(void *x)
{
	int key = 666;
	int id;
	Panel6 *p;
	int sw;

	p = createseg("/tmp/pdp6_panel", sizeof(Panel6));
	if(p == nil)
		exit(1);

	// set switches from previous state
	setnlights(p->sw0, data_sw+18, 18, 0400000);
	setnlights(p->sw1, data_sw, 18, 0400000);
	setnlights(p->sw2, ma_sw, 18, 0400000);
	misc_sw[0].state = !!(p->sw3 & SW_REPEAT);
	misc_sw[1].state = !!(p->sw3 & SW_ADDR_STOP);
	misc_sw[2].state = !!(p->sw3 & SW_POWER);
	misc_sw[3].state = !!(p->sw3 & SW_MEM_DISABLE);

	for(;;) {
		p->sw0 = getnswitches(data_sw+18, 0400000, 18, 1);
		p->sw1 = getnswitches(data_sw, 0400000, 18, 1);
		p->sw2 = getnswitches(ma_sw, 0400000, 18, 1);
		sw = 0;
		if(keys[0].state == 1) sw |= KEY_START;
		if(keys[0].state == 2) sw |= KEY_READIN;
		if(keys[1].state == 1) sw |= KEY_INST_CONT;
		if(keys[1].state == 2) sw |= KEY_MEM_CONT;
		if(keys[2].state == 1) sw |= KEY_INST_STOP;
		if(keys[2].state == 2) sw |= KEY_MEM_STOP;
		if(keys[3].state == 1) sw |= KEY_IO_RESET;
		if(keys[3].state == 2) sw |= KEY_EXEC;
		if(keys[4].state == 1) sw |= KEY_DEP;
		if(keys[4].state == 2) sw |= KEY_DEP_NXT;
		if(keys[5].state == 1) sw |= KEY_EX;
		if(keys[5].state == 2) sw |= KEY_EX_NXT;
		if(misc_sw[0].state == 1) sw |= SW_REPEAT;
		if(misc_sw[1].state == 1) sw |= SW_ADDR_STOP;
		if(misc_sw[2].state == 1) sw |= SW_POWER;
		if(misc_sw[3].state == 1) sw |= SW_MEM_DISABLE;
		p->sw3 = sw;

		sw = 0;
		if(keys[6].state == 1) sw |= KEY_MOTOR_OFF;
		if(keys[6].state == 2) sw |= KEY_MOTOR_ON;
		if(keys[7].state == 1) sw |= KEY_PTP_FEED;
		if(keys[7].state == 2) sw |= KEY_PTR_FEED;
		p->sw4 = sw;

		setnlights(p->lights0, mi_l+18, 18, 0400000);
		setnlights(p->lights1, mi_l, 18, 0400000);
		setnlights(p->lights2, ma_l, 18, 0400000);
		setnlights(p->lights3, ir_l, 18, 0400000);
		setnlights(p->lights4, pc_l, 18, 0400000);

		misc_l[0].state = !!(p->lights5 & L5_PI_ON);
		misc_l[1].state = !!(p->lights5 & L5_MC_STOP);
		misc_l[2].state = !!(p->lights5 & L5_RUN);
		misc_l[3].state = !!(p->lights5 & L5_REPEAT);
		misc_l[4].state = !!(p->lights5 & L5_ADDR_STOP);
		misc_l[5].state = !!(p->lights5 & L5_POWER);
		misc_l[6].state = !!(p->lights5 & L5_MEM_DISABLE);
		setnlights(p->lights5, pio_l, 7, 0100);

		setnlights(p->lights6, pir_l, 7, 0100);
		setnlights(p->lights6, pih_l, 7, 020000);

		usleep(30);
	}
}

void*
openserial(void *x)
{
	int fd;

	fd = open("/tmp/panel", O_RDWR);
	if(fd < 0)
		err("Couldn't open serial connection");
	talkserial(fd);
	close(fd);
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

		read(fd, p, 0x5);
		p += 0x5;

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

		setlights(msg[n++], pr_l, 8);
		setlights(msg[n++], rlr_l, 8);
		setlights(msg[n++], rla_l, 8);

		setlights(msg[n++]<<1, tty_l, 7);
		setlights(msg[n++], ttibuf_l, 8);

		for(i = 0; i < n; i++)
			printf("%02X ", msg[i]);
		printf("\n");
	}

	close(fd);
}

int
main(int argc, char *argv[])
{
	SDL_Event ev;
	SDL_MouseButtonEvent *mbev;
	SDL_MouseMotionEvent *mmev;
	Element *e;
	int i;
	int w, h;
	pthread_t comthread;

	SDL_Init(SDL_INIT_EVERYTHING);
	IMG_Init(IMG_INIT_PNG);

	if(SDL_CreateWindowAndRenderer(1399, 200, 0, &window, &renderer) < 0)
		err("SDL_CreateWindowAndRenderer() failed: %s\n", SDL_GetError());

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

//	findlayout(&w, &h);
	findlayout_small(&w, &h);

	SDL_SetWindowSize(window, w, h);

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

//	pthread_create(&comthread, nil, talki2c, nil);
//	pthread_create(&comthread, nil, servethread, nil);
//	pthread_create(&comthread, nil, talkserial, nil);
	pthread_create(&comthread, nil, shmthread, nil);

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

		SDL_SetRenderDrawColor(renderer, 0xe6, 0xe6, 0xe6, 0xff);
		SDL_RenderClear(renderer);

		drawpanel(renderer, &indpanel1);
		drawpanel(renderer, &indpanel2);
		drawpanel(renderer, &extrapanel);
		drawpanel(renderer, &iopanel);
		drawpanel(renderer, &oppanel);

		for(i = 0, e = lamps; i < nelem(lamps); i++, e++)
			drawelement(renderer, e, 1);
		for(i = 0, e = keys; i < nelem(keys); i++, e++)
			drawelement(renderer, e, 1);
		for(i = 0, e = switches; i < nelem(switches); i++, e++)
			drawelement(renderer, e, 1);

		SDL_RenderPresent(renderer);
		SDL_Delay(30);
	}
	return 0;
}
