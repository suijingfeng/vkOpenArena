#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "vk_instance.h"
#include "ref_import.h" 


#ifndef NDEBUG

static VkDebugReportCallbackEXT h_debugCB;

static FILE* fpLogProc;

static cvar_t* r_logCalledProc;


VKAPI_ATTR VkBool32 VKAPI_CALL vk_DebugCallback(
        VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT object_type,
        uint64_t object, size_t location, int32_t message_code, 
        const char* layer_prefix, const char* message, void* user_data )
{

    switch(flags)
    {
        case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
            ri.Printf(PRINT_ALL, "INFORMATION: %s %s\n", layer_prefix, message);
            break;
        case VK_DEBUG_REPORT_WARNING_BIT_EXT:
            ri.Printf(PRINT_WARNING, "WARNING: %s %s\n", layer_prefix, message);
            break;
        case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
            ri.Printf(PRINT_WARNING, "PERFORMANCE:%s %s\n",layer_prefix, message);
            break;
        case VK_DEBUG_REPORT_ERROR_BIT_EXT:
            ri.Printf(PRINT_WARNING, "ERROR: %s %s\n", layer_prefix, message);
            break;
        case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
            ri.Printf(PRINT_WARNING, "DEBUG: %s %s\n", layer_prefix, message);
            break;
    }
    
	return VK_FALSE;

}
#endif


void vk_createDebugCallback( )
{
#ifndef NDEBUG
    r_logCalledProc = ri.Cvar_Get( "r_logCalledProc", "0", CVAR_TEMP );
    
    // PFN_vkDebugReportCallbackEXT qvkDebugCB = vk_DebugCallback;
    ri.Printf( PRINT_ALL, " vk_createDebugCallback() \n" ); 
    
    VkDebugReportCallbackCreateInfoEXT desc;
    desc.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    desc.pNext = NULL;
    desc.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_ERROR_BIT_EXT;
    desc.pfnCallback = vk_DebugCallback;
    desc.pUserData = NULL;

    VK_CHECK(qvkCreateDebugReportCallbackEXT(vk.instance, &desc, NULL, &h_debugCB));


    // init
    // r_logCalledProc->integer = needLog;

    fpLogProc = fopen( "CalledVulkanApiLog.txt", "wt" );

#endif
}


void vk_destroyDebugReportHandle(void)
{
#ifndef NDEBUG
    ri.Printf( PRINT_ALL, " Destroy callback function handle. \n" );
	qvkDestroyDebugReportCallbackEXT(vk.instance, h_debugCB, NULL);
    h_debugCB = 0;

    if(fpLogProc != NULL)
    {
        fclose( fpLogProc );
	    fpLogProc = NULL;
    }
#endif
}


static const char * cvtResToStr(VkResult result)
{
    switch(result)
    {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_OUT_OF_POOL_MEMORY_KHR:
            return "VK_ERROR_OUT_OF_POOL_MEMORY_KHR";
        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";
//
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_NOT_PERMITTED_EXT:
            return "VK_ERROR_NOT_PERMITTED_EXT";
//
        case VK_RESULT_MAX_ENUM:
            return "VK_RESULT_MAX_ENUM";
        case VK_RESULT_RANGE_SIZE:
            return "VK_RESULT_RANGE_SIZE"; 
        case VK_ERROR_FRAGMENTATION_EXT:
            return "VK_ERROR_FRAGMENTATION_EXT";
    }

    return "UNKNOWN_ERROR";
}



void debug_vkapi_call(VkResult result, const char * const proc_str)
{
#ifndef NDEBUG
    if ( (fpLogProc!= NULL) && (r_logCalledProc->integer == 1) )
	{
		fprintf( fpLogProc, "%s\n", proc_str );
	}

    if (result != VK_SUCCESS) 
    {
        ri.Printf(PRINT_ALL, "Vulkan: error %s returned by %s \n", cvtResToStr(result), proc_str);
    }
#endif
}
