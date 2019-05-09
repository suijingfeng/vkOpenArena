#ifndef VK_SWAPCHAIN_H_
#define VK_SWAPCHAIN_H_

#include "VKimpl.h"

void vk_recreateSwapChain(void);

void vk_createSwapChain( VkDevice device, VkSurfaceKHR surface, 
        VkSurfaceFormatKHR surface_format, VkPresentModeKHR presentMode,
        VkSwapchainKHR * const pHSwapChain);
void vk_destroySwapChain(void);

#endif
