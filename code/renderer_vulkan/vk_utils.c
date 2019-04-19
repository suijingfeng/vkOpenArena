#include "VKimpl.h"
#include "vk_instance.h"
#include "vk_utils.h"


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


void printInstanceExtensionsSupported_f(void)
{
    uint32_t i = 0;

	uint32_t nInsExt = 0;
    // To retrieve a list of supported extensions before creating an instance
	VK_CHECK( qvkEnumerateInstanceExtensionProperties( NULL, &nInsExt, NULL) );

    assert(nInsExt > 0);

    VkExtensionProperties* pInsExt = (VkExtensionProperties *) malloc(sizeof(VkExtensionProperties) * nInsExt);
    
    VK_CHECK(qvkEnumerateInstanceExtensionProperties( NULL, &nInsExt, pInsExt));

    ri.Printf(PRINT_ALL, "\n");

    ri.Printf(PRINT_ALL, "----- Total %d Instance Extension Supported -----\n", nInsExt);
    for (i = 0; i < nInsExt; ++i)
    {            
        ri.Printf(PRINT_ALL, "%s\n", pInsExt[i].extensionName );
    }
    ri.Printf(PRINT_ALL, "----- ------------------------------------- -----\n\n");
   
    free(pInsExt);
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
}

