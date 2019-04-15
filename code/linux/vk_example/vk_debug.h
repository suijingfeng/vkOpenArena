#ifndef VK_DEBUG_H_
#define VK_DEBUG_H_

#include "vk_common.h"
#include "demo.h"

struct PFN_EXT_DebugUtils_t
{
    PFN_vkCreateDebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessengerEXT;
    PFN_vkSubmitDebugUtilsMessageEXT SubmitDebugUtilsMessageEXT;
    PFN_vkCmdBeginDebugUtilsLabelEXT CmdBeginDebugUtilsLabelEXT;
    PFN_vkCmdEndDebugUtilsLabelEXT CmdEndDebugUtilsLabelEXT;
    PFN_vkCmdInsertDebugUtilsLabelEXT CmdInsertDebugUtilsLabelEXT;
    PFN_vkSetDebugUtilsObjectNameEXT SetDebugUtilsObjectNameEXT;
};


extern struct PFN_EXT_DebugUtils_t pFn_vkd;

void vk_createDebugUtils(struct demo * pDemo);

void vk_getDebugUtilsFnPtr(struct demo * pDemo);

void vk_destroyDebugUtils(struct demo * pDemo);

VkDebugUtilsMessengerCreateInfoEXT * vk_setDebugUtilsMsgInfo(struct demo * pDemo);

const char * vk_assertStandValidationLayer(void);


#endif
