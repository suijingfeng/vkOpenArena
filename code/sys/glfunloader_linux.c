
/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
This file is part of Quake III Arena source code.
Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
/*
** LINUX_QGL.C
**
** This file implements the operating system binding of GL to QGL function
** pointers.  When doing a port of Quake2 you must implement the following
** two functions:
**
** QGL_Init() - loads libraries, assigns function pointers, etc.
** QGL_Shutdown() - unloads libraries, NULLs function pointers
*/

#include <unistd.h>
#include <sys/types.h>
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../renderer/qgl.h"
#include "../renderer/tr_types.h"
#include "unix_glw.h"

#include <dlfcn.h>

#define GLE( ret, name, ... ) ret ( APIENTRY * q##name )( __VA_ARGS__ );
QGL_LinX11_PROCS;
QGL_Swp_PROCS;
#undef GLE


void *GL_GetProcAddress( const char *symbol )
{
	void *sym = dlsym( glw_state.OpenGLLib, symbol );
	if ( !sym )
	{
		fprintf(stderr, "Error resolving %s\n", symbol);
	}

	return sym;
}

char *do_dlerror( void );


/*
** QGL_Init
**
** This is responsible for binding our qgl function pointers to 
** the appropriate GL stuff.  In Windows this means doing a 
** LoadLibrary and a bunch of calls to GetProcAddress.  On other
** operating systems we need to do the right thing, whatever that
** might be.
** 
*/
qboolean QGL_Init( const char *dllname )
{
	Com_Printf( "...initializing QGL\n" );

	if ( glw_state.OpenGLLib == NULL )
	{
		Com_Printf( "...loading '%s' : ", dllname );

		glw_state.OpenGLLib = dlopen( dllname, RTLD_NOW | RTLD_GLOBAL );

		if ( glw_state.OpenGLLib == NULL )
		{
#if 0
			char fn[1024];

			Com_Printf( "\n...loading '%s' : ", dllname );
			// if we are not setuid, try current directory
			if ( dllname != NULL )
			{
				if ( getcwd( fn, sizeof( fn ) ) )
				{
					Q_strcat( fn, sizeof( fn ), "/" );
					Q_strcat( fn, sizeof( fn ), dllname );
					glw_state.OpenGLLib = dlopen( fn, RTLD_NOW );
				}

				if ( glw_state.OpenGLLib == NULL )
				{
					Com_Printf( "failed\n" );
					Com_Printf( "QGL_Init: Can't load %s from /etc/ld.so.conf or current dir: %s\n", dllname, do_dlerror() );
					return qfalse;
				}
			}
			else
#endif
			{
				Com_Printf( "failed\n" );
				Com_Printf( "QGL_Init: Can't load %s from /etc/ld.so.conf: %s\n", dllname, do_dlerror() );
				return qfalse;
			}
		}

		Com_Printf( "succeeded\n" );
	}

#define GLE( ret, name, ... ) q##name = GL_GetProcAddress( XSTRING( name ) ); if ( !q##name ) { frintf(stderr, "Error resolving core X11 functions\n" ); return qfalse; }
	QGL_LinX11_PROCS;
#undef GLE

	return qtrue;
}
