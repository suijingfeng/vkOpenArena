#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

#include "demo.h"
#include "vk_common.h"
#include "vk_swapchain.h"


struct PFN_KHR_SurfacePresent_t pFn_vkhr;

void vk_clearSurfacePresentPFN(void)
{
    memset(&pFn_vkhr, 0, sizeof(pFn_vkhr));
}

static void vk_create_device(struct demo * const pDemo)
{

    float queue_priorities[1] = {0.0};
    VkDeviceQueueCreateInfo queues[2];
    queues[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queues[0].pNext = NULL;
    queues[0].queueFamilyIndex = pDemo->graphics_queue_family_index;
    queues[0].queueCount = 1;
    queues[0].pQueuePriorities = queue_priorities;
    queues[0].flags = 0;

    VkDeviceCreateInfo device = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = queues,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = pDemo->enabled_extension_count,
        .ppEnabledExtensionNames = (const char *const *)pDemo->extension_names,
        .pEnabledFeatures = NULL,  // If specific features are required, pass them in here
    };

    if (pDemo->separate_present_queue)
    {
        queues[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queues[1].pNext = NULL;
        queues[1].queueFamilyIndex = pDemo->present_queue_family_index;
        queues[1].queueCount = 1;
        queues[1].pQueuePriorities = queue_priorities;
        queues[1].flags = 0;
        device.queueCreateInfoCount = 2;
    }
    
    VK_CHECK( vkCreateDevice(pDemo->gpu, &device, NULL, &pDemo->device) );

    printf(" Logical device created \n");
}


void vk_getDeviceProcAddrKHR(struct demo * const pDemo)
{
    static PFN_vkGetDeviceProcAddr g_gdpa;

#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                                                           \
{                                                                                                       \
    if (!g_gdpa)                                                                                        \
        g_gdpa = (PFN_vkGetDeviceProcAddr) vkGetInstanceProcAddr(pDemo->inst, "vkGetDeviceProcAddr");   \
    pFn_vkhr.fp##entrypoint = (PFN_vk##entrypoint)g_gdpa(dev, "vk" #entrypoint);                        \
    if (pFn_vkhr.fp##entrypoint == NULL) {                                                              \
        ERR_EXIT("vkGetDeviceProcAddr failed to find vk" #entrypoint, "vkGetDeviceProcAddr Failure");   \
    }                                                                                                   \
}

    GET_DEVICE_PROC_ADDR(pDemo->device, CreateSwapchainKHR);
    GET_DEVICE_PROC_ADDR(pDemo->device, DestroySwapchainKHR);
    GET_DEVICE_PROC_ADDR(pDemo->device, GetSwapchainImagesKHR);
    GET_DEVICE_PROC_ADDR(pDemo->device, AcquireNextImageKHR);
    GET_DEVICE_PROC_ADDR(pDemo->device, QueuePresentKHR);

#undef GET_DEVICE_PROC_ADDR
}


static void vk_create_surface(struct demo * const pDemo)
{
    printf("vk_create_surface. \n");

    VkXcbSurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.connection = pDemo->connection;
    createInfo.window = pDemo->xcb_window;
    VK_CHECK( vkCreateXcbSurfaceKHR(pDemo->inst, &createInfo, NULL, &pDemo->surface) );
}


static void vk_selectGraphicAndPresentQueue(struct demo * const pDemo)
{
    printf("vk_selectGraphicAndPresentQueue. \n");

    // Iterate over each queue to learn whether it supports presenting:
    VkBool32 * const supportsPresent = (VkBool32 *) 
        malloc(pDemo->queue_family_count * sizeof(VkBool32));
    
    for (uint32_t i = 0; i < pDemo->queue_family_count; ++i)
    {
        pFn_vkhr.fpGetPhysicalDeviceSurfaceSupportKHR(
                pDemo->gpu, i, pDemo->surface, &supportsPresent[i]);
    }

    // Search for a graphics and a present queue in the array of queue families, 
    // try to find one that supports both
    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t presentQueueFamilyIndex = UINT32_MAX;

    for (uint32_t i = 0; i < pDemo->queue_family_count; ++i)
    {
        if ((pDemo->queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            if (graphicsQueueFamilyIndex == UINT32_MAX)
            {
                graphicsQueueFamilyIndex = i;
            }

            if (supportsPresent[i] == VK_TRUE)
            {
                graphicsQueueFamilyIndex = i;
                presentQueueFamilyIndex = i;
                break;
            }
        }
    }

    if (presentQueueFamilyIndex == UINT32_MAX)
    {
        // If didn't find a queue that supports both graphics and present, then
        // find a separate present queue.
        for (uint32_t i = 0; i < pDemo->queue_family_count; ++i)
        {
            if (supportsPresent[i] == VK_TRUE )
            {
                presentQueueFamilyIndex = i;
                break;
            }
        }
    }

    // Generate error if could not find both a graphics and a present queue
    if (graphicsQueueFamilyIndex == UINT32_MAX || presentQueueFamilyIndex == UINT32_MAX) {
        ERR_EXIT("Could not find both graphics and present queues\n", "Swapchain Initialization Failure");
    }


    pDemo->graphics_queue_family_index = graphicsQueueFamilyIndex;
    pDemo->present_queue_family_index = presentQueueFamilyIndex;
    pDemo->separate_present_queue = (pDemo->graphics_queue_family_index != pDemo->present_queue_family_index);


    printf("queue_family_count: %d, graphicsQueueFamilyIndex: %d, presentQueueFamilyIndex: %d\n",
        pDemo->queue_family_count, graphicsQueueFamilyIndex, presentQueueFamilyIndex);
    
    printf("separate present queue: %d\n", pDemo->separate_present_queue);
    
    free(supportsPresent);
}



static void vk_chooseSurfaceFormat(struct demo * const pDemo)
{

    // Get the list of VkFormat's that are supported:
    uint32_t formatCount;
    
    VK_CHECK( pFn_vkhr.fpGetPhysicalDeviceSurfaceFormatsKHR( 
                pDemo->gpu, pDemo->surface, &formatCount, NULL) );

    VkSurfaceFormatKHR * const surfFormats = (VkSurfaceFormatKHR *)
        malloc(formatCount * sizeof(VkSurfaceFormatKHR));

    VK_CHECK( pFn_vkhr.fpGetPhysicalDeviceSurfaceFormatsKHR(pDemo->gpu, pDemo->surface, &formatCount, surfFormats));

    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED)
    {
        pDemo->format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else
    {
        assert(formatCount >= 1);
        pDemo->format = surfFormats[0].format;
    }
    
    pDemo->color_space = surfFormats[0].colorSpace;
    
    printf("%d formats. \n", formatCount);

    free(surfFormats);
}


void vk_create_semaphores(struct demo * const pDemo)
{
    printf("vk_create_semaphores\n");

    // Create semaphores to synchronize acquiring presentable buffers before
    // rendering and waiting for drawing to be complete before presenting
    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
    };

    // Create fences that we can use to throttle if we get too far
    // ahead of the image presents
    VkFenceCreateInfo fence_ci = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    
    for (uint32_t i = 0; i < FRAME_LAG; ++i)
    {
        VK_CHECK( vkCreateFence(pDemo->device, &fence_ci, NULL, &pDemo->fences[i]) );

        VK_CHECK( vkCreateSemaphore(pDemo->device, &semaphoreCreateInfo, NULL, &pDemo->image_acquired_semaphores[i]) );

        VK_CHECK( vkCreateSemaphore(pDemo->device, &semaphoreCreateInfo, NULL, &pDemo->draw_complete_semaphores[i]) );

        if (pDemo->separate_present_queue)
        {
            VK_CHECK( vkCreateSemaphore(pDemo->device, &semaphoreCreateInfo, NULL, &pDemo->image_ownership_semaphores[i]) );
        }
    }
}



static void vk_create_swapchain_view(struct demo * const pDemo)
{

    printf(" Creat swapchain view. \n ");  
    
    //////
    VK_CHECK( pFn_vkhr.fpGetSwapchainImagesKHR(
            pDemo->device, pDemo->swapchain, &pDemo->swapchainImageCount, NULL) );

    
    VkImage* const pSwapchainImages = (VkImage *) 
        malloc( pDemo->swapchainImageCount * sizeof(VkImage));
    
    assert(pSwapchainImages);

    VK_CHECK( pFn_vkhr.fpGetSwapchainImagesKHR(pDemo->device, 
                pDemo->swapchain, &pDemo->swapchainImageCount, pSwapchainImages) );


    // TODO: memory leak ...
    // NOTE: each time relocate ...
    // SwapchainImageResources * const oldSwapchainImgRes = demo->swapchain_image_resources; 
    pDemo->swapchain_image_resources = (SwapchainImageResources *)
        malloc( sizeof(SwapchainImageResources) * pDemo->swapchainImageCount );
    
    assert(pDemo->swapchain_image_resources);

    for (uint32_t i = 0; i < pDemo->swapchainImageCount; ++i)
    {
        VkImageViewCreateInfo color_image_view = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .format = pDemo->format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_R,
                .g = VK_COMPONENT_SWIZZLE_G,
                .b = VK_COMPONENT_SWIZZLE_B,
                .a = VK_COMPONENT_SWIZZLE_A,
            },
            .image = pSwapchainImages[i],
            .subresourceRange = { 
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .flags = 0,
        };

        pDemo->swapchain_image_resources[i].image = pSwapchainImages[i];

        VK_CHECK( vkCreateImageView(pDemo->device, &color_image_view, NULL, &pDemo->swapchain_image_resources[i].view) );
    }

    free(pSwapchainImages);
}


static void vk_setPresentMode(struct demo * const pDemo)
{
    // The FIFO present mode is guaranteed by the spec to be supported
    // and to have no tearing. 

    // There are times when you may wish to use another present mode. 
    // The following code shows how to select them, and the comments 
    // provide some reasons you may wish to use them.
    //
    // It should be noted that Vulkan 1.0 doesn't provide a method for
    // synchronizing rendering with the presentation engine's display.
    // There is a method provided for throttling rendering with the display,
    // but there are some presentation engines for which this method will
    // not work. If an application doesn't throttle its rendering, 
    // and if it renders much faster than the refresh rate of the display,
    // this can waste power on mobile devices. That is because power is
    // being spent rendering images that may never be seen.

    // VK_PRESENT_MODE_IMMEDIATE_KHR is for applications that don't care
    // about tearing, or have some way of synchronizing their rendering
    // with the display.
    // VK_PRESENT_MODE_MAILBOX_KHR may be useful for applications that
    // generally render a new presentable image every refresh cycle, 
    // but are occasionally early. In this case, the application wants 
    // the new image to be displayed instead of the previously-queued-
    // for-presentation image that has not yet been displayed.
    // VK_PRESENT_MODE_FIFO_RELAXED_KHR is for applications that generally
    // render a new presentable image every refresh cycle, but are occasionally
    // late.  In this case (perhaps because of stuttering/latency concerns),
    // the application wants the late image to be immediately displayed,
    // even though that may mean some tearing.   
    uint32_t presentModeCount = 0;
    
	VK_CHECK( pFn_vkhr.fpGetPhysicalDeviceSurfacePresentModesKHR( 
				pDemo->gpu, pDemo->surface, &presentModeCount, NULL) );
    
	VkPresentModeKHR* const presentModes = (VkPresentModeKHR *) 
		malloc( presentModeCount * sizeof(VkPresentModeKHR) );
    
	printf("presentModeCount: %d\n", presentModeCount);

    assert(presentModes > 0);
    
    VK_CHECK( pFn_vkhr.fpGetPhysicalDeviceSurfacePresentModesKHR(
		pDemo->gpu, pDemo->surface, &presentModeCount, presentModes) );



    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;


    for (size_t i = 0; i < presentModeCount; ++i)
    {
        if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            printf("presentMode: VK_PRESENT_MODE_IMMEDIATE_KHR \n");
            break;
        }
        else if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            printf("presentMode: VK_PRESENT_MODE_MAILBOX_KHR \n");
            break;
        }
    }


    pDemo->presentMode = swapchainPresentMode;

    free(presentModes);
}


void init_vk_swapchain(struct demo * const pDemo)
{
//  Create a WSI surface for the window:
    vk_create_surface(pDemo);

    vk_selectGraphicAndPresentQueue(pDemo);

    vk_create_device(pDemo);

    
    vkGetDeviceQueue(pDemo->device, pDemo->graphics_queue_family_index, 0, &pDemo->graphics_queue);
    if (!pDemo->separate_present_queue)
    {
        pDemo->present_queue = pDemo->graphics_queue;
    }
    else
    {
        vkGetDeviceQueue(pDemo->device, pDemo->present_queue_family_index, 0, &pDemo->present_queue);
    }


    vk_getDeviceProcAddrKHR(pDemo);

    vk_chooseSurfaceFormat(pDemo);

    vk_create_semaphores(pDemo);
    
    // Get Memory information and properties
    vkGetPhysicalDeviceMemoryProperties(pDemo->gpu, &pDemo->memory_properties);
}


void vk_prepare_buffers(struct demo * const pDemo)
{
    VkSwapchainKHR oldSwapchain = pDemo->swapchain;

    VkSurfaceCapabilitiesKHR surfCapabilities;
    VK_CHECK( pFn_vkhr.fpGetPhysicalDeviceSurfaceCapabilitiesKHR(pDemo->gpu, pDemo->surface, &surfCapabilities) );

	printf("Check the surface capabilities and formats. \n");


    VkExtent2D swapchainExtent;
    // width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
    if (surfCapabilities.currentExtent.width == 0xFFFFFFFF)
    {
        // If the surface size is undefined, the size is set to the size
        // of the images requested, which must fit within the minimum and
        // maximum values.
        swapchainExtent.width = pDemo->width;
        swapchainExtent.height = pDemo->height;

        if (swapchainExtent.width < surfCapabilities.minImageExtent.width)
        {
            swapchainExtent.width = surfCapabilities.minImageExtent.width;
        }
        else if (swapchainExtent.width > surfCapabilities.maxImageExtent.width)
        {
            swapchainExtent.width = surfCapabilities.maxImageExtent.width;
        }

        if (swapchainExtent.height < surfCapabilities.minImageExtent.height)
        {
            swapchainExtent.height = surfCapabilities.minImageExtent.height;
        }
        else if (swapchainExtent.height > surfCapabilities.maxImageExtent.height)
        {
            swapchainExtent.height = surfCapabilities.maxImageExtent.height;
        }
    }
    else
    {
        // If the surface size is defined, the swap chain size must match
        swapchainExtent = surfCapabilities.currentExtent;
        pDemo->width = surfCapabilities.currentExtent.width;
        pDemo->height = surfCapabilities.currentExtent.height;
    }

    if (pDemo->width == 0 || pDemo->height == 0)
    {
        pDemo->is_minimized = true;
        return;
    }
    else
    {
        pDemo->is_minimized = false;
    }

	printf("surface size: %d, %d. \n", pDemo->width, pDemo->height);


    vk_setPresentMode(pDemo);

        
    // Determine the number of VkImages to use in the swap chain.
    // Application desires to acquire 3 images at a time for triple buffering
    uint32_t desiredNumOfSwapchainImages = 3;

    if (desiredNumOfSwapchainImages < surfCapabilities.minImageCount)
    {
        desiredNumOfSwapchainImages = surfCapabilities.minImageCount;
    }
    // If maxImageCount is 0, we can ask for as many images as we want;
    // otherwise we're limited to maxImageCount
    if ((surfCapabilities.maxImageCount > 0) && (desiredNumOfSwapchainImages > surfCapabilities.maxImageCount))
    {
        // Application must settle for fewer images than desired:
        desiredNumOfSwapchainImages = surfCapabilities.maxImageCount;
    }

	printf("desiredNumOfSwapchainImages: %d\n", desiredNumOfSwapchainImages);


    VkSurfaceTransformFlagsKHR preTransform;
    if (surfCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        
        printf("preTransform: VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR \n");

    }
	else
	{
        preTransform = surfCapabilities.currentTransform;
        printf("preTransform: surfCapabilities.currentTransform \n");
    }

    // Find a supported composite alpha mode - one of these is guaranteed to be set
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };

    
    for (uint32_t i = 0; i < ARRAY_SIZE(compositeAlphaFlags); ++i)
    {
        if (surfCapabilities.supportedCompositeAlpha & compositeAlphaFlags[i])
        {
            compositeAlpha = compositeAlphaFlags[i];
            break;
        }
    }


	printf("compositeAlpha: %d\n", compositeAlpha);

    
    VkSwapchainCreateInfoKHR swapchain_info =
    {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .surface = pDemo->surface,
        .minImageCount = desiredNumOfSwapchainImages,
        .imageFormat = pDemo->format,
        .imageColorSpace = pDemo->color_space,
        .imageExtent = {
            .width = swapchainExtent.width,
            .height = swapchainExtent.height, },
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = preTransform,
        .compositeAlpha = compositeAlpha,
        .imageArrayLayers = 1,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .presentMode = pDemo->presentMode,
        .oldSwapchain = oldSwapchain,
        .clipped = true,
    };
    
    
    VK_CHECK( pFn_vkhr.fpCreateSwapchainKHR(pDemo->device, &swapchain_info, NULL, &pDemo->swapchain) );


    // If we just re-created an existing swapchain, 
    // we should destroy the old swapchain at this point.
    // Note: destroying the swapchain also cleans up all its associated
    // presentable images once the platform is done with them.
    if (oldSwapchain != VK_NULL_HANDLE)
    {
        printf(" Destroy old Swapchain.  \n ");
        pFn_vkhr.fpDestroySwapchainKHR(pDemo->device, oldSwapchain, NULL);
    }

    //////////////////////////////
    vk_create_swapchain_view(pDemo);
/*
    if(oldSwapchainImgRes)
    {
        free(oldSwapchainImgRes);
        printf(" free old Swapchain. \n ");        
    }
*/
}
