#include <windows.h>
#include "VKimpl.h"
#include "tr_cvar.h"
#include "icon_oa.h"
#include "glConfig.h"
#include "ref_import.h"

#define	MAIN_WINDOW_CLASS_NAME	"OpenArena"

struct WindowSystem_s
{
	HINSTANCE		vk_library_handle;		// Handle to refresh DLL 


    HWND            hWindow;

	HINSTANCE		hInstance;
	qboolean		activeApp;
	qboolean		isMinimized;
	OSVERSIONINFO	osversion;

	// when we get a windows message, we store the time off so keyboard processing
	// can know the exact time of an event
	unsigned		sysMsgTime;
};


struct WindowSystem_s g_win;


void* vk_getInstanceProcAddrImpl(void)
{
	ri.Printf(PRINT_ALL, " Initializing Vulkan subsystem \n");
    
	g_win.vk_library_handle = LoadLibrary("vulkan-1.dll");

	if (g_win.vk_library_handle == NULL)
	{
		ri.Printf(PRINT_ALL, " Loading Vulkan DLL Failed. \n");
		ri.Error(ERR_FATAL, " Could not loading %s\n", "vulkan-1.dll");
	}

	ri.Printf( PRINT_ALL, "Loading vulkan DLL succeeded. \n" );

	return GetProcAddress(vk_library_handle, "vkGetInstanceProcAddr");
}    


// With Win32, minImageExtent, maxImageExtent, and currentExtent must always equal the window size.
// The currentExtent of a Win32 surface must have both width and height greater than 0, or both of
// them 0.
// Due to above restrictions, it is only possible to create a new swapchain on this
// platform with imageExtent being equal to the current size of the window.

// The window size may become (0, 0) on this platform (e.g. when the window is
// minimized), and so a swapchain cannot be created until the size changes.
void vk_createSurfaceImpl(VkInstance hInstance, VkSurfaceKHR* const pSurface);
{
	VkWin32SurfaceCreateInfoKHR desc;
	desc.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	desc.pNext = nullptr;
	desc.flags = 0;
    // hinstance and hwnd are the Win32 HINSTANCE and HWND for the window
    // to associate the surface with.

	// This function returns a module handle for the specified module 
	// if the file is mapped into the address space of the calling process.
    //
	desc.hinstance = GetModuleHandle(nullptr);
	desc.hwnd = g_win.hWindow;
	VK_CHECK( vkCreateWin32SurfaceKHR(hInstance, &desc, NULL, pSurface) );
}


/*
====================
MainWndProc: main window procedure
====================
*/
static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_MOUSEWHEEL:
		// Windows 98/Me, Windows NT 4.0 and later - uses WM_MOUSEWHEEL
		// only relevant for non-DI input and when console is toggled in window mode
		// if console is toggled in window mode (KEYCATCH_CONSOLE) then mouse is released 
		// and DI doesn't see any mouse wheel

        ri.Printf(PRINT_ALL, " WM_MOUSEWHEEL: \n");

		if (!r_fullscreen->integer)
		{
	
			// processMouseWheelMsg(g_wv.sysMsgTime, HIWORD(wParam) / WHEEL_DELTA);
				// when an application processes the WM_MOUSEWHEEL message, it must return zero
			return 0;
		}
			// 
		break;

	case WM_CREATE:
        
        ri.Printf(PRINT_ALL, " WM_CREATE: \n");

		// g_wv.hWnd = hWnd;

		// vid_xpos = Cvar_Get ("vid_xpos", "3", CVAR_ARCHIVE);
		// vid_ypos = Cvar_Get ("vid_ypos", "22", CVAR_ARCHIVE);
		// r_fullscreen = Cvar_Get("r_fullscreen", "1", CVAR_ARCHIVE | CVAR_LATCH);

		break;

	case WM_DESTROY:
		// let sound and input know about this?
		// g_wv.hWnd = NULL;
        ri.Printf(PRINT_ALL, " WM_DESTROY: \n");
		break;

	case WM_CLOSE:
		// Cbuf_ExecuteText(EXEC_APPEND, "quit");
        ri.Printf(PRINT_ALL, " WM_CLOSE: \n");

		break;

	case WM_ACTIVATE:
	{
        /* 
		VID_AppActivate();

		int fActive = LOWORD(wParam);

		g_wv.isMinimized = (qboolean) HIWORD(wParam);

		if (fActive && !g_wv.isMinimized)
		{
			g_wv.activeApp = qtrue;
		}
		else
		{
			g_wv.activeApp = qfalse;
		}

		// minimize/restore mouse-capture on demand
		if (!g_wv.activeApp)
		{
			IN_Activate(qfalse);
		}
		else
		{
			IN_Activate(qtrue);
		}
		SNDDMA_Activate();
*/
		ri.Printf(" VID_AppActivate: \n");

	} break;

	case WM_MOVE:
	{
        ri.Printf(PRINT_ALL, " WM_MOVE: \n");
        /*
		if (!r_fullscreen->integer)
		{
			int xPos = (short)LOWORD(lParam);    // horizontal position 
			int yPos = (short)HIWORD(lParam);    // vertical position 
			
			RECT r;
			r.left = 0;
			r.top = 0;
			r.right = 1;
			r.bottom = 1;
			// Retrieves information about the specified window. The function also retrieves
			// the 32-bit (DWORD) value at the specified offset into the extra window memory. 
			// GWL_STYLE: Retrieves the window styles.
			int style = GetWindowLong(hWnd, GWL_STYLE);
			AdjustWindowRect(&r, style, FALSE);

			// Cvar_SetValue( "vid_xpos", xPos + r.left);
			// Cvar_SetValue( "vid_ypos", yPos + r.top);
			// vid_xpos->modified = qfalse;
			// vid_ypos->modified = qfalse;
			if (g_wv.activeApp)
			{
				IN_Activate(qtrue);
			}
		}
        */
	}
	break;

	// this is complicated because Win32 seems to pack multiple mouse events into
	// one update sometimes, so we always check all states and look for events
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEMOVE:
	{
		int	temp = 0;

		if (wParam & MK_LBUTTON)
			temp |= 1;

		if (wParam & MK_RBUTTON)
			temp |= 2;

		if (wParam & MK_MBUTTON)
			temp |= 4;
        ri.Printf(PRINT_ALL, " BUTTON: %d\n", temp);

		// IN_MouseEvent(temp);
	} break;

	case WM_SYSCOMMAND:
    {
		if (wParam == SC_SCREENSAVE)
			return 0;
    } break;

	case WM_SYSKEYDOWN:
		if (wParam == 13)
		{
			if (r_fullscreen)
			{
				// Cvar_SetValue("r_fullscreen", !r_fullscreen->integer);
				// Cbuf_AddText("vid_restart\n");
                ri.Cmd_ExecuteText(EXEC_APPEND, "vid_restart\n");

			}
			return 0;
		}
		// fall through
	case WM_KEYDOWN:
		// Sys_QueEvent(g_wv.sysMsgTime, SE_KEY, MapKey(lParam), qtrue, 0, NULL);
		break;

	case WM_SYSKEYUP:
	case WM_KEYUP:
		// Sys_QueEvent(g_wv.sysMsgTime, SE_KEY, MapKey(lParam), qfalse, 0, NULL);
		break;

	case WM_CHAR:
		//  Sys_QueEvent(g_wv.sysMsgTime, SE_CHAR, wParam, 0, 0, NULL);
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


HWND WINAPI create_main_window(int width, int height, qboolean fullscreen)
{
	ri.Printf(PRINT_ALL, " Creating main window. \n");

	//
	// register the window class if necessary
	//

    ri.Printf(PRINT_ALL, " main window class registered. \n");

    WNDCLASS wc;

    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC) MainWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = g_win.hInstance;
    wc.hIcon         = LoadIcon( g_win.hInstance, MAKEINTRESOURCE(IDI_ICON1) );
    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = (HBRUSH) (void *)COLOR_GRAYTEXT;
    wc.lpszMenuName  = 0;
    wc.lpszClassName = MAIN_WINDOW_CLASS_NAME;

    if ( !RegisterClass( &wc ) )
    {
        ri.Error( ERR_FATAL, "create_main_window: could not register window class" );
    }
    ri.Printf( PRINT_ALL, " Window class registered. \n" );

	//
	// compute width and height
	//
    RECT r;
	r.left = 0;
	r.top = 0;
	r.right  = width;
	r.bottom = height;

    int	stylebits;
	if ( fullscreen )
	{
		stylebits = WS_POPUP | WS_VISIBLE | WS_SYSMENU;
	}
	else
	{
		stylebits = WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_VISIBLE | WS_SYSMENU;
		AdjustWindowRect (&r, stylebits, FALSE);
	}

	int w = r.right - r.left;
	int h = r.bottom - r.top;

    int x, y;

	if ( fullscreen  )
	{
		x = 0;
		y = 0;
	}
	else
	{
		cvar_t* vid_xpos = ri.Cvar_Get ("vid_xpos", "", 0);
		cvar_t* vid_ypos = ri.Cvar_Get ("vid_ypos", "", 0);
		x = vid_xpos->integer;
		y = vid_ypos->integer;

		// adjust window coordinates if necessary 
		// so that the window is completely on screen
		if ( x < 0 )
			x = 0;
		if ( y < 0 )
			y = 0;

        int desktop_width = GetDesktopWidth();
        int desktop_height = GetDesktopHeight();

		if (w < desktop_width && h < desktop_height)
		{
			if ( x + w > desktop_width )
				x = ( desktop_width - w );
			if ( y + h > desktop_height )
				y = ( desktop_height - h );
		}
	}


	HWND hwnd = CreateWindowEx(
			0, 
			MAIN_WINDOW_CLASS_NAME,
			MAIN_WINDOW_CLASS_NAME,
			stylebits,
			x, y, w, h,
			NULL,
			NULL,
			g_win.hInstance,
			NULL);

	if (!hwnd)
	{
		ri.Error (ERR_FATAL, "create_main_window() - Couldn't create window");
	}

	ShowWindow(hwnd, SW_SHOW);

	UpdateWindow(hwnd);
	
    ri.Printf(PRINT_ALL, " Window created @%d,%d (%dx%d)\n", x, y, w, h);
    
    return hwnd;
}


void vk_createWindowImpl(void)
{
    // This function set the render window's height and width.
    R_SetWinMode( r_mode->integer, GetDesktopWidth(), GetDesktopHeight() , 60 );

	// Create window.

	g_win.hWindow = create_main_window( vk_getWinWidth(), vk_getWinHeight(), r_fullscreen->integer);
	
    SetForegroundWindow(g_wv.hWnd);
	SetFocus(g_wv.hWnd);

    // WG_CheckHardwareGamma();
}


void vk_destroyWindowImpl(void)
{
	if (g_win.hWindow)
	{
		ri.Printf(PRINT_ALL, " Destroying Vulkan window. \n");
		
        DestroyWindow(g_win.hWindow);

		g_win.hWindow = NULL;
	}
} 
