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


// vid_sdl2.c -- The file responsible for all things display related.
//            -- Also contains the code for the video mode options


#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "quakedef.h"
#include "d_local.h"

#include <SDL2/SDL_opengl.h>
#include <stdint.h>


#include "sdl_common.h"


#define SAFE_FREE(p) free(p); p = NULL

viddef_t vid; // global video state
unsigned short d_8to16table[256]; // unused

typedef uint32_t u32;
typedef uint8_t u8;


typedef struct
{
	int w;
	int h;
	int bytes_per_pixel;
	int refresh_rate;
} vid_resolution_t;

typedef struct
{
	union
	{
		struct
		{
			u8 r;
			u8 g;
			u8 b;
			u8 a;
		};
		u32 rgba;
	};
} color_lookup_t;

typedef enum
{
	RENDER_BACKEND_SDL,
	RENDER_BACKEND_OPENGL,
} render_backend_t;

// Implement D_BeginDirectRect as a command buffer
typedef struct
{
	int x;
	int y;
	int width;
	int height;
	int size;
	byte *bitmap;
} direct_rect_command;

// This should only load 24x24 pixel images at 1 byte per pixel,
// so 1024 bytes per command is more than enough
#define DIRECT_RECT_COMMAND_CAP 64
#define DIRECT_RECT_MEMORY_CAP (1024 * DIRECT_RECT_COMMAND_CAP)
direct_rect_command drect_commands[DIRECT_RECT_COMMAND_CAP] = {0};
int drect_command_count = 0;
u8 *drect_memory = 0;
size_t drect_bytes_written = 0;


/**********************************
* Forward declarations
**********************************/
void IN_ActivateMouse(void);
static void VID_ShutdownQuakeFramebuffer(void);
static void VID_InitQuakeFramebuffer(void);
static void VID_SetFramebufferMode(int width, int height);
static void VID_SetColorMod(float r, float g, float b);
void VID_ReallocateTexture(void);
void VID_ColorMod_f();
static void D_FlushDirectRect(void);


/**********************************
* Video menu logic and rendering
**********************************/
void VID_MenuKey(int key);
void VID_MenuDraw(void);
void VID_MenuInit(void);
extern void M_Menu_Options_f (void);
extern void M_Print (int cx, int cy, char *str);
extern void M_PrintWhite (int cx, int cy, char *str);
extern void M_DrawCharacter (int cx, int line, int num);
extern void M_DrawTransPic (int x, int y, qpic_t *pic);
extern void M_DrawPic (int x, int y, qpic_t *pic);


/**********************************
* OpenGL Extensions
**********************************/
typedef void (APIENTRY *pfn_glLinkProgram) (GLuint program);
typedef void (APIENTRY *pfn_glAttachShader) (GLuint program, GLuint shader);
typedef void (APIENTRY *pfn_glShaderSource) (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (APIENTRY *pfn_glUseProgram) (GLuint program);
typedef void (APIENTRY *pfn_glUniform3fv) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY *pfn_glUniform1i) (GLint location, GLint v0);
typedef GLint (APIENTRY *pfn_glGetUniformLocation) (GLuint program, const GLchar *name);
typedef void (APIENTRY *pfn_glCompileShader) (GLuint shader);
typedef GLuint (APIENTRY *pfn_glCreateProgram) (void);
typedef GLuint (APIENTRY *pfn_glCreateShader) (GLenum type);
typedef void (APIENTRY *pfn_glDeleteProgram) (GLuint program);
typedef void (APIENTRY *pfn_glDeleteShader) (GLuint shader);
typedef void (APIENTRY *pfn_glDetachShader) (GLuint program, GLuint shader);
typedef void (APIENTRY *pfn_glGetProgramiv) (GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRY *pfn_glGetProgramInfoLog) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRY *pfn_glGetShaderiv) (GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRY *pfn_glGetShaderInfoLog) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRY *pfn_glGetShaderSource) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source);


/**********************************
* OpenGL Base
**********************************/
typedef void (APIENTRY *pfn_glBindTexture) (GLenum target, GLuint texture);
typedef void (APIENTRY *pfn_glMatrixMode) (GLenum mode);
typedef void (APIENTRY *pfn_glLoadIdentity) (void);
typedef void (APIENTRY *pfn_glColor4f) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (APIENTRY *pfn_glEnable) (GLenum cap);
typedef void (APIENTRY *pfn_glDisable) (GLenum cap);
typedef void (APIENTRY *pfn_glDeleteTextures) (GLsizei n, const GLuint *textures);

typedef void (APIENTRY *pfn_glTexSubImage2D) (GLenum target, GLint level,
                                       GLint xoffset, GLint yoffset,
                                       GLsizei width, GLsizei height,
                                       GLenum format, GLenum type,
                                       const GLvoid *pixels);

typedef void (APIENTRY *pfn_glTexImage2D) (GLenum target, GLint level,
                                    GLint internalFormat,
                                    GLsizei width, GLsizei height,
                                    GLint border, GLenum format, GLenum type,
                                    const GLvoid *pixels);

typedef void (APIENTRY *pfn_glClear) (GLbitfield mask);
typedef void (APIENTRY *pfn_glViewport) (GLint x, GLint y,
                                    GLsizei width, GLsizei height);

typedef void (APIENTRY *pfn_glBegin) (GLenum mode);
typedef void (APIENTRY *pfn_glEnd) (void);
typedef void (APIENTRY *pfn_glTexCoord2f) (GLfloat s, GLfloat t);
typedef void (APIENTRY *pfn_glVertex2f) (GLfloat x, GLfloat y);
typedef void (APIENTRY *pfn_glColor3f) (GLfloat red, GLfloat green, GLfloat blue);

typedef void (APIENTRY *pfn_glTexParameteri) (GLenum target, GLenum pname, GLint param);

// Macro magic to avoid typos
// Follows the later Quake convention of prefixing 'q' to gl calls,
// such as qglBegin, qglEnd

#define VID_GL_INIT_FUNC(f) pfn_##f q##f = 0

// Everything required for rudimentary glsl support
VID_GL_INIT_FUNC(glLinkProgram);
VID_GL_INIT_FUNC(glAttachShader);
VID_GL_INIT_FUNC(glShaderSource);
VID_GL_INIT_FUNC(glUseProgram);
VID_GL_INIT_FUNC(glUniform3fv);
VID_GL_INIT_FUNC(glUniform1i);
VID_GL_INIT_FUNC(glGetUniformLocation);
VID_GL_INIT_FUNC(glCompileShader);
VID_GL_INIT_FUNC(glCreateProgram);
VID_GL_INIT_FUNC(glCreateShader);
VID_GL_INIT_FUNC(glDeleteProgram);
VID_GL_INIT_FUNC(glDeleteShader);
VID_GL_INIT_FUNC(glDetachShader);
VID_GL_INIT_FUNC(glGetProgramiv);
VID_GL_INIT_FUNC(glGetProgramInfoLog);
VID_GL_INIT_FUNC(glGetShaderiv);
VID_GL_INIT_FUNC(glGetShaderInfoLog);
VID_GL_INIT_FUNC(glGetShaderSource);

// Basic OpenGL. I didn't want a library dependency for the software renderer, so they're being loaded at runtime
VID_GL_INIT_FUNC(glBindTexture);
VID_GL_INIT_FUNC(glMatrixMode);
VID_GL_INIT_FUNC(glLoadIdentity);
VID_GL_INIT_FUNC(glColor4f);
VID_GL_INIT_FUNC(glEnable);
VID_GL_INIT_FUNC(glDisable);
VID_GL_INIT_FUNC(glDeleteTextures);
VID_GL_INIT_FUNC(glTexSubImage2D);
VID_GL_INIT_FUNC(glTexImage2D);
VID_GL_INIT_FUNC(glClear);
VID_GL_INIT_FUNC(glViewport);
VID_GL_INIT_FUNC(glBegin);
VID_GL_INIT_FUNC(glEnd);
VID_GL_INIT_FUNC(glVertex2f);
VID_GL_INIT_FUNC(glTexCoord2f);
VID_GL_INIT_FUNC(glColor3f);
VID_GL_INIT_FUNC(glTexParameteri);

#undef VID_GL_INIT_FUNC


// Quake's internal framebuffer variables
static int vid_surfcachesize;
static void *vid_surfcache;
static void *vid_memory8; // 8 bit quake framebuffer. See VID_InitQuakeFramebuffer
static void *vid_memory32; // 32 bit RGBA texture to output
static color_lookup_t vid_palette32_ref[256]; // Lookup table to convert each 8 bit number to 32 bit RGBA colour
static color_lookup_t vid_palette32_mod[256]; // Modulated palette. Used as the final output
static int vid_allocated_quake_framebuffer;
static int vid_zbuffer_highmark = 0;

// Framebuffer size for the game
// 320x200 for the old school DOS look
// Set with fb_mode
static int quake_width  = 320*1;
static int quake_height = 200*1;

// Nice configs:
// 1.0 0.95 0.95
// 0.9 0.8 0.8
// see VID_SetColorMod
static vec3_t vid_colormod = {1, 1, 1};


// Rendering backends
// SDL_Render will work pretty much anywhere. The downside is that we have to
// manually convert framebuffer from an 8bit colormap to a 32bit rgba texture on the cpu

// OpenGL will also work on most machines. I've added glsl support to convert the colormap in a fragment shader,
// if the graphics card and drivers can handle it. That way we save on memory bandwidth as we only have to upload an 8 bit texture
// Otherwise, it'll fall back to the same logic as the SDL_Render backend by converting the colormap to a 32bit rgba texture


static render_backend_t render_backend = RENDER_BACKEND_SDL;
// static render_backend_t render_backend = RENDER_BACKEND_OPENGL;

#define FB_MODE_COUNT 3
extern cvar_t fb_mode;
static const vid_resolution_t fb_modes[FB_MODE_COUNT] =
{
	{320 * 1, 200 * 1},
	{320 * 2, 200 * 2},
	{320 * 3, 200 * 3},
};



// For resetting the video mode when using the "Test" option
typedef enum
{
	STATE_PTR_TEMP,
	STATE_PTR_ACTUAL,
} video_option_state_pointer_type;

#define VID_TEST_TIMEOUT 5
static void SetStatePointer(video_option_state_pointer_type type);
static void VID_ApplyOptions(void);
static void DefineVideoOptions(void);
static double vid_config_timer_begin = 0;
static qboolean vid_config_timer_enabled = false;
qboolean vid_first_time = true;

static void VID_SetFramebufferMode(int width, int height)
{
	quake_width = width;
	quake_height = height;

	VID_ShutdownQuakeFramebuffer();
	VID_InitQuakeFramebuffer();
	VID_ReallocateTexture();

	Con_Printf("%dx%d framebuffer resolution\n", width, height);

	// Force updates the (Quake) viewport
	vid.recalc_refdef = 1;
}

static void VID_SetFramebufferMode2(const vid_resolution_t *res)
{
	VID_SetFramebufferMode(res->w, res->h);
}

void VID_FBMode_cb(cvar_t *cvar)
{
	int mode = cvar->value;
	if(mode < 1 || mode > FB_MODE_COUNT)
	{
		Con_Printf("Invalid framebuffer mode\n");
		Con_Printf("Try values from 1 to %d\n", FB_MODE_COUNT);
	}
	else
	{
		VID_SetFramebufferMode2(&fb_modes[mode - 1]);
	}
	float value = Q_clamp(mode, 1, FB_MODE_COUNT);
	Cvar_SetValueQuick(cvar, value);
}




typedef struct
{
	SDL_Window *Window;
	SDL_Renderer *Renderer;
	SDL_Texture *RenderTarget;

	SDL_GLContext *GLContext;

	qboolean BackendInitialized;

} vid_context;

static vid_context ctx;

int vid_client_width = 0;
int vid_client_height = 0;

/**********************************
* OpenGL backend
**********************************/
static qboolean gl_glsl_enabled = false; // Always disabled at startup
static GLuint gl_loc_palette;
static GLuint gl_shader_prog;
static GLuint gl_texture;


// Just display the texture, nothing fancy
static const char *gl_vertex_shader =
"#version 110\n"
"varying out vec2 vf_Texcoords;\n"
"varying out vec4 vf_Color;\n"
"void main() {\n"
	// "vec4 v = gl_Vertex\n;"
	// "v.y *= -1.0\n;"
	"gl_Position = gl_Vertex;\n"
	"vf_Texcoords = gl_MultiTexCoord0.xy;\n"
	"vf_Color = gl_Color;\n" // 1.075, 1, 1 to match dosbox. Slightly grittier feel
"}\n"
;

// Move the color lookup to the fragment shader
// Todo: Use an integer texture? This will require using usampler2D instead of sampler2D
static const char *gl_fragment_shader =
"#version 110\n"
"uniform sampler2D Tex0;\n"
"uniform vec3 palette[256];\n"
"varying in vec2 vf_Texcoords;\n"
"varying in vec4 vf_Color;\n"
"void main() {\n"
	"int idx = int(texture2D(Tex0, vf_Texcoords).r * 255.0);\n"
	"vec4 c = vec4(palette[idx], 1) * vf_Color;\n"
	"gl_FragColor = c;\n"
"}\n"
;

GLuint GL_CompileShaderFromMemory(const char *Source, size_t Len, GLuint Type)
{
	const char *ShaderString = 0;
	char Errbuf[1024];
	int l;
	int success = 0;
	GLuint shader = qglCreateShader(Type);
	qglShaderSource(shader, 1, &Source, 0);
	qglCompileShader(shader);

	switch(Type)
	{
		case GL_VERTEX_SHADER:
			ShaderString = "vertex";
			break;
		case GL_FRAGMENT_SHADER:
			ShaderString = "fragment";
		default:
			ShaderString = "unknown";
	}

	qglGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if(success != GL_TRUE)
	{
		qglGetShaderInfoLog(shader, sizeof(Errbuf), &l, Errbuf);
		fprintf(stderr, "ERROR: Unable to compile %s shader\n", ShaderString);
		fprintf(stderr, "%s\n", Errbuf);
		exit(0);
	}

	return shader;
}

void GL_LinkProgram(GLuint program)
{
	char Errbuf[1024];
	int l;
	int success = 0;
	qglLinkProgram(program);
	qglGetProgramiv(program, GL_LINK_STATUS, &success);
	if(success != GL_TRUE)
	{
		qglGetProgramInfoLog(program, sizeof(Errbuf), &l, Errbuf);
		fprintf(stderr, "ERROR: Unable to link program\n");
		fprintf(stderr, "%s\n", Errbuf);
		exit(0);
	}
}



void GL_AllocateTexture(void)
{
	GLuint TextureFormat = 0;
	if(gl_glsl_enabled)
	{
		TextureFormat = GL_RED;
	}
	else
	{
		TextureFormat = GL_RGBA;
	}

	qglTexImage2D(GL_TEXTURE_2D, 0, TextureFormat, quake_width, quake_height, 0, TextureFormat, GL_UNSIGNED_BYTE, 0);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

static void GL_LoadExtensions(qboolean load_glsl)
{
	gl_glsl_enabled = load_glsl;

	#define VID_GL_LOAD_EXTENSION(name) \
	do { \
		q##name = SDL_GL_GetProcAddress(#name); \
		if(!(q##name)) \
		{ \
			if(load_glsl) gl_glsl_enabled = 0; \
			return; \
		} \
	} while(0)

	VID_GL_LOAD_EXTENSION(glBindTexture);
	VID_GL_LOAD_EXTENSION(glMatrixMode);
	VID_GL_LOAD_EXTENSION(glLoadIdentity);
	VID_GL_LOAD_EXTENSION(glColor4f);
	VID_GL_LOAD_EXTENSION(glEnable);
	VID_GL_LOAD_EXTENSION(glDisable);
	VID_GL_LOAD_EXTENSION(glDeleteTextures);
	VID_GL_LOAD_EXTENSION(glTexSubImage2D);
	VID_GL_LOAD_EXTENSION(glTexImage2D);
	VID_GL_LOAD_EXTENSION(glClear);
	VID_GL_LOAD_EXTENSION(glViewport);
	VID_GL_LOAD_EXTENSION(glBegin);
	VID_GL_LOAD_EXTENSION(glEnd);
	VID_GL_LOAD_EXTENSION(glVertex2f);
	VID_GL_LOAD_EXTENSION(glTexCoord2f);
	VID_GL_LOAD_EXTENSION(glColor3f);
	VID_GL_LOAD_EXTENSION(glTexParameteri);

	if(load_glsl)
	{
		VID_GL_LOAD_EXTENSION(glLinkProgram);
		VID_GL_LOAD_EXTENSION(glAttachShader);
		VID_GL_LOAD_EXTENSION(glShaderSource);
		VID_GL_LOAD_EXTENSION(glUseProgram);
		VID_GL_LOAD_EXTENSION(glUniform3fv);
		VID_GL_LOAD_EXTENSION(glUniform1i);
		VID_GL_LOAD_EXTENSION(glGetUniformLocation);
		VID_GL_LOAD_EXTENSION(glCompileShader);
		VID_GL_LOAD_EXTENSION(glCreateProgram);
		VID_GL_LOAD_EXTENSION(glCreateShader);
		VID_GL_LOAD_EXTENSION(glDeleteProgram);
		VID_GL_LOAD_EXTENSION(glDeleteShader);
		VID_GL_LOAD_EXTENSION(glDetachShader);
		VID_GL_LOAD_EXTENSION(glGetProgramiv);
		VID_GL_LOAD_EXTENSION(glGetProgramInfoLog);
		VID_GL_LOAD_EXTENSION(glGetShaderiv);
		VID_GL_LOAD_EXTENSION(glGetShaderInfoLog);
		VID_GL_LOAD_EXTENSION(glGetShaderSource);
	}

#undef VID_GL_LOAD_EXTENSION
}

void GL_InitBackend(void)
{
	GLuint VertexShader, FragmentShader;
	qboolean load_glsl = false;

	if(render_backend != RENDER_BACKEND_OPENGL) return;

	ctx.GLContext = SDL_GL_CreateContext(ctx.Window);
	if(!ctx.GLContext)
	{
		Sys_Error("Could not create an OpenGL context");
	}
	if(SDL_GL_MakeCurrent(ctx.Window, ctx.GLContext))
	{
		Sys_Error("Could not set OpenGL context as current");
	}

	// Just in case...
	gl_glsl_enabled = 0;

	// softquake -- Use the same switches as Quakespasm
	if(COM_CheckParm("-noglsl"))
	{
		load_glsl = false;
		Con_Printf("GLSL capability disabled at the command line\n");
	}
	else
	{
		load_glsl = true;
	}

	GL_LoadExtensions(load_glsl);

	Con_Printf("GLSL enabled: %s\n", gl_glsl_enabled ? "yes" : "no");

	if(gl_glsl_enabled)
	{
		gl_shader_prog = qglCreateProgram();

		VertexShader = GL_CompileShaderFromMemory(gl_vertex_shader, strlen(gl_vertex_shader), GL_VERTEX_SHADER);
		FragmentShader = GL_CompileShaderFromMemory(gl_fragment_shader, strlen(gl_fragment_shader), GL_FRAGMENT_SHADER);

		qglAttachShader(gl_shader_prog, VertexShader);
		qglAttachShader(gl_shader_prog, FragmentShader);
		GL_LinkProgram(gl_shader_prog);
		qglDetachShader(gl_shader_prog, VertexShader);
		qglDetachShader(gl_shader_prog, FragmentShader);
		qglDeleteShader(VertexShader);
		qglDeleteShader(FragmentShader);

		gl_loc_palette = qglGetUniformLocation(gl_shader_prog, "palette[0]");
		qglUseProgram(gl_shader_prog);
	}
	else
	{
		qglEnable(GL_TEXTURE_2D);
	}

	qglBindTexture(GL_TEXTURE_2D, gl_texture);
	GL_AllocateTexture();

	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();

	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();

	qglColor4f(1, 1, 1, 1);

	qglDisable(GL_CULL_FACE);
}

void GL_ShutdownBackend(void)
{
	if(render_backend != RENDER_BACKEND_OPENGL) return;

	qglDeleteTextures(1, &gl_texture);

	if(gl_glsl_enabled)
	{
		qglUseProgram(0);
		qglDeleteProgram(gl_shader_prog);
	}

	SDL_GL_DeleteContext(ctx.GLContext);
}

void VID_GetClientWindowDimensions(int *w, int *h)
{
	SDL_GetWindowSize(ctx.Window, w, h);
}

qboolean VID_IsFullscreen(void)
{
	Uint32 Flags = SDL_GetWindowFlags(ctx.Window);
	return (Flags & SDL_WINDOW_FULLSCREEN) || (Flags & SDL_WINDOW_FULLSCREEN_DESKTOP);
}

int VID_SDL2GetBPP(const SDL_DisplayMode *Mode)
{
	return SDL_BITSPERPIXEL(Mode->format);
}

// Todo: Factor in refresh rate and depth
qboolean VID_ModeIsValid(int width, int height, int refresh_rate, int bpp)
{
	int disp_index = 0;
	int num_modes = SDL_GetNumDisplayModes(disp_index);
	if(num_modes > 0)
	{
		int i;
		for(i = 0; i < num_modes; i++)
		{
			SDL_DisplayMode mode;
			if(SDL_GetDisplayMode(disp_index, i, &mode))
			{
				continue;
			}
			if(mode.w == width
			&& mode.h == height
			&& mode.refresh_rate == refresh_rate)
			{
				return true;
			}
		}
	}
	return false;
}


/**********************************
* Cvars
**********************************/


// Video mode dimensions
cvar_t vid_width = {"vid_width", "640", true};
cvar_t vid_height = {"vid_height", "480", true};

// Video mode refresh rate
// Following the conventions of Quakespasm and others
cvar_t vid_refreshrate = {"vid_refreshrate", "60", true};
cvar_t vid_bpp = {"vid_bpp", "24", true};



cvar_t vid_mode = {"vid_mode", "0", true};


void VID_Restart(void);
void VID_Restart_f(void)
{
	VID_Restart();
	VID_SetVsync(vid_vsync.value);
}

// These were being used in multiple places they're in a function call now
int CreateRenderTargetSDL2(void)
{
	SDL_DestroyTexture(ctx.RenderTarget);
	ctx.RenderTarget = SDL_CreateTexture(ctx.Renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, quake_width, quake_height);
	return ctx.RenderTarget != NULL;
}

int CreateRendererSDL2(qboolean vsync_enabled)
{
	Uint32 Flags = SDL_RENDERER_ACCELERATED;

	if(vsync_enabled) Flags |= SDL_RENDERER_PRESENTVSYNC;

	SDL_DestroyRenderer(ctx.Renderer);
	ctx.Renderer = SDL_CreateRenderer(ctx.Window, -1, Flags);
	return ctx.Renderer != NULL;
}

void VID_ReallocateTexture(void)
{
	if(!ctx.BackendInitialized) return;

	switch(render_backend)
	{
		case RENDER_BACKEND_SDL:
			CreateRenderTargetSDL2();
			break;
		case RENDER_BACKEND_OPENGL:
			qglBindTexture(gl_texture, GL_TEXTURE_2D);
			GL_AllocateTexture();
			break;
		default:
			Sys_Error("Render backend not handled\n");
	}
}

SDL_DisplayMode *VID_GetCurrentDisplayMode(void)
{
	static SDL_DisplayMode Mode;
	int DisplayIndex = SDL_GetWindowDisplayIndex(ctx.Window);
	int Rc = SDL_GetCurrentDisplayMode(DisplayIndex, &Mode);
	q_assert(Rc == 0);
	return &Mode;
}

void VID_LockVariables(void)
{
	Cvar_LockVariable(&vid_width);
	Cvar_LockVariable(&vid_height);
	Cvar_LockVariable(&vid_refreshrate);
	Cvar_LockVariable(&vid_bpp);
}

void VID_UnlockVariables(void)
{
	Cvar_UnlockVariable(&vid_width);
	Cvar_UnlockVariable(&vid_height);
	Cvar_UnlockVariable(&vid_refreshrate);
	Cvar_UnlockVariable(&vid_bpp);
}

void VID_Restart(void)
{
	int WindowWidth = 640;
	int WindowHeight = 480;
	int RefreshRate = 60;
	int Bpp = 24;
	int Fullscreen = 0;
	Uint32 FullscreenFlag = 0;
	Uint32 WindowFlags = 0;

	VID_UnlockVariables();

	if(vid_fullscreen.value)
	{
		Fullscreen = 1;
	}
	else
	{
		WindowWidth = vid_width.value;
		WindowHeight = vid_height.value;
	}

	if(render_backend == RENDER_BACKEND_OPENGL)
	{
		WindowFlags |= SDL_WINDOW_OPENGL;

		SDL_GL_SetAttribute(GL_RED_BITS, 8);
		SDL_GL_SetAttribute(GL_GREEN_BITS, 8);
		SDL_GL_SetAttribute(GL_BLUE_BITS, 8);
	}

	if(!ctx.Window)
	{
		ctx.Window = SDL_CreateWindow(SQ_Version("SoftQuake SDL"), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
				WindowWidth, WindowHeight, WindowFlags | SDL_WINDOW_HIDDEN);
		if(!ctx.Window)
		{
			Sys_Error("Could not create window");
		}

		// SDL_DestroyRenderer also destroys any textures allocated with it
		SDL_DestroyRenderer(ctx.Renderer);
		ctx.Renderer = 0;
		ctx.RenderTarget = 0;
	}

	if(render_backend == RENDER_BACKEND_SDL)
	{
		if(!ctx.Renderer)
		{
			if(!CreateRendererSDL2(vid_vsync.value))
			{
				Sys_Error("Could not create renderer");
			}
		}
		if(!ctx.RenderTarget)
		{
			if(!CreateRenderTargetSDL2())
			{
				Sys_Error("Could not create render target");
			}
		}
	}

	if(Fullscreen)
	{
		// Get the closest display of the window
		// Get its dimensions
		// Set the window size
		// Set fullscreen

		// New plan:
		// Check if vid_width and vid_height are valid
		// If they are, set the display to that mode


		SDL_DisplayMode Mode = {0};
		int DisplayIndex = SDL_GetWindowDisplayIndex(ctx.Window);
		SDL_GetCurrentDisplayMode(DisplayIndex, &Mode);
		WindowWidth = Mode.w;
		WindowHeight = Mode.h;

		if(vid_desktopfullscreen.value)
		{
			FullscreenFlag = SDL_WINDOW_FULLSCREEN_DESKTOP;
		}
		else
		{
			int desired_w = vid_width.value;
			int desired_h = vid_height.value;
			FullscreenFlag = SDL_WINDOW_FULLSCREEN;

			// Check commandline -width and -height parameters
			VID_CheckParams(&desired_w, &desired_h);

			if(VID_ModeIsValid(desired_w, desired_h, vid_refreshrate.value, vid_bpp.value))
			{
				Cvar_SetValueQuick(&vid_width, desired_w);
				Cvar_SetValueQuick(&vid_height, desired_h);

				WindowWidth = vid_width.value;
				WindowHeight = vid_height.value;
				RefreshRate = vid_refreshrate.value;
				Bpp = vid_bpp.value;
			}
			else
			{
				Con_Printf("ERROR: %dx%d is not a valid fullscreen mode\n", desired_w, desired_h);
				Con_Printf("Defaulting to current monitor resolution: %dx%d\n", WindowWidth, WindowHeight);
				Cvar_SetValueQuick(&vid_width, WindowWidth);
				Cvar_SetValueQuick(&vid_height, WindowHeight);
				Cvar_SetValueQuick(&vid_refreshrate, RefreshRate);
				Cvar_SetValueQuick(&vid_bpp, Bpp);

				// Note: On the first run, Host_Init overwrites the variables that we set because it runs
				// Cbuf_InsertText ("exec quake.rc\n");
				// We could introduce a locking mechanism to prevent cvars from being overwritten
			}
		}
	}
	else
	{
		// Check windowed mode -width and -height parameters
		int desired_w = WindowWidth;
		int desired_h = WindowHeight;
		VID_CheckParams(&desired_w, &desired_h);
		WindowWidth = desired_w;
		WindowHeight = desired_h;
		Cvar_SetValueQuick(&vid_width, desired_w);
		Cvar_SetValueQuick(&vid_height, desired_h);
	}

	if(VID_IsFullscreen())
	{
		// Don't force a fullscreen change if we're already fullscreen
		// Toggle it first
		SDL_SetWindowFullscreen(ctx.Window, FullscreenFlag);
		SDL_SetWindowSize(ctx.Window, WindowWidth, WindowHeight);
	}
	else
	{
		SDL_SetWindowSize(ctx.Window, WindowWidth, WindowHeight);
		SDL_SetWindowFullscreen(ctx.Window, FullscreenFlag);
	}

	// "Real" fullscreen mode
	// Change the display mode here
	// SDL_WINDOW_FULLSCREEN_DESKTOP also contains the SDL_WINDOW_FULLSCREEN bits
	if((FullscreenFlag & SDL_WINDOW_FULLSCREEN_DESKTOP) == SDL_WINDOW_FULLSCREEN)
	{
		// softquake -- Found an X11 bug (maybe an SDL bug too)
		// Steps to reproduce:
		// Set your monitor resolution to anything other than its native resolution
		// Set the display mode to "Fullscreen"
		// The window size will change, but the monitor display resolution will stay exactly the same
		// Minimal example C code
		/*
		    // Assumes that your current display resolution is not the native one, and that the new mode is a valid mode

			SDL_Window *Window = SDL_CreateWindow("TestWindow", 0, 0, 800, 600, 0);

			SDL_DisplayMode Mode;
			SDL_GetWindowDisplayMode(Window, &Mode);
			Mode.w = 1280;
			Mode.h = 720;
			Mode.refresh_rate = 60;
			SDL_SetWindowSize(Window, Mode.w, Mode.h);
			SDL_SetWindowFullscreen(Window, SDL_WINDOW_FULLSCREEN);
			SDL_SetWindowDisplayMode(Window, &Mode);
		*/

		// Intended behaviour:
		// The monitor resolution should change to the requested video mode

		// I used xrandr to confirm that changing the display modes works in the command line, while the game was running

		SDL_SetWindowSize(ctx.Window, WindowWidth, WindowHeight);
		SDL_SetWindowFullscreen(ctx.Window, FullscreenFlag);
		SDL_DisplayMode Mode = {0};
		Mode.w = WindowWidth;
		Mode.h = WindowHeight;
		Mode.refresh_rate = RefreshRate;
		Mode.format = SDL_GetWindowPixelFormat(ctx.Window);

		// Note: I've discovered that SDL will simply ignore the refresh_rate member if it's invalid
		SDL_SetWindowDisplayMode(ctx.Window, &Mode);
	}


	// Todo: This line will always centre the window
	// Comment this line out to have the windowed-mode window go back to its previous position when toggled
	SDL_SetWindowPosition(ctx.Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	SDL_ShowWindow(ctx.Window);
	// SDL_SetWindowResizable(ctx.Window, Fullscreen ? 0 : 1);

	VID_GetClientWindowDimensions(&vid_client_width, &vid_client_height);

	// If these cvars aren't set, then it allows for toggling between windowed and fullscreen modes with Alt-Enter
	// Cvar_SetValueQuick(&vid_width, vid_client_width);
	// Cvar_SetValueQuick(&vid_height, vid_client_height);
}

static void VID_ShutdownQuakeFramebuffer(void)
{
	SAFE_FREE(vid_memory32);
	SAFE_FREE(vid_memory8);

	// This is needed
	D_FlushCaches();
	Hunk_FreeToHighMark(vid_zbuffer_highmark);

	vid_allocated_quake_framebuffer = 0;
}

static void VID_InitQuakeFramebuffer(void)
{
	int bytes_per_line;

	if(vid_allocated_quake_framebuffer) return;

	vid_allocated_quake_framebuffer = 1;

	bytes_per_line = quake_width * 1; // 8 bit colormap

	// Most of this setup logic was borrowed from vid_x.c
	memset(&vid, 0, sizeof(vid));
	vid.width = quake_width;
	vid.height = quake_height;
	vid.rowbytes = bytes_per_line;
	vid.conwidth = vid.width;
	vid.conheight = vid.height;
	vid.conrowbytes = vid.rowbytes;
	vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);

	vid.maxwarpwidth = WARP_WIDTH;
	vid.maxwarpheight = WARP_HEIGHT;
	vid.numpages = 1;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));

	// Let Quake use an 8 bit buffer to store its palette information
	// We'll convert it later in VID_SetPalette / VID_Update
	vid_memory8 = malloc(vid.width * vid.height * 1);
	vid_memory32 = malloc(vid.width * vid.height * 4);
	vid.buffer = vid_memory8;
	vid.conbuffer = vid.buffer;

	// Z buffer and surf cache
	// Not sure how they're used but the game needs this memory layout in order to work
	// The logic for this setup is found in vid_x.c, in function ResetFrameBuffer / ResetSharedFrameBuffers
	{
		size_t buffersize = vid.width * vid.height * sizeof(*d_pzbuffer);
		size_t surfcachesize = D_SurfaceCacheForRes(vid.width, vid.height);
		size_t zbuffersize = buffersize + surfcachesize;

		// Allocate Z buffer
		// softquake -- I had some memory allocation issues using Hunk_HighAllocName when allocating a zbuffer larger than 1280*720
		// Allocating with malloc instead seems to work
		// This seems to be a constraint of the original engine
		// I won't be doing any "limit removing" in this port, so this code can stay
		// Update: This can be fixed by passing the -mem commandline parameter to request more memory on startup
		// ex: -mem 64 asks for 64 megabytes of memory
		// The default is 8 megabytes. See parms.memsize in 'main_sdl.c'
		vid_zbuffer_highmark = Hunk_HighMark();
		d_pzbuffer = Hunk_HighAllocName(zbuffersize, "video");

		if(!d_pzbuffer)
		{
			Sys_Error ("Not enough memory for video mode\n");
		}
		// Allocate surface cache
		{
			size_t surfcache_offset = buffersize;
			u8 *zbuffer = (u8 *)d_pzbuffer;
			vid_surfcache = &zbuffer[surfcache_offset];
			D_InitCaches(vid_surfcache, surfcachesize);
		}
	}
}

void VID_ChooseBackend(void)
{
	if(Q_strcmp("sdl", sw_backend.string) == 0)
	{
		render_backend = RENDER_BACKEND_SDL;
		return;
	}

	if(Q_strcmp("ogl", sw_backend.string) == 0)
	{
		render_backend = RENDER_BACKEND_OPENGL;
		return;
	}

	Con_Printf("Invalid backend chosen: %s\n", sw_backend.string);
	Con_Printf("Defaulting to SDL2");
	render_backend = RENDER_BACKEND_SDL;
}

void VID_InitBackend(void)
{
	const char *RenderBackendString = 0;
	switch(render_backend)
	{
		case RENDER_BACKEND_SDL:
			// Todo: This is currently done in VID_Restart
			RenderBackendString = "SDL_Render";
			break;
		case RENDER_BACKEND_OPENGL:
			GL_InitBackend();
			RenderBackendString = "OpenGL";
			break;
		default:
			Sys_Error("Render backend not handled\n");
	}

	VID_SetVsync(vid_vsync.value);

	q_assert(RenderBackendString);
	ctx.BackendInitialized = true;
	Con_Printf("Render backend: %s\n", RenderBackendString);
}

void VID_SDLMode_f(void)
{
	// Debug code
	SDL_DisplayMode *Mode = VID_GetCurrentDisplayMode();
	Con_Printf("w: %d\n", Mode->w);
	Con_Printf("h: %d\n", Mode->h);
	Con_Printf("rate: %d\n", Mode->refresh_rate);
}

/**********************************
* VID
**********************************/
void VID_Init(unsigned char *palette)
{
	int Fullscreen = 0;

	Cvar_RegisterVariable(&vid_fullscreen);
	Cvar_RegisterVariable(&vid_width);
	Cvar_RegisterVariable(&vid_height);
	Cvar_RegisterVariable(&vid_refreshrate);
	Cvar_RegisterVariable(&vid_bpp);

	Cvar_RegisterCallback(&fb_mode, VID_FBMode_cb);

	VID_RegisterCommonVars();

	Cmd_AddCommand("vid_restart", VID_Restart_f);

	Cmd_AddCommand("vid_colormod", VID_ColorMod_f);

	// Todo: Get rid of this. Temp code
	Cmd_AddCommand("sdl_mode", VID_SDLMode_f);

	// This assumes that Cbuf_Init was called before VID_Init
	// See Host_Init in host.c and make sure this is the case
	CFG_ReadVars();

	VID_ChooseBackend();

	// softquake -- Add developer mode
	if(COM_CheckParm("-developer")) developer.value = 1;

	if(SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
	{
		Sys_Error("Failed to initialize SDL: %s\n", SDL_GetError());
	}

	vid_menudrawfn = VID_MenuDraw;
	vid_menukeyfn = VID_MenuKey;
	vid_menuinitfn = VID_MenuInit;


	VID_Restart();
	VID_InitBackend();

	IN_ActivateMouse();
	VID_InitQuakeFramebuffer();
	DefineVideoOptions();

	drect_memory = malloc(DIRECT_RECT_MEMORY_CAP);

	vid_first_time = false;
}

void VID_ShiftPalette(unsigned char *palette)
{
	VID_SetPalette(palette);
}

// Do this in on the CPU to get consistent color saturation logic
// This allows us to use vid_colormod values greater than 1 without worrying
// about SDL or OpenGL clamping them to 1
void VID_ModPalette(void)
{
	int i;

	if(vid_colormod[0] == 1 && vid_colormod[1] == 1 && vid_colormod[2] == 1)
	{
		// Fast path
		// Could use a pointer instead to get rid of this memcpy
		memcpy(vid_palette32_mod, vid_palette32_ref, sizeof(vid_palette32_mod));
	}
	else
	{
		for(i = 0; i < 256; i++)
		{
			// Converted to ints so we don't underflow
			int r = vid_palette32_ref[i].r;
			int g = vid_palette32_ref[i].g;
			int b = vid_palette32_ref[i].b;
			int a = vid_palette32_ref[i].a;

			// This saturates the color, which is the intended effect for now
			r *= vid_colormod[0]; r = Q_clamp(r, 0, 255);
			g *= vid_colormod[1]; g = Q_clamp(g, 0, 255);
			b *= vid_colormod[2]; b = Q_clamp(b, 0, 255);

			vid_palette32_mod[i].r = r;
			vid_palette32_mod[i].g = g;
			vid_palette32_mod[i].b = b;
			vid_palette32_mod[i].a = a;
		}
	}
}

void VID_StorePalette(unsigned char *palette)
{
	int i;

	for(i = 0; i < 256; i++)
	{
		int r = palette[i * 3 + 0];
		int g = palette[i * 3 + 1];
		int b = palette[i * 3 + 2];
		int a = 0xFF;

		vid_palette32_ref[i].r = r;
		vid_palette32_ref[i].g = g;
		vid_palette32_ref[i].b = b;
		vid_palette32_ref[i].a = a;
	}
}

void GL_UpdatePalette(void)
{
	if(gl_glsl_enabled)
	{
		int i;

		struct
		{
			float r;
			float g;
			float b;
		} glsl_vec3[256];

		for(i = 0; i < 256; i++)
		{
			float r = vid_palette32_mod[i].r / 255.0f;
			float g = vid_palette32_mod[i].g / 255.0f;
			float b = vid_palette32_mod[i].b / 255.0f;

			glsl_vec3[i].r = r;
			glsl_vec3[i].g = g;
			glsl_vec3[i].b = b;
		}
		// Fewer api calls
		qglUniform3fv(gl_loc_palette, 256, (float *)glsl_vec3);
	}
	else
	{
		// Already done in VID_SetPalette
	}
}

void VID_UpdatePalette(void)
{
	switch(render_backend)
	{
		case RENDER_BACKEND_SDL:
			// Already taken care of by VID_SetPalette
			break;
		case RENDER_BACKEND_OPENGL:
			GL_UpdatePalette();
			break;
		default:
			Sys_Error("Render backend not handled\n");
	}
}

// 256 palette
void VID_SetPalette(unsigned char *palette)
{
	VID_StorePalette(palette);
	VID_ModPalette();
	VID_UpdatePalette();
}

// No indexed texture modes, just doing it directly in software
static void CopyColormap8ToTexture32(void)
{
	int x, y;
	int idx = 0;
	const u8 *vram8 = vid_memory8;
	color_lookup_t *vram32 = (color_lookup_t *)vid_memory32;

	// Now unrolled for a bit of speed gain
	q_assert(vid.width % 4 == 0);

	for(y = 0; y < vid.height; y++)
	{
		for(x = 0; x < vid.width; x += 4)
		{
			vram32[idx + 0] = vid_palette32_mod[vram8[idx + 0]];
			vram32[idx + 1] = vid_palette32_mod[vram8[idx + 1]];
			vram32[idx + 2] = vid_palette32_mod[vram8[idx + 2]];
			vram32[idx + 3] = vid_palette32_mod[vram8[idx + 3]];

			idx += 4;
		}
	}
}

void SW_Render(void)
{
	CopyColormap8ToTexture32();

	SDL_RenderClear(ctx.Renderer);
	SDL_UpdateTexture(ctx.RenderTarget, 0, vid_memory32, vid.width * 4);
	SDL_RenderCopyEx(ctx.Renderer, ctx.RenderTarget, 0, 0, 0, 0, SDL_FLIP_NONE);
	SDL_RenderPresent(ctx.Renderer);
}

void GL_Render(void)
{
	const float s = 1;

	if(gl_glsl_enabled)
	{
		qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, quake_width, quake_height, GL_RED, GL_UNSIGNED_BYTE, vid_memory8);
	}
	else
	{
		CopyColormap8ToTexture32();
		qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, quake_width, quake_height, GL_RGBA, GL_UNSIGNED_BYTE, vid_memory32);
	}


	qglClear(GL_COLOR_BUFFER_BIT);
	qglViewport(0, 0, vid_client_width, vid_client_height);

	qglBegin(GL_QUADS);

	// Note: y is flipped in the glVertex2f calls so we don't have to do any more work in the vertex shader
	// To flip it back to OpenGL's coordinate system, simply invert the signs of the y components
	qglTexCoord2f(0, 0); qglVertex2f(-1, 1 * s);
	qglTexCoord2f(0, 1); qglVertex2f(-1, -1 * s);
	qglTexCoord2f(1, 1); qglVertex2f(1, -1 * s);
	qglTexCoord2f(1, 0); qglVertex2f(1, 1 * s);

	qglEnd();

	SDL_GL_SwapWindow(ctx.Window);
}

void SW_InitBackend(void)
{
}


void VID_Update(vrect_t *rects)
{
	D_FlushDirectRect();

	switch(render_backend)
	{
		case RENDER_BACKEND_SDL:
			SW_Render();
			break;
		case RENDER_BACKEND_OPENGL:
			GL_Render();
			break;
		default:
			Sys_Error("Render backend not handled\n");
	}

	// Countdown timer for "Test" option in the video settings
	if(vid_config_timer_enabled)
	{
		if((realtime - vid_config_timer_begin) >= VID_TEST_TIMEOUT)
		{
			// Reset video mode back to the way it was
			vid_config_timer_enabled = false;
			SetStatePointer(STATE_PTR_ACTUAL);
			VID_ApplyOptions();
			SetStatePointer(STATE_PTR_TEMP);
		}
	}
}


void VID_Shutdown(void)
{
	GL_ShutdownBackend();

	SDL_DestroyRenderer(ctx.Renderer);
	SDL_DestroyWindow(ctx.Window);
	SDL_Quit();

	VID_ShutdownQuakeFramebuffer();
}



/**********************************
* Direct Rect
**********************************/
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
	// From vid_dos.c
	if((width > 24) || (height > 24) || (width < 1) || (height < 1))
	{
		return;
	}
	if(width & 0x03)
	{
		return;
	}
#if 1
	{
	int bitmap_size = width * height;
	u8 *drect_memory_dst = 0;
	direct_rect_command *cmd = 0;
	if(bitmap_size + drect_bytes_written >= DIRECT_RECT_MEMORY_CAP)
	{
		Con_DPrintf("DirectRect: Bitmap buffer overflow\n");
		return;
	}
	if(drect_command_count >= DIRECT_RECT_COMMAND_CAP)
	{
		Con_DPrintf("DirectRect: Command buffer overflow\n");
		return;
	}
	if(!vid_memory8)
	{
		return;
	}
	if(!drect_memory)
	{
		return;
	}

	// Implement DirectRect drawing as a command buffer
	// The idea is that the game may draw over the icons with Draw_FadeScreen,
	// and I want to guarantee that we draw over that effect.
	// Maybe it's a non-issue, more testing is needed.

	// Temporary: Only use the first slot, since we're only drawing once per frame anyway.

	// Fix this if drawing more images!!!
	drect_bytes_written = 0;
	drect_memory_dst = &drect_memory[drect_bytes_written];

	// Increment drect_bytes_written if this is changed

	memcpy(drect_memory_dst, pbitmap, bitmap_size);
	cmd = &drect_commands[0];
	drect_command_count = 1;
	cmd->x = x;
	cmd->y = y;
	cmd->width = width;
	cmd->height = height;
	cmd->size = bitmap_size;
	cmd->bitmap = drect_memory_dst;
	}
#else
	// This was probably meant to write to the VGA hardware
	// Since we're writing to our own memory, check if its valid first
	{
	int dx, dy;
	u8 *vram8 = vid_memory8;
	if(!vram8) return;
	for(dy = 0; dy < height; dy++)
	{
		int y_offset = dy + y;
		for(dx = 0; dx < width; dx++)
		{
			int x_offset = dx + x;
			int p = y_offset * quake_width + x_offset;
			vram8[p] = *pbitmap;
			pbitmap++;
		}
	}
	}
#endif
}

void D_EndDirectRect (int x, int y, int width, int height)
{
	// Nothing to do
}

static void D_FlushDirectRect(void)
{
	u8 *vram8 = vid_memory8;
	int i;

	int cmd_count = drect_command_count;
	drect_command_count = 0;
	drect_bytes_written = 0;
	if(!vram8) return;

	for(i = 0; i < cmd_count; i++)
	{
		const direct_rect_command *cmd = &drect_commands[i];
		int x = cmd->x;
		int y = cmd->y;
		int width = cmd->width;
		int height = cmd->height;
		byte *pbitmap = cmd->bitmap;
		int dx, dy;
		for(dy = 0; dy < height; dy++)
		{
			int y_base = dy + y;
			int y_offset = y_base * quake_width;
			for(dx = 0; dx < width; dx++)
			{
				int x_offset = dx + x;
				int p = y_offset + x_offset;
				vram8[p] = *pbitmap;
				pbitmap++;
			}
		}
	}
}

void VID_SetVsync(qboolean enable)
{
	if(enable) enable = 1;

	switch(render_backend)
	{
		case RENDER_BACKEND_SDL:
			// This is dumb but it's the only way to do this through the current SDL2 render api (2.0.12)
			CreateRendererSDL2(enable);
			CreateRenderTargetSDL2();

			// Not available in my current version of SDL2
			// According to the documentation, 2.0.18 is the lowest version to support this call
			// SDL_RenderSetVSync(ctx.Renderer, enable);

			break;
		case RENDER_BACKEND_OPENGL:
			SDL_GL_SetSwapInterval(enable);
			break;
		default:
			Sys_Error("Render backend not handled\n");
	}
}

// Allows the player to customize the final output of the framebuffer beyond Quake's default brightness controls.
// Could be used for gamma correction or just overall messing about with colors.
// vid_palette32_ref is used as a reference palette, which is then stored into vid_palette32_mod
// after being multipled by the vid_colormod factors.
// My preferences: (tries to match DOSBox)
// 1.0 0.95 0.95
// 1.05 1.0 1.0
static void VID_SetColorMod(float r, float g, float b)
{
	vid_colormod[0] = r;
	vid_colormod[1] = g;
	vid_colormod[2] = b;

	VID_ModPalette();
	VID_UpdatePalette();
}

void VID_ColorMod_f(void)
{
	vec3_t rgb;
	int argc = Cmd_Argc();
	int args = Q_min(4, argc);
	VectorCopy(vid_colormod, rgb);

	if(args > 1)
	{
		int i;
		for(i = 1; i < args; i++)
		{
			char *arg = Cmd_Argv(i);
			rgb[i - 1] = Q_atof(arg);
		}
		VID_SetColorMod(rgb[0], rgb[1], rgb[2]);
	}
	else
	{
		// Todo: If we print more existing variables, introduce some sort of indentation function
		Con_Printf(" %.2f %.2f %.2f\n", vid_colormod[0], vid_colormod[1], vid_colormod[2]);
	}
}

qboolean VID_IsMinimized(void)
{
	return !(SDL_GetWindowFlags(ctx.Window) & SDL_WINDOW_SHOWN);
}


/**********************************
* Video mode logic
* This is possibly a huge over-engineered mess
* but it works
**********************************/


typedef struct
{
	int cursor;
	int text_style;
	qboolean mode_invalid;
} video_options;


typedef struct
{
	int row;
	const char *text;
	qboolean selectable;
} video_option;

typedef enum
{
	VID_OPT_VIDMODE,
	VID_OPT_DEPTH,
	VID_OPT_REFRESH_RATE,
	VID_OPT_MODESTATE,
	VID_OPT_VSYNC,
	VID_OPT_FB_MODE,
	VID_OPT_TEST,
	VID_OPT_APPLY,

	VID_OPT_TOTAL
} video_option_type;


typedef struct
{
	// In case an option has multiple things that can be set from it
	// 4 is an arbitrary number but it should cover our use case
	int selection_index[4];
} video_option_state;

/**********************************
* Video option variables
**********************************/
static video_option vid_options[VID_OPT_TOTAL];

static video_option_state vid_option_states_actual[VID_OPT_TOTAL];
static video_option_state vid_option_states_tmp[VID_OPT_TOTAL];
static video_option_state *vid_option_state_ptr = &vid_option_states_actual[0];

// This is for SDL2's SDL_GetNumDisplayModes / SDL_GetDisplayMode
static int vid_mode_count = 0;
SDL_DisplayMode *vid_modes = 0;
// static vid_resolution_t *vid_modes = 0;

static video_options vo;
static qboolean vid_entered_menu = false;

typedef enum
{
	MS_WINDOWED,
	MS_FULLSCREEN,
	MS_DESKTOPFULLSCREEN,

	MS_MODE_TOTAL

} modestate_t;

static const char *vid_mode_names[MS_MODE_TOTAL] =
{
	"Windowed",
	"Fullscreen",
	"Borderless",
};


// Forward declarations
static int GetStateIndex(video_option_type type, int index);
static int GetStateIndexDefault(video_option_type type);

void UI_SetTextStyle(qboolean selectable)
{
	vo.text_style = selectable;
}

static void DefineVideoOptions(void)
{
	int row = 0;
#define Newline() row++
#define SetOption(num, _text, _selectable) \
	vid_options[num].row = row; \
	vid_options[num].text = _text; \
	vid_options[num].selectable = _selectable; \
	row++

	// Note: Order matters here!
	// The SetOption macros must be in the same order as the enum that uses it

	SetOption(VID_OPT_VIDMODE, 			"Video mode", 			1);
	SetOption(VID_OPT_DEPTH, 			"Depth", 				0);
	SetOption(VID_OPT_REFRESH_RATE, 	"Refresh rate", 		0);
	SetOption(VID_OPT_MODESTATE, 		"Display mode", 		1);
	SetOption(VID_OPT_VSYNC, 			"Vsync", 				1);

	Newline();
	SetOption(VID_OPT_FB_MODE, 			"Framebuffer mode", 	1);

	Newline();
	SetOption(VID_OPT_TEST, 			"Test", 				1);
	SetOption(VID_OPT_APPLY, 			"Apply", 				1);

#undef Newline
#undef SetOption
}

void UI_PrintGenericItem(int col, int row, qboolean selectable, const char *text)
{
	int x = col;
	int y = row * 8 + 32;
	if(selectable)
	{
		M_Print(x, y, (char *)text);
	}
	else
	{
		M_PrintWhite(x, y, (char *)text);
	}
}

// 8 y units between menu items
// Note: uses global text style state
// Set to one for default behaviour
void UI_PrintItem(int col, int row, const char *text)
{
	UI_PrintGenericItem(col, row, vo.text_style, text);
}

void UI_PrintCheckbox(int col, int row, int on)
{
	UI_PrintItem(col, row, on ? "on" : "off");
}

// x macro magic
// This controls the alignment of the video mode text
// This way, we don't have to change the code in multiple places
#define xm(x) #x
#define q_stringify(xx) xm(xx)
#define UI_TEXT_ALIGNMENT 21
#define UI_ALIGNMENT_STRING "%"q_stringify(UI_TEXT_ALIGNMENT)"s"

// #undef xm
// #undef q_stringify

// We could pre-process the menu list to avoid these print calls
void UI_PrintMenuItem(int col, int row, const char *text, ...)
{
	char tmp[256];
	char output[256];
	va_list vl;

	va_start(vl, text);
	q_vsnprintf(tmp, sizeof(tmp), text, vl);
	va_end(vl);

	// Now align it and print
	const char *alignment_string = UI_ALIGNMENT_STRING;
	q_snprintf(output, sizeof(output), alignment_string, tmp);
	UI_PrintItem(col, row, output);
}

void UI_PrintCentered(int row, const char *text)
{
	// Assumes 320 canvas width, 8 units per character
	int len = strlen(text) * 8;
	int middle = 320 / 2;
	int col = middle - (len / 2);

	UI_PrintItem(col, row, text);
}

void VID_MenuDraw(void)
{
	qpic_t *p;

	// 8 y units between menu items
	// Begin at y=32
	// 18 units right aligned
	// Begin descrptions at x=184

	// x=168 for the cursor

	// The decorations
	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/vidmodes.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	// M_Print(13 * 8, 36, "Hello");

	// UI_PrintMenuItem(16, 0, "      First");
	// UI_PrintMenuItem(16, 1, "     Second");
	// UI_PrintMenuItem(16, 2, "             Third");

	// UI_PrintMenuItem(16, 3, "Video mode"); UI_PrintItem(184, 3, "Desc mode");
	// UI_PrintMenuItem(16, 4, "Game mode"); UI_PrintItem(184, 4, "Desc mode");


	{
		int i = 0;
		const int x = 16;
		const int x_desc = (UI_TEXT_ALIGNMENT + 5) * 8;
		const char *info_text = 0;
		int screen_w, screen_h;
		SDL_DisplayMode *disp_mode = 0;
		for(i = 0; i < VID_OPT_TOTAL; i++)
		{
			int row = vid_options[i].row;
			const char *text = vid_options[i].text;

			UI_SetTextStyle(vid_options[i].selectable);
			switch(i)
			{
				case VID_OPT_VIDMODE:
					UI_PrintMenuItem(x, row, text);

					disp_mode = &vid_modes[GetStateIndexDefault(VID_OPT_VIDMODE)];
					if(vo.mode_invalid)
					{
						screen_w = vid_width.value;
						screen_h = vid_height.value;
					}
					else
					{
						screen_w = disp_mode->w;
						screen_h = disp_mode->h;
					}
					UI_PrintItem(x_desc, row, va("%dx%d", screen_w, screen_h));
					break;
				case VID_OPT_DEPTH:
					UI_PrintMenuItem(x, row, text);

					disp_mode = &vid_modes[GetStateIndexDefault(VID_OPT_VIDMODE)];
					UI_PrintItem(x_desc, row, va("%d", VID_SDL2GetBPP(disp_mode)));
					break;
				case VID_OPT_REFRESH_RATE:
					UI_PrintMenuItem(x, row, text);

					disp_mode = &vid_modes[GetStateIndexDefault(VID_OPT_VIDMODE)];
					UI_PrintItem(x_desc, row, va("%d", disp_mode->refresh_rate));
					break;
				case VID_OPT_FB_MODE:
				{
					// Note: fb_index is 1 based, but the underlying array is not
					int fb_index = GetStateIndexDefault(VID_OPT_FB_MODE) - 1;
					UI_PrintMenuItem(x, row, text);
					UI_PrintItem(x_desc, row, va("%dx%d\n", fb_modes[fb_index].w, fb_modes[fb_index].h));
				}
					break;
				case VID_OPT_MODESTATE:
				{
					int screen_mode_index = GetStateIndexDefault(VID_OPT_MODESTATE);
					UI_PrintMenuItem(x, row, text);
					UI_PrintItem(x_desc, row, vid_mode_names[screen_mode_index]);
				}
					break;
				case VID_OPT_VSYNC:
					UI_PrintMenuItem(x, row, text);
					UI_PrintCheckbox(x_desc, row, GetStateIndexDefault(VID_OPT_VSYNC));
					break;

				case VID_OPT_TEST:
					UI_PrintMenuItem(x, row, text);
				case VID_OPT_APPLY:
					UI_PrintMenuItem(x, row, text);
					break;
			}
		}

		video_option_type cur_option = vo.cursor;
		switch(cur_option)
		{
			case VID_OPT_VIDMODE:
			{
				int w, h;
				VID_GetClientWindowDimensions(&w, &h);
				info_text = va("Current window size: %dx%d", w, h);
			}
				break;
			case VID_OPT_FB_MODE:
				info_text = "This is the in-game resolution";
				break;
			case VID_OPT_MODESTATE:
				if(GetStateIndexDefault(cur_option) == MS_FULLSCREEN)
				{
					info_text = "Note: Changes your desktop resolution";
				}
				break;
			case VID_OPT_TEST:
				info_text = "Test this configuration for " q_stringify(VID_TEST_TIMEOUT) " seconds";
				break;
		}

		if(info_text)
		{
			// Note: If we ever exceed this row for the menu items, the code will have to change
			UI_SetTextStyle(0);
			UI_PrintCentered(16, info_text);
		}
	}


	// Cursor
	// (Text alignment + 3) * 8
	int cur_row = vid_options[vo.cursor].row;
	M_DrawCharacter ((UI_TEXT_ALIGNMENT + 3) * 8, cur_row * 8 + 32, 12+((int)(realtime*4)&1));
}



static void SetStatePointer(video_option_state_pointer_type type)
{
	switch(type)
	{
		case STATE_PTR_TEMP:
			vid_option_state_ptr = vid_option_states_tmp;
			break;
		case STATE_PTR_ACTUAL:
			vid_option_state_ptr = vid_option_states_actual;
			break;
	}
}

static void CopyStateTmpToStateActual(void)
{
	memcpy(vid_option_states_actual, vid_option_state_ptr, sizeof(vid_option_states_actual));
}



static void VID_ApplyOptions(void)
{
	// No fancy loops for now, just set everything manually
	// I'll opt to use the cvar names

	int fb_mode_index = GetStateIndexDefault(VID_OPT_FB_MODE);
	int vsync_mode = GetStateIndexDefault(VID_OPT_VSYNC);
	int screen_mode = GetStateIndexDefault(VID_OPT_MODESTATE);


	// Todo: Clean this up
	int vid_mode_index = GetStateIndexDefault(VID_OPT_VIDMODE);
	SDL_DisplayMode *desired_disp_mode = &vid_modes[vid_mode_index];
	SDL_DisplayMode *cur_disp_mode = 0;

	Cbuf_AddText(va("vid_width %d\n", desired_disp_mode->w));
	Cbuf_AddText(va("vid_height %d\n", desired_disp_mode->h));

	switch(screen_mode)
	{
		case MS_WINDOWED:
			Cbuf_AddText("vid_fullscreen 0\n");
			break;
		case MS_FULLSCREEN:
			Cbuf_AddText("vid_fullscreen 1\n");
			Cbuf_AddText("vid_desktopfullscreen 0\n");
			break;
		case MS_DESKTOPFULLSCREEN:
			Cbuf_AddText("vid_fullscreen 1\n");
			Cbuf_AddText("vid_desktopfullscreen 1\n");
			break;
	}


	Cbuf_AddText(va("vid_vsync %d\n", vsync_mode));

	// Always set vid_refreshrate to match the selected video mode option
	// This is just to avoid headaches and dependency issues when calling vid_restart
	// The refresh rate is only used if the new window type has the SDL_FULLSCREEN flag
	// It should be overwritten at the end of this function with the result of VID_GetCurrentDisplayMode
	// to match the real display mode
	Cbuf_AddText(va("vid_refreshrate %d\n", desired_disp_mode->refresh_rate));
	Cbuf_AddText(va("vid_bpp %d\n", VID_SDL2GetBPP(desired_disp_mode)));

	// Fb mode
	Cbuf_AddText(va("fb_mode %d\n", fb_mode_index));

	// Should be last
	Cbuf_AddText("vid_restart\n");


	// Process the command buffer before re-checking the video mode
	Cbuf_Execute();

	// Update refresh rate and bpp after vid_restart, because the monitor might not match the game settings
	cur_disp_mode = VID_GetCurrentDisplayMode();

	Cvar_SetValueQuick(&vid_refreshrate, cur_disp_mode->refresh_rate);
	Cvar_SetValueQuick(&vid_bpp, VID_SDL2GetBPP(cur_disp_mode));
}


// Note: Includes maxval
int WrapInt(int val, int minval, int maxval)
{
	if(val > maxval)
	{
		val = minval;
	}
	else if(val < minval)
	{
		val = maxval;
	}
	return val;
}

static void VID_SelectOption(int dir)
{
	int cursor = vo.cursor;
	qboolean changed = false;
	const size_t vid_option_size = sizeof(vid_option_states_actual);
	video_option_state *opt = &vid_option_state_ptr[cursor];

	switch(cursor)
	{
		case VID_OPT_FB_MODE:
			opt->selection_index[0] = WrapInt(opt->selection_index[0] + dir, 1, 3);
			break;

		case VID_OPT_VIDMODE:
			// SDL2 sorts video modes from largest to smallest, let's reverse the direction
			// so it's more intutitve for the user
			opt->selection_index[0] = WrapInt(opt->selection_index[0] + (-dir), 0, vid_mode_count - 1);
			vo.mode_invalid = false;
			break;

		case VID_OPT_VSYNC:
			opt->selection_index[0] = WrapInt(opt->selection_index[0] + dir, 0, 1);
			break;

		case VID_OPT_MODESTATE:
			opt->selection_index[0] = WrapInt(opt->selection_index[0] + dir, 0, MS_MODE_TOTAL - 1);
			break;

		case VID_OPT_APPLY:
			changed = Q_memcmp(vid_option_states_actual, vid_option_states_tmp, vid_option_size);
			CopyStateTmpToStateActual();

			SetStatePointer(STATE_PTR_ACTUAL);

			// If the player uses "Test" and hits "Apply" within the timeout period,
			// there's no need to apply the options again
			if(changed && !vid_config_timer_enabled)
			{
				VID_ApplyOptions();
			}

			SetStatePointer(STATE_PTR_TEMP);

			vid_config_timer_begin = 0;
			vid_config_timer_enabled = false;

			break;

		case VID_OPT_TEST:
			changed = Q_memcmp(vid_option_states_actual, vid_option_states_tmp, vid_option_size);
			SetStatePointer(STATE_PTR_TEMP);
			if(changed)
			{
				VID_ApplyOptions();
				vid_config_timer_begin = realtime;
				vid_config_timer_enabled = true;
			}
			break;
	}
}

static void SetStateIndex(video_option_type type, int index, int value)
{
	vid_option_state_ptr[type].selection_index[index] = value;
}

static void SetStateIndexDefault(video_option_type type, int value)
{
	SetStateIndex(type, 0, value);
}

static int GetStateIndex(video_option_type type, int index)
{
	return vid_option_state_ptr[type].selection_index[index];
}

static int GetStateIndexDefault(video_option_type type)
{
	return GetStateIndex(type, 0);
}


static void VID_SyncVideoOptions(void)
{
	// Safe default
	modestate_t screen_mode = MS_WINDOWED;
	vo.mode_invalid = false;

	// Todo: We should probably rely on SDL2 for the window settings,
	// rather than our cached variables
	SetStateIndexDefault(VID_OPT_FB_MODE, (int)fb_mode.value);
	SetStateIndexDefault(VID_OPT_VSYNC, (int)vid_vsync.value);


	if(vid_fullscreen.value)
	{
		if(vid_desktopfullscreen.value)
		{
			screen_mode = MS_DESKTOPFULLSCREEN;
		}
		else
		{
			screen_mode = MS_FULLSCREEN;
		}
	}
	else
	{
		screen_mode = MS_WINDOWED;
	}
	SetStateIndexDefault(VID_OPT_MODESTATE, (int)screen_mode);

	Cvar_SetValueQuick(&vid_width, vid_client_width);
	Cvar_SetValueQuick(&vid_height, vid_client_height);

	// Video modes
	{
		int i = 0;
		int reported_mode_count = 0;
		int valid_mode_count = 0;
		int current_mode_index = -1;
		// Note: 0 is the default monitor
		reported_mode_count = SDL_GetNumDisplayModes(0);
		SAFE_FREE(vid_modes);
		vid_modes = malloc(reported_mode_count * sizeof *vid_modes);


		for(i = 0; i < reported_mode_count; i++)
		{
			SDL_DisplayMode mode;
			if(SDL_GetDisplayMode(0, i, &mode) == 0)
			{
				vid_modes[valid_mode_count] = mode;
				int bpp = VID_SDL2GetBPP(&mode);

				if(mode.w == vid_width.value
				&& mode.h == vid_height.value
				&& mode.refresh_rate == vid_refreshrate.value
				&& bpp == vid_bpp.value)
				{
					current_mode_index = i;
				}
				valid_mode_count++;
			}
		}

		if(current_mode_index == -1)
		{
			Con_Printf("Video mode is invalid");
			Con_Printf("Defaulting to the native resolution of the monitor");
			current_mode_index = 0;
			vo.mode_invalid = true;
		}

		// Todo: Handle the case where video modes found are different to the reported video modes
		q_assert(valid_mode_count == reported_mode_count);
		vid_mode_count = valid_mode_count;
		SetStateIndexDefault(VID_OPT_VIDMODE, current_mode_index);
	}
}

void VID_MenuInit(void)
{
	vid_entered_menu = true;
	SetStatePointer(STATE_PTR_TEMP);
	VID_SyncVideoOptions();
	CopyStateTmpToStateActual();
}

void VID_MenuKey(int key)
{
	// Sounds:
	// menu1: Up/down arrow
	// menu2: Enter/leave sound
	// menu3: Cycle choices within a single option

	int nextopt = 0;

	switch(key)
	{
		case K_ESCAPE:
			vid_entered_menu = false;
			M_Menu_Options_f ();
			break;

		case K_DOWNARROW:
			vo.cursor += 1;
			vo.cursor %= VID_OPT_TOTAL;
			S_LocalSound("misc/menu1.wav");
			break;

		case K_UPARROW:
			vo.cursor -= 1;
			if(vo.cursor < 0) vo.cursor = VID_OPT_TOTAL - 1;
			S_LocalSound("misc/menu1.wav");
			break;

		case K_LEFTARROW:
		case K_RIGHTARROW:
			nextopt = key == K_RIGHTARROW ? 1 : -1;
			S_LocalSound("misc/menu3.wav");
			VID_SelectOption(nextopt);
			break;

		// Advance to the next choice in the option if possible
		case K_ENTER:
			nextopt = 1;
			// if(vid_options[vo.cursor].selectable)
			{
				S_LocalSound("misc/menu2.wav");
				VID_SelectOption(nextopt);
			}
			break;
	}
}
