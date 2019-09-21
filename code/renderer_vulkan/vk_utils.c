#include "VKimpl.h"
#include "vk_instance.h"
#include "vk_utils.h"
#include "vk_image.h"

#include "ref_import.h" 


extern uint32_t R_GetNumberOfImageCreated(void);
extern uint32_t R_GetStagingBufferSize(void);
extern uint32_t R_GetScreenShotBufferSizeKB(void);

const char * ColorSpaceEnum2str(enum VkColorSpaceKHR cs)
{
	switch (cs)
	{
	case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
		return "VK_COLOR_SPACE_SRGB_NONLINEAR_KHR";
	case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT:
		return "VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT";
	case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:
		return "VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT";
	case VK_COLOR_SPACE_DCI_P3_LINEAR_EXT:
		return "VK_COLOR_SPACE_DCI_P3_LINEAR_EXT";
	case VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT:
		return "VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT";
	case VK_COLOR_SPACE_BT709_LINEAR_EXT:
		return "VK_COLOR_SPACE_BT709_LINEAR_EXT";
	case VK_COLOR_SPACE_BT709_NONLINEAR_EXT:
		return "VK_COLOR_SPACE_BT709_NONLINEAR_EXT";
	case VK_COLOR_SPACE_BT2020_LINEAR_EXT:
		return "VK_COLOR_SPACE_BT2020_LINEAR_EXT";
	case VK_COLOR_SPACE_HDR10_ST2084_EXT:
		return "VK_COLOR_SPACE_HDR10_ST2084_EXT";
	case VK_COLOR_SPACE_DOLBYVISION_EXT:
		return "VK_COLOR_SPACE_DOLBYVISION_EXT";
	case VK_COLOR_SPACE_HDR10_HLG_EXT:
		return "VK_COLOR_SPACE_HDR10_HLG_EXT";
	case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT:
		return "VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT";
	case VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT:
		return "VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT";
	case VK_COLOR_SPACE_PASS_THROUGH_EXT:
		return "VK_COLOR_SPACE_PASS_THROUGH_EXT";
	case VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT:
		return "VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT";
	default:
		return "Not_Known";
	}
}

// SNORM: Signed normalized integer;
// UNORM: Unsigned normalized integer;

// there are too many format ...
// This is only my rx560 on win10 supported format ...

const char * VkFormatEnum2str(VkFormat fmt)
{
	switch(fmt)
	{
		case VK_FORMAT_UNDEFINED:
			return "VK_FORMAT_UNDEFINED";
		case VK_FORMAT_B8G8R8A8_UNORM:
			return "VK_FORMAT_B8G8R8A8_UNORM";
		case VK_FORMAT_B8G8R8A8_SNORM:
			return "VK_FORMAT_B8G8R8A8_SNORM";
		case VK_FORMAT_B8G8R8A8_SRGB:
			return "VK_FORMAT_B8G8R8A8_SRGB";
		case VK_FORMAT_R16G16B16A16_UNORM:
			return "VK_FORMAT_R16G16B16A16_UNORM";
		case VK_FORMAT_R16G16B16A16_SNORM:
			return "VK_FORMAT_R16G16B16A16_SNORM";
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			return "VK_FORMAT_R16G16B16A16_SFLOAT";
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
			return "VK_FORMAT_A2R10G10B10_UNORM_PACK32";
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
			return "VK_FORMAT_A2B10G10R10_UNORM_PACK32";
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
			return "VK_FORMAT_B10G11R11_UFLOAT_PACK32";
		case VK_FORMAT_R8G8B8A8_UNORM:
			return "VK_FORMAT_R8G8B8A8_UNORM";
		case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
			return "VK_FORMAT_A8B8G8R8_UNORM_PACK32";
		case VK_FORMAT_R8G8B8A8_SNORM:
			return "VK_FORMAT_R8G8B8A8_SNORM";
		case VK_FORMAT_R8G8B8A8_SRGB:
			return "VK_FORMAT_R8G8B8A8_SRGB";
		case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
			return "VK_FORMAT_A8B8G8R8_SNORM_PACK32";
		case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
			return "VK_FORMAT_A8B8G8R8_SRGB_PACK32";
		case VK_FORMAT_R5G6B5_UNORM_PACK16:
			return "VK_FORMAT_R5G6B5_UNORM_PACK16";
		case VK_FORMAT_B5G6R5_UNORM_PACK16:
			return "VK_FORMAT_B5G6R5_UNORM_PACK16";
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
			return "VK_FORMAT_A1R5G5B5_UNORM_PACK16";

		default :
			return "SEE vulkan_core.h";

	}

}

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
    
    ri.Printf(PRINT_ALL, "Render area: %dx%d\n", vk_getWinWidth(), vk_getWinHeight());
    ri.Printf(PRINT_ALL, "Window dimension: %dx%d\n", ri.GetWinWidth(), ri.GetWinHeight());


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


void printSurfaceFormatSupported_f(void)
{
	uint32_t nSurfmt;


	// Get the numbers of VkFormat's that are supported
	// "vk.surface" is the surface that will be associated with the swapchain.
	// "vk.surface" must be a valid VkSurfaceKHR handle
	qvkGetPhysicalDeviceSurfaceFormatsKHR(vk.physical_device, vk.surface, &nSurfmt, NULL);
	assert(nSurfmt > 0);

	VkSurfaceFormatKHR * pSurfFmts =
		(VkSurfaceFormatKHR *) malloc( nSurfmt * sizeof(VkSurfaceFormatKHR) );

	// To query the supported swapchain format-color space pairs for a surface
	qvkGetPhysicalDeviceSurfaceFormatsKHR(vk.physical_device, vk.surface, &nSurfmt, pSurfFmts);

	ri.Printf(PRINT_ALL, " -------- Total %d surface formats supported. -------- \n", nSurfmt);

	for (uint32_t i = 0; i < nSurfmt; ++i)
	{
		ri.Printf(PRINT_ALL, " [%d] format: %s, color space: %s \n", i, 
			VkFormatEnum2str( pSurfFmts[i].format ),
			ColorSpaceEnum2str( pSurfFmts[i].colorSpace ));
	}

	ri.Printf(PRINT_ALL, " --- ----------------------------------- --- \n");

	free(pSurfFmts);
}
