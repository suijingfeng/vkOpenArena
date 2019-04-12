#ifndef VK_VALIDATION_H_
#define VK_VALIDATION_H_

#ifndef NDEBUG

#include "VKimpl.h"

const char * cvtResToStr(VkResult result);

VKAPI_ATTR VkBool32 VKAPI_CALL vk_DebugCallback(
        VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT object_type,
        uint64_t object, size_t location, int32_t message_code, 
        const char* layer_prefix, const char* message, void* user_data );

void vk_createDebugCallback( PFN_vkDebugReportCallbackEXT qvkDebugCB);


#define VK_CHECK(function_call) { \
	VkResult result = function_call; \
	if (result != VK_SUCCESS) \
		fprintf(stderr, "Vulkan: error %s returned by %s \n", cvtResToStr(result), #function_call); \
}

#else

#define VK_CHECK(function_call)     function_call;

#endif


#endif
