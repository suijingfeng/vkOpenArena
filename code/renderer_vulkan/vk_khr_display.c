#include "vk_instance.h"


VkDisplayKHR DisplayArray[4];


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
            ri.Printf(PRINT_ALL, " Display %d: %s. \n Resolution: %dx%d, Dimension: %dx%d. \n",
                i, pDisplayProps[i].displayName, 
                pDisplayProps[i].physicalResolution.width, 
                pDisplayProps[i].physicalResolution.height,
                pDisplayProps[i].physicalDimensions.width, 
                pDisplayProps[i].physicalDimensions.height );

            if(pDisplayProps[i].planeReorderPossible)
                ri.Printf(PRINT_ALL, " Support Reorder. ");
            if(pDisplayProps[i].persistentContent)
                ri.Printf(PRINT_ALL, " Support Infrequent update for power saving. ");

        }
        ri.Printf(PRINT_ALL, " -------- --------------------- --------\n");

    
        for(i = 0; (i<4) && (i<displayCnt); ++i)
            DisplayArray[i] = pDisplayProps[i].display;
        
        free(pDisplayProps);
    }


    uint32_t disModeCnt = 0;
    
    VK_CHECK( qvkGetDisplayModePropertiesKHR(vk.physical_device, DisplayArray[0], &disModeCnt, NULL) );

    if (disModeCnt > 0)
    {
        VkDisplayModePropertiesKHR * pModeProps = (VkDisplayModePropertiesKHR *) 
            malloc( disModeCnt * sizeof(VkDisplayModePropertiesKHR) );

        VK_CHECK( qvkGetDisplayModePropertiesKHR(vk.physical_device, DisplayArray[0], &disModeCnt, pModeProps) );

        for (i = 0; i<disModeCnt; ++i)
        {
            ri.Printf(PRINT_ALL, " Display Mode Propertie %d: %dx%d %.1f hz. \n",
                i, pModeProps[i].parameters.visibleRegion.width, pModeProps[i].parameters.visibleRegion.height,
                pModeProps[i].parameters.refreshRate / 1000.0f  );
        }

        free(pModeProps);
    }


}
