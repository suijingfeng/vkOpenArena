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
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// win_local.h: Win32-specific Quake3 header file

#ifndef WIN_PUBLIC_H_
#define WIN_PUBLIC_H_

#include <windows.h>
#include <stdbool.h>

typedef struct WinVars_s {
	
	HINSTANCE		reflib_library;		// Handle to refresh DLL 

	HWND			hWnd; // main window
	HINSTANCE		hInstance;
	int				winWidth;
	int				winHeight;

	int				desktopWidth;
	int				desktopHeight;

	bool			isFullScreen;

	bool			activeApp;
	bool			isMinimized;
	OSVERSIONINFO	osversion;

	// when we get a windows message, we store the time off so keyboard processing
	// can know the exact time of an event
	unsigned		sysMsgTime;
} WinVars_t;


/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

struct glconfig_s;

void GLimp_Init(struct glconfig_s * const pConfig, void ** pContext);
void GLimp_Shutdown(void);
void GLimp_EndFrame(void);
void GLimp_SetGamma(unsigned char red[256], unsigned char green[256], unsigned char blue[256]);
void Impl_Logging(char * const pComment);

// NOTE TTimo linux works with float gamma value, not the gamma table
// the params won't be used, getting the r_gamma cvar directly


#endif