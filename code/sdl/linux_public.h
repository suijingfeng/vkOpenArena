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


#ifndef LINUX_PUBLIC_H_
#define LINUX_PUBLIC_H_

#if !( defined __linux__ || defined __FreeBSD__ || defined __sun || defined MACOS_X )
#error You should include this file only on Linux/FreeBSD/Solaris platforms
#endif



#if defined(__linux__)

#include <xcb/xcb.h>

typedef struct xcb_windata_s {

    xcb_connection_t *connection;
    xcb_window_t hWnd;
    xcb_window_t root;
    int     screenIdx;

	int		winWidth;
	int		winHeight;

	int		desktopWidth;
	int		desktopHeight;
	int		isFullScreen;
	int		isMinimized;
	unsigned int sysMsgTime;

} WinVars_t;

#else 

// unix freebsd ... ???

#endif

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

#include "../renderercommon/tr_types.h"

void WinSys_Init(glconfig_t * const pConfig, void ** pContext);
void WinSys_Shutdown(void);
void WinSys_EndFrame(void);
void WinSys_SetGamma(unsigned char red[256], unsigned char green[256], unsigned char blue[256]);
void FileSys_Logging(char * const pComment);

// NOTE TTimo linux works with float gamma value, not the gamma table
// the params won't be used, getting the r_gamma cvar directly


#endif
