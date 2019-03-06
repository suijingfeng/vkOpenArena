#ifndef SYS_PUBLIC_H_
#define SYS_PUBLIC_H_

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../renderercommon/tr_types.h"


void Sys_SetBinaryPath(const char *path);
char* Sys_GetBinaryPath(void);
char* Sys_DefaultInstallPath(void);

void* QDECL Sys_LoadDll(const char* name, qboolean useSystemLib);
void  QDECL Sys_UnloadDll(void* dllHandle);


#endif
