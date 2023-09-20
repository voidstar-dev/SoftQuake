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

// Same as the sofware renderer
viddef_t	vid;				// global video state


// GLQUAKE craziness
int		texture_extension_number = 1;
unsigned short	d_8to16table[256];
unsigned 		d_8to24table[256];
unsigned char	d_15to8table[65536];

int isPermedia = 0;

qboolean gl_mtexable = 0;

float gldepthmin, gldepthmax;

int	texture_mode = GL_NEAREST;

// Renderer details
const char *gl_vendor = 0;
const char *gl_renderer = 0;
const char *gl_version = 0;
const char *gl_extensions = 0;

cvar_t	gl_ztrick = {"gl_ztrick","0"};



// Windowed mode dimensions
cvar_t vid_width = {"vid_width", "640", true};
cvar_t vid_height = {"vid_height", "480", true};

// From r_main.c. Copy the software version
cvar_t	r_clearcolor = {"r_clearcolor","2", CV_CALLBACK};

extern cvar_t ui_scale;

// Same as d_iface.h
#define WARP_WIDTH 320
#define WARP_HEIGHT 200

typedef struct
{
	float r;
	float g;
	float b;
} color3f;


typedef struct
{
	SDL_Window *Window;
	SDL_GLContext GLContext;
} sdl_context;

typedef struct
{
	int w;
	int h;
	int rate;
} vid_display_mode;

int vid_client_width = 0;
int vid_client_height = 0;
qboolean vid_first_time = true;


static sdl_context ctx;
static color3f gl_clearcolor_palette[256];

// The viewport dimensions for Quake's 2D drawing
// Does not affect GL_BeginRendering
// This is just what works for me, feel free to change this
// You don't want to set this too high, because there's no UI scaling by default
// Use 320x200 as a base
const int quake_width_2d = 320*2;
const int quake_height_2d = 200*2;


/**********************************
* Forward declarations
**********************************/
qboolean VID_IsFullscreen(void);

void VID_RecalcViddef(int w, int h)
{
	// This effectively sets the UI scale
	extern qpic_t *conback;

	vid.width = w;
	vid.height = h;
	vid.conwidth = w;
	vid.conheight = h;

	// This is needed to resize the console
	conback->width = w;
	conback->height = h;

	vid.aspect = ((float)vid.width / (float)vid.height) * (320.0 / 240.0);
	vid.recalc_refdef = 1;
}

void UI_Scale_cb(cvar_t *var)
{
	float s = var->value;
	float w, h;

	// Make this a bit more intuitive - larger numbers mean bigger scale
	// Quake does the inverse
	if(s) s = 1.0 / s;

	w = quake_width_2d * s;
	h = quake_height_2d * s;

	w = Q_max(w, 320);
	h = Q_max(h, 240);

	VID_RecalcViddef(w, h);
}

void VID_CalcUIScale()
{
	cvar_t var;

#ifdef SOFTQUAKE_GL_UI_SCALING_FIX
	var.value = ui_scale.value;
#else
	var.value = 1;
#endif
	UI_Scale_cb(&var);
}




void GL_InitClearColorPalette(unsigned char *palette)
{
	int i;
	for(i = 0; i < 256; i++)
	{
		float r = palette[i * 3 + 0] / 255.0f;
		float g = palette[i * 3 + 1] / 255.0f;
		float b = palette[i * 3 + 2] / 255.0f;

		gl_clearcolor_palette[i].r = r;
		gl_clearcolor_palette[i].g = g;
		gl_clearcolor_palette[i].b = b;
	}
}

void GL_SetClearColorFromPalette(int index)
{
	float r = gl_clearcolor_palette[index].r;
	float g = gl_clearcolor_palette[index].g;
	float b = gl_clearcolor_palette[index].b;

	glClearColor(r, g, b, 1);
}

// Implement r_clearcolor in the OpenGL version
void R_ClearColor_cb(cvar_t *var)
{
	// Some magic to mod negative numbers
	// Thanks to tsoding (smoothlife)
	// This is a general form
	// int a = var->value;
	// int b = 256;
	// int val = (a % b + b) % b;

	// From d_edge.c
	int val = ((int)var->value) & 0xFF;
	GL_SetClearColorFromPalette(val);
}


void GL_Init(unsigned char *palette);
void VID_Vsync_f();

int VID_EnumerateDisplayModes(vid_display_mode *modes, int *mode_count)
{
	// Only consider the primary monitor
	int DispModeCount;
	int DispIndex = 0;

	DispModeCount = SDL_GetNumDisplayModes(DispIndex);
	if(DispModeCount < 1) return 0;

	if(mode_count) *mode_count = DispModeCount;

	if(modes)
	{
		int i;
		SDL_DisplayMode Mode = {0};
		for(i = 0; i < DispModeCount; i++)
		{
			SDL_GetDisplayMode(DispIndex, i, &Mode);
			modes[i].w = Mode.w;
			modes[i].h = Mode.h;
			modes[i].rate = Mode.refresh_rate;
		}
	}

	return 1;
}


// Updates the current video mode
// Currently this is very simple, but it's good enough
void VID_Restart(void)
{
	Uint32 FullscreenFlag = 0;
	if(vid_fullscreen.value)
	{
		if(vid_desktopfullscreen.value)
		{
			FullscreenFlag = SDL_WINDOW_FULLSCREEN_DESKTOP;
		}
		else
		{
			FullscreenFlag = SDL_WINDOW_FULLSCREEN;
		}
	}
	else
	{
		FullscreenFlag = 0;
	}

	if(VID_IsFullscreen())
	{
		// Don't force a fullscreen change if we're already fullscreen
		// Toggle it first
		// SDL_SetWindowFullscreen(ctx.Window, FullscreenFlag);
		// SDL_SetWindowSize(ctx.Window, WindowWidth, WindowHeight);
	}
	// else
	{
		// SDL_SetWindowSize(ctx.Window, WindowWidth, WindowHeight);
		// SDL_SetWindowFullscreen(ctx.Window, FullscreenFlag);
	}

	SDL_SetWindowFullscreen(ctx.Window, FullscreenFlag);
	SDL_SetWindowSize(ctx.Window, vid_width.value, vid_height.value);

	VID_SetVsync(vid_vsync.value);
}

void VID_Restart_f(void)
{
	VID_Restart();
}

void VID_Init(unsigned char *palette)
{
	int WindowWidth = 640;
	int WindowHeight = 480;

	Cvar_RegisterVariable(&vid_fullscreen);
	Cvar_RegisterVariable(&vid_width);
	Cvar_RegisterVariable(&vid_height);

	Cvar_RegisterVariable (&gl_ztrick);

#ifdef SOFTQUAKE_GL_CLEARCOLOR_FIX
	Cvar_RegisterVariable(&r_clearcolor);
	Cvar_RegisterCallback(&r_clearcolor, R_ClearColor_cb);
#endif

#ifdef SOFTQUAKE_GL_UI_SCALING_FIX
	Cvar_RegisterCallback(&ui_scale, UI_Scale_cb);
#endif

	Cmd_AddCommand("vid_restart", VID_Restart_f);


	VID_RegisterCommonVars();

	CFG_ReadVars();

	// softquake -- Add developer mode
	if(COM_CheckParm("-developer")) developer.value = 1;

	SDL_InitSubSystem(SDL_INIT_VIDEO);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	{
		// The default parameters may fail, so set a default
		// vid_display_mode modes[128];
		// int mode_count = 0;
		// if(VID_EnumerateDisplayModes(modes, &mode_count))
		// {
		// }
	}

	ctx.Window = SDL_CreateWindow(SQ_Version("SoftQuake GL SDL"),
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			WindowWidth, WindowHeight,
			SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);

	if(!ctx.Window)
	{
		Sys_Error("Could not create window\n");
	}

	ctx.GLContext = SDL_GL_CreateContext(ctx.Window);
	if(!ctx.GLContext)
	{
		Sys_Error("Could not create OpenGL context\n");
	}

	if(SDL_GL_MakeCurrent(ctx.Window, ctx.GLContext) != 0)
	{
		Sys_Error("Could not make OpenGL context current\n");
	}

	VID_Restart();

	VID_GetClientWindowDimensions(&vid_client_width, &vid_client_height);

	GL_Init(palette);

	memset(&vid, 0, sizeof(vid));
	vid.maxwarpwidth = WARP_WIDTH;
	vid.maxwarpheight = WARP_HEIGHT;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));
	vid.numpages = 2;

	// Calculate the scale of the UI based on the SOFTQUAKE_GL_UI_SCALING_FIX define
	VID_CalcUIScale();




	// Must be done once in VID_Init
	// SUPER IMPORTANT!!!
	// SUPER IMPORTANT!!!
	VID_SetPalette(palette);
	// VID_Init8bitPalette();
	// SUPER IMPORTANT!!!
	// SUPER IMPORTANT!!!

	// Old quake has some special files for OpenGL, it seems
	{
		char gldir[MAX_OSPATH];
		q_snprintf(gldir, sizeof(gldir), "%s/glquake", com_gamedir);
		Sys_mkdir(gldir);
	}

	// Manually update the client dimensions on launch so we get the correct size
	SDL_ShowWindow(ctx.Window);
	SDL_GL_GetDrawableSize(ctx.Window, &vid_client_width, &vid_client_height);

	vid_first_time = false;
}

qboolean VID_Is8bit()
{
	return 0;
}

void VID_ShiftPalette(unsigned char *palette)
{
	// Stub
}

// Copied directly from gl_vidlinuxglx.c
void VID_SetPalette(unsigned char *palette)
{
	byte	*pal;
	unsigned r,g,b;
	unsigned v;
	int     r1,g1,b1;
	int		j,k,l,m;
	unsigned short i;
	unsigned	*table;
	char s[255];
	int dist, bestdist;

//
// 8 8 8 encoding
//
	pal = palette;
	table = d_8to24table;
	for (i=0 ; i<256 ; i++)
	{
		r = pal[0];
		g = pal[1];
		b = pal[2];
		pal += 3;

		v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
		*table++ = v;
	}
	d_8to24table[255] &= 0xffffff;	// 255 is transparent

	for (i=0; i < (1<<15); i++) {
		/* Maps
		000000000000000
		000000000011111 = Red  = 0x1F
		000001111100000 = Blue = 0x03E0
		111110000000000 = Grn  = 0x7C00
		*/
		r = ((i & 0x1F) << 3)+4;
		g = ((i & 0x03E0) >> 2)+4;
		b = ((i & 0x7C00) >> 7)+4;
		pal = (unsigned char *)d_8to24table;
		for (v=0,k=0,bestdist=10000*10000; v<256; v++,pal+=4) {
			r1 = (int)r - (int)pal[0];
			g1 = (int)g - (int)pal[1];
			b1 = (int)b - (int)pal[2];
			dist = (r1*r1)+(g1*g1)+(b1*b1);
			if (dist < bestdist) {
				k=v;
				bestdist = dist;
			}
		}
		d_15to8table[i]=k;
	}
}

qboolean VID_IsFullscreen(void)
{
	Uint32 Flags = SDL_GetWindowFlags(ctx.Window);
	return (Flags & SDL_WINDOW_FULLSCREEN) || (Flags & SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void VID_GetClientWindowDimensions(int *w, int *h)
{
	SDL_GetWindowSize(ctx.Window, w, h);
}

void VID_Shutdown()
{
	SDL_GL_MakeCurrent(ctx.Window, 0);
	SDL_GL_DeleteContext(ctx.GLContext);
	SDL_DestroyWindow(ctx.Window);

	SDL_zero(ctx);
}

void GL_BeginRendering (int *x, int *y, int *width, int *height)
{
	*x = 0;
	*y = 0;

	*width = vid_client_width;
	*height = vid_client_height;
}


// Swap buffers
void GL_EndRendering()
{
	// Cheat here and always flag the status bar so that it is always shown
	// Otherwise, it will be covered by whatever glClearColor is set to most of the time
	// Todo: Find a better place for this
	if(gl_clear.value)
	{
		Sbar_Changed();
	}

	SDL_GL_SwapWindow(ctx.Window);
}

static const char *gl_extensions_wanted[] =
{
	"GL_ARB_multitexture",
};

#if 0
// This is dumb. We're re-parsing the string every time!
int GL_ParseExtensionString(const char *complete_extension_string, const char *desired_extension)
{
	int found = 0;

	int desired_len = strlen(desired_extension);

	if(complete_extension_string && desired_extension)
	{
		const char *begin = 0;
		const char *end = 0;
		int extension_len = 0;
		int len = 0;

		for(;;)
		{
			while(complete_extension_string[0] && isspace(complete_extension_string[0]))
			{
				complete_extension_string++;
			}

			begin = complete_extension_string;
			while(complete_extension_string[0] && !isspace(complete_extension_string[0]))
			{
				complete_extension_string++;
			}
			end = complete_extension_string;

			extension_len = end - begin;

			if(!complete_extension_string[0] || extension_len == 0)
			{
				break;
			}

			if(desired_len != extension_len)
			{
				continue;
			}

			// Todo: Fix lack of const in Q_memcmp
			len = desired_len;
			if(Q_memcmp((void *)begin, (void *)desired_extension, len) == 0)
			{
				found = 1;
				break;
			}
		}
	}
	return found;
}
#else
// I didn't even realize this call existed, nice
int GL_ParseExtensionString(const char *complete_extension_string, const char *desired_extension)
{
	return SDL_GL_ExtensionSupported(desired_extension);
}
#endif

void GL_CheckExtensions()
{
	const char *desired_extension = 0;
	gl_mtexable = 0;

	gl_extensions = glGetString (GL_EXTENSIONS);


	// Todo: Make an array or enum for parsing if this gets unweildy
	// For a single extension, no need
	desired_extension = "GL_ARB_multitexture";

	if(COM_CheckParm("-nomtex"))
	{
		Con_Printf("Multitexture support disabled at the command line\n");
	}
	else if(GL_ParseExtensionString(gl_extensions, desired_extension))
	{
		// Old code
		// qglMTexCoord2fSGIS = SDL_GL_GetProcAddress("glMTexCoord2fSGIS");
		// qglSelectTextureSGIS = SDL_GL_GetProcAddress("glSelectTextureSGIS");

		// softquake -- Use standardised OpenGL functions for multitexture
		qglMTexCoord2fSGIS = SDL_GL_GetProcAddress("glMultiTexCoord2f");
		qglSelectTextureSGIS = SDL_GL_GetProcAddress("glActiveTexture");

		if(qglMTexCoord2fSGIS
		&& qglSelectTextureSGIS)
		{
			gl_mtexable = 1;
		}
		else
		{
			gl_mtexable = 0;
		}
	}

	if(gl_mtexable) Con_Printf("FOUND: %s\n", desired_extension);
}

void GL_Init(unsigned char *palette)
{
	gl_vendor = glGetString (GL_VENDOR);
	Con_Printf ("GL_VENDOR: %s\n", gl_vendor);
	gl_renderer = glGetString (GL_RENDERER);
	Con_Printf ("GL_RENDERER: %s\n", gl_renderer);

	gl_version = glGetString (GL_VERSION);
	Con_Printf ("GL_VERSION: %s\n", gl_version);

	// DONT print the extensions. The engine wasn't designed to handle this many characters
	// Probably an sprintf culprit somewhere
	// Update: I solved this by introducing a buffered print
	// It works, but it's just way too much text.
	// Instead, we'll just parse what we need and print that
	// Con_BufferedPrint("GL_EXTENSIONS: ");
	// Con_BufferedPrint(gl_extensions);
	// Con_BufferedPrint("\n");

	GL_CheckExtensions();

	// Get the classic colors back.
	GL_InitClearColorPalette(palette);
	// GL_SetClearColorFromPalette(2);

	// This was the original call from the other gl_vid implementations.
	// Nice for debugging map leaks, but does not look good
	// See R_ClearColor_cb for the replacement
#ifndef SOFTQUAKE_GL_CLEARCOLOR_FIX
	glClearColor (1,0,0,0);
#endif

	glCullFace(GL_FRONT);
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.666);

	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel (GL_FLAT);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

void VID_SetVsync(qboolean enable)
{
	if(enable) enable = 1;
	SDL_GL_SetSwapInterval(enable);
}

#if 0
void VID_Vsync_f()
{
	char *arg = Cmd_Argv(1);
	if(!arg) return;

	if(Q_strcmp(arg, "1") == 0)
	{
		VID_SetVsync(1);
		return;
	}
	if(Q_strcmp(arg, "0") == 0)
	{
		VID_SetVsync(0);
		return;
	}
}
#endif

qboolean VID_IsMinimized(void)
{
	return !(SDL_GetWindowFlags(ctx.Window) & SDL_WINDOW_SHOWN);
}

void VID_LockVariables()
{
	// Stub
}

void VID_UnlockVariables()
{
	// Stub
}
