#include "ref_import.h"
#include "tr_cvar.h"
#include "VKimpl.h"
#include "vk_instance.h"

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

/*

A surface has changed in such a way that it is no longer compatible with the swapchain,
and further presentation requests using the swapchain will fail. Applications must 
query the new surface properties and recreate their swapchain if they wish to continue
presenting to the surface.

VK_IMAGE_LAYOUT_PRESENT_SRC_KHR must only be used for presenting a presentable image
for display. A swapchain's image must be transitioned to this layout before calling
vkQueuePresentKHR, and must be transitioned away from this layout after calling
vkAcquireNextImageKHR.

*/


// vulkan does not have the concept of a "default framebuffer", hence it requires an
// infrastruture that will own the buffers we will render to before we visualize them
// on the screen. This infrastructure is known as the swap chain and must be created
// explicity in vulkan. The swap chain is essentially a queue of images that are 
// waiting to be presented to the screen. The general purpose of the swap chain is to
// synchronize the presentation of images with the refresh rate of the screen.

// 1) Basic surface capabilities (min/max number of images in the swap chain,
//    min/max number of images in the swap chain).
// 2) Surcface formats(pixel format, color space)
// 3) Available presentation modes


void vk_recreateSwapChain(void)
{

    ri.Printf( PRINT_ALL, " Recreate swap chain \n");

    if( r_fullscreen->integer )
    {
        ri.Cvar_Set( "r_fullscreen", "0" );
        r_fullscreen->modified = qtrue;
    }

    // hasty prevent crash.
    ri.Cmd_ExecuteText (EXEC_NOW, "vid_restart\n");
}


// create swap chain
void vk_createSwapChain(VkDevice device, VkSurfaceKHR HSurface, 
        VkSurfaceFormatKHR surface_format, VkPresentModeKHR presentMode,
        VkSwapchainKHR * const pHSwapChain)
{

    VkSurfaceCapabilitiesKHR surfCaps;

    
    // To query the basic capabilities of a surface, needed in order to create a swapchain
	VK_CHECK( qvkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.physical_device, HSurface, &surfCaps) );

    // VK_IMAGE_USAGE_TRANSFER_DST_BIT is required by image clear operations.
	if( (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == 0 )
		ri.Error(ERR_FATAL, "VK_IMAGE_USAGE_TRANSFER_DST_BIT is not supported by you GPU.\n");

	// VK_IMAGE_USAGE_TRANSFER_SRC_BIT is required in order to take screenshots.
	if( (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) == 0 )
		ri.Error(ERR_FATAL, "VK_IMAGE_USAGE_TRANSFER_SRC_BIT is not supported by you GPU.\n");

    // vk.surface_caps = surCaps;
    
    // The swap extent is the resolution of the swap chain images and its almost 
    // always exactly equal to the resolution of the window that we're drawing to.

    VkExtent2D image_extent = surfCaps.currentExtent;
    if ( (image_extent.width == 0xffffffff) && (image_extent.height == 0xffffffff))
    {
        image_extent.width = MIN( surfCaps.maxImageExtent.width, 
                MAX(surfCaps.minImageExtent.width, 640u) );
        image_extent.height = MIN( surfCaps.maxImageExtent.height, 
                MAX(surfCaps.minImageExtent.height, 480u) );
    }

    ri.Printf(PRINT_ALL, " image_extent.width: %d, image_extent.height: %d\n",
            image_extent.width, image_extent.height);


    // The number of images in the swap chain, essentially the present queue length
    // The implementation specifies the minimum amount of images to functions properly
    //
    // The Spec Say:
    // image_count must <= VkSurfaceCapabilitiesKHR.maxImageCount
    // image_count must >= VkSurfaceCapabilitiesKHR.minImageCount

    // maxImageCount is the maximum number of images the specified device
    // supports for a swapchain created for the surface, and will be either 0,
    // or greater than or equal to minImageCount. A value of 0 means that 
    // there is no limit on the number of images, though there may be limits 
    // related to the total amount of memory used by presentable images.

    // Formulas such as min(N, maxImageCount) are not correct, 
    // since maxImageCount may be zero.
    //
	// determine the swapchain image count
    uint32_t image_count;
    if (surfCaps.maxImageCount == 0)
    {
        image_count = MAX_SWAPCHAIN_IMAGES;
    }
    else
    {
        switch (presentMode)
        {
            case VK_PRESENT_MODE_MAILBOX_KHR:
                image_count = MAX(3u, surfCaps.minImageCount); break;

            case VK_PRESENT_MODE_FIFO_KHR:
            case VK_PRESENT_MODE_IMMEDIATE_KHR:
            default:
                image_count = MAX(2u, surfCaps.minImageCount); break;
        }

        // minImageCount must be less than or equal to the value returned in
        // the maxImageCount member of VkSurfaceCapabilitiesKHR the structure
        // returned by vkGetPhysicalDeviceSurfaceCapabilitiesKHR for the surface 
        // if the returned maxImageCount is not zero
        //
        // minImageCount must be greater than or equal to the value returned in
        // the minImageCount member of VkSurfaceCapabilitiesKHR the structure 
        // returned by vkGetPhysicalDeviceSurfaceCapabilitiesKHR for the surface

        image_count = MIN(image_count+1, surfCaps.maxImageCount);
    }

    ri.Printf(PRINT_ALL, " \n minImageCount: %d, maxImageCount: %d, setted: %d\n",
            surfCaps.minImageCount, surfCaps.maxImageCount, image_count);



    ri.Printf(PRINT_ALL, "\n Create vk.swapchain. \n");
    // Regardless of which platform you're running on, the resulting
    // VkSurfaceKHR handle refers to Vulkan's view of a window, In order
    // to actually present anything to that surface, it's necessary
    // to create a special image that can be used to store the data
    // in window. On most platforms, this type of image is either owned
    // by or tightly integrated with the window system, so rather 
    // creating a normal vulkan image object, we use a second object
    // called swapchain to manage one or more image objects.
    //
    // swapchain objects are used to ask the native window system to
    // created one or more images that can be used to present into a
    // Vulkan surface. each swapchain object manages a set of images,
    // usually in some form of ring buffer. 
    //
    // The application can ask the the swapchain for the next available
    // image, render into it, and then hand the image back to the 
    // swap chain ready for display. By managing presentable images
    // in a ring or queue, one image can be presented to the diaplay
    // while another is being drawn to by the application, overlapping
    // the operation of the window system and application.
    // 


    VkSwapchainCreateInfoKHR desc;
    desc.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    desc.pNext = NULL;
    desc.flags = 0;
    desc.surface = HSurface;
    // minImageCount is the minimum number of presentable images that the application needs.
    // The implementation will either create the swapchain with at least that many images, 
    // or it will fail to create the swapchain.
    desc.minImageCount = image_count;
    desc.imageFormat = surface_format.format;
    desc.imageColorSpace = surface_format.colorSpace;
    desc.imageExtent = image_extent;
    desc.imageArrayLayers = 1;

    // render images to a separate image first to perform operations like post-processing
    desc.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | 
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | 
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    // An image is owned by one queue family at a time and ownership
    // must be explicitly transfered before using it in an another
    // queue family. This option offers the best performance.
    desc.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    desc.queueFamilyIndexCount = 0;
    desc.pQueueFamilyIndices = NULL;

    // we can specify that a certain transform should be applied to
    // images in the swap chain if it is support, like a 90 degree
    // clockwise rotation  or horizontal flip, To specify that you
    // do not want any transformation, simply dprcify the current
    // transformation
    desc.preTransform = surfCaps.currentTransform;

    // The compositeAlpha field specifies if the alpha channel
    // should be used for blending with other windows int the
    // windows system. 
    desc.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    desc.presentMode = presentMode;

    // we don't care about the color of pixels that are obscured.
    desc.clipped = VK_TRUE;

    // With Vulkan it's possible that your swap chain becomes invalid or unoptimized
    // while your application is running, for example because the window was resized.
    // In that case the swap chain actually needs to be recreated from scratch and a
    // reference to the old one must be specified in this field.
    desc.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK( qvkCreateSwapchainKHR(device, &desc, NULL, pHSwapChain) );
}

void vk_destroySwapChain(void)
{
    qvkDestroySwapchainKHR(vk.device, vk.swapchain, NULL);
    ri.Printf(PRINT_ALL, " Destroy vk.swapchain. \n");
}
