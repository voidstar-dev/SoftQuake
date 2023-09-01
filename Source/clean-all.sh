#!/bin/sh

# Developer use
# Deletes all build files
# Does not remove the build directories themselves

make clean
./build-cross-win32.sh clean
meson compile -C lbuild --clean
meson compile -C wbuild --clean
rm bin/*
