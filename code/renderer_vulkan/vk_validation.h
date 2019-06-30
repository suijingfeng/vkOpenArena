#ifndef VK_VALIDATION_H_
#define VK_VALIDATION_H_

void vk_createDebugCallback( void );

void vk_destroyDebugReportHandle( void );

void debug_vkapi_call(VkResult result, const char * const proc_str);

#endif
