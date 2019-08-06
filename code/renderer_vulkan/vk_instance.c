#include <string.h>
#include <stdlib.h>

#include "VKimpl.h"
#include "vk_instance.h"
#include "vk_image.h"
#include "vk_instance.h"
#include "vk_shade_geometry.h"
#include "vk_pipelines.h"
#include "vk_frame.h"
#include "vk_shaders.h"
#include "vk_validation.h"
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
PFN_vkCmdClearColorImage                        qvkCmdClearColorImage;
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
static VkBool32 vk_assertStandValidationLayer(void)
{
    // Look For Standard Validation Layer
    VkBool32 found = VK_FALSE;
    
    // static const char instance_validation_layers_name[] = {"VK_LAYER_LUNARG_standard_validation"};

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
			
			if (!strcmp("VK_LAYER_LUNARG_standard_validation", instance_layers[j].layerName))
            {
                found = VK_TRUE;
            }
        }

         ri.Printf(PRINT_ALL, " ------- ---------------------------- -------- \n");
        
        free(instance_layers);
        
        if(found == VK_FALSE) {
            ri.Printf(PRINT_WARNING, " Failed to find required validation layer.\n\n");
        }
    }
    else
    {
        ri.Printf(PRINT_WARNING, "No standard validation layer available! \n");
    }
	return found;
}
#endif


// this function not be placed in vk_utils.c because 
// I dont want to qvkEnumerateInstanceExtensionProperties
// available to other files ...
void printInstanceExtensionsSupported_f(void)
{
    uint32_t i = 0;

	uint32_t nInsExt = 0;
    // To retrieve a list of supported extensions before creating an instance
	VK_CHECK( qvkEnumerateInstanceExtensionProperties( NULL, &nInsExt, NULL) );

    assert(nInsExt > 0);

    VkExtensionProperties* const pInsExt = (VkExtensionProperties *) malloc(sizeof(VkExtensionProperties) * nInsExt);
    
    VK_CHECK( qvkEnumerateInstanceExtensionProperties( NULL, &nInsExt, pInsExt) );

    ri.Printf(PRINT_ALL, "\n");

    ri.Printf(PRINT_ALL, "----- Total %d Instance Extension Supported -----\n", nInsExt);
    for (i = 0; i < nInsExt; ++i)
    {            
        ri.Printf(PRINT_ALL, "%s\n", pInsExt[i].extensionName );
    }
    ri.Printf(PRINT_ALL, "----- ------------------------------------- -----\n\n");
   
    free(pInsExt);
}


// Platform dependent code, not elegant :(
// doing this to avoid enable every instance extension,
// not knowing if it is necessary.
static void vk_fillRequiredInstanceExtention(
	const VkExtensionProperties * const pInsExt,
	const uint32_t nInsExt,
	const char* ppInsExt[16],
    uint32_t * nExt )
{
    uint32_t enExtCnt = 0;
    uint32_t i = 0;

#if defined(_WIN32) || defined(_WIN64)

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


static void vk_createInstance(VkInstance* const pInstance)
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
	appInfo.pEngineName = "VulkanArena";
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

	char* ppInstanceExtEnabled[16] = { 0 };
	// malloc(sizeof(char *) * (nInsExt));

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
    instanceCreateInfo.enabledLayerCount = 0;
	instanceCreateInfo.ppEnabledLayerNames = NULL;

#ifndef NDEBUG
	if( vk_assertStandValidationLayer() )
	{
		ri.Printf(PRINT_ALL, "Using VK_LAYER_LUNARG_standard_validation. \n");

		const char* const validation_layer_name = "VK_LAYER_LUNARG_standard_validation";    
		instanceCreateInfo.enabledLayerCount = 1;
		instanceCreateInfo.ppEnabledLayerNames = &validation_layer_name;
	}
#endif


    VkResult e = qvkCreateInstance(&instanceCreateInfo, NULL, pInstance);
    if(e == VK_SUCCESS)
    {
        ri.Printf(PRINT_ALL, " Vulkan create instance success! \n\n");
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
   
    // free(ppInstanceExtEnabled);

    free(pInsExt);
}

// Vulkan functions can be divided into three levels,  global, instance, and device. 
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

    qvkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) vk_getInstanceProcAddrImpl();
    
    if( qvkGetInstanceProcAddr == NULL)
    {
        ri.Error(ERR_FATAL, "Failed to find entrypoint vkGetInstanceProcAddr. "); 
    }


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

    // We have created a Vulkan Instance object. 
    // The next step is to enumerate physical devices, 
    // choose one of them, and create a logical device from it.
    // These operations are performed with instance-level functions,
    // of which we need to acquire the addresses.

    // Instance-level functions are used mainly for operations on
    // physical devices. There are multiple instance-level functions.
    
#define INIT_INSTANCE_FUNCTION(func)                                    \
    q##func = (PFN_##func) qvkGetInstanceProcAddr(vk.instance, #func);  \
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
        ri.Printf(PRINT_ALL, " VK_KHR_Display Supported, Loading functions for this instance extention. \n");

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


void vk_getProcAddress(void)
{
    vk_loadGlobalLevelFunctions();

    vk_createInstance( &vk.instance );
    // We have created a Vulkan Instance object, then 
    // use that instance to load instance level functions
    vk_loadInstanceLevelFunctions();
}


uint32_t vk_getWinWidth(void)
{
	return vk.renderArea.extent.width;
}

uint32_t vk_getWinHeight(void)
{
	return vk.renderArea.extent.height;
}


void vk_clearProcAddress(void)
{

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
    qvkCmdClearColorImage                       = NULL;
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


void vk_destroy_instance(void)
{

    ri.Printf( PRINT_ALL, " Destroy surface: vk.surface. \n" );
    
    // make sure that the surface is destroyed before the instance
    qvkDestroySurfaceKHR(vk.instance, vk.surface, NULL);

    vk_destroyDebugReportHandle();

    ri.Printf( PRINT_ALL, " Destroy instance: vk.instance. \n" );
	qvkDestroyInstance(vk.instance, NULL);
}
