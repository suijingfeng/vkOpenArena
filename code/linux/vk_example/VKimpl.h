#ifndef VKIMPL_H_
#define VKIMPL_H_


/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/


#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_XCB_KHR 1

#include "vulkan/vulkan.h"

void vk_createWindow(void);
void vk_destroyWindow(void);

void vk_getInstanceProcAddrImpl(void);

void vk_createSurfaceImpl(void);

void vk_minimizeWindow( void );

#endif
