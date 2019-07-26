#include "VKimpl.h"
#include "vk_instance.h"
#include "vk_utils.h"
#include "vk_image.h"

#include "ref_import.h" 


extern uint32_t R_GetNumberOfImageCreated(void);
extern uint32_t R_GetStagingBufferSize(void);
extern uint32_t R_GetScreenShotBufferSizeKB(void)
;

void printMemUsageInfo_f(void)
{
    // approm	 for debug info
    ri.Printf(PRINT_ALL, " %d image created, %d MB GPU memory used for store those image. \n", 
           R_GetNumberOfImageCreated() , 
           R_GetGpuMemConsumedByImage() );

    //  up bound, just informational
    ri.Printf(PRINT_ALL, " %d KB staging buffer( used for upload image to GPU). \n", 
           R_GetStagingBufferSize() / 1024 + 1);

    ri.Printf(PRINT_ALL, " %d KB screenshut buffer. \n", R_GetScreenShotBufferSizeKB());

}


void printDeviceExtensionsSupported_f(void)
{
    uint32_t nDevExts = 0;

    // To query the extensions available to a given physical device
    VK_CHECK( qvkEnumerateDeviceExtensionProperties( vk.physical_device, NULL, &nDevExts, NULL) );

    assert(nDevExts > 0);

    VkExtensionProperties* pDevExt = 
        (VkExtensionProperties *) malloc(sizeof(VkExtensionProperties) * nDevExts);

    qvkEnumerateDeviceExtensionProperties(
            vk.physical_device, NULL, &nDevExts, pDevExt);


    ri.Printf(PRINT_ALL, "--------- Total %d Device Extension Supported ---------\n", nDevExts);
    uint32_t i;
    for (i=0; i<nDevExts; ++i)
    {
        ri.Printf(PRINT_ALL, " %s \n", pDevExt[i].extensionName);
    }
    ri.Printf(PRINT_ALL, "--------- ----------------------------------- ---------\n");

    free(pDevExt);
}



void printVulkanInfo_f( void ) 
{
    ri.Printf( PRINT_ALL, "\n-------- Active 3D API: Vulkan --------\n" );

    // To query general properties of physical devices once enumerated
    VkPhysicalDeviceProperties props;
    qvkGetPhysicalDeviceProperties(vk.physical_device, &props);

    uint32_t major = VK_VERSION_MAJOR(props.apiVersion);
    uint32_t minor = VK_VERSION_MINOR(props.apiVersion);
    uint32_t patch = VK_VERSION_PATCH(props.apiVersion);

    const char* device_type;
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        device_type = "INTEGRATED_GPU";
    else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        device_type = "DISCRETE_GPU";
    else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
        device_type = "VIRTUAL_GPU";
    else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
        device_type = "CPU";
    else
        device_type = "Unknown";

    const char* vendor_name = "unknown";
    if (props.vendorID == 0x1002) {
        vendor_name = "Advanced Micro Devices, Inc.";
    } else if (props.vendorID == 0x10DE) {
        vendor_name = "NVIDIA";
    } else if (props.vendorID == 0x8086) {
        vendor_name = "Intel Corporation";
    }

    ri.Printf(PRINT_ALL, "Vk api version: %d.%d.%d\n", major, minor, patch);
    ri.Printf(PRINT_ALL, "Vk driver version: %d\n", props.driverVersion);
    ri.Printf(PRINT_ALL, "Vk vendor id: 0x%X (%s)\n", props.vendorID, vendor_name);
    ri.Printf(PRINT_ALL, "Vk device id: 0x%X\n", props.deviceID);
    ri.Printf(PRINT_ALL, "Vk device type: %s\n", device_type);
    ri.Printf(PRINT_ALL, "Vk device name: %s\n", props.deviceName);

    printInstanceExtensionsSupported_f();
    printDeviceExtensionsSupported_f();
    printMemUsageInfo_f();
    
    ri.Printf(PRINT_ALL, " Pretentation mode: ");
    switch( vk.present_mode )
    {
        case VK_PRESENT_MODE_IMMEDIATE_KHR:
            ri.Printf(PRINT_ALL, " VK_PRESENT_MODE_IMMEDIATE_KHR. \n"); break;
        case VK_PRESENT_MODE_MAILBOX_KHR:
            ri.Printf(PRINT_ALL, " VK_PRESENT_MODE_MAILBOX_KHR. \n"); break;
        case VK_PRESENT_MODE_FIFO_KHR:
            ri.Printf(PRINT_ALL, " VK_PRESENT_MODE_FIFO_KHR. \n"); break;
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
            ri.Printf(PRINT_ALL, " VK_PRESENT_MODE_FIFO_RELAXED_KHR. \n"); break;
        default:
            ri.Printf(PRINT_WARNING, " %d. \n", vk.present_mode); break;
    }
    
}

void printPresentModesSupported_f(void)
{

    uint32_t nPM = 0;

    qvkGetPhysicalDeviceSurfacePresentModesKHR(vk.physical_device, vk.surface, &nPM, NULL);

    assert(nPM > 0);

    VkPresentModeKHR * pPresentModes = (VkPresentModeKHR *) malloc( nPM * sizeof(VkPresentModeKHR) );

    qvkGetPhysicalDeviceSurfacePresentModesKHR(vk.physical_device, vk.surface, &nPM, pPresentModes);

    ri.Printf(PRINT_ALL, "-------- Total %d present mode supported. -------- \n", nPM);
    uint32_t i;
    for (i = 0; i < nPM; ++i)
    {
        switch( pPresentModes[i] )
        {
            case VK_PRESENT_MODE_IMMEDIATE_KHR:
                ri.Printf(PRINT_ALL, " [%d] VK_PRESENT_MODE_IMMEDIATE_KHR \n", i);
                break;
            case VK_PRESENT_MODE_MAILBOX_KHR:
                ri.Printf(PRINT_ALL, " [%d] VK_PRESENT_MODE_MAILBOX_KHR \n", i);
                break;
            case VK_PRESENT_MODE_FIFO_KHR:
                ri.Printf(PRINT_ALL, " [%d] VK_PRESENT_MODE_FIFO_KHR \n", i);
                break;
            case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
                ri.Printf(PRINT_ALL, " [%d] VK_PRESENT_MODE_FIFO_RELAXED_KHR \n", i);
                break;
            default:
                ri.Printf(PRINT_WARNING, "Unknown presentation mode: %d. \n",
                        pPresentModes[i]);
                break;
        }
    }

    free(pPresentModes);
}
