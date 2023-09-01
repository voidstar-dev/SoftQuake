#!/bin/sh

# Simple packaging script
# Assumes the Quake binaries and libs are already built, and are in the bin folder

linux_dir="../Release/Linux"

win32_dir="../Release/Windows"


make_directories()
{
	mkdir -p "$linux_dir"
	mkdir -p "$win32_dir"
}

fail()
{
	mv softquake bin
	exit
}

make_vertool()
{
	make -f Makefile.vertool || fail
}

zip_files()
{
	# This is a dumb hack to create an archive with the desired folder name
	# without creating a new folder to hold the existing files
	mv bin softquake || fail

	cp -v instructions.txt softquake

	version=$(./vertool)
	softquake_base_ver="softquake-$version"

	# a: Auto compress (use extension to determine method)
	# c: Create
	# v: Verbose
	# f: File (must be last)
	linux_archive="$softquake_base_ver-linux.tar.gz"
	tar -acvvf "$linux_archive" \
		softquake/softquake-sdl \
		softquake/glquake-sdl \
		softquake/libSDL2_mixer-2.0.so.0 \
		softquake/instructions.txt \
		|| fail

	win32_archive="$softquake_base_ver-win32.zip"
	zip "$win32_archive" \
		softquake/softquake-sdl.exe \
		softquake/glquake-sdl.exe \
		softquake/SDL2.dll \
		softquake/SDL2_mixer.dll \
		softquake/instructions.txt \
		|| fail

	mv softquake bin || fail
}

move_files()
{
	mv -v "$linux_archive" "$linux_dir"
	mv -v "$win32_archive" "$win32_dir"
}

make_vertool
make_directories
zip_files
move_files
