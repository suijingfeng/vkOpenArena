#ifndef VKIMPL_H_
#define VKIMPL_H_

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

void vk_createWindowImpl(void);
void vk_destroyWindowImpl(void);
void vk_minimizeWindowImpl(void);

void* vk_getInstanceProcAddrImpl(void);

void vk_createSurfaceImpl(VkInstance hInstance, VkSurfaceKHR* const pSurface);

#endif
