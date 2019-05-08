#ifndef VK_FRAME_H_
#define VK_FRAME_H_

void vk_begin_frame(void);
void vk_end_frame(void);


void vk_createFrameBuffers(uint32_t w, uint32_t h, VkRenderPass h_rpass,
        uint32_t fbCount, VkFramebuffer* pFrameBuffers );
void vk_destroyFrameBuffers(void);

//void vk_createSwapChain(VkDevice device, VkSurfaceKHR surface, VkSurfaceFormatKHR surface_format);
void vk_createRenderPass(VkDevice device, VkRenderPass * pRenderPassObj,
        VkFormat colorFormat, VkFormat depthFormat);

void vk_create_sync_primitives(void);
void vk_destroy_sync_primitives(void);

#endif
