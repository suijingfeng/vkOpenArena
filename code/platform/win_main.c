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
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// win_main.c
#include "win_public.h"

#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
//#include <VersionHelpers.h>

#include "../client/client.h"
#include "../qcommon/qcommon.h"

#include "resource.h"
#include "win_sysconsole.h"


extern WinVars_t g_wv;

cvar_t * com_viewlog;



/*
=============
Sys_Error

Show the early console as an error dialog
=============
*/
void QDECL Sys_Error( const char * error, ... )
{
	va_list		argptr;
	char		text[4096];
        MSG        msg;

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	Sys_Print( text );
	Sys_Print( "\n" );

	Sys_SetErrorText( text );
	Sys_ShowConsole( 1, qtrue );

	timeEndPeriod( 1 );

	IN_Shutdown();

	// wait for the user to quit
	while ( 1 )
	{
		if (!GetMessage (&msg, NULL, 0, 0))
			Com_Quit_f ();
		TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

	Sys_DestroyConsole();

	exit (1);
}

/*
==============
Sys_Quit
==============
*/
void Sys_Quit( void )
{
	timeEndPeriod( 1 );
	IN_Shutdown();
	Sys_DestroyConsole();

	exit (0);
}



//=======================================================================


int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

	char sys_cmdline[MAX_STRING_CHARS];

	int	totalMsec = 0;
	int countMsec = 0;

	// should never get a previous instance in Win32
	if (hPrevInstance) {
		return 0;
	}

	g_wv.hInstance = hInstance;
	Q_strncpyz(sys_cmdline, lpCmdLine, sizeof(sys_cmdline));

	g_wv.osversion.dwOSVersionInfoSize = sizeof(g_wv.osversion);

	if (!GetVersionEx(&g_wv.osversion))
		Sys_Error("Couldn't get OS info");

	if (g_wv.osversion.dwMajorVersion < 4)
		Sys_Error("Quake3 requires Windows version 4 or greater");
	if (g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32s)
		Sys_Error("Quake3 doesn't run on Win32s");

	Sys_PlatformInit();

	// done before Com/Sys_Init since we need this for error output
	Sys_CreateConsole();

	// no abort/retry/fail errors

	// SEM_FAILCRITICALERRORS = 0x0001 :
	// The system does not display the critical - error - handler message box. 
	// Instead, the system sends the error to the calling process.
	// Best practice is that all applications call the process - wide SetErrorMode
	// function with a parameter of SEM_FAILCRITICALERRORS at startup. 
	// This is to prevent error mode dialogs from hanging the application.

	SetErrorMode(SEM_FAILCRITICALERRORS);

	// get the initial time base
	Sys_Milliseconds();


	com_viewlog = Cvar_Get("viewlog", "0", CVAR_CHEAT);

	Com_Init(sys_cmdline);


	// IN_Init();		// FIXME: not in dedicated?

	NET_Init();

	{
		char cwd[MAX_OSPATH];
		_getcwd(cwd, sizeof(cwd));

		Com_Printf(" Working directory: %s\n", cwd);
	}
	// hide the early console since we've reached the point where we
	// have a working graphics subsystems
	if ( !com_dedicated->integer && !com_viewlog->integer ) {
		Sys_ShowConsole( 0, qfalse );
	}

        // main game loop
	while( 1 )
	{
		// if not running as a game client, sleep a bit
		if ( g_wv.isMinimized || ( com_dedicated && com_dedicated->integer ) ) {
			Sleep( 5 );
		}

		// set low precision every frame, because some system calls
		// reset it arbitrarily
		// _controlfp( _PC_24, _MCW_PC );
		//_controlfp( -1, _MCW_EM  ); // no exceptions, even if some crappy
                                // syscall turns them back on!

		int startTime = Sys_Milliseconds();

		// make sure mouse and joystick are only called once a frame
		IN_Frame();

		// run the game
		Com_Frame();

		int endTime = Sys_Milliseconds();
		totalMsec += endTime - startTime;
		++countMsec;
	}
	// never gets here
}
