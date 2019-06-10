#ifndef VK_BUFFERS_H_
#define VK_BUFFERS_H_

#include "VKimpl.h"


void vk_createBufferResource(const uint32_t Size, VkBufferUsageFlags Usage,
        VkMemoryPropertyFlagBits MemTypePrefered,
        VkBuffer * const pBuf, VkDeviceMemory * const pDevMem );

void vk_destroyBufferResource(VkBuffer hBuf, VkDeviceMemory hDevMem);
    


#endif
