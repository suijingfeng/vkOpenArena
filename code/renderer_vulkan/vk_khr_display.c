#include "vk_instance.h"

void vk_displayInfo_f(void)
{
    uint32_t displayCnt = 0;
    uint32_t i = 0;
    //VkDisplayPropertiesKHR* pDisplayProps = NULL;
    // I have no idea, this just not work on ubuntu16.04 384.130 nvidia driver
    VK_CHECK( qvkGetPhysicalDeviceDisplayPropertiesKHR(vk.physical_device, &displayCnt, NULL) );

    if (displayCnt != 0)
    {
        VkDisplayPropertiesKHR* pDisplayProps = 
            (VkDisplayPropertiesKHR* )malloc( displayCnt * sizeof(VkDisplayPropertiesKHR) );
        VK_CHECK( qvkGetPhysicalDeviceDisplayPropertiesKHR(vk.physical_device, &displayCnt, pDisplayProps) );
        
        ri.Printf(PRINT_ALL, " -------- Total %d Minitor find --------\n", displayCnt);
        for (i = 0; i< displayCnt; ++i)
        {
            ri.Printf(PRINT_ALL, "[%d]: display name: %s, resolution: %dx%d",
                i, pDisplayProps[i].displayName, 
                pDisplayProps[i].physicalResolution.width, 
                pDisplayProps[i].physicalResolution.height);
        }
        ri.Printf(PRINT_ALL, " -------- --------------------- --------\n");
        
        free(pDisplayProps);
    }
}
