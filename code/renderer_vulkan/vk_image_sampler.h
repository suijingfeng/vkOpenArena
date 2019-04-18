#ifndef IMAGE_SAMPLER_H_
#define IMAGE_SAMPLER_H_

#include "VKimpl.h"

void vk_free_sampler(void);
VkSampler vk_find_sampler( VkBool32 isMipmap, VkBool32 isRepeatTexture );

//void vk_set_sampler(int m);

#endif
