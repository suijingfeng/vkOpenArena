#ifndef VK_INSTANCE_H_
#define VK_INSTANCE_H_

#include "VKimpl.h"
#include <stdio.h>



// Initializes VK_Instance structure.
void vk_getProcAddress(void);
void vk_clearProcAddress(void);

#ifndef NDEDBG

const char * cvtResToStr(VkResult result);

#define VK_CHECK(function_call) { \
	VkResult result = function_call; \
	if (result != VK_SUCCESS) \
		fprintf(stderr, "Vulkan: error %s returned by %s \n", cvtResToStr(result), #function_call); \
}

#else

#define VK_CHECK(function_call)	\
	function_call;

#endif


#define MAX_SWAPCHAIN_IMAGES    8

// Vk_Instance contains engine-specific vulkan resources that persist entire renderer lifetime.
// This structure is initialized/deinitialized by vk_initialize/vk_shutdown functions correspondingly.
struct Vk_Instance {
	VkInstance instance ;
	VkPhysicalDevice physical_device;

    // Native platform surface or window objects are abstracted by surface objects,
    // which are represented by VkSurfaceKHR handles. The VK_KHR_surface extension
    // declares the VkSurfaceKHR object, and provides a function for destroying
    // VkSurfaceKHR objects. Separate platform-specific extensions each provide a
    // function for creating a VkSurfaceKHR object for the respective platform.
	VkSurfaceKHR surface;
	VkSurfaceFormatKHR surface_format;
    VkSurfaceCapabilitiesKHR surface_caps;

// Depth/stencil formats are considered opaque and need not be stored
// in the exact number of bits pertexel or component ordering indicated
// by the format enum. However, implementations must not substitute a
// different depth or stencil precision than that described in the
// format (e.g. D16 must not be implemented as D24 or D32).

// The features for the set of formats (VkFormat) supported by the
// implementation are queried individually using the 
// vkGetPhysicalDeviceFormatProperties command.
// To determine the set of valid usage bits for a given format, 
// call vkGetPhysicalDeviceFormatProperties.

// depth and stencil aspects of a given image subresource must always be in the same layout.

    VkFormat fmt_DepthStencil;
	VkPhysicalDeviceMemoryProperties devMemProperties;
	
    uint32_t queue_family_index;
	VkDevice device;
	VkQueue queue;

	VkSwapchainKHR swapchain;
	uint32_t swapchain_image_count ;
	VkImage swapchain_images_array[MAX_SWAPCHAIN_IMAGES];
	VkImageView swapchain_image_views[MAX_SWAPCHAIN_IMAGES];
	uint32_t idx_swapchain_image;


	VkCommandPool command_pool;
	VkCommandBuffer command_buffer;

	VkImage depth_image;
	VkDeviceMemory depth_image_memory;
	VkImageView depth_image_view;

	VkRenderPass render_pass;
	VkFramebuffer framebuffers[MAX_SWAPCHAIN_IMAGES];

	VkDescriptorPool descriptor_pool;
	VkDescriptorSetLayout set_layout;

    // Pipeline layout: the uniform and push values referenced by 
    // the shader that can be updated at draw time
	VkPipelineLayout pipeline_layout;

    VkBool32 isBlitSupported;

    VkBool32 isInitialized;

#ifndef NDEBUG
    VkDebugReportCallbackEXT h_debugCB;
#endif
};



extern struct Vk_Instance vk;



#endif
