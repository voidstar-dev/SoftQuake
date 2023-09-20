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

// Inspired by Quakespasm's version method

#define SOFTQUAKE_VERSION_MAJOR 1
#define SOFTQUAKE_VERSION_MINOR 1
#define SOFTQUAKE_VERSION_PATCH 0

#define SQ_STRING1(x) #x
#define SQ_STRING(x) SQ_STRING1(x)

#define SOFTQUAKE_VERSION SQ_STRING(SOFTQUAKE_VERSION_MAJOR) "." SQ_STRING(SOFTQUAKE_VERSION_MINOR) "." SQ_STRING(SOFTQUAKE_VERSION_PATCH)

const char *SQ_Version(const char *prefix);
