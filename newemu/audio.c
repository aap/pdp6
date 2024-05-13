#include "common.h"
#include "pdp6.h"

#include <SDL2/SDL.h>

static SDL_AudioDeviceID dev;
static int nsamples;
static u64 nexttime;

#define SAMPLE_TIME (1000000000/48000)

void
initmusic(void)
{
	SDL_AudioSpec spec;

	SDL_Init(SDL_INIT_AUDIO);

	memset(&spec, 0, sizeof(spec));
	spec.freq = 48000;
	spec.format = AUDIO_U8;
	spec.channels = 1;
	spec.samples = 1024;	// whatever this is
	spec.callback = nil;
	dev = SDL_OpenAudioDevice(nil, 0, &spec, nil, 0);
}

void
stopmusic(void)
{
	if(dev == 0)
		return;

	SDL_PauseAudioDevice(dev, 1);
	SDL_ClearQueuedAudio(dev);
	nsamples = 0;
	nexttime = 0;
}

void
svc_music(PDP6 *pdp)
{
	u8 s;

	if(dev == 0 || nexttime >= simtime)
		return;

	if(nexttime == 0)
		nexttime = simtime + SAMPLE_TIME;
	else
		nexttime += SAMPLE_TIME;

	// queue up a few samples
	if(nsamples < 10) {
		nsamples++;
		// then start playing
		if(nsamples == 10)
			SDL_PauseAudioDevice(dev, 0);
	}

	s = 0;
	for(int i = 0; i < 6; i++)
		if(pdp->mi & (1<<i))
			s += 40;
	SDL_QueueAudio(dev, &s, 1);
}
