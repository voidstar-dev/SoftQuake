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

#ifndef _GL_TEXMGR_H_
#define _GL_TEXMGR_H_

#define TEX_USAGE_LIGHTMAP 	(1 << 0)	// Dynamic lightmap. Not used yet
#define TEX_USAGE_PIC 		(1 << 1)	// Anything 2D/UI related
#define TEX_USAGE_MODEL 	(1 << 2)	// Entity models
#define TEX_USAGE_WORLD 	(1 << 3)	// Worldmodel texture
#define TEX_USAGE_SKY 		(1 << 4)	// Skybox. Found in gl_warp.c. Uses a special code path in R_InitSky
#define TEX_USAGE_TEXT 		(1 << 5)	// Text characters
#define TEX_USAGE_CONSOLE	(1 << 6)	// Console decorations


// Force a texture to have a certain filter regardless of the gl_texturemode setting
// This was initially needed because the sky doesn't have a mipmap and could not display
// if the mode included a mipmap, but I've since found other uses for it and modified
// TEX_CacheAdd and GL_LoadTexture to accept the preference argument

#define TEX_PREF_FILTER_FORCE_NONE 		(0 << 0) 	// Use the current texture filters
#define TEX_PREF_FILTER_FORCE_NEAREST 	(1 << 1) 	// Always use nearest filtering
#define TEX_PREF_FILTER_FORCE_LINEAR 	(1 << 2) 	// Always use linear filtering
#define TEX_PREF_FILTER_FORCE_AUTO 		(1 << 3) 	// Whatever min and mag filters are, just use the non-mipmapped versions


// The only textures we care about changing when using gl_texturemode
#define TEX_MODE_MASK \
	  (TEX_USAGE_MODEL| TEX_USAGE_WORLD | TEX_USAGE_SKY | TEX_USAGE_TEXT | TEX_USAGE_CONSOLE | TEX_USAGE_PIC)

// #define SOFTQUAKE_GL_OVERRIDE_TEXTURE_PREF

// Force UI elements to obey a certain texture filter rule
// These are my personal preferences, but you can override them by defining this macro
// The alternative is to use the current gl_texturemode for all textures
#ifndef SOFTQUAKE_GL_OVERRIDE_TEXTURE_PREF
	#define TEX_PREF_PIC (TEX_PREF_FILTER_FORCE_NEAREST)
	#define TEX_PREF_CONSOLE (TEX_PREF_FILTER_FORCE_NEAREST)
	#define TEX_PREF_TEXT (TEX_PREF_FILTER_FORCE_NEAREST)
#else
	#define TEX_PREF_PIC (TEX_PREF_FILTER_FORCE_AUTO)
	#define TEX_PREF_CONSOLE (TEX_PREF_FILTER_FORCE_AUTO)
	#define TEX_PREF_TEXT (TEX_PREF_FILTER_FORCE_AUTO)
#endif

typedef char tex_identifier[64];
typedef struct texture_cache_t texture_cache_t;
struct texture_cache_t
{
	int usage;
	int gl_texturenum;
	int gl_texturepref;

	// Precalculate min/mag filters
	// Only valid if TEX_PREF is TEX_PREF_FILTER_FORCE_LINEAR or TEX_PREF_FILTER_FORCE_NEAREST
	int filter;

	tex_identifier identifier;
};

void TEX_CacheAdd(int gl_texturenum, int usage, int gl_texturepref, tex_identifier identifier);
void TEX_CacheAddScrap(int gl_texturenum, int gl_texturepref);
void TEX_SetFilter(int min_filter, int mag_filter, int mask);
void TEX_GetTexturePrefFilter(int force_filter, int min_filter, int mag_filter, int *min_filter_out, int *mag_filter_out);

void TEX_SetGLFilterFromPref(int min_filter, int mag_filter, int gl_texturenum, int gl_texturepref);
qboolean TEX_CacheContains(int usage, char identifier[], texture_cache_t **ret);
void TEX_CacheAddAtEnd(int gl_texturenum, int usage, int gl_texturepref, tex_identifier identifier);
void TEX_CacheItemUpdate(texture_cache_t *t, int gl_texturenum, int usage, int gl_texturepref);



#endif /* _GL_TEXMGR_H_ */
