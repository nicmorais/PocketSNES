#include <stdlib.h>
#include <SDL.h>

#include <sal.h>

#include "apu.h"

static SDL_AudioSpec audiospec;

static void sdl_audio_callback (void *userdata, Uint8 *stream, int len)
{
	S9xMixSamples(stream, len / sizeof(int16_t));
}

s32 sal_AudioInit(s32 rate, s32 bits, s32 stereo, s32 Hz)
{
	audiospec.freq = rate;
	audiospec.channels = (stereo + 1);
	audiospec.format = AUDIO_S16SYS;

	audiospec.samples = rate / Hz;
	if (!stereo && (audiospec.samples & 1))
		audiospec.samples--;

	audiospec.callback = sdl_audio_callback;

	if (SDL_OpenAudio(&audiospec, NULL) < 0) {
		fprintf(stderr, "SDL audio initialisation failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return SAL_ERROR;
	}
	else
	{
		fprintf(stderr, "SDL audio initialised successfully\n");
		return SAL_OK;
	}
}

void sal_AudioPause(void)
{
	SDL_PauseAudio(1);
	fprintf(stderr, "SDL audio paused\n");
}

void sal_AudioResume(void)
{
	SDL_PauseAudio(0);
	fprintf(stderr, "SDL audio resumed\n");
}

void sal_AudioClose(void)
{
	SDL_CloseAudio();
	fprintf(stderr, "SDL audio finalised\n");
}

void sal_AudioSetVolume(s32 l, s32 r)
{
}
