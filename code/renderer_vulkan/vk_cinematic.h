#ifndef VK_CINEMATIC_H_
#define VK_CINEMATIC_H_

#include "tr_shader.h"

void vk_initScratchImage(void);
void vk_destroyScratchImage(void);

void R_SetCinematicShader(const struct shader_s * const pShader);

#endif
