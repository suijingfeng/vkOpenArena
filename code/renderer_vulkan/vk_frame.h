#ifndef VK_FRAME_H_
#define VK_FRAME_H_

void vk_begin_frame(void);
void vk_end_frame(void);


void vk_createFrameBuffers(uint32_t w, uint32_t h, VkRenderPass h_rpass,
        uint32_t fbCount, VkFramebuffer * const pFrameBuffers );
void vk_destroyFrameBuffers(void);

void vk_createColorAttachment(VkDevice lgDev, const VkSwapchainKHR HSwapChain, 
        VkFormat surFmt, uint32_t * const pSwapchainLen,
        VkImageView * const pSwapChainImgViews);

void vk_createRenderPass(VkDevice device, VkFormat colorFormat, 
        VkFormat depthFormat, VkRenderPass * const pRenderPassObj);

void vk_create_sync_primitives(void);
void vk_destroy_sync_primitives(void);

void vk_createDepthAttachment(int Width, int Height, VkFormat depthFmt);
//void vk_destroyDepthAttachment(void);

#endif
