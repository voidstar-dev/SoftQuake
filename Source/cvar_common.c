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


// cvar_common.c: Common cvars between the GL version and the software rendered version

// Why this file exists:
// Quake will overwrite config.cfg only with variables that are flagged to be archived
// Anything else will be lost
// Initially, switching between GLQuake and SoftQuake meant that certain cvars were being removed
// Now, all shared configuation cvars will persist in both versions :)

// Note: Any cvars already registered are simply ignored by the engine, so no worries there
// I don't want to change the original call sites of Cvar_RegisterVariable, 
// since this file is my own personal addition, and is not guaranteed to appear elsewhere

// The original Quake does not have a readonly flag, which would be useful to prevent accidental modification
// of variables only required by GLQuake or vice-versa

#include "quakedef.h"

cvar_t	r_shadows = {"r_shadows","0", true}; // softquake -- archive this
cvar_t	gl_clear = {"gl_clear","0", true}; // softquake -- archive this
cvar_t	gl_flashblend = {"gl_flashblend","1", true}; // softquake -- archive this
cvar_t	gl_zfix = {"gl_zfix", "0", true}; // softquake -- Add zfix from Quakespasm

// fb_mode, used in vid_sdl2
cvar_t fb_mode = {"fb_mode", "1", CV_ARCHIVE | CV_CALLBACK};

// I like my crispy textures, so we remember this cvar now
// Still, the GLQuake default is used to start with
#ifdef SOFTQUAKE_GL_TEXTUREMODE_FIX
cvar_t gl_texturemode = {"gl_texturemode", "gl_linear_mipmap_linear", CV_ARCHIVE | CV_CALLBACK};
#endif

#ifdef SOFTQUAKE_GL_UI_SCALING_FIX
cvar_t ui_scale = {"ui_scale", "1", CV_ARCHIVE | CV_CALLBACK};
#endif

void R_RegisterCommonCvars()
{
	Cvar_RegisterVariable(&r_shadows);
	Cvar_RegisterVariable(&gl_clear);
	Cvar_RegisterVariable(&gl_flashblend);
	Cvar_RegisterVariable(&gl_zfix);

#ifdef SOFTQUAKE_GL_TEXTUREMODE_FIX
	Cvar_RegisterVariable(&gl_texturemode);
#endif

#ifdef SOFTQUAKE_GL_UI_SCALING_FIX
	Cvar_RegisterVariable(&ui_scale);
#endif

	Cvar_RegisterVariable(&fb_mode);
}

// Add any future RegisterCommonCvars functions here for the subsystem of your chosing
