#!/bin/sh

# Script for MinGW cross-compilation
# See 3_config_win32_cross.mk for the specifics

# When using MSYS2, CC can also be gcc
# On Linux, it should still be explicitly specified as the mingw compiler

compiler="i686-w64-mingw32-gcc"
exec make WIN32=1 CC="$compiler" $*
