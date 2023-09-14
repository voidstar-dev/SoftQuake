/*
Copyright (C) 2023-2023 softquake

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// snd_sdl.c Audio output

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include "quakedef.h"
#include "sound.h"

static volatile dma_t sdl_audio_dma;
static unsigned char *sdl_audio_buffer;
static SDL_AudioDeviceID sdl_audio_device;

static volatile int sdl_audio_buffer_size = 0;

typedef struct
{
	int speed; // hz
	int channels;
	SDL_AudioFormat format;
} snd_config_t;

static snd_config_t snd_config =
{
	11025,
	2,
	AUDIO_S16SYS,
};


// Todo: Will this work for negative numbers?
static int NextPowerOfTwo(int n)
{
	if(n < 2) return n;

	if((n) & (n - 1))
	{
		int val = 1;
		while(val < n)
		{
			val <<= 1;
		}
		n = val;
	}
	return n;
}

static void SND_AudioCallback(void *userdata, Uint8 *stream, int len)
{
	// Credits to Quakespasm for this circular buffer implementation
	int	pos, tobufend;
	int	len1, len2;

	if (!shm)
	{	/* shouldn't happen, but just in case */
		memset(stream, 0, len);
		return;
	}

	pos = (shm->samplepos * (shm->samplebits / 8));
	if (pos >= sdl_audio_buffer_size)
		shm->samplepos = pos = 0;

	tobufend = sdl_audio_buffer_size - pos;  /* bytes to buffer's end. */
	len1 = len;
	len2 = 0;

	if (len1 > tobufend)
	{
		len1 = tobufend;
		len2 = len - len1;
	}

	memcpy(stream, shm->buffer + pos, len1);

	if (len2 <= 0)
	{
		shm->samplepos += (len1 / (shm->samplebits / 8));
	}
	else
	{	/* wraparound? */
		memcpy(stream + len1, shm->buffer, len2);
		shm->samplepos = (len2 / (shm->samplebits / 8));
	}

	if (shm->samplepos >= sdl_audio_buffer_size) shm->samplepos = 0;
}

static void CleanupShm()
{
	if(shm)
	{
		free(shm->buffer);
		memset((void *)shm, 0, sizeof *shm);
	}
}

static void SND_Shutdown()
{
	SDL_CloseAudioDevice(sdl_audio_device);
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	CleanupShm();
}

static int SetArg0i(char *arg, int *retval, int desired_val)
{
	int changed = 0;
	if(!retval) return changed;

	if(COM_CheckParm(arg))
	{
		*retval = desired_val;
		changed = 1;
	}
	return changed;
}

static void SetArg1i(char *arg, int *retval)
{
	int i;

	if(!retval) return;

	i = COM_CheckParm(arg);
	if(i > 0 && i < com_argc - 1)
	{
		int val = Q_atoi(com_argv[i + 1]);

		// Todo: Check for valid string

		*retval = val;
	}
}

// Support commandline arguments for audio
static void SND_InitConfig(void)
{
#if 0
	-sndspeed <speed>
	Set sound speed.  Usual values are 8000, 11025, 22050 and 44100.
	Default is 11025.

	-sndmono
	Set mono sound

	-sndstereo
	Set stereo sound (default if supported)

	-sndbits <8 or 16>
#endif

	int bits = 16;
	int speed = 0;

	SetArg1i("-sndspeed", &speed);
	SetArg1i("-sndbits", &bits);
	SetArg0i("-sndstereo", &snd_config.channels, 2);
	SetArg0i("-sndmono", &snd_config.channels, 1);
	SetArg1i("-sndbits", &bits);

	if(speed > 0)
	{
		// Todo: Clamp speed?
		snd_config.speed = speed;
	}
	if(bits == 16)
	{
		snd_config.format = AUDIO_S16SYS;
	}
	if(bits == 8 || loadas8bit.value)
	{
		snd_config.format = AUDIO_U8;
		Cvar_SetValue("loadas8bit", 1);
	}
}

// I used snd_win.c for some guidance on the audio format, sample bits, and hz (shm->speed)
qboolean SNDDMA_Init()
{
	// Note: shm must be a valid pointer even if we fail to initialize
	shm = &sdl_audio_dma;
	memset((void *)shm, 0, sizeof(*shm));

	if(SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
	{
		Con_Printf("Could not initialize sound subsystem: %s\n", SDL_GetError());
		return 0;
	}

	SND_InitConfig();

	shm->channels = snd_config.channels;
	shm->speed = snd_config.speed;
	shm->samplebits = snd_config.format & 0xFF; // SDL2 specifies that the lowest 8 bits contain the number of bits per sample

	// Allocate enough buffer memory for 4 seconds of audio and make sure it is a power of 2
	sdl_audio_buffer_size = NextPowerOfTwo(shm->speed * shm->channels * shm->samplebits * 2);
	sdl_audio_buffer = malloc(sdl_audio_buffer_size);


	if(!sdl_audio_buffer)
	{
		SND_Shutdown();
		return 0;
	}

	shm->buffer = sdl_audio_buffer;
	shm->samples = sdl_audio_buffer_size / (shm->samplebits / 8);
	shm->submission_chunk = 1;

	{
		SDL_AudioSpec DesiredSpec = {0};
		SDL_AudioSpec ObtainedSpec = {0};

		q_assert(shm->samplebits == 16 || shm->samplebits == 8);
		DesiredSpec.freq = shm->speed;
		DesiredSpec.format = snd_config.format;
		DesiredSpec.channels = shm->channels;
		DesiredSpec.callback = SND_AudioCallback;


		// DesiredSpec.samples = 256; // This HAS to be 256. Any higher and we get audio dropouts/latency.
		// Only true if we use the default 11025 sample rate

		// From Quakespasm
		if (DesiredSpec.freq <= 11025)
			DesiredSpec.samples = 256;
		else if (DesiredSpec.freq <= 22050)
			DesiredSpec.samples = 512;
		else if (DesiredSpec.freq <= 44100)
			DesiredSpec.samples = 1024;
		else if (DesiredSpec.freq <= 56000)
			DesiredSpec.samples = 2048; /* for 48 kHz */
		else
			DesiredSpec.samples = 4096; /* for 96 kHz */

		sdl_audio_device = SDL_OpenAudioDevice(0, 0, &DesiredSpec, &ObtainedSpec, 0);

		if(sdl_audio_device <= 0)
		{
			Con_Printf("Could not open audio device: %s\n", SDL_GetError());
			SND_Shutdown();
			return 0;
		}
	}

	// This actually plays the device
	SDL_PauseAudioDevice(sdl_audio_device, 0);

	return 1;
}

void SNDDMA_Shutdown()
{
	SND_Shutdown();
}

int SNDDMA_GetDMAPos()
{
	return shm->samplepos;
}

void SNDDMA_LockBuffer()
{
	SDL_LockAudioDevice(sdl_audio_device);
}

void SNDDMA_Submit()
{
	SDL_UnlockAudioDevice(sdl_audio_device);
}
