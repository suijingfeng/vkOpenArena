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

void vk_createSurfaceImpl(void);

void vk_minimizeWindowImpl( void );

void vk_fillRequiredInstanceExtention( 
        const VkExtensionProperties * const pInsExt, const uint32_t nInsExt, 
        const char ** const ppInsExt, uint32_t * nExt );

#endif
