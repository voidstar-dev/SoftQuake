#!/bin/sh

# Meson build (linux)

build_dir=lbuild
compiler=gcc
export CC=$compiler
meson $build_dir || exit
meson install -C $build_dir
