// keep XCB only

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <stdbool.h>

#include <X11/Xutil.h>

#include "vk_impl_xcb.h"

#include "linmath.h"
#include "gettime.h"

#include "demo.h"
#include "model.h"
#include "vk_shader.h"
#include "vk_texture.h"

#define MILLION 1000000L
#define BILLION 1000000000L



static void flush_init_cmd(struct demo *demo)
{
    VkResult  err;

    // This function could get called twice if the texture uses a staging buffer
    // In that case the second call should be ignored
    if (demo->cmd == VK_NULL_HANDLE) return;

    err = vkEndCommandBuffer(demo->cmd);
    assert(!err);

    VkFence fence;
    VkFenceCreateInfo fence_ci = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = NULL, .flags = 0};
    err = vkCreateFence(demo->device, &fence_ci, NULL, &fence);
    assert(!err);

    const VkCommandBuffer cmd_bufs[] = {demo->cmd};
    VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                .pNext = NULL,
                                .waitSemaphoreCount = 0,
                                .pWaitSemaphores = NULL,
                                .pWaitDstStageMask = NULL,
                                .commandBufferCount = 1,
                                .pCommandBuffers = cmd_bufs,
                                .signalSemaphoreCount = 0,
                                .pSignalSemaphores = NULL};

    err = vkQueueSubmit(demo->graphics_queue, 1, &submit_info, fence);
    assert(!err);

    err = vkWaitForFences(demo->device, 1, &fence, VK_TRUE, UINT64_MAX);
    assert(!err);

    vkFreeCommandBuffers(demo->device, demo->cmd_pool, 1, cmd_bufs);
    vkDestroyFence(demo->device, fence, NULL);
    demo->cmd = VK_NULL_HANDLE;
}



static void vk_draw_build_cmd(struct demo *demo, VkCommandBuffer cmd_buf) {
    VkDebugUtilsLabelEXT label;
    memset(&label, 0, sizeof(label));
    const VkCommandBufferBeginInfo cmd_buf_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        .pInheritanceInfo = NULL,
    };
    const VkClearValue clear_values[2] = {
        [0] = {.color.float32 = {0.2f, 0.2f, 0.2f, 0.2f}},
        [1] = {.depthStencil = {1.0f, 0}},
    };
    const VkRenderPassBeginInfo rp_begin = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = NULL,
        .renderPass = demo->render_pass,
        .framebuffer = demo->swapchain_image_resources[demo->current_buffer].framebuffer,
        .renderArea.offset.x = 0,
        .renderArea.offset.y = 0,
        .renderArea.extent.width = demo->width,
        .renderArea.extent.height = demo->height,
        .clearValueCount = 2,
        .pClearValues = clear_values,
    };
    VkResult  err;

    err = vkBeginCommandBuffer(cmd_buf, &cmd_buf_info);

    if (demo->validate) {
        // Set a name for the command buffer
        VkDebugUtilsObjectNameInfoEXT cmd_buf_name = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .pNext = NULL,
            .objectType = VK_OBJECT_TYPE_COMMAND_BUFFER,
            .objectHandle = (uint64_t)cmd_buf,
            .pObjectName = "CubeDrawCommandBuf",
        };
        demo->SetDebugUtilsObjectNameEXT(demo->device, &cmd_buf_name);

        label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        label.pNext = NULL;
        label.pLabelName = "DrawBegin";
        label.color[0] = 0.4f;
        label.color[1] = 0.3f;
        label.color[2] = 0.2f;
        label.color[3] = 0.1f;
        demo->CmdBeginDebugUtilsLabelEXT(cmd_buf, &label);
    }

    assert(!err);
    vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    if (demo->validate) {
        label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        label.pNext = NULL;
        label.pLabelName = "InsideRenderPass";
        label.color[0] = 8.4f;
        label.color[1] = 7.3f;
        label.color[2] = 6.2f;
        label.color[3] = 7.1f;
        demo->CmdBeginDebugUtilsLabelEXT(cmd_buf, &label);
    }

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, demo->pipeline);
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, demo->pipeline_layout, 0, 1,
                            &demo->swapchain_image_resources[demo->current_buffer].descriptor_set, 0, NULL);
    VkViewport viewport;
    memset(&viewport, 0, sizeof(viewport));
    viewport.height = (float)demo->height;
    viewport.width = (float)demo->width;
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    VkRect2D scissor;
    memset(&scissor, 0, sizeof(scissor));
    scissor.extent.width = demo->width;
    scissor.extent.height = demo->height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

    if (demo->validate) {
        label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        label.pNext = NULL;
        label.pLabelName = "ActualDraw";
        label.color[0] = -0.4f;
        label.color[1] = -0.3f;
        label.color[2] = -0.2f;
        label.color[3] = -0.1f;
        demo->CmdBeginDebugUtilsLabelEXT(cmd_buf, &label);
    }

    vkCmdDraw(cmd_buf, 12 * 3, 1, 0, 0);
    if (demo->validate) {
        demo->CmdEndDebugUtilsLabelEXT(cmd_buf);
    }

    // Note that ending the renderpass changes the image's layout from
    // COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR
    vkCmdEndRenderPass(cmd_buf);
    if (demo->validate) {
        demo->CmdEndDebugUtilsLabelEXT(cmd_buf);
    }

    if (demo->separate_present_queue) {
        // We have to transfer ownership from the graphics queue family to the
        // present queue family to be able to present.  Note that we don't have
        // to transfer from present queue family back to graphics queue family at
        // the start of the next frame because we don't care about the image's
        // contents at that point.
        VkImageMemoryBarrier image_ownership_barrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                                        .pNext = NULL,
                                                        .srcAccessMask = 0,
                                                        .dstAccessMask = 0,
                                                        .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                        .srcQueueFamilyIndex = demo->graphics_queue_family_index,
                                                        .dstQueueFamilyIndex = demo->present_queue_family_index,
                                                        .image = demo->swapchain_image_resources[demo->current_buffer].image,
                                                        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

        vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0,
                             NULL, 1, &image_ownership_barrier);
    }
    if (demo->validate) {
        demo->CmdEndDebugUtilsLabelEXT(cmd_buf);
    }
    err = vkEndCommandBuffer(cmd_buf);
    assert(!err);
}


void vk_build_image_ownership_cmd(struct demo *demo, int i)
{
    VkResult err;

    const VkCommandBufferBeginInfo cmd_buf_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        .pInheritanceInfo = NULL,
    };
    err = vkBeginCommandBuffer(demo->swapchain_image_resources[i].graphics_to_present_cmd, &cmd_buf_info);
    assert(!err);

    VkImageMemoryBarrier image_ownership_barrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                                    .pNext = NULL,
                                                    .srcAccessMask = 0,
                                                    .dstAccessMask = 0,
                                                    .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                    .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                    .srcQueueFamilyIndex = demo->graphics_queue_family_index,
                                                    .dstQueueFamilyIndex = demo->present_queue_family_index,
                                                    .image = demo->swapchain_image_resources[i].image,
                                                    .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    vkCmdPipelineBarrier(demo->swapchain_image_resources[i].graphics_to_present_cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &image_ownership_barrier);
    err = vkEndCommandBuffer(demo->swapchain_image_resources[i].graphics_to_present_cmd);
    assert(!err);
}




static void vk_prepare_buffers(struct demo *demo)
{
    VkResult err;
    VkSwapchainKHR oldSwapchain = demo->swapchain;

    // Check the surface capabilities and formats
    VkSurfaceCapabilitiesKHR surfCapabilities;
    err = demo->fpGetPhysicalDeviceSurfaceCapabilitiesKHR(demo->gpu, demo->surface, &surfCapabilities);
    assert(!err);

    uint32_t presentModeCount;
    err = demo->fpGetPhysicalDeviceSurfacePresentModesKHR(demo->gpu, demo->surface, &presentModeCount, NULL);
    assert(!err);
    VkPresentModeKHR *presentModes = (VkPresentModeKHR *)malloc(presentModeCount * sizeof(VkPresentModeKHR));
    assert(presentModes);
    err = demo->fpGetPhysicalDeviceSurfacePresentModesKHR(demo->gpu, demo->surface, &presentModeCount, presentModes);
    assert(!err);

    VkExtent2D swapchainExtent;
    // width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
    if (surfCapabilities.currentExtent.width == 0xFFFFFFFF) {
        // If the surface size is undefined, the size is set to the size
        // of the images requested, which must fit within the minimum and
        // maximum values.
        swapchainExtent.width = demo->width;
        swapchainExtent.height = demo->height;

        if (swapchainExtent.width < surfCapabilities.minImageExtent.width) {
            swapchainExtent.width = surfCapabilities.minImageExtent.width;
        } else if (swapchainExtent.width > surfCapabilities.maxImageExtent.width) {
            swapchainExtent.width = surfCapabilities.maxImageExtent.width;
        }

        if (swapchainExtent.height < surfCapabilities.minImageExtent.height) {
            swapchainExtent.height = surfCapabilities.minImageExtent.height;
        } else if (swapchainExtent.height > surfCapabilities.maxImageExtent.height) {
            swapchainExtent.height = surfCapabilities.maxImageExtent.height;
        }
    } else {
        // If the surface size is defined, the swap chain size must match
        swapchainExtent = surfCapabilities.currentExtent;
        demo->width = surfCapabilities.currentExtent.width;
        demo->height = surfCapabilities.currentExtent.height;
    }

    if (demo->width == 0 || demo->height == 0) {
        demo->is_minimized = true;
        return;
    } else {
        demo->is_minimized = false;
    }

    // The FIFO present mode is guaranteed by the spec to be supported
    // and to have no tearing.  It's a great default present mode to use.
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    //  There are times when you may wish to use another present mode.  The
    //  following code shows how to select them, and the comments provide some
    //  reasons you may wish to use them.
    //
    // It should be noted that Vulkan 1.0 doesn't provide a method for
    // synchronizing rendering with the presentation engine's display.  There
    // is a method provided for throttling rendering with the display, but
    // there are some presentation engines for which this method will not work.
    // If an application doesn't throttle its rendering, and if it renders much
    // faster than the refresh rate of the display, this can waste power on
    // mobile devices.  That is because power is being spent rendering images
    // that may never be seen.

    // VK_PRESENT_MODE_IMMEDIATE_KHR is for applications that don't care about
    // tearing, or have some way of synchronizing their rendering with the
    // display.
    // VK_PRESENT_MODE_MAILBOX_KHR may be useful for applications that
    // generally render a new presentable image every refresh cycle, but are
    // occasionally early.  In this case, the application wants the new image
    // to be displayed instead of the previously-queued-for-presentation image
    // that has not yet been displayed.
    // VK_PRESENT_MODE_FIFO_RELAXED_KHR is for applications that generally
    // render a new presentable image every refresh cycle, but are occasionally
    // late.  In this case (perhaps because of stuttering/latency concerns),
    // the application wants the late image to be immediately displayed, even
    // though that may mean some tearing.

    if (demo->presentMode != swapchainPresentMode) {
        for (size_t i = 0; i < presentModeCount; ++i) {
            if (presentModes[i] == demo->presentMode) {
                swapchainPresentMode = demo->presentMode;
                break;
            }
        }
    }
    if (swapchainPresentMode != demo->presentMode) {
        ERR_EXIT("Present mode specified is not supported\n", "Present mode unsupported");
    }

    // Determine the number of VkImages to use in the swap chain.
    // Application desires to acquire 3 images at a time for triple
    // buffering
    uint32_t desiredNumOfSwapchainImages = 3;
    if (desiredNumOfSwapchainImages < surfCapabilities.minImageCount) {
        desiredNumOfSwapchainImages = surfCapabilities.minImageCount;
    }
    // If maxImageCount is 0, we can ask for as many images as we want;
    // otherwise we're limited to maxImageCount
    if ((surfCapabilities.maxImageCount > 0) && (desiredNumOfSwapchainImages > surfCapabilities.maxImageCount)) {
        // Application must settle for fewer images than desired:
        desiredNumOfSwapchainImages = surfCapabilities.maxImageCount;
    }

    VkSurfaceTransformFlagsKHR preTransform;
    if (surfCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        preTransform = surfCapabilities.currentTransform;
    }

    // Find a supported composite alpha mode - one of these is guaranteed to be set
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for (uint32_t i = 0; i < ARRAY_SIZE(compositeAlphaFlags); i++) {
        if (surfCapabilities.supportedCompositeAlpha & compositeAlphaFlags[i]) {
            compositeAlpha = compositeAlphaFlags[i];
            break;
        }
    }

    VkSwapchainCreateInfoKHR swapchain_ci = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .surface = demo->surface,
        .minImageCount = desiredNumOfSwapchainImages,
        .imageFormat = demo->format,
        .imageColorSpace = demo->color_space,
        .imageExtent =
            {
                .width = swapchainExtent.width,
                .height = swapchainExtent.height,
            },
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = preTransform,
        .compositeAlpha = compositeAlpha,
        .imageArrayLayers = 1,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .presentMode = swapchainPresentMode,
        .oldSwapchain = oldSwapchain,
        .clipped = true,
    };
    uint32_t i;
    err = demo->fpCreateSwapchainKHR(demo->device, &swapchain_ci, NULL, &demo->swapchain);
    assert(!err);

    // If we just re-created an existing swapchain, we should destroy the old
    // swapchain at this point.
    // Note: destroying the swapchain also cleans up all its associated
    // presentable images once the platform is done with them.
    if (oldSwapchain != VK_NULL_HANDLE) {
        demo->fpDestroySwapchainKHR(demo->device, oldSwapchain, NULL);
    }

    err = demo->fpGetSwapchainImagesKHR(demo->device, demo->swapchain, &demo->swapchainImageCount, NULL);
    assert(!err);

    VkImage *swapchainImages = (VkImage *)malloc(demo->swapchainImageCount * sizeof(VkImage));
    assert(swapchainImages);
    err = demo->fpGetSwapchainImagesKHR(demo->device, demo->swapchain, &demo->swapchainImageCount, swapchainImages);
    assert(!err);

    demo->swapchain_image_resources =
        (SwapchainImageResources *)malloc(sizeof(SwapchainImageResources) * demo->swapchainImageCount);
    assert(demo->swapchain_image_resources);

    for (i = 0; i < demo->swapchainImageCount; i++) {
        VkImageViewCreateInfo color_image_view = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .format = demo->format,
            .components =
                {
                    .r = VK_COMPONENT_SWIZZLE_R,
                    .g = VK_COMPONENT_SWIZZLE_G,
                    .b = VK_COMPONENT_SWIZZLE_B,
                    .a = VK_COMPONENT_SWIZZLE_A,
                },
            .subresourceRange =
                {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1},
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .flags = 0,
        };

        demo->swapchain_image_resources[i].image = swapchainImages[i];

        color_image_view.image = demo->swapchain_image_resources[i].image;

        err = vkCreateImageView(demo->device, &color_image_view, NULL, &demo->swapchain_image_resources[i].view);
        assert(!err);
    }


    if (NULL != presentModes) {
        free(presentModes);
    }
}



static void demo_destroy_texture_image(struct demo *demo, struct texture_object *tex_objs) {
    /* clean up staging resources */
    vkFreeMemory(demo->device, tex_objs->mem, NULL);
    vkDestroyImage(demo->device, tex_objs->image, NULL);
}



static void demo_prepare_descriptor_layout(struct demo *demo) {
    const VkDescriptorSetLayoutBinding layout_bindings[2] = {
        [0] =
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = NULL,
            },
        [1] =
            {
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = DEMO_TEXTURE_COUNT,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = NULL,
            },
    };
    const VkDescriptorSetLayoutCreateInfo descriptor_layout = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .bindingCount = 2,
        .pBindings = layout_bindings,
    };
    VkResult  err;

    err = vkCreateDescriptorSetLayout(demo->device, &descriptor_layout, NULL, &demo->desc_layout);
    assert(!err);

    const VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .setLayoutCount = 1,
        .pSetLayouts = &demo->desc_layout,
    };

    err = vkCreatePipelineLayout(demo->device, &pPipelineLayoutCreateInfo, NULL, &demo->pipeline_layout);
    assert(!err);
}

static void demo_prepare_render_pass(struct demo *demo) {
    // The initial layout for the color and depth attachments will be LAYOUT_UNDEFINED
    // because at the start of the renderpass, we don't care about their contents.
    // At the start of the subpass, the color attachment's layout will be transitioned
    // to LAYOUT_COLOR_ATTACHMENT_OPTIMAL and the depth stencil attachment's layout
    // will be transitioned to LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL.  At the end of
    // the renderpass, the color attachment's layout will be transitioned to
    // LAYOUT_PRESENT_SRC_KHR to be ready to present.  This is all done as part of
    // the renderpass, no barriers are necessary.
    const VkAttachmentDescription attachments[2] = {
        [0] =
            {
                .format = demo->format,
                .flags = 0,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            },
        [1] =
            {
                .format = demo->depth.format,
                .flags = 0,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            },
    };
    const VkAttachmentReference color_reference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkAttachmentReference depth_reference = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    const VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .flags = 0,
        .inputAttachmentCount = 0,
        .pInputAttachments = NULL,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_reference,
        .pResolveAttachments = NULL,
        .pDepthStencilAttachment = &depth_reference,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = NULL,
    };
    const VkRenderPassCreateInfo rp_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 0,
        .pDependencies = NULL,
    };
    VkResult  err;

    err = vkCreateRenderPass(demo->device, &rp_info, NULL, &demo->render_pass);
    assert(!err);
}





static void demo_prepare_pipeline(struct demo *demo) {
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
    VkResult  err;

    memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
    memset(&dynamicState, 0, sizeof dynamicState);
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

    err = vkCreatePipelineCache(demo->device, &pipelineCache, NULL, &demo->pipelineCache);
    assert(!err);

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

    err = vkCreateGraphicsPipelines(demo->device, demo->pipelineCache, 1, &pipeline, NULL, &demo->pipeline);
    assert(!err);

    vkDestroyShaderModule(demo->device, demo->frag_shader_module, NULL);
    vkDestroyShaderModule(demo->device, demo->vert_shader_module, NULL);
}

static void demo_prepare_descriptor_pool(struct demo *demo) {
    const VkDescriptorPoolSize type_counts[2] = {
        [0] =
            {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = demo->swapchainImageCount,
            },
        [1] =
            {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = demo->swapchainImageCount * DEMO_TEXTURE_COUNT,
            },
    };
    const VkDescriptorPoolCreateInfo descriptor_pool = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .maxSets = demo->swapchainImageCount,
        .poolSizeCount = 2,
        .pPoolSizes = type_counts,
    };
    VkResult  err;

    err = vkCreateDescriptorPool(demo->device, &descriptor_pool, NULL, &demo->desc_pool);
    assert(!err);
}

static void demo_prepare_descriptor_set(struct demo *demo) {
    VkDescriptorImageInfo tex_descs[DEMO_TEXTURE_COUNT];
    VkWriteDescriptorSet writes[2];
    VkResult  err;

    VkDescriptorSetAllocateInfo alloc_info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                              .pNext = NULL,
                                              .descriptorPool = demo->desc_pool,
                                              .descriptorSetCount = 1,
                                              .pSetLayouts = &demo->desc_layout};

    VkDescriptorBufferInfo buffer_info;
    buffer_info.offset = 0;
    buffer_info.range = sizeof(struct vktexcube_vs_uniform);

    memset(&tex_descs, 0, sizeof(tex_descs));
    for (unsigned int i = 0; i < DEMO_TEXTURE_COUNT; i++) {
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

    for (unsigned int i = 0; i < demo->swapchainImageCount; i++) {
        err = vkAllocateDescriptorSets(demo->device, &alloc_info, &demo->swapchain_image_resources[i].descriptor_set);
        assert(!err);
        buffer_info.buffer = demo->swapchain_image_resources[i].uniform_buffer;
        writes[0].dstSet = demo->swapchain_image_resources[i].descriptor_set;
        writes[1].dstSet = demo->swapchain_image_resources[i].descriptor_set;
        vkUpdateDescriptorSets(demo->device, 2, writes, 0, NULL);
    }
}

static void demo_prepare_framebuffers(struct demo *demo)
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
    VkResult  err;
    uint32_t i;

    for (i = 0; i < demo->swapchainImageCount; i++)
    {
        attachments[0] = demo->swapchain_image_resources[i].view;
        err = vkCreateFramebuffer(demo->device, &fb_info, NULL, &demo->swapchain_image_resources[i].framebuffer);
        assert(!err);
    }
}


void demo_prepare(struct demo *demo)
{
    VkResult  err;
    if (demo->cmd_pool == VK_NULL_HANDLE) {
        const VkCommandPoolCreateInfo cmd_pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = NULL,
            .queueFamilyIndex = demo->graphics_queue_family_index,
            .flags = 0,
        };
        err = vkCreateCommandPool(demo->device, &cmd_pool_info, NULL, &demo->cmd_pool);
        assert(!err);
    }

    const VkCommandBufferAllocateInfo cmd = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = demo->cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    err = vkAllocateCommandBuffers(demo->device, &cmd, &demo->cmd);
    assert(!err);
    VkCommandBufferBeginInfo cmd_buf_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = 0,
        .pInheritanceInfo = NULL,
    };
    err = vkBeginCommandBuffer(demo->cmd, &cmd_buf_info);
    assert(!err);

    vk_prepare_buffers(demo);

    if (demo->is_minimized) {
        demo->prepared = false;
        return;
    }


    vk_prepare_depth(demo);
    vk_prepare_textures(demo);
    
    prepare_cube_data_buffers(demo);

    demo_prepare_descriptor_layout(demo);
    demo_prepare_render_pass(demo);
    demo_prepare_pipeline(demo);

    for (uint32_t i = 0; i < demo->swapchainImageCount; i++) {
        err = vkAllocateCommandBuffers(demo->device, &cmd, &demo->swapchain_image_resources[i].cmd);
        assert(!err);
    }

    if (demo->separate_present_queue) {
        const VkCommandPoolCreateInfo present_cmd_pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = NULL,
            .queueFamilyIndex = demo->present_queue_family_index,
            .flags = 0,
        };
        err = vkCreateCommandPool(demo->device, &present_cmd_pool_info, NULL, &demo->present_cmd_pool);
        assert(!err);
        const VkCommandBufferAllocateInfo present_cmd_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = NULL,
            .commandPool = demo->present_cmd_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        for (uint32_t i = 0; i < demo->swapchainImageCount; i++) {
            err = vkAllocateCommandBuffers(demo->device, &present_cmd_info,
                                           &demo->swapchain_image_resources[i].graphics_to_present_cmd);
            assert(!err);
            vk_build_image_ownership_cmd(demo, i);
        }
    }

    demo_prepare_descriptor_pool(demo);
    demo_prepare_descriptor_set(demo);

    demo_prepare_framebuffers(demo);

    for (uint32_t i = 0; i < demo->swapchainImageCount; i++) {
        demo->current_buffer = i;
        vk_draw_build_cmd(demo, demo->swapchain_image_resources[i].cmd);
    }

    /*
     * Prepare functions above may generate pipeline commands
     * that need to be flushed before beginning the render loop.
     */
    flush_init_cmd(demo);
    if (demo->staging_texture.image) {
        demo_destroy_texture_image(demo, &demo->staging_texture);
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
    for (i = 0; i < FRAME_LAG; i++) {
        vkWaitForFences(demo->device, 1, &demo->fences[i], VK_TRUE, UINT64_MAX);
        vkDestroyFence(demo->device, demo->fences[i], NULL);
        vkDestroySemaphore(demo->device, demo->image_acquired_semaphores[i], NULL);
        vkDestroySemaphore(demo->device, demo->draw_complete_semaphores[i], NULL);
        if (demo->separate_present_queue) {
            vkDestroySemaphore(demo->device, demo->image_ownership_semaphores[i], NULL);
        }
    }

    // If the window is currently minimized, demo_resize has already done some cleanup for us.
    if (!demo->is_minimized) {
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
        demo->fpDestroySwapchainKHR(demo->device, demo->swapchain, NULL);

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
    if (demo->validate) {
        demo->DestroyDebugUtilsMessengerEXT(demo->inst, demo->dbg_messenger, NULL);
    }
    vkDestroySurfaceKHR(demo->inst, demo->surface, NULL);



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
    demo.frameCount = INT32_MAX;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--use_staging") == 0) {
            demo.use_staging_buffer = true;
            continue;
        }
        if ((strcmp(argv[i], "--present_mode") == 0) && (i < argc - 1)) {
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

        if (strcmp(argv[i], "--c") == 0 && demo.frameCount == INT32_MAX && i < argc - 1 &&
            sscanf(argv[i + 1], "%d", &demo.frameCount) == 1 && demo.frameCount >= 0) {
            i++;
            continue;
        }

        fprintf(stderr,
                "Usage:\n  \t[--use_staging] [--validate] [--validate-checks-disabled] [--break]\n"
                "\t[--c <framecount>] [--incremental_present] [--display_timing]\n"
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

    init_connection_xcb(&demo);

    vk_init(&demo);

    demo.width = 500;
    demo.height = 500;

    demo.spin_angle = 4.0f;
    demo.spin_increment = 0.2f;
    demo.pause = false;

    mat4x4_perspective(demo.projection_matrix, (float)degreesToRadians(45.0f), 1.0f, 0.1f, 100.0f);
    mat4x4_look_at(demo.view_matrix, eye, origin, up);
    mat4x4_identity(demo.model_matrix);

    demo.projection_matrix[1][1] *= -1;  // Flip projection matrix from GL to Vulkan orientation.



    create_window_xcb(&demo);

    init_vk_swapchain(&demo);

    demo_prepare(&demo);

    run_xcb(&demo);

    vk_cleanup(&demo);

    return 0;
}
