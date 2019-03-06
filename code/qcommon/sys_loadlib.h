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
#ifndef SYS_LOADLIB_H_
#define SYS_LOADLIB_H_

#include "q_shared.h"

    #ifdef _WIN32
        #include <windows.h>
        #define Sys_LoadLibrary(f)      (void*)LoadLibrary(f)
        #define Sys_UnloadLibrary(h)    FreeLibrary((HMODULE)h)
        #define Sys_LoadFunction(h,fn)  (void*)GetProcAddress((HMODULE)h,fn)
        #define Sys_LibraryError()      "unknown"
    #else
        #include <dlfcn.h>
        #define Sys_LoadLibrary(f)      dlopen(f,RTLD_NOW)
        #define Sys_UnloadLibrary(h)    dlclose(h)
        #define Sys_LoadFunction(h,fn)  dlsym(h,fn)
        #define Sys_LibraryError()      dlerror()
    #endif


void Sys_SetBinaryPath(const char *path);
char* Sys_GetBinaryPath(void);
void Sys_SetDefaultInstallPath(const char *path);
char* Sys_DefaultInstallPath(void);

#ifdef MACOS_X
char* Sys_DefaultAppPath(void);
#endif

//void  Sys_SetDefaultHomePath(const char *path);
const char *Sys_DefaultHomePath(void);

const char *Sys_Dirname( char *path );
const char *Sys_Basename( char *path );



void* QDECL Sys_LoadDll(const char *name, qboolean useSystemLib);

// general development dll loading for virtual machine testing
void* QDECL Sys_LoadGameDll( const char *name, intptr_t (QDECL **entryPoint)(int, ...),
				  intptr_t (QDECL *systemcalls)(intptr_t, ...) );

void Sys_UnloadDll( void *dllHandle );


#endif
