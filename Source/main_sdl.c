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
#include <SDL2/SDL.h>

// From sys_* files
extern char *basedir;
void Sys_Init(void);


// From vid
extern cvar_t vid_vsync;

static void PrintSDLVersion(void)
{
	SDL_version Compiled;
	SDL_version Linked;

	SDL_VERSION(&Compiled);
	SDL_GetVersion(&Linked);

	Sys_Printf("Compiled against SDL version: %d.%d.%d\n", Compiled.major, Compiled.minor, Compiled.patch);
	Sys_Printf("Linked against SDL version:   %d.%d.%d\n", Linked.major, Linked.minor, Linked.patch);
}

void Main_AtExit(void)
{
	SDL_Quit();
}

static float Millis(float m)
{
	return m / 1000.0;
}

int main(int argc, char **argv)
{
	double		time, oldtime, newtime;
	quakeparms_t parms;
	extern int vcrFile;
	extern int recording;
	int mem_argc;

	memset(&parms, 0, sizeof(parms));

	atexit(Main_AtExit);

	

	COM_InitArgv(argc, argv);
	parms.argc = com_argc;
	parms.argv = com_argv;

#ifdef GLQUAKE
	parms.memsize = 16*1024*1024;
#else
	parms.memsize = 8*1024*1024;
#endif
	mem_argc = COM_CheckParm("-mem");
	if(mem_argc)
	{
		parms.memsize = (int) (Q_atof(com_argv[mem_argc+1]) * 1024 * 1024);
	}
	parms.membase = malloc (parms.memsize);
	parms.basedir = basedir;

	if (COM_CheckParm("-nostdout"))
	{
	   fclose(stdout);
	   sys_stdout = 0;
	}

	PrintSDLVersion();

	Con_Printf ("SDL SoftQuake -- Version %s\n", SOFTQUAKE_VERSION);

    Host_Init(&parms);

	Sys_Init();



    oldtime = Sys_FloatTime () - 0.1;
    for(;;)
    {
		// find time spent rendering last frame
        newtime = Sys_FloatTime ();
        time = newtime - oldtime;
		double elapsed = time;

        if (cls.state == ca_dedicated)
        {   // play vcrfiles at max speed
            if (time < sys_ticrate.value && (vcrFile == -1 || recording) )
            {
				SDL_Delay(1);
                continue;       // not time to run a server only tic yet
            }
            time = sys_ticrate.value;
        }

        if (time > sys_ticrate.value*2)
            oldtime = newtime;
        else
            oldtime += time;

        Host_Frame (time);

		if(VID_IsMinimized())
		{
			SDL_Delay(16);
		}


		// Borrowed from Quakespasm
		// This will usually happen if vsync is off or the video system is set to a higher refresh rate than 60hz
		// We don't want to hog the CPU in this case, so sleep a bit
		if((time < Millis(20) && !cls.timedemo) && vid_vsync.value)
		{
			SDL_Delay(1);
		}
		// static int counter = 0;
		// counter++;
		// if(counter == 100)
		{
			// char buf[256];
			// counter = 0;
			// int fps = 1.0 / host_frametime;
			// q_snprintf(buf, sizeof(buf), "FPS: %d\n", fps);
			// Con_Printf ("%s", buf);
		}
    }
}
