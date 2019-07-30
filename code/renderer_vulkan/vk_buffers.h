#ifndef VK_BUFFERS_H_
#define VK_BUFFERS_H_

#include "VKimpl.h"


void vk_createBufferResource(const uint32_t Size, VkBufferUsageFlags Usage,
        VkMemoryPropertyFlagBits MemTypePrefered,
        VkBuffer * const pBuf, VkDeviceMemory * const pDevMem );

void vk_destroyBufferResource(VkBuffer hBuf, VkDeviceMemory hDevMem);


void vk_createStagingBuffer(uint32_t size);
void vk_destroyStagingBuffer(void);
void VK_UploadImageToStagBuffer(const unsigned char * const pUploadBuffer, uint32_t buffer_size);
void vk_stagBufToDevLocal(VkImage hImage, VkBufferImageCopy* const pRegion, const uint32_t nRegion);
#endif
