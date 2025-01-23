#include "pdp6.h"
#include <unistd.h>
#include <signal.h>
#include <SDL.h>
#include <unistd.h>

char *joy_ident = JOY_IDENT;
char *ojoy_ident = OJOY_IDENT;

static int fd;

typedef struct Joy Joy;
struct Joy
{
	Device dev;
	IOBus *bus;
};


typedef struct OJoy OJoy;
struct OJoy
{
	Device dev;
	IOBus *bus;
};


#define HAVE_GAMECONTROLLERFROMINSTANCEID \
  ((SDL_MAJOR_VERSION > 2) || (SDL_MAJOR_VERSION == 2 && \
   (SDL_MINOR_VERSION > 0) || (SDL_PATCHLEVEL >= 4)))

static void
joyinit(void)
{
  SDL_Joystick *y;
  int i, n;

  SDL_InitSubSystem(SDL_INIT_JOYSTICK);
#if HAVE_GAMECONTROLLERFROMINSTANCEID
  SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
#endif

  if (SDL_JoystickEventState (SDL_ENABLE) < 0) {
    fprintf (stderr, "Joystick event state failure\r\n");
    return;
  }

#if HAVE_GAMECONTROLLERFROMINSTANCEID
  if (SDL_GameControllerEventState (SDL_ENABLE) < 0) {
    fprintf (stderr, "Game controller event state failure\r\n");
    return;
  }
#endif

  n = SDL_NumJoysticks();

  fprintf (stderr, "Controllers: %d\r\n", n);

  for (i = 0; i < n; i++) {
#if HAVE_GAMECONTROLLERFROMINSTANCEID
    SDL_GameController *x;
    if (SDL_IsGameController (i)) {
      x = SDL_GameControllerOpen (i);
      if (x == NULL) {
        fprintf (stderr, "Controller %d FAIL\r\n", i);
      } else {
        fprintf (stderr, "Controller %d OK\r\n", i);
        printf("Name: %s\r\n", SDL_GameControllerNameForIndex(i));
      }
    } else
#endif
    {
      y = SDL_JoystickOpen (i);
      if (y == NULL) {
        fprintf (stderr, "Joystick %d FAIL\r\n", i);
      } else {
        fprintf (stderr, "Joystick %d OK\r\n", i);
        printf("Name: %s\r\n", SDL_JoystickNameForIndex(i));
        printf("Number of Axes: %d\r\n", SDL_JoystickNumAxes(y));
        printf("Number of Buttons: %d\r\n", SDL_JoystickNumButtons(y));
        printf("Number of Balls: %d\r\n", SDL_JoystickNumBalls(y));
      }
    }
  }
}

static void
cscopeinit(void)
{
	fd = dial("localhost", 4200);
	if(fd == -1)
		fprintf(stderr, "Could not connect to color scope.\n");
	signal(SIGPIPE, SIG_IGN);
}

#define CONO_BLUE    000000020  /* Color scope blue enable */
#define CONO_SPCWAR  000000040  /* Spacewar consoles */
#define CONO_GREEN   000020000  /* Color scope green enable */
#define CONO_RANDOM  000040000  /* Random switches */
#define CONO_RED     020000000  /* Color scope red enable */
#define CONO_KNIGHT  040000000  /* Switches in TK's office */

static word cono_bits = 0;
static word datao_bits = 0;
static word spcwar_bits = 0777777777777;
static word obits = 0;

static int red, green, blue;

static void
cscope_plot(void)
{
	int x = (datao_bits >> 9) & 0777;
	int y = datao_bits & 0777;
	int data;
	char buf[4];
	data = red << 28;
	data += green << 24;
	data += blue << 20;
	data += x;
	data += y << 10;
	buf[0] = data & 0xFF;
	buf[1] = (data >> 8) & 0xFF;
	buf[2] = (data >> 16) & 0xFF;
	buf[3] = (data >> 24) & 0xFF;
	if (fd == -1) {
		fd = dial("localhost", 4200);
		if (fd != -1)
			printf("Reconnected color scope\n");
	}
	if (fd != -1) {
		if (writen(fd, buf, sizeof buf) == -1) {
			printf("Lost color scope\n");
			close(fd);
			fd = -1;
		}
	}
}

void joy_motion (SDL_JoyAxisEvent *ev)
{
  int m, n = ev->which;
  m = n * 18;
  n *= 9;

  if (ev->axis == 0) {
    spcwar_bits |= 0600LL << n;
    if (ev->value < -10000)
      spcwar_bits &= ~(0200LL << n);
    if (ev->value > 10000)
      spcwar_bits &= ~(0400LL << n);

    obits &= ~(060LL << m);
    if (ev->value < -10000)
      obits |= 020LL << m;
    if (ev->value > 10000)
      obits |= 040LL << m;
  } else if (ev->axis == 1) {
    spcwar_bits |= 0100LL << n;
    obits &= ~(0300LL << m);
    if (ev->value < -10000) {
      spcwar_bits &= ~(0100LL << n);
      obits |= 0200LL << m;
    } else if (ev->value > 10000) {
      spcwar_bits &= ~(0100LL << n);
      obits |= 0100LL << m;
    }
  }
}

void joy_button (SDL_JoyButtonEvent *ev)
{
  int m, n = ev->which;
  word x = 0, y = 0;
  m = n * 18;
  n *= 9;

  switch (ev->button) {
  case 0: x = 000010; y = 020; break; //Torpedo
  case 1: x = 000004; y = 040; break; //Hyperspace
  case 2: x = 020000; break; //Beacon beam
  case 3: x = 000001; break; //Time K
  case 4: x = 000002; break; //Time 0
  case 5: x = 000400; break; //"Xmit data" = torpedo
  case 6: x = 010000; break; //"General cancel" = destruct
  }

  if (ev->state) {
    spcwar_bits &= ~(y << n);
    obits |= x << m;
  }  else {
    spcwar_bits |= y << n;
    obits &= ~(x << m);
  }
}

void controller_motion (SDL_ControllerAxisEvent *event)
{
  SDL_JoyAxisEvent e;
  e.which = event->which;
  e.axis = event->axis;
  e.value = event->value;
  joy_motion (&e);
}

void controller_button (SDL_ControllerButtonEvent *event)
{
  SDL_JoyButtonEvent e;
  SDL_GameControllerButtonBind b;
  SDL_GameController *c;

#if HAVE_GAMECONTROLLERFROMINSTANCEID
  c = SDL_GameControllerFromInstanceID (event->which);
#endif
  b = SDL_GameControllerGetBindForButton (c, event->button);

  e.which = event->which;
  e.button = b.value.button;
  e.state = event->state;
  joy_button (&e);
}



static void
wake_joy(void *dev)
{
	Joy *joy;
	IOBus *bus;

	joy = dev;
	bus = joy->bus;

	if(bus->devcode == JOY){
		if(IOB_STATUS){
		}
		if(IOB_DATAI){
		  if (cono_bits & CONO_SPCWAR)
		    bus->c12 |= spcwar_bits;
		}
		if(IOB_CONO_CLEAR){
		  cono_bits = 0;
		}
		if(IOB_CONO_SET){
		  cono_bits |= bus->c12;
		  if (cono_bits & CONO_RED)
		    red = (cono_bits >> 12) & 017;
		  if (cono_bits & CONO_GREEN)
		    green = (cono_bits >> 6) & 017;
		  if (cono_bits & CONO_BLUE)
		    blue = cono_bits & 017;
		}
		if(IOB_DATAO_CLEAR){
		  datao_bits = 0;
		}
		if(IOB_DATAO_SET){
		  datao_bits |= bus->c12;
		  cscope_plot();
		}
	}
}

static void
wake_ojoy(void *dev)
{
	OJoy *joy;
	IOBus *bus;

	joy = dev;
	bus = joy->bus;

	if(bus->devcode == OJOY){
		if(IOB_STATUS){
		}
		if(IOB_DATAI){
                  bus->c12 |= obits;
		}
		if(IOB_CONO_CLEAR){
		}
		if(IOB_CONO_SET){
		}
		if(IOB_DATAO_CLEAR){
		}
		if(IOB_DATAO_SET){
		}
	}
}

static void
joyioconnect(Device *dev, IOBus *bus)
{
	Joy *joy;
	joy = (Joy*)dev;
	joy->bus = bus;
	bus->dev[JOY] = (Busdev){ joy, wake_joy, 0 };
}


static void
ojoyioconnect(Device *dev, IOBus *bus)
{
	OJoy *joy;
	joy = (OJoy*)dev;
	joy->bus = bus;
	bus->dev[OJOY] = (Busdev){ joy, wake_ojoy, 0 };
}


static Device joyproto = {
	nil, nil, "",
	nil, nil,
	joyioconnect,
	nil, nil
};

static Device ojoyproto = {
	nil, nil, "",
	nil, nil,
	ojoyioconnect,
	nil, nil
};

Device*
makejoy(int argc, char *argv[])
{
	Joy *joy;

	joy = malloc(sizeof(Joy));
	memset(joy, 0, sizeof(Joy));

	joy->dev = joyproto;
	joy->dev.type = joy_ident;


	joyinit();
	cscopeinit();

	return &joy->dev;
}

Device*
makeojoy(int argc, char *argv[])
{
	OJoy *joy;

	joy = malloc(sizeof(OJoy));
	memset(joy, 0, sizeof(OJoy));

	joy->dev = ojoyproto;
	joy->dev.type = ojoy_ident;

        joyinit();

	return &joy->dev;
}
