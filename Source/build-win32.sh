#!/bin/sh

# Meson build (win32-cross-compile)

build_dir=wbuild
compiler=i686-w64-mingw32-gcc
export CC=$compiler 
meson $build_dir || exit
meson install -C $build_dir
