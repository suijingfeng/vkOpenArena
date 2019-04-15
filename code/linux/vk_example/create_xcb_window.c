#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <xcb/xcb.h>

#include "linmath.h"
#include "vk_common.h"
#include "vk_swapchain.h"
#include "demo.h"
#include "model.h"


void xcb_createWindow(struct demo *demo)
{

    uint32_t value_mask, value_list[32];

    demo->xcb_window = xcb_generate_id(demo->connection);

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = demo->screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    xcb_create_window(demo->connection, XCB_COPY_FROM_PARENT, demo->xcb_window, demo->screen->root, 0, 0, demo->width, demo->height,
                      0, XCB_WINDOW_CLASS_INPUT_OUTPUT, demo->screen->root_visual, value_mask, value_list);

    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(demo->connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(demo->connection, cookie, 0);

    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(demo->connection, 0, 16, "WM_DELETE_WINDOW");
    demo->atom_wm_delete_window = xcb_intern_atom_reply(demo->connection, cookie2, 0);

    xcb_change_property(demo->connection, XCB_PROP_MODE_REPLACE, demo->xcb_window, (*reply).atom, 4, 32, 1,
                        &(*demo->atom_wm_delete_window).atom);
    free(reply);

    xcb_map_window(demo->connection, demo->xcb_window);

    // Force the x/y coordinates to 100,100 results are identical in consecutive
    // runs
    const uint32_t coords[] = {100, 100};
    xcb_configure_window(demo->connection, demo->xcb_window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);

    printf(" %d x %d window created with xcb !^_^! \n", demo->width, demo->height);
}


void xcb_initConnection(struct demo *demo)
{

    printf(" xcb init connection.\n");

    int scr;

    const char *display_envar = getenv("DISPLAY");
    
    if (display_envar == NULL || display_envar[0] == '\0')
    {
        ERR_EXIT("Environment variable DISPLAY requires a valid value.\nExiting ...\n", 0);
    }

    demo->connection = xcb_connect(NULL, &scr);
    
    if (xcb_connection_has_error(demo->connection) > 0)
    {
        ERR_EXIT("Cannot find a compatible Vulkan installable client driver (ICD).\nExiting ...\n", 0);
    }

    const xcb_setup_t * setup = xcb_get_setup(demo->connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    
    while (scr-- > 0)
        xcb_screen_next(&iter);

    demo->screen = iter.data;
}


static void demo_resize(struct demo *demo)
{
    uint32_t i;

    // Don't react to resize until after first initialization.
    if (!demo->prepared) {
        if (demo->is_minimized) {
            demo_prepare(demo);
        }
        return;
    }
    // In order to properly resize the window, we must re-create the swapchain
    // AND redo the command buffers, etc.
    //
    // First, perform part of the demo_cleanup() function:
    demo->prepared = false;
    vkDeviceWaitIdle(demo->device);

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

    vkDestroyImageView(demo->device, demo->depth.view, NULL);
    vkDestroyImage(demo->device, demo->depth.image, NULL);
    vkFreeMemory(demo->device, demo->depth.mem, NULL);

    for (i = 0; i < demo->swapchainImageCount; i++) {
        vkDestroyImageView(demo->device, demo->swapchain_image_resources[i].view, NULL);
        vkFreeCommandBuffers(demo->device, demo->cmd_pool, 1, &demo->swapchain_image_resources[i].cmd);
        vkDestroyBuffer(demo->device, demo->swapchain_image_resources[i].uniform_buffer, NULL);
        vkFreeMemory(demo->device, demo->swapchain_image_resources[i].uniform_memory, NULL);
    }
    vkDestroyCommandPool(demo->device, demo->cmd_pool, NULL);
    demo->cmd_pool = VK_NULL_HANDLE;
    if (demo->separate_present_queue) {
        vkDestroyCommandPool(demo->device, demo->present_cmd_pool, NULL);
    }
    free(demo->swapchain_image_resources);

    // Second, re-perform the demo_prepare() function, which will re-create the
    // swapchain:
    demo_prepare(demo);
}


static void handle_xcb_event(struct demo *demo, const xcb_generic_event_t *event)
{
    uint8_t event_code = event->response_type & 0x7f;
    switch (event_code)
    {
        case XCB_EXPOSE:
            // TODO: Resize window
            break;
        case XCB_CLIENT_MESSAGE:
            if ((*(xcb_client_message_event_t *)event).data.data32[0] == (*demo->atom_wm_delete_window).atom) {
                demo->quit = true;
            }
            break;
        case XCB_KEY_RELEASE: {
            const xcb_key_release_event_t *key = (const xcb_key_release_event_t *)event;

            switch (key->detail) {
                case 0x9:  // Escape
                    demo->quit = true;
                    break;
                case 0x71:  // left arrow key
                    demo->spin_angle -= demo->spin_increment;
                    break;
                case 0x72:  // right arrow key
                    demo->spin_angle += demo->spin_increment;
                    break;
                case 0x41:  // space bar
                    demo->pause = !demo->pause;
                    break;
            }
        } break;
        case XCB_CONFIGURE_NOTIFY: {
            const xcb_configure_notify_event_t *cfg = (const xcb_configure_notify_event_t *)event;
            if ((demo->width != cfg->width) || (demo->height != cfg->height)) {
                demo->width = cfg->width;
                demo->height = cfg->height;
                demo_resize(demo);
            }
        } break;
        default:
            break;
    }
}


void update_data_buffer(struct demo *demo)
{
    mat4x4 MVP, Model, VP;
    int matrixSize = sizeof(MVP);
    uint8_t *pData;

    mat4x4_mul(VP, demo->projection_matrix, demo->view_matrix);

    // Rotate around the Y axis
    mat4x4_dup(Model, demo->model_matrix);
    mat4x4_rotate(demo->model_matrix, Model, 0.0f, 1.0f, 0.0f, (float)degreesToRadians(demo->spin_angle));
    mat4x4_mul(MVP, VP, demo->model_matrix);

    VK_CHECK( vkMapMemory(demo->device, demo->swapchain_image_resources[demo->current_buffer].uniform_memory, 0, VK_WHOLE_SIZE, 0,
                      (void **)&pData) );

    memcpy(pData, (const void *)&MVP[0][0], matrixSize);

    vkUnmapMemory(demo->device, demo->swapchain_image_resources[demo->current_buffer].uniform_memory);
}



static void xcb_draw(struct demo *demo)
{
    VkResult err;

    // Ensure no more than FRAME_LAG renderings are outstanding
    vkWaitForFences(demo->device, 1, &demo->fences[demo->frame_index], VK_TRUE, UINT64_MAX);
    vkResetFences(demo->device, 1, &demo->fences[demo->frame_index]);

    do {
        // Get the index of the next available swapchain image:
        err = pFn_vkhr.fpAcquireNextImageKHR(demo->device, demo->swapchain, UINT64_MAX,
                demo->image_acquired_semaphores[demo->frame_index], VK_NULL_HANDLE, &demo->current_buffer);

        if (err == VK_ERROR_OUT_OF_DATE_KHR)
        {
            // demo->swapchain is out of date (e.g. the window was resized) and
            // must be recreated:
            demo_resize(demo);
        }
        else if (err == VK_SUBOPTIMAL_KHR)
        {
            // demo->swapchain is not as optimal as it could be, 
            // but the platform's presentation engine will still
            // present the image correctly.
            break;
        }
        else
        {
            assert(!err);
        }
    } while (err != VK_SUCCESS);

    update_data_buffer(demo);


    // Wait for the image acquired semaphore to be signaled to ensure
    // that the image won't be rendered to until the presentation
    // engine has fully released ownership to the application, and it is
    // okay to render to the image.
    VkPipelineStageFlags pipe_stage_flags;
    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.pWaitDstStageMask = &pipe_stage_flags;
    pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &demo->image_acquired_semaphores[demo->frame_index];
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &demo->swapchain_image_resources[demo->current_buffer].cmd;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &demo->draw_complete_semaphores[demo->frame_index];

    VK_CHECK( vkQueueSubmit(demo->graphics_queue, 1, &submit_info, demo->fences[demo->frame_index]) );

    if (demo->separate_present_queue)
    {
        // If we are using separate queues, change image ownership to the
        // present queue before presenting, waiting for the draw complete
        // semaphore and signalling the ownership released semaphore when finished
        VkFence nullFence = VK_NULL_HANDLE;
        pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &demo->draw_complete_semaphores[demo->frame_index];
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &demo->swapchain_image_resources[demo->current_buffer].graphics_to_present_cmd;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &demo->image_ownership_semaphores[demo->frame_index];
        
        VK_CHECK( vkQueueSubmit(demo->present_queue, 1, &submit_info, nullFence) );
    }

    // If we are using separate queues we have to wait for image ownership,
    // otherwise wait for draw complete
    VkPresentInfoKHR present = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = (demo->separate_present_queue) ? &demo->image_ownership_semaphores[demo->frame_index]
                                                          : &demo->draw_complete_semaphores[demo->frame_index],
        .swapchainCount = 1,
        .pSwapchains = &demo->swapchain,
        .pImageIndices = &demo->current_buffer,
    };


    err = pFn_vkhr.fpQueuePresentKHR(demo->present_queue, &present);
    demo->frame_index += 1;
    demo->frame_index %= FRAME_LAG;

    if (err == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // demo->swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        demo_resize(demo);
    }
    else if (err == VK_SUBOPTIMAL_KHR)
    {
        // demo->swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
    }
    else
    {
        assert(!err);
    }
}



void run_xcb(struct demo *demo)
{
    xcb_flush(demo->connection);

    while (!demo->quit)
    {
        xcb_generic_event_t *event;

        if (demo->pause)
        {
            event = xcb_wait_for_event(demo->connection);
        }
        else
        {
            event = xcb_poll_for_event(demo->connection);
        }

        while (event)
        {
            handle_xcb_event(demo, event);
            free(event);
            event = xcb_poll_for_event(demo->connection);
        }

        xcb_draw(demo);
        demo->curFrame++;
    }
}
