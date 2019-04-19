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

static void updateMVP(struct texcube_vs_uniform * pData, struct demo *demo)
{
    // mat4x4 MVP;
    mat4x4 VP;
    mat4x4_mul(VP, demo->projection_matrix, demo->view_matrix);
    mat4x4_mul(pData->mvp, VP, demo->model_matrix);
    //memcpy(data.mvp, MVP, sizeof(MVP));
    //dumpMatrix("MVP", MVP);
}


void uploadDataToUniformBuffer(struct texcube_vs_uniform * const pData, struct demo * const demo)
{
    
    updateMVP(pData, demo);

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


void vk_createUniformBufferAndBindItWithMemory(struct demo * pDemo, VkBufferCreateInfo * pBufInfo, VkBuffer * pUniBuf, VkDeviceMemory* pDevMem )
{
    VK_CHECK( vkCreateBuffer(pDemo->device, pBufInfo, NULL, pUniBuf) );

    VkMemoryRequirements mem_reqs;

    vkGetBufferMemoryRequirements(pDemo->device, *pUniBuf, &mem_reqs);

    VkMemoryAllocateInfo mem_alloc;

    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.allocationSize = mem_reqs.size;
    mem_alloc.memoryTypeIndex = 0;

    bool pass = memory_type_from_properties(pDemo, mem_reqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &mem_alloc.memoryTypeIndex);
    assert(pass);

    VK_CHECK( vkAllocateMemory(pDemo->device, &mem_alloc, NULL, pDevMem) );

    VK_CHECK( vkBindBufferMemory(pDemo->device, *pUniBuf, *pDevMem, 0) );
}


void prepare_cube_data_buffers(struct demo *demo)
{

    struct texcube_vs_uniform data;
   
    uploadDataToUniformBuffer(&data, demo);

    VkBufferCreateInfo buf_info;
    memset(&buf_info, 0, sizeof(buf_info));
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buf_info.size = sizeof(data);


 
    for (unsigned int i = 0; i < demo->swapchainImageCount; i++)
    {
        vk_createUniformBufferAndBindItWithMemory(demo, &buf_info, &demo->swapchain_image_resources[i].uniform_buffer, &demo->swapchain_image_resources[i].uniform_memory);

        uint8_t *pData = NULL;

        VK_CHECK( vkMapMemory(demo->device, demo->swapchain_image_resources[i].uniform_memory, 0, VK_WHOLE_SIZE, 0, (void **)&pData) );

        memcpy(pData, &data, sizeof(data) );

        vkUnmapMemory(demo->device, demo->swapchain_image_resources[i].uniform_memory);

    }
}
