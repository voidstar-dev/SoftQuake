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

// cd_sdl.c -- CD audio emulation. Requires music files to be in the <gamedir>/music folder.

#include "quakedef.h"

#include <SDL2/SDL_mixer.h>


// These are ordered in terms of precedence
// When loading a music track, we try out all of the formats in this order
enum
{
	CD_AUDIO_FMT_MP3,
	CD_AUDIO_FMT_OGG,
	CD_AUDIO_FMT_FLAC,
	CD_AUDIO_FMT_WAV,

	CD_AUDIO_FMT_COUNT
};

typedef struct
{
	const char *extension;
	int enabled;

	Uint32 format_mask; // Internal use for SDL_mixer
} cd_audio_format;

typedef struct
{
	Mix_Music *track;
	void *data; // backing store for the music. Must be valid while the track is loaded
	int quake_filehandle; // Must be closed after being opened with COM_FindFile
} cd_track;

// softquake --  I don't know what the oldest compiler to support designated initializers is,
// this might have to go
static cd_audio_format cd_audio_formats[CD_AUDIO_FMT_COUNT] =
{
	[CD_AUDIO_FMT_MP3]  = {"mp3", 0, MIX_INIT_MP3},
	[CD_AUDIO_FMT_OGG]  = {"ogg", 0, MIX_INIT_OGG},
	[CD_AUDIO_FMT_FLAC] = {"flac", 0, MIX_INIT_FLAC},
	[CD_AUDIO_FMT_WAV]  = {"wav", 0, 0},
};

static cd_track cd_current_track =
{
	.track = 0,
	.data = 0,
	.quake_filehandle = -1
};

static int cd_volume = 0;
static int cd_audio_initialized = 0;


static void FreeTrack(void)
{
	if(cd_current_track.track)
	{
		Mix_FreeMusic(cd_current_track.track);
		cd_current_track.track = 0;
	}
	if(cd_current_track.data)
	{
		SDL_free(cd_current_track.data);
		cd_current_track.data = 0;
	}
	if(cd_current_track.quake_filehandle != -1)
	{
		Sys_FileClose(cd_current_track.quake_filehandle);
		cd_current_track.quake_filehandle = -1;
	}
}

// 'track' is the track number
// Currently, all tracks are expected to be in the following form:
// <gamedir>/music/trackXX.extension
// Eg: id1/music/track03.mp3
static void LoadTrack(byte track)
{
	char filename[MAX_OSPATH];
	int i;

	CDAudio_Pause();
	FreeTrack();

	for(i = 0; i < CD_AUDIO_FMT_COUNT; i++)
	{
		cd_audio_format *fmt = &cd_audio_formats[i];

		q_snprintf(filename, sizeof(filename), "music/track%02d.%s", track, fmt->extension);
		if(fmt->enabled)
		{
			// Quake has an internal filesystem structure
			// COM_FindFile will search for the track in the current com_gamedir first,
			// and traverse back into the root id1 folder
			// This seems to do what we want for now, as mods with the 'music' directory are indeed
			// prioritized over the id1 music directory

			// com_gamedir is the current game/mod directory
			// id1 by default, but can be overriden with the -game option in the commandline

			// This also means that mods 'without' a music directory will default to
			// playing music from the id1/music directory, if possible

			// I'm basing this logic from how Quakespasm handles its background music
			// Note that, it may not always be desirable to default to the id1 folder
			// This was how I wrote the code initially, always hardcoded to the current com_gamedir
			// COM_FindFile was never used
			// q_snprintf(filename, sizeof(filename), "%s/music/track%02d.%s", com_gamedir, track, fmt->extension);
			// cd_current_track = Mix_LoadMUS(filename);

			// SDL2 doesn't support SDL_RWFromFP on Windows, so we have to do things a little differently
			// I had to move cd_current_track to its own struct because of a memory leak
			// The backing data store from Sys_FileRead needs to be valid while the track hasn't been reloaded
			int filesize = 0;
			int handle = -1;
			filesize = COM_FindFile(filename, &handle, 0);
			if(handle != -1 && (filesize > 0))
			{
				void *dest = SDL_malloc(filesize);
				if(dest)
				{
					SDL_RWops *sdl_handle = 0;
					int bytes_read = 0;
					bytes_read = Sys_FileRead(handle, dest, filesize);
					sdl_handle = SDL_RWFromConstMem(dest, filesize);
					cd_current_track.track = Mix_LoadMUS_RW(sdl_handle, 1);
					cd_current_track.quake_filehandle = handle;
					if(!cd_current_track.track)
					{
						SDL_free(dest);
					}
					else
					{
						cd_current_track.data = dest;
					}
				}
			}
			if(cd_current_track.track) break;
		}
	}

	if(!cd_current_track.track)
	{
		Con_Printf("Error: Could not load audio track: %s\n", filename);
	}
	else
	{
		Con_DPrintf("Success: Loaded audio track: %s\n", filename);
	}
}

void CDAudio_Play(byte track, qboolean looping)
{
	Con_DPrintf("Play: %d. Looping: %d\n", track, looping);
	Con_DPrintf("Game: %s\n", com_gamedir);

	if(!cd_audio_initialized) return;

	LoadTrack(track);

	if(cd_current_track.track)
	{
		Mix_PlayMusic(cd_current_track.track, looping ? -1 : 0);
	}
}

void CDAudio_Stop(void)
{
	Mix_PauseMusic();
	Mix_RewindMusic();
}

void CDAudio_Pause(void)
{
	Mix_PauseMusic();
}

void CDAudio_Resume(void)
{
	Mix_ResumeMusic();
}

// Assumes a normalized volume from 0 to 1, but we clamp just in case
// Converts to a volume value that SDL_mixer can use
static int ConvertNormalizedVolumeToMixerVolume(float vol)
{
	vol = Q_clamp(vol, 0, 1);
	return vol * 128.0f;
}

// Assumes volume has already been converted to something that SDL_Mixer can use
// Currently, this is an integer in the range of 0 to 128
static void CDAudio_SetVolume(int mixer_volume)
{
	Mix_VolumeMusic(mixer_volume);
}

void CDAudio_Update(void)
{
	// Not sure what this does yet
	// It might be meant for a circular buffer type api
	// For now we use it to track volume changes

	int last_volume = cd_volume;
	cd_volume = ConvertNormalizedVolumeToMixerVolume(bgmvolume.value);
	if(last_volume != cd_volume)
	{
		CDAudio_SetVolume(cd_volume);
	}
}

static void CDAudio_ShutdownLocal(void)
{
	Mix_PauseMusic();
	FreeTrack();

	Mix_CloseAudio();
	Mix_Quit();
}

static void PrintMixVersion(void)
{
	SDL_version Compiled;
	SDL_version Linked;
	const SDL_version *LinkedP;

	MIX_VERSION(&Compiled);
	LinkedP = Mix_Linked_Version();
	Linked = *LinkedP;

	Sys_Printf("Compiled against SDL_mixer version: %d.%d.%d\n", Compiled.major, Compiled.minor, Compiled.patch);
	Sys_Printf("Linked against SDL_mixer version:   %d.%d.%d\n", Linked.major, Linked.minor, Linked.patch);
}

// #define CD_DEBUG
#ifdef CD_DEBUG
static void PlayTrack_f(void)
{
	int argc = Cmd_Argc();
	if(argc > 1)
	{
		int num = Q_atoi(Cmd_Argv(1));
		CDAudio_Play(num, true);
	}
	else
	{
		Con_Printf("No track number specified\n");
	}
}
#endif

int CDAudio_Init(void)
{
	Uint32 Formats = MIX_INIT_MP3 | MIX_INIT_OGG | MIX_INIT_FLAC;
	Uint32 ObtainedFormats = 0;
	int i;
	int cd_freq = 22050/2;
	int cd_sample_bytes = 1024;

#ifdef CD_DEBUG
	Cmd_AddCommand("track", PlayTrack_f);
#endif

	Con_Printf("CD Audio Initialization\n");

	if (COM_CheckParm("-nocdaudio"))
	{
		Con_Printf("CD Audio disabled at the command line\n");
		return 0;
	}

	ObtainedFormats = Mix_Init(Formats);
	PrintMixVersion();
	// Con_Printf("%x, %x\n", Formats, ObtainedFormats);

	if(ObtainedFormats == 0)
	{
		// Could not open audio at all
		Con_Printf("Could not initialize CD Audio: %s\n", SDL_GetError());
		CDAudio_ShutdownLocal();
		return 0;
	}

	// Todo: Audio format and frequency from shm (snd_sdl.c)
	// Increased buffer size from 512 to 1024 bytes to accomodate higher frequency input files (tested at 96k hz)
	if(Mix_OpenAudio(cd_freq, AUDIO_S16SYS, 2, cd_sample_bytes) != 0)
	{
		Con_Printf("Could not open CD Audio Mixer: %s\n", SDL_GetError());
		CDAudio_ShutdownLocal();
		return 0;
	}

	Con_Printf("CD Audio frequency   : %d Hz\n", cd_freq);
	Con_Printf("CD Audio buffer size : %d bytes\n", cd_sample_bytes);


	// Set the enabled flag explicitly
	for(i = 0; i < CD_AUDIO_FMT_COUNT; i++)
	{
		cd_audio_formats[i].enabled = (ObtainedFormats & cd_audio_formats[i].format_mask) != 0;
	}
	// We assume that WAV is always supported
	cd_audio_formats[CD_AUDIO_FMT_WAV].enabled = 1;

	if(Formats != ObtainedFormats)
	{
		// Some formats are not supported
		// This might be okay, we just don't parse for those files

		Con_Printf("Warning: Some CD audio formats are not supported\n");
		for(i = 0; i < CD_AUDIO_FMT_COUNT; i++)
		{
			if(!cd_audio_formats[i].enabled)
			{
				Con_Printf("Not supported: %s\n", cd_audio_formats[i].extension);
			}
		}
	}
	for(i = 0; i < CD_AUDIO_FMT_COUNT; i++)
	{
		if(cd_audio_formats[i].enabled)
		{
			Con_Printf("Supported: %s\n", cd_audio_formats[i].extension);
		}
	}

	// Start off with silence
	// Fixes case where bgmvolume.value is 0 on launch
	cd_volume = 0;
	CDAudio_SetVolume(cd_volume);

	cd_audio_initialized = 1;
	return 1;
}


void CDAudio_Shutdown(void)
{
	CDAudio_ShutdownLocal();
}
