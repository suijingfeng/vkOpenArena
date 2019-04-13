#ifndef VK_COMMON_H_
#define VK_COMMON_H_

#include <stdio.h>
#include "VKimpl.h"

const char * cvtResToStr(VkResult result);


#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))


#define ERR_EXIT(err_msg, err_class)        \
    do {                                    \
        fprintf(stderr, "%s\n", err_msg);   \
        fflush(stderr);                     \
        exit(1);                            \
    } while (0)



#ifndef NDEBUG

#define VK_CHECK(function_call) { \
	VkResult result = function_call; \
	if (result != VK_SUCCESS) \
		fprintf(stderr, "Vulkan: error %s returned by %s \n", cvtResToStr(result), #function_call); \
}

#else

#define VK_CHECK(function_call)     function_call;

#endif

#endif
