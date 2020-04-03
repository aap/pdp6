#include "pdp6.h"
#include <SDL2/SDL.h>

//#define JUSTTESTING

char *dis_ident = DIS_IDENT;


static SDL_Window *window;
static SDL_Renderer *renderer;

#define LDB(p, s, w) ((w)>>(p) & (1<<(s))-1)
#define XLDB(ppss, w) LDB((ppss)>>6 & 077, (ppss)&077, w)
#define MASK(p, s) ((1<<(s))-1 << (p))
#define DPB(b, p, s, w) ((w)&~MASK(p,s) | (b)<<(p) & MASK(p,s))
#define XDPB(b, ppss, w) DPB(b, (ppss)>>6 & 077, (ppss)&077, w)

enum Modes
{
	PM,
	XYM,
	SM,
	CM,
	VM,
	VCM,
	IM,
	SBM
};

#ifdef JUSTTESTING

#define PARAM_W(m, lp, s, sc, in) \
	((in) |\
	(sc)<<4 |\
	(s)<<9 |\
	(lp)<<11 |\
	(m)<<13)

#define XYM_W(p, m, lp, i, xy) \
	((xy) |\
	(i)<<10 |\
	(lp)<<11 |\
	(m)<<13 |\
	(p)<<16)

#define VM_W(e, i, y, x) \
	((x) |\
	(y)<<8 |\
	(i)<<16 |\
	(e)<<17)

#define IM_W(e, i, p1, p2, p3, p4) \
	((p4) |\
	(p3)<<4 |\
	(p2)<<8 |\
	(p1)<<12 |\
	(i)<<16 |\
	(e)<<17)

static word list[] = {
	PARAM_W(XYM, 0, 0, 4, 017),
	XYM_W(1, XYM, 0, 0, 100),
	XYM_W(0, IM, 0, 1, 100),
	IM_W(1, 1, 012, 012, 012, 012),
	PARAM_W(VM, 0, 0, 0, 0),
	VM_W(1, 1, 50, 20),
	PARAM_W(XYM, 0, 0, 07, 0),
	XYM_W(1, XYM, 0, 0, 950),
	XYM_W(0, CM, 0, 0, 0),
	036<<12 | 034<<6 | 040,
	01<<12 | 02<<6 | 03,
	04<<12 | 05<<6 | 06,
	07<<12 | 010<<6 | 011,
	012<<12 | 013<<6 | 014,
	015<<12 | 016<<6 | 017,
	020<<12 | 034<<6 | 033,
	040<<12 | 021<<6 | 022,
	023<<12 | 024<<6 | 025,
	026<<12 | 027<<6 | 030,
	031<<12 | 032<<6 | 0,
	033<<12 | 034<<6 | 040,
	041<<12 | 042<<6 | 043,
	044<<12 | 045<<6 | 046,
	047<<12 | 050<<6 | 051,
	052<<12 | 053<<6 | 054,
	055<<12 | 056<<6 | 057,
	033<<12 | 034<<6 | 040,
	060<<12 | 061<<6 | 062,
	063<<12 | 064<<6 | 065,
	066<<12 | 067<<6 | 070,
	071<<12 | 072<<6 | 073,
	074<<12 | 075<<6 | 076,
	077<<12 | 037<<6,
	PARAM_W(0, 1, 3, 0, 0),
	~0
};
static word *listp = list;

#endif

typedef struct Pixel Pixel;
struct Pixel
{
	u16 x, y;
	u8 l;
};

typedef struct CRT CRT;
struct CRT
{
	Pixel pxlist[1024*1024];
	int numPixels;
	int phos1List[1024*1024];
	int numPhos1;
	int phos2List[1024*1024];
	int numPhos2;
	int clearList[1024*1024];
	int numClearPixels;
	int tmpList[1024*1024];
	int numTmp;

	u8 pxvals[1024*1024];	// intensity
	u8 phos1vals[1024*1024];
	u8 phos2vals[1024*1024];

	u32 pixels[1024*1024];	// RGB values
};

typedef struct Dis340 Dis340;
struct Dis340
{
	/* 344 interface for PDP-6.
	 * no schematics unfortunately */
	Device dev;
	IOBus *bus;
	int pia_data;
	int pia_spec;
	word ib;	/* interface buffer */
	int ibc;	/* number of words in IB */

	/* 340 display and 342 char gen */
	int intdelay;	/* has an intensify delay been triggered */
	hword br;	/* 18 bits */
	int mode;	/* 3 bits */
	int i;		/* intensity - 3 bits */
	int sz;		/* scale - 2 bits */
	int brm;	/* 7 bits */
	int s;		/* shift - 4 bits */
	int x, y;	/* 10 bits */
	/* flip flops */
	int stop;
	int halt;
	int move;
	int lp_find;
	int lp_flag;
	int lp_enable;
	int rfd;
	int hef, vef;
	/* br shifted for IM and CM */
	int pnts;
	int chrs;
	/* values for BRM counting */
	int brm_add;
	int brm_comp;

	/* 342 char gen */
	int cgstate;	/* 0 - inactive
			 * 1 - drawing character */
	int shift;
	int *cp;	/* current char pointer */

	/* simulated tube */
	CRT *crt;
};

static void plotpixel(CRT *crt, int x, int y, int intensity);

static void
intensify(Dis340 *dis)
{
	plotpixel(dis->crt, dis->x, 1023-dis->y, dis->i);
}

static void
rfd(Dis340 *dis)
{
	dis->rfd = 1;
	dis->hef = 0;
	dis->vef = 0;
}

static void
escape(Dis340 *dis)
{
	dis->mode = 0;
	dis->lp_find = 0;
}

static void
initiate(Dis340 *dis)
{
	dis->intdelay = 0;
	dis->cgstate = 0;

	dis->shift = 0;

	dis->lp_enable = 0;	// TODO: not in schematics?
	dis->lp_find = 0;
	dis->lp_flag = 0;
	dis->stop = 0;
	escape(dis);
	rfd(dis);
}

static void
read_to_mode(Dis340 *dis)
{
	dis->mode = LDB(13, 3, dis->br);
	if((dis->br & 0010000) == 0010000)
		dis->lp_enable = !!(dis->br & 04000);
}

static void
shift_s(Dis340 *dis)
{
	dis->s >>= 1;
	dis->pnts <<= 4;
	dis->chrs <<= 6;
}

static void
count_brm(Dis340 *dis)
{
	dis->brm = ((dis->brm+dis->brm_add) ^ dis->brm_comp) & 0177;
	shift_s(dis);
}

static void
count_x(Dis340 *dis, int s, int r, int l)
{
	if(r) dis->x += s;
	if(l) dis->x -= s;
	if(r || l) dis->move = 1;
	if(dis->x >= 1024 || dis->x < 0) dis->hef = 1;
	dis->x &= 01777;
	if(dis->mode == IM && dis->s&1)
		dis->halt = 1;
	if(dis->mode == CM)
		dis->move = 0;
}

static void
count_y(Dis340 *dis, int s, int u, int d)
{
	if(u) dis->y += s;
	if(d) dis->y -= s;
	if(u || d) dis->move = 1;	// actually in count_x
	if(dis->y >= 1024 || dis->y < 0) dis->vef = 1;
	dis->y &= 01777;
	dis->lp_flag |= dis->lp_find && dis->lp_enable;
}

static void
getdir(int brm, int xy, int c, int *pos, int *neg)
{
	int dir;
	int foo;

	foo = ~brm & c;
	dir =	foo&0001 && xy&0100 ||
		foo&0002 && xy&0040 ||
		foo&0004 && xy&0020 ||
		foo&0010 && xy&0010 ||
		foo&0020 && xy&0004 ||
		foo&0040 && xy&0002 ||
		foo&0100 && xy&0001;
	*pos = 0;
	*neg = 0;
	if(dir){
		if(xy & 0200)
			*neg = 1;
		else
			*pos = 1;
	}
}

#include "chargen.inc"

static void
nextchar(Dis340 *dis)
{
	dis->cgstate = 0;
	if(dis->s & 1)
		rfd(dis);
	else{
		shift_s(dis);
		dis->intdelay = 1;
	}
}

static void
cg_count(Dis340 *dis)
{
	int pnt;
	int u, d, l, r;

	pnt = *dis->cp++;

	if(pnt == 0){
		/* end of character */
end:
		nextchar(dis);
		return;
	}

	if(pnt & 040){
		/* escape from char */
		dis->cgstate = 0;
		escape(dis);
		rfd(dis);
		return;
	}
	if(pnt & 0100)
		dis->x = 0;
	if(pnt & 0200){
		dis->shift = 0;
		goto end;
	}
	if(pnt & 0400){
		dis->shift = 64;
		goto end;
	}
	r = (pnt & 014) == 010;
	l = (pnt & 014) == 014;
	u = (pnt & 003) == 002;
	d = (pnt & 003) == 003;
	count_y(dis, 1<<dis->sz, u, d);
	count_x(dis, 1<<dis->sz, r, l);
	/* 1μs after count */
	if(pnt & 020)
		intensify(dis);
	/* 0.5μs delay */
}

/* return whether to loop */
static void
idp(Dis340 *dis)
{
	uchar pnt;
	int brm, c;
	int u, d, l, r;
	int halt;

	dis->intdelay = 0;
	switch(dis->mode){
	case PM:
		// TODO: what is RI?
		read_to_mode(dis);
		/* PM PULSE */
		if(dis->br & 010) dis->i = LDB(0, 3, dis->br);
		if(dis->br & 0100) dis->sz = LDB(4, 2, dis->br);
		if(dis->br & 02000) dis->stop = 1;
		else rfd(dis);
		break;

	case XYM:
		if(dis->lp_flag)
			break;
		read_to_mode(dis);
		if(dis->br & 0200000){
			dis->y = 0;	// CLEAR Y
			dis->y |= dis->br & 01777;	// LOAD Y
			/* if intensify 35μs delay */
		}else{
			/* 35μs delay */
			dis->x = 0;	// CLEAR X
			dis->x |= dis->br & 01777;	// LOAD X
		}
		/* 0.5μs delay */
		if(dis->br & 0002000)
			intensify(dis);
		rfd(dis);
		break;

	case CM:
		/* Tell CG to start */
		dis->cp = chars[LDB(12, 6, dis->chrs) + dis->shift];
		dis->cgstate = 1;
		break;

	case IM:
		halt = dis->halt;
		count_brm(dis);
		pnt = LDB(16, 4, dis->pnts);
		r = (pnt & 014) == 010;
		l = (pnt & 014) == 014;
		u = (pnt & 003) == 002;
		d = (pnt & 003) == 003;
		goto count;

	case VM:
	case VCM:
		halt = dis->halt;
		brm = dis->brm;
		count_brm(dis);
		/* actually the complement inputs to COUNT BRM
		 * but it's easier to find them out after the addition. */
		c = dis->brm^brm;
		getdir(brm, LDB(0, 8, dis->br), c, &r, &l);
		getdir(brm, LDB(8, 8, dis->br), c, &u, &d);


count:
		count_y(dis, 1<<dis->sz, u, d);
		count_x(dis, 1<<dis->sz, r, l);
		if(halt){
			/* at COUNT X */
			if(dis->br & 0400000)
				escape(dis);
			rfd(dis);
		}else{
			/* delay at COUNT BRM */
			if(dis->mode == VM && dis->brm == 0177)
				dis->halt = 1;
			if(dis->hef || dis->vef){
				escape(dis);
				if(dis->mode == VCM)
					rfd(dis);
			}else if(!dis->rfd && !dis->lp_flag)
				dis->intdelay = 1;
		}
		break;

	/* unsupported */
	case SM:
	case SBM:
		break;
	}
}

/* figure out add and complement values for COUNT BRM */
static void
initbrm(Dis340 *dis)
{
	int m;
	int dx, dy, d;

	dis->brm_add = 1;

	dx = LDB(0, 7, dis->br);
	dy = LDB(8, 7, dis->br);
	d = dx > dy ? dx : dy;
	for(m = 0100; m > 040; m >>= 1)
		if(d & m)
			break;
		else
			dis->brm_add *= 2;
	dis->brm_comp = dis->brm_add - 1;
}

static void
data_sync(Dis340 *dis)
{
	// CLR BR
	dis->br = 0;
	dis->rfd = 0;

	// CLR BRM
	dis->brm = 0;
	dis->halt = 0;
	dis->move = 0;
	dis->s = 0;	// CLEAR S
	dis->hef = 0;	// CLR CF
	dis->vef = 0;	// CLR CF

	/* 2.8μs */

	// READ TO S
	dis->lp_flag |= dis->lp_find && dis->lp_enable;
	if(dis->mode == IM) dis->s |= 010;
	if(dis->mode == CM) dis->s |= 004;

	// LOAD BR
	dis->br |= dis->ib>>18 & RT;
//printf("DIS: %06o\n", dis->br);

	dis->pnts = dis->br;
	dis->chrs = dis->br;
	initbrm(dis);

	// SHIFT IB
	dis->ib = dis->ib<<18 & LT;
	dis->ibc--;

	dis->intdelay = 1;
}

static void
intdelay(Dis340 *dis)
{
	/* 0.5μs intensify delay */
	if(dis->move && dis->br&0200000)
		intensify(dis);
	/* pulse after delay */
	idp(dis);
}

#ifdef JUSTTESTING

static void*
disthread(void *arg)
{
	Dis340 *dis;
	word w;

	dis = arg;

	initiate(dis);

	for(;;){
		if(dis->rfd){
			w = *listp++;
			if(w == ~0)
				listp = list;
			else{
				dis->ib = w << 18;
				dis->ibc = 1;
				data_sync(dis);
			}
		}
		if(dis->intdelay)
			intdelay(dis);
		if(dis->cgstate)
			cg_count(dis);
	}
	return nil;
}

#endif

/*
 * CRT simulation ported from https://www.masswerk.at/minskytron/
 */

typedef struct Color Color;
struct Color
{
	u8 r, g, b, a;
};

static Lock crtlock;

static Color phos1 = { 0x3d, 0xaa, 0xf7, 0xff };
static Color phos1blur = { 0, 0x63, 0xeb, 0xff };
static Color phos2 = { 0x79, 0xcc, 0x3e, 0xff };
static Color phos2aged = { 0x7e, 0xba, 0x1e, 0xff };
#define RGBA(r, g, b, a) ((r)<<24 | (g)<<16 | (b)<<8 | (a))
#define RGB(r, g, b) RGBA(r, g, b, 255);
static u8 pxintensity[8];
static Color phos1ramp[256];
static Color phos2ramp[256];
static u32 bg = 0;

static float pixelSustainPercent = 0.935;
static int pixelSustainMin = 6;
static int pixelSustainMax = 144;
static float pixelSustainSensitivity = 1.715;
static float pixelSustainRangeOffset = 0.45;
static float pixelSustainFactor;
static float pixelSustainFactorMin = 0.8;
static float pixelSustainFactorMax = 1.0;

static int blurLevelsDefault = 4;
static float blurFactorSquare = 0.3;
static float blurFactorLinear = 0.024;
static float blurFactorDiagonal = 0.3;

static int phos2AlphaCorr = 25;
static int phos2MinVal;
static int pixelIntensityMax = 240;
static int pixelIntensityMin = 8;
static int pixelHaloMinIntensity = 28;
static int intensityRange;
static int intensityOffset;

static float sustainFuzzyness = 0.015;
static float sustainFactor;
static int sustainTransferCf;

float frand(void) { return (double)rand()/RAND_MAX; }

static void
crtinit(void)
{
	int i, j, ofs;
	Color col1, col2;
	float c;
	int psm;

	intensityRange = pixelIntensityMax - pixelIntensityMin;
	intensityOffset = pixelIntensityMin+(1.0-pixelSustainRangeOffset)*intensityRange;
	sustainTransferCf = intensityOffset * pixelSustainSensitivity;
	pixelSustainFactor = pixelSustainFactorMin +
		pixelSustainPercent*(pixelSustainFactorMax-pixelSustainFactorMin);
	sustainFactor = pixelSustainFactor;


	/* Intensities */
	for(i = 0; i < 8; i++)
		pxintensity[i] = pixelIntensityMin + i/7.0 * intensityRange;

	/* Phosphor 1 ramp */
	ofs = pxintensity[4]/4;
	for(i = 0; i < ofs; i++){
		phos1ramp[i] = phos1blur;
		phos1ramp[i].a = i < pixelHaloMinIntensity ?
			pixelHaloMinIntensity : i;
	}
	for(j = 0; i < 256; i++, j++){
		c = (float)j/(255-ofs);
		c *= c;
		phos1ramp[i].r = phos1blur.r*(1.0-c) + phos1.r*c;
		phos1ramp[i].g = phos1blur.g*(1.0-c) + phos1.g*c;
		phos1ramp[i].b = phos1blur.b*(1.0-c) + phos1.b*c;
		phos1ramp[i].a = i < pixelHaloMinIntensity ?
			pixelHaloMinIntensity : i;
	}

	/* Phosphor 2 ramp */
	col1 = phos2;
	col2 = phos2aged;

	c = (255-phos2AlphaCorr)/255.0;
	col1.r *= c; col1.g *= c; col1.b *= c;
	c = (255-phos2AlphaCorr*0.25)/255.0;
	col2.r *= c; col2.g *= c; col2.b *= c;
	psm = pixelSustainMax + phos2AlphaCorr;

	phos2MinVal = 0;
	for(i = 0; i < 256; i++){
		c = i/255.0;
		phos2ramp[i].r = col1.r*c + col2.r*(1.0-c);
		phos2ramp[i].g = col1.g*c + col2.g*(1.0-c);
		phos2ramp[i].b = col1.b*c + col2.b*(1.0-c);
		phos2ramp[i].a = psm*(c + -0.5*(cos(M_PI*c) - 1))/2.0;
		if(phos2ramp[i].a < 3)
			phos2MinVal = i;
	}
}

/* Queue pixel to display */
static void
plotpixel(CRT *crt, int x, int y, int intensity)
{
	int i, v;
	Pixel *px;

	intensity = pxintensity[intensity];
	i = 1024*y + x;
	v = crt->pxvals[i];

	lock(&crtlock);
	if(v == 0){
		/* Add new pixel */
		px = &crt->pxlist[crt->numPixels++];
		px->x = x;
		px->y = y;
		px->l = blurLevelsDefault;
		crt->pxvals[i] = intensity;
	}else if(intensity > v)
		/* Intensify old pixel */
		crt->pxvals[i] = intensity;
	unlock(&crtlock);
}


/* Intensify phos1 value */
static void
renderpixel(CRT *crt, int x, int y, int a, int level)
{
	int p, v, b;
	float f;

	p = y*1024 + x;
	v = crt->phos1vals[p];
	if(v >= 255) return;
	if(v == 0){
//		printf("adding phos1 %d\n", p);
		crt->phos1List[crt->numPhos1++] = p;
	}
	v += a;
	crt->phos1vals[p] = v > 255 ? 255 : v;

	if(--level){
		f = a/255.0;
		b = f*f*255*blurFactorSquare + a*blurFactorLinear;
		if(b > 0){
			if(x < 1023) renderpixel(crt, x+1, y, b, level);
			if(y < 1023) renderpixel(crt, x, y+1, b, level);
			if(x > 0) renderpixel(crt, x-1, y, b, level);
			if(y > 0) renderpixel(crt, x, y-1, b, level);
			b *= blurFactorDiagonal;
			if(b > 0){
				if(y > 0){
					if(x < 1023)
						renderpixel(crt, x+1, y-1, b, level);
					if(x > 0)
						renderpixel(crt, x-1, y-1, b, level);
				}
				if(y < 1023){
					if(x < 1023)
						renderpixel(crt, x+1, y+1, b, level);
					if(x > 0)
						renderpixel(crt, x-1, y+1, b, level);
				}
			}
		}
	}
}

/* Intensify all phos1 values from queued pixels */
static void
renderpixels(CRT *crt)
{
	int i, p;
	Pixel *px;

	lock(&crtlock);
	px = crt->pxlist;
	for(i = 0; i < crt->numPixels; i++){
		p = 1024*px->y + px->x;
		renderpixel(crt, px->x, px->y, crt->pxvals[p], px->l);
		crt->pxvals[p] = 0;
		px++;
	}
	crt->numPixels = 0;
	unlock(&crtlock);
}

static void
drawtoscreen(CRT *crt, int i, int phos2, int phos1)
{
	float c;
	int r, g, b, a;
	Color p1col = phos1ramp[phos1];
	Color p2col = phos2ramp[phos2];

	if(phos2 == 0 || phos1 == 255){
		/* only Phosphor 1 */
		p1col.a = (phos2 == 0 && phos1 < pixelHaloMinIntensity) ?
			pixelHaloMinIntensity : phos1;
		crt->pixels[i] = RGBA(p1col.r, p1col.g, p1col.b, p1col.a);
	}else{
		/* both phosphors */
		c = p2col.a/255.0 * (1.0 - phos1/255.0);
		r = (p1col.r + p2col.r*c + p1col.r + p2col.r)/2.0;
		g = (p1col.g + p2col.g*c + p1col.g + p2col.g)/2.0;
		b = (p1col.b + p2col.b*c + p1col.b + p2col.b)/2.0;
		a = phos1 + p2col.a;
		if(r > 255) r = 255;
		if(g > 255) g = 255;
		if(b > 255) b = 255;
		if(a > 255) a = 255;
		crt->pixels[i] = RGBA(r, g, b, a);
	}
}

static void
render(CRT *crt)
{
	int i, p, v, u;
	float f;
	Color c;

	renderpixels(crt);

	/* clear faded phos1 pixels */
	for(i = 0; i < crt->numClearPixels; i++)
		crt->pixels[crt->clearList[i]] = bg;
	crt->numClearPixels = 0;

	/* process phosphor 2 */
	for(i = 0; i < crt->numPhos2; i++){
		p = crt->phos2List[i];
		c = phos2ramp[crt->phos2vals[p]];
		crt->pixels[p] = RGBA(c.r, c.g, c.b, c.a);
	}

	/* process phosphor 1 */
	for(i = 0; i < crt->numPhos1; i++){
		p = crt->phos1List[i];
		v = crt->phos1vals[p];
		if(v >= pixelSustainMin){
			/* Phos1 bright enough, leave a Phos2 trace */
			if(crt->phos2vals[p] == 0)
				crt->phos2List[crt->numPhos2++] = p;
			f = v >= intensityOffset ?
				1.0 :
				1.0 - (float)(intensityOffset-v)/sustainTransferCf;
			u = f*255;
			if(crt->phos2vals[p] < u)
				crt->phos2vals[p] = u;
		}else if(crt->phos2vals[p] == 0)
			/* Not bright enough,
			 * clear pixel if phos2 wasn't drawn */
			crt->clearList[crt->numClearPixels++] = p;
		/* Apply pixel with phos1 and phos2 values */
		drawtoscreen(crt, p, crt->phos2vals[p], v);
		crt->phos1vals[p] = 0;
	}
	crt->numPhos1 = 0;

	/* fade phosphor 2 */
	crt->numTmp = 0;
	for(i = 0; i < crt->numPhos2; i++){
		p = crt->phos2List[i];
		v = crt->phos2vals[p];
		v *= sustainFactor*(1.0-(frand()-0.5)*sustainFuzzyness);
		if(v >= phos2MinVal){
			crt->phos2vals[p] = v;
			crt->tmpList[crt->numTmp++] = p;
		}else{
			crt->phos2vals[p] = 0;
			crt->pixels[p] = bg;
		}
	}
	memcpy(crt->phos2List, crt->tmpList, crt->numTmp*sizeof(int));
	crt->numPhos2 = crt->numTmp;
}

static void*
renderthread(void *arg)
{
	Dis340 *dis;
	SDL_Event ev;
	SDL_Texture *tex;

	dis = arg;

	crtinit();

	if(SDL_CreateWindowAndRenderer(1024, 1024, 0, &window, &renderer) < 0)
		err("SDL_CreateWindowAndRenderer() failed: %s\n", SDL_GetError());
	SDL_SetWindowTitle(window, "Type 340 display");
	tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_STREAMING, 1024, 1024);
	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

	lock(&initlock);
	awaitinit--;
	unlock(&initlock);

	for(;;){
#ifdef JUSTTESTING
		while(SDL_PollEvent(&ev))
			switch(ev.type){
			case SDL_KEYDOWN:
				if(ev.key.keysym.scancode == SDL_SCANCODE_Q)
					exit(0);
				else
					initiate(dis);
				break;
			}
#endif
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
		SDL_RenderClear(renderer);

		render(dis->crt);
//		SDL_Delay(30);

		SDL_UpdateTexture(tex, nil, dis->crt->pixels, 1024*sizeof(u32));
		SDL_RenderCopy(renderer, tex, nil, nil);

		SDL_RenderPresent(renderer);
	}
	return nil;
}

#ifdef JUSTTESTING
void
main340(void)
{
	static Dis340 DIS_;
	static CRT CRT_;
	static Dis340 *dis = &DIS_;

	dis->crt = &CRT_;

	threadcreate(disthread, dis);
	threadcreate(renderthread, dis);
for(;;);
}
#endif


static void
recalc_dis_req(Dis340 *dis)
{
	int data, spec;
	u8 req_data, req_spec;

// TODO: what about RFD?
	data = dis->ibc == 0 && dis->rfd;
	spec = dis->lp_flag ||
		dis->hef || dis->vef ||
		dis->stop && dis->br&01000;
	req_data = data ? 0200 >> dis->pia_data : 0;
	req_spec = spec ? 0200 >> dis->pia_spec : 0;

	setreq2(dis->bus, DIS, req_data | req_spec);
}

static void
discycle(void *p)
{
	Dis340 *dis;

	dis = p;
	if(dis->rfd && dis->ibc > 0)
		data_sync(dis);
	if(dis->intdelay)
		intdelay(dis);
	if(dis->cgstate)
		cg_count(dis);
	recalc_dis_req(dis);
}

static void
wake_dis(void *dev)
{
	Dis340 *dis;
	IOBus *bus;

	dis = dev;
	bus = dis->bus;
	if(IOB_RESET){
		dis->pia_data = 0;
		dis->pia_spec = 0;

		dis->ib = 0;
		dis->ibc = 0;

		initiate(dis);
	}

	if(bus->devcode == DIS){
		if(IOB_STATUS){
			if(dis->lp_flag) bus->c12 |= F25;
			if(dis->hef || dis->vef) bus->c12 |= F26;
			if(dis->stop && dis->br&01000) bus->c12 |= F27;
// TODO: what about RFD?
			if(dis->ibc == 0 && dis->rfd) bus->c12 |= F28;
			bus->c12 |= (dis->pia_spec & 7) << 3;
			bus->c12 |= dis->pia_data & 7;
//printf("DIS STATUS %012lo\n", bus->c12);
		}
		if(IOB_DATAI){
			bus->c12 |= dis->x;
			bus->c12 |= dis->y << 18;
//printf("DIS DATAI %012lo\n", bus->c12);
		}
		if(IOB_CONO_CLEAR){
//printf("DIS CONO CLEAR\n");
			dis->pia_data = 0;
			dis->pia_spec = 0;
		}
		if(IOB_CONO_SET){
			dis->pia_data |= bus->c12 & 7;
			dis->pia_spec |= bus->c12>>3 & 7;

			if(bus->c12 & F29)
				initiate(dis);
//printf("DIS CONO SET %012lo\n", bus->c12);
		}
		if(IOB_DATAO_CLEAR){
//printf("DIS DATAO CLEAR\n");
			dis->ib = 0;
			dis->ibc = 0;
		}
		if(IOB_DATAO_SET){
			dis->ib |= bus->c12;
			dis->ibc = 2;
//printf("DIS DATAO SET %012lo\n", bus->c12);
		}
	}
	recalc_dis_req(dis);
}

static void
disioconnect(Device *dev, IOBus *bus)
{
	Dis340 *dis;
	dis = (Dis340*)dev;
	dis->bus = bus;
	bus->dev[DIS] = (Busdev){ dis, wake_dis, 0 };
}


static Device disproto = {
	nil, nil, "",
	nil, nil,
	disioconnect,
	nil, nil
};

Device*
makedis(int argc, char *argv[])
{
	Dis340 *dis;
	Task t;

	dis = malloc(sizeof(Dis340));
	memset(dis, 0, sizeof(Dis340));
	dis->crt = malloc(sizeof(CRT));
	memset(dis->crt, 0, sizeof(CRT));

	dis->dev = disproto;
	dis->dev.type = dis_ident;

	/* dunno about the frequency here */
//	t = (Task){ nil, discycle, dis, 50, 0 };
	t = (Task){ nil, discycle, dis, 20, 0 };
	addtask(t);

	lock(&initlock);
	awaitinit++;
	unlock(&initlock);

	/* There's a race somewhere here */
	threadcreate(renderthread, dis);

	return &dis->dev;
}
