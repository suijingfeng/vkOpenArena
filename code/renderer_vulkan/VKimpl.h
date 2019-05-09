#ifndef VKIMPL_H_
#define VKIMPL_H_


/*
==========================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

==========================================================
*/


#define VK_NO_PROTOTYPES
#include "../vulkan/vulkan.h"

void vk_createWindowImpl(void);
void vk_destroyWindowImpl(void);

void vk_getInstanceProcAddrImpl(void);

void vk_createSurfaceImpl(VkSurfaceKHR* surface);

void vk_minimizeWindowImpl( void );


#endif
