#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>


#include "demo.h"
#include "vk_common.h"
#include "vk_debug.h"
#include "vk_swapchain.h"

#define APP_SHORT_NAME "cube"


void vk_selectPhysicalDevice(struct demo * const pDemo)
{
    uint32_t gpu_count;

    /* Make initial call to query gpu_count, then second call for gpu info*/
    VK_CHECK( vkEnumeratePhysicalDevices(pDemo->inst, &gpu_count, NULL) );

    if (gpu_count > 0)
    {
        VkPhysicalDevice * physical_devices = (VkPhysicalDevice *) malloc(sizeof(VkPhysicalDevice) * gpu_count);
        VK_CHECK( vkEnumeratePhysicalDevices(pDemo->inst, &gpu_count, physical_devices) );
        /* For cube demo we just grab the first physical device */
        pDemo->gpu = physical_devices[0];
        free(physical_devices);
    }
    else
    {
        ERR_EXIT("vkEnumeratePhysicalDevices reported zero accessible devices.\n\n"
            "Do you have a compatible Vulkan installable client driver (ICD) installed?\n", 0);
    }
    
    printf(" Select physical device, we just grab the first physical device for this demo app. \n");
}



void vk_checkSurfaceExtension(struct demo * const pDemo)
{
    uint32_t instance_extension_count = 0;
    /* Look for instance extensions */
    VkBool32 surfaceExtFound = 0;
    memset(pDemo->extension_names, 0, sizeof(pDemo->extension_names));

    VK_CHECK( vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL) );

    if (instance_extension_count > 0)
    {
        VkExtensionProperties* instance_extensions = 
            (VkExtensionProperties* ) malloc(sizeof(VkExtensionProperties) * instance_extension_count);
        VK_CHECK( vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, instance_extensions) );

        for (uint32_t i = 0; i < instance_extension_count; i++)
        {
            if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName))
            {
                surfaceExtFound = 1;
                pDemo->extension_names[pDemo->enabled_extension_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
            }

            if (!strcmp(VK_KHR_XCB_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName)) {
                pDemo->extension_names[pDemo->enabled_extension_count++] = VK_KHR_XCB_SURFACE_EXTENSION_NAME;
            }

            if (!strcmp(VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME, instance_extensions[i].extensionName)) {
                pDemo->extension_names[pDemo->enabled_extension_count++] = VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME;
            }

            if (!strcmp(VK_EXT_DISPLAY_SURFACE_COUNTER_EXTENSION_NAME, instance_extensions[i].extensionName)) {
                pDemo->extension_names[pDemo->enabled_extension_count++] = VK_EXT_DISPLAY_SURFACE_COUNTER_EXTENSION_NAME;
            }

#ifndef NDEBUG
            if (!strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, instance_extensions[i].extensionName))
            {
                if (pDemo->validate) {
                    pDemo->extension_names[pDemo->enabled_extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
                }
            }
#endif
        }

        printf("-------- Total %d instance extensions supported --------\n", instance_extension_count);
        for (uint32_t i = 0; i < instance_extension_count; i++)
        {
            printf(" %s \n", instance_extensions[i].extensionName);
        }

        printf("-------- Enabled instance extensions on this app --------\n");
        for (uint32_t i = 0; i < pDemo->enabled_extension_count; ++i)
        {
            printf(" %s \n", pDemo->extension_names[i]);
        }
        printf("-------- --------------------------------------- --------\n");
        free(instance_extensions);
    }

    if (!surfaceExtFound) {
        ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_SURFACE_EXTENSION_NAME
                 " extension.\n\n"
                 "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
                 "Please look at the Getting Started guide for additional information.\n",
                 "vkCreateInstance Failure");
    }
}


void vk_createInstance(struct demo * pDemo)
{

    const VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = APP_SHORT_NAME,
        .applicationVersion = 0,
        .pEngineName = APP_SHORT_NAME,
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_0,
    };

    uint32_t instance_layer_count = 0;
    const char * pInstanceValidLayer = NULL;
    
    if (pDemo->validate)
    {
        pInstanceValidLayer = vk_assertStandValidationLayer();
        if(NULL != pInstanceValidLayer )
            instance_layer_count = 1;
    }
    
    // We only enable VK_LAYER_LUNARG_standard_validation,
    // so don't be so complicated
    VkInstanceCreateInfo inst_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = (pDemo->validate ? vk_setDebugUtilsMsgInfo(pDemo) : NULL),
        .pApplicationInfo = &app,
        .enabledLayerCount = (pDemo->validate ? instance_layer_count : 0),
        .ppEnabledLayerNames = &pInstanceValidLayer,
        .enabledExtensionCount = pDemo->enabled_extension_count,
        .ppEnabledExtensionNames = (const char *const *)pDemo->extension_names,
    };

    
    VkResult err = vkCreateInstance(&inst_info, NULL, &pDemo->inst);
    if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
        ERR_EXIT(
            "Cannot find a compatible Vulkan installable client driver (ICD).\n\n"
            "Please look at the Getting Started guide for additional information.\n",
            "vkCreateInstance Failure");
    } else if (err == VK_ERROR_EXTENSION_NOT_PRESENT) {
        ERR_EXIT(
            "Cannot find a specified extension library.\n"
            "Make sure your layers path is set appropriately.\n",
            "vkCreateInstance Failure");
    } else if (err) {
        ERR_EXIT(
            "vkCreateInstance failed.\n\n"
            "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
            "Please look at the Getting Started guide for additional information.\n",
            "vkCreateInstance Failure");
    }
    
    printf(" Vulkan Instance Created. !^_^! \n\n");
}


void vk_getInstanceProcAddrKHR(struct demo * pDemo)
{
#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                                                            \
{                                                                                                           \
    pFn_vkhr.fp##entrypoint = (PFN_vk##entrypoint)vkGetInstanceProcAddr(inst, "vk" #entrypoint);            \
    if (pFn_vkhr.fp##entrypoint == NULL) {                                                                  \
        ERR_EXIT("vkGetInstanceProcAddr failed to find vk" #entrypoint, "vkGetInstanceProcAddr Failure");   \
    }                                                                                                       \
}

    GET_INSTANCE_PROC_ADDR(pDemo->inst, GetPhysicalDeviceSurfaceSupportKHR);
    GET_INSTANCE_PROC_ADDR(pDemo->inst, GetPhysicalDeviceSurfaceCapabilitiesKHR);
    GET_INSTANCE_PROC_ADDR(pDemo->inst, GetPhysicalDeviceSurfaceFormatsKHR);
    GET_INSTANCE_PROC_ADDR(pDemo->inst, GetPhysicalDeviceSurfacePresentModesKHR);
//    GET_INSTANCE_PROC_ADDR(pDemo->inst, GetSwapchainImagesKHR);

#undef GET_INSTANCE_PROC_ADDR
}


void vk_checkSwapchainExt(struct demo * pDemo)
{
    /* Look for device extensions */
    uint32_t device_extension_count = 0;
    VkBool32 swapchainExtFound = 0;
    
    pDemo->enabled_extension_count = 0;
    
    memset(pDemo->extension_names, 0, sizeof(pDemo->extension_names));

    VK_CHECK( vkEnumerateDeviceExtensionProperties(pDemo->gpu, NULL, &device_extension_count, NULL) );

    assert (device_extension_count > 0);
    
    {
        VkExtensionProperties * device_extensions = 
            (VkExtensionProperties *) malloc( sizeof(VkExtensionProperties) * device_extension_count );
        VK_CHECK( vkEnumerateDeviceExtensionProperties(pDemo->gpu, NULL, &device_extension_count, device_extensions) );

        for (uint32_t i = 0; i < device_extension_count; i++)
        {
            if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, device_extensions[i].extensionName))
            {
                swapchainExtFound = 1;
                pDemo->extension_names[pDemo->enabled_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
                break;
            }
        }

        printf("-------- Total %d device extensions supported --------\n", device_extension_count);
        for (uint32_t i = 0; i < device_extension_count; i++)
        {
            printf(" %s \n", device_extensions[i].extensionName);
        }

        printf("-------- Enabled device extensions on this app --------\n");
        for (uint32_t i = 0; i < pDemo->enabled_extension_count; ++i)
        {
            printf(" %s \n", pDemo->extension_names[i]);
        }
        printf("-------- --------------------------------------- --------\n");


        free(device_extensions);
    }

    if (!swapchainExtFound) {
        ERR_EXIT("vkEnumerateDeviceExtensionProperties failed to find the " VK_KHR_SWAPCHAIN_EXTENSION_NAME
                 " extension.\n\nDo you have a compatible Vulkan installable client driver (ICD) installed?\n"
                 "Please look at the Getting Started guide for additional information.\n",
                 "vkCreateInstance Failure");
    }
}


void vk_init(struct demo *demo)
{
    printf("---- Initial Vulkan. ----\n");

    demo->enabled_extension_count = 0;

    vk_checkSurfaceExtension(demo);

    vk_createInstance(demo);

    vk_selectPhysicalDevice(demo);
    
    vk_checkSwapchainExt(demo);

    if (demo->validate)
    {
        vk_createDebugUtils(demo);
    }
    
    vkGetPhysicalDeviceProperties(demo->gpu, &demo->gpu_props);

    /* Call with NULL data to get count */
    vkGetPhysicalDeviceQueueFamilyProperties(demo->gpu, &demo->queue_family_count, NULL);
    assert(demo->queue_family_count >= 1);

    demo->queue_props = (VkQueueFamilyProperties *) 
        malloc( demo->queue_family_count * sizeof(VkQueueFamilyProperties) );
    vkGetPhysicalDeviceQueueFamilyProperties(demo->gpu, &demo->queue_family_count, demo->queue_props);

    // Query fine-grained feature support for this device.
    //  If app has specific feature requirements it should check supported
    //  features based on this query
    VkPhysicalDeviceFeatures physDevFeatures;
    vkGetPhysicalDeviceFeatures(demo->gpu, &physDevFeatures);

    vk_getInstanceProcAddrKHR(demo);
}
