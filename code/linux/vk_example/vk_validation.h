#ifndef VK_VALIDATION_H_
#define VK_VALIDATION_H_

#include "vk_common.h"


#ifndef NDEBUG


const char * cvtResToStr(VkResult result);

VKAPI_ATTR VkBool32 VKAPI_CALL vk_DebugCallback(
        VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT object_type,
        uint64_t object, size_t location, int32_t message_code, 
        const char* layer_prefix, const char* message, void* user_data );

void vk_createDebugCallback( PFN_vkDebugReportCallbackEXT qvkDebugCB);


#endif


#endif
