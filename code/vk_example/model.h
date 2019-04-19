#ifndef MODEL_H_
#define MODEL_H_

#include "demo.h"

struct texcube_vs_uniform {
    // Must start with MVP
    float mvp[4][4];
    float position[12 * 3][4];
    float attr[12 * 3][4];
};


bool memory_type_from_properties(struct demo *demo, uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex);
void prepare_cube_data_buffers(struct demo *demo);

#endif
