# Mingw config for Win32 cross-compilation
# Tested on Linux and MSYS2

# With mingw, -ffast-math seems to be OK
# Sean Barret mentioned this in his stb_image.h file, not sure if this is still relevant in 2023
# It's something to do with mingw Windows cross-compilation and sse2 being enabled
# https://github.com/nothings/stb/issues/81
CFLAGS = -fcommon -m32 -mconsole -O2 -march=native
# CFLAGS += -mfpmath=sse -mstackrealign

LIB_BASE = thirdparty/lib/win32/x86
LIBDIR = -L$(LIB_BASE)
SDL_DEPS = $(LIB_BASE)/SDL2.dll

libSDL2_mixer = SDL2_mixer.dll

LIBS += -lwinmm -l:SDL2.lib -l:SDL2main.lib -l:SDL2.dll
GL_LIBS = -lopengl32

# Sys
SYS_OBJS += sys_sdl_win.o pl_win.o

DEFINES += -DSOFTQUAKE_COMPAT # Must be defined for win32

# Where the objects are compiled
GL_OBJ_DIR := build/win32/glquake
SW_OBJ_DIR := build/win32/softquake

EXE_SUFFIX = .exe

CC ?= i686-w64-mingw32-gcc
