# Emulated CD Audio. Requires SDL2_mixer
ENABLE_CD_AUDIO := 1

# Sound effects
ENABLE_SOUND := 1

# PNG screenshots
# By default, softquake uses PCX, glquake uses TGA
ENABLE_PNG := 0

# -----------------------------------------
# GLQuake fixes
# Disable all of these to match the original GLQuake release
# Please see softquake-notes.txt for more info
# -----------------------------------------
ENABLE_GL_LIGHTMAP_FIX 		:= 0
ENABLE_GL_FULLBRIGHT_FIX 	:= 0
ENABLE_GL_TEXTUREMODE_FIX 	:= 0
ENABLE_GL_CLEARCOLOR_FIX 	:= 0
ENABLE_GL_UI_SCALING_FIX 	:= 0

ifeq ($(ENABLE_GL_FULLBRIGHT_FIX),1)
	DEFINES += -DSOFTQUAKE_GL_FULLBRIGHT_FIX
endif

ifeq ($(ENABLE_GL_LIGHTMAP_FIX),1)
	DEFINES += -DSOFTQUAKE_GL_LIGHTMAP_FIX
endif

ifeq ($(ENABLE_GL_CLEARCOLOR_FIX),1)
	DEFINES += -DSOFTQUAKE_GL_CLEARCOLOR_FIX
endif

ifeq ($(ENABLE_GL_TEXTUREMODE_FIX),1)
	DEFINES += -DSOFTQUAKE_GL_TEXTUREMODE_FIX
endif

ifeq ($(ENABLE_GL_UI_SCALING_FIX),1)
	DEFINES += -DSOFTQUAKE_GL_UI_SCALING_FIX
endif

# Sound
ifeq ($(ENABLE_SOUND),1)
	SND_OBJS += snd_sdl.o
	SND_OBJS += snd_dma_sdl.o
	SND_OBJS += snd_mix.o
	SND_OBJS += snd_mem.o
else
	SND_OBJS += snd_null.o
endif

# CD Audio
ifeq ($(ENABLE_CD_AUDIO),1)
	SND_OBJS += cd_sdl.o
	LIBS += -l:$(libSDL2_mixer)
	SDL_DEPS += $(LIB_BASE)/$(libSDL2_mixer)
else
	SND_OBJS += cd_null.o
endif

# Screenshots
ifeq ($(ENABLE_PNG),1)
	IMAGE_OBJS += stb_image_write.o
	DEFINES += -DSOFTQUAKE_ENABLE_PNG
endif
