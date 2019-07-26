#ifndef VK_DESCRIPTOR_SETS_H_
#define VK_DESCRIPTOR_SETS_H_

#include <stdint.h>

void vk_createDescriptorPool(uint32_t numDes, VkDescriptorPool* const pPool);
void vk_createDescriptorSetLayout(VkDescriptorSetLayout * const pSetLayout);
void vk_allocOneDescptrSet(VkDescriptorSet * const pSetRet);
void vk_destroy_descriptor_pool(void);
void vk_reset_descriptor_pool(void);
#endif
