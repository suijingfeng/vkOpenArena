// keep XCB only

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>


#include "vk_window_xcb.h"

#include "linmath.h"
#include "gettime.h"

#include "demo.h"
#include "model.h"
#include "vk_shader.h"
#include "vk_texture.h"
#include "vk_common.h"
#include "vk_render_pass.h"
#include "vk_cmd.h"
#include "vk_swapchain.h"
#include "vk_debug.h"

#define MILLION 1000000L
#define BILLION 1000000000L


static void destroy_texture_image(struct demo *demo, struct texture_object *tex_objs)
{
    /* clean up staging resources */
    vkFreeMemory(demo->device, tex_objs->mem, NULL);
    vkDestroyImage(demo->device, tex_objs->image, NULL);
}



static void vk_prepare_descriptor_layout(struct demo *demo)
{
    VkDescriptorSetLayoutBinding layout_bindings[2];
    
    layout_bindings[0].binding = 0;
    layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_bindings[0].descriptorCount = 1;
    layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layout_bindings[0].pImmutableSamplers = NULL;

    layout_bindings[1].binding = 1;
    layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layout_bindings[1].descriptorCount = DEMO_TEXTURE_COUNT;
    layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layout_bindings[1].pImmutableSamplers = NULL;

    const VkDescriptorSetLayoutCreateInfo descriptor_layout = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .bindingCount = 2,
        .pBindings = layout_bindings,
    };

    VK_CHECK( vkCreateDescriptorSetLayout(demo->device, &descriptor_layout, NULL, &demo->desc_layout) );

    const VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .setLayoutCount = 1,
        .pSetLayouts = &demo->desc_layout
    };

    VK_CHECK( vkCreatePipelineLayout(demo->device, &pPipelineLayoutCreateInfo, NULL, &demo->pipeline_layout) );
}


static void vk_prepare_pipeline(struct demo *demo)
{
    VkGraphicsPipelineCreateInfo pipeline;
    VkPipelineCacheCreateInfo pipelineCache;
    VkPipelineVertexInputStateCreateInfo vi;
    VkPipelineInputAssemblyStateCreateInfo ia;
    VkPipelineRasterizationStateCreateInfo rs;
    VkPipelineColorBlendStateCreateInfo cb;
    VkPipelineDepthStencilStateCreateInfo ds;
    VkPipelineViewportStateCreateInfo vp;
    VkPipelineMultisampleStateCreateInfo ms;
    VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
    VkPipelineDynamicStateCreateInfo dynamicState;

    memset(dynamicStateEnables, 0, sizeof(dynamicStateEnables));
    memset(&dynamicState, 0, sizeof(dynamicState));
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates = dynamicStateEnables;

    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.layout = demo->pipeline_layout;

    memset(&vi, 0, sizeof(vi));
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_BACK_BIT;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.depthClampEnable = VK_FALSE;
    rs.rasterizerDiscardEnable = VK_FALSE;
    rs.depthBiasEnable = VK_FALSE;
    rs.lineWidth = 1.0f;

    memset(&cb, 0, sizeof(cb));
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    VkPipelineColorBlendAttachmentState att_state[1];
    memset(att_state, 0, sizeof(att_state));
    att_state[0].colorWriteMask = 0xf;
    att_state[0].blendEnable = VK_FALSE;
    cb.attachmentCount = 1;
    cb.pAttachments = att_state;

    memset(&vp, 0, sizeof(vp));
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    vp.scissorCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;

    memset(&ds, 0, sizeof(ds));
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    ds.depthBoundsTestEnable = VK_FALSE;
    ds.back.failOp = VK_STENCIL_OP_KEEP;
    ds.back.passOp = VK_STENCIL_OP_KEEP;
    ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
    ds.stencilTestEnable = VK_FALSE;
    ds.front = ds.back;

    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.pSampleMask = NULL;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    vk_prepare_vs(demo);
    vk_prepare_fs(demo);

    // Two stages: vs and fs
    VkPipelineShaderStageCreateInfo shaderStages[2];
    memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = demo->vert_shader_module;
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = demo->frag_shader_module;
    shaderStages[1].pName = "main";

    memset(&pipelineCache, 0, sizeof(pipelineCache));
    pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    VK_CHECK( vkCreatePipelineCache(demo->device, &pipelineCache, NULL, &demo->pipelineCache) );

    pipeline.pVertexInputState = &vi;
    pipeline.pInputAssemblyState = &ia;
    pipeline.pRasterizationState = &rs;
    pipeline.pColorBlendState = &cb;
    pipeline.pMultisampleState = &ms;
    pipeline.pViewportState = &vp;
    pipeline.pDepthStencilState = &ds;
    pipeline.stageCount = ARRAY_SIZE(shaderStages);
    pipeline.pStages = shaderStages;
    pipeline.renderPass = demo->render_pass;
    pipeline.pDynamicState = &dynamicState;

    pipeline.renderPass = demo->render_pass;

    VK_CHECK( vkCreateGraphicsPipelines(demo->device, demo->pipelineCache, 1, &pipeline, NULL, &demo->pipeline) );

    vkDestroyShaderModule(demo->device, demo->frag_shader_module, NULL);
    vkDestroyShaderModule(demo->device, demo->vert_shader_module, NULL);
}


static void prepare_descriptor_pool(struct demo *demo)
{
    VkDescriptorPoolSize type_counts[2];
    
    type_counts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    type_counts[0].descriptorCount = demo->swapchainImageCount;
    type_counts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    type_counts[1].descriptorCount = demo->swapchainImageCount * DEMO_TEXTURE_COUNT;

    const VkDescriptorPoolCreateInfo descriptor_pool = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .maxSets = demo->swapchainImageCount,
        .poolSizeCount = 2,
        .pPoolSizes = type_counts,
    };

    VK_CHECK( vkCreateDescriptorPool(demo->device, &descriptor_pool, NULL, &demo->desc_pool) );
}


static void prepare_descriptor_set(struct demo *demo)
{
    VkDescriptorImageInfo tex_descs[DEMO_TEXTURE_COUNT];
    VkWriteDescriptorSet writes[2];

    VkDescriptorSetAllocateInfo alloc_info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                              .pNext = NULL,
                                              .descriptorPool = demo->desc_pool,
                                              .descriptorSetCount = 1,
                                              .pSetLayouts = &demo->desc_layout};

    VkDescriptorBufferInfo buffer_info;
    buffer_info.offset = 0;
    buffer_info.range = sizeof(struct texcube_vs_uniform);

    memset(&tex_descs, 0, sizeof(tex_descs));
    
    for (unsigned int i = 0; i < DEMO_TEXTURE_COUNT; i++)
    {
        tex_descs[i].sampler = demo->textures[i].sampler;
        tex_descs[i].imageView = demo->textures[i].view;
        tex_descs[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    }

    memset(&writes, 0, sizeof(writes));

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &buffer_info;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = DEMO_TEXTURE_COUNT;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = tex_descs;

    for (unsigned int i = 0; i < demo->swapchainImageCount; i++)
    {
        VK_CHECK( vkAllocateDescriptorSets(demo->device, &alloc_info, &demo->swapchain_image_resources[i].descriptor_set) );

        buffer_info.buffer = demo->swapchain_image_resources[i].uniform_buffer;
        writes[0].dstSet = demo->swapchain_image_resources[i].descriptor_set;
        writes[1].dstSet = demo->swapchain_image_resources[i].descriptor_set;
        vkUpdateDescriptorSets(demo->device, 2, writes, 0, NULL);
    }
}



static void prepare_framebuffers(struct demo *demo)
{
    VkImageView attachments[2];
    attachments[1] = demo->depth.view;

    const VkFramebufferCreateInfo fb_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = NULL,
        .renderPass = demo->render_pass,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .width = demo->width,
        .height = demo->height,
        .layers = 1,
    };

    uint32_t i;

    for (i = 0; i < demo->swapchainImageCount; i++)
    {
        attachments[0] = demo->swapchain_image_resources[i].view;
        VK_CHECK( vkCreateFramebuffer(demo->device, &fb_info, NULL, &demo->swapchain_image_resources[i].framebuffer) );
    }
}


void demo_prepare(struct demo *demo)
{
    
    if (demo->cmd_pool == VK_NULL_HANDLE)
    {
        const VkCommandPoolCreateInfo cmd_pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = NULL,
            .queueFamilyIndex = demo->graphics_queue_family_index,
            .flags = 0,
        };

        VK_CHECK( vkCreateCommandPool(demo->device, &cmd_pool_info, NULL, &demo->cmd_pool) );
    }

    const VkCommandBufferAllocateInfo cmd =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = demo->cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VK_CHECK( vkAllocateCommandBuffers(demo->device, &cmd, &demo->cmd) );

    VkCommandBufferBeginInfo cmd_buf_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = 0,
        .pInheritanceInfo = NULL,
    };

    VK_CHECK( vkBeginCommandBuffer(demo->cmd, &cmd_buf_info) );

    vk_prepare_buffers(demo);

    if (demo->is_minimized) {
        demo->prepared = false;
        return;
    }

    vk_prepare_depth(demo);
    vk_prepare_textures(demo);
    
    prepare_cube_data_buffers(demo);

    vk_prepare_descriptor_layout(demo);
    vk_prepare_render_pass(demo);
    vk_prepare_pipeline(demo);

    for (uint32_t i = 0; i < demo->swapchainImageCount; i++)
    {
        VK_CHECK( vkAllocateCommandBuffers(demo->device, &cmd, &demo->swapchain_image_resources[i].cmd) );
    }

    if (demo->separate_present_queue)
    {
        const VkCommandPoolCreateInfo present_cmd_pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = NULL,
            .queueFamilyIndex = demo->present_queue_family_index,
            .flags = 0,
        };

        VK_CHECK( vkCreateCommandPool(demo->device, &present_cmd_pool_info, NULL, &demo->present_cmd_pool) );
        
        const VkCommandBufferAllocateInfo present_cmd_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = NULL,
            .commandPool = demo->present_cmd_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        for (uint32_t i = 0; i < demo->swapchainImageCount; i++)
        {
            VK_CHECK( vkAllocateCommandBuffers(demo->device, &present_cmd_info,
                                           &demo->swapchain_image_resources[i].graphics_to_present_cmd) );
        
            vk_build_image_ownership_cmd(demo, i);
        }
    }

    prepare_descriptor_pool(demo);
    prepare_descriptor_set(demo);

    prepare_framebuffers(demo);

    for (uint32_t i = 0; i < demo->swapchainImageCount; i++)
    {
        demo->current_buffer = i;
        vk_draw_build_cmd(demo, demo->swapchain_image_resources[i].cmd);
    }

    /*
     * Prepare functions above may generate pipeline commands
     * that need to be flushed before beginning the render loop.
     */
    flush_init_cmd(demo);
    if (demo->staging_texture.image)
    {
        destroy_texture_image(demo, &demo->staging_texture);
    }

    demo->current_buffer = 0;
    demo->prepared = true;
}




static void vk_cleanup(struct demo *demo)
{
    uint32_t i;

    demo->prepared = false;
    vkDeviceWaitIdle(demo->device);

    // Wait for fences from present operations
    for (i = 0; i < FRAME_LAG; i++)
    {
        vkWaitForFences(demo->device, 1, &demo->fences[i], VK_TRUE, UINT64_MAX);
        vkDestroyFence(demo->device, demo->fences[i], NULL);
        vkDestroySemaphore(demo->device, demo->image_acquired_semaphores[i], NULL);
        vkDestroySemaphore(demo->device, demo->draw_complete_semaphores[i], NULL);
        if (demo->separate_present_queue) {
            vkDestroySemaphore(demo->device, demo->image_ownership_semaphores[i], NULL);
        }
    }

    // If the window is currently minimized, demo_resize has already done some cleanup for us.
    if (!demo->is_minimized)
    {
        for (i = 0; i < demo->swapchainImageCount; i++) {
            vkDestroyFramebuffer(demo->device, demo->swapchain_image_resources[i].framebuffer, NULL);
        }
        vkDestroyDescriptorPool(demo->device, demo->desc_pool, NULL);

        vkDestroyPipeline(demo->device, demo->pipeline, NULL);
        vkDestroyPipelineCache(demo->device, demo->pipelineCache, NULL);
        vkDestroyRenderPass(demo->device, demo->render_pass, NULL);
        vkDestroyPipelineLayout(demo->device, demo->pipeline_layout, NULL);
        vkDestroyDescriptorSetLayout(demo->device, demo->desc_layout, NULL);

        for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
            vkDestroyImageView(demo->device, demo->textures[i].view, NULL);
            vkDestroyImage(demo->device, demo->textures[i].image, NULL);
            vkFreeMemory(demo->device, demo->textures[i].mem, NULL);
            vkDestroySampler(demo->device, demo->textures[i].sampler, NULL);
        }
        pFn_vkhr.fpDestroySwapchainKHR(demo->device, demo->swapchain, NULL);

        vkDestroyImageView(demo->device, demo->depth.view, NULL);
        vkDestroyImage(demo->device, demo->depth.image, NULL);
        vkFreeMemory(demo->device, demo->depth.mem, NULL);

        for (i = 0; i < demo->swapchainImageCount; i++) {
            vkDestroyImageView(demo->device, demo->swapchain_image_resources[i].view, NULL);
            vkFreeCommandBuffers(demo->device, demo->cmd_pool, 1, &demo->swapchain_image_resources[i].cmd);
            vkDestroyBuffer(demo->device, demo->swapchain_image_resources[i].uniform_buffer, NULL);
            vkFreeMemory(demo->device, demo->swapchain_image_resources[i].uniform_memory, NULL);
        }
        free(demo->swapchain_image_resources);
        free(demo->queue_props);
        vkDestroyCommandPool(demo->device, demo->cmd_pool, NULL);

        if (demo->separate_present_queue) {
            vkDestroyCommandPool(demo->device, demo->present_cmd_pool, NULL);
        }
    }
    vkDeviceWaitIdle(demo->device);
    vkDestroyDevice(demo->device, NULL);
    
    vk_destroyDebugUtils(demo);

    vkDestroySurfaceKHR(demo->inst, demo->surface, NULL);

    vk_clearSurfacePresentPFN();

    xcb_destroy_window(demo->connection, demo->xcb_window);
    xcb_disconnect(demo->connection);
    free(demo->atom_wm_delete_window);

    vkDestroyInstance(demo->inst, NULL);
}



int main(int argc, char **argv)
{
    struct demo demo;

    vec3 eye = {0.0f, 3.0f, 5.0f};
    vec3 origin = {0, 0, 0};
    vec3 up = {0.0f, 1.0f, 0.0};

    memset(&demo, 0, sizeof(struct demo));
    
    demo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    demo.is_minimized = false;
    demo.cmd_pool = VK_NULL_HANDLE;
    demo.quit = false;
    demo.curFrame = 0;
    demo.frame_index = 0;
    
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--use_staging") == 0) {
            demo.use_staging_buffer = true;
            continue;
        }

        if ((strcmp(argv[i], "--present_mode") == 0) && (i < argc - 1))
        {
            demo.presentMode = atoi(argv[i + 1]);
            i++;
            continue;
        }
        if (strcmp(argv[i], "--break") == 0) {
            demo.use_break = true;
            continue;
        }
        if (strcmp(argv[i], "--validate") == 0) {
            demo.validate = true;
            continue;
        }

        fprintf(stderr,
                "Usage:\n  \t[--use_staging] [--validate] [--break]\n"
                "\t[--present_mode <present mode enum>]\n"
                "\t <present_mode_enum>\tVK_PRESENT_MODE_IMMEDIATE_KHR = %d\n"
                "\t\t\t\tVK_PRESENT_MODE_MAILBOX_KHR = %d\n"
                "\t\t\t\tVK_PRESENT_MODE_FIFO_KHR = %d\n"
                "\t\t\t\tVK_PRESENT_MODE_FIFO_RELAXED_KHR = %d\n",
                VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_FIFO_KHR,
                VK_PRESENT_MODE_FIFO_RELAXED_KHR);
        fflush(stderr);
        exit(1);
    }

    xcb_initConnection(&demo);

    vk_init(&demo);

    demo.width = 500;
    demo.height = 500;

    demo.spin_angle = 4.0f;
    demo.spin_increment = 0.2f;
    demo.pause = false;

    mat4x4_perspective(demo.projection_matrix, (float)degreesToRadians(45.0f), 1.0f, 0.1f, 100.0f);
    mat4x4_look_at(demo.view_matrix, eye, origin, up);
    mat4x4_identity(demo.model_matrix);
    
    // Flip projection matrix from GL to Vulkan orientation.
    demo.projection_matrix[1][1] = -demo.projection_matrix[1][1];

    xcb_createWindow(&demo);

    init_vk_swapchain(&demo);

    demo_prepare(&demo);

////
    run_xcb(&demo);

    vk_cleanup(&demo);

    return 0;
}
