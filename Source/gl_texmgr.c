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
**********************************/

#include "quakedef.h"

// This is more like... cap per game session
// Maybe make an LRU cache or something
// There's no obvious place that I've found where texture handles are explicitly released (yet)
// so we just accumulate until we run out.
// There is Host_ClearMemory() in host.c, but if we manually reset the tex_cache_used there,
// some textures remain untracked for the remainder of the game

// In gl_draw.c, the constant MAX_GLTEXTURES, and accompanying variable numgltextures appear to serve a similar purpose.
// Nowhere in the codebase is it ever reset to 0, so I'm assuing we'll never hit the texture limit
// for the vanilla game and official expansions of the mid-late 90s

// I naiively went through each level in the vanilla game with the map command, and found about 600 textures loaded in total

// Good thing modern computers have lots of memory right
// The performance of this can be improved by making a texture cache for each specific type
// so we don't have to check for it in TEX_SetFilter

// Note: texture_extension_number leaks memory!!!
// I fixed this in GL_LoadTexture, but the fix only works if gl_texmgr.c is used
// gl_texmgr_null.c does nothing to address the leak

#define TEXTURE_CACHE_CAP_PER_GAME 2048

#define IGNORE_TEXTURE_USAGE_MASK (TEX_USAGE_LIGHTMAP)

typedef struct
{
	texture_cache_t cache[TEXTURE_CACHE_CAP_PER_GAME];
	int used;
} texture_collection_t;

static texture_collection_t generic_cache;
static texture_collection_t pic_cache;

static void SetNearestOrLinearTexturePref(texture_cache_t *t);


/******************************************************************
* This code fixes the memory leak in GL_LoadTexture.
* It does nothing to change the default gl_texturemode behaviour.
* Define SOFTQUAKE_GL_TEXTUREMODE_FIX before compling to enable
* a more familar behaviour of gl_texturemode.
* For more info, read softquake-notes.txt
******************************************************************/

static int strncmp_lhs_greater(char *s1, char *s2, size_t size)
{
	// We can't use Q_strncmp because it only tests equality
	int i = strncmp(s1, s2, size);
	return (i > 0);
}

// ~3 ms for a full travel command (custom alias that loads every map in the base game)
// I finally implemented a real binary search
// This is just the old code wrapped in the desired api
// Returns -1 if not found
// Returns 0 or above if found (the index in the cache)
#if 0
static int BinaryFind(const char identifier[64])
{
	int i;
	for(i = 0; i < generic_cache.used; i++)
	{
		texture_cache_t *t = &generic_cache.cache[i];
		if(Q_strncmp(t->identifier, (char *)identifier, sizeof(t->identifier)) == 0)
		{
			return i;
		}
	}
	return -1;
}
#else
static int strncmp_fixed(const char *s1, const char *s2, size_t size)
{
	int n = strncmp(s1, s2, size);
	if(n < 0) return -1; // s1 smaller
	if(n > 0) return 1; // s1 greater
	return 0; // equal
}
// Expects generic_cache to be sorted
// Returns -1 if not found
// Returns 0 or above if found (the index in the cache)
static int BinaryFind(const tex_identifier identifier)
{
	// Thanks K&R, once again your book has helped me out
	int lo = 0;
	int hi = generic_cache.used - 1;
	while(lo <= hi)
	{
		int mid = (lo + hi) / 2;
		texture_cache_t *t = &generic_cache.cache[mid];
		int eq = strncmp_fixed(identifier, t->identifier, sizeof(t->identifier));
		switch(eq)
		{
			case -1: // left
				hi = mid - 1;
				break;
			case 1: // right
				lo = mid + 1;
				break;
			case 0: // done
				return mid;
		}
	}
	return -1;
}
#endif


static void LinearInsert(texture_collection_t *textures, const texture_cache_t *t)
{
	int tex_cache_used = textures->used;
	if(tex_cache_used >= TEXTURE_CACHE_CAP_PER_GAME)
	{
		Sys_Error("Too many textures loaded for the texture cache");
	}
	textures->cache[tex_cache_used] = *t;
	textures->used++;
}


static void PicInsert(const texture_cache_t *t)
{
	if(t->usage != TEX_USAGE_PIC)
	{
		Sys_Error("Expected TEX_USAGE_PIC for this texture\n");
	}
	LinearInsert(&pic_cache, t);
}


static void GenericInsert(const texture_cache_t *t)
{

}

static void BinaryInsert(texture_collection_t *textures, const texture_cache_t *t)
{
	int tex_cache_used = textures->used;
	texture_cache_t *tex_cache = textures->cache;
	int idx = 1;
	int end = tex_cache_used + 1;

	// Todo: Make a seperate cache for lightmaps?
	if(tex_cache_used >= TEXTURE_CACHE_CAP_PER_GAME)
	{
		Sys_Error("Too many textures loaded for the texture cache");
	}
	if(t->usage & TEX_USAGE_PIC)
	{
		Sys_Error("TEX_USAGE_PIC cannot be used in BinaryInsert");
	}

	// I don't know what this sort is called, some kind of psuedo insertion-sort
	// Anyway, it's not that fast, but...
	// Since this is only done once per texture for the entire runtime of the game, 
	// the overhead for this will go away quickly
	// BinaryFind is called much more frequently
	// Compare starting from the end instead

	tex_cache[tex_cache_used] = *t;
	while(idx < end)
	{
		int ptr = end - idx;
		texture_cache_t *lhs = &tex_cache[ptr - 1];
		texture_cache_t *rhs = &tex_cache[ptr - 0];
		if(strncmp_lhs_greater(lhs->identifier, rhs->identifier, sizeof(lhs->identifier)))
		{
			texture_cache_t tmp = *lhs;
			*lhs  = *rhs;
			*rhs = tmp;
		}
		idx++;
	}
	textures->used++;
}


void TEX_CacheItemUpdate(texture_cache_t *t, int gl_texturenum, int usage, int gl_texturepref)
{
	t->gl_texturenum = gl_texturenum;
	t->usage = usage;
	t->gl_texturepref = gl_texturepref;
	SetNearestOrLinearTexturePref(t);
}

// Generic add, handles all cases
void TEX_CacheAdd(int gl_texturenum, int usage, int gl_texturepref, tex_identifier identifier)
{
	int index = -1;

	// Ignore these for now... 
	if(usage & (IGNORE_TEXTURE_USAGE_MASK)) return;
	// We require a unique id here. Only TEX_USAGE_PIC is valid
	if(!identifier[0])
	{
		if(usage == TEX_USAGE_PIC)
		{
			goto add_texture;
		}
		return;
	}

	index = BinaryFind(identifier);
	if(index != -1)
	{
		// Texture already exists, just update its state
		texture_cache_t *tex_cache = generic_cache.cache;
		texture_cache_t *t = &tex_cache[index];
		TEX_CacheItemUpdate(t, gl_texturenum, usage, gl_texturepref);
		return;
	}

add_texture:
	TEX_CacheAddAtEnd(gl_texturenum, usage, gl_texturepref, identifier);
	// Con_Printf("Added texture no. %d (%d)\n", tex_cache_used, gl_texturenum);
}


void TEX_CacheAddScrap(int gl_texturenum, int gl_texturepref)
{
	// It seems like scrap blocks are various statusbar elements
	// Treat them like pics for now

	// Avoid buffer overflow from access
	char noname[64] = {0};
	TEX_CacheAdd(gl_texturenum, TEX_USAGE_PIC, gl_texturepref, noname);
}



qboolean TEX_CacheContains(int usage, char identifier[], texture_cache_t **ret)
{
	int i;
	if(!identifier) return false;
	if(!identifier[0]) return false;

	if((usage & TEX_USAGE_PIC) && identifier[0])
	{
		Sys_Error("TEX_USAGE_PIC textures should not have a valid identifier");
	}

	i = BinaryFind(identifier);
	if(i == -1) return false;
	
	if(ret) *ret = &generic_cache.cache[i];
	return true;
}


void TEX_CacheAddAtEnd(int gl_texturenum, int usage, int gl_texturepref, tex_identifier identifier)
{
	texture_cache_t t;

	if(usage & (IGNORE_TEXTURE_USAGE_MASK)) return;

	TEX_CacheItemUpdate(&t, gl_texturenum, usage, gl_texturepref);

	if(!(usage & TEX_USAGE_PIC))
	{
		Q_strncpy(t.identifier, identifier, sizeof(t.identifier));
		BinaryInsert(&generic_cache, &t);
	}
	else
	{
		t.identifier[0] = 0;
		LinearInsert(&pic_cache, &t);
	}
}


/******************************************************************
* This code only deals with setting the filter mode
* If you only care about preventing memory leaks but want to keep
* the legacy behaviour of gl_texturemode, 
* don't define SOFTQUAKE_GL_TEXTUREMODE_FIX
******************************************************************/

#ifdef SOFTQUAKE_GL_TEXTUREMODE_FIX

void TEX_SetGLFilterFromPref(int min_filter, int mag_filter, int gl_texturenum, int gl_texturepref)
{
	int min_filter_pref, mag_filter_pref;

	TEX_GetTexturePrefFilter(gl_texturepref, min_filter, mag_filter, &min_filter_pref, &mag_filter_pref);

	GL_Bind(gl_texturenum);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter_pref);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter_pref);
}


static void _TEX_SetFilter(texture_collection_t *textures, int min_filter, int mag_filter, int mask)
{
	int prefs = 0;
	int nonprefs = 0;
	int i;
	texture_cache_t *tex_cache = textures->cache;
	int tex_cache_used = textures->used;
	int min_filter_auto, mag_filter_auto;

	// Cache this
	TEX_GetTexturePrefFilter(TEX_PREF_FILTER_FORCE_AUTO, min_filter, mag_filter, &min_filter_auto, &mag_filter_auto);

	for(i = 0; i < tex_cache_used; i++)
	{
		texture_cache_t *t = &tex_cache[i];
		if(t->usage & mask)
		{
			GL_Bind(t->gl_texturenum);


#if 1
			if(!t->gl_texturepref)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
			}
			else
			{
				// Not sure how much faster or slower this is when we cache things
				// The slowest part of this is making the GL calls
				if(t->gl_texturepref & (TEX_PREF_FILTER_FORCE_AUTO))
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter_auto);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter_auto);
				}
				else
				{
					int filter = t->filter;
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
				}
			}
#else
			if(!t->gl_texturepref)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
			}
			else
			{
				// Not cached
				int min_filter_pref, mag_filter_pref;
				TEX_GetTexturePrefFilter(t->gl_texturepref, min_filter, mag_filter, &min_filter_pref, &mag_filter_pref);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter_pref);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter_pref);
			}
#endif
		}
	}
}

// This allows the player to use gl_texturemode and have it take effect immediately
// The way id originally coded it, the new filter would only apply after starting a new map
// See Draw_TextureMode_f in gl_draw.c
void TEX_SetFilter(int min_filter, int mag_filter, int mask)
{
	if(mask & TEX_USAGE_PIC)
	{
		_TEX_SetFilter(&pic_cache, min_filter, mag_filter, mask);
	}
	
	mask &= ~TEX_USAGE_PIC;
	if(mask)
	{
		_TEX_SetFilter(&generic_cache, min_filter, mag_filter, mask);
	}

}

void TEX_GetTexturePrefFilter(int force_filter, int min_filter, int mag_filter, int *min_filter_out, int *mag_filter_out)
{
	if(force_filter & TEX_PREF_FILTER_FORCE_NEAREST)
	{
		*min_filter_out = GL_NEAREST;
		*mag_filter_out = GL_NEAREST;
		return;
	}
	else if(force_filter & TEX_PREF_FILTER_FORCE_LINEAR)
	{
		*min_filter_out = GL_LINEAR;
		*mag_filter_out = GL_LINEAR;
		return;
	}
	else if(force_filter & TEX_PREF_FILTER_FORCE_AUTO)
	{
#if 0
		if(min_filter == GL_LINEAR_MIPMAP_LINEAR || min_filter == GL_LINEAR_MIPMAP_NEAREST)
		{
			*min_filter_out = GL_LINEAR;
		}
		else if(min_filter == GL_NEAREST_MIPMAP_LINEAR || min_filter == GL_NEAREST_MIPMAP_NEAREST)
		{
			*min_filter_out = GL_NEAREST;
		}
#else
		// Assume mag_filter is correct
		*min_filter_out = mag_filter;
#endif
	}
	else
	{
		*min_filter_out = min_filter;
	}
	*mag_filter_out = mag_filter;
}

static void SetNearestOrLinearTexturePref(texture_cache_t *t)
{
	if(t->gl_texturepref & TEX_PREF_FILTER_FORCE_LINEAR)
	{
		t->filter = GL_LINEAR;
	}
	if(t->gl_texturepref & TEX_PREF_FILTER_FORCE_NEAREST)
	{
		t->filter = GL_NEAREST;
	}
}

#else
// No-ops
static void SetNearestOrLinearTexturePref(texture_cache_t *t)
{
}

void TEX_SetFilter(int min_filter, int mag_filter, int mask)
{
}

void TEX_SetGLFilterFromPref(int min_filter, int mag_filter, int gl_texturenum, int gl_texturepref)
{
}

void TEX_GetTexturePrefFilter(int force_filter, int min_filter, int mag_filter, int *min_filter_out, int *mag_filter_out)
{
}

#endif /* SOFTQUAKE_GL_TEXTUREMODE_FIX */
