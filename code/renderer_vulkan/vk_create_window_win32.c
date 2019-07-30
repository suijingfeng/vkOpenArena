#include <windows.h>
#include "VKimpl.h"
#include "tr_cvar.h"
#include "icon_oa.h"
#include "glConfig.h"
#include "ref_import.h"
#include "../vulkan/vulkan_win32.h"
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

	return GetProcAddress(g_win.vk_library_handle, "vkGetInstanceProcAddr");
}    


// With Win32, minImageExtent, maxImageExtent, and currentExtent must always equal the window size.
// The currentExtent of a Win32 surface must have both width and height greater than 0, or both of
// them 0.
// Due to above restrictions, it is only possible to create a new swapchain on this
// platform with imageExtent being equal to the current size of the window.

// The window size may become (0, 0) on this platform (e.g. when the window is
// minimized), and so a swapchain cannot be created until the size changes.
void vk_createSurfaceImpl(VkInstance hInstance, VkSurfaceKHR* const pSurface)
{
	VkWin32SurfaceCreateInfoKHR desc;
	desc.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	desc.pNext = NULL;
	desc.flags = 0;
    // hinstance and hwnd are the Win32 HINSTANCE and HWND for the window
    // to associate the surface with.

	// This function returns a module handle for the specified module 
	// if the file is mapped into the address space of the calling process.
    //
	desc.hinstance = GetModuleHandle(NULL);
	desc.hwnd = g_win.hWindow;
	VK_CHECK( vkCreateWin32SurfaceKHR(hInstance, &desc, NULL, pSurface) );
}



void vk_createWindowImpl(void)
{
    // This function set the render window's height and width.
    R_SetWinMode( r_mode->integer, GetDesktopWidth(), GetDesktopHeight() , 60 );

	// Create window.

	g_win.hWindow = create_main_window( vk_getWinWidth(), vk_getWinHeight(), r_fullscreen->integer);
	
    SetForegroundWindow(g_win.hWindow);
	SetFocus(g_win.hWindow);

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
