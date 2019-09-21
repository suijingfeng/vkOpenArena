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
// win_public.h: OS-specific Quake3 header file

#ifndef WIN_PUBLIC_H_
#define WIN_PUBLIC_H_

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>


typedef struct WinVars_s {
	
	HWND		hWnd; // main window
	HINSTANCE	hInstance;
	int		screenIdx;
	int		winWidth;
	int		winHeight;
	int		desktopWidth;
	int		desktopHeight;
	int		isFullScreen;
	int		isMinimized;
	int		winStyle;
	int		activeApp;
	OSVERSIONINFO	osversion;

	// when we get a windows message, we store the time off so keyboard processing
	// can know the exact time of an event
	unsigned	sysMsgTime;
} WinVars_t;

#else

#if defined(__linux__)

  #if defined(USING_XCB)
    #include <xcb/xcb.h>
  #elif defined(USING_XLIB)
    #include <X11/Xutil.h>
  #endif

    typedef struct WinData_s {
  #if defined(USING_XCB)
        xcb_connection_t *connection;
        xcb_window_t hWnd;
        xcb_window_t root;
  #elif defined(USING_XLIB)
        Display* pDisplay;
        Window  hWnd;
        Window root;
  #endif
        int	monitorCount;
        int	screenIdx;
        int	desktop_x;
        int	desktop_y;
        int	winWidth;
        int	winHeight;
        int	desktopWidth;
        int	desktopHeight;
        int	isFullScreen;
        int	isMinimized;
        int winStyle;
        int	activeApp;
        int	gammaSet;
        int	randr_ext;
        int	randr_active;
        int	randr_gamma;
        unsigned int sysMsgTime;
        void * hGraphicLib; // instance of OpenGL library

    } WinVars_t;

#endif

// unix freebsd ... ???

#endif

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/


void WinSys_Init(void ** pContext, int type);
void WinSys_Shutdown(void);
void WinSys_EndFrame(void);
void WinSys_SetGamma(unsigned char red[256], unsigned char green[256], unsigned char blue[256]);
void FileSys_Logging(char * const pComment);


int WinSys_GetWinWidth(void);
int WinSys_GetWinHeight(void);
int WinSys_IsWinFullscreen(void);


void WinMinimize_f(void);
// NOTE TTimo linux works with float gamma value, not the gamma table
// the params won't be used, getting the r_gamma cvar directly


#endif
