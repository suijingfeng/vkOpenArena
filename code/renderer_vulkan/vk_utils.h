#ifndef VK_UTILS_H_
#define VK_UTILS_H_

void printDeviceExtensionsSupported_f(void);
void printVulkanInfo_f( void );
void printPresentModesSupported_f(void);
void printMemUsageInfo_f(void);
void printSurfaceFormatSupported_f(void);
const char * ColorSpaceEnum2str(enum VkColorSpaceKHR cs);
const char * VkFormatEnum2str(VkFormat fmt);
#endif
