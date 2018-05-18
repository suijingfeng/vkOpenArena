#ifndef SYS_PUBLIC_H_
#define SYS_PUBLIC_H_

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../renderer_oa/tr_types.h"


#ifdef _WIN32
    #include <windows.h>
    #define Sys_LoadLibrary(f)      (void*)LoadLibrary(f)
    #define Sys_UnloadLibrary(h)    FreeLibrary((HMODULE)h)
    #define Sys_LoadFunction(h,fn)  (void*)GetProcAddress((HMODULE)h,fn)
    #define Sys_LibraryError()      "unknown"
#else
    #include <dlfcn.h>
    #define Sys_LoadLibrary(f)      dlopen(f, RTLD_NOW | RTLD_GLOBAL)
    #define Sys_UnloadLibrary(h)    dlclose(h)
    #define Sys_LoadFunction(h,fn)  dlsym(h,fn)
    #define Sys_LibraryError()      dlerror()
#endif


typedef struct sym_s
{
	void **symbol;
	const char *name;
} sym_t;

typedef struct
{
	void *OpenGLLib; // instance of OpenGL library
	FILE *log_fp;

	int	monitorCount;

	qboolean gammaSet;

	qboolean cdsFullscreen;

	glconfig_t *config; // feedback to renderer module

	qboolean dga_ext;

	qboolean vidmode_ext;
	qboolean vidmode_active;
	qboolean vidmode_gamma;

	qboolean randr_ext;
	qboolean randr_active;
	qboolean randr_gamma;

	qboolean desktop_ok;
	int desktop_width;
	int desktop_height;
	int desktop_x;
	int desktop_y;
} glwstate_t;


////////////// path.c /////////////////
void Sys_SetBinaryPath(const char *path);
char* Sys_BinaryPath(void);
char* Sys_DefaultInstallPath(void);

void* QDECL Sys_LoadDll(const char* name, qboolean useSystemLib);
void  QDECL Sys_UnloadDll(void* dllHandle);


#endif
