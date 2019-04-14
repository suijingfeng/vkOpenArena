#ifndef VK_INSTANCE_H_
#define VK_INSTANCE_H_

#include "vk_common.h"
#include "vk_instance.h"
#include <stdio.h>


// Initializes VK_Instance structure.
void vk_createInstanceAndDevice(void);
void vk_destroyInstanceAndDevice(void);

void vk_createSurfaceImpl(void);


// Vk_Instance contains engine-specific vulkan resources that persist entire renderer lifetime.
// This structure is initialized/deinitialized by vk_initialize/vk_shutdown functions correspondingly.
struct VkInstance_t {
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

    VkBool32 isBlitSupported;

#ifndef NDEBUG
    VkDebugReportCallbackEXT h_debugCB;
#endif
};



extern struct VkInstance_t vk;



#endif
