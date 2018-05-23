#include "sys_public.h"
#include "sys_local.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif


#ifdef _WIN32
    #define Sys_LibraryError() "unknown"
#else
    #define Sys_LibraryError() dlerror()
#endif


static void* Sys_LoadLibrary(const char* dllname)
{
#ifdef _WIN32
    return (void*)LoadLibrary(dllname);
#else
    return dlopen(dllname, RTLD_NOW | RTLD_GLOBAL);
#endif
}



void* Sys_GetFunAddr(void* handle, const char* fun)
{
#ifdef _WIN32
    return GetProcAddress((HMODULE)handle, fun);
#else
    return dlsym(handle, fun);
#endif
}



/*
=================
Sys_DllExtension

Check if filename should be allowed to be loaded as a DLL.
=================
*/
int Sys_DllExtension(const char* name)
{
	if ( COM_CompareExtension( name, DLL_EXT ) )
    {
		return 1;
	}

#ifdef __APPLE__
	// Allow system frameworks without dylib extensions
	// i.e., /System/Library/Frameworks/OpenAL.framework/OpenAL
	if ( strncmp( name, "/System/Library/Frameworks/", 27 ) == 0 )
    {
		return qtrue;
	}
#endif

	// Check for format of filename.so.1.2.3
	const char* p = strstr( name, DLL_EXT "." );
	char c = 0;

	if ( p ) {
		p += strlen( DLL_EXT );

		// Check if .so is only followed for periods and numbers.
		while ( *p ) {
			c = *p;

			if ( !isdigit( c ) && c != '.' ) {
				return qfalse;
			}

			p++;
		}

		// Don't allow filename to end in a period. file.so., file.so.0., etc
		if ( c != '.' ) {
			return 1;
		}
	}

	return 0;
}



void Sys_UnloadDll( void *dllHandle )
{
	if( !dllHandle )
	{
		printf("Sys_UnloadDll(NULL)\n");
		return;
	}

#ifdef _WIN32
    FreeLibrary((HMODULE)dllHandle);
#else
    dlclose(dllHandle);
#endif
}

/*
=================
Sys_LoadDll

First try to load library name from system library path,
from executable path, then fs_basepath.
=================
*/

void* Sys_LoadDll(const char *name, qboolean useSystemLib)
{
	void* dllhandle = NULL;

    if(Sys_DllExtension(name) == 0)
	{
		fprintf(stderr, "Refusing to attempt to load library \"%s\": Extension not allowed.\n", name);
		return NULL;
	}
	
	if(useSystemLib)
    {
        printf(" Trying to load \"%s\"...\n", name);
        dllhandle = Sys_LoadLibrary(name);
    }


	if(dllhandle == NULL)
    {
		const char *topDir = Sys_GetBinaryPath();
		char libPath[256];

		if(!*topDir)
			topDir = ".";

		printf(" Trying to load \"%s\" from \"%s\"...\n", name, topDir);
		snprintf(libPath, sizeof(libPath), "%s%c%s", topDir, PATH_SEP, name);
        dllhandle = Sys_LoadLibrary(libPath);

		if(dllhandle == NULL)
		{
			const char *basePath = Cvar_VariableString("fs_basepath");
			
			if(!basePath || !*basePath)
				basePath = ".";
			
			if(FS_FilenameCompare(topDir, basePath))
			{
				printf("Trying to load \"%s\" from \"%s\"...\n", name, basePath);
				snprintf(libPath, sizeof(libPath), "%s%c%s", basePath, PATH_SEP, name);
				dllhandle = Sys_LoadLibrary(libPath);
			}
			
			if(dllhandle == NULL)
				printf("Loading \"%s\" failed, %s\n", name, Sys_LibraryError());
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
void* Sys_LoadGameDll(const char* name,	intptr_t (QDECL **entryPoint)(int, ...), intptr_t (*systemcalls)(intptr_t, ...))
{

	assert(name);
    if(Sys_DllExtension(name) == 0)
	{
		fprintf(stderr, "Refusing to attempt to load library \"%s\": Extension not allowed.\n", name);
		return NULL;
	}
	

    printf(" Trying to load \"%s\"...\n", name);
    void* libHandle = Sys_LoadLibrary(name);

	if(libHandle == NULL)
	{
		printf("Sys_LoadGameDll(%s) failed:\n\"%s\"\n", name, Sys_LibraryError());
		return NULL;
	}

    void ( *dllEntry )(intptr_t (*syscallptr)(intptr_t, ...));

	dllEntry = Sys_GetFunAddr( libHandle, "dllEntry" );
	*entryPoint = Sys_GetFunAddr( libHandle, "vmMain" );

	if ( !*entryPoint || !dllEntry )
	{
		printf ( "Sys_LoadGameDll(%s) failed to find vmMain function:\n\"%s\" !\n", name, Sys_LibraryError( ) );
		Sys_UnloadDll(libHandle);

		return NULL;
	}

	printf( "Sys_LoadGameDll(%s) found vmMain function at %p\n", name, *entryPoint );

	dllEntry( systemcalls );

	return libHandle;
}




