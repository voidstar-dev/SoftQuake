# SoftQuake

SoftQuake is a project that aims to provide a cross-platform port of the software rendered version of Quake.
It's directly based on the 1999 release of the Quake source code by id Software.

Primary goals:
* Match the original DOS visuals and feel as closely as possible.
* Minimal quality of life changes.
* No internal engine changes or limit removing.
* Cross-platform code.

GLQuake is also ported, mostly as a historical curiosity.
At this point (in 2023) there are a vast number of GLQuake based ports that look a lot nicer, and bring more features to the table.
It's not the primary focus of this project, but it's there if you want it.

For more info, please read [softquake-notes.txt](Source/softquake-notes.txt)

## Quick Start
There are pre-compiled binaries available in the [Release](https://github.com/voidstar-dev/SoftQuake/releases) section.

Just pick the OS that suits you, and download.

Linux users will need a `32 bit` version of SDL2 installed on their machines.

**The following sections are for those who want to build this code from source.**
## Dependencies
### Linux
* gcc
* make
* SDL2 (32 bit)
* SDL2_mixer (32 bit, optional).
* OpenGL 2
* Meson (optional)


### Windows (cross-compiled)
* MSYS2
* i686-w64-mingw32-gcc
* make
* OpenGL 2 (provided by your video card driver)
* SDL2 (32 bit, either use the included [dll](Source/thirdparty/lib/win32/x86) or provide one yourself)


## Setting up the Windows build
1. Install [MSYS2](https://www.msys2.org/)
2. Launch `MSYS2 MinGW 32-bit`
3. [Update](https://www.msys2.org/docs/updating/) MSYS2. A simple `pacman -Syy` followed by `pacman -Suy` might be sufficient
3. Type in the following command:

```
pacman -S make i686-w64-mingw32-gcc
```

4. Follow [these instructions](#windows)

*All instructions for the Windows build can be followed on a Linux system,
provided you have i686-w64-mingw32-gcc installed (you won't need MSYS2)*


## Build Instructions (Make)
### Linux
1. Navigate to `Source`
2. Run `make`
3. The final output will be in the `bin` directory


### Windows
1. Navigate to `Source`
2. Run `./build-cross-win32.sh` (this script accepts command line arguments just like `make`)
3. The final output will be in the `bin` directory



## Build Instructions (Meson)
### Linux
1. Navigate to `Source`
2. Run `./build-linux.sh`
3. The final output will be in the `bin` directory

### Windows
1. Todo


## Building the thirdparty dependencies
**This step has not been thoroughly tested yet, your results may vary**

### Linux
0. You may use your own system-wide copy of SDL2_mixer if you prefer, and skip this step
1. Navigate to `Source/thirdparty/SDL2_mixer/src`
2. Run `./build.sh linux`
3. `libSDL2_mixer-2.0.so.0` will be built
4. Copy it to `Source/thirdparty/lib/linux/x86`

### Windows
1. Navigate to `Source/thirdparty/SDL2_mixer/src`
2. Run `./build.sh win32`
3. `SDL2_mixer.dll` and `SDL2_mixer.lib` will be built
4. Copy both files to `Source/thirdparty/lib/win32/x86`

## Build configuration
There are a number of compile-time options available:

* `ENABLE_CD_AUDIO`
* `ENABLE_SOUND`
* `ENABLE_GL_LIGHTMAP_FIX`
* `ENABLE_GL_FULLBRIGHT_FIX`
* `ENABLE_GL_TEXTUREMODE_FIX`
* `ENABLE_GL_CLEARCOLOR_FIX`
* `ENABLE_GL_UI_SCALING_FIX`

Please read [softquake-notes.txt](Source/softquake-notes.txt) for a detailed overview of what they do.

By default, all options are set to use the same parameters as the original source code release.

* [4_options.mk](Source/4_options.mk)
* [meson.build](Source/meson.build)


## Missing features
These are *actual* missing features and not just *nice-to-have* things.
This is by no means an exhaustive list, feel free to file an issue.
* Networking
* Dedicated server

## Bugs
Todo: Formalise bug reports (use Github for now)

There are probably bugs.

https://github.com/voidstar-dev/SoftQuake/issues

## Other platforms
I can only program for and test on platforms and operating systems that I actually have access to.

Contributors are welcome!

## Thanks
* id Software
* QuakeSpasm developers
* SDL2 developers
* Darkplaces developers (this readme format is larely based on their page)
* Meson developers
* Everyone else that I have missed for keeping this game alive
