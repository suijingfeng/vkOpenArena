#include <windows.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
//#include <VersionHelpers.h>

#include "../client/client.h"
#include "../qcommon/qcommon.h"

/*
========================================================================

LOAD/UNLOAD DLL

========================================================================
*/


/*
=================
Sys_LoadDll

Used to load a development dll instead of a virtual machine

TTimo: added some verbosity in debug
=================
*/


// fqpath param added 7/20/02 by T.Ray - Sys_LoadDll is only called in vm.c at this time
// fqpath will be empty if dll not loaded, otherwise will hold fully qualified path of dll module loaded
// fqpath buffersize must be at least MAX_QPATH+1 bytes long
void * QDECL Sys_LoadDll( const char * const name, char * fqpath, 
	intptr_t( QDECL **entryPoint )( int, ... ),
	intptr_t( QDECL *systemcalls )( intptr_t, ... ) )
{
	static int	lastWarning = 0;
	HINSTANCE	libHandle;
	void (QDECL *dllEntry)(intptr_t(QDECL *syscallptr)(intptr_t, ...));
	char* basepath;
	char* cdpath;
	char* gamedir;
	char* fn;
#ifdef NDEBUG
	int	timestamp;
	int ret;
#endif
	char	filename[MAX_QPATH];

	*fqpath = 0;		// added 7/20/02 by T.Ray

	Com_sprintf(filename, sizeof(filename), "%s.dll", name);

#ifdef NDEBUG
	timestamp = Sys_Milliseconds();
	if (((timestamp - lastWarning) > (5 * 60000)) && !Cvar_VariableIntegerValue("dedicated")
		&& !Cvar_VariableIntegerValue("com_blindlyLoadDLLs")) {
		if (FS_FileExists(filename)) {
			lastWarning = timestamp;
			ret = MessageBoxEx(NULL, "You are about to load a .DLL executable that\n"
				"has not been verified for use with Quake III Arena.\n"
				"This type of file can compromise the security of\n"
				"your computer.\n\n"
				"Select 'OK' if you choose to load it anyway.",
				"Security Warning", MB_OKCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON2 | MB_TOPMOST | MB_SETFOREGROUND,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));
			if (ret != IDOK) {
				return NULL;
			}
		}
	}
#endif

#ifndef NDEBUG
	libHandle = LoadLibrary(filename);
	if (libHandle)
		Com_Printf("LoadLibrary '%s' ok\n", filename);
	else
		Com_Printf("LoadLibrary '%s' failed\n", filename);
	if (!libHandle) {
#endif
		basepath = Cvar_VariableString("fs_basepath");
		cdpath = Cvar_VariableString("fs_cdpath");
		gamedir = Cvar_VariableString("fs_game");

		fn = FS_BuildOSPath(basepath, gamedir, filename);
		libHandle = LoadLibrary(fn);
#ifndef NDEBUG
		if (libHandle)
			Com_Printf("LoadLibrary '%s' ok\n", fn);
		else
			Com_Printf("LoadLibrary '%s' failed\n", fn);
#endif

		if (!libHandle) {
			if (cdpath[0]) {
				fn = FS_BuildOSPath(cdpath, gamedir, filename);
				libHandle = LoadLibrary(fn);
#ifndef NDEBUG
				if (libHandle)
					Com_Printf("LoadLibrary '%s' ok\n", fn);
				else
					Com_Printf("LoadLibrary '%s' failed\n", fn);
#endif
			}

			if (!libHandle) {
				return NULL;
			}
		}
#ifndef NDEBUG
	}
#endif

	dllEntry = (void (QDECL *)(intptr_t(QDECL *)(intptr_t, ...)))GetProcAddress(libHandle, "dllEntry");
	*entryPoint = (intptr_t(QDECL *)(int, ...))GetProcAddress(libHandle, "vmMain");
	if (!*entryPoint || !dllEntry) {
		FreeLibrary(libHandle);
		return NULL;
	}
	dllEntry(systemcalls);

	if (libHandle)
		Q_strncpyz(fqpath, filename, MAX_QPATH);		// added 7/20/02 by T.Ray
	return libHandle;
}


void Sys_UnloadDll(void * const dllHandle)
{
	if (!dllHandle) {
		return;
	}
	if (!FreeLibrary((HMODULE)dllHandle))
	{
		Com_Error(ERR_FATAL, "Sys_UnloadDll FreeLibrary failed");
	}
}