#ifndef VK_CMD_H_
#define VK_CMD_H_

void vk_create_command_pool(VkCommandPool* const pPool);
void vk_create_command_buffer(VkCommandPool pool, VkCommandBuffer* const pBuf);
void vk_freeCmdBufs(VkCommandBuffer* const pCmdBuf);
void vk_destroy_command_pool(void);

void vk_beginRecordCmds(VkCommandBuffer HCmdBuf);
void vk_commitRecordedCmds(VkCommandBuffer HCmdBuf);

#endif
