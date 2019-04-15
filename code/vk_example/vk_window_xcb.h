#ifndef VK_WINDOW_XCB_H_
#define VK_WINDOW_XCB_H_


#include "vk_common.h"
#include "demo.h"

void xcb_createWindow(struct demo *demo);
void xcb_initConnection(struct demo *demo);

#endif

