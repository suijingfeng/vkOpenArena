#include "VKimpl.h"

#include "vk_instance.h"
#include "tr_cvar.h"
#include "glConfig.h"
#include "ref_import.h"


extern PFN_vkGetInstanceProcAddr qvkGetInstanceProcAddr;

#if defined(_WIN32) || defined(_WIN64)

#include "../win32/win_public.h"

HINSTANCE vk_library_handle = NULL;		// Handle to refresh DLL 

#elif defined(__unix__) || defined(__linux) || defined(__linux__)

#include <dlfcn.h>

#include "../linux/linux_public.h"


void * vk_library_handle = NULL; // instance of Vulkan library

#elif defined(MACOS_X) || defined(__APPLE_CC__)

//
// macos ? what ?
//
#endif



void* vk_getInstanceProcAddrImpl(void)
{
	ri.Printf(PRINT_ALL, " Initializing Vulkan subsystem \n");
    
#if defined(_WIN32) || defined(_WIN64)

	vk_library_handle = LoadLibrary("vulkan-1.dll");

	if (vk_library_handle == NULL)
	{
		ri.Error(ERR_FATAL, " Could not loading vulkan-1.dll. \n");
	}

	ri.Printf( PRINT_ALL, "Loading vulkan DLL succeeded. \n" );

	return GetProcAddress(vk_library_handle, "vkGetInstanceProcAddr");

#elif defined(__unix__) || defined(__linux) || defined(__linux__)

	vk_library_handle = dlopen("libvulkan.so.1", RTLD_NOW);

	if (vk_library_handle == NULL)
	{
		ri.Error(ERR_FATAL, " Load libvulkan.so.1 failed. \n");
	}

	ri.Printf(PRINT_ALL, "Loading vulkan DLL succeeded. \n");

	ri.Printf(PRINT_ALL, " Get instance proc address. (using XCB)\n");

	return dlsym(vk_library_handle, "vkGetInstanceProcAddr");

#else

	// macos ?

#endif
}


void vk_cleanInstanceProcAddrImpl(void)
{
#if defined(_WIN32) || defined(_WIN64)
	FreeLibrary(vk_library_handle);
	vk_library_handle = NULL;
#elif defined(__unix__) || defined(__linux) || defined(__linux__)
	dlclose(vk_library_handle);
#else
	// macos ?
	vk_library_handle = NULL;
#endif

	ri.Printf(PRINT_ALL, " vulkan DLL freed. \n");
}

// With Win32, minImageExtent, maxImageExtent, and currentExtent must always equal the window size.
// The currentExtent of a Win32 surface must have both width and height greater than 0, or both of
// them 0.
// Due to above restrictions, it is only possible to create a new swapchain on this
// platform with imageExtent being equal to the current size of the window.

// The window size may become (0, 0) on this platform (e.g. when the window is
// minimized), and so a swapchain cannot be created until the size changes.
void vk_createSurfaceImpl(VkInstance hInstance, void * pCtx, VkSurfaceKHR* const pSurface)
{
	WinVars_t * pWinCtx = (WinVars_t*)pCtx;

#if defined(_WIN32) || defined(_WIN64)
	PFN_vkCreateWin32SurfaceKHR qvkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)
		qvkGetInstanceProcAddr( hInstance, "vkCreateWin32SurfaceKHR");

	if (qvkCreateWin32SurfaceKHR == NULL)
	{
		ri.Error(ERR_FATAL, "Failed to find entrypoint vkCreateWin32SurfaceKHR\n");
	}

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

#elif defined(__unix__) || defined(__linux) || defined(__linux__)

	PFN_vkCreateXcbSurfaceKHR qvkCreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR)
		qvkGetInstanceProcAddr(hInstance, "vkCreateXcbSurfaceKHR");

	if (qvkCreateXcbSurfaceKHR == NULL)
	{
		ri.Error(ERR_FATAL, "Failed to find entrypoint vkCreateXcbSurfaceKHR\n");
	}

	VkXcbSurfaceCreateInfoKHR createInfo;

	createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.connection = pWinCtx->connection;
	createInfo.window = pWinCtx->hWnd;
	qvkCreateXcbSurfaceKHR(hInstance, &createInfo, NULL, pSurface);

#endif

	R_SetWinMode(r_mode->integer, pWinCtx->desktopWidth, pWinCtx->desktopHeight, 60);
}
