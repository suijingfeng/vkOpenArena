#include "sys_loadlib.h"
#include "qcommon.h"

///////////////////// sys path /////////////////////

static char binaryPath[ MAX_OSPATH ] = { 0 };
static char installPath[ MAX_OSPATH ] = { 0 };


void Sys_SetBinaryPath(const char *path)
{
	Q_strncpyz(binaryPath, path, sizeof(binaryPath));
}


char* Sys_GetBinaryPath(void)
{
	return binaryPath;
}


void Sys_SetDefaultInstallPath(const char *path)
{
	Q_strncpyz(installPath, path, sizeof(installPath));
}


char* Sys_DefaultInstallPath(void)
{
	if (*installPath)
		return installPath;
	else
		return Sys_Cwd();
}

/*
=================
Sys_DefaultAppPath
=================
*/
char *Sys_DefaultAppPath(void)
{
	return Sys_GetBinaryPath();
}


////////////////////////////////////////////////////

void Sys_UnloadDll( void *dllHandle )
{
	if( !dllHandle )
	{
		Com_Printf("Sys_UnloadDll(NULL)\n");
		return;
	}

	Sys_UnloadLibrary(dllHandle);
}

/*
=================
Sys_LoadDll

First try to load library name from system library path,
from executable path, then fs_basepath.
=================
*/

void *Sys_LoadDll(const char *name, qboolean useSystemLib)
{
	void *dllhandle = NULL;

	if(useSystemLib)
	{
		Com_Printf("Trying to load \"%s\"...\n", name);
		dllhandle = Sys_LoadLibrary(name);
	}
	
	if(!dllhandle)
	{
		const char *topDir;
		char libPath[MAX_OSPATH];
		int len;

		topDir = Sys_GetBinaryPath();

		if(!*topDir)
			topDir = ".";

		len = Com_sprintf(libPath, sizeof(libPath), "%s%c%s", topDir, PATH_SEP, name);
		if(len < sizeof(libPath))
		{
			Com_Printf("Trying to load \"%s\" from \"%s\"...\n", name, topDir);
			dllhandle = Sys_LoadLibrary(libPath);
		}
		else
		{
			Com_Printf("Skipping trying to load \"%s\" from \"%s\", file name is too long.\n", name, topDir);
		}

		if(!dllhandle)
		{
			const char *basePath = Cvar_VariableString("fs_basepath");
			
			if(!basePath || !*basePath)
				basePath = ".";
			
			if(FS_FilenameCompare(topDir, basePath))
			{
				len = Com_sprintf(libPath, sizeof(libPath), "%s%c%s", basePath, PATH_SEP, name);
				if(len < sizeof(libPath))
				{
					Com_Printf("Trying to load \"%s\" from \"%s\"...\n", name, basePath);
					dllhandle = Sys_LoadLibrary(libPath);
				}
				else
				{
					Com_Printf("Skipping trying to load \"%s\" from \"%s\", file name is too long.\n", name, basePath);
				}
			}
			
			if(!dllhandle)
				Com_Printf("Loading \"%s\" failed.\n", name);
            else
				Com_Printf("Loading \"%s\" succeed.\n", name);
		}
	}
	
	return dllhandle;
}



/*
=================
Sys_LoadGameDll

Used to load a development dll instead of a virtual machine
=================
*/
void *Sys_LoadGameDll(const char *name, intptr_t (QDECL **entryPoint)(int, ...), intptr_t (*systemcalls)(intptr_t, ...))
{
	void *libHandle;
	void (*dllEntry)(intptr_t (*syscallptr)(intptr_t, ...));

	assert(name);

	Com_Printf( "Loading DLL file: %s\n", name);
	libHandle = Sys_LoadLibrary(name);

	if(!libHandle)
	{
		Com_Printf("Sys_LoadGameDll(%s) failed:\n\"%s\"\n", name, Sys_LibraryError());
		return NULL;
	}

	dllEntry = Sys_LoadFunction( libHandle, "dllEntry" );
	*entryPoint = Sys_LoadFunction( libHandle, "vmMain" );

	if ( !*entryPoint || !dllEntry )
	{
		Com_Printf ( "Sys_LoadGameDll(%s) failed to find vmMain function:\n\"%s\" !\n", name, Sys_LibraryError( ) );
		Sys_UnloadLibrary(libHandle);

		return NULL;
	}

	Com_Printf ( "Sys_LoadGameDll(%s) found vmMain function at %p\n", name, *entryPoint );
	dllEntry( systemcalls );

	return libHandle;
}

