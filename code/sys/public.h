#ifndef SYS_PUBLIC_H_
#define SYS_PUBLIC_H_

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../renderercommon/tr_types.h"


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




////////////// path.c /////////////////
void Sys_SetBinaryPath(const char *path);
char* Sys_GetBinaryPath(void);
char* Sys_DefaultInstallPath(void);

void* QDECL Sys_LoadDll(const char* name, qboolean useSystemLib);
void  QDECL Sys_UnloadDll(void* dllHandle);


#endif
