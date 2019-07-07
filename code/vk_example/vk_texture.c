#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>


#include "vk_texture.h"
#include "vk_common.h"


static const char *tex_files[] = {
    "textures/plasmaa.tga",
    "textures/icona_blue.png", 
    "textures/img_lunarg.jpg",
    "textures/oldfiar2.jpg",
    "textures/bloodbg.jpg",
    "textures/grenfiar.jpg"
};

// #define STB_IMAGE_WRITE_STATIC
// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/*
static void R_SaveToJPEG(unsigned char* const pImg, uint32_t width, uint32_t height, char * const fileName )
{
	int error = stbi_write_jpg (fileName, width, height, 3, pImg, 90);    
    //int error = stbi_write_jpg_to_func
    
    if(error == 0)
    {
        printf("failed writing %s to the disk. \n", fileName);
    }
    else
    {
        printf("write %dx%d to %s success! \n", width, height, fileName);
    }
}
*/


static bool memory_type_from_properties(struct demo *demo, uint32_t typeBits, 
        VkFlags requirements_mask, uint32_t *typeIndex)
{
    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; ++i)
    {
        if ((typeBits & 1) == 1)
        {
            // Type is available, does it match user properties?
            if ((demo->memory_properties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask)
            {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    // No memory types matched, return failure
    return false;
}


static void prepare_texture_image(struct demo *demo, const char *filename, struct texture_object *tex_obj,
                                       VkImageTiling tiling, VkImageUsageFlags usage, VkFlags required_props)
{
    const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;
    int32_t tex_width;
    int32_t tex_height;
    uint32_t n;
    
    unsigned char *pSrc = stbi_load(filename, &tex_width, &tex_height, &n, 4);
    
    printf(" Texture loaded: %s, %dx%d, %d channals. \n", filename, tex_width, tex_height, n);

/*
    if (!loadTexture(filename, NULL, NULL, &tex_width, &tex_height))
    {
        ERR_EXIT("Failed to load textures", "Load Texture Failure");
    }
*/

    tex_obj->tex_width = tex_width;
    tex_obj->tex_height = tex_height;

    const VkImageCreateInfo image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = tex_format,
        .extent = {tex_width, tex_height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .flags = 0,
        .initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED,
    };

    VkMemoryRequirements mem_reqs;

    VK_CHECK( vkCreateImage(demo->device, &image_create_info, NULL, &tex_obj->image) );


    vkGetImageMemoryRequirements(demo->device, tex_obj->image, &mem_reqs);

    tex_obj->mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    tex_obj->mem_alloc.pNext = NULL;
    tex_obj->mem_alloc.allocationSize = mem_reqs.size;
    tex_obj->mem_alloc.memoryTypeIndex = 0;

    bool pass = memory_type_from_properties(demo, mem_reqs.memoryTypeBits, required_props,
            &tex_obj->mem_alloc.memoryTypeIndex);
    assert(pass);

    /* allocate memory */
    VK_CHECK( vkAllocateMemory(demo->device, &tex_obj->mem_alloc, NULL, &(tex_obj->mem)) );

    /* bind memory */
    VK_CHECK( vkBindImageMemory(demo->device, tex_obj->image, tex_obj->mem, 0) );

    if (required_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        const VkImageSubresource subres = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .arrayLayer = 0,
        };
        
        VkSubresourceLayout layout;
        
        void *data;

        vkGetImageSubresourceLayout(demo->device, tex_obj->image, &subres, &layout);

        VK_CHECK( vkMapMemory(demo->device, tex_obj->mem, 0, tex_obj->mem_alloc.allocationSize, 0, &data) );

        memcpy(data, pSrc, tex_width*tex_height*4);        
        vkUnmapMemory(demo->device, tex_obj->mem);
    }

    tex_obj->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}


static void transform_image_layout( struct demo *demo, VkImage image, VkImageAspectFlags aspectMask,
        VkImageLayout old_image_layout, VkImageLayout new_image_layout,
        VkAccessFlagBits srcAccessMask, VkPipelineStageFlags src_stages,
        VkPipelineStageFlags dest_stages)
{
    assert(demo->cmd);

    VkImageMemoryBarrier image_memory_barrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                                 .pNext = NULL,
                                                 .srcAccessMask = srcAccessMask,
                                                 .dstAccessMask = 0,
                                                 .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                 .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                 .oldLayout = old_image_layout,
                                                 .newLayout = new_image_layout,
                                                 .image = image,
                                                 .subresourceRange = {aspectMask, 0, 1, 0, 1}};

    switch (new_image_layout)
    
    {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            /* Make sure anything that was copying from this image has completed */
            image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            break;

        default:
            image_memory_barrier.dstAccessMask = 0;
            break;
    }

    VkImageMemoryBarrier *pmemory_barrier = &image_memory_barrier;

    vkCmdPipelineBarrier(demo->cmd, src_stages, dest_stages, 0, 0, NULL, 0, NULL, 1, pmemory_barrier);
}


void vk_prepare_textures(struct demo *demo)
{
    const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormatProperties props;
    uint32_t i;

    vkGetPhysicalDeviceFormatProperties(demo->gpu, tex_format, &props);

    for (i = 0; i < DEMO_TEXTURE_COUNT; ++i)
    {

        if ((props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) && !demo->use_staging_buffer)
        {
            printf( " Device can texture using linear textures.\n");
            prepare_texture_image(demo, tex_files[i], &demo->textures[i], 
					VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            // Nothing in the pipeline needs to be complete to start, and don't allow fragment
            // shader to run until layout transition completes
            transform_image_layout(demo, demo->textures[i].image, VK_IMAGE_ASPECT_COLOR_BIT, 
					VK_IMAGE_LAYOUT_PREINITIALIZED, demo->textures[i].imageLayout, 0,
				   	VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            demo->staging_texture.image = 0;
        }
        else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
        {
            /* Must use staging buffer to copy linear texture to optimized */
            printf( " Device can texture using optimal Tiling Features textures.\n");

            memset(&demo->staging_texture, 0, sizeof(demo->staging_texture));
            prepare_texture_image(demo, tex_files[i], &demo->staging_texture, VK_IMAGE_TILING_LINEAR,
                                       VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            prepare_texture_image(demo, tex_files[i], &demo->textures[i], VK_IMAGE_TILING_OPTIMAL,
                                       (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            transform_image_layout(demo, demo->staging_texture.image, VK_IMAGE_ASPECT_COLOR_BIT,
				   	VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, 
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            transform_image_layout(demo, demo->textures[i].image, VK_IMAGE_ASPECT_COLOR_BIT, 
					VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
				   	VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            VkImageCopy copy_region = {
                .srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
                .srcOffset = {0, 0, 0},
                .dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
                .dstOffset = {0, 0, 0},
                .extent = {demo->staging_texture.tex_width, demo->staging_texture.tex_height, 1},
            };
            vkCmdCopyImage(demo->cmd, demo->staging_texture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    demo->textures[i].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

            transform_image_layout(demo, demo->textures[i].image, VK_IMAGE_ASPECT_COLOR_BIT,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, demo->textures[i].imageLayout,
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        }
        else {
            /* Can't support VK_FORMAT_R8G8B8A8_UNORM !? */
            assert(!"No support for R8G8B8A8_UNORM as texture image format");
        }

        const VkSamplerCreateInfo sampler = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = NULL,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .mipLodBias = 0.0f,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = 1,
            .compareOp = VK_COMPARE_OP_NEVER,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
            .unnormalizedCoordinates = VK_FALSE,
        };

        VkImageViewCreateInfo view = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .image = VK_NULL_HANDLE,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = tex_format,
            .components =
                {
                    VK_COMPONENT_SWIZZLE_R,
                    VK_COMPONENT_SWIZZLE_G,
                    VK_COMPONENT_SWIZZLE_B,
                    VK_COMPONENT_SWIZZLE_A,
                },
            .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
            .flags = 0,
        };

        /* create sampler */
        VK_CHECK ( vkCreateSampler(demo->device, &sampler, NULL, &demo->textures[i].sampler) );

        /* create image view */
        view.image = demo->textures[i].image;
        
        VK_CHECK ( vkCreateImageView(demo->device, &view, NULL, &demo->textures[i].view) );
    }
}


// =========================================
//  depth image
// =========================================

void vk_prepare_depth(struct demo * const pDemo)
{
    const VkFormat depth_format = VK_FORMAT_D16_UNORM;
    const VkImageCreateInfo image = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depth_format,
        .extent = {pDemo->width, pDemo->height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .flags = 0,
    };

    
    VkImageViewCreateInfo view = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .image = VK_NULL_HANDLE,
        .format = depth_format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, 
            .baseMipLevel = 0, 
            .levelCount = 1, 
            .baseArrayLayer = 0,
            .layerCount = 1 },
        .flags = 0,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
    };

    
    VkMemoryRequirements mem_reqs;

    pDemo->depth.format = depth_format;

    /* create image */
    VK_CHECK ( vkCreateImage(pDemo->device, &image, NULL, &pDemo->depth.image) );


    vkGetImageMemoryRequirements(pDemo->device, pDemo->depth.image, &mem_reqs);

    pDemo->depth.mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    pDemo->depth.mem_alloc.pNext = NULL;
    pDemo->depth.mem_alloc.allocationSize = mem_reqs.size;
    pDemo->depth.mem_alloc.memoryTypeIndex = 0;

    bool pass = memory_type_from_properties(pDemo, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                       &pDemo->depth.mem_alloc.memoryTypeIndex);
    assert(pass);

    /* allocate memory */
    VK_CHECK( vkAllocateMemory(pDemo->device, &pDemo->depth.mem_alloc, NULL, &pDemo->depth.mem) );

    /* bind memory */
    VK_CHECK( vkBindImageMemory(pDemo->device, pDemo->depth.image, pDemo->depth.mem, 0) );

    /* create image view */
    view.image = pDemo->depth.image;
    VK_CHECK( vkCreateImageView(pDemo->device, &view, NULL, &pDemo->depth.view) );
}
