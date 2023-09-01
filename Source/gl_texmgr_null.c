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

/**********************************
* Texture manager for GLQuake
* Null implementation
**********************************/

#include "quakedef.h"

void TEX_CacheAdd(int gl_texturenum, int usage, int gl_texturepref, char identifier[64])
{
}

void TEX_SetFilter(int min_filter, int mag_filter, int mask)
{
}


void TEX_GetTexturePrefFilter(int force_filter, int min_filter, int mag_filter, int *min_filter_out, int *mag_filter_out)
{
}

void TEX_CacheAddScrap(int gl_texturenum, int gl_texturepref)
{
}


void TEX_SetGLFilterFromPref(int min_filter, int mag_filter, int gl_texturenum, int gl_texturepref)
{
}

qboolean TEX_CacheContains(int usage, char identifier[])
{
	// Must return false to maintain the old behaviour
	return false;
}

void TEX_AddAtEnd(int gl_texturenum, int usage, int gl_texturepref, char identifier[64])
{
}
