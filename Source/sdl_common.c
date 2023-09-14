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

#include "quakedef.h"
#include "sdl_common.h"


// Borrowed from Quakespasm
cvar_t vid_desktopfullscreen = {"vid_desktopfullscreen", "1", CV_ARCHIVE};
cvar_t vid_vsync = {"vid_vsync", "1", CV_ARCHIVE | CV_CALLBACK};



// Fullscreen mode
cvar_t vid_fullscreen = {"vid_fullscreen", "0", true};

// Rendering backend for the software renderer
// Really, what is used to display the final image
// The quake renderer is still doing most of the heavy lifting
// Options: sdl -- Use sdl2_render
// Options: ogl -- Use OpenGL
cvar_t sw_backend = {"_sw_backend", "sdl", CV_ARCHIVE | CV_CALLBACK};

// Access config.cfg right before we init the video system
void CFG_ReadVars(void)
{
	Cbuf_InsertText("exec config.cfg\n");
	Cbuf_Execute();
}

void VID_SetVsync(qboolean enable);

void VID_Vsync_cb(cvar_t *var)
{
	VID_SetVsync(var->value);
}

void SW_Backend_f(void)
{
	if(Cmd_Argc() > 1)
	{
		const char *s = Cmd_Argv(Cmd_Argc() - 1);
		Cvar_SetStringQuick(&sw_backend, s);
		Con_Printf("Rendering backend set to \"%s\"\n", s);
#ifdef GLQUAKE
		Con_Printf("This cvar has no effect in GLQuake\n");
#else
		Con_Printf("Requires a full restart to take effect\n");
#endif
	}
	else
	{
		Con_Printf("Possible options:\n");
		Con_Printf("sdl    Use SDL2 to display the image\n");
		Con_Printf("ogl    Use OpenGL to display the image\n");

		Con_Printf("sw_backend is \"%s\"\n", sw_backend.string);
	}
}

// static int windowed_width = 0;
// static int windowed_height = 0;
// static int fullscreen_width = 0;
// static int fullscreen_height = 0;
void VID_ToggleFullscreen(void)
{
	// Todo: Handle this more elegantly
	// The way this is currently done is awful
	// We should really just remember the windowed dimensions and the fullscreen dimensions seperately
	int fs = vid_fullscreen.value;
	extern viddef_t vid;
	
	fs = !fs;
	Cvar_SetValueQuick(&vid_fullscreen, fs);
	Cbuf_InsertText("vid_restart\n");
	vid.recalc_refdef = 1;
}


// Copied from snd_sdl.c
// Might want to move this to its own file if this is reused again
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

qboolean SetArg1i(char *arg, int *retval)
{
	int i;

	if(!retval) return false;

	i = COM_CheckParm(arg);
	if(i > 0 && i < com_argc - 1)
	{
		int val = Q_atoi(com_argv[i + 1]);

		// Todo: Check for valid string

		*retval = val;

		return true;
	}
	return false;
}

void VID_CheckParams(int *width, int *height)
{
	// Only check these on launch
	extern qboolean vid_first_time;
	if(vid_first_time)
	{
		// -width and -height apply in the following circumstances:
		// vid_desktopfullscreen 0 vid_fullscreen 1
		// vid_fullscreen 0
		// In other words, only "real" fullscreen mode, or windowed mode.
		qboolean have_width = SetArg1i("-width", width);
		qboolean have_height = SetArg1i("-height", height);

		if(have_width && !have_height)
		{
			*height = *width * 3 / 4;
		}
		if(!have_width && have_height)
		{
			*width = *height * 4 / 3;
		}
	}
}

void VID_RegisterCommonVars(void)
{
	Cvar_RegisterVariable(&vid_desktopfullscreen);
	
	Cvar_RegisterVariable(&vid_vsync);
	Cvar_RegisterCallback(&vid_vsync, VID_Vsync_cb);

	Cvar_RegisterVariable(&sw_backend);
	Cmd_AddCommand("sw_backend", SW_Backend_f);
}
