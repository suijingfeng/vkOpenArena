#include <stdio.h>
#include <dlfcn.h>

#include "vk_common.h"

#include "qvk.h"


#define Sys_LoadLibrary(f)      dlopen(f,RTLD_NOW)
#define Sys_UnloadLibrary(h)    dlclose(h)
#define Sys_LoadFunction(h,fn)  dlsym(h,fn)
#define Sys_LibraryError()      dlerror()

// instance of Vulkan library
static void* pVulkanLib_h = NULL;


void vk_loadLib(void)
{
    // Load Vulkan DLL.
#if defined( _WIN32 )
    const char* dll_name = "vulkan-1.dll";
#elif defined(MACOS_X)
    const char* dll_name = "what???";
#else
    const char* dll_name = "libvulkan.so.1";
#endif

    printf("Loading Library: '%s'\n", dll_name);
    
    pVulkanLib_h = Sys_LoadLibrary(dll_name);

    if (pVulkanLib_h == NULL) {
        fprintf(stderr, "Error: could not load %s\n", dll_name);
    }
    
    printf(" %s loaded. \n", dll_name);
}

void vk_unLoadLib(void)
{
    Sys_UnloadLibrary(pVulkanLib_h);
    
    printf(" Vulkan library unloaded. \n");

    pVulkanLib_h = NULL;
}


void vk_getInstanceProcAddrImpl(void)
{
    qvkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)Sys_LoadFunction(pVulkanLib_h, "vkGetInstanceProcAddr");
    if( qvkGetInstanceProcAddr == NULL)
    {
        printf("Failed to find entrypoint vkGetInstanceProcAddr\n"); 
    }
    
    printf(" Get instance proc address. (using XCB)\n");
}
