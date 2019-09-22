#include "vk_instance.h"
#include "ref_import.h" 

extern PFN_vkGetInstanceProcAddr qvkGetInstanceProcAddr;


// VK_KHR_display
static PFN_vkGetPhysicalDeviceDisplayPropertiesKHR         qvkGetPhysicalDeviceDisplayPropertiesKHR;
static PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR    qvkGetPhysicalDeviceDisplayPlanePropertiesKHR;
static PFN_vkGetDisplayPlaneSupportedDisplaysKHR           qvkGetDisplayPlaneSupportedDisplaysKHR;
static PFN_vkGetDisplayModePropertiesKHR                   qvkGetDisplayModePropertiesKHR;
static PFN_vkCreateDisplayModeKHR                          qvkCreateDisplayModeKHR;
static PFN_vkGetDisplayPlaneCapabilitiesKHR                qvkGetDisplayPlaneCapabilitiesKHR;
static PFN_vkCreateDisplayPlaneSurfaceKHR                  qvkCreateDisplayPlaneSurfaceKHR;



void VK_GetExtraProcAddr(VkInstance hInstance)
{

#define INIT_INSTANCE_FUNCTION(func)                                    \
    q##func = (PFN_##func) qvkGetInstanceProcAddr(hInstance, #func);    \
    if (q##func == NULL) {                                              \
        ri.Error(ERR_FATAL, "Failed to find entrypoint %s", #func);     \
    }
    
    // The platform-specific extensions allow a VkSurface object to be
    // created that represents a native window owned by the operating
    // system or window system. These extensions are typically used to
    // render into a window with no border that covers an entire display
    // it is more often efficient to render directly to a display instead.
    if(vk.isKhrDisplaySupported)
    {
        ri.Printf(PRINT_ALL, " VK_KHR_Display Supported, Loading functions for this instance extention. \n");

        INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceDisplayPropertiesKHR);
        INIT_INSTANCE_FUNCTION(vkGetPhysicalDeviceDisplayPlanePropertiesKHR);
        INIT_INSTANCE_FUNCTION(vkGetDisplayPlaneSupportedDisplaysKHR);
        INIT_INSTANCE_FUNCTION(vkGetDisplayModePropertiesKHR);
        INIT_INSTANCE_FUNCTION(vkCreateDisplayModeKHR);
        INIT_INSTANCE_FUNCTION(vkGetDisplayPlaneCapabilitiesKHR);
        INIT_INSTANCE_FUNCTION(vkCreateDisplayPlaneSurfaceKHR);
    }

#undef INIT_INSTANCE_FUNCTION

}


void VK_CleatExtraProcAddr(void)
{
    qvkGetPhysicalDeviceDisplayPropertiesKHR        = NULL;
    qvkGetPhysicalDeviceDisplayPlanePropertiesKHR   = NULL;
    qvkGetDisplayPlaneSupportedDisplaysKHR          = NULL;
    qvkGetDisplayModePropertiesKHR                  = NULL;
    qvkCreateDisplayModeKHR                         = NULL;
    qvkGetDisplayPlaneCapabilitiesKHR               = NULL;
    qvkCreateDisplayPlaneSurfaceKHR                 = NULL;
}


static VkDisplayKHR DisplayArray[4];

static void vk_get_DisplayModeProps(VkDisplayKHR hDis)
{
    uint32_t i = 0; 
    uint32_t disModeCnt = 0;
    
    VK_CHECK( qvkGetDisplayModePropertiesKHR(vk.physical_device, hDis, &disModeCnt, NULL) );

    if (disModeCnt > 0)
    {
        VkDisplayModePropertiesKHR * pModeProps = (VkDisplayModePropertiesKHR *) 
            malloc( disModeCnt * sizeof(VkDisplayModePropertiesKHR) );

        memset(pModeProps, 0, disModeCnt * sizeof(VkDisplayModePropertiesKHR));
        VK_CHECK( qvkGetDisplayModePropertiesKHR(vk.physical_device, hDis, &disModeCnt, pModeProps) );

        for (i = 0; i<disModeCnt; ++i)
        {
            ri.Printf(PRINT_ALL, " Display Mode Propertie %d: %dx%d %.1f hz. \n",
                i, pModeProps[i].parameters.visibleRegion.width, pModeProps[i].parameters.visibleRegion.height,
                pModeProps[i].parameters.refreshRate / 1000.0f  );
        }

        free(pModeProps);
    }
}


void vk_displayInfo_f(void)
{
    
	if (vk.isKhrDisplaySupported)
	{
		uint32_t displayCnt = 0;
		uint32_t i = 0;
		//VkDisplayPropertiesKHR* pDisplayProps = NULL;
		// I have no idea, this just not work on ubuntu16.04 384.130 nvidia driver
		// ubuntu18.04 Gnome, Nvidia 390.116, Aspire-V3-772, GTX760 not working
		VK_CHECK(qvkGetPhysicalDeviceDisplayPropertiesKHR(vk.physical_device, &displayCnt, NULL));

		if (displayCnt != 0)
		{
			VkDisplayPropertiesKHR* pDisplayProps =
				(VkDisplayPropertiesKHR*)malloc(displayCnt * sizeof(VkDisplayPropertiesKHR));

			memset(pDisplayProps, 0, displayCnt * sizeof(VkDisplayPropertiesKHR));

			VK_CHECK(qvkGetPhysicalDeviceDisplayPropertiesKHR(vk.physical_device, &displayCnt, pDisplayProps));

			ri.Printf(PRINT_ALL, " -------- Total %d Minitor find --------\n", displayCnt);
			for (i = 0; i < displayCnt; ++i)
			{
				ri.Printf(PRINT_ALL, " Display %d: %s. \n Resolution: %dx%d, Dimension: %dx%d. \n",
					i, pDisplayProps[i].displayName,
					pDisplayProps[i].physicalResolution.width,
					pDisplayProps[i].physicalResolution.height,
					pDisplayProps[i].physicalDimensions.width,
					pDisplayProps[i].physicalDimensions.height);

				if (pDisplayProps[i].planeReorderPossible)
					ri.Printf(PRINT_ALL, " Support Reorder. ");
				if (pDisplayProps[i].persistentContent)
					ri.Printf(PRINT_ALL, " Support Infrequent update for power saving. ");

			}
			ri.Printf(PRINT_ALL, " -------- --------------------- --------\n");


			for (i = 0; (i < 4) && (i < displayCnt); ++i)
				DisplayArray[i] = pDisplayProps[i].display;

			free(pDisplayProps);

			// ensure that DisplayArray[0] != VK_NULL_HANDLE
			//
			vk_get_DisplayModeProps(DisplayArray[0]);
		}
	}
	else
	{
		ri.Printf(PRINT_ALL, " Khr Display not supported on your OS. \n");
	}
}
