#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "linmath.h"
#include "vk_common.h"
#include "model.h"

struct texcube_vs_uniform {
    // Must start with MVP
    float mvp[4][4];
    float position[12 * 3][4];
    float attr[12 * 3][4];
};

//-------------------------------------------------------------------------
// Mesh and VertexFormat Data
//-------------------------------------------------------------------------
// clang-format off
static const float g_vertex_buffer_data[] = 
{
    -1.0f,-1.0f,-1.0f,  // -X side
    -1.0f,-1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // -Z side
     1.0f, 1.0f,-1.0f,
     1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // -Y side
     1.0f,-1.0f,-1.0f,
     1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f,-1.0f,
     1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,

    -1.0f, 1.0f,-1.0f,  // +Y side
    -1.0f, 1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
     1.0f, 1.0f, 1.0f,
     1.0f, 1.0f,-1.0f,

     1.0f, 1.0f,-1.0f,  // +X side
     1.0f, 1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f,-1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,

    -1.0f, 1.0f, 1.0f,  // +Z side
    -1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
};

static const float g_uv_buffer_data[] = 
{
    0.0f, 1.0f,  // -X side
    1.0f, 1.0f,
    1.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 0.0f,
    0.0f, 1.0f,

    1.0f, 1.0f,  // -Z side
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 0.0f,

    1.0f, 0.0f,  // -Y side
    1.0f, 1.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 0.0f,

    1.0f, 0.0f,  // +Y side
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,

    1.0f, 0.0f,  // +X side
    0.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,

    0.0f, 0.0f,  // +Z side
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
};


static void uniformBufUpdateMVP(struct texcube_vs_uniform * const pData, struct demo * const demo)
{
    // mat4x4 MVP;
    mat4x4 VP;
    mat4x4_mul(VP, demo->projection_matrix, demo->view_matrix);
    mat4x4_mul(pData->mvp, VP, demo->model_matrix);
    //memcpy(data.mvp, MVP, sizeof(MVP));
    //dumpMatrix("MVP", MVP);
}


static void uniformBufUploadVertexData(struct texcube_vs_uniform * const pData, const struct demo * const demo)
{
	uint32_t i;
    for (i = 0; i < 12 * 3; ++i)
    {
        pData->position[i][0] = g_vertex_buffer_data[i * 3];
        pData->position[i][1] = g_vertex_buffer_data[i * 3 + 1];
        pData->position[i][2] = g_vertex_buffer_data[i * 3 + 2];
        pData->position[i][3] = 1.0f;
        
		pData->attr[i][0] = g_uv_buffer_data[2 * i];
        pData->attr[i][1] = g_uv_buffer_data[2 * i + 1];
        pData->attr[i][2] = 0;
        pData->attr[i][3] = 0;
    }
}

// bool memory_type_from_properties(struct demo *demo, uint32_t typeBits, 
//         VkFlags requirements_mask, uint32_t *typeIndex);
//  

static uint32_t FindMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties * const pMemProp,
        uint32_t typeBits, VkFlags requirements_mask)
{
   // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; ++i)
    {
        if ((typeBits & 1) == 1)
        {
            // Type is available, does it match user properties?
            if ((pMemProp->memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask)
            {
                return i;
            }
        }
        typeBits >>= 1;
    }

    fprintf(stderr, " can not find suitable memmeory. \n");

    return 0;
}

static void vk_createUniformBufferAndBindItWithMemory(const struct demo * const pDemo, 
        VkBufferCreateInfo * pBufInfo, VkBuffer * const pUniBuf, VkDeviceMemory * const pDevMem )
{

    VK_CHECK( vkCreateBuffer(pDemo->device, pBufInfo, NULL, pUniBuf) );
    
    printf( " Uniform Buffer Created. \n");

    VkMemoryRequirements mem_reqs;

    vkGetBufferMemoryRequirements(pDemo->device, *pUniBuf, &mem_reqs);

    VkMemoryAllocateInfo mem_alloc_info;
    mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc_info.pNext = NULL;
    mem_alloc_info.allocationSize = mem_reqs.size;
    mem_alloc_info.memoryTypeIndex = FindMemoryTypeIndex(  
            &pDemo->memory_properties, mem_reqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    VK_CHECK( vkAllocateMemory(pDemo->device, &mem_alloc_info, NULL, pDevMem) );
    
    printf( " Uniform Buffer Memory Allocated. \n");

    VK_CHECK( vkBindBufferMemory(pDemo->device, *pUniBuf, *pDevMem, 0) );
}


void vk_upload_cube_data_buffers(struct demo * const demo)
{

    struct texcube_vs_uniform data;
    
    uniformBufUpdateMVP(&data, demo);
    uniformBufUploadVertexData(&data, demo);

    VkBufferCreateInfo buf_info;
    memset(&buf_info, 0, sizeof(buf_info));
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buf_info.size = sizeof(data);

    for (unsigned int i = 0; i < demo->swapchainImageCount; ++i)
    {
        vk_createUniformBufferAndBindItWithMemory(demo, &buf_info, 
                &demo->swapchain_image_resources[i].uniform_buffer, 
                &demo->swapchain_image_resources[i].uniform_memory);

        uint8_t *pData = NULL;

        VK_CHECK( vkMapMemory(demo->device, demo->swapchain_image_resources[i].uniform_memory,
                    0, VK_WHOLE_SIZE, 0, (void **)&pData) );

        memcpy(pData, &data, sizeof(data) );

        vkUnmapMemory(demo->device, demo->swapchain_image_resources[i].uniform_memory);
    }
}


void vk_prepare_descriptor_set(struct demo * const demo)
{
	uint32_t i;

    VkDescriptorImageInfo tex_descs[DEMO_TEXTURE_COUNT];
    //memset(&tex_descs, 0, sizeof(tex_descs));
    for (i = 0; i < DEMO_TEXTURE_COUNT; ++i)
    {
        tex_descs[i].sampler = demo->textures[i].sampler;
        tex_descs[i].imageView = demo->textures[i].view;
        tex_descs[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    }



    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = NULL,
        .descriptorPool = demo->desc_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &demo->desc_layout
	};

	
    VkDescriptorBufferInfo buffer_info;
    buffer_info.offset = 0;
    buffer_info.range = sizeof(struct texcube_vs_uniform);

    
	VkWriteDescriptorSet writes[2];

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

    for (i = 0; i < demo->swapchainImageCount; ++i)
    {
        VK_CHECK( vkAllocateDescriptorSets(demo->device, &alloc_info,
                    &demo->swapchain_image_resources[i].descriptor_set) );

        buffer_info.buffer = demo->swapchain_image_resources[i].uniform_buffer;
        writes[0].dstSet = demo->swapchain_image_resources[i].descriptor_set;
        writes[1].dstSet = demo->swapchain_image_resources[i].descriptor_set;
        vkUpdateDescriptorSets(demo->device, 2, writes, 0, NULL);
    }
}
