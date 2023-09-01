#!/bin/sh

# Compiles a 32 bit shared library version of SDL2_mixer
# gcc used for linux
# mingw used for cross-compilation to a win32 dll

INCLUDE="-I../include -I ../../include/SDL2 -Icodecs -I."
CFLAGS="-O2 -m32"
SRC_COMMON="*.c codecs/*.c"
DEFS="-DMUSIC_MP3_DRMP3 -DMUSIC_OGG -DOGG_USE_STB -DMUSIC_WAV -DMUSIC_FLAC_DRFLAC"
FLAGS_COMMON="$INCLUDE $CFLAGS $DEFS"

build_common()
{
	echo "Compling $BIN"
	$CC $FLAGS_COMMON $LIBS $SRC_COMMON $LDFLAGS  -o $BIN  || exit
	echo "Built shared library: $BIN"
}

# Build everything in one step because I was having trouble linking using i686-w64-mingw32-ld
# Some CRT linking errors ensued(undefined reference to...)
# Previously I was using dlltool and dllwrap, but I want to keep the toolchain the same across OS's
build_win32()
{

	# -static-libgcc and -shared have to be specified
	# --no-insert-timestamp assures that builds are identical when built from the same source
	CC="i686-w64-mingw32-gcc"
	BIN_BASE="SDL2_mixer"
	BIN="$BIN_BASE".dll
	IMPLIB="$BIN_BASE".lib
	LIBS="-l:SDL2.dll"
	LDFLAGS="-L../../lib/win32/x86 -shared -static-libgcc -Wl,--out-implib,$IMPLIB -Wl,--no-insert-timestamp"

	build_common

	# Removes timestamp information from the .lib file
	# Note: Only strip debug information so that ld can still use it for linking (meson requires this)
	strip -D --strip-debug  "$IMPLIB"

	chmod 644 "$BIN"
}


build_linux()
{
	CC="gcc"
	BIN="libSDL2_mixer-2.0.so.0"
	LIBS="-lSDL2"
	LDFLAGS="-shared -fPIC"

	build_common
}

build_help()
{
	echo "Usage: $0 <option>"
	echo "Build SDL2_mixer as a shared library"
	echo "Options are:"
	echo "linux"
	echo "    Build a linux shared object file"
	echo ""

	echo "win32"
	echo "    Build a win32 dll"
	echo ""

	echo "help"
	echo "    Show this help"
	exit
}

main()
{
	case "$1" in
		"linux")
		build_linux
		;;

		"win32")
		build_win32
		;;

		"help")
		build_help
		;;

		*)
		build_help
		;;
	esac
}

main $1
