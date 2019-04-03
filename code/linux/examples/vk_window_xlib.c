#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>

#include "vk_window_xlib.h"

#if defined(VK_USE_PLATFORM_XLIB_KHR)

void demo_create_xlib_window(struct demo *demo)
{
    const char *display_envar = getenv("DISPLAY");
    if (display_envar == NULL || display_envar[0] == '\0') {
        printf("Environment variable DISPLAY requires a valid value.\nExiting ...\n");
        fflush(stdout);
        exit(1);
    }

    XInitThreads();
    demo->display = XOpenDisplay(NULL);
    long visualMask = VisualScreenMask;
    int numberOfVisuals;
    XVisualInfo vInfoTemplate = {};
    vInfoTemplate.screen = DefaultScreen(demo->display);
    XVisualInfo *visualInfo = XGetVisualInfo(demo->display, visualMask, &vInfoTemplate, &numberOfVisuals);

    Colormap colormap =
        XCreateColormap(demo->display, RootWindow(demo->display, vInfoTemplate.screen), visualInfo->visual, AllocNone);

    XSetWindowAttributes windowAttributes = {};
    windowAttributes.colormap = colormap;
    windowAttributes.background_pixel = 0xFFFFFFFF;
    windowAttributes.border_pixel = 0;
    windowAttributes.event_mask = KeyPressMask | KeyReleaseMask | StructureNotifyMask | ExposureMask;

    demo->xlib_window = XCreateWindow(demo->display, RootWindow(demo->display, vInfoTemplate.screen), 0, 0, demo->width,
                                      demo->height, 0, visualInfo->depth, InputOutput, visualInfo->visual,
                                      CWBackPixel | CWBorderPixel | CWEventMask | CWColormap, &windowAttributes);

    XSelectInput(demo->display, demo->xlib_window, ExposureMask | KeyPressMask);
    XMapWindow(demo->display, demo->xlib_window);
    XFlush(demo->display);
    demo->xlib_wm_delete_window = XInternAtom(demo->display, "WM_DELETE_WINDOW", False);
}


void demo_handle_xlib_event(struct demo *demo, const XEvent *event)
{
    switch (event->type)
    {
        case ClientMessage:
            if ((Atom)event->xclient.data.l[0] == demo->xlib_wm_delete_window) demo->quit = true;
            break;
        case KeyPress:
            switch (event->xkey.keycode) {
                case 0x9:  // Escape
                    demo->quit = true;
                    break;
                case 0x71:  // left arrow key
                    demo->spin_angle -= demo->spin_increment;
                    break;
                case 0x72:  // right arrow key
                    demo->spin_angle += demo->spin_increment;
                    break;
                case 0x41:  // space bar
                    demo->pause = !demo->pause;
                    break;
            }
            break;
        case ConfigureNotify:
            if ((demo->width != event->xconfigure.width) || (demo->height != event->xconfigure.height)) {
                demo->width = event->xconfigure.width;
                demo->height = event->xconfigure.height;
                demo_resize(demo);
            }
            break;
        default:
            break;
    }
}


void demo_run_xlib(struct demo *demo)
{
    while (!demo->quit)
    {
        XEvent event;

        if (demo->pause) {
            XNextEvent(demo->display, &event);
            demo_handle_xlib_event(demo, &event);
        }
        while (XPending(demo->display) > 0) {
            XNextEvent(demo->display, &event);
            demo_handle_xlib_event(demo, &event);
        }

        demo_draw(demo);
        demo->curFrame++;
        if (demo->frameCount != INT32_MAX && demo->curFrame == demo->frameCount) demo->quit = true;
    }
}

#endif

