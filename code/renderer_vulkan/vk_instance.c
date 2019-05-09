#include <string.h>
#include <stdlib.h>

#include "VKimpl.h"
#include "vk_instance.h"
#include "tr_globals.h"
#include "vk_image.h"
#include "vk_instance.h"
#include "vk_shade_geometry.h"
#include "vk_pipelines.h"
#include "vk_frame.h"
#include "vk_shaders.h"
#include "ref_import.h" 


struct Vk_Instance vk;

//
// Vulkan API functions used by the renderer.
//
PFN_vkGetInstanceProcAddr						qvkGetInstanceProcAddr;

// Global Level
PFN_vkCreateInstance							qvkCreateInstance;
PFN_vkEnumerateInstanceExtensionProperties		qvkEnumerateInstanceExtensionProperties;
PFN_vkEnumerateInstanceLayerProperties          qvkEnumerateInstanceLayerProperties;


// Instance Level
PFN_vkCreateDevice								qvkCreateDevice;
PFN_vkDestroyInstance							qvkDestroyInstance;
PFN_vkEnumerateDeviceExtensionProperties		qvkEnumerateDeviceExtensionProperties;
PFN_vkEnumeratePhysicalDevices					qvkEnumeratePhysicalDevices;
PFN_vkGetDeviceProcAddr							qvkGetDeviceProcAddr;
PFN_vkGetPhysicalDeviceFeatures					qvkGetPhysicalDeviceFeatures;
PFN_vkGetPhysicalDeviceFormatProperties			qvkGetPhysicalDeviceFormatProperties;
PFN_vkGetPhysicalDeviceMemoryProperties			qvkGetPhysicalDeviceMemoryProperties;
PFN_vkGetPhysicalDeviceProperties				qvkGetPhysicalDeviceProperties;
PFN_vkGetPhysicalDeviceQueueFamilyProperties	qvkGetPhysicalDeviceQueueFamilyProperties;


PFN_vkDestroySurfaceKHR							qvkDestroySurfaceKHR;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR	qvkGetPhysicalDeviceSurfaceCapabilitiesKHR;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR		qvkGetPhysicalDeviceSurfaceFormatsKHR;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR	qvkGetPhysicalDeviceSurfacePresentModesKHR;
PFN_vkGetPhysicalDeviceSurfaceSupportKHR		qvkGetPhysicalDeviceSurfaceSupportKHR;

// VK_KHR_display
PFN_vkGetPhysicalDeviceDisplayPropertiesKHR         qvkGetPhysicalDeviceDisplayPropertiesKHR;
PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR    qvkGetPhysicalDeviceDisplayPlanePropertiesKHR;
PFN_vkGetDisplayPlaneSupportedDisplaysKHR           qvkGetDisplayPlaneSupportedDisplaysKHR;
PFN_vkGetDisplayModePropertiesKHR                   qvkGetDisplayModePropertiesKHR;
PFN_vkCreateDisplayModeKHR                          qvkCreateDisplayModeKHR;
PFN_vkGetDisplayPlaneCapabilitiesKHR                qvkGetDisplayPlaneCapabilitiesKHR;
PFN_vkCreateDisplayPlaneSurfaceKHR                  qvkCreateDisplayPlaneSurfaceKHR;

#ifndef NDEBUG
PFN_vkCreateDebugReportCallbackEXT				qvkCreateDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT				qvkDestroyDebugReportCallbackEXT;
#endif


// Device Level
PFN_vkAllocateCommandBuffers					qvkAllocateCommandBuffers;
PFN_vkAllocateDescriptorSets					qvkAllocateDescriptorSets;
PFN_vkAllocateMemory							qvkAllocateMemory;
PFN_vkBeginCommandBuffer						qvkBeginCommandBuffer;
PFN_vkBindBufferMemory							qvkBindBufferMemory;
PFN_vkBindImageMemory							qvkBindImageMemory;
PFN_vkCmdBeginRenderPass						qvkCmdBeginRenderPass;
PFN_vkCmdBindDescriptorSets						qvkCmdBindDescriptorSets;
PFN_vkCmdBindIndexBuffer						qvkCmdBindIndexBuffer;
PFN_vkCmdBindPipeline							qvkCmdBindPipeline;
PFN_vkCmdBindVertexBuffers						qvkCmdBindVertexBuffers;
PFN_vkCmdBlitImage								qvkCmdBlitImage;
PFN_vkCmdClearAttachments						qvkCmdClearAttachments;
PFN_vkCmdCopyBufferToImage						qvkCmdCopyBufferToImage;
PFN_vkCmdCopyImage								qvkCmdCopyImage;
PFN_vkCmdCopyImageToBuffer                      qvkCmdCopyImageToBuffer;
PFN_vkCmdDraw									qvkCmdDraw;
PFN_vkCmdDrawIndexed							qvkCmdDrawIndexed;
PFN_vkCmdEndRenderPass							qvkCmdEndRenderPass;
PFN_vkCmdPipelineBarrier						qvkCmdPipelineBarrier;
PFN_vkCmdPushConstants							qvkCmdPushConstants;
PFN_vkCmdSetDepthBias							qvkCmdSetDepthBias;
PFN_vkCmdSetScissor								qvkCmdSetScissor;
PFN_vkCmdSetViewport							qvkCmdSetViewport;
PFN_vkCreateBuffer								qvkCreateBuffer;
PFN_vkCreateCommandPool							qvkCreateCommandPool;
PFN_vkCreateDescriptorPool						qvkCreateDescriptorPool;
PFN_vkCreateDescriptorSetLayout					qvkCreateDescriptorSetLayout;
PFN_vkCreateFence								qvkCreateFence;
PFN_vkCreateFramebuffer							qvkCreateFramebuffer;
PFN_vkCreateGraphicsPipelines					qvkCreateGraphicsPipelines;
PFN_vkCreateImage								qvkCreateImage;
PFN_vkCreateImageView							qvkCreateImageView;
PFN_vkCreatePipelineLayout						qvkCreatePipelineLayout;
PFN_vkCreateRenderPass							qvkCreateRenderPass;
PFN_vkCreateSampler								qvkCreateSampler;
PFN_vkCreateSemaphore							qvkCreateSemaphore;
PFN_vkCreateShaderModule						qvkCreateShaderModule;
PFN_vkDestroyBuffer								qvkDestroyBuffer;
PFN_vkDestroyCommandPool						qvkDestroyCommandPool;
PFN_vkDestroyDescriptorPool						qvkDestroyDescriptorPool;
PFN_vkDestroyDescriptorSetLayout				qvkDestroyDescriptorSetLayout;
PFN_vkDestroyDevice								qvkDestroyDevice;
PFN_vkDestroyFence								qvkDestroyFence;
PFN_vkDestroyFramebuffer						qvkDestroyFramebuffer;
PFN_vkDestroyImage								qvkDestroyImage;
PFN_vkDestroyImageView							qvkDestroyImageView;
PFN_vkDestroyPipeline							qvkDestroyPipeline;
PFN_vkDestroyPipelineLayout						qvkDestroyPipelineLayout;
PFN_vkDestroyRenderPass							qvkDestroyRenderPass;
PFN_vkDestroySampler							qvkDestroySampler;
PFN_vkDestroySemaphore							qvkDestroySemaphore;
PFN_vkDestroyShaderModule						qvkDestroyShaderModule;
PFN_vkDeviceWaitIdle							qvkDeviceWaitIdle;
PFN_vkEndCommandBuffer							qvkEndCommandBuffer;
PFN_vkFreeCommandBuffers						qvkFreeCommandBuffers;
PFN_vkFreeDescriptorSets						qvkFreeDescriptorSets;
PFN_vkFreeMemory								qvkFreeMemory;
PFN_vkGetBufferMemoryRequirements				qvkGetBufferMemoryRequirements;
PFN_vkGetDeviceQueue							qvkGetDeviceQueue;
PFN_vkGetImageMemoryRequirements				qvkGetImageMemoryRequirements;
PFN_vkGetImageSubresourceLayout					qvkGetImageSubresourceLayout;
PFN_vkMapMemory									qvkMapMemory;
PFN_vkUnmapMemory                               qvkUnmapMemory;
PFN_vkQueueSubmit								qvkQueueSubmit;
PFN_vkQueueWaitIdle								qvkQueueWaitIdle;
PFN_vkResetDescriptorPool						qvkResetDescriptorPool;
PFN_vkResetFences								qvkResetFences;
PFN_vkUpdateDescriptorSets						qvkUpdateDescriptorSets;
PFN_vkWaitForFences								qvkWaitForFences;
PFN_vkAcquireNextImageKHR						qvkAcquireNextImageKHR;
PFN_vkCreateSwapchainKHR						qvkCreateSwapchainKHR;
PFN_vkDestroySwapchainKHR						qvkDestroySwapchainKHR;
PFN_vkGetSwapchainImagesKHR						qvkGetSwapchainImagesKHR;
PFN_vkQueuePresentKHR							qvkQueuePresentKHR;



#ifndef NDEBUG

VKAPI_ATTR VkBool32 VKAPI_CALL vk_DebugCallback(
        VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT object_type,
        uint64_t object, size_t location, int32_t message_code, 
        const char* layer_prefix, const char* message, void* user_data )
{

    switch(flags)
    {
        case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
            ri.Printf(PRINT_ALL, "INFORMATION: %s\n", message);
            break;
        case VK_DEBUG_REPORT_WARNING_BIT_EXT:
            ri.Printf(PRINT_WARNING, "WARNING: %s\n", message);
            break;
        case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
            ri.Printf(PRINT_WARNING, "PERFORMANCE: %s\n", message);
            break;
        case VK_DEBUG_REPORT_ERROR_BIT_EXT:
            ri.Printf(PRINT_WARNING, "ERROR: %s\n", message);
            break;
        case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
            ri.Printf(PRINT_WARNING, "DEBUG: %s\n", message);
            break;
    }
	return VK_FALSE;
}


static void vk_createDebugCallback( PFN_vkDebugReportCallbackEXT qvkDebugCB)
{
    ri.Printf( PRINT_ALL, " vk_createDebugCallback() \n" ); 
    
    VkDebugReportCallbackCreateInfoEXT desc;
    desc.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    desc.pNext = NULL;
    desc.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_ERROR_BIT_EXT;
    desc.pfnCallback = qvkDebugCB;
    desc.pUserData = NULL;

    VK_CHECK(qvkCreateDebugReportCallbackEXT(vk.instance, &desc, NULL, &vk.h_debugCB));
}

#endif


static void vk_assertStandValidationLayer(void)
{
    // Look For Standard Validation Layer
    VkBool32 found = VK_FALSE;
    
    static const char instance_validation_layers_name[] = {"VK_LAYER_LUNARG_standard_validation"};

    uint32_t instance_layer_count = 0;

    VK_CHECK( qvkEnumerateInstanceLayerProperties(&instance_layer_count, NULL) );

    if (instance_layer_count > 0)
    {

        VkLayerProperties * instance_layers = (VkLayerProperties *) 
            malloc( sizeof(VkLayerProperties) * instance_layer_count );

        VK_CHECK( qvkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layers) );

        ri.Printf(PRINT_ALL, " ------- %d instance layer available -------- \n", instance_layer_count);
        uint32_t j;
        for (j = 0; j < instance_layer_count; ++j)
        {
            ri.Printf(PRINT_ALL, " %s\n", instance_layers[j].layerName);
        }

        for (j = 0; j < instance_layer_count; ++j)
        {
            if (!strcmp(instance_validation_layers_name, instance_layers[j].layerName))
            {
                found = VK_TRUE;
                ri.Printf(PRINT_ALL, " Standard validation found. \n");
                break;
            }
        }

        free(instance_layers);
        
        if(found == 0) {
            ri.Printf(PRINT_WARNING, " Failed to find required validation layer.\n\n");
        }
    }
    else
    {
        ri.Printf(PRINT_WARNING, "No instance layer available! \n");
    }
}


//// Platform dependent code, not elegant :(
//
static void vk_fillRequiredInstanceExtention( 
        const VkExtensionProperties * const pInsExt, const uint32_t nInsExt, 
        const char ** const ppInsExt, uint32_t * nExt )
{
    uint32_t enExtCnt = 0;
    uint32_t i = 0;
#if defined(_WIN32)

    #ifndef VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    #define VK_KHR_WIN32_SURFACE_EXTENSION_NAME "VK_KHR_win32_surface"
    #endif
    
    for (i = 0; i < nInsExt; ++i)
    {
        // Platform dependent stuff,
        // Enable VK_KHR_win32_surface
        if( 0 == strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, pInsExt[i].extensionName) )
        {
            ppInsExt[enExtCnt] = pInsExt[i].extensionName;
            enExtCnt += 1;
            continue;
        }

        // common part

        // Enable VK_EXT_direct_mode_display
        // TODO: add doc why enable this, what's the useful ?
        if( 0 == strcmp(VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME, pInsExt[i].extensionName) )
        {
            ppInsExt[enExtCnt] = pInsExt[i].extensionName;
            enExtCnt += 1;
            continue;
        }

        // Enable VK_EXT_display_surface_counter
        // TODO: add doc why enable this, what's the useful ?
        if( 0 == strcmp(VK_EXT_DISPLAY_SURFACE_COUNTER_EXTENSION_NAME, pInsExt[i].extensionName) )
        {
            ppInsExt[enExtCnt] = pInsExt[i].extensionName;
            enExtCnt += 1;
            continue;
        }

        // Enable VK_KHR_display
        // TODO: add doc why enable this, what's the useful ?
        if( 0 == strcmp(VK_KHR_DISPLAY_EXTENSION_NAME, pInsExt[i].extensionName) )
        {
            ppInsExt[enExtCnt] = pInsExt[i].extensionName;
            enExtCnt += 1;
            vk.isKhrDisplaySupported = VK_TRUE;
            continue;
        }

        // Enable VK_KHR_surface
        if( 0 == strcmp(VK_KHR_SURFACE_EXTENSION_NAME, pInsExt[i].extensionName) )
        {
            ppInsExt[enExtCnt] = pInsExt[i].extensionName;
            enExtCnt += 1;
            continue;
        }
    }

    #ifndef NDEBUG
    //  VK_EXT_debug_report 
    ppInsExt[enExtCnt] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    enExtCnt += 1;
    #endif

#elif defined(__unix__) || defined(__linux) || defined(__linux__)

    #ifndef VK_KHR_XCB_SURFACE_EXTENSION_NAME
    #define VK_KHR_XCB_SURFACE_EXTENSION_NAME "VK_KHR_xcb_surface"
    #endif

    #ifndef VK_KHR_XLIB_SURFACE_EXTENSION_NAME
    #define VK_KHR_XLIB_SURFACE_EXTENSION_NAME "VK_KHR_xlib_surface"
    #endif

    #ifndef VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME
    #define VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME "VK_EXT_acquire_xlib_display"
    #endif 

    for (i = 0; i < nInsExt; ++i)
    {

        // Platform dependent stuff,
        // Enable VK_KHR_xcb_surface
        // TODO: How can i force SDL2 using XCB instead XLIB ???
        if( 0 == strcmp(VK_KHR_XCB_SURFACE_EXTENSION_NAME, pInsExt[i].extensionName) )
        {
            ppInsExt[enExtCnt] = pInsExt[i].extensionName;
            enExtCnt += 1;
            continue;
        }

        // Enable VK_KHR_xlib_surface
        if( 0 == strcmp(VK_KHR_XLIB_SURFACE_EXTENSION_NAME, pInsExt[i].extensionName) )
        {
            ppInsExt[enExtCnt] = pInsExt[i].extensionName;
            enExtCnt += 1;
            continue;
        }

        // Enable VK_EXT_acquire_xlib_display
        if( 0 == strcmp(VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME, pInsExt[i].extensionName) )
        {
            ppInsExt[enExtCnt] = pInsExt[i].extensionName;
            enExtCnt += 1;
            continue;
        }


        // common part
        // Enable VK_EXT_direct_mode_display
        // TODO: add doc why enable this, what's the useful ?
        if( 0 == strcmp(VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME, pInsExt[i].extensionName) )
        {
            ppInsExt[enExtCnt] = pInsExt[i].extensionName;
            enExtCnt += 1;
            continue;
        }

        // Enable VK_EXT_display_surface_counter
        // TODO: add doc why enable this, what's the useful ?
        if( 0 == strcmp(VK_EXT_DISPLAY_SURFACE_COUNTER_EXTENSION_NAME, pInsExt[i].extensionName) )
        {
            ppInsExt[enExtCnt] = pInsExt[i].extensionName;
            enExtCnt += 1;
            continue;
        }

        // Enable VK_KHR_display
        // TODO: add doc why enable this, what's the useful ?
        if( 0 == strcmp(VK_KHR_DISPLAY_EXTENSION_NAME, pInsExt[i].extensionName) )
        {
            ppInsExt[enExtCnt] = pInsExt[i].extensionName;
            enExtCnt += 1;
            vk.isKhrDisplaySupported = VK_TRUE;
            continue;
        }

        // Enable VK_KHR_surface
        if( 0 == strcmp(VK_KHR_SURFACE_EXTENSION_NAME, pInsExt[i].extensionName) )
        {
            ppInsExt[enExtCnt] = pInsExt[i].extensionName;
            enExtCnt += 1;
            continue;
        }
    }

    #ifndef NDEBUG
    //  VK_EXT_debug_report 
    ppInsExt[enExtCnt] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    enExtCnt += 1;
    #endif

#else
    // TODO: CHECK OUT
    // All of the instance extention enabled, Does this reasonable ?
    for (i = 0; i < nInsExt; ++i)
    {    
        ppInsExt[i] = pInsExt[i].extensionName;
    }
#endif

    *nExt = enExtCnt;
}


static void vk_createInstance(void)
{
    // There is no global state in Vulkan and all per-application state
    // is stored in a VkInstance object. Creating a VkInstance object 
    // initializes the Vulkan library and allows the application to pass
    // information about itself to the implementation.
    ri.Printf(PRINT_ALL, " Creating instance: vk.instance\n");
	
    // The version of Vulkan that is supported by an instance may be 
    // different than the version of Vulkan supported by a device or
    // physical device. Because Vulkan 1.0 implementations may fail
    // with VK_ERROR_INCOMPATIBLE_DRIVER, applications should determine
    // the version of Vulkan available before calling vkCreateInstance.
    // If the vkGetInstanceProcAddr returns NULL for vkEnumerateInstanceVersion,
    // it is a Vulkan 1.0 implementation. Otherwise, the application can
    // call vkEnumerateInstanceVersion to determine the version of Vulkan.

    VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = NULL;
	appInfo.pApplicationName = "OpenArena";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "VulKan Arena";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    // apiVersion must be the highest version of Vulkan that the
    // application is designed to use, encoded as described in the
    // API Version Numbers and Semantics section. The patch version
    // number specified in apiVersion is ignored when creating an 
    // instance object. Only the major and minor versions of the 
    // instance must match those requested in apiVersion.
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);


	VkInstanceCreateInfo instanceCreateInfo;
	
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    // pNext is NULL or a pointer to an extension-specific structure.
	instanceCreateInfo.pNext = NULL;
    // flags is reserved for future use.
    instanceCreateInfo.flags = 0;
    // pApplicationInfo is NULL or a pointer to an instance of
    // VkApplicationInfo. If not NULL, this information helps 
    // implementations recognize behavior inherent to classes
    // of applications.
    instanceCreateInfo.pApplicationInfo = &appInfo;

    
	// check extensions availability
    //
    // Extensions may define new Vulkan commands, structures, and enumerants.
    // For compilation purposes, the interfaces defined by registered extensions,
    // including new structures and enumerants as well as function pointer types
    // for new commands, are defined in the Khronos-supplied vulkan_core.h 
    // together with the core API. However, commands defined by extensions may
    // not be available for static linking - in which case function pointers to
    // these commands should be queried at runtime as described in Command Function
    // Pointers.
    // Extensions may be provided by layers as well as by a Vulkan implementation.
    //
    // check extensions availability
	uint32_t nInsExt = 0;
    uint32_t i = 0;
	uint32_t EnableInsExtCount = 0;
    // To retrieve a list of supported extensions before creating an instance
	VK_CHECK( qvkEnumerateInstanceExtensionProperties( NULL, &nInsExt, NULL) );

    assert(nInsExt > 0);

    VkExtensionProperties * pInsExt = (VkExtensionProperties *) malloc(sizeof(VkExtensionProperties) * nInsExt);
    VK_CHECK(qvkEnumerateInstanceExtensionProperties( NULL, &nInsExt, pInsExt));


    const char** const ppInstanceExtEnabled = malloc( sizeof(char *) * (nInsExt) );

    // Each platform-specific extension is an instance extension.
    // The application must enable instance extensions with vkCreateInstance
    // before using them.
    vk_fillRequiredInstanceExtention(pInsExt, nInsExt, ppInstanceExtEnabled, &EnableInsExtCount);

    ri.Printf(PRINT_ALL, "\n -------- Total %d instance extensions. -------- \n", nInsExt);
    for (i = 0; i < nInsExt; ++i)
    {    
        ri.Printf(PRINT_ALL, " %s \n", pInsExt[i].extensionName);
    }

    ri.Printf(PRINT_ALL, "\n -------- %d instance extensions Enable. -------- \n", EnableInsExtCount);
    for (i = 0; i < EnableInsExtCount; ++i)
    {    
        ri.Printf(PRINT_ALL, " %s \n", ppInstanceExtEnabled[i]);
    }
    ri.Printf(PRINT_ALL, " -------- ----------------------------- -------- \n\n");


    instanceCreateInfo.enabledExtensionCount = EnableInsExtCount;
	instanceCreateInfo.ppEnabledExtensionNames = ppInstanceExtEnabled;

#ifndef NDEBUG
    ri.Printf(PRINT_ALL, "Using VK_LAYER_LUNARG_standard_validation\n");

    const char* const validation_layer_name = "VK_LAYER_LUNARG_standard_validation";    
    instanceCreateInfo.enabledLayerCount = 1;
	instanceCreateInfo.ppEnabledLayerNames = &validation_layer_name;
#else
    instanceCreateInfo.enabledLayerCount = 0;
	instanceCreateInfo.ppEnabledLayerNames = NULL;
#endif


    VkResult e = qvkCreateInstance(&instanceCreateInfo, NULL, &vk.instance);
    if(e == VK_SUCCESS)
    {
        ri.Printf(PRINT_ALL, "--- Vulkan create instance success! ---\n\n");
    }
    else if (e == VK_ERROR_INCOMPATIBLE_DRIVER)
	{
		// The requested version of Vulkan is not supported by the driver 
		// or is otherwise incompatible for implementation-specific reasons.
        ri.Error(ERR_FATAL, 
            "The requested version of Vulkan is not supported by the driver.\n" );
    }
    else if (e == VK_ERROR_EXTENSION_NOT_PRESENT)
    {
        ri.Error(ERR_FATAL, "Cannot find a specified extension library.\n");
    }
    else 
    {
        ri.Error(ERR_FATAL, "%d, returned by qvkCreateInstance.\n", e);
    }
   
    free(ppInstanceExtEnabled);

    free(pInsExt);
}

// Vulkan functions can be divided into three levels, 
// which are global, instance, and device. 
// 
// Device-level functions are used to perform typical operations
// such as drawing, shader-modules creation, image creation, or data copying.
//
// Instance-level functions allow us to create logical devices. 
// To do all this, and to load device and instance-level functions,
// we need to create an Instance. 
//
// This operation is performed with global-level functions, 
// which we need to load first.

static void vk_loadGlobalLevelFunctions(void)
{
    ri.Printf(PRINT_ALL, " Loading vulkan instance functions \n");

    // In Vulkan, there are only three global-level functions:
    // vkEnumerateInstanceExtensionProperties(), 
    // vkEnumerateInstanceLayerProperties(), and
    // vkCreateInstance(). 
    //
    // They are used during Instance creation to check, 
    // what instance-level extensions and layers are available
    // and to create the Instance itself.

    // qvkGetInstanceProcAddr 
    vk_getInstanceProcAddrImpl();


#define INIT_GLOBAL_LEVEL_FUNCTION( func )                          \
    q##func = (PFN_##func) qvkGetInstanceProcAddr( NULL, #func );   \
    if( q##func == NULL ) {                                         \
        ri.Error(ERR_FATAL, "Failed to find entrypoint %s", #func); \
}
	INIT_GLOBAL_LEVEL_FUNCTION(vkCreateInstance)
	INIT_GLOBAL_LEVEL_FUNCTION(vkEnumerateInstanceExtensionProperties)
    // This embarrassing, i get NULL if loding this fun after create instance
    // on ubuntu 16.04, 1.0.49
    INIT_GLOBAL_LEVEL_FUNCTION(vkEnumerateInstanceLayerProperties)

#undef INIT_GLOBAL_LEVEL_FUNCTION
}



static void vk_loadInstanceLevelFunctions(void)
{

    ri.Printf(PRINT_ALL, " Loading Instance level functions. \n");

    vk_createInstance();

    // Loading instance-level functions

    // We have created a Vulkan Instance object. 
    // The next step is to enumerate physical devices, 
    // choose one of them, and create a logical device from it.
    // These operations are performed with instance-level functions,
    // of which we need to acquire the addresses.

    // Instance-level functions are used mainly for operations on
    // physical devices. There are multiple instance-level functions.
    
#define INIT_INSTANCE_FUNCTION(func)                                    \
    q##func = (PFN_##func)qvkGetInstanceProcAddr(vk.instance, #func);   \
    if (q##func == NULL) {                                              \
        ri.Error(ERR_FATAL, "Failed to find entrypoint %s", #func);     \
    }

	INIT_INSTANCE_FUNCTION(vkCreateDevice)
	INIT_INSTANCE_FUNCTION(vkDestroyInstance)
    INIT_INSTANCE_FUNCTION(vkDestroySurfaceKHR)
	INIT_INSTANCE_FUNCTION(vkEnumerateDeviceExtensionProperties)

	INIT_INSTANCE_FUNCTION(vkEnumeratePhysicalDevices)
	INIT_INSTANCE_FUNCTION(vkGetDeviceProcAddr)

    INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceFeatures)
    INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceProperties)
	INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceFormatProperties)
	INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceMemoryProperties)
	INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties)

    INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
	INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceFormatsKHR)
	INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfacePresentModesKHR)
	INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceSupportKHR)


    // The platform-specific extensions allow a VkSurface object to be
    // created that represents a native window owned by the operating
    // system or window system. These extensions are typically used to
    // render into a window with no border that covers an entire display
    // it is more often efficient to render directly to a display instead.
    if(vk.isKhrDisplaySupported)
    {
        ri.Printf(PRINT_ALL, " VK_KHR_Display Supported, Loading associate functions for this instance extention. \n");

        INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceDisplayPropertiesKHR);
        INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceDisplayPlanePropertiesKHR);
        INIT_INSTANCE_FUNCTION(vkGetDisplayPlaneSupportedDisplaysKHR);
        INIT_INSTANCE_FUNCTION(vkGetDisplayModePropertiesKHR);
        INIT_INSTANCE_FUNCTION(vkCreateDisplayModeKHR);
        INIT_INSTANCE_FUNCTION(vkGetDisplayPlaneCapabilitiesKHR);
        INIT_INSTANCE_FUNCTION(vkCreateDisplayPlaneSurfaceKHR);
    }

#ifndef NDEBUG
    INIT_INSTANCE_FUNCTION(vkCreateDebugReportCallbackEXT)
	INIT_INSTANCE_FUNCTION(vkDestroyDebugReportCallbackEXT)	//
#endif

#undef INIT_INSTANCE_FUNCTION

}



////////////////////////////////


static void vk_selectPhysicalDevice(void)
{
    // After initializing the Vulkan library through a VkInstance
    // we need to look for and select a graphics card in the system
    // that supports the features we need. In fact we can select any
    // number of graphics cards and use them simultaneously.
	uint32_t gpu_count = 0;

    // Initial call to query gpu_count, then second call for gpu info.
	qvkEnumeratePhysicalDevices(vk.instance, &gpu_count, NULL);

	if (gpu_count <= 0)
		ri.Error(ERR_FATAL, "Vulkan: no physical device found");

    VkPhysicalDevice *pPhyDev = (VkPhysicalDevice *) malloc (sizeof(VkPhysicalDevice) * gpu_count);
    
    // TODO: multi graphic cards selection support
    VK_CHECK(qvkEnumeratePhysicalDevices(vk.instance, &gpu_count, pPhyDev));
    // For demo app we just grab the first physical device
    vk.physical_device = pPhyDev[0];
	
    free(pPhyDev);

    ri.Printf(PRINT_ALL, " Total %d graphics card, the first one is choosed. \n", gpu_count);

    ri.Printf(PRINT_ALL, " Get physical device memory properties: vk.devMemProperties \n");
    qvkGetPhysicalDeviceMemoryProperties(vk.physical_device, &vk.devMemProperties);
}


const char * ColorSpaceEnum2str(enum VkColorSpaceKHR cs)
{
    switch(cs)
    {
        case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
            return "VK_COLOR_SPACE_SRGB_NONLINEAR_KHR";
        case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT:
            return "VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT";
        case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:
            return "VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT";
        case VK_COLOR_SPACE_DCI_P3_LINEAR_EXT:
            return "VK_COLOR_SPACE_DCI_P3_LINEAR_EXT";
        case VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT:
            return "VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT";
        case VK_COLOR_SPACE_BT709_LINEAR_EXT:
            return "VK_COLOR_SPACE_BT709_LINEAR_EXT";
        case VK_COLOR_SPACE_BT709_NONLINEAR_EXT:
            return "VK_COLOR_SPACE_BT709_NONLINEAR_EXT";
        case VK_COLOR_SPACE_BT2020_LINEAR_EXT:
            return "VK_COLOR_SPACE_BT2020_LINEAR_EXT";
        case VK_COLOR_SPACE_HDR10_ST2084_EXT:
            return "VK_COLOR_SPACE_HDR10_ST2084_EXT";
        case VK_COLOR_SPACE_DOLBYVISION_EXT:
            return "VK_COLOR_SPACE_DOLBYVISION_EXT";
        case VK_COLOR_SPACE_HDR10_HLG_EXT:
            return "VK_COLOR_SPACE_HDR10_HLG_EXT";
        case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT:
            return "VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT";
        case VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT:
            return "VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT";
        case VK_COLOR_SPACE_PASS_THROUGH_EXT:
            return "VK_COLOR_SPACE_PASS_THROUGH_EXT";
        case VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT:
            return "VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT";
        default:
            return "Not_Known";
    }
}

static void vk_assertSurfaceCapabilities(VkSurfaceKHR HSurface)
{
    
    // To query supported format features which are properties of the physical device
	ri.Printf(PRINT_ALL, "\n --------  Query supported format features --------\n");
    
    VkFormatProperties props;


    // To determine the set of valid usage bits for a given format,
    // ========================= color ================
    qvkGetPhysicalDeviceFormatProperties(vk.physical_device, vk.surface_format.format, &props);
    
    // Check if the device supports blitting to linear images 
    if ( props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT )
        ri.Printf(PRINT_ALL, " Linear Tiling Features supported. \n");

    if ( props.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT ) 
    {
        ri.Printf(PRINT_ALL, " Blitting from linear tiled images supported.\n");
    }

    if ( props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT )
    {
        ri.Printf(PRINT_ALL, " Blitting from optimal tiled images supported.\n");
        vk.isBlitSupported = VK_TRUE;
    }


    //=========================== depth =====================================
    qvkGetPhysicalDeviceFormatProperties(vk.physical_device, VK_FORMAT_D24_UNORM_S8_UINT, &props);
    if ( props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT )
    {
        ri.Printf(PRINT_ALL, " VK_FORMAT_D24_UNORM_S8_UINT optimal Tiling feature supported.\n");
        vk.fmt_DepthStencil = VK_FORMAT_D24_UNORM_S8_UINT;
    }
    else
    {
        qvkGetPhysicalDeviceFormatProperties(vk.physical_device, VK_FORMAT_D32_SFLOAT_S8_UINT, &props);

        if ( props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT )
        {
            ri.Printf(PRINT_ALL, " VK_FORMAT_D32_SFLOAT_S8_UINT optimal Tiling feature supported.\n");
            vk.fmt_DepthStencil = VK_FORMAT_D32_SFLOAT_S8_UINT;
        }
        else
        {
            //formats[0] = VK_FORMAT_X8_D24_UNORM_PACK32;
		    //formats[1] = VK_FORMAT_D32_SFLOAT;
            // never get here.
	        ri.Error(ERR_FATAL, " Failed to find depth attachment format.\n");
        }
    }

    ri.Printf(PRINT_ALL, " -------- --------------------------- --------\n\n");
}


static void vk_selectSurfaceFormat(VkSurfaceKHR HSurface)
{
    uint32_t nSurfmt;
    uint32_t i;

    // Get the numbers of VkFormat's that are supported
    // "vk.surface" is the surface that will be associated with the swapchain.
    // "vk.surface" must be a valid VkSurfaceKHR handle
    VK_CHECK( qvkGetPhysicalDeviceSurfaceFormatsKHR(vk.physical_device, HSurface, &nSurfmt, NULL) );
    assert(nSurfmt > 0);

    VkSurfaceFormatKHR * pSurfFmts = 
        (VkSurfaceFormatKHR *) malloc( nSurfmt * sizeof(VkSurfaceFormatKHR) );

    // To query the supported swapchain format-color space pairs for a surface
    VK_CHECK( qvkGetPhysicalDeviceSurfaceFormatsKHR(vk.physical_device, HSurface, &nSurfmt, pSurfFmts) );

    ri.Printf(PRINT_ALL, " -------- Total %d surface formats supported. -------- \n", nSurfmt);
    
    for( i = 0; i < nSurfmt; ++i)
    {
        ri.Printf(PRINT_ALL, " [%d], format %d: ,color space: %s \n",
            i, pSurfFmts[i].format, ColorSpaceEnum2str(pSurfFmts[i].colorSpace));
    }


    // If the format list includes just one entry of VK_FORMAT_UNDEFINED, the surface
    // has no preferred format. Otherwise, at least one supported format will be returned.
    if ( (nSurfmt == 1) && (pSurfFmts[0].format == VK_FORMAT_UNDEFINED) )
    {
        // special case that means we can choose any format
        vk.surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
        vk.surface_format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        ri.Printf(PRINT_ALL, "VK_FORMAT_R8G8B8A8_UNORM\n");
        ri.Printf(PRINT_ALL, "VK_COLORSPACE_SRGB_NONLINEAR_KHR\n");
    }
    else
    {
        ri.Printf(PRINT_ALL, " we choose: \n" );

        for( i = 0; i < nSurfmt; i++)
        {
            if( ( pSurfFmts[i].format == VK_FORMAT_B8G8R8A8_UNORM) &&
                ( pSurfFmts[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) )
            {

                ri.Printf(PRINT_ALL, " format = VK_FORMAT_B8G8R8A8_UNORM \n");
                ri.Printf(PRINT_ALL, " colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR \n");
                
                vk.surface_format = pSurfFmts[i];
                break;
            }
        }

        if (i == nSurfmt)
            vk.surface_format = pSurfFmts[0];
    }

    ri.Printf(PRINT_ALL, " --- ----------------------------------- --- \n");

    free(pSurfFmts);
}


// Vulkan device execute work that is submitted to queues.
// each device will have one or more queues, and each of those queues
// will belong to one of the device's queue families. A queue family
// is a group of queues that have identical capabilities but are 
// able to run in parallel.
//
// The number of queue families, the capabilities of each family,
// and the number of queues belonging to each family are all
// properties of the physical device.
static void vk_selectQueueFamilyForPresentation(void)
{
    // Almosty every operation in Vulkan, anything from drawing textures,
    // requires commands to be submitted to a queue. There are different
    // types of queues that originate from differnet queue families and
    // each family of queues allows only a subset of commands. 
    // For example, there could be a queue family allows processing of 
    // compute commands or one that only allows memory thansfer related
    // commands. We need to check which queue families are supported by
    // the device and which one of these supports the commands that we use.


    uint32_t nQueueFamily;
    uint32_t i;

    qvkGetPhysicalDeviceQueueFamilyProperties(vk.physical_device, &nQueueFamily, NULL);
    
    assert(nQueueFamily > 0);

    VkQueueFamilyProperties* pQueueFamilies = (VkQueueFamilyProperties *) malloc (
            nQueueFamily * sizeof(VkQueueFamilyProperties) );

    // To query properties of queues available on a physical device
    qvkGetPhysicalDeviceQueueFamilyProperties(vk.physical_device, &nQueueFamily, pQueueFamilies);

    ri.Printf(PRINT_ALL, "\n -------- Total %d Queue families -------- \n",  nQueueFamily);

    // info
    // Queues within a family are essentially identical. 
    // Queues in different families may have different internal capabilities
    // that can't be expressed easily in the Vulkan API. For this reason,
    // an implementation might choose to report similar queues as members 
    // of different families.
    //
    // All commands that are allowed on a queue that supports transfer operations are
    // also allowed on a queue that supports either graphics or compute operations.
    // Thus, if the capabilities of a queue family include VK_QUEUE_GRAPHICS_BIT or
    // VK_QUEUE_COMPUTE_BIT, then reporting the VK_QUEUE_TRANSFER_BIT capability
    // separately for that queue family is OPTIONAL.
    // 
    for (i = 0; i < nQueueFamily; ++i)
    {
        // print every queue family's capability
        ri.Printf(PRINT_ALL, " Queue family [%d]: %d queues ", 
                i, pQueueFamilies[i].queueCount );

        if( pQueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT )
            ri.Printf(PRINT_ALL, " Graphic ");
        
        if( pQueueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT )
            ri.Printf(PRINT_ALL, " Compute ");

        if( pQueueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT )
            ri.Printf(PRINT_ALL, " Transfer ");

        if( pQueueFamilies[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT )
            ri.Printf(PRINT_ALL, " Sparse ");


        VkBool32 isPresentSupported = VK_FALSE;
        VK_CHECK(qvkGetPhysicalDeviceSurfaceSupportKHR(
                    vk.physical_device, i, vk.surface, &isPresentSupported));

        if (isPresentSupported)
        {
            ri.Printf(PRINT_ALL, " presentation supported. \n --------\n");
        }
        else
        {
            ri.Printf(PRINT_ALL, " \n -------- \n");
        }
    }


    // Select queue family with presentation and graphics support
    // Iterate over each queue to learn whether it supports presenting:
    vk.queue_family_index = -1;
    
    for (i = 0; i < nQueueFamily; ++i)
    {
        // To look for a queue family that has the capability of presenting
        // to our window surface
        
        VkBool32 presentation_supported = VK_FALSE;
        VK_CHECK(qvkGetPhysicalDeviceSurfaceSupportKHR(
                    vk.physical_device, i, vk.surface, &presentation_supported));

        if (presentation_supported && 
                (pQueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            vk.queue_family_index = i;
            
            ri.Printf(PRINT_ALL, " Queue family index %d selected for presentation.\n",
                    vk.queue_family_index);

            break;
        }
    }

    ri.Printf(PRINT_ALL, " -------- ---------------------------- -------- \n\n");

    free(pQueueFamilies);

    if (vk.queue_family_index == -1)
        ri.Error(ERR_FATAL, "Vulkan: failed to find queue family");
}



void vk_checkSwapChainExtention(const char * const pName)
{
    /* Look for device extensions */


    //  Not all graphics cards are capble of presenting images directly
    //  to a screen for various reasons, for example because they are 
    //  designed for servers and don't have any display outputs. 
    //  Secondly, since image presentation is heavily tied into the 
    //  window system and the surfaces associated with windows, it is
    //  not actually part of the vulkan core. You have to enable the
    //  VK_KHR_swapchain device extension after querying for its support.
    uint32_t nDevExts = 0;
    
    VkBool32 swapchainExtFound = 0;

    // To query the numbers of extensions available to a given physical device
    ri.Printf( PRINT_ALL, " Check for VK_KHR_swapchain extension. \n" );

    qvkEnumerateDeviceExtensionProperties( vk.physical_device, NULL, &nDevExts, NULL);

    VkExtensionProperties* pDeviceExt = 
        (VkExtensionProperties *) malloc( sizeof(VkExtensionProperties) * nDevExts );

    qvkEnumerateDeviceExtensionProperties( vk.physical_device, NULL, &nDevExts, pDeviceExt);


    uint32_t j;
    for (j = 0; j < nDevExts; j++)
    {
        if (0 == strcmp(pName, pDeviceExt[j].extensionName))
        {
            swapchainExtFound = VK_TRUE;
            break;
        }
    }
    
    if (VK_FALSE == swapchainExtFound)
        ri.Error(ERR_FATAL, "VK_KHR_SWAPCHAIN_EXTENSION_NAME is not available on you GPU driver.");

    // info
    ri.Printf( PRINT_ALL, "-------- Total %d device extensions supported --------\n", nDevExts);
    for (j = 0; j < nDevExts; ++j)
    {
        ri.Printf( PRINT_ALL, "%s \n", pDeviceExt[j].extensionName);
    }

    ri.Printf(PRINT_ALL, "-------- Enabled device extensions on this app --------\n");
    
    ri.Printf(PRINT_ALL, " %s \n", pName);

    ri.Printf(PRINT_ALL, "-------- --------------------------------------- ------\n\n");


    free(pDeviceExt);
}



static void vk_createLogicalDevice(void)
{
////////////////////////
    const char* enable_ext_names[1] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    vk_checkSwapChainExtention(enable_ext_names[0]);

    const float priority = 1.0;
    VkDeviceQueueCreateInfo queue_desc;
    queue_desc.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_desc.pNext = NULL;
    queue_desc.flags = 0;
    queue_desc.queueFamilyIndex = vk.queue_family_index;
    queue_desc.queueCount = 1;
    queue_desc.pQueuePriorities = &priority;


    // Query fine-grained feature support for this device. If APP 
    // has specific feature requirements it should check supported
    // features based on this query.

	VkPhysicalDeviceFeatures features;
	qvkGetPhysicalDeviceFeatures(vk.physical_device, &features);
	if (features.shaderClipDistance == VK_FALSE)
		ri.Error(ERR_FATAL,
            "vk_create_device: shaderClipDistance feature is not supported");
	if (features.fillModeNonSolid == VK_FALSE)
	    ri.Error(ERR_FATAL,
            "vk_create_device: fillModeNonSolid feature is not supported");


    VkDeviceCreateInfo device_desc;
    device_desc.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_desc.pNext = NULL;
    device_desc.flags = 0;
    device_desc.queueCreateInfoCount = 1;
    // Creating a logical device also creates the queues associated with that device.
    device_desc.pQueueCreateInfos = &queue_desc;
    device_desc.enabledLayerCount = 0;
    device_desc.ppEnabledLayerNames = NULL;
    device_desc.enabledExtensionCount = 1;
    device_desc.ppEnabledExtensionNames = enable_ext_names;
    device_desc.pEnabledFeatures = &features;
    

    // After selecting a physical device to use we need to set up a
    // logical device to interface with it. The logical device 
    // creation process id similar to the instance creation process
    // and describes the features we want to use. we also need to 
    // specify which queues to create now that we've queried which
    // queue families are available. You can create multiple logical
    // devices from the same physical device if you have varying requirements.
    ri.Printf( PRINT_ALL, " Create logical device: vk.device \n" );
    VK_CHECK(qvkCreateDevice(vk.physical_device, &device_desc, NULL, &vk.device));

}


static void vk_loadDeviceFunctions(void)
{
    ri.Printf( PRINT_ALL, " Loading device level function. \n" );

    #define INIT_DEVICE_FUNCTION(func)                              \
    q##func = (PFN_ ## func)qvkGetDeviceProcAddr(vk.device, #func); \
    if (q##func == NULL) {                                          \
        ri.Error(ERR_FATAL, "Failed to find entrypoint %s", #func); \
    }     

	INIT_DEVICE_FUNCTION(vkAllocateCommandBuffers)
	INIT_DEVICE_FUNCTION(vkAllocateDescriptorSets)
	INIT_DEVICE_FUNCTION(vkAllocateMemory)
	INIT_DEVICE_FUNCTION(vkBeginCommandBuffer)
	INIT_DEVICE_FUNCTION(vkBindBufferMemory)
	INIT_DEVICE_FUNCTION(vkBindImageMemory)
	INIT_DEVICE_FUNCTION(vkCmdBeginRenderPass)
	INIT_DEVICE_FUNCTION(vkCmdBindDescriptorSets)
	INIT_DEVICE_FUNCTION(vkCmdBindIndexBuffer)
	INIT_DEVICE_FUNCTION(vkCmdBindPipeline)
	INIT_DEVICE_FUNCTION(vkCmdBindVertexBuffers)
	INIT_DEVICE_FUNCTION(vkCmdBlitImage)
	INIT_DEVICE_FUNCTION(vkCmdClearAttachments)
	INIT_DEVICE_FUNCTION(vkCmdCopyBufferToImage)
	INIT_DEVICE_FUNCTION(vkCmdCopyImage)
    INIT_DEVICE_FUNCTION(vkCmdCopyImageToBuffer)
	INIT_DEVICE_FUNCTION(vkCmdDraw)
	INIT_DEVICE_FUNCTION(vkCmdDrawIndexed)
	INIT_DEVICE_FUNCTION(vkCmdEndRenderPass)
	INIT_DEVICE_FUNCTION(vkCmdPipelineBarrier)
	INIT_DEVICE_FUNCTION(vkCmdPushConstants)
	INIT_DEVICE_FUNCTION(vkCmdSetDepthBias)
	INIT_DEVICE_FUNCTION(vkCmdSetScissor)
	INIT_DEVICE_FUNCTION(vkCmdSetViewport)
	INIT_DEVICE_FUNCTION(vkCreateBuffer)
	INIT_DEVICE_FUNCTION(vkCreateCommandPool)
	INIT_DEVICE_FUNCTION(vkCreateDescriptorPool)
	INIT_DEVICE_FUNCTION(vkCreateDescriptorSetLayout)
	INIT_DEVICE_FUNCTION(vkCreateFence)
	INIT_DEVICE_FUNCTION(vkCreateFramebuffer)
	INIT_DEVICE_FUNCTION(vkCreateGraphicsPipelines)
	INIT_DEVICE_FUNCTION(vkCreateImage)
	INIT_DEVICE_FUNCTION(vkCreateImageView)
	INIT_DEVICE_FUNCTION(vkCreatePipelineLayout)
	INIT_DEVICE_FUNCTION(vkCreateRenderPass)
	INIT_DEVICE_FUNCTION(vkCreateSampler)
	INIT_DEVICE_FUNCTION(vkCreateSemaphore)
	INIT_DEVICE_FUNCTION(vkCreateShaderModule)
	INIT_DEVICE_FUNCTION(vkDestroyBuffer)
	INIT_DEVICE_FUNCTION(vkDestroyCommandPool)
	INIT_DEVICE_FUNCTION(vkDestroyDescriptorPool)
	INIT_DEVICE_FUNCTION(vkDestroyDescriptorSetLayout)
	INIT_DEVICE_FUNCTION(vkDestroyDevice)
	INIT_DEVICE_FUNCTION(vkDestroyFence)
	INIT_DEVICE_FUNCTION(vkDestroyFramebuffer)
	INIT_DEVICE_FUNCTION(vkDestroyImage)
	INIT_DEVICE_FUNCTION(vkDestroyImageView)
	INIT_DEVICE_FUNCTION(vkDestroyPipeline)
	INIT_DEVICE_FUNCTION(vkDestroyPipelineLayout)
	INIT_DEVICE_FUNCTION(vkDestroyRenderPass)
	INIT_DEVICE_FUNCTION(vkDestroySampler)
	INIT_DEVICE_FUNCTION(vkDestroySemaphore)
	INIT_DEVICE_FUNCTION(vkDestroyShaderModule)
	INIT_DEVICE_FUNCTION(vkDeviceWaitIdle)
	INIT_DEVICE_FUNCTION(vkEndCommandBuffer)
	INIT_DEVICE_FUNCTION(vkFreeCommandBuffers)
	INIT_DEVICE_FUNCTION(vkFreeDescriptorSets)
	INIT_DEVICE_FUNCTION(vkFreeMemory)
	INIT_DEVICE_FUNCTION(vkGetBufferMemoryRequirements)
	INIT_DEVICE_FUNCTION(vkGetDeviceQueue)
	INIT_DEVICE_FUNCTION(vkGetImageMemoryRequirements)
	INIT_DEVICE_FUNCTION(vkGetImageSubresourceLayout)
	INIT_DEVICE_FUNCTION(vkMapMemory)
	INIT_DEVICE_FUNCTION(vkUnmapMemory)
	INIT_DEVICE_FUNCTION(vkQueueSubmit)
	INIT_DEVICE_FUNCTION(vkQueueWaitIdle)
	INIT_DEVICE_FUNCTION(vkResetDescriptorPool)
	INIT_DEVICE_FUNCTION(vkResetFences)
	INIT_DEVICE_FUNCTION(vkUpdateDescriptorSets)
	INIT_DEVICE_FUNCTION(vkWaitForFences)
    
	INIT_DEVICE_FUNCTION(vkCreateSwapchainKHR)
	INIT_DEVICE_FUNCTION(vkDestroySwapchainKHR)
	INIT_DEVICE_FUNCTION(vkGetSwapchainImagesKHR)
    INIT_DEVICE_FUNCTION(vkAcquireNextImageKHR)
	INIT_DEVICE_FUNCTION(vkQueuePresentKHR)

    #undef INIT_DEVICE_FUNCTION
}


static VkPresentModeKHR vk_selectPresentationMode(VkSurfaceKHR HSurface)
{
    // The presentation is arguably the most impottant setting for the swap chain
    // because it represents the actual conditions for showing images to the screen
    // There four possible modes available in Vulkan:

    // 1) VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted by your application
    //    are transferred to the screen right away, which may result in tearing.
    //
    // 2) VK_PRESENT_MODE_FIFO_KHR: The swap chain is a queue where the display
    //    takes an image from the front of the queue when the display is refreshed
    //    and the program inserts rendered images at the back of the queue. If the
    //    queue is full then the program has to wait. This is most similar to 
    //    vertical sync as found in modern games
    //
    // 3) VK_PRESENT_MODE_FIFO_RELAXED_KHR: variation of 2)
    //
    // 4) VK_PRESENT_MODE_MAILBOX_KHR: another variation of 2), the image already
    //    queued are simply replaced with the newer ones. This mode can be used
    //    to avoid tearing significantly less latency issues than standard vertical
    //    sync that uses double buffering.
    uint32_t nPM = 0, i;
    
    VkBool32 mailbox_supported = VK_FALSE;
    VkBool32 immediate_supported = VK_FALSE;
    
    // Look for the best mode available.

    qvkGetPhysicalDeviceSurfacePresentModesKHR(vk.physical_device, HSurface, &nPM, NULL);

    assert(nPM > 0);

    VkPresentModeKHR * pPresentModes = (VkPresentModeKHR *) malloc( nPM * sizeof(VkPresentModeKHR) );

    qvkGetPhysicalDeviceSurfacePresentModesKHR(vk.physical_device, HSurface, &nPM, pPresentModes);

    ri.Printf(PRINT_ALL, "-------- Total %d present mode supported. -------- \n", nPM);
    for ( i = 0; i < nPM; ++i)
    {
        switch( pPresentModes[i] )
        {
            case VK_PRESENT_MODE_IMMEDIATE_KHR:
                ri.Printf(PRINT_ALL, " [%d] VK_PRESENT_MODE_IMMEDIATE_KHR \n", i);
                immediate_supported = VK_TRUE;
                break;
            case VK_PRESENT_MODE_MAILBOX_KHR:
                ri.Printf(PRINT_ALL, " [%d] VK_PRESENT_MODE_MAILBOX_KHR \n", i);
                mailbox_supported = VK_TRUE;
                break;
            case VK_PRESENT_MODE_FIFO_KHR:
                ri.Printf(PRINT_ALL, " [%d] VK_PRESENT_MODE_FIFO_KHR \n", i);
                break;
            case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
                ri.Printf(PRINT_ALL, " [%d] VK_PRESENT_MODE_FIFO_RELAXED_KHR \n", i);
                break;
            default:
                ri.Printf(PRINT_WARNING, "Unknown presentation mode: %d. \n",
                        pPresentModes[i]);
                break;
        }
    }

    free(pPresentModes);


    if (mailbox_supported)
    {
        ri.Printf(PRINT_ALL, " Presentation with VK_PRESENT_MODE_MAILBOX_KHR mode. \n");
        return VK_PRESENT_MODE_MAILBOX_KHR;
    }
    else if(immediate_supported)
    {
        ri.Printf(PRINT_ALL, " Presentation with VK_PRESENT_MODE_IMMEDIATE_KHR mode. \n");
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    }

    // VK_PRESENT_MODE_FIFO_KHR mode is guaranteed to
    // be available according to the spec.
    ri.Printf(PRINT_ALL, "\n Presentation with VK_PRESENT_MODE_FIFO_KHR mode. \n");
    ri.Printf(PRINT_ALL, "-------- ----------------------------- --------\n");

    return VK_PRESENT_MODE_FIFO_KHR;
}


void vk_getProcAddress(void)
{
    vk_loadGlobalLevelFunctions();

    vk_assertStandValidationLayer();


    vk_loadInstanceLevelFunctions();

#ifndef NDEBUG
	// Create debug callback.
    vk_createDebugCallback(vk_DebugCallback);
#endif

    // The window surface needs to be created right after the instance creation,
    // because it can actually influence the presentation mode selection.
	vk_createSurfaceImpl( &vk.surface ); 

    // select physical device
    vk_selectPhysicalDevice();

    vk_selectSurfaceFormat(vk.surface);
    
    vk_assertSurfaceCapabilities(vk.surface);
    
    vk.present_mode = vk_selectPresentationMode(vk.surface);

    vk_selectQueueFamilyForPresentation();

    vk_createLogicalDevice();

    // Get device level functions. depended on the created logical device
    // thus must be called AFTER vk_createLogicalDevice.
    vk_loadDeviceFunctions();

    // a call to retrieve queue handle
	qvkGetDeviceQueue(vk.device, vk.queue_family_index, 0, &vk.queue);
    //
    //     Queue Family Index
    //
    // The queue family index is used in multiple places in Vulkan in order to
    // tie operations to a specific family of queues. When retrieving a handle 
    // to the queue via vkGetDeviceQueue, the queue family index is used to
    // select which queue family to retrieve the VkQueue handle from.
    // 
    // When creating a VkCommandPool object, a queue family index is specified
    // in the VkCommandPoolCreateInfo structure. Command buffers from this pool
    // can only be submitted on queues corresponding to this queue family.
    //
    // When creating VkImage (see Images) and VkBuffer (see Buffers) resources,
    // a set of queue families is included in the VkImageCreateInfo and 
    // VkBufferCreateInfo structures to specify the queue families that can 
    // access the resource.
    //
    // When inserting a VkBufferMemoryBarrier or VkImageMemoryBarrier
    // a source and destination queue family index is specified to allow the 
    // ownership of a buffer or image to be transferred from one queue family
    // to another.
}


void vk_clearProcAddress(void)
{

    ri.Printf( PRINT_ALL, " Destroy surface: vk.surface. \n" );
    // make sure that the surface is destroyed before the instance
    qvkDestroySurfaceKHR(vk.instance, vk.surface, NULL);

#ifndef NDEBUG
    ri.Printf( PRINT_ALL, " Destroy callback function: vk.h_debugCB. \n" );

	qvkDestroyDebugReportCallbackEXT(vk.instance, vk.h_debugCB, NULL);
#endif

    ri.Printf( PRINT_ALL, " Destroy logical device: vk.device. \n" );
    // Device queues are implicitly cleaned up when the device is destroyed
    // so we don't need to do anything in clean up
	qvkDestroyDevice(vk.device, NULL);

    ri.Printf( PRINT_ALL, " Destroy instance: vk.instance. \n" );
	qvkDestroyInstance(vk.instance, NULL);

// ===========================================================
    ri.Printf( PRINT_ALL, " clear all proc address \n" );

    // Global Level
	qvkCreateInstance                           = NULL;
	qvkEnumerateInstanceExtensionProperties		= NULL;
    qvkEnumerateInstanceLayerProperties         = NULL;
    
    // Instance Level;
	qvkCreateDevice								= NULL;
	qvkDestroyInstance							= NULL;
	qvkEnumerateDeviceExtensionProperties		= NULL;

	qvkEnumeratePhysicalDevices					= NULL;
	qvkGetDeviceProcAddr						= NULL;
	qvkGetPhysicalDeviceFeatures				= NULL;
	qvkGetPhysicalDeviceFormatProperties		= NULL;
	qvkGetPhysicalDeviceMemoryProperties		= NULL;
	qvkGetPhysicalDeviceProperties				= NULL;
	qvkGetPhysicalDeviceQueueFamilyProperties	= NULL;


    qvkGetPhysicalDeviceDisplayPropertiesKHR    = NULL;
    qvkGetPhysicalDeviceDisplayPlanePropertiesKHR = NULL;
    qvkGetDisplayPlaneSupportedDisplaysKHR      = NULL;
    qvkGetDisplayModePropertiesKHR              = NULL;
    qvkCreateDisplayModeKHR                     = NULL;
    qvkGetDisplayPlaneCapabilitiesKHR           = NULL;
    qvkCreateDisplayPlaneSurfaceKHR             = NULL;


    qvkDestroySurfaceKHR						= NULL;
	qvkGetPhysicalDeviceSurfaceCapabilitiesKHR	= NULL;
	qvkGetPhysicalDeviceSurfaceFormatsKHR		= NULL;
	qvkGetPhysicalDeviceSurfacePresentModesKHR	= NULL;
	qvkGetPhysicalDeviceSurfaceSupportKHR		= NULL;
#ifndef NDEBUG
	qvkCreateDebugReportCallbackEXT				= NULL;
	qvkDestroyDebugReportCallbackEXT			= NULL;
#endif

	qvkAllocateCommandBuffers					= NULL;
	qvkAllocateDescriptorSets					= NULL;
	qvkAllocateMemory							= NULL;
	qvkBeginCommandBuffer						= NULL;
	qvkBindBufferMemory							= NULL;
	qvkBindImageMemory							= NULL;
	qvkCmdBeginRenderPass						= NULL;
	qvkCmdBindDescriptorSets					= NULL;
	qvkCmdBindIndexBuffer						= NULL;
	qvkCmdBindPipeline							= NULL;
	qvkCmdBindVertexBuffers						= NULL;
	qvkCmdBlitImage								= NULL;
	qvkCmdClearAttachments						= NULL;
	qvkCmdCopyBufferToImage						= NULL;
	qvkCmdCopyImage								= NULL;
    qvkCmdCopyImageToBuffer                     = NULL;
	qvkCmdDraw									= NULL;
	qvkCmdDrawIndexed							= NULL;
	qvkCmdEndRenderPass							= NULL;
	qvkCmdPipelineBarrier						= NULL;
	qvkCmdPushConstants							= NULL;
	qvkCmdSetDepthBias							= NULL;
	qvkCmdSetScissor							= NULL;
	qvkCmdSetViewport							= NULL;
	qvkCreateBuffer								= NULL;
	qvkCreateCommandPool						= NULL;
	qvkCreateDescriptorPool						= NULL;
	qvkCreateDescriptorSetLayout				= NULL;
	qvkCreateFence								= NULL;
	qvkCreateFramebuffer						= NULL;
	qvkCreateGraphicsPipelines					= NULL;
	qvkCreateImage								= NULL;
	qvkCreateImageView							= NULL;
	qvkCreatePipelineLayout						= NULL;
	qvkCreateRenderPass							= NULL;
	qvkCreateSampler							= NULL;
	qvkCreateSemaphore							= NULL;
	qvkCreateShaderModule						= NULL;
	qvkDestroyBuffer							= NULL;
	qvkDestroyCommandPool						= NULL;
	qvkDestroyDescriptorPool					= NULL;
	qvkDestroyDescriptorSetLayout				= NULL;
	qvkDestroyDevice							= NULL;
	qvkDestroyFence								= NULL;
	qvkDestroyFramebuffer						= NULL;
	qvkDestroyImage								= NULL;
	qvkDestroyImageView							= NULL;
	qvkDestroyPipeline							= NULL;
	qvkDestroyPipelineLayout					= NULL;
	qvkDestroyRenderPass						= NULL;
	qvkDestroySampler							= NULL;
	qvkDestroySemaphore							= NULL;
	qvkDestroyShaderModule						= NULL;
	qvkDeviceWaitIdle							= NULL;
	qvkEndCommandBuffer							= NULL;
	qvkFreeCommandBuffers						= NULL;
	qvkFreeDescriptorSets						= NULL;
	qvkFreeMemory								= NULL;
	qvkGetBufferMemoryRequirements				= NULL;
	qvkGetDeviceQueue							= NULL;
	qvkGetImageMemoryRequirements				= NULL;
	qvkGetImageSubresourceLayout				= NULL;
	qvkMapMemory								= NULL;
    qvkUnmapMemory                              = NULL;
	qvkQueueSubmit								= NULL;
	qvkQueueWaitIdle							= NULL;
	qvkResetDescriptorPool						= NULL;
	qvkResetFences								= NULL;
	qvkUpdateDescriptorSets						= NULL;
	qvkWaitForFences							= NULL;
	qvkAcquireNextImageKHR						= NULL;
	qvkCreateSwapchainKHR						= NULL;
	qvkDestroySwapchainKHR						= NULL;
	qvkGetSwapchainImagesKHR					= NULL;
	qvkQueuePresentKHR							= NULL;
}


const char * cvtResToStr(VkResult result)
{
    switch(result)
    {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_OUT_OF_POOL_MEMORY_KHR:
            return "VK_ERROR_OUT_OF_POOL_MEMORY_KHR";
        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";
//
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_NOT_PERMITTED_EXT:
            return "VK_ERROR_NOT_PERMITTED_EXT";
//
        case VK_RESULT_MAX_ENUM:
            return "VK_RESULT_MAX_ENUM";
        case VK_RESULT_RANGE_SIZE:
            return "VK_RESULT_RANGE_SIZE"; 
        case VK_ERROR_FRAGMENTATION_EXT:
            return "VK_ERROR_FRAGMENTATION_EXT";
    }

    return "UNKNOWN_ERROR";
}
