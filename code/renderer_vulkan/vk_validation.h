#ifndef VK_VALIDATION_H_
#define VK_VALIDATION_H_

void VK_SettingDebugCallback( VkInstance hInstance );

void VK_DestroyDebugReportHandle( VkInstance hInstance );

void debug_vkapi_call(VkResult result, const char * const proc_str);

#endif
