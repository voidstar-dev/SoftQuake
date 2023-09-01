# This file compiles the final target binary
# Specify either TARGET_SOFTQUAKE=1, or TARGET_GLQUAKE=1, but not both

# The main Makefile should take care of this

include 1_srcs.mk

ifeq ($(WIN32),1)
	include 3_config_win32_cross.mk
else
	include 3_config_linux.mk
endif

include 4_options.mk


INCLUDE += -Ithirdparty/include

GL_DEFINES = -DGLQUAKE

COMMON_OBJS = 
COMMON_OBJS += $(SHARED_OBJS) 
COMMON_OBJS += $(INPUT_OBJS) 
COMMON_OBJS += $(SND_OBJS)
COMMON_OBJS += $(MAIN_OBJS)
COMMON_OBJS += $(SYS_OBJS)

GL_OBJS =
GL_OBJS += $(R_GL_OBJS) 
GL_OBJS += gl_vidsdl.o
GL_OBJS += $(COMMON_OBJS)

SW_OBJS =
SW_OBJS += $(R_SW_OBJS) 
SW_OBJS += vid_sdl2.o
SW_OBJS += $(COMMON_OBJS)

# Since this file is should be recursively called, targets are defaulted to 0
TARGET_SOFTQUAKE ?= 0
TARGET_GLQUAKE ?= 0

# Convoluted way to nest if-statements
# Esentially, you can only compile a single target from this file
# It's an error to omit a target

ifeq ($(TARGET_GLQUAKE),0)
ifeq ($(TARGET_SOFTQUAKE),0) 
$(error No target specified)
endif
endif

ifeq ($(TARGET_GLQUAKE),1)
ifeq ($(TARGET_SOFTQUAKE),1) 
$(error Only a single target can be specified)
endif
endif


# Final build steps
# If we somehow messed up, use the current directory
# Otherwise, the clean target will be using the root
# Very dangerous...
OBJ_DIR ?= .
ifeq ($(TARGET_GLQUAKE),1)
	OBJ_DIR := $(GL_OBJ_DIR)
	OBJS := $(addprefix $(OBJ_DIR)/, $(GL_OBJS))
	DEFINES += $(GL_DEFINES)
	LIBS += $(GL_LIBS)
	TARGET=$(BIN_DIR)/glquake-sdl$(EXE_SUFFIX)
endif

ifeq ($(TARGET_SOFTQUAKE),1)
	OBJ_DIR := $(SW_OBJ_DIR)
	OBJS := $(addprefix $(OBJ_DIR)/, $(SW_OBJS))
	TARGET=$(BIN_DIR)/softquake-sdl$(EXE_SUFFIX)
endif


LDFLAGS ?=
BIN_DIR=bin

# Should dependencies be copied optionally?
# This line does just that
#cp $(SDL_DEPS) $(BIN_DIR)/ || echo "Could not copy dependencies. Continuing with build" && exit 0

$(TARGET): $(OBJS) $(BIN_DIR)
	$(CC) -o $(TARGET) $(OBJS) $(CFLAGS) $(LDFLAGS) $(LIBDIR) $(LIBS)
	cp $(SDL_DEPS) $(BIN_DIR)/
	@echo "Built target: $(TARGET)"


# Thanks doomgeneric, I had no idea about this syntax
# They are called "Order Only Prerequisites" by GNU Make
$(OBJS): | $(OBJ_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR)/%.o: %.c
	$(CC) -c $< -o $@ $(DEFINES) $(INCLUDE) $(CFLAGS)


.PHONY: clean1

clean1:
	rm $(OBJ_DIR)/*.o
