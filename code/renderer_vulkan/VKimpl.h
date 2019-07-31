#ifndef VK_IMPL_H_
#define VK_IMPL_H_

/*
==========================================================

      IMPLEMENTATION/PLATFORM SPECIFIC FUNCTIONS

Different platform use different libs, therefore can have
different implementation, however the interface should be
consistant.
==========================================================
*/

#define VK_NO_PROTOTYPES
#include "../vulkan/vulkan.h"
#include "../vulkan/vk_platform.h"

void vk_createWindowImpl(void);
void vk_destroyWindowImpl(void);
void vk_minimizeWindowImpl(void);

void* vk_getInstanceProcAddrImpl(void);

void vk_createSurfaceImpl(VkInstance hInstance, VkSurfaceKHR* const pSurface);

#endif
