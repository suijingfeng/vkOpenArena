#ifndef SYS_PUBLIC_H_
#define SYS_PUBLIC_H_

/*
==============================================================

	NON-PORTABLE SYSTEM SERVICES

	About 30+ functions
==============================================================
*/

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

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



//void  Sys_SetDefaultHomePath(const char *path);
const char *Sys_DefaultHomePath(void);

const char *Sys_Dirname(char *path);
const char *Sys_Basename(char *path);



void* QDECL Sys_LoadDll(const char *name, qboolean useSystemLib);

// general development dll loading for virtual machine testing
void* QDECL Sys_LoadGameDll(const char *name, intptr_t(QDECL **entryPoint)(int, ...),
	intptr_t(QDECL *systemcalls)(intptr_t, ...));

void Sys_UnloadDll(void *dllHandle);

char* Sys_GetCurrentUser(void);

void	QDECL Sys_Error(const char *error, ...) __attribute__((noreturn, format(printf, 1, 2)));
void	Sys_Quit(void) __attribute__((noreturn));


void	Sys_Print(const char *msg);
void    Sys_AnsiColorPrint(const char *msg);
// Sys_Milliseconds should only be used for profiling purposes,
// any game related timing information should come from event timestamps
int	Sys_Milliseconds(void);

qboolean Sys_RandomBytes(byte *string, int len);

// the system console is shown when a dedicated server is running
void	Sys_DisplaySystemConsole(qboolean show);

void	Sys_SetErrorText(const char *text);

void	Sys_SendPacket(int length, const void *data, netadr_t to);

qboolean	Sys_StringToAdr(const char *s, netadr_t *a, netadrtype_t family);
//Does NOT parse port numbers, only base addresses.

qboolean	Sys_IsLANAddress(netadr_t adr);
void		Sys_ShowIP(void);

FILE	*Sys_FOpen(const char *ospath, const char *mode);
qboolean Sys_Mkdir(const char *path);
FILE	*Sys_Mkfifo(const char *ospath);
char	*Sys_Cwd(void);


// Console
void CON_Shutdown(void);
void CON_Init(void);
char *CON_Input(void);
unsigned int CON_LogSize(void);

unsigned int CON_LogRead(char *out, unsigned int outSize);
void CON_Print(const char *message);
unsigned int CON_LogWrite(const char *in);


char **Sys_ListFiles(const char *directory, const char *extension, char *filter, int *numfiles, qboolean wantsubs);
void	Sys_FreeFileList(char **list);
void	Sys_Sleep(int msec);

qboolean Sys_LowPhysicalMemory(void);

void Sys_SetEnv(const char *name, const char *value);


typedef enum
{
	DR_YES = 0,
	DR_NO = 1,
	DR_OK = 0,
	DR_CANCEL = 1
} dialogResult_t;

typedef enum
{
	DT_INFO,
	DT_WARNING,
	DT_ERROR,
	DT_YES_NO,
	DT_OK_CANCEL
} dialogType_t;

dialogResult_t Sys_Dialog(dialogType_t type, const char *message, const char *title);

qboolean Sys_WritePIDFile(void);


#ifdef MACOS_X
char* Sys_DefaultAppPath(void);
char* Sys_StripAppBundle(char *pwd);
#endif

void Sys_PlatformInit(void);
void Sys_PlatformExit(void);


// signals
void Sys_InitSignal(void);
void Sys_SigHandler(int signal);


// error, log, exception, exit
void Sys_Exit(int exitCode);
void Sys_ErrorDialog(const char *error);

//
int Sys_PID(void);
qboolean Sys_PIDIsRunning(int pid);

// note that this isn't journaled...
char* Sys_GetClipboardData(void);	


// Input subsystem
void IN_Init(void);
void IN_Restart(void);

void IN_Shutdown(void);

void IN_Activate(qboolean active);
void IN_Frame(void);
void IN_MouseEvent(int mstate);

#endif