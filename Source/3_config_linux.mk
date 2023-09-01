# Linux config

CFLAGS =
CFLAGS = -fcommon -m32 -O2 -march=native
# CFLAGS += -mfpmath=sse -mstackrealign

LIB_BASE = thirdparty/lib/linux/x86
LIBDIR = -L$(LIB_BASE)

libSDL2_mixer = libSDL2_mixer-2.0.so.0

LIBS += -lm -lSDL2
GL_LIBS = -lGL

# Sys
SYS_OBJS += sys_sdl_unix.o

# Where the objects are compiled
GL_OBJ_DIR := build/linux/glquake
SW_OBJ_DIR := build/linux/softquake

EXE_SUFFIX =

LDFLAGS = -Wl,-rpath,.

CC = gcc
