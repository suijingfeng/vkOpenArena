#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <signal.h>


#if defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_XCB_KHR)
#include <X11/Xutil.h>
#endif

#if defined(VK_USE_PLATFORM_XLIB_KHR)


extern struct demo;

void demo_create_xlib_window(struct demo *demo);
void demo_handle_xlib_event(struct demo *demo, const XEvent *event);
void demo_run_xlib(struct demo *demo);

#endif

