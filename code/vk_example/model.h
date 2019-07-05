#ifndef MODEL_H_
#define MODEL_H_

#include "demo.h"

struct texcube_vs_uniform {
    // Must start with MVP
    float mvp[4][4];
    float position[12 * 3][4];
    float attr[12 * 3][4];
};


void vk_upload_cube_data_buffers(struct demo * const demo);

#endif
