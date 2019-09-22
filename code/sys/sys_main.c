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

#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

#include "../sys/sys_public.h"


#ifdef DEDICATED
#	define PID_FILENAME PRODUCT_NAME "_server.pid"
#else
#	define PID_FILENAME PRODUCT_NAME ".pid"
#endif

/*
=================
Sys_PIDFileName
=================
*/
static char *Sys_PIDFileName(void)
{
	const char *homePath = Cvar_VariableString("fs_homepath");

	if( *homePath != '\0' )
		return va( "%s/%s", homePath, PID_FILENAME);

	return NULL;
}

/*
=================
Sys_WritePIDFile

Return qtrue if there is an existing stale PID file
=================
*/
qboolean Sys_WritePIDFile( void )
{
	char *pidFile = Sys_PIDFileName( );
	FILE *f;
	qboolean stale = qfalse;

	if( pidFile == NULL )
		return qfalse;

	// First, check if the pid file is already there
	if( ( f = fopen( pidFile, "r" ) ) != NULL )
	{
		char  pidBuffer[ 64 ] = { 0 };
		int pid = fread( pidBuffer, sizeof( char ), sizeof( pidBuffer ) - 1, f );
		fclose( f );

		if(pid > 0)
		{
			pid = atoi( pidBuffer );
			if( !Sys_PIDIsRunning( pid ) )
				stale = qtrue;
		}
		else
			stale = qtrue;
	}

	if( FS_CreatePath( pidFile ) ) {
		return 0;
	}

	if( ( f = fopen( pidFile, "w" ) ) != NULL )
	{
		fprintf( f, "%d", Sys_PID( ) );
		fclose( f );
	}
	else
		Com_Printf( S_COLOR_YELLOW "Couldn't write %s.\n", pidFile );

	return stale;
}

/*
=================
Sys_Exit

Single exit point (regular exit or in case of error)
=================
*/
__attribute__ ((noreturn)) void Sys_Exit( int exitCode )
{
	CON_Shutdown( );

	if( exitCode < 2 && com_fullyInitialized)
	{
		// Normal exit
		char *pidFile = Sys_PIDFileName( );

		if( pidFile != NULL )
			remove( pidFile );
	}

	NET_Shutdown( );

	exit( exitCode );
}


void Sys_Quit( void )
{
	Sys_Exit( 0 );
}



void Sys_Error( const char *error, ... )
{
	va_list argptr;
	char string[1024];

	va_start(argptr, error);
	Q_vsnprintf(string, sizeof(string), error, argptr);
	va_end(argptr);

	Sys_ErrorDialog( string );

	Sys_Exit( 3 );
}



/*
============
Sys_FileTime

returns -1 if not present
============
*/
int Sys_FileTime( char *path )
{
	struct stat buf;

	if (stat (path,&buf) == -1)
		return -1;

	return buf.st_mtime;
}



void Sys_ParseArgs( int argc, char **argv )
{
	if( argc == 2 )
	{
		if( !strcmp( argv[1], "--version" ) || !strcmp( argv[1], "-v" ) )
		{
			const char* date = __DATE__;
#ifdef DEDICATED
			fprintf( stdout, Q3_VERSION " dedicated server (%s)\n", date );
#else
			fprintf( stdout, Q3_VERSION " client (%s)\n", date );
#endif
			Sys_Exit( 0 );
		}
	}
}



#ifndef DEFAULT_BASEDIR
#	ifdef MACOS_X
#		define DEFAULT_BASEDIR Sys_StripAppBundle(Sys_GetBinaryPath())
#	else
#		define DEFAULT_BASEDIR Sys_GetBinaryPath()
#	endif
#endif



#if defined( _WIN32 ) && !defined(DEDICATED)

#include "../platform/win_public.h"

extern WinVars_t g_wv;

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	//char sys_cmdline[MAX_STRING_CHARS];

	int	totalMsec = 0;
	int countMsec = 0;

	// should never get a previous instance in Win32
	if (hPrevInstance) {
		return 0;
	}

	g_wv.hInstance = hInstance;
	//Q_strncpyz(sys_cmdline, lpCmdLine, sizeof(sys_cmdline));

	g_wv.osversion.dwOSVersionInfoSize = sizeof(g_wv.osversion);

	if (!GetVersionEx(&g_wv.osversion))
		Sys_Error("Couldn't get OS info");

	if (g_wv.osversion.dwMajorVersion < 4)
		Sys_Error("Quake3 requires Windows version 4 or greater");
	if (g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32s)
		Sys_Error("Quake3 doesn't run on Win32s");

	Sys_PlatformInit();
	// Sys_InitSignal();
	
	// done before Com/Sys_Init since we need this for error output
	// Sys_CreateConsole();

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
	Sys_SetDefaultInstallPath(DEFAULT_BASEDIR);
	Com_Printf(" Working directory: %s\n", Sys_Cwd());
	// com_viewlog = Cvar_Get("viewlog", "0", CVAR_CHEAT);
	CON_Init();
	Com_Init(lpCmdLine);
	NET_Init();

#if	!defined(DEDICATED)
	IN_Init();		// FIXME: not in dedicated?
#endif

	
	// hide the early console since we've reached the point where we
	// have a working graphics subsystems
	// if (!com_dedicated->integer && !com_viewlog->integer) {
	//	Sys_ShowConsole(0, qfalse);
	// }

	// pump the message loop
	// Dispatches incoming sent messages, checks the thread message queue 
	// for a posted message, and retrieves the message (if any exist).
	
	// main game loop
	while (1)
	{
		/*
				// if not running as a game client, sleep a bit
				if (g_wv.isMinimized || (com_dedicated && com_dedicated->integer)) {
					Sleep(5);
				}

				// set low precision every frame, because some system calls
				// reset it arbitrarily
				// _controlfp( _PC_24, _MCW_PC );
				//_controlfp( -1, _MCW_EM  ); // no exceptions, even if some crappy
										// syscall turns them back on!

				int startTime = Sys_Milliseconds();
		*/
		// make sure mouse and joystick are only called once a frame
		// IN_Frame();
		MSG	msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage(&msg, NULL, 0, 0)) {
				Com_Quit_f();
			}

			// save the msg time, because wndprocs don't have access to the timestamp
			g_wv.sysMsgTime = msg.time;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		
		
		IN_Frame(); // youurayy input lag fix
			// run the game
		Com_Frame();
		
/*
		int endTime = Sys_Milliseconds();
		totalMsec += endTime - startTime;
		++countMsec;
*/
	}
	// never gets here
	return 0;
}

#else

int main( int argc, char **argv )
{

	char commandLine[ MAX_STRING_CHARS ] = { 0 };


	Sys_PlatformInit();
    Sys_InitSignal();
	// Set the initial time base
	Sys_Milliseconds();

#ifdef MACOS_X
	// This is passed if we are launched by double-clicking
	if ( argc >= 2 && Q_strncmp( argv[1], "-psn", 4 ) == 0 )
		argc = 1;
#endif

	Sys_ParseArgs( argc, argv );
	Sys_SetBinaryPath( Sys_Dirname( argv[ 0 ] ) );
	Sys_SetDefaultInstallPath( DEFAULT_BASEDIR );

	// Concatenate the command line for passing to Com_Init
	for(int i = 1; i < argc; ++i)
	{
		const qboolean containsSpaces = (strchr(argv[i], ' ') != NULL);
		if(containsSpaces)
			Q_strcat( commandLine, sizeof( commandLine ), "\"" );

		Q_strcat( commandLine, sizeof( commandLine ), argv[ i ] );

		if(containsSpaces)
			Q_strcat( commandLine, sizeof( commandLine ), "\"" );

		Q_strcat( commandLine, sizeof( commandLine ), " " );
	}

	CON_Init();
	Com_Init(commandLine);
	NET_Init();

	while( 1 )
	{
		// youurayy input lag fix
		IN_Frame();
		Com_Frame();
	}

	return 0;
}

#endif
