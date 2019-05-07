#ifndef VK_DESCRIPTOR_SETS_H_
#define VK_DESCRIPTOR_SETS_H_

#include <stdint.h>

void vk_createDescriptorPool(uint32_t numDes);
void vk_createDescriptorSetLayout(void);
void vk_allocOneDescptrSet(VkDescriptorSet * pSetRet);
void vk_destroy_descriptor_pool(void);

#endif
