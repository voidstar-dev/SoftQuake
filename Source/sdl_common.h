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

#ifndef _SDL_COMMON_H_
#define _SDL_COMMON_H_

#include <SDL2/SDL.h>

extern int vid_client_width;
extern int vid_client_height;

void VID_GetClientWindowDimensions(int *w, int *h);
void CFG_ReadVars(void);


void VID_CheckParams(int *width, int *height);
void VID_RegisterCommonVars(void);

extern cvar_t vid_desktopfullscreen;
extern cvar_t vid_vsync;
extern cvar_t vid_fullscreen;
extern cvar_t sw_backend;

#endif /* _SDL_COMMON_H_ */
