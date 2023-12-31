/*
Copyright (C) 1996-1997 Id Software, Inc.
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
// cvar.c -- dynamic variable tracking

#include "quakedef.h"

cvar_t	*cvar_vars;
char	*cvar_null_string = "";

/*
============
Cvar_FindVar
============
*/
cvar_t *Cvar_FindVar (char *var_name)
{
	cvar_t	*var;
	
	for (var=cvar_vars ; var ; var=var->next)
		if (!Q_strcmp (var_name, var->name))
			return var;

	return NULL;
}

/*
============
Cvar_VariableValue
============
*/
float	Cvar_VariableValue (char *var_name)
{
	cvar_t	*var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
	return Q_atof (var->string);
}


/*
============
Cvar_VariableString
============
*/
char *Cvar_VariableString (char *var_name)
{
	cvar_t *var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return cvar_null_string;
	return var->string;
}


/*
============
Cvar_CompleteVariable
============
*/
char *Cvar_CompleteVariable (char *partial)
{
	cvar_t		*cvar;
	int			len;
	
	len = Q_strlen(partial);
	
	if (!len)
		return NULL;
		
// check functions
	for (cvar=cvar_vars ; cvar ; cvar=cvar->next)
		if (!Q_strncmp (partial,cvar->name, len))
			return cvar->name;

	return NULL;
}

cvar_t *Cvar_SetString (char *var_name, char *value)
{
	cvar_t	*var;
	qboolean changed;
	
	var = Cvar_FindVar (var_name);
	if (!var)
	{	// there is an error in C code if this happens
		Con_Printf ("Cvar_Set: variable %s not found\n", var_name);
		return 0;
	}

	if (var->archive & CV_LOCKED)
	{
		Con_Printf ("Cvar_Set: variable %s is locked for readonly access\n", var_name);
		return var;
	}

	changed = Q_strcmp(var->string, value);
	
	Z_Free (var->string);	// free the old value string
	
	var->string = Z_Malloc (Q_strlen(value)+1);
	Q_strcpy (var->string, value);
	var->value = Q_atof (var->string);
	if (var->server && changed)
	{
		if (sv.active)
			SV_BroadcastPrintf ("\"%s\" changed to \"%s\"\n", var->name, var->string);
	}

	return var;
}

/*
============
Cvar_Set
============
*/
void Cvar_Set (char *var_name, char *value)
{
	// softquake -- Add callback functionality and move set function to seperate setting and callback
	cvar_t *var = Cvar_SetString(var_name, value);
	if(var)
		Cvar_RunCallback(var);
}


/*
============
Cvar_SetValueQuick
Like Cvar_SetValue but does not call the callback
============
*/
void Cvar_SetValueQuick (cvar_t *var, float value)
{
	// softquake -- Add setting cvar float value by pointer
	// This is dumb because we already have the pointer
	// Todo: Make this faster
	char	val[32];

	sprintf (val, "%f",value);
	Cvar_SetString (var->name, val);
}

void Cvar_SetStringQuick (cvar_t *var, const char *s)
{
	Cvar_SetString(var->name, (char *)s);
}

/*
============
Cvar_SetValue
============
*/
void Cvar_SetValue (char *var_name, float value)
{
	char	val[32];
	
	sprintf (val, "%f",value);
	Cvar_Set (var_name, val);
}


/*
============
Cvar_RegisterVariable

Adds a freestanding variable to the variable list.
============
*/
void Cvar_RegisterVariable (cvar_t *variable)
{
	char	*oldstr;
	
// first check to see if it has allready been defined
	if (Cvar_FindVar (variable->name))
	{
		Con_Printf ("Can't register variable %s, allready defined\n", variable->name);
		return;
	}
	
// check for overlap with a command
	if (Cmd_Exists (variable->name))
	{
		Con_Printf ("Cvar_RegisterVariable: %s is a command\n", variable->name);
		return;
	}
		
// copy the value off, because future sets will Z_Free it
	oldstr = variable->string;
	variable->string = Z_Malloc (Q_strlen(variable->string)+1);	
	Q_strcpy (variable->string, oldstr);
	variable->value = Q_atof (variable->string);
	
// link the variable in
	variable->next = cvar_vars;
	cvar_vars = variable;
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qboolean	Cvar_Command (void)
{
	cvar_t			*v;

// check variables
	v = Cvar_FindVar (Cmd_Argv(0));
	if (!v)
		return false;
		
// perform a variable print or set
	if (Cmd_Argc() == 1)
	{
		Con_Printf ("\"%s\" is \"%s\"\n", v->name, v->string);
		return true;
	}

	Cvar_Set (v->name, Cmd_Argv(1));
	return true;
}


/*
============
Cvar_WriteVariables

Writes lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables (FILE *f)
{
	cvar_t	*var;
	
	for (var = cvar_vars ; var ; var = var->next)
		if (var->archive & CV_ARCHIVE) // softquake -- compare with mask instead  of absolute value
			fprintf (f, "%s \"%s\"\n", var->name, var->string);
}

// softquake -- New additions
/*
============
Cvar_RegisterCallback
============
*/
void Cvar_RegisterCallback(cvar_t *var, cvar_callback callback)
{
	if(var->archive & CV_CALLBACK)
	{
		var->callback = callback;
	}
	else
	{
		// This is an error in the C code
		Sys_Error("Tried to register a callback for \"%s\" when no callback flag was set\n", var->name);
	}
}

/*
============
Cvar_RunCallback
============
*/
void Cvar_RunCallback(cvar_t *var)
{
	if(var->archive & CV_CALLBACK)
	{
		if(var->callback)
		{
			var->callback(var);
		}
	}
}

/*
============
Cvar_RunAllCallbacks
============
*/
void Cvar_RunAllCallbacks(void)
{
	cvar_t	*var;
	
	for (var=cvar_vars ; var ; var=var->next)
		Cvar_RunCallback(var);
}

/*
============
Cvar_LockVariable
============
*/
void Cvar_LockVariable(cvar_t *var)
{
	var->archive |= CV_LOCKED;
}

void Cvar_UnlockVariable(cvar_t *var)
{
	var->archive &= (~CV_LOCKED);
}
