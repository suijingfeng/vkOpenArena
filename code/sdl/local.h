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

#if !( defined __linux__ || defined __FreeBSD__ || defined __sun || defined MACOS_X )
#error You should include this file only on Linux/FreeBSD/Solaris platforms
#endif

#ifndef SYS_LOCAL_H_
#define SYS_LOCAL_H_


#include "public.h"

#include <X11/Xlib.h>
#include <X11/Xfuncproto.h>



#if defined(_WIN32)
#define OPENGL_DRIVER_NAME	"opengl32"
#elif defined(MACOS_X)
#define OPENGL_DRIVER_NAME	"/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib"
#endif


qboolean BuildGammaRampTable( unsigned char *red, unsigned char *green, unsigned char *blue, int gammaRampSize, unsigned short table[3][4096] );
/*
// DGA extension
qboolean DGA_Init( Display *_dpy );
void DGA_Mouse( qboolean enable );
void DGA_Done( void );
*/

// VidMode extension
qboolean VidMode_Init( void );
void VidMode_Done( void );
qboolean VidMode_SetMode( int *width, int *height, int *rate );
void VidMode_RestoreMode( void );
void VidMode_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] );
void VidMode_RestoreGamma( void );

// XRandR extension
qboolean RandR_Init( int x, int y, int w, int h );
void RandR_Done( void );
void RandR_UpdateMonitor( int x, int y, int w, int h );
qboolean RandR_SetMode( int *width, int *height, int *rate );
void RandR_RestoreMode( void );
void RandR_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] );
void RandR_RestoreGamma( void );

////////////////////////////////////////////////////////////////////////////////////////////////

// Console
void CON_Shutdown( void );
void CON_Init( void );
char* CON_Input( void );
void CON_Print( const char *message );
qboolean IsStdinATTY( void );

unsigned int CON_LogSize( void );
unsigned int CON_LogWrite( const char *in );
unsigned int CON_LogRead( char *out, unsigned int outSize );


typedef struct
{
	void *OpenGLLib; // instance of OpenGL library
	FILE *log_fp;

	int	monitorCount;

	qboolean gammaSet;

	qboolean cdsFullscreen;

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


typedef struct sym_s
{
	void **symbol;
	const char *name;
} sym_t;

#ifdef MACOS_X
char *Sys_StripAppBundle( char *pwd );
#endif

void Sys_GLimpSafeInit( void );
void Sys_GLimpInit( void );
void Sys_PlatformInit( void );
void Sys_PlatformExit( void );
void Sys_SigHandler( int signal );
void Sys_ErrorDialog( const char *error );
void Sys_AnsiColorPrint( const char *msg );
void Sys_InitSignal(void);
int Sys_PID( void );
void Sys_Exit( int exitCode );

qboolean Sys_PIDIsRunning( int pid );

////////////////// GLX /////////////////////////

#ifdef _WIN32

#define QGL_Win32_PROCS \
	GLE( HGLRC, wglCreateContext, HDC ) \
	GLE( BOOL,  wglDeleteContext ,HGLRC ) \
	GLE( HGLRC, wglGetCurrentContext, VOID ) \
	GLE( PROC,  wglGetProcAddress, LPCSTR ) \
	GLE( BOOL,  wglMakeCurrent, HDC, HGLRC )

#define QGL_Swp_PROCS \
	GLE( BOOL,	wglSwapIntervalEXT, int interval )

#else


#endif


#ifdef _WIN32
	QGL_Win32_PROCS;
//#endif
//#if ( (defined __linux__ )  || (defined __FreeBSD__ ) || (defined __sun) )
#else // assume in opposition to win32
//	QGL_LinX11_PROCS;
#endif

#undef GLE


#endif
