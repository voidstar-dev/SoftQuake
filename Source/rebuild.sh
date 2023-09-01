#!/bin/sh

# Developer use
# Rebuilds the entire source tree
# Note: Build libs before the main source, because the libraries are copied into the bin directory

build_libs()
{
	lib_base="../../lib"

	pushd thirdparty/SDL2_mixer/src
	
	./build.sh linux
	./build.sh win32

	linux_dir="$lib_base/linux/x86"
	win32_dir="$lib_base/win32/x86"

	cp -v libSDL2_mixer-2.0.so.0 "$linux_dir"
	cp -v SDL2_mixer.dll SDL2_mixer.lib "$win32_dir"

	popd
}

build_src()
{
	procs=-j$(nproc)

	make clean
	make "$procs"

	./build-cross-win32.sh clean
	./build-cross-win32.sh "$procs"
}

build_libs
build_src


