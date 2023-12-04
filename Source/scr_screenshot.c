/*
Copyright (C) 2023-2023 softquake

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// scr_screenshot.c -- General screenshot utilities and PNG writing to replace the default PCX and TGA formats

#include "quakedef.h"

// Check name availability in the gamedir path
// Assumes that the name starts with "quake00"
qboolean SCR_CheckAvailableName(screenshot_filename *out_filename, const char *ext)
{
	char checkname[MAX_OSPATH];
	int i;

	// Basename moved here to avoid code duplication
	// Follows this format:
	// quake00.ext
	// quake01.ext
	// quake02.ext
	// ...
	q_snprintf(out_filename->buf, sizeof(out_filename->buf), "quake00.%s", ext);

	for(i=0 ; i<=99 ; i++)
	{
		out_filename->buf[5] = i/10 + '0';
		out_filename->buf[6] = i%10 + '0';
		q_snprintf (checkname, sizeof(checkname), "%s/%s", com_gamedir, out_filename->buf);
		if (Sys_FileTime(checkname) == -1)
			break;	// file doesn't exist
	}
	if(i==100)
	{
		return false;
 	}
	return true;
}


#ifdef SOFTQUAKE_ENABLE_PNG
#include "stb_image_write.h"

typedef struct
{
	int handle;
} png_write_data;

static void PNGWriteCallback(void *context, void *data, int size)
{
	// In case this function ever gets called more than once, we use Sys_FileWrite instead of COM_WriteFile,
	// which expects the entirety of the data in one call.
	// The current version of stb_image_write 'does' write the file in one call, but just in case...
	png_write_data *ctx = context;
	Sys_FileWrite(ctx->handle, data, size);
}

void SCR_WritePNGFile(const char *filename, const void *data, int width, int height, int channels, qboolean flip)
{
	png_write_data ctx = {0};
	char full_filepath[MAX_OSPATH] = {0};
	int stride_in_bytes = width * channels;

	// This is duplicating the logic found in COM_WriteFile
	q_snprintf(full_filepath, sizeof(full_filepath), "%s/%s", com_gamedir, filename);
	ctx.handle = Sys_FileOpenWrite(full_filepath);

	stbi_write_png_compression_level = 5;
	stbi_flip_vertically_on_write(flip);

	if(ctx.handle == -1)
	{
		Sys_Printf("Could not open file '%s' for writing\n", full_filepath);
	}
	else
	{
		// stb_image_write returns 0 on failure
		int rc = stbi_write_png_to_func(PNGWriteCallback, &ctx, width, height, channels, data, stride_in_bytes);
		if(rc == 0)
		{
			Sys_Printf("ERROR: Could not write png file: %s\n", full_filepath);
		}
		Sys_FileClose(ctx.handle);
	}
}

#endif /* SOFTQUAKE_ENABLE_PNG */
