#ifndef DEMO_H_
#define DEMO_H_

#include <stdbool.h>

#include "linmath.h"
#include "vk_impl_xcb.h"

// Allow a maximum of two outstanding presentation operations.
#define FRAME_LAG 2
#define DEMO_TEXTURE_COUNT 1

typedef struct {
    VkImage image;
    VkCommandBuffer cmd;
    VkCommandBuffer graphics_to_present_cmd;
    VkImageView view;
    VkBuffer uniform_buffer;
    VkDeviceMemory uniform_memory;
    VkFramebuffer framebuffer;
    VkDescriptorSet descriptor_set;
} SwapchainImageResources;


/*
 * structure to track all objects related to a texture.
 */
struct texture_object {
    VkSampler sampler;

    VkImage image;
    VkImageLayout imageLayout;

    VkMemoryAllocateInfo mem_alloc;
    VkDeviceMemory mem;
    VkImageView view;
    int32_t tex_width, tex_height;
};

struct demo
{
    xcb_connection_t *connection;
    xcb_screen_t *screen;
    xcb_window_t xcb_window;
    xcb_intern_atom_reply_t *atom_wm_delete_window;
    
    
    VkSurfaceKHR surface;
    bool prepared;
    bool use_staging_buffer;
    bool separate_present_queue;
    bool is_minimized;

    bool syncd_with_actual_presents;
    uint64_t refresh_duration;
    uint64_t refresh_duration_multiplier;
    uint64_t target_IPD;  // image present duration (inverse of frame rate)
    uint64_t prev_desired_present_time;
    uint32_t next_present_id;
    uint32_t last_early_id;  // 0 if no early images
    uint32_t last_late_id;   // 0 if no late images

    VkInstance inst;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkQueue graphics_queue;
    VkQueue present_queue;
    uint32_t graphics_queue_family_index;
    uint32_t present_queue_family_index;
    VkSemaphore image_acquired_semaphores[FRAME_LAG];
    VkSemaphore draw_complete_semaphores[FRAME_LAG];
    VkSemaphore image_ownership_semaphores[FRAME_LAG];
    VkPhysicalDeviceProperties gpu_props;
    VkQueueFamilyProperties *queue_props;
    VkPhysicalDeviceMemoryProperties memory_properties;

    uint32_t enabled_extension_count;
    uint32_t enabled_layer_count;
    char *extension_names[64];
    char *enabled_layers[64];

    int width, height;
    VkFormat format;
    VkColorSpaceKHR color_space;

    PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;
    PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
    PFN_vkQueuePresentKHR fpQueuePresentKHR;
    uint32_t swapchainImageCount;
    VkSwapchainKHR swapchain;
    SwapchainImageResources *swapchain_image_resources;
    VkPresentModeKHR presentMode;
    VkFence fences[FRAME_LAG];
    int frame_index;

    VkCommandPool cmd_pool;
    VkCommandPool present_cmd_pool;

    struct {
        VkFormat format;

        VkImage image;
        VkMemoryAllocateInfo mem_alloc;
        VkDeviceMemory mem;
        VkImageView view;
    } depth;

    struct texture_object textures[DEMO_TEXTURE_COUNT];
    struct texture_object staging_texture;

    VkCommandBuffer cmd;  // Buffer for initialization commands
    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout desc_layout;
    VkPipelineCache pipelineCache;
    VkRenderPass render_pass;
    VkPipeline pipeline;

    mat4x4 projection_matrix;
    mat4x4 view_matrix;
    mat4x4 model_matrix;

    float spin_angle;
    float spin_increment;
    bool pause;

    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;

    VkDescriptorPool desc_pool;

    bool quit;
    int32_t curFrame;
    int32_t frameCount;
    bool validate;
    bool use_break;

    PFN_vkCreateDebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessengerEXT;
    PFN_vkSubmitDebugUtilsMessageEXT SubmitDebugUtilsMessageEXT;
    PFN_vkCmdBeginDebugUtilsLabelEXT CmdBeginDebugUtilsLabelEXT;
    PFN_vkCmdEndDebugUtilsLabelEXT CmdEndDebugUtilsLabelEXT;
    PFN_vkCmdInsertDebugUtilsLabelEXT CmdInsertDebugUtilsLabelEXT;
    PFN_vkSetDebugUtilsObjectNameEXT SetDebugUtilsObjectNameEXT;
    VkDebugUtilsMessengerEXT dbg_messenger;

    uint32_t current_buffer;
    uint32_t queue_family_count;
};


void create_window_xcb(struct demo *demo);
void init_connection_xcb(struct demo *demo);
void run_xcb(struct demo *demo);
void demo_prepare(struct demo *demo);
void update_data_buffer(struct demo *demo);
void vk_init(struct demo *demo);
void init_vk_swapchain(struct demo *demo);

#define ERR_EXIT(err_msg, err_class) \
    do {                             \
        printf("%s\n", err_msg);     \
        fflush(stdout);              \
        exit(1);                     \
    } while (0)


#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#endif
