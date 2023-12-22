#include "pdp6.h"
#include <SDL2/SDL.h>

/* Number of samples in audio buffer. */
#define AUDIO_SAMPLES (4*4096)
/* Number of samples in MI buffer. */
#define MI_SAMPLES (100*1024)

static Uint8 buffer[MI_SAMPLES];
static Uint8 audio[2*AUDIO_SAMPLES];

int samples = 0;
int refill = 1;

void music_sample(word mi)
{
	Uint8 x = 0;
        int i;
	if(samples == MI_SAMPLES)
		return;
        for(i = 0; i < 6; i++) {
          if(mi & (1 << i))
            x+= 40;
        }
        buffer[samples++] = x;

        if(refill) {
          int i, j;
          fprintf (stderr, "[%d]", samples);
          for(i = 0; i < 2*AUDIO_SAMPLES; i++) {
            j = (int)((float)i * (float)MI_SAMPLES / (float)(2*AUDIO_SAMPLES) + .5);
            audio[i] = buffer[j];
          }
          samples = 0;
          refill = 0;
        }
}

static void callback(void *data, Uint8 *stream, int len)
{
	if (refill)
		memset(stream, 0, len);
	else
		memcpy(stream, audio, len);
	refill = 1;
}

void music_init(Apr *apr)
{
	SDL_AudioSpec want, have;
	SDL_AudioDeviceID dev;

	SDL_memset(&want, 0, sizeof want);
	want.freq = 48000;
	want.format = AUDIO_U8;
	want.channels = 1;
	want.samples = AUDIO_SAMPLES;
	want.callback = callback;
	want.userdata = apr;

	dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	if (dev == 0)
		SDL_Log("Failed to open audio: %s", SDL_GetError());
	else {
		if (have.format != want.format)
			SDL_Log("We didn't get Float32 audio format.");
                fprintf(stderr, "Frequency: %d\n", have.freq);
                fprintf(stderr, "Channels: %d\n", have.channels);
                fprintf(stderr, "Samples: %d\n", have.samples);
		SDL_PauseAudioDevice(dev, 0);
	}
}
