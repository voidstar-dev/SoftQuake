#!/bin/sh
# Developer use
# Just for quick iteration/developing

meson compile -C lbuild || exit


if [ "$1" = "run" ]; then
	cd lbuild
	./glquake-sdl -basedir ../
	# ./softquake-sdl -basedir ../
	exit
fi

