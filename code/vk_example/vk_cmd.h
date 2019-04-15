#ifndef VK_CMD_H_
#define VK_CMD_H_

#include "vk_common.h"
#include "demo.h"

void flush_init_cmd(struct demo *demo);
void vk_draw_build_cmd(struct demo *demo, VkCommandBuffer cmd_buf);
void vk_build_image_ownership_cmd(struct demo *demo, int i);

#endif
