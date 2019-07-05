#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "linmath.h"
#include "vk_common.h"
#include "model.h"

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


static void uniformBufUpdateMVP(struct texcube_vs_uniform * const pData, const struct demo * const demo)
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
    for (unsigned int i = 0; i < 12 * 3; i++)
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
