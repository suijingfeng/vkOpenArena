#ifndef VK_APP_H_
#define VK_APP_H_

  
    #define MAX_SWAPCHAIN_IMAGES    8
	VkSwapchainKHR swapchain;
	uint32_t swapchain_image_count ;
	VkImage swapchain_images_array[MAX_SWAPCHAIN_IMAGES];
	VkImageView swapchain_image_views[MAX_SWAPCHAIN_IMAGES];
	uint32_t idx_swapchain_image;


	VkCommandPool command_pool;
	VkCommandBuffer command_buffer;

	VkImage depth_image;
	VkDeviceMemory depth_image_memory;
	VkImageView depth_image_view;

	VkRenderPass render_pass;
	VkFramebuffer framebuffers[MAX_SWAPCHAIN_IMAGES];

	VkDescriptorPool descriptor_pool;
	VkDescriptorSetLayout set_layout;

    // Pipeline layout: the uniform and push values referenced by 
    // the shader that can be updated at draw time
	VkPipelineLayout pipeline_layout;

    VkBool32 isInitialized;



#endif
