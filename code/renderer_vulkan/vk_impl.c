#include <windows.h>
#include "VKimpl.h"
#include "../vulkan/vulkan_win32.h"
#include "vk_instance.h"
#include "tr_cvar.h"
#include "icon_oa.h"
#include "glConfig.h"
#include "ref_import.h"

#include "../win32/win_public.h"

#define	MAIN_WINDOW_CLASS_NAME	"OpenArena"


extern PFN_vkGetInstanceProcAddr qvkGetInstanceProcAddr;

#if defined(_WIN32) || defined(_WIN64)

PFN_vkCreateWin32SurfaceKHR	qvkCreateWin32SurfaceKHR;

HINSTANCE vk_library_handle = NULL;		// Handle to refresh DLL 

WinVars_t * pWinCtx = NULL;

#elif defined(__unix__) || defined(__linux) || defined(__linux__)

PFN_vkCreateXcbSurfaceKHR qvkCreateXcbSurfaceKHR;

WinVars_t * pXcbCtx = NULL;

void * vk_library_handle = NULL; // instance of Vulkan library

#else

// macos ?

#endif


void* vk_getInstanceProcAddrImpl(void)
{
	ri.Printf(PRINT_ALL, " Initializing Vulkan subsystem \n");
    
	vk_library_handle = LoadLibrary("vulkan-1.dll");

	if (vk_library_handle == NULL)
	{
		ri.Printf(PRINT_ALL, " Loading Vulkan DLL Failed. \n");
		ri.Error(ERR_FATAL, " Could not loading %s\n", "vulkan-1.dll");
	}

	ri.Printf( PRINT_ALL, "Loading vulkan DLL succeeded. \n" );

	return GetProcAddress(vk_library_handle, "vkGetInstanceProcAddr");
}

void vk_cleanInstanceProcAddrImpl(void)
{
	FreeLibrary(vk_library_handle);

	vk_library_handle = NULL;

	ri.Printf(PRINT_ALL, " vulkan DLL freed. \n");
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

	// WinVars_t * pWinCtx = (WinVars_t*)pContext;

	qvkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR) 
		qvkGetInstanceProcAddr( hInstance, "vkCreateWin32SurfaceKHR");

	VkWin32SurfaceCreateInfoKHR desc;
	desc.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	desc.pNext = NULL;
	desc.flags = 0;
    // hinstance and hwnd are the Win32 HINSTANCE and HWND for the window
    // to associate the surface with.

	// This function returns a module handle for the specified module 
	// if the file is mapped into the address space of the calling process.
    //
	desc.hinstance = pWinCtx->hInstance;
	desc.hwnd = pWinCtx->hWnd;
	VK_CHECK( qvkCreateWin32SurfaceKHR(hInstance, &desc, NULL, pSurface) );
}



void vk_createWindowImpl(void)
{
	ri.Printf(PRINT_ALL, " Create window fot vulkan . \n");
    
	// This function set the render window's height and width.
    // R_SetWinMode( r_mode->integer, GetDesktopWidth(), GetDesktopHeight() , 60 );

	// Create window.
	ri.GLimpInit(glConfig_getAddressOf(), &pWinCtx);
}


void vk_destroyWindowImpl(void)
{
	ri.GLimpShutdown();
	ri.Printf(PRINT_ALL, " Destroying Vulkan window. \n");	
}


void vk_minimizeWindowImpl(void)
{
	;
}