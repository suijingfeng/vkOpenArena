#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "vk_common.h"
#include "vk_shader.h"

static VkShaderModule prepare_shader_module(struct demo *demo, const unsigned char* code, size_t size)
{
    VkShaderModule module;
    VkShaderModuleCreateInfo moduleCreateInfo;

    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;
    moduleCreateInfo.flags = 0;
    moduleCreateInfo.codeSize = size;
    moduleCreateInfo.pCode = (const uint32_t*)code;

    VK_CHECK( vkCreateShaderModule(demo->device, &moduleCreateInfo, NULL, &module) );

    return module;
}


extern unsigned char cube_vert_spv[];
extern int cube_vert_spv_size;

extern unsigned char cube_frag_spv[];
extern int cube_frag_spv_size;


void vk_prepare_vs(struct demo *demo)
{
    demo->vert_shader_module =
        prepare_shader_module(demo, cube_vert_spv, cube_vert_spv_size);
}

void vk_prepare_fs(struct demo *demo)
{
    demo->frag_shader_module = 
        prepare_shader_module(demo, cube_frag_spv, cube_frag_spv_size);
}
