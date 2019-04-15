#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "vk_common.h"
#include "vk_instance.h"
#include "qvk.h"
#include "vk_validation.h"


struct VkInstance_t vk;


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
		fprintf(stderr, "Vulkan: no physical device found");

    VkPhysicalDevice *pPhyDev = (VkPhysicalDevice *) malloc (sizeof(VkPhysicalDevice) * gpu_count);
    
    // TODO: multi graphic cards selection support
    VK_CHECK(qvkEnumeratePhysicalDevices(vk.instance, &gpu_count, pPhyDev));
    // For demo app we just grab the first physical device
    vk.physical_device = pPhyDev[0];
	
    free(pPhyDev);

    printf( " Total %d graphics card, the first one is choosed. \n", gpu_count);

    printf( " Get physical device memory properties: vk.devMemProperties \n");
    qvkGetPhysicalDeviceMemoryProperties(vk.physical_device, &vk.devMemProperties);
}



static void vk_selectSurfaceFormat(void)
{
    uint32_t nSurfmt;
    
    printf( "\n -------- vk_selectSurfaceFormat() -------- \n");


    // Get the numbers of VkFormat's that are supported
    // "vk.surface" is the surface that will be associated with the swapchain.
    // "vk.surface" must be a valid VkSurfaceKHR handle
    VK_CHECK(qvkGetPhysicalDeviceSurfaceFormatsKHR(vk.physical_device, vk.surface, &nSurfmt, NULL));
    assert(nSurfmt > 0);

    VkSurfaceFormatKHR *pSurfFmts = 
        (VkSurfaceFormatKHR *) malloc ( nSurfmt * sizeof(VkSurfaceFormatKHR) );

    // To query the supported swapchain format-color space pairs for a surface
    VK_CHECK(qvkGetPhysicalDeviceSurfaceFormatsKHR(vk.physical_device, vk.surface, &nSurfmt, pSurfFmts));

    // If the format list includes just one entry of VK_FORMAT_UNDEFINED, the surface
    // has no preferred format. Otherwise, at least one supported format will be returned.
    if ( (nSurfmt == 1) && (pSurfFmts[0].format == VK_FORMAT_UNDEFINED) )
    {
        // special case that means we can choose any format
        vk.surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
        vk.surface_format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        printf( "VK_FORMAT_R8G8B8A8_UNORM\n");
        printf( "VK_COLORSPACE_SRGB_NONLINEAR_KHR\n");
    }
    else
    {
        uint32_t i;
        printf( " Total %d surface formats supported, we choose: \n", nSurfmt);

        for( i = 0; i < nSurfmt; i++)
        {
            if( ( pSurfFmts[i].format == VK_FORMAT_B8G8R8A8_UNORM) &&
                ( pSurfFmts[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) )
            {

                printf( " format = VK_FORMAT_B8G8R8A8_UNORM \n");
                printf( " colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR \n");
                
                vk.surface_format = pSurfFmts[i];
                break;
            }
        }

        if (i == nSurfmt)
            vk.surface_format = pSurfFmts[0];
    }

    free(pSurfFmts);


    // To query the basic capabilities of a surface, needed in order to create a swapchain
	VK_CHECK(qvkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.physical_device, vk.surface, &vk.surface_caps));

    // VK_IMAGE_USAGE_TRANSFER_DST_BIT is required by image clear operations.
	if ((vk.surface_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == 0)
		fprintf(stderr, "VK_IMAGE_USAGE_TRANSFER_DST_BIT is not supported by you GPU.\n");

	// VK_IMAGE_USAGE_TRANSFER_SRC_BIT is required in order to take screenshots.
	if ((vk.surface_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) == 0)
		fprintf(stderr, "VK_IMAGE_USAGE_TRANSFER_SRC_BIT is not supported by you GPU.\n");


    // To query supported format features which are properties of the physical device
	
    VkFormatProperties props;


    // To determine the set of valid usage bits for a given format,
    // call vkGetPhysicalDeviceFormatProperties.

    // ========================= color ================
    qvkGetPhysicalDeviceFormatProperties(vk.physical_device, vk.surface_format.format, &props);
    
    // Check if the device supports blitting to linear images 
    if ( props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT )
        printf( "--- Linear TilingFeatures supported. ---\n");

    if ( props.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT ) 
    {
        printf( "--- Blitting from linear tiled images supported. ---\n");
    }

    if ( props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT )
    {
        printf( "--- Blitting from optimal tiled images supported. ---\n");
        vk.isBlitSupported = VK_TRUE;
    }


    //=========================== depth =====================================
    qvkGetPhysicalDeviceFormatProperties(vk.physical_device, VK_FORMAT_D24_UNORM_S8_UINT, &props);
    if ( props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT )
    {
        printf( " VK_FORMAT_D24_UNORM_S8_UINT optimal Tiling feature supported.\n");
        vk.fmt_DepthStencil = VK_FORMAT_D24_UNORM_S8_UINT;
    }
    else
    {
        qvkGetPhysicalDeviceFormatProperties(vk.physical_device, VK_FORMAT_D32_SFLOAT_S8_UINT, &props);

        if ( props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT )
        {
            printf( " VK_FORMAT_D32_SFLOAT_S8_UINT optimal Tiling feature supported.\n");
            vk.fmt_DepthStencil = VK_FORMAT_D32_SFLOAT_S8_UINT;
        }
        else
        {
            //formats[0] = VK_FORMAT_X8_D24_UNORM_PACK32;
		    //formats[1] = VK_FORMAT_D32_SFLOAT;
            // never get here.
	        fprintf(stderr, " Failed to find depth attachment format.\n");
        }
    }

    printf( " -------- --------------------------- --------\n");
}


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

    uint32_t nSurfmt;
    qvkGetPhysicalDeviceQueueFamilyProperties(vk.physical_device, &nSurfmt, NULL);
    
    assert(nSurfmt > 0);

    VkQueueFamilyProperties* pQueueFamilies = (VkQueueFamilyProperties *) malloc (
            nSurfmt * sizeof(VkQueueFamilyProperties) );

    // To query properties of queues available on a physical device
    qvkGetPhysicalDeviceQueueFamilyProperties(vk.physical_device, &nSurfmt, pQueueFamilies);

    // Select queue family with presentation and graphics support
    // Iterate over each queue to learn whether it supports presenting:
    vk.queue_family_index = -1;
    
    uint32_t i;
    for (i = 0; i < nSurfmt; ++i)
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
            
            printf( " Queue family for presentation selected: %d\n",
                    vk.queue_family_index);

            break;
        }
    }

    free(pQueueFamilies);

    if (vk.queue_family_index == -1)
        fprintf(stderr, "Vulkan: failed to find queue family");
}


static void vk_createLogicalDevice(void)
{
    static const char* device_extensions[1] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

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
    printf(  " Check for VK_KHR_swapchain extension. \n" );

    qvkEnumerateDeviceExtensionProperties( vk.physical_device, NULL, &nDevExts, NULL);

    VkExtensionProperties* pDeviceExt = 
        (VkExtensionProperties *) malloc(sizeof(VkExtensionProperties) * nDevExts);

    qvkEnumerateDeviceExtensionProperties( vk.physical_device, NULL, &nDevExts, pDeviceExt);


    uint32_t j;
    for (j = 0; j < nDevExts; j++)
    {
        if (!strcmp(device_extensions[0], pDeviceExt[j].extensionName))
        {
            swapchainExtFound = VK_TRUE;
            break;
        }
    }
    if (VK_FALSE == swapchainExtFound)
        fprintf(stderr, "VK_KHR_SWAPCHAIN_EXTENSION_NAME is not available");

    free(pDeviceExt);


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
		fprintf(stderr,
            "vk_create_device: shaderClipDistance feature is not supported");
	if (features.fillModeNonSolid == VK_FALSE)
	    fprintf(stderr,
            "vk_create_device: fillModeNonSolid feature is not supported");


    VkDeviceCreateInfo device_desc;
    device_desc.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_desc.pNext = NULL;
    device_desc.flags = 0;
    device_desc.queueCreateInfoCount = 1;
    device_desc.pQueueCreateInfos = &queue_desc;
    device_desc.enabledLayerCount = 0;
    device_desc.ppEnabledLayerNames = NULL;
    device_desc.enabledExtensionCount = 1;
    device_desc.ppEnabledExtensionNames = device_extensions;
    device_desc.pEnabledFeatures = &features;
    

    // After selecting a physical device to use we need to set up a
    // logical device to interface with it. The logical device 
    // creation process id similar to the instance creation process
    // and describes the features we want to use. we also need to 
    // specify which queues to create now that we've queried which
    // queue families are available. You can create multiple logical
    // devices from the same physical device if you have varying requirements.
    printf(  " Create logical device: vk.device \n" );
    VK_CHECK(qvkCreateDevice(vk.physical_device, &device_desc, NULL, &vk.device));
}


void vk_createInstanceAndDevice(void)
{
    vk_loadInstanceFunctions();

#ifndef NDEBUG
	// Create debug callback.
    vk_createDebugCallback(vk_DebugCallback);
#endif

    // The window surface needs to be created right after the instance creation,
    // because it can actually influence the presentation mode selection.
	vk_createSurfaceImpl(); 
   
    // select physical device
    vk_selectPhysicalDevice();

    vk_selectSurfaceFormat(); 

    vk_selectQueueFamilyForPresentation();

    vk_createLogicalDevice();

    // Get device level functions.
    vk_loadDeviceFunctions();

    // a call to retrieve queue handle
	qvkGetDeviceQueue(vk.device, vk.queue_family_index, 0, &vk.queue);
}


void vk_destroyInstanceAndDevice(void)
{

    printf(  " Destroy logical device: vk.device. \n" );
    // Device queues are implicitly cleaned up when the device is destroyed
    // so we don't need to do anything in clean up
	qvkDestroyDevice(vk.device, NULL);
    
    printf(  " Destroy surface: vk.surface. \n" );
    // make sure that the surface is destroyed before the instance
    qvkDestroySurfaceKHR(vk.instance, vk.surface, NULL);

#ifndef NDEBUG
    printf(" Destroy callback function: vk.h_debugCB. \n" );

	qvkDestroyDebugReportCallbackEXT(vk.instance, vk.h_debugCB, NULL);
#endif

    printf(  " Destroy instance: vk.instance. \n" );
	qvkDestroyInstance(vk.instance, NULL);

// ===========================================================
    printf(  " clear all proc address \n" );
    vk_clearProcAddress();
}


