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

#include "quakedef.h"

// Version string for the window title
const char *SQ_Version(const char *prefix)
{
	static char buf[256];
	if(prefix)
	{
		q_snprintf(buf, sizeof(buf), "%s %s", prefix, SOFTQUAKE_VERSION);
	}
	else
	{
		q_snprintf(buf, sizeof(buf), "%s", SOFTQUAKE_VERSION);
	}
	return buf;
}
