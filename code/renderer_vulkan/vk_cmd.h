#ifndef VK_CMD_H_
#define VK_CMD_H_

void vk_create_command_pool(VkCommandPool* pPool);
void vk_create_command_buffer(VkCommandPool pool, VkCommandBuffer* pBuf);
void vk_destroy_commands(void);

#endif
