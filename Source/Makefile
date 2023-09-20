# Recursively call the recipe file with the intended target
# Note: One must use the $(MAKE) builtin variable to pass arguments to recursive invokations of make

.PHONY: softquake glquake clean help

# Only run one target at a time
# Currently this is to prevent dependency issues with precompiled headers
# The targets themselves 'can' be parallelized
.NOTPARALLEL: all

all: softquake glquake

softquake:
	$(MAKE) -f 5_recipe.mk TARGET_SOFTQUAKE=1

glquake:
	$(MAKE) -f 5_recipe.mk TARGET_GLQUAKE=1

clean:
	$(MAKE) -f 5_recipe.mk TARGET_SOFTQUAKE=1 clean1; \
	$(MAKE) -f 5_recipe.mk TARGET_GLQUAKE=1 clean1

help:
	@echo "=================================== Help ==================================="
	@echo ""
	@echo "$(MAKE)               -- Builds all targets"
	@echo "$(MAKE) softquake     -- Builds softquake-sdl (Software rendered version)"
	@echo "$(MAKE) glquake       -- Builds glquake-sdl (OpenGL version)"
	@echo "$(MAKE) clean         -- Deletes all object files"
	@echo ""
	@echo "Built files will be located in the 'bin' folder"
	@echo ""
	@echo "============================================================================"
