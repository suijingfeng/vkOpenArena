#ifndef SYS_PUBLIC_H_
#define SYS_PUBLIC_H_

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../renderercommon/tr_types.h"

// input system
void IN_Init(void);
void IN_Frame(void);
void IN_Shutdown(void);

void IN_ActivateMouse( void );
void IN_DeactivateMouse( void );
qboolean IN_MouseActive( void );

//glimp
void GLimp_Init( glconfig_t *config, qboolean context );
void GLimp_EndFrame( void );
void GLimp_Shutdown( qboolean unloadDLL );
void* GLimp_GetProcAddress(const char *symbol);


void SetGammaImpl( unsigned char red[256], unsigned char green[256], unsigned char blue[256] );
void InitGammaImpl( glconfig_t *config );



////////////// path.c /////////////////
void Sys_SetBinaryPath(const char *path);
char* Sys_GetBinaryPath(void);
char* Sys_DefaultInstallPath(void);


////////////// load lib /////////////////
void* QDECL Sys_LoadDll(const char* name, qboolean useSystemLib);
void* QDECL Sys_GetFunAddr(void* handle, const char* symbol);
void  QDECL Sys_UnloadDll(void* dllHandle);

#endif
