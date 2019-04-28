#include "pdp6.h"
#include <SDL2/SDL.h>

char *joy_ident = JOY_IDENT;

typedef struct Joy Joy;
struct Joy
{
	Device dev;
	IOBus *bus;
	int pia_data;
	int pia_spec;
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

static word bits = 0777777777777;

void joy_motion (SDL_JoyAxisEvent *ev)
{
  int n = ev->which;
  n *= 9;

  if (ev->axis == 0) {
    bits |= 0600LL << n;
    if (ev->value < -10000)
      bits &= ~(0200LL << n);
    if (ev->value > 10000)
      bits &= ~(0400LL << n);
  } else if (ev->axis == 1) {
    bits |= 0100LL << n;
    if (ev->value < -3000)
      bits &= ~(0100LL << n);
  }
}

void joy_button (SDL_JoyButtonEvent *ev)
{
  int n = ev->which;
  word y = 0;
  n *= 9;

  switch (ev->button) {
  case 0: y = 020; break; //Torpedo
  case 1: y = 040; break; //Hyperspace
  }

  if (ev->state)
    bits &= ~(y << n);
  else
    bits |= y << n;
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
                  bus->c12 |= bits;
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


static Device joyproto = {
	nil, nil, "",
	nil, nil,
	joyioconnect,
	nil, nil
};

Device*
makejoy(int argc, char *argv[])
{
	Joy *joy;
	Task t;

	joy = malloc(sizeof(Joy));
	memset(joy, 0, sizeof(Joy));

	joy->dev = joyproto;
	joy->dev.type = joy_ident;

	//addtask(t);
        joyinit();

	return &joy->dev;
}
