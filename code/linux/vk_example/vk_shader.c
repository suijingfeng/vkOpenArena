#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>


#include "vk_shader.h"


static VkShaderModule prepare_shader_module(struct demo *demo, const uint32_t *code, size_t size) {
    VkShaderModule module;
    VkShaderModuleCreateInfo moduleCreateInfo;
    VkResult  err;

    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;
    moduleCreateInfo.flags = 0;
    moduleCreateInfo.codeSize = size;
    moduleCreateInfo.pCode = code;

    err = vkCreateShaderModule(demo->device, &moduleCreateInfo, NULL, &module);
    assert(!err);

    return module;
}

void vk_prepare_vs(struct demo *demo)
{
    const uint32_t vs_code[] = {
#include "cube.vert.inc"
    };
    demo->vert_shader_module = prepare_shader_module(demo, vs_code, sizeof(vs_code));
}

void vk_prepare_fs(struct demo *demo)
{
    const uint32_t fs_code[] =
    {
#include "cube.frag.inc"
    };
    demo->frag_shader_module = prepare_shader_module(demo, fs_code, sizeof(fs_code));
}
