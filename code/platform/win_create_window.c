/*
** WIN_DXIMP.C
**
** This file contains ALL Win32 specific stuff having to do with the
** directx 12 refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** DXimp_EndFrame
** DXimp_Init
** DXimp_LogComment
** DXimp_Shutdown
**
*/

#include "../client/client.h"
#include "win_gamma.h"
#include "win_log.h"
#include "win_public.h"
#include "resource.h"


extern WinVars_t g_wv;


int WinSys_IsWinFullscreen(void)
{
	return g_wv.isFullScreen;
}

int WinSys_GetWinWidth(void)
{
	return g_wv.winWidth;
}

int WinSys_GetWinHeight(void)
{
	return g_wv.winHeight;
}

int WinSys_IsWinMinimized(void)
{
	return g_wv.isMinimized;
}

extern LRESULT WINAPI MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


typedef struct vidmode_s
{
	const char * description;
	int         width, height;
} vidmode_t;

static const vidmode_t r_vidModes[] =
{
	{ "Mode  0: 320x240",		320,	240},
	{ "Mode  1: 400x300",		400,	300},
	{ "Mode  2: 512x384",		512,	384},
	{ "Mode  3: 640x480",		640,	480},
	{ "Mode  4: 800x600",		800,	600},
	{ "Mode  5: 960x720",		960,	720},
	{ "Mode  6: 1024x768",		1024,	768},
	{ "Mode  7: 1152x864",		1152,	864},
	{ "Mode  8: 1280x1024",		1280,	1024},
	{ "Mode  9: 1600x1200",		1600,	1200},
	{ "Mode 10: 2048x1536",		2048,	1536},
	{ "Mode 11: 856x480 (wide)",856,	480},
	{ "Mode 12: 1280x720",		1280,	720},
	{ "Mode 13: 1280x768",		1280,	768},
	{ "Mode 14: 1280x800",		1280,	800},
	{ "Mode 15: 1280x960",		1280,	960},
	{ "Mode 16: 1360x768",		1360,	768},
	{ "Mode 17: 1366x768",		1366,	768}, // yes there are some out there on that extra 6
	{ "Mode 18: 1360x1024",		1360,	1024},
	{ "Mode 19: 1400x1050",		1400,	1050},
	{ "Mode 20: 1400x900",		1400,	900},
	{ "Mode 21: 1600x900",		1600,	900},
	{ "Mode 22: 1680x1050",		1680,	1050},
	{ "Mode 23: 1920x1080",		1920,	1080},
	{ "Mode 24: 1920x1200",		1920,	1200},
	{ "Mode 25: 1920x1440",		1920,	1440},
	{ "Mode 26: 2560x1080",		2560,	1080},
	{ "Mode 27: 2560x1600",		2560,	1600},
	{ "Mode 28: 3840x2160 (4K)",3840,	2160}
};

const static int s_numVidModes = (sizeof(r_vidModes) / sizeof(r_vidModes[0]));



// always returu a valid mode ...
int R_GetModeInfo(int * const width, int * const height, int mode, const int desktopWidth, const int desktopHeight)
{
	// corse error handle, only in wondowed mode , we need get a initial window resolution
	if (mode < -2 || mode >= s_numVidModes)
	{
		// just 640 * 480;
		*width = 640;
		*height = 480;
		return 3;
	}
	else if (mode == -2)
	{
		*width = desktopWidth;
		*height = desktopHeight;

		return -2;
	}
	else if (mode == -1)
	{
		// custom
		*width = 1280;
		*height = 720;

		return -1;
	}


	const vidmode_t * pVm = &r_vidModes[mode];
	if (pVm->width > desktopWidth || pVm->height > desktopHeight)
	{
		// even large than the destop resolution, but we are not in fullscreen mode ...
		// we just give a minial default ...
		*width = 640;
		*height = 480;
		return 3;
	}
	else
	{
		*width = pVm->width;
		*height = pVm->height;
		return mode;
	}


	return mode;
}


void R_ListDisplayMode_f( void )
{
	Com_Printf( "\n" );
	for (int i = 0; i < s_numVidModes; ++i)
	{
		Com_Printf( "%s \n", r_vidModes[i].description);
	}
	Com_Printf( "\n" );
}

void win_InitDisplayModel(void)
{
	Cmd_AddCommand("printDisplayModes", R_ListDisplayMode_f);
}

void win_EndDisplayModel(void)
{
	Cmd_RemoveCommand("printDisplayModes");
}

static int GetDesktopColorDepth(void)
{
	HDC hdc = GetDC(GetDesktopWindow());
	int value = GetDeviceCaps(hdc, BITSPIXEL);
	ReleaseDC(GetDesktopWindow(), hdc);
	return value;
}


static int GetDesktopWidth(void)
{
	HDC hdc = GetDC(GetDesktopWindow());
	int value = GetDeviceCaps(hdc, HORZRES);
	ReleaseDC(GetDesktopWindow(), hdc);
	return value;
}


static int GetDesktopHeight(void)
{
	HDC hdc = GetDC(GetDesktopWindow());
	int value = GetDeviceCaps(hdc, VERTRES);
	ReleaseDC(GetDesktopWindow(), hdc);
	return value;
}



static void win_createWindowImpl( void )
{
    const char MAIN_WINDOW_CLASS_NAME[] = { "OpenArena" };
    
	Com_Printf( " Initializing window subsystem. \n" );

	cvar_t* r_fullscreen = Cvar_Get("r_fullscreen", "0", 0);

	cvar_t* r_mode = Cvar_Get("r_mode", "3", 0);

	g_wv.desktopWidth = GetDesktopWidth();
	g_wv.desktopHeight = GetDesktopHeight();
	
    int x, y, w, h;

	Com_Printf( " Desktop Color Depth: %d \n" , GetDesktopColorDepth() );

	if ( r_fullscreen->integer )
	{
		// fullscreen 
		g_wv.winStyle = WS_POPUP | WS_VISIBLE;
		g_wv.winWidth = g_wv.desktopWidth;
		g_wv.winHeight = g_wv.desktopHeight;
		g_wv.isFullScreen = 1;
        
        x = 0;
		y = 0;
		w = g_wv.desktopWidth;
		h = g_wv.desktopHeight;

		Com_Printf(" Fullscreen mode. \n");
	}
	else
	{
		g_wv.winStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;


		int width;
		int height;
		RECT r;

		int mode = R_GetModeInfo(&width, &height, r_mode->integer, g_wv.desktopWidth, g_wv.desktopHeight);

		g_wv.winWidth = width;
		g_wv.winHeight = height;
		g_wv.isFullScreen = 0;

		r.left = 0;
		r.top = 0;
		r.right = width;
		r.bottom = height;
		// Compute window rectangle dimensions based on requested client area dimensions.
		AdjustWindowRect(&r, g_wv.winStyle, FALSE);

		x = CW_USEDEFAULT;
		y = CW_USEDEFAULT;
		w = r.right - r.left;
		h = r.bottom - r.top;

		Com_Printf(" windowed mode: %d. \n", mode);

	}


	// g_wv.hWnd = create_main_window(g_wv.winWidth, g_wv.winHeight, g_wv.isFullScreen);

	//
	// register the window class if necessary
	//

	static int isWinRegistered = 0;

	if (isWinRegistered != 1)
	{
		WNDCLASS wc;

		memset(&wc, 0, sizeof(wc));

		wc.style = 0;
		wc.lpfnWndProc = MainWndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = g_wv.hInstance;
		wc.hIcon = LoadIcon(g_wv.hInstance, MAKEINTRESOURCE(IDI_ICON1));
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(void *)COLOR_GRAYTEXT;
		wc.lpszMenuName = 0;
		wc.lpszClassName = MAIN_WINDOW_CLASS_NAME;

		if (!RegisterClass(&wc))
		{
			Com_Error(ERR_FATAL, "create main window: could not register window class");
		}

		isWinRegistered = 1;

		Com_Printf(" Window class registered. \n");
	}


	HWND hwnd = CreateWindowEx(
		0,
		MAIN_WINDOW_CLASS_NAME,
		MAIN_WINDOW_CLASS_NAME,
		// The following are the window styles. After the window has been
		// created, these styles cannot be modified, except as noted.
		g_wv.winStyle,
        x, y, w, h,
        NULL,
		NULL,
		// A handle to the instance of the module to be associated with the window
		g_wv.hInstance,
		&g_wv);

	if (!hwnd)
	{
		Com_Error(ERR_FATAL, " Couldn't create window ");
	}


	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	g_wv.hWnd = hwnd;


	// Brings the thread that created the specified window into the foreground 
	// and activates the window. Keyboard input is directed to the window, and
	// various visual cues are changed for the user. 
	// The system assigns a slightly higher priority to the thread that created
	// the foreground window than it does to other threads.
	if ( SetForegroundWindow( g_wv.hWnd ) )
		Com_Printf( " Bring window into the foreground successed. \n" );

	// Sets the keyboard focus to the specified window.
	// A handle to the window that will receive the keyboard input.
	// If this parameter is NULL, keystrokes are ignored.
	SetFocus(g_wv.hWnd);
}


static void win_destroyWindowImpl(void)
{
	Com_Printf( " Shutting down DX12 subsystem. \n");

	if (g_wv.hWnd)
	{
		Com_Printf( " Destroying window system. \n");
		
		DestroyWindow( g_wv.hWnd );

		g_wv.hWnd = NULL;
	}
}



void WinSys_Init(void **pContext, int type)
{
    
    Com_Printf( " WinSys_Init: type= %d. \n", type);
	
    win_createWindowImpl();
	
	win_InitDisplayModel();

	win_InitLoging();

	*pContext = &g_wv;
}


void WinSys_Shutdown(void)
{
	win_destroyWindowImpl();

	// For DX12 mode we still have qgl pointers initialized with placeholder values.
	// Reset them the same way as we do in opengl mode.

	win_restoreGamma();

	win_EndDisplayModel();

	win_EndLoging();
}


void WinSys_EndFrame(void)
{
	;
}
