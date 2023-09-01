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
#include "sdl_common.h"


typedef struct
{
	int x;
	int y;
} mouse;


// When not set, mouse grab is released whenever the game has the menu or console up
// Otherwise, the mouse is always grabbed
cvar_t in_grabmouse = {"in_grabmouse", "0", true};


static mouse mouse_delta;
static mouse last_os_cursor_pos;
static qboolean in_hasfocus = true;
static qboolean in_ignore_mouse_motion = false;

qboolean VID_IsFullscreen(void);
void IN_ActivateMouse(void);
int IN_MouseFilter(void *Data, SDL_Event *Event);


/**********************************
* Mouse motion
**********************************/
// Init mouse code here
void IN_Init(void)
{
	SDL_zero(mouse_delta);
	SDL_zero(last_os_cursor_pos);

	Cvar_RegisterVariable(&in_grabmouse);
	SDL_InitSubSystem(SDL_INIT_EVENTS);
	IN_ActivateMouse();

	SDL_SetEventFilter(IN_MouseFilter, 0);
}

static void IN_MouseMove(usercmd_t *cmd)
{
	int mouse_x = mouse_delta.x;
	int mouse_y = mouse_delta.y;

	mouse_delta.x = 0;
	mouse_delta.y = 0;

	// softquake -- On my Linux OS, SDL still tracks mouse movement even when the game window does not have focus
	// This may be a quirk of my WM, but either way, this simply disables mouse movement from happening when it shouldn't
	// Requires more testing to be sure
	if(!in_hasfocus)
	{
		return;
	}

	mouse_x *= sensitivity.value;
	mouse_y *= sensitivity.value;

	if ( (in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1) ))
	{
		cmd->sidemove += m_side.value * mouse_x;
	}
	else
	{
		cl.viewangles[YAW] -= m_yaw.value * mouse_x;
	}
	if (in_mlook.state & 1)
	{
		// if(mouse_x || mouse_y)
		{
			V_StopPitchDrift ();
		}
	}

	if ( (in_mlook.state & 1) && !(in_strafe.state & 1))
	{
		cl.viewangles[PITCH] += m_pitch.value * mouse_y;
		if (cl.viewangles[PITCH] > 80) cl.viewangles[PITCH] = 80;
		if (cl.viewangles[PITCH] < -70) cl.viewangles[PITCH] = -70;
	}
	else
	{
		if ((in_strafe.state & 1) && noclip_anglehack)
		{
			cmd->upmove -= m_forward.value * mouse_y;
		}
		else
		{
			cmd->forwardmove -= m_forward.value * mouse_y;
		}
	}
}
void IN_Move(usercmd_t *cmd)
{
	IN_MouseMove(cmd);
}

typedef struct
{
	int down;
} mouse_button;
enum
{
	MBUTTON_LEFT,
	MBUTTON_RIGHT,

	MB_TOTAL
};

static mouse_button buttons_cur[MB_TOTAL];
static mouse_button buttons_prev[MB_TOTAL];


// This grabs the mouse input
void IN_ActivateMouse(void)
{
	if(!SDL_GetRelativeMouseMode())
	{
		int mx, my;
#if 0
		SDL_GetMouseState(&mx, &my);
		last_os_cursor_pos.x = mx;
		last_os_cursor_pos.y = my;
#endif
		SDL_SetRelativeMouseMode(1);
	}
}

// This ungrabs the mouse input
void IN_DeactivateMouse(void)
{
	if(SDL_GetRelativeMouseMode())
	{
		qboolean lastfocus = in_hasfocus;
		SDL_SetRelativeMouseMode(0);

		// Hacky way of restoring the mouse cursor position to where it was last visible
		// Not tested at all, I'll leave this until later
		// Right now we're just going with SDL2's default behaviour
#if 0
		SDL_Window *w = SDL_GetMouseFocus();
		in_hasfocus = false;
		in_ignore_mouse_motion =  true;
		SDL_WarpMouseInWindow(w, last_os_cursor_pos.x, last_os_cursor_pos.y);
		in_hasfocus = lastfocus;
#endif
	}
}

void IN_SetMouseModeGeneric(void)
{
	// Global key variable 'key_dest'
	// I had to look at gl_vidlinuxgl.c to see how it was used
	// For a truly vanilla experience, we always want to keep the mouse grabbed and never call this code
	// However, the reality is that it's not very nice to the player of this game, including myself :)
	// That said, we only act on this flag if we're not in fullscreen mode

	if(!in_hasfocus)
	{
		IN_DeactivateMouse();
		return;
	}

	if(!in_grabmouse.value && !VID_IsFullscreen())
	{
		if(key_dest == key_game)
		{
			IN_ActivateMouse();
		}
		else
		{
			IN_DeactivateMouse();
		}
	}
	else
	{
		IN_ActivateMouse();
	}
}

void IN_SetMouseModeWin32(void)
{
	// The Windows windowing system seems to take care of mouse focus for us
	// From my own testing, there's not much we need to do
	// Yes, this is duplicated code but I want to keep the logic seperate for now

	if(!in_hasfocus)
	{
		return;
	}

	if(!in_grabmouse.value && !VID_IsFullscreen())
	{
		if(key_dest == key_game)
		{
			IN_ActivateMouse();
		}
		else
		{
			IN_DeactivateMouse();
		}
	}
	else
	{
		IN_ActivateMouse();
	}

	if(in_grabmouse.value)
	{
		IN_ActivateMouse();
	}
}

void IN_Commands(void)
{
	int i;

	for(i = 0; i < MB_TOTAL; i++)
	{
		if(buttons_cur[i].down && !(buttons_prev[i].down))
		{
			Key_Event(K_MOUSE1 + i, 1);
		}

		if(!buttons_cur[i].down && buttons_prev[i].down)
		{
			Key_Event(K_MOUSE1 + i, 0);
		}
		buttons_prev[i] = buttons_cur[i];
	}


#ifndef _WIN32
	IN_SetMouseModeGeneric();
#else
	IN_SetMouseModeWin32();
#endif /* !_WIN32 */

	// Todo: Other platforms
}

void IN_Shutdown(void)
{
	SDL_SetRelativeMouseMode(0);
}


#if 0
int TranslateKeyFromKeycode(int SdlKey)
{
	int key = 0;
	switch(SdlKey)
	{
		case SDLK_PAGEUP: key = K_PGUP; break;

		case SDLK_PAGEDOWN:	 key = K_PGDN; break;

		case SDLK_HOME:	 key = K_HOME; break;

		case SDLK_END:	 key = K_END; break;

		case SDLK_LEFT:	 key = K_LEFTARROW; break;

		case SDLK_RIGHT:	key = K_RIGHTARROW;		break;

		case SDLK_DOWN:	 key = K_DOWNARROW; break;

		case SDLK_UP:		 key = K_UPARROW;	 break;

		case SDLK_ESCAPE: key = K_ESCAPE;		break;

		case SDLK_KP_ENTER:
		case SDLK_RETURN: key = K_ENTER;		 break;

		case SDLK_TAB:		key = K_TAB;			 break;

		case SDLK_F1:		 key = K_F1;				break;

		case SDLK_F2:		 key = K_F2;				break;

		case SDLK_F3:		 key = K_F3;				break;

		case SDLK_F4:		 key = K_F4;				break;

		case SDLK_F5:		 key = K_F5;				break;

		case SDLK_F6:		 key = K_F6;				break;

		case SDLK_F7:		 key = K_F7;				break;

		case SDLK_F8:		 key = K_F8;				break;

		case SDLK_F9:		 key = K_F9;				break;

		case SDLK_F10:		key = K_F10;			 break;

		case SDLK_F11:		key = K_F11;			 break;

		case SDLK_F12:		key = K_F12;			 break;

		case SDLK_BACKSPACE: key = K_BACKSPACE; break;

		// case SDLK_KP_Delete:
		case SDLK_DELETE: key = K_DEL; break;

		case SDLK_PAUSE:	key = K_PAUSE;		 break;

		case SDLK_LSHIFT:
		case SDLK_RSHIFT:	key = K_SHIFT;		break;

		case SDLK_EXECUTE:
		case SDLK_LCTRL:
		case SDLK_RCTRL:	key = K_CTRL;		 break;

		case SDLK_RALT:
		case SDLK_LALT:
		key = K_ALT;			break;

		// case SDLK_KP_Begin: key = K_AUX30;	break;

		case SDLK_INSERT: key = K_INS; break;

		case SDLK_ASTERISK:
		case SDLK_KP_MULTIPLY: key = '*'; break;

		case SDLK_PLUS:
		case SDLK_KP_PLUS: key = '+'; break;

		case SDLK_MINUS:
		case SDLK_KP_MINUS: key = '-'; break;

		case SDLK_KP_DIVIDE: key = '/'; break;

		case SDLK_KP_PERIOD: key = '.'; break;

		// softquake -- Add keypad number codes
		case SDLK_KP_0: key = K_KP_0; break;
		case SDLK_KP_1: key = K_KP_1; break;
		case SDLK_KP_2: key = K_KP_2; break;
		case SDLK_KP_3: key = K_KP_3; break;
		case SDLK_KP_4: key = K_KP_4; break;
		case SDLK_KP_5: key = K_KP_5; break;
		case SDLK_KP_6: key = K_KP_6; break;
		case SDLK_KP_7: key = K_KP_7; break;
		case SDLK_KP_8: key = K_KP_8; break;
		case SDLK_KP_9: key = K_KP_9; break;

		default:
			key = tolower(SdlKey);
	}

	return key;
}
#else
// Uses the physical location of buttons, rather than the local keyboard layout
// This tricks the game into thinking that it's always using an ascii keyboard
// The problem with this currently is that it breaks text input - Only English text is recognized
// Adapted from Quakespasm
int TranslateKeyFromScancode(int SdlScancode)
{
	int key = 0;
	switch(SdlScancode)
	{
		case SDL_SCANCODE_A: key = 'a'; break;
		case SDL_SCANCODE_B: key = 'b'; break;
		case SDL_SCANCODE_C: key = 'c'; break;
		case SDL_SCANCODE_D: key = 'd'; break;
		case SDL_SCANCODE_E: key = 'e'; break;
		case SDL_SCANCODE_F: key = 'f'; break;
		case SDL_SCANCODE_G: key = 'g'; break;
		case SDL_SCANCODE_H: key = 'h'; break;
		case SDL_SCANCODE_I: key = 'i'; break;
		case SDL_SCANCODE_J: key = 'j'; break;
		case SDL_SCANCODE_K: key = 'k'; break;
		case SDL_SCANCODE_L: key = 'l'; break;
		case SDL_SCANCODE_M: key = 'm'; break;
		case SDL_SCANCODE_N: key = 'n'; break;
		case SDL_SCANCODE_O: key = 'o'; break;
		case SDL_SCANCODE_P: key = 'p'; break;
		case SDL_SCANCODE_Q: key = 'q'; break;
		case SDL_SCANCODE_R: key = 'r'; break;
		case SDL_SCANCODE_S: key = 's'; break;
		case SDL_SCANCODE_T: key = 't'; break;
		case SDL_SCANCODE_U: key = 'u'; break;
		case SDL_SCANCODE_V: key = 'v'; break;
		case SDL_SCANCODE_W: key = 'w'; break;
		case SDL_SCANCODE_X: key = 'x'; break;
		case SDL_SCANCODE_Y: key = 'y'; break;
		case SDL_SCANCODE_Z: key = 'z'; break;

		case SDL_SCANCODE_1: key = '1'; break;
		case SDL_SCANCODE_2: key = '2'; break;
		case SDL_SCANCODE_3: key = '3'; break;
		case SDL_SCANCODE_4: key = '4'; break;
		case SDL_SCANCODE_5: key = '5'; break;
		case SDL_SCANCODE_6: key = '6'; break;
		case SDL_SCANCODE_7: key = '7'; break;
		case SDL_SCANCODE_8: key = '8'; break;
		case SDL_SCANCODE_9: key = '9'; break;
		case SDL_SCANCODE_0: key = '0'; break;

		case SDL_SCANCODE_RETURN2: 			key = K_ENTER; break;
		case SDL_SCANCODE_SPACE: 			key = K_SPACE; break;

		case SDL_SCANCODE_EQUALS: 			key = '='; break;
		case SDL_SCANCODE_LEFTBRACKET: 		key = '['; break;
		case SDL_SCANCODE_RIGHTBRACKET: 	key = ']'; break;
		case SDL_SCANCODE_BACKSLASH: 		key = '\\'; break;
		case SDL_SCANCODE_NONUSHASH: 		key = '#'; break;
		case SDL_SCANCODE_SEMICOLON: 		key = ';'; break;
		case SDL_SCANCODE_APOSTROPHE: 		key = '\''; break;
		case SDL_SCANCODE_GRAVE: 			key = '`'; break;
		case SDL_SCANCODE_COMMA: 			key = ','; break;
		case SDL_SCANCODE_PERIOD: 			key = '.'; break;
		case SDL_SCANCODE_SLASH: 			key = '/'; break;
		case SDL_SCANCODE_NONUSBACKSLASH: 	key = '\\'; break;


		case SDL_SCANCODE_PAGEUP: 		key = K_PGUP; break;
		case SDL_SCANCODE_PAGEDOWN:	 	key = K_PGDN; break;

		case SDL_SCANCODE_HOME:	 		key = K_HOME; break;
		case SDL_SCANCODE_END:	 		key = K_END; break;

		case SDL_SCANCODE_LEFT:	 		key = K_LEFTARROW; break;
		case SDL_SCANCODE_RIGHT:		key = K_RIGHTARROW; break;
		case SDL_SCANCODE_DOWN:	 		key = K_DOWNARROW; break;
		case SDL_SCANCODE_UP:		 	key = K_UPARROW; break;

		case SDL_SCANCODE_ESCAPE: 		key = K_ESCAPE; break;

		case SDL_SCANCODE_KP_ENTER:
		case SDL_SCANCODE_RETURN: 		key = K_ENTER; break;

		case SDL_SCANCODE_TAB:			key = K_TAB; break;
		case SDL_SCANCODE_F1:			key = K_F1; break;
		case SDL_SCANCODE_F2:			key = K_F2; break;
		case SDL_SCANCODE_F3:			key = K_F3; break;
		case SDL_SCANCODE_F4:			key = K_F4; break;
		case SDL_SCANCODE_F5:			key = K_F5; break;
		case SDL_SCANCODE_F6:			key = K_F6; break;
		case SDL_SCANCODE_F7:			key = K_F7; break;
		case SDL_SCANCODE_F8:			key = K_F8; break;
		case SDL_SCANCODE_F9:			key = K_F9; break;
		case SDL_SCANCODE_F10:			key = K_F10; break;
		case SDL_SCANCODE_F11:			key = K_F11; break;
		case SDL_SCANCODE_F12:			key = K_F12; break;

		case SDL_SCANCODE_BACKSPACE: 	key = K_BACKSPACE; break;

									 // case SDL_SCANCODE_KP_Delete:
		case SDL_SCANCODE_DELETE: 		key = K_DEL; break;
		case SDL_SCANCODE_PAUSE:		key = K_PAUSE; break;

		case SDL_SCANCODE_LSHIFT:
		case SDL_SCANCODE_RSHIFT:		key = K_SHIFT; break;

		case SDL_SCANCODE_EXECUTE:

		case SDL_SCANCODE_LCTRL:
		case SDL_SCANCODE_RCTRL:		key = K_CTRL; break;

		case SDL_SCANCODE_RALT:
		case SDL_SCANCODE_LALT:			key = K_ALT; break;

		case SDL_SCANCODE_INSERT: 		key = K_INS; break;

		// case '*':
		case SDL_SCANCODE_KP_MULTIPLY: 	key = '*'; break;

		// case '+':
		case SDL_SCANCODE_KP_PLUS: 		key = '+'; break;

		case SDL_SCANCODE_MINUS:
		case SDL_SCANCODE_KP_MINUS: 	key = '-'; break;

		case SDL_SCANCODE_KP_DIVIDE: 	key = '/'; break;

		case SDL_SCANCODE_KP_PERIOD: 	key = '.'; break;

		// softquake -- Add keypad number codes
		case SDL_SCANCODE_KP_0: 		key = K_KP_0; break;
		case SDL_SCANCODE_KP_1: 		key = K_KP_1; break;
		case SDL_SCANCODE_KP_2: 		key = K_KP_2; break;
		case SDL_SCANCODE_KP_3: 		key = K_KP_3; break;
		case SDL_SCANCODE_KP_4: 		key = K_KP_4; break;
		case SDL_SCANCODE_KP_5: 		key = K_KP_5; break;
		case SDL_SCANCODE_KP_6: 		key = K_KP_6; break;
		case SDL_SCANCODE_KP_7: 		key = K_KP_7; break;
		case SDL_SCANCODE_KP_8: 		key = K_KP_8; break;
		case SDL_SCANCODE_KP_9: 		key = K_KP_9; break;

		default:
			key = 0;
	}

	return key;
}

#endif

int IN_MouseFilter(void *Data, SDL_Event *Event)
{
	// Return 1 to add event to the queue
	// Return 0 to ignore the event

	if(Event->type == SDL_MOUSEMOTION)
	{
		// Ignore mouse events when we don't have mouse focus.
		// I think simply returning 0 is sufficient, but we'll set the values just in case.
		// This fixes alt-tabbing in Windows (both for windowed and fullscreen modes).
		// Without this filter, the values returned by xrel and yrel
		// will not be consistent whenever the window is tabbed back into focus.
		// Essentially, the player's view direction was changing on every alt-tab.

		// Forcing SDL_SetRelativeMouseMode in the SDL_WINDOWEVENT_FOCUS_GAINED and SDL_WINDOWEVENT_FOCUS_LOST
		// cases was casuing alt-tab to not work correctly (the Quake window is always the topmost window)
		// so I opted for this method instead.

		// I still don't like it because it doesn't remember the OS cursor position.
		// That can be fixed by keeping track of it on every mode switch, but that's too unpredictable.

		// Thanks to Quakespasm for the idea

		// Todo: NOT THREAD SAFE. Use SDL_Atomic...
		if(!in_hasfocus || in_ignore_mouse_motion)
		{
			Event->motion.xrel = 0;
			Event->motion.yrel = 0;
			return 0;
		}
	}
	return 1;
}

void IN_SendKeyEvents(void)
{
	SDL_Event Event;
	int down = 0;
	int quakekey = 0;
	int mousepressed = 0;
	int wheelevent = 0;

	while(SDL_PollEvent(&Event))
	{
		switch(Event.type)
		{
			case SDL_QUIT:
				Sys_Quit();
				break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				// Get rid of key queue, and handle key presses directly as they appear
				// For some reason this was casuing an infinite loop when SCR_ModalMessage was called
				// on a new game
				down = (Event.type == SDL_KEYDOWN);
				// quakekey = TranslateKeyFromKeycode(Event.key.keysym.sym);
				quakekey = TranslateKeyFromScancode(Event.key.keysym.scancode);
				Key_Event(quakekey, down);
				break;
			case SDL_MOUSEMOTION:
				// Note: Accumulate the values!
				// Thanks to Quakespasm
				mouse_delta.x += Event.motion.xrel;
				mouse_delta.y += Event.motion.yrel;
				break;
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
				mousepressed = (Event.type == SDL_MOUSEBUTTONDOWN);
				if(Event.button.button == SDL_BUTTON_LEFT) buttons_cur[MBUTTON_LEFT].down = mousepressed;
				if(Event.button.button == SDL_BUTTON_RIGHT) buttons_cur[MBUTTON_RIGHT].down = mousepressed;
				break;
			case SDL_MOUSEWHEEL:
				// Note: Quake's original .qc files lacked the ability to select a previous weapon,
				// so MWHEELUP does nothing.
				// This was addressed by later fan mods, and is not a bug!
				// However, things like the console scrollback buffer do work
				if(Event.wheel.y > 0)
				{
					wheelevent = K_MWHEELUP;
					Key_Event(wheelevent, 1);
					Key_Event(wheelevent, 0);
				}
				else if(Event.wheel.y < 0)
				{
					wheelevent = K_MWHEELDOWN;
					Key_Event(wheelevent, 1);
					Key_Event(wheelevent, 0);
				}
				break;

			case SDL_WINDOWEVENT:
				switch(Event.window.event)
				{
					case SDL_WINDOWEVENT_SIZE_CHANGED:
						vid_client_width = Event.window.data1;
						vid_client_height = Event.window.data2;
						break;

					case SDL_WINDOWEVENT_FOCUS_LOST:
						in_hasfocus = false;
						break;
					case SDL_WINDOWEVENT_FOCUS_GAINED:
						in_hasfocus = true;
						break;
				}
				break;
		}
	}
}

// Moved from sys file for code reuse
void Sys_SendKeyEvents(void)
{
	IN_Commands();
	IN_SendKeyEvents();
	in_ignore_mouse_motion = false;
}
