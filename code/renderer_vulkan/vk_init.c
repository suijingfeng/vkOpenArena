#include "VKimpl.h"
#include "vk_instance.h"
#include "vk_frame.h"
#include "vk_cmd.h"
#include "vk_pipelines.h"
#include "vk_shade_geometry.h"
#include "vk_shaders.h"
#include "vk_descriptor_sets.h"
#include "vk_swapchain.h"
#include "ref_import.h" 
#include "vk_validation.h"
#include "tr_cvar.h"
#include "vk_utils.h"


//
// Just list a few feature the color surface format supported
//
static void VK_CheckSurfaceCapabilities(VkPhysicalDevice hGPU, const VkSurfaceFormatKHR * const pSurfFmt)
{
	// To query supported format features which are properties of the physical device
	ri.Printf(PRINT_ALL, "\n --------  Query supported features of the color surfce --------\n");

	VkFormatProperties props;


	// To determine the set of valid usage bits for a given format,
	// ========================= color ================
	NO_CHECK( qvkGetPhysicalDeviceFormatProperties(hGPU, pSurfFmt->format, &props) );

	// Check if the device supports blitting to linear images 
	if (props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
	{
		ri.Printf(PRINT_ALL, " Linear Tiling Features supported. \n");
	}
    
	if (props.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)
	{
		ri.Printf(PRINT_ALL, " Blitting from linear tiled images supported.\n");
	}


	ri.Printf(PRINT_ALL, " -------- --------------------------- --------\n\n");
}


// setting the format of the color surface 
// why BGR not RGB ??? 
static void VK_SelectColorSurfaceFormat(VkPhysicalDevice hGPU, VkSurfaceKHR hSurface, VkSurfaceFormatKHR * const pSurfmt)
{
	uint32_t nSurfmt;

	// Get the numbers of VkFormat's that are supported
	// "vk.surface" is the surface that will be associated with the swapchain.
	// "vk.surface" must be a valid VkSurfaceKHR handle
	VK_CHECK( qvkGetPhysicalDeviceSurfaceFormatsKHR(hGPU, hSurface, &nSurfmt, NULL));
	
	if(nSurfmt <= 0)
		ri.Error(ERR_FATAL, " numbers of VkFormat's that are supported is %d", nSurfmt);
		

	VkSurfaceFormatKHR * pSurfFmts =
		(VkSurfaceFormatKHR *) malloc(nSurfmt * sizeof(VkSurfaceFormatKHR));

	// To query the supported swapchain format-color space pairs for a surface
	VK_CHECK( qvkGetPhysicalDeviceSurfaceFormatsKHR(hGPU, hSurface, &nSurfmt, pSurfFmts) );

	ri.Printf(PRINT_ALL, " -------- Total %d surface formats supported. -------- \n", nSurfmt);

	for (uint32_t i = 0; i < nSurfmt; ++i)
	{
		ri.Printf(PRINT_ALL, " [%d] format: %s, color space: %s \n",
			i, VkFormatEnum2str(pSurfFmts[i].format), ColorSpaceEnum2str(pSurfFmts[i].colorSpace));
	}


	// If the format list includes just one entry of VK_FORMAT_UNDEFINED, the surface
	// has no preferred format. Otherwise, at least one supported format will be returned.
	if ((nSurfmt == 1) && (pSurfFmts[0].format == VK_FORMAT_UNDEFINED))
	{
		// special case that means we can choose any format
		pSurfmt->format = VK_FORMAT_B8G8R8A8_UNORM;
		pSurfmt->colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		ri.Printf(PRINT_ALL, "VK_FORMAT_R8G8B8A8_UNORM\n");
		ri.Printf(PRINT_ALL, "VK_COLORSPACE_SRGB_NONLINEAR_KHR\n");
	}
	else
	{
		uint32_t i;

		ri.Printf(PRINT_ALL, " we choose: \n");

		for( i = 0; i < nSurfmt; ++i)
		{
			if ((pSurfFmts[i].format == VK_FORMAT_B8G8R8A8_UNORM) &&
				(pSurfFmts[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR))
			{

				ri.Printf(PRINT_ALL, " format = VK_FORMAT_B8G8R8A8_UNORM \n");
				ri.Printf(PRINT_ALL, " colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR \n");

				*pSurfmt = pSurfFmts[i];

				break;
			}
		}

		if (i == nSurfmt)
		{
		        // if get here, just grab the first ... 
			*pSurfmt = pSurfFmts[0];
		}
	}

	ri.Printf(PRINT_ALL, " --- ----------------------------------- --- \n");

	VK_CheckSurfaceCapabilities( hGPU, pSurfmt );

	free(pSurfFmts);
}


// Set the depth format ...
static void VK_SelectDepthSurfaceFormat(VkPhysicalDevice hGPU, VkFormat* const pDepthStencilFmt)
{
	VkFormatProperties props;

	//=========================== depth =====================================
	NO_CHECK( qvkGetPhysicalDeviceFormatProperties(hGPU, VK_FORMAT_D24_UNORM_S8_UINT, &props) );
	
	if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		ri.Printf(PRINT_ALL, " VK_FORMAT_D24_UNORM_S8_UINT optimal Tiling feature supported.\n");
		*pDepthStencilFmt = VK_FORMAT_D24_UNORM_S8_UINT;
	}
	else
	{
		NO_CHECK( qvkGetPhysicalDeviceFormatProperties(hGPU, VK_FORMAT_D32_SFLOAT_S8_UINT, &props) );

		if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			ri.Printf(PRINT_ALL, " VK_FORMAT_D32_SFLOAT_S8_UINT optimal Tiling feature supported.\n");
			*pDepthStencilFmt = VK_FORMAT_D32_SFLOAT_S8_UINT;
		}
		else
		{
			//formats[0] = VK_FORMAT_X8_D24_UNORM_PACK32;
			//formats[1] = VK_FORMAT_D32_SFLOAT;
			// never get here.
			ri.Error(ERR_FATAL, " Failed to find a depth attachment format.\n");
		}
	}
}



static VkPresentModeKHR VK_SelectPresentationMode(VkPhysicalDevice hGPU, VkSurfaceKHR HSurface)
{
        // The presentation is arguably the most important setting for the swap chain
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
        uint32_t nPM = 0;

        VkBool32 mailbox_supported = VK_FALSE;
        VkBool32 immediate_supported = VK_FALSE;

        // Look for the best mode available.
        // suijingfeng: we don't give the user a choice, because its seems
        // that no big difference between mailbox and immediate mode
        VK_CHECK( qvkGetPhysicalDeviceSurfacePresentModesKHR(hGPU, HSurface, &nPM, NULL) );

        assert(nPM > 0);

        VkPresentModeKHR * pPresentModes = (VkPresentModeKHR *) malloc(nPM * sizeof(VkPresentModeKHR));

        VK_CHECK( qvkGetPhysicalDeviceSurfacePresentModesKHR(hGPU, HSurface, &nPM, pPresentModes) );

        ri.Printf(PRINT_ALL, "-------- Total %d present mode supported. -------- \n", nPM);
        for (uint32_t i = 0; i < nPM; ++i)
        {
                switch (pPresentModes[i])
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

        ri.Printf(PRINT_ALL, "\n");
        if (mailbox_supported)
        {
                ri.Printf(PRINT_ALL, " Presentation with VK_PRESENT_MODE_MAILBOX_KHR mode. \n");
                ri.Printf(PRINT_ALL, "-------- ----------------------------- --------\n");
                return VK_PRESENT_MODE_MAILBOX_KHR;
        }
        else if (immediate_supported)
        {
                ri.Printf(PRINT_ALL, " Presentation with VK_PRESENT_MODE_IMMEDIATE_KHR mode. \n");
                ri.Printf(PRINT_ALL, "-------- ----------------------------- --------\n");
                return VK_PRESENT_MODE_IMMEDIATE_KHR;
        }

        // FIFO_KHR mode is guaranteed to be available according to the spec.
        // this is worsest, lag...
        ri.Printf(PRINT_ALL, " Presentation with VK_PRESENT_MODE_FIFO_KHR mode. \n");
        ri.Printf(PRINT_ALL, "-------- ----------------------------- --------\n");
        return VK_PRESENT_MODE_FIFO_KHR;
}


static void VK_SelectPhysicalDevice(VkInstance hInstance, VkPhysicalDevice* const pGPU)
{
        // After initializing the Vulkan library through a VkInstance
        // we need to look for and select a graphics card in the system
        // that supports the features we need. In fact we can select any
        // number of graphics cards and use them simultaneously.
        uint32_t cntGpu = 0;

        // Initial call to query cntGpu, then second call for gpu info.
        VK_CHECK( qvkEnumeratePhysicalDevices(hInstance, &cntGpu, NULL) );

        if (cntGpu <= 0) {
                ri.Error(ERR_FATAL, "Vulkan: no physical device found");
        }

        VkPhysicalDevice * pPhyDevArray = (VkPhysicalDevice *) malloc(sizeof(VkPhysicalDevice) * cntGpu);


        VK_CHECK( qvkEnumeratePhysicalDevices(hInstance, &cntGpu, pPhyDevArray) );
        // Select the right gpu from r_gpuIndex

        if (cntGpu == 1)
        {
                // we have only one GPU, no choice ...
                *pGPU = pPhyDevArray[0];
                r_gpuIndex->integer = 1;
        }
        else
        {
                // let the user make the choice, we simply do a out of range check ...
                if (r_gpuIndex->integer < 0)
                {
                        r_gpuIndex->integer = 0;
                }
                else if (r_gpuIndex->integer >= cntGpu)
                {
                        r_gpuIndex->integer = cntGpu - 1;
                }

                *pGPU = pPhyDevArray[r_gpuIndex->integer];
        }

        // free the memory allocated from the heap
        free(pPhyDevArray);

        ri.Printf(PRINT_ALL, " Total %d graphics card, selected card index: [%d]. \n",
                        cntGpu, r_gpuIndex->integer);

}


// Vulkan device execute work that is submitted to queues.
// each device will have one or more queues, and each of those queues
// will belong to one of the device's queue families.
// A queue family is a group of queues that have identical capabilities
// but are able to run in parallel. The number of queue families,
// the capabilities of each family, and the number of queues
// belonging to each family are all properties of the physical device.
static uint32_t VK_SelectQueueFamilyForPresentation(VkPhysicalDevice hGPU, VkSurfaceKHR HSurface)
{
        // Almosty every operation in Vulkan, anything from drawing textures,
        // requires commands to be submitted to a queue. There are different
        // types of queues that originate from differnet queue families and
        // each family of queues allows only a subset of commands.
        // For example, there could be a queue family allows processing of
        // compute commands or one that only allows memory thansfer related commands.
        //
        // We need to check which queue families are supported by the device
        // and which one of these supports the commands that we use.

        uint32_t cntQueueFamily;

        NO_CHECK( qvkGetPhysicalDeviceQueueFamilyProperties(hGPU, &cntQueueFamily, NULL) );

        assert(cntQueueFamily > 0);

        VkQueueFamilyProperties* const pQueueFamilies = (VkQueueFamilyProperties *)
                malloc(cntQueueFamily * sizeof(VkQueueFamilyProperties));

        // To query properties of queues available on a physical device
        NO_CHECK( qvkGetPhysicalDeviceQueueFamilyProperties(hGPU, &cntQueueFamily, pQueueFamilies) );

        ri.Printf(PRINT_ALL, "\n -------- Total %d Queue families -------- \n", cntQueueFamily);

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
        for (uint32_t i = 0; i < cntQueueFamily; ++i)
        {
                // print every queue family's capability
                ri.Printf(PRINT_ALL, " Queue family [%d] have %d queues, support: ",
                                i, pQueueFamilies[i].queueCount);

                if (pQueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                        ri.Printf(PRINT_ALL, " Graphic ");

                if (pQueueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                        ri.Printf(PRINT_ALL, " Compute ");

                if (pQueueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
                        ri.Printf(PRINT_ALL, " Transfer ");

                if (pQueueFamilies[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
                        ri.Printf(PRINT_ALL, " Sparse ");


                VkBool32 isPresentSupported = VK_FALSE;
                VK_CHECK( qvkGetPhysicalDeviceSurfaceSupportKHR(
                                        hGPU, i, HSurface, &isPresentSupported) );

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

        uint32_t idx;
        for (idx = 0; idx < cntQueueFamily; ++idx)
        {
                // To look for a queue family that has the capability of presenting
                // to our window surface

                VkBool32 presentation_supported = VK_FALSE;
                VK_CHECK( qvkGetPhysicalDeviceSurfaceSupportKHR(
                                        hGPU, idx, HSurface, &presentation_supported) );

                if (presentation_supported &&
                                (pQueueFamilies[idx].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
                {
                        ri.Printf(PRINT_ALL, " Queue family index %d selected for presentation.\n", idx);

                        // free memory before leave, else memory leak !!!
                        break;
                }
        }

        free( pQueueFamilies );


        if (idx == cntQueueFamily)
        {
                // Intel interated GPU will crash on linux vulkan driver ...
                ri.Error(ERR_FATAL, " Failed to find a queue family for presentation. ");
        }

        ri.Printf(PRINT_ALL, " -------- ---------------------------- -------- \n\n");

        return idx;
}



static void VK_CheckSwapChainExtention(VkPhysicalDevice hGPU, const char * const pName)
{
        // Look for device extensions
        //
        // Not all graphics cards are capble of presenting images directly to a screen
        // for various reasons, for example because they are designed for servers and 
        // don't have any display outputs.
        //
        // Secondly, since image presentation is heavily tied into the window system
        // and the surfaces associated with windows, it is not actually part of the
        // vulkan core. You have to enable the VK_KHR_swapchain device extension
        // after querying for its support.
        //
        uint32_t cntDevExts = 0;

        VkBool32 swapchainExtFound = 0;

        // To query the numbers of extensions available to a given physical device
        ri.Printf(PRINT_ALL, " Check for VK_KHR_swapchain extension. \n");

        VK_CHECK( qvkEnumerateDeviceExtensionProperties(hGPU, NULL, &cntDevExts, NULL) );

        VkExtensionProperties* const pDeviceExt =
                (VkExtensionProperties *) malloc(sizeof(VkExtensionProperties) * cntDevExts);

        VK_CHECK( qvkEnumerateDeviceExtensionProperties(hGPU, NULL, &cntDevExts, pDeviceExt) );

        // info
        ri.Printf(PRINT_ALL, "-------- Total %d device extensions supported --------\n", cntDevExts);
        for (uint32_t j = 0; j < cntDevExts; ++j)
        {
                ri.Printf(PRINT_ALL, "%s \n", pDeviceExt[j].extensionName);
        }
        ri.Printf(PRINT_ALL, "-------- Enabled device extensions on this app --------\n");

        ri.Printf(PRINT_ALL, " %s \n", pName);

        ri.Printf(PRINT_ALL, "-------- --------------------------------------- ------\n\n");


        for (uint32_t j = 0; j < cntDevExts; ++j)
        {
                if (0 == strcmp(pName, pDeviceExt[j].extensionName))
                {
                        swapchainExtFound = VK_TRUE;
                        break;
                }
        }

        free(pDeviceExt);

        if (VK_FALSE == swapchainExtFound)
        {
                ri.Error(ERR_FATAL, "VK_KHR_SWAPCHAIN_EXTENSION_NAME is not available on you GPU driver.");
        }
}



static void VK_CreateLogicalDevice( VkPhysicalDevice hGPU,
                                    const char* const* ppExtNamesEnabled,
                                    const uint32_t nExtEnabled,
                                    const uint32_t idxQueueFamily,
                                    VkDevice * const pLogicalDev )
{
        uint32_t j;
        const float priority = 1.0;

        for(j = 0; j < nExtEnabled; ++j)
        {
                VK_CheckSwapChainExtention(hGPU, ppExtNamesEnabled[j]);
        }

        VkDeviceQueueCreateInfo queue_desc;
        queue_desc.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_desc.pNext = NULL;
        queue_desc.flags = 0;
        queue_desc.queueFamilyIndex = idxQueueFamily;
        queue_desc.queueCount = 1;
        queue_desc.pQueuePriorities = &priority;


        // Query fine-grained feature support for this physical device.
        // If APP has specific feature requirements it should check supported
        // features based on this query.

        VkPhysicalDeviceFeatures features;

        NO_CHECK( qvkGetPhysicalDeviceFeatures(hGPU, &features) );

        if (features.shaderClipDistance == VK_FALSE)
        {
                ri.Error(ERR_FATAL,
                                "vk_create_device: shaderClipDistance feature is not supported");
                // vulkan need this to render portal and mirrors
                // wandering if we can provide a soft impl ...
        }
        if (features.fillModeNonSolid == VK_FALSE) {
                ri.Error(ERR_FATAL,
                                "vk_create_device: fillModeNonSolid feature is not supported");
        }

        VkDeviceCreateInfo device_desc;
        device_desc.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_desc.pNext = NULL;
        device_desc.flags = 0;
        device_desc.queueCreateInfoCount = 1;
        // Creating a logical device also creates the queues associated with that device.
        device_desc.pQueueCreateInfos = &queue_desc;
        device_desc.enabledLayerCount = 0;
        device_desc.ppEnabledLayerNames = NULL;
        device_desc.enabledExtensionCount = nExtEnabled;
        device_desc.ppEnabledExtensionNames = ppExtNamesEnabled;
        device_desc.pEnabledFeatures = &features;


        // After selecting a physical device to use we need to set up
        // a logical device to interface with it. The logical device
        // creation process is similar to the instance creation process
        // and describes the features we want to use. We also need to
        // specify which queues to create now that we've queried which
        // queue families are available. You can create multiple logical
        // devices from the same physical device if you have varying
        // requirements.
        ri.Printf(PRINT_ALL, " Create logical device: vk.device \n");
        VK_CHECK( qvkCreateDevice(hGPU, &device_desc, NULL, pLogicalDev) );
}


static void VK_LoadDeviceFunctions(VkDevice hLogicalDevice)
{
        ri.Printf(PRINT_ALL, " Loading device level function. \n");

#define INIT_DEVICE_FUNCTION(func)                                              \
        q##func = (PFN_ ## func)qvkGetDeviceProcAddr(hLogicalDevice, #func);    \
        if (q##func == NULL) {                                                  \
                ri.Error(ERR_FATAL, "Failed to find entrypoint %s", #func);     \
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
        INIT_DEVICE_FUNCTION(vkCmdClearColorImage)
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


// vk_init have nothing to do with tr_init
// vk_instance should be small
// 
// After calling this function we get fully functional vulkan subsystem.
// This function is responsible for initializing a valid Vulkan subsystem.
void vk_initialize(void * pWinContext)
{
        VK_CreateInstance(&vk.instance);

        // Create debug callback.
        VK_SettingDebugCallback(vk.instance);

        // The window surface needs to be created right after the instance creation,
        // because it can actually influence the presentation mode selection.

        VK_CreateSurfaceImpl(vk.instance, pWinContext, &vk.surface);

        // select physical device
        VK_SelectPhysicalDevice(vk.instance, &vk.physical_device);

        VK_SelectColorSurfaceFormat( vk.physical_device, vk.surface, &vk.surface_format);

        VK_SelectDepthSurfaceFormat( vk.physical_device, &vk.fmt_DepthStencil);


        vk.present_mode = VK_SelectPresentationMode(vk.physical_device, vk.surface);

        vk.queue_family_index = VK_SelectQueueFamilyForPresentation(vk.physical_device, vk.surface);


        //////////
        const char* enable_features_array[1] =
        {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };


        VK_CreateLogicalDevice(vk.physical_device, enable_features_array, 1, vk.queue_family_index, &vk.device);

        // Get device level functions. depended on the created logical device
        // thus must be called AFTER vk_createLogicalDevice.
        VK_LoadDeviceFunctions(vk.device);


        // a call to retrieve queue handle
        // The queues are constructed when the device is created,
        // For this reason, we don't create queues, but obtain them
        // from the device. Maybe the queue is abstraction of
        // specific hardware ...
        NO_CHECK( qvkGetDeviceQueue(vk.device, vk.queue_family_index, 0, &vk.queue) );
        //
        //     Queue Family Index
        //
        // The queue family index is used in multiple places in Vulkan
        // in order to tie operations to a specific family of queues.
        // When retrieving a handle to the queue via vkGetDeviceQueue,
        // the queue family index is used to select which queue family
        // to retrieve the VkQueue handle from.
        //
        // When creating a VkCommandPool object, a queue family index
        // is specified in the VkCommandPoolCreateInfo structure.
        // Command buffers from this pool can only be submitted on queues
        // corresponding to this queue family.
        //
        // When creating VkImage (see Images) and VkBuffer (see Buffers)
        // resources, a set of queue families is included in the
        // VkImageCreateInfo and VkBufferCreateInfo structures to specify
        // the queue families that can access the resource.
        //
        // When inserting a VkBufferMemoryBarrier or VkImageMemoryBarrier
        // a source and destination queue family index is specified to allow
        // the ownership of a buffer or image to be transferred from one
        // queue family to another.

        // Swapchain. vk.physical_device required to be init.
        vk_createSwapChain(vk.device, vk.surface, vk.surface_format, vk.present_mode,
                        &vk.swapchain);


        vk_createColorAttachment(vk.device, vk.swapchain, vk.surface_format.format,
                        &vk.swapchain_image_count, vk.color_image_views);


        // Sync primitives.
        vk_create_sync_primitives();

        // we have to create a command pool before we can create command buffers
        // command pools manage the memory that is used to store the buffers and
        // command buffers are allocated from them.
        vk_create_command_pool( &vk.command_pool );

        ri.Printf(PRINT_ALL, " Create command buffer: vk.command_buffer. \n");
        vk_create_command_buffer(vk.command_pool, &vk.command_buffer);

        ri.Printf(PRINT_ALL, " Create command buffer: vk.tmpRecordBuffer. \n");
        vk_create_command_buffer(vk.command_pool, &vk.tmpRecordBuffer);

        vk_createDepthAttachment(vk.renderArea.extent.width, vk.renderArea.extent.height, vk.fmt_DepthStencil);
        // Depth attachment image.
        vk_createRenderPass(vk.device, vk.surface_format.format, vk.fmt_DepthStencil, &vk.render_pass);
        vk_createFrameBuffers(vk.renderArea.extent.width, vk.renderArea.extent.height,
                        vk.render_pass, vk.swapchain_image_count, vk.framebuffers );

        // Pipeline layout.
        // You can use uniform values in shaders, which are globals similar to
        // dynamic state variables that can be changes at the drawing time to
        // alter the behavior of your shaders without having to recreate them.
        // They are commonly used to create texture samplers in the fragment 
        // shader. The uniform values need to be specified during pipeline
        // creation by creating a VkPipelineLayout object.

        // MAX_DRAWIMAGES = 2048
        vk_createDescriptorPool(2048, &vk.descriptor_pool);
        // The set of sets that are accessible to a pipeline are grouped into 
        // pipeline layout. Pipelines are created with respect to this pipeline
        // layout. Those descriptor sets can be bound into command buffers 
        // along with compatible pipelines to allow those pipeline to access
        // the resources in them.
        vk_createDescriptorSetLayout(&vk.set_layout);
        // These descriptor sets layouts are aggregated into a single pipeline layout.
        vk_createPipelineLayout(vk.set_layout, &vk.pipeline_layout);

        //
        vk_createVertexBuffer();
        vk_createIndexBuffer();
        // vk_createScreenShotBuffer(vk.renderArea.extent.width * vk.renderArea.extent.height * 8);

        //
        // Shader modules.
        //
        vk_loadShaderModules();

        //
        // Standard pipelines.
        //
        vk_createStandardPipelines();
        //
        // debug pipelines
        //
        vk_createDebugPipelines();

        vk.isInitialized = VK_TRUE;
}


VkBool32 isVKinitialied( void )
{
        return vk.isInitialized;
}

// Shutdown vulkan subsystem by releasing resources acquired by Vk_Instance.
void vk_shutdown( void )
{
        ri.Printf( PRINT_ALL, "vk_shutdown()\n" );

        NO_CHECK( qvkDeviceWaitIdle( vk.device ) );

        // we should delete the framebuffers before the image views
        // and the render pass that they are based on.
        vk_destroyFrameBuffers( vk.framebuffers );

        vk_destroyRenderPass( vk.render_pass );
        vk_destroyDepthAttachment();
        vk_destroyColorAttachment();
        vk_destroySwapChain();


        vk_destroy_shading_data();


        vk_destroy_sync_primitives();

        vk_destroyShaderModules();

        // Those pipelines can be used across different maps ?
        // so we only destroy it when the client quit.
        vk_destroyGlobalStagePipeline();
        vk_destroyDebugPipelines();

        vk_destroy_pipeline_layout();

        vk_destroy_descriptor_pool();

        ri.Printf( PRINT_ALL, " Free command buffers: vk.command_buffer. \n" );
        vk_freeCmdBufs(&vk.command_buffer);
        ri.Printf( PRINT_ALL, " Free command buffers: vk.tmpRecordBuffer. \n" );
        vk_freeCmdBufs(&vk.tmpRecordBuffer);

        vk_destroy_command_pool();

        ri.Printf( PRINT_ALL, " Destroy logical device: vk.device. \n" );
        // Device queues are implicitly cleaned up when the device is destroyed
        // so we don't need to do anything in clean up
        qvkDestroyDevice(vk.device, NULL);


        VK_DestroyInstance();

        // ===========================================================

        ri.Printf( PRINT_ALL, " clear vk struct: vk \n" );
}
