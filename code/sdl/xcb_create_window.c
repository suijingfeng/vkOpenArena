#ifndef __linux__
#error You should not be including this file on non-Linux platforms
#endif

#include <xcb/xcb.h>

#include <dlfcn.h>

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "linux_public.h"

#include "xcb_input.h"

struct xcb_windata_s s_xcb_win;

static cvar_t* r_mode;				// video mode
static cvar_t* r_customwidth;
static cvar_t* r_customheight;
static cvar_t* r_customaspect;

static cvar_t* r_fullscreen;


typedef struct vidmode_s
{
    const char *description;
    int         width, height;
	float		pixelAspect;		// pixel width / height
} vidmode_t;


static const vidmode_t r_vidModes[] =
{
	{ "Mode  0: 320x240",		320,	240,	1 },
	{ "Mode  1: 400x300",		400,	300,	1 },
	{ "Mode  2: 512x384",		512,	384,	1 },
	{ "Mode  3: 640x480 (480p)",	640,	480,	1 },
	{ "Mode  4: 800x600",		800,	600,	1 },
	{ "Mode  5: 960x720",		960,	720,	1 },
	{ "Mode  6: 1024x768",		1024,	768,	1 },
	{ "Mode  7: 1152x864",		1152,	864,	1 },
	{ "Mode  8: 1280x1024",		1280,	1024,	1 },
	{ "Mode  9: 1600x1200",		1600,	1200,	1 },
	{ "Mode 10: 2048x1536",		2048,	1536,	1 },
	{ "Mode 11: 856x480",		856,	480,	1 }, // Q3 MODES END HERE AND EXTENDED MODES BEGIN
	{ "Mode 12: 1280x720 (720p)",	1280,	720,	1 },
	{ "Mode 13: 1280x768",		1280,	768,	1 },
	{ "Mode 14: 1280x800",		1280,	800,	1 },
	{ "Mode 15: 1280x960",		1280,	960,	1 },
	{ "Mode 16: 1360x768",		1360,	768,	1 },
	{ "Mode 17: 1366x768",		1366,	768,	1 }, // yes there are some out there on that extra 6
	{ "Mode 18: 1360x1024",		1360,	1024,	1 },
	{ "Mode 19: 1400x1050",		1400,	1050,	1 },
	{ "Mode 20: 1400x900",		1400,	900,	1 },
	{ "Mode 21: 1600x900",		1600,	900,	1 },
	{ "Mode 22: 1680x1050",		1680,	1050,	1 },
	{ "Mode 23: 1920x1080 (1080p)",	1920,	1080,	1 },
	{ "Mode 24: 1920x1200",		1920,	1200,	1 },
	{ "Mode 25: 1920x1440",		1920,	1440,	1 },
    { "Mode 26: 2560x1080",		2560,	1080,	1 },
    { "Mode 27: 2560x1600",		2560,	1600,	1 },
	{ "Mode 28: 3840x2160 (4K)",	3840,	2160,	1 }
};

static const int s_numVidModes = 29;


static void R_DisplayResolutionList_f( void )
{
	Com_Printf( "\n" );
	for (uint32_t i = 0; i < s_numVidModes; i++ )
	{
		Com_Printf( "%s\n", r_vidModes[i].description );
	}
	Com_Printf("\n" );
}



xcb_intern_atom_reply_t *atom_wm_delete_window;


void WinSys_Init(void ** pContext)
{
	Com_Printf( " Initializing window subsystem. \n" );
   
    // despect its name prefixed with r_
    // but they have nothing to do with the renderer. 
    r_mode = Cvar_Get( "r_mode", "3", CVAR_ARCHIVE | CVAR_LATCH );
    r_fullscreen = Cvar_Get( "r_fullscreen", "0", CVAR_ARCHIVE | CVAR_LATCH );
    r_customwidth = Cvar_Get( "r_customwidth", "960", CVAR_ARCHIVE | CVAR_LATCH );
    r_customheight = Cvar_Get( "r_customheight", "540", CVAR_ARCHIVE | CVAR_LATCH );
    r_customaspect = Cvar_Get( "r_customaspect", "1.78", CVAR_ARCHIVE | CVAR_LATCH );
    
    // Cmd_AddCommand( "displayResoList", R_DisplayResolutionList_f );

    //
	*pContext = &s_xcb_win;


	Com_Printf( " Setting up XCB connection... \n" );

    const char * const pDisplay_envar = getenv("DISPLAY");
    
    if (pDisplay_envar == NULL || pDisplay_envar[0] == '\0')
    {
        Com_Printf(" Environment variable DISPLAY requires a valid value.");
    }
    else
    {
        Com_Printf(" $(DISPLAY) = %s \n", pDisplay_envar);
    }

    // An X program first needs to open the connection to the X server
    // if displayname = NULL, uses the DISPLAY environment variable
    // returns the screen number of the connection, 
    // can provide NULL if you don't care.

    s_xcb_win.connection = xcb_connect(NULL, &s_xcb_win.screenIdx);
    
    if (xcb_connection_has_error(s_xcb_win.connection) > 0)
    {
        Com_Error(ERR_FATAL, "Cannot set up connection using XCB... ");
    }

    // ==============================================================================
    // Once we have opened a connection to an X server, we should check some basic
    // information about it: what screens it has, what is the size (width and height)
    // of the screen, how many colors it supports (black and white ? 256 colors ? ). 
    // We get such information from the xcbscreent structure:
    // =====================================================================

    // Get the screen whose number is screenNum
    const xcb_setup_t * const pXcbSetup = xcb_get_setup(s_xcb_win.connection);
    
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(pXcbSetup);
    
    // we want the screen at index screenNum of the iterator
    for (int i = 0; i < s_xcb_win.screenIdx; ++i) 
    {
        xcb_screen_next (&iter);
    }

    xcb_screen_t * pScreen = iter.data;
    
    s_xcb_win.desktopWidth = pScreen->width_in_pixels;
    s_xcb_win.desktopHeight = pScreen->height_in_pixels;
    s_xcb_win.root = pScreen->root;
    // report 

    Com_Printf("\nInformations of screen: %d\n", pScreen->root);
    Com_Printf("  width.................: %d\n", pScreen->width_in_pixels);
    Com_Printf("  height................: %d\n", pScreen->height_in_pixels);
    Com_Printf("  white pixel...........: %d\n", pScreen->white_pixel);
    Com_Printf("  black pixel...........: %d\n", pScreen->black_pixel);
    Com_Printf("  allowed depths len....: %d\n", pScreen->allowed_depths_len);
    Com_Printf("  root depth............: %d\n", pScreen->root_depth);
    Com_Printf("\n");


    // =======================================================================
    // =======================================================================

    // After we got some basic information about our screen, 
    // we can create our first window. In the X Window System,
    // a window is characterized by an Id. So, in XCB, a window
    // is of type of uint32_t.
    // 
    // We first ask for a new Id for our window
    s_xcb_win.hWnd = xcb_generate_id(s_xcb_win.connection);

    /*
    Then, XCB supplies the following function to create new windows:

    xcb_void_cookie_t xcb_create_window ( xcb_connection_t *connection, 
    uint8_t depth, xcb_window_t wid, xcb_window_t parent,
    int16_t x, int16_t y, uint16_t width, uint16_t height,
    uint16_t border_width, uint16_t _class, xcb_visualid_t visual,
    uint32_t value_mask, const uint32_t* value_list );
    */
    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t width = s_xcb_win.desktopWidth;
	uint16_t height = s_xcb_win.desktopHeight;
    
    // -------------------------------------------------
    // developing now, its prefix started with r_
    // but it have nothing to do with the renderer ...
    r_fullscreen->integer = 0;
    r_mode->integer = 3;
    // -------------------------------------------------
    if(r_fullscreen->integer)
    {
        // if fullscreen is true, then we use desktop video resolution
        r_mode->integer = -2;
        Com_Printf( " Setting fullscreen mode. \n" );
    }
    else
    {
       	// r_mode->integer = R_GetModeInfo(&width, &height,
        //        r_mode->integer, s_xcb_win.desktopWidth, s_xcb_win.desktopHeight);
        width = 1280;
	    height = 720;
    }
    
    // R_SetWinMode(r_mode->integer, 640, 480, 60);

    // During the creation of a window, you should give it what kind of events it wishes to receive. 
    uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    // The values that a mask could take are given by the xcb_cw_t enumeration:
    uint32_t value_list[2];

    value_list[0] = pScreen->white_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_PRESS | 
                    XCB_EVENT_MASK_KEY_RELEASE | 
                    XCB_EVENT_MASK_BUTTON_PRESS |
                    XCB_EVENT_MASK_BUTTON_RELEASE | 
                    XCB_EVENT_MASK_ENTER_WINDOW | 
                    XCB_EVENT_MASK_LEAVE_WINDOW |
                    XCB_EVENT_MASK_POINTER_MOTION | 
                    XCB_EVENT_MASK_KEYMAP_STATE | 
                    XCB_EVENT_MASK_FOCUS_CHANGE |
                    XCB_EVENT_MASK_EXPOSURE |
                    XCB_EVENT_MASK_VISIBILITY_CHANGE |
                    XCB_EVENT_MASK_PROPERTY_CHANGE | 
                    XCB_EVENT_MASK_STRUCTURE_NOTIFY;
    //
    // XCB_COPY_FROM_PARENT: depth (same as root) the depth is taken from the parent window.
    // screen->root        : parent window
    // s_xcb_win.hWnd: The ID with which you will refer to the new window, created by xcb_generate_id. 

    // screen->root: The parent window of the new window.
    // 10: Width of the window's border (in pixels) 
    // XCB_WINDOW_CLASS_INPUT_OUTPUT: without documention ???
    // 
    xcb_create_window( s_xcb_win.connection, 
            XCB_COPY_FROM_PARENT, 
            s_xcb_win.hWnd, 
            pScreen->root,
            x, y, width, height, 10, 
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            pScreen->root_visual,
            value_mask, value_list);
    
    s_xcb_win.winWidth = width;
    s_xcb_win.winHeight = height;


    // If the window has already been created, we can use the xcb_change_window_attributes()
    // function to set the events that the window will receive. The subsection Configuring a
    // window shows its prototype. 
    
    // configure to send notification when window is destroyed
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(s_xcb_win.connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(s_xcb_win.connection, 0, 16, "WM_DELETE_WINDOW");
    
    xcb_intern_atom_reply_t * reply = xcb_intern_atom_reply(s_xcb_win.connection, cookie, 0);
    
    atom_wm_delete_window = xcb_intern_atom_reply(s_xcb_win.connection, cookie2, 0);

    static const char* pVkTitle = "OpenArena";
    /* Set the title of the window */
    xcb_change_property (s_xcb_win.connection, XCB_PROP_MODE_REPLACE, s_xcb_win.hWnd,
        XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(pVkTitle), pVkTitle);

    /* set the title of the window icon */

    static const char * pIconTitle = "OpenArena (iconified)";
    
    xcb_change_property (s_xcb_win.connection, XCB_PROP_MODE_REPLACE, s_xcb_win.hWnd, 
        XCB_ATOM_WM_ICON_NAME, XCB_ATOM_STRING, 8, strlen(pIconTitle), pIconTitle);

    xcb_change_property(s_xcb_win.connection, XCB_PROP_MODE_REPLACE, 
            s_xcb_win.hWnd, (*reply).atom, 4, 32, 1, &(*atom_wm_delete_window).atom);
    

    // The fact that we created the window does not mean that it will be drawn on screen.
    // By default, newly created windows are not mapped on the screen (they are invisible).
    // In order to make our window visible, we use the function xcb_map_window()
    //
    // Mapping a window causes the window to appear on the screen, Un-mapping it causes it 
    // to be removed from the screen (although the window as a logical entity still exists). 
    // This gives the effect of making a window hidden (unmapped) and shown again (mapped). 
    
    Com_Printf(" ... xcb map the window ... \n");
    
    xcb_map_window(s_xcb_win.connection, s_xcb_win.hWnd);
	    
    //  Make sure commands are sent before we pause so that the window gets shown
    xcb_flush (s_xcb_win.connection);
    
    free(reply);

    // input system ?
    IN_Init();

}


void WinSys_Shutdown(void)
{
    xcb_destroy_window( s_xcb_win.connection, s_xcb_win.hWnd);
    
    free(atom_wm_delete_window);
    
    //xcb_disconnect(connection);
    memset(&s_xcb_win, 0, sizeof(s_xcb_win));


    Cmd_RemoveCommand("displayResoList");

    Com_Printf(" Window subsystem destroyed. \n");
}


void WinSys_EndFrame(void)
{
    ;
}


void WinSys_SetGamma(unsigned char red[256], unsigned char green[256], unsigned char blue[256])
{
    Com_Printf(" WinSys_SetGamma: Not Implemented Now. \n");
}


void FileSys_Logging(char * const pComment)
{
    Com_Printf(" FileSys_Logging: GammaNot Implemented Now. \n");
}
