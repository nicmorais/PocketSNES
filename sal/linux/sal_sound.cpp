#include <stdlib.h>
#include <SDL.h>

#include <sal.h>

#include "apu.h"

#define BUFFER_SAMPLES 1024

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

	audiospec.samples = BUFFER_SAMPLES;
	if (!stereo && (audiospec.samples & 1))
		audiospec.samples--;

	audiospec.callback = sdl_audio_callback;

	if (SDL_OpenAudio(&audiospec, NULL) < 0) {
		fprintf(stderr, "Unable to initialize audio.\n");
		return SAL_ERROR;
	}

	return SAL_OK;
}

void sal_AudioPause(void)
{
	SDL_PauseAudio(1);
}

void sal_AudioResume(void)
{
	SDL_PauseAudio(0);
}

void sal_AudioClose(void)
{
	SDL_CloseAudio();
}

void sal_AudioSetVolume(s32 l, s32 r)
{
}
