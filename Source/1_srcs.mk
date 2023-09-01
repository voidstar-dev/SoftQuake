# Quake sources
# The object files have been broken up into their respective build targets

SND_OBJS = 

R_GL_OBJS =
# R_GL_OBJS += gl_common.o # Not needed
R_GL_OBJS += gl_draw.o
R_GL_OBJS += gl_mesh.o
R_GL_OBJS += gl_model.o
R_GL_OBJS += gl_refrag.o
R_GL_OBJS += gl_rlight.o
R_GL_OBJS += gl_rmain.o
R_GL_OBJS += gl_rmisc.o
R_GL_OBJS += gl_rsurf.o
R_GL_OBJS += gl_screen.o
R_GL_OBJS += gl_warp.o

# New
R_GL_OBJS += gl_texmgr.o


R_SW_OBJS =
R_SW_OBJS = d_edge.o \
	   d_fill.o \
	   d_init.o \
	   d_modech.o \
	   d_part.o \
	   d_polyse.o \
	   d_scan.o \
	   d_sky.o \
	   d_sprite.o \
	   d_surf.o \
	   d_vars.o \
	   d_zpoint.o \
	   \
 	   draw.o \
	   model.o \
	   nonintel.o \
	   r_aclip.o \
	   r_alias.o \
	   r_bsp.o \
	   r_draw.o \
	   r_edge.o \
	   r_efrag.o \
	   r_light.o \
	   r_main.o \
	   r_misc.o \
	   r_sky.o \
	   r_sprite.o \
	   r_surf.o \
	   screen.o \


SHARED_OBJS = chase.o \
	   cl_demo.o \
	   cl_input.o \
	   cl_main.o \
	   cl_parse.o \
	   cl_tent.o \
	   cmd.o \
	   common.o \
	   console.o \
	   crc.o \
	   cvar.o \
	   host.o \
	   host_cmd.o \
	   keys.o \
	   mathlib.o \
	   menu.o \
	   net_loop.o \
	   net_main.o \
	   net_none.o \
	   net_vcr.o \
	   pr_cmds.o \
	   pr_edict.o \
	   pr_exec.o \
	   r_part.o \
	   r_vars.o \
	   sbar.o \
	   sv_main.o \
	   sv_move.o \
	   sv_phys.o \
	   sv_user.o \
	   view.o \
	   wad.o \
	   world.o \
	   zone.o \



# Input
INPUT_OBJS = in_sdl.o

# Main
# Remove this if testing other sys files
MAIN_OBJS = main_sdl.o

# New additions
SHARED_OBJS += sdl_common.o cvar_common.o softquake_version.o
