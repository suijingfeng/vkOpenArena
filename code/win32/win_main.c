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
#include "win_input.h"


#define MEM_THRESHOLD 96*1024*1024

extern WinVars_t g_wv;

// define this to use alternate spanking method
// I found out that the regular way doesn't work on my box for some reason
// see the associated spank.sh script
// #define ALT_SPANK
/*
#ifdef ALT_SPANK
#include <stdio.h>
#include <sys\stat.h>

int fh = 0;

void Spk_Open(char *name)
{
  fh = open( name, O_TRUNC | O_CREAT | O_WRONLY, S_IREAD | S_IWRITE );
};

void Spk_Close()
{
  if (!fh)
    return;

  close( fh );
  fh = 0;
}

void Spk_Printf (const char *text, ...)
{
  va_list argptr;
  char buf[32768];

  if (!fh)
    return;

  va_start (argptr,text);
  vsprintf (buf, text, argptr);
  write(fh, buf, (int)strlen(buf));
  _commit(fh);
  va_end (argptr);

};
#endif
*/


/*
==================
 Retrieves information about the system's current
 usage of both physical and virtual memory.
==================
*/

qboolean Sys_LowPhysicalMemory(void)
{
	MEMORYSTATUSEX stat;

	stat.dwLength = sizeof(stat);

	// You can use the GlobalMemoryStatusEx function to 
	// determine how much memory your application can 
	// allocate without severely impacting other applications.
	//
	// The information returned by the GlobalMemoryStatusEx function
	// is volatile. There is no guarantee that two sequential calls 
	// to this function will return the same information.
	GlobalMemoryStatusEx (&stat);

	Com_Printf( " There is %ld percent of memory in use.\n", 
		stat.dwMemoryLoad );
	Com_Printf( " There are %I64d total MB of physical memory.\n", 
		stat.ullTotalPhys / (1024 * 1024) );
	Com_Printf( " There are %I64d free MB of physical memory.\n",
		stat.ullAvailPhys / (1024 * 1024) );
	Com_Printf( " There are %I64d total MB of paging file.\n",
		stat.ullTotalPageFile / (1024 * 1024) );
	Com_Printf( " There are %I64d free MB of paging file.\n",
		stat.ullAvailPageFile / (1024 * 1024) );
	Com_Printf( " There are %I64d total MB of virtual memory.\n",
		stat.ullTotalVirtual / (1024 * 1024) );
	Com_Printf( " There are %I64d free  MB of virtual memory.\n",
		stat.ullAvailVirtual / (1024 * 1024) );

	// Show the amount of extended memory available.

	Com_Printf( "There are %I64d free MB of extended memory.\n",
		stat.ullAvailExtendedVirtual / (1024 * 1024) );

	// Com_sprintf(search, sizeof(search), "%s\\*", basedir);

	return (stat.ullTotalPhys <= MEM_THRESHOLD) ? qtrue : qfalse;
}


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



/*
==============
Sys_Mkdir
==============
*/
void Sys_Mkdir( const char *path ) {
	_mkdir (path);
}

/*
==============
Sys_Cwd
==============
*/
char * Sys_Cwd( void )
{
	static char cwd[MAX_OSPATH];

	_getcwd( cwd, sizeof( cwd ) - 1 );
	cwd[MAX_OSPATH-1] = 0;

	return cwd;
}

/*
==============
Sys_DefaultCDPath
==============
*/
char * Sys_DefaultCDPath( void )
{
	return "";
}

/*
==============
Sys_DefaultBasePath
==============
*/
char * Sys_DefaultBasePath( void ) {
	return Sys_Cwd();
}

/*
==============================================================

DIRECTORY SCANNING

==============================================================
*/

#define	MAX_FOUND_FILES	0x1000

void Sys_ListFilteredFiles( const char *basedir, char *subdirs, char *filter, char **list, int *numfiles )
{
	char search[MAX_OSPATH], newsubdirs[MAX_OSPATH];
	char filename[MAX_OSPATH];
	int	findhandle;
	struct _finddata_t findinfo;

	if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
		return;
	}

	if (strlen(subdirs)) {
		Com_sprintf( search, sizeof(search), "%s\\%s\\*", basedir, subdirs );
	}
	else {
		Com_sprintf( search, sizeof(search), "%s\\*", basedir );
	}

	findhandle = _findfirst (search, &findinfo);
	if (findhandle == -1) {
		return;
	}

	do {
		if (findinfo.attrib & _A_SUBDIR) {
			if (Q_stricmp(findinfo.name, ".") && Q_stricmp(findinfo.name, "..")) {
				if (strlen(subdirs)) {
					Com_sprintf( newsubdirs, sizeof(newsubdirs), "%s\\%s", subdirs, findinfo.name);
				}
				else {
					Com_sprintf( newsubdirs, sizeof(newsubdirs), "%s", findinfo.name);
				}
				Sys_ListFilteredFiles( basedir, newsubdirs, filter, list, numfiles );
			}
		}
		if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
			break;
		}
		Com_sprintf( filename, sizeof(filename), "%s\\%s", subdirs, findinfo.name );
		if (!Com_FilterPath( filter, filename, qfalse ))
			continue;
		list[ *numfiles ] = CopyString( filename );
		(*numfiles)++;
	} while ( _findnext (findhandle, &findinfo) != -1 );

	_findclose (findhandle);
}


static qboolean strgtr(const char *s0, const char *s1)
{
	int l0, l1, i;

	l0 = (int)strlen(s0);
	l1 = (int)strlen(s1);

	if (l1<l0) {
		l0 = l1;
	}

	for(i=0;i<l0;i++) {
		if (s1[i] > s0[i]) {
			return qtrue;
		}
		if (s1[i] < s0[i]) {
			return qfalse;
		}
	}
	return qfalse;
}

char **Sys_ListFiles( const char *directory, const char *extension, char *filter, int *numfiles, qboolean wantsubs )
{

	int	nfiles = 0;
	char		**listCopy;
	char		*list[MAX_FOUND_FILES];
	struct _finddata_t findinfo;
	intptr_t  findhandle;
	int			flag;
	int			i;

	if (filter)
	{
		Sys_ListFilteredFiles( directory, "", filter, list, &nfiles );

		list[ nfiles ] = 0;
		*numfiles = nfiles;

		if (!nfiles)
			return NULL;

		listCopy = (char**) Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ) );
		for ( i = 0 ; i < nfiles ; ++i )
		{
			listCopy[i] = list[i];
		}
		listCopy[i] = NULL;

		return listCopy;
	}

	if ( !extension) {
		extension = "";
	}

	// passing a slash as extension will find directories
	if ( extension[0] == '/' && extension[1] == 0 ) {
		extension = "";
		flag = 0;
	} else {
		flag = _A_SUBDIR;
	}


	char search[MAX_OSPATH];
	Com_sprintf( search, sizeof(search), "%s\\*%s", directory, extension );

	// search
	nfiles = 0;

	findhandle = _findfirst (search, &findinfo);
	if (findhandle == -1) {
		*numfiles = 0;
		return NULL;
	}

	do {
		if ( (!wantsubs && flag ^ ( findinfo.attrib & _A_SUBDIR )) || (wantsubs && findinfo.attrib & _A_SUBDIR) ) {
			if ( nfiles == MAX_FOUND_FILES - 1 ) {
				break;
			}
			list[ nfiles ] = CopyString( findinfo.name );
			nfiles++;
		}
	} while ( _findnext (findhandle, &findinfo) != -1 );

	list[ nfiles ] = 0;

	_findclose (findhandle);

	// return a copy of the list
	*numfiles = nfiles;

	if ( !nfiles ) {
		return NULL;
	}

	listCopy = (char**) Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ) );
	for ( i = 0 ; i < nfiles ; ++i )
	{
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;

	do {
		flag = 0;
		for(i=1; i<nfiles; ++i)
		{
			if (strgtr(listCopy[i-1], listCopy[i])) {
				char *temp = listCopy[i];
				listCopy[i] = listCopy[i-1];
				listCopy[i-1] = temp;
				flag = 1;
			}
		}
	} while(flag);

	return listCopy;
}


void Sys_FreeFileList( char ** const list )
{
	if ( !list ) {
		return;
	}

	for (int i = 0 ; list[i] ; ++i )
	{
		Z_Free( list[i] );
	}

	Z_Free( list );
}



/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
static void Sys_In_Restart_f( void )
{
	IN_Shutdown();
	IN_Init();
}


/*
=================
Sys_Net_Restart_f

Restart the network subsystem
=================
*/
static void Sys_Net_Restart_f( void )
{
	NET_Restart();
}


/*
================
Sys_Init

Called after the common systems (cvars, files, etc) are initialized
================
*/

void Sys_Init( void )
{
#define OSR2_BUILD_NUMBER 1111
#define WIN98_BUILD_NUMBER 1998

	// Applications not manifested for Windows 10 return false, 
	// even if the current operating system version is Windows 10.
	/*
	
	if( IsWindows10OrGreater() )
		Com_Printf("Platform: Windows10. \n");
	else
		Com_Error(ERR_FATAL, "Direct12 not supported on your windows version. \n");
	
	*/

	int cpuid;

	// make sure the timer is high precision, otherwise
	// NT gets 18ms resolution
	// timeBeginPeriod function requests a minimum resolution for periodic timers.
	// Minimum timer resolution, in milliseconds, for the application or device driver.
	// A lower value specifies a higher (more accurate) resolution.
	timeBeginPeriod( 1 );

	Cmd_AddCommand ("in_restart", Sys_In_Restart_f);
	Cmd_AddCommand ("net_restart", Sys_Net_Restart_f);

	g_wv.osversion.dwOSVersionInfoSize = sizeof( g_wv.osversion );

	if (!GetVersionEx (&g_wv.osversion))
		Sys_Error ("Couldn't get OS info");

	if (g_wv.osversion.dwMajorVersion < 4)
		Sys_Error ("Quake3 requires Windows version 4 or greater");
	if (g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32s)
		Sys_Error ("Quake3 doesn't run on Win32s");

	if ( g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32_NT )
	{
		Cvar_Set( "arch", "winnt" );
	}
	else if ( g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
	{
		if ( LOWORD( g_wv.osversion.dwBuildNumber ) >= WIN98_BUILD_NUMBER )
		{
			Cvar_Set( "arch", "win98" );
		}
		else if ( LOWORD( g_wv.osversion.dwBuildNumber ) >= OSR2_BUILD_NUMBER )
		{
			Cvar_Set( "arch", "win95 osr2.x" );
		}
		else
		{
			Cvar_Set( "arch", "win95" );
		}
	}
	else
	{
		Cvar_Set( "arch", "unknown Windows variant" );
	}



	//
	// figure out our CPU
	//
	Cvar_Get( "sys_cpustring", "detect", 0 );
	if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring"), "detect" ) )
	{
		Com_Printf( "...detecting CPU, found " );

		cpuid = Sys_GetProcessorId();

		switch ( cpuid )
		{
		case CPUID_GENERIC:
			Cvar_Set( "sys_cpustring", "generic" );
			break;
		default:
			Com_Error( ERR_FATAL, "Unknown cpu type %d\n", cpuid );
			break;
		}
	}
	else
	{
		Com_Printf( "...forcing CPU type to " );
		if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "generic" ) )
		{
			cpuid = CPUID_GENERIC;
		}
		else
		{
			Com_Printf( "WARNING: unknown sys_cpustring '%s'\n", Cvar_VariableString( "sys_cpustring" ) );
			cpuid = CPUID_GENERIC;
		}
	}
	Cvar_SetValue( "sys_cpuid", cpuid );
	
	Com_Printf( "%s\n", Cvar_VariableString( "sys_cpustring" ) );

	Cvar_Set( "username", Sys_GetCurrentUser() );

	IN_Init();		// FIXME: not in dedicated?
#undef OSR2_BUILD_NUMBER
#undef WIN98_BUILD_NUMBER
}


//=======================================================================


int WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	char cwd[MAX_OSPATH];
	char sys_cmdline[MAX_STRING_CHARS];

	int	totalMsec = 0;
	int countMsec = 0;

    // should never get a previous instance in Win32
    if ( hPrevInstance ) {
        return 0;
	}

	g_wv.hInstance = hInstance;
	Q_strncpyz( sys_cmdline, lpCmdLine, sizeof( sys_cmdline ) );

	// done before Com/Sys_Init since we need this for error output
	Sys_CreateConsole();

	// no abort/retry/fail errors
	
	// SEM_FAILCRITICALERRORS = 0x0001 :
	// The system does not display the critical - error - handler message box. 
	// Instead, the system sends the error to the calling process.
	// Best practice is that all applications call the process - wide SetErrorMode
	// function with a parameter of SEM_FAILCRITICALERRORS at startup. 
	// This is to prevent error mode dialogs from hanging the application.

	SetErrorMode( SEM_FAILCRITICALERRORS );

	// get the initial time base
	Sys_Milliseconds();


	Com_Init( sys_cmdline );
	NET_Init();

	_getcwd (cwd, sizeof(cwd));

	Com_Printf(" Working directory: %s\n", cwd);

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
