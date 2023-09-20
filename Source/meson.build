# To use, see build-linux.sh or build-win32.sh for examples

project('softquake', 'c', default_options: ['buildtype=plain', 'warning_level=0'])

gl_src = []
sw_src = []
shared_src = []
snd_src = []
in_src = []
main_src = []
sys_src = []
image_src = []
defines = []
cflags = []
gl_cflags = []
warnings_to_ignore = []
libs = []
libs_to_copy = []
ldflags = []
includes = []
deps = []
gl_deps = []
rpath = ''

is_nix = false
is_windows = false

cc = meson.get_compiler('c')
is_windows = build_machine.system() == 'windows' or cc.get_define('_WIN32') != ''
if is_windows != true
  is_nix = build_machine.system() == 'linux' or build_machine.system() == 'darwin'
endif


# Declare local libraries upfront
if is_windows
  lib_prefix = 'thirdparty/lib/win32/x86'
  libSDL2_mixer = ['-l:SDL2_mixer.dll', '-l:SDL2_mixer.lib']
elif is_nix
  lib_prefix = 'thirdparty/lib/linux/x86'
  libSDL2_mixer = ['-l:libSDL2_mixer-2.0.so.0']
else
  assert(false, 'Unsupported OS')
endif

libdir = '..'/lib_prefix
ldflags += '-L' + libdir
rpath += libdir

# Where we install
# Global installs are not desired, just keep it local
bin_dir = meson.current_source_dir()/'bin'

r_gl_src = [
  'gl_draw.c',
  'gl_mesh.c',
  'gl_model.c',
  'gl_refrag.c',
  'gl_rlight.c',
  'gl_rmain.c',
  'gl_rmisc.c',
  'gl_rsurf.c',
  'gl_screen.c',
  'gl_warp.c',
]

r_sw_src = [
  'd_edge.c',
  'd_fill.c',
  'd_init.c',
  'd_modech.c',
  'd_part.c',
  'd_polyse.c',
  'd_scan.c',
  'd_sky.c',
  'd_sprite.c',
  'd_surf.c',
  'd_vars.c',
  'd_zpoint.c',

  'draw.c',
  'model.c',
  'nonintel.c',
  'r_aclip.c',
  'r_alias.c',
  'r_bsp.c',
  'r_draw.c',
  'r_edge.c',
  'r_efrag.c',
  'r_light.c',
  'r_main.c',
  'r_misc.c',
  'r_sky.c',
  'r_sprite.c',
  'r_surf.c',
  'screen.c',
]

shared_src = [
  'chase.c',
  'cl_demo.c',
  'cl_input.c',
  'cl_main.c',
  'cl_parse.c',
  'cl_tent.c',
  'cmd.c',
  'common.c',
  'console.c',
  'crc.c',
  'cvar.c',
  'host.c',
  'host_cmd.c',
  'keys.c',
  'mathlib.c',
  'menu.c',
  'net_loop.c',
  'net_main.c',
  'net_none.c',
  'net_vcr.c',
  'pr_cmds.c',
  'pr_edict.c',
  'pr_exec.c',
  'r_part.c',
  'r_vars.c',
  'sbar.c',
  'sv_main.c',
  'sv_move.c',
  'sv_phys.c',
  'sv_user.c',
  'view.c',
  'wad.c',
  'world.c',
  'zone.c',
]

# -------------- Build options --------------
# I tried sticking them into a meson_options.txt file to do things the 'right way',
# but the current version does not check for changes to this file, meaning options are only ever set on the first run.
# I guess the intended use was to provide a command-line interface for options, rather than editing the text directly.

# Various GLQuake fixes
# Please read softquake-notes.txt to learn about what they do.
# If you do enable these fixes, I recommend keeping them even in the software build.
# They'll effectively be no-ops (in the software build), but the important thing is that
# the cvars set by these fixes will be preserved no matter which executable you run.
ENABLE_GL_FULLBRIGHT_FIX	= 0
ENABLE_GL_LIGHTMAP_FIX		= 0
ENABLE_GL_TEXTUREMODE_FIX	= 0
ENABLE_GL_CLEARCOLOR_FIX	= 0
ENABLE_GL_UI_SCALING_FIX	= 0

# Audio
ENABLE_SOUND	  = 1	# Sound effects
ENABLE_CD_AUDIO	  = 1	# Background music

# Image
# Use the PNG file format for screenshots
# Otherwise uses PCX for softquake, and TGA for glquake
ENABLE_PNG		  = 0



if ENABLE_GL_FULLBRIGHT_FIX == 1
  defines += '-DSOFTQUAKE_GL_FULLBRIGHT_FIX'
endif

if ENABLE_GL_LIGHTMAP_FIX == 1
  defines += '-DSOFTQUAKE_GL_LIGHTMAP_FIX'
endif

if ENABLE_GL_TEXTUREMODE_FIX == 1
  defines += '-DSOFTQUAKE_GL_TEXTUREMODE_FIX'
endif

if ENABLE_GL_CLEARCOLOR_FIX == 1
  defines += '-DSOFTQUAKE_GL_CLEARCOLOR_FIX'
endif

if ENABLE_GL_UI_SCALING_FIX == 1
  defines += '-DSOFTQUAKE_GL_UI_SCALING_FIX'
endif


if ENABLE_SOUND == 1
  snd_src += 'snd_sdl.c'
  snd_src += 'snd_dma_sdl.c'
  snd_src += 'snd_mix.c'
  snd_src += 'snd_mem.c'
else
  snd_src += 'snd_null.c'
endif


if ENABLE_CD_AUDIO == 1
  snd_src += 'cd_sdl.c'
  libs += libSDL2_mixer
else
  snd_src += 'cd_null.c'
endif

if ENABLE_PNG == 1
  defines += '-DSOFTQUAKE_ENABLE_PNG'
  image_src += 'stb_image_write.c'
endif

image_src += 'scr_screenshot.c'

# Render target specifics
sw_src += 'vid_sdl2.c'
sw_src += r_sw_src

gl_src += 'gl_vidsdl.c'
gl_src += 'gl_texmgr.c'
gl_src += r_gl_src


shared_src += 'cvar_common.c'
shared_src += 'sdl_common.c'
shared_src += 'softquake_version.c'
in_src += 'in_sdl.c'
main_src += 'main_sdl.c'

# OS SPECIFIC STUFF
# Instead checking every step, just do one OS at a time

if is_windows
  sys_src += ['sys_sdl_win.c', 'pl_win.c']

  libSDL2 = ['-l:SDL2.dll', '-l:SDL2.lib', '-l:SDL2main.lib']
  deps += cc.find_library('winmm') 
  gl_deps += cc.find_library('opengl32')
  includes += 'thirdparty/include'
  defines += '-DSOFTQUAKE_COMPAT'
  libs += libSDL2

  # If cross compiling on Linux, meson doesn't follow the .exe suffix rule 
  # so we have to specify it explicitly
  exe_suffix = 'exe'

  libs_to_copy = [lib_prefix/'SDL2.dll']
  if ENABLE_CD_AUDIO == 1
	libs_to_copy += [lib_prefix/'SDL2_mixer.dll']
  endif

elif is_nix
  sys_src += ['sys_sdl_unix.c']

  libSDL2 = cc.find_library('SDL2')
  deps += libSDL2
  deps += cc.find_library('m', required: false)
  gl_deps += cc.find_library('GL')

  exe_suffix = []
  
  if ENABLE_CD_AUDIO == 1
	libs_to_copy = [lib_prefix/'libSDL2_mixer-2.0.so.0']
  endif

else
  assert(false, 'Unsupported OS')
endif

warnings_to_ignore += '-Wno-unknown-pragmas'

# Arch
# Warning: Do NOT use -ffast math in the software renderer. It breaks some things in the code
# To reproduce, load E1M4 and go to the door just before the bridge near the exit
defines += '-Did386=0'
cflags += defines
cflags += '-fcommon'
cflags += '-m32' # 32 bit compile
# cflags += '-march=native'
# cflags += '-mfpmath=sse' # Use sse instructions instead of the x87 FPU
# cflags += '-mstackrealign'

cflags += '-O2'
# cflags += '-g3'
cflags += warnings_to_ignore

gl_cflags += '-ffast-math'
gl_cflags += '-DGLQUAKE'

ldflags += ['-m32'] # 32 bit compile

common_src = shared_src + main_src + in_src + sys_src + snd_src + image_src
gl_src += common_src
sw_src += common_src

local_libs = files(libs_to_copy)
install_data(local_libs, install_dir: bin_dir)

if true
executable('softquake-sdl',
  sw_src,
  c_args: [cflags],

  link_args: [ldflags, libs],
  build_rpath: rpath,
  include_directories: includes,
  install_dir: bin_dir,
  install: true,
  install_rpath: '.',
  name_suffix: exe_suffix,
  dependencies: deps)
endif

if true
executable('glquake-sdl',
  gl_src,
  c_args: [cflags, gl_cflags],

  link_args: [ldflags, libs],
  build_rpath: rpath,
  include_directories: includes,
  install_dir: bin_dir,
  install: true,
  install_rpath: '.',
  name_suffix: exe_suffix,
  dependencies: [deps, gl_deps])
endif
