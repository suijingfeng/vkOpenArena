#include <stdlib.h>
#include <stdio.h>

#include <xcb/xcb.h>

#include "vk_common.h"
#include "vulkanlib.h"
#include "vk_instance.h"
#include "qvk.h"

// To create a VkSurfaceKHR object for an X11 window,
// using the XCB client-side library
static PFN_vkCreateXcbSurfaceKHR qvkCreateXcbSurfaceKHR;

xcb_connection_t * connection_xcb;
xcb_drawable_t window_xcb;
xcb_screen_t * screen;

void GetDesktopResolution_xcb(int * width, int * height)
{
    *width = screen->width_in_pixels;
    *height = screen->height_in_pixels;
}


void vk_createSurfaceImpl(void)
{
    qvkCreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR)qvkGetInstanceProcAddr(vk.instance, "vkCreateXcbSurfaceKHR");
    if( qvkCreateXcbSurfaceKHR == NULL)
    {
        printf("Failed to find entrypoint qvkCreateXcbSurfaceKHR\n"); 
    }
   
    VkXcbSurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.connection = connection_xcb;
    createInfo.window = window_xcb;

    qvkCreateXcbSurfaceKHR(vk.instance, &createInfo, NULL, &vk.surface);

    printf("qvkCreateXcbSurfaceKHR\n");
}


int main ()
{
  xcb_gcontext_t       foreground;
  xcb_generic_event_t *e;
  uint32_t             mask = 0;
  uint32_t             values[2];

  /* geometric objects */
  xcb_point_t          points[] = {
    {10, 10},
    {10, 20},
    {20, 10},
    {20, 20}};

  xcb_point_t          polyline[] = {
    {50, 10},
    { 5, 20},     /* rest of points are relative */
    {25,-20},
    {10, 10}};

  xcb_segment_t        segments[] = {
    {100, 10, 140, 30},
    {110, 25, 130, 60}};

  xcb_rectangle_t      rectangles[] = {
    { 10, 50, 40, 20},
    { 80, 50, 10, 40}};

  xcb_arc_t            arcs[] = {
    {10, 100, 60, 40, 0, 90 << 6},
    {90, 100, 55, 40, 0, 270 << 6}};

  /* Open the connection to the X server */
  connection_xcb = xcb_connect (NULL, NULL);

  /* Get the first screen */
  screen = xcb_setup_roots_iterator (xcb_get_setup (connection_xcb)).data;

  /* Create black (foreground) graphic context */
  window_xcb = screen->root;

  foreground = xcb_generate_id (connection_xcb);
  mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
  values[0] = screen->black_pixel;
  values[1] = 0;
  xcb_create_gc (connection_xcb, foreground, window_xcb, mask, values);

  /* Ask for our window's Id */
  window_xcb = xcb_generate_id(connection_xcb);

  /* Create the window */
  mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  values[0] = screen->white_pixel;
  values[1] = XCB_EVENT_MASK_EXPOSURE;
  xcb_create_window (connection_xcb,                /* Connection          */
                     XCB_COPY_FROM_PARENT,          /* depth               */
                     window_xcb,                    /* window Id           */
                     screen->root,                  /* parent window       */
                     0, 0,                          /* x, y                */
                     150, 150,                      /* width, height       */
                     10,                            /* border_width        */
                     XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class               */
                     screen->root_visual,           /* visual              */
                     mask, values);                 /* masks */

  /* Map the window on the screen */
  xcb_map_window (connection_xcb, window_xcb);


  vk_loadLib();
  vk_createInstanceAndDevice();
  
  /* We flush the request */
  xcb_flush (connection_xcb);

    while ((e = xcb_wait_for_event (connection_xcb)))
    {
        switch (e->response_type & ~0x80)
        {
            case XCB_EXPOSE:
            {
                /* We draw the points */
                xcb_poly_point (connection_xcb, XCB_COORD_MODE_ORIGIN, window_xcb, foreground, 4, points);

                /* We draw the polygonal line */
                xcb_poly_line (connection_xcb, XCB_COORD_MODE_PREVIOUS, window_xcb, foreground, 4, polyline);

                /* We draw the segements */
                xcb_poly_segment (connection_xcb, window_xcb, foreground, 2, segments);

                /* We draw the rectangles */
                xcb_poly_rectangle (connection_xcb, window_xcb, foreground, 2, rectangles);

                /* We draw the arcs */
                xcb_poly_arc (connection_xcb, window_xcb, foreground, 2, arcs);

                /* We flush the request */
                xcb_flush (connection_xcb);

                break;
            }
            default:
            {
                /* Unknown event type, ignore it */
                break;
            }
        }
        /* Free the Generic Event */
        free (e);
    }
  
    vk_destroyInstanceAndDevice();
    vk_unLoadLib();
    qvkCreateXcbSurfaceKHR = NULL;
    return 0;
}
