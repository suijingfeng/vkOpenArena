#include "tr_cvar.h"
#include "ref_import.h"
#include "vk_instance.h"

#include "../sdl/win_public.h"

extern PFN_vkGetInstanceProcAddr qvkGetInstanceProcAddr;

#if defined(_WIN32) || defined(_WIN64)



HINSTANCE vk_library_handle = NULL;		// Handle to refresh DLL 

#elif defined(__unix__) || defined(__linux) || defined(__linux__)

#include <dlfcn.h>


void * vk_library_handle = NULL; // instance of Vulkan library

#elif defined(MACOS_X) || defined(__APPLE_CC__)

//
// macos ? what ?
//
#endif



void* VK_GetInstanceProcAddrImpl(void)
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
    else
    {
	    ri.Printf(PRINT_ALL, " libvulkan.so.1 loaded. \n");
    }

	ri.Printf(PRINT_ALL, " Get instance proc address. \n");

	return dlsym(vk_library_handle, "vkGetInstanceProcAddr");

#else

	// macos ?

#endif
}


void VK_CleanInstanceProcAddrImpl(void)
{
#if defined(_WIN32) || defined(_WIN64)
	FreeLibrary(vk_library_handle);
	vk_library_handle = NULL;
#elif defined(__unix__) || defined(__linux) || defined(__linux__)
	dlclose(vk_library_handle);
	vk_library_handle = NULL;
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
void VK_CreateSurfaceImpl(VkInstance hInstance, void * pCtx, VkSurfaceKHR* const pSurface)
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
	
    VK_CHECK( qvkCreateXcbSurfaceKHR(hInstance, &createInfo, NULL, pSurface) );
	
    
    ri.Printf(PRINT_ALL, " CreateXcbSurface done. \n");

#endif

}


// Platform dependent code, not elegant :(
// doing this to avoid enable every instance extension,
// not knowing if it is necessary.
void VK_FillRequiredInstanceExtention(
	const VkExtensionProperties * const pInsExt,
	const uint32_t nInsExt,
	const char* ppInsExt[16],
    uint32_t * const nExt )
{
    uint32_t cntEnabledExt = 0;
    uint32_t i = 0;

    
    for (i = 0; i < nInsExt; ++i)
    {
        // Platform dependent stuff,
        // Enable VK_KHR_win32_surface
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        
        if( 0 == strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, pInsExt[i].extensionName) )
        {
            ppInsExt[cntEnabledExt] = pInsExt[i].extensionName;
            cntEnabledExt += 1;
            continue;
        }

#elif defined(VK_USE_PLATFORM_XCB_KHR)    
    
        if( 0 == strcmp(VK_KHR_XCB_SURFACE_EXTENSION_NAME, pInsExt[i].extensionName) )
        {
            ppInsExt[cntEnabledExt] = pInsExt[i].extensionName;
            cntEnabledExt += 1;
            continue;
        }
    
#elif defined(VK_USE_PLATFORM_XLIB_KHR) 

        // Enable VK_KHR_xlib_surface
        // TODO:xlib support
        if( 0 == strcmp(VK_KHR_XLIB_SURFACE_EXTENSION_NAME, pInsExt[i].extensionName) )
        {
            ppInsExt[cntEnabledExt] = pInsExt[i].extensionName;
            cntEnabledExt += 1;
            continue;
        }

        // Enable VK_EXT_acquire_xlib_display
        if( 0 == strcmp(VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME, pInsExt[i].extensionName) )
        {
            ppInsExt[cntEnabledExt] = pInsExt[i].extensionName;
            cntEnabledExt += 1;
            continue;
        }

#else

        // TODO: MacOS ???
        // All of the instance extention enabled, Does this reasonable ?
        for (i = 0; i < nInsExt; ++i)
        {    
            ppInsExt[i] = pInsExt[i].extensionName;
        }

#endif
        
        // must enable part ... 
        //////////////////  Enable VK_KHR_surface  //////////////////////////
        if( 0 == strcmp(VK_KHR_SURFACE_EXTENSION_NAME, pInsExt[i].extensionName) )
        {
            ppInsExt[cntEnabledExt] = pInsExt[i].extensionName;
            cntEnabledExt += 1;
            continue;
        }


        /////////////////////////////////
        // other useless common part,
        /////////////////////////////////

        // Enable VK_EXT_direct_mode_display
        // TODO: add doc why enable it, does it useful ?
        //
        if( 0 == strcmp(VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME, pInsExt[i].extensionName) )
        {
            ppInsExt[cntEnabledExt] = pInsExt[i].extensionName;
            cntEnabledExt += 1;
            continue;
        }

        // Enable VK_KHR_display
        // TODO: add doc why enable this, what's the useful ?
        if( 0 == strcmp(VK_KHR_DISPLAY_EXTENSION_NAME, pInsExt[i].extensionName) )
        {
            ppInsExt[cntEnabledExt] = pInsExt[i].extensionName;
            cntEnabledExt += 1;
            vk.isKhrDisplaySupported = VK_TRUE;
            continue;
        }
    }

#ifndef NDEBUG
        //  VK_EXT_debug_report 
    ppInsExt[cntEnabledExt] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    cntEnabledExt += 1;
#endif

    *nExt = cntEnabledExt;
}
