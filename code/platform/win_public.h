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

// win_public.h: OS/window system dependent header file

#ifndef WIN_PUBLIC_H_
#define WIN_PUBLIC_H_

#if defined(_WIN32) || defined(_WIN64)

  #include <windows.h>

#elif defined(__linux__)

  #if defined(USING_XCB)
    #include <xcb/xcb.h>
  #elif defined(USING_XLIB)
    #include <X11/Xutil.h>
  #endif

#endif

typedef struct WinData_s {
#if defined(_WIN32) || defined(_WIN64)
    HWND hWnd; // main window
    HINSTANCE hInstance;
    OSVERSIONINFO osversion;
#elif defined(USDING_WAYLAND)

    struct wl_display * pDisplay;
    struct wl_surface * hWnd;

    struct wl_registry *registry;
    struct wl_compositor *compositor;

#elif defined(USING_XCB)
    xcb_connection_t *connection;
    xcb_window_t hWnd;
    xcb_window_t root;
#elif defined(USING_XLIB)
    Display* pDisplay;
    Window  hWnd;
    Window root;

    int randr_ext;
    int randr_active;
    int randr_gamma;
#endif
    unsigned int monitorCount;
    int screenIdx;
    int desktop_x;
    int desktop_y;
    int winWidth;
    int winHeight;
    int desktopWidth;
    int desktopHeight;
    int isFullScreen;
    int isMinimized;
    int isLostFocused;
    int winStyle;
    int activeApp;
    int gammaSet;

    // when we get a windows message, we store the time off so keyboard processing
    // can know the exact time of an event

    unsigned int sysMsgTime;
} WinVars_t;


// unix freebsd ... ???


/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/


void WinSys_Init(void ** pContext, int type);
void WinSys_Shutdown(void);
void WinSys_EndFrame(void);
void WinSys_SetGamma(unsigned char red[256], unsigned char green[256], unsigned char blue[256]);


int WinSys_GetWinWidth(void);
int WinSys_GetWinHeight(void);
int WinSys_IsWinFullscreen(void);
int WinSys_IsWinMinimized(void);
int WinSys_IsWinLostFocused(void);
void WinSys_UpdateFocusedStatus(int lost);
unsigned int WinSys_GetNumberOfMonitor(void);

void WinMinimize_f(void);


void FileSys_Logging(const char * const pComment);

#endif
