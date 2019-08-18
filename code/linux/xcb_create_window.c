#ifndef __linux__
#error You should not be including this file on non-Linux platforms
#endif

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>

#include "linux_public.h"


static struct xcb_windata_s s_xcb_win;


void WinSys_Init(void ** pContext)
{
	Com_Printf( " Initializing window subsystem. \n" );
   

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
    Com_Printf("  alled depths len......: %d\n", pScreen->allowed_depths_len);
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
    r_mode->integer = 12;
    // -------------------------------------------------
    if(r_fullscreen->integer)
    {
        // if fullscreen is true, then we use desktop video resolution
        r_mode->integer = -2;
        ri.Printf( PRINT_ALL, " Setting fullscreen mode:");
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

    value_list[0] = screen->white_pixel;
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
    
    // If the window has already been created, we can use the xcb_change_window_attributes()
    // function to set the events that the window will receive. The subsection Configuring a
    // window shows its prototype. 
    
    // configure to send notification when window is destroyed
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(s_xcb_win.connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(s_xcb_win.connection, 0, 16, "WM_DELETE_WINDOW");
    
    xcb_intern_atom_reply_t * reply = xcb_intern_atom_reply(s_xcb_win.connection, cookie, 0);
    xcb_intern_atom_reply_t * atom_wm_delete_window = xcb_intern_atom_reply(s_xcb_win.connection, cookie2, 0);

    static const char* pVkTitle = "OpenArena";
    /* Set the title of the window */
    xcb_change_property (s_xcb_win.connection, XCB_PROP_MODE_REPLACE, s_xcb_win.window,
        XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(pVkTitle), pVkTitle);

    /* set the title of the window icon */

    static const char * pIconTitle = "OpenArena (iconified)";
    
    xcb_change_property (s_xcb_win.connection, XCB_PROP_MODE_REPLACE, s_xcb_win.window, 
        XCB_ATOM_WM_ICON_NAME, XCB_ATOM_STRING, 8, strlen(pIconTitle), pIconTitle);

    xcb_change_property(s_xcb_win.connection, XCB_PROP_MODE_REPLACE, 
            s_xcb_win.window, (*reply).atom, 4, 32, 1, &(*atom_wm_delete_window).atom);
    

    // The fact that we created the window does not mean that it will be drawn on screen.
    // By default, newly created windows are not mapped on the screen (they are invisible).
    // In order to make our window visible, we use the function xcb_map_window()
    //
    // Mapping a window causes the window to appear on the screen, Un-mapping it causes it 
    // to be removed from the screen (although the window as a logical entity still exists). 
    // This gives the effect of making a window hidden (unmapped) and shown again (mapped). 
    
    Com_Printf(" ... xcb map the window ... \n");
    
    xcb_map_window(s_xcb_win.connection, s_xcb_win.window);
	    
    //  Make sure commands are sent before we pause so that the window gets shown
    xcb_flush (s_xcb_win.connection);
    
    free(reply);
    free(atom_wm_delete_window);



    // input system ?
    // IN_Init(&s_xcb_win);

}


void WinSys_Shutdown(void)
{
    xcb_destroy_window( s_xcb_win.connection, s_xcb_win.window);

    //xcb_disconnect(connection);
    memset(&s_xcb_win, 0, sizeof(s_xcb_win));

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
















// In the X Window System, a window is characterized by an Id.
// So, in XCB, typedef uint32_t xcb_window_t
typedef uint32_t xcb_window_t;
typedef uint32_t xcb_gcontext_t;


struct xcb_windata_s {
    xcb_connection_t *connection;
    xcb_window_t window;
    xcb_window_t   root;
};

static struct xcb_windata_s s_xcb_win;

static PFN_vkCreateXcbSurfaceKHR qvkCreateXcbSurfaceKHR;

static void* vk_library_handle; // instance of Vulkan library

//                Properties and Atoms
// A property is a collection of named, typed data. 
// The window system has a set of predefined properties (the name of a window, size hints, and so on), 
// and users can define any other arbitrary information and associate it with windows. Each property
// has a name, which is an ISO Latin-1 string. For each named property, a unique identifier (atom)
// is associated with it. A property also has a type, for example, string or integer. These types
// are also indicated using atoms, so arbitrary new types can be defined. Data of only one type 
// may be associated with a single property name. Clients can store and retrieve properties 
// associated with windows. For efficiency reasons, an atom is used rather than a character string.
// XInternAtom() can be used to obtain the atom for property names.
//
// A property is also stored in one of several possible formats. The X server can store the information
// as 8-bit quantities, 16-bit quantities, or 32-bit quantities. This permits the X server
// to present the data in the byte order that the client expects.

// If you define further properties of complex type, you must encode and decode them yourself.
// These functions must be carefully written if they are to be portable. For further information
// about how to write a library extension, see "Extensions".

// The type of a property is defined by an atom, which allows for arbitrary extension in this type scheme.
// 
// Certain property names are predefined in the server for commonly used functions. 
// The atoms for these properties are defined in X11/Xatom.h. To avoid name clashes
// with user symbols, the #define name for each atom has the XA_ prefix. 
// For definitions of these properties, see below. For an explanation of the functions
// that let you get and set much of the information stored in these predefined properties,
// see "Inter-Client Communication Functions".
//
// The core protocol imposes no semantics on these property names, 
// but semantics are specified in other X Consortium standards, 
// such as the Inter-Client Communication Conventions Manual and
// the X Logical Font Description Conventions.

// You can use properties to communicate other information between applications.
// The functions described in this section let you define new properties and get
// the unique atom IDs in your applications.

// Although any particular atom can have some client interpretation within each
// of the name spaces, atoms occur in five distinct name spaces within the protocol:

//  Selections
//  Property names
//  Property types
//  Font properties
//  Type of a ClientMessage event (none are built into the X server)
//
//  The built-in selection property names are:
//
// PRIMARY
// SECONDARY
//
// The built-in property names are:
// CUT_BUFFER0  	RESOURCE_MANAGER
// CUT_BUFFER1	    WM_CLASS
// CUT_BUFFER2	    WM_CLIENT_MACHINE
// CUT_BUFFER3	    WM_COLORMAP_WINDOWS
// CUT_BUFFER4	    WM_COMMAND
// CUT_BUFFER5	    WM_HINTS
// CUT_BUFFER6	    WM_ICON_NAME
// CUT_BUFFER7	    WM_ICON_SIZE
// RGB_BEST_MAP	    WM_NAME
// RGB_BLUE_MAP	    WM_NORMAL_HINTS
// RGB_DEFAULT_MAP	WM_PROTOCOLS
// RGB_GRAY_MAP	    WM_STATE
// RGB_GREEN_MAP	WM_TRANSIENT_FOR
// RGB_RED_MAP	    WM_ZOOM_HINTS
// 
// The built-in property types are:

// ARC	POINT
// ATOM	RGB_COLOR_MAP
// BITMAP	RECTANGLE
// CARDINAL	STRING
// COLORMAP	VISUALID
// CURSOR	WINDOW
// DRAWABLE	WM_HINTS
// FONT	WM_SIZE_HINTS
// INTEGER	
// PIXMAP
//
//
// The built-in font property names are:
// MIN_SPACE	STRIKEOUT_DESCENT
// NORM_SPACE	STRIKEOUT_ASCENT
// MAX_SPACE	ITALIC_ANGLE
// END_SPACE	X_HEIGHT
// SUPERSCRIPT_X	QUAD_WIDTH
// SUPERSCRIPT_Y	WEIGHT
// SUBSCRIPT_X	POINT_SIZE
// SUBSCRIPT_Y	RESOLUTION
// UNDERLINE_POSITION	COPYRIGHT
// UNDERLINE_THICKNESS	NOTICE
// FONT_NAME	FAMILY_NAME
// FULL_NAME	CAP_HEIGHT
//
// For further information about font properties, see "Font Metrics".

// To return an atom for a given name, use XInternAtom(). 
// To return atoms for an array of names, use XInternAtoms().
//
// To return a name for a given atom identifier, use XGetAtomName().
// To return the names for an array of atom identifiers, use XGetAtomNames().
//
//
//  Basic XCB notions
//  XCB has been created to eliminate the need for programs to actually implement the X protocol layer.
//  This library gives a program a very low-level access to any X server. Since the protocol is 
//  standardized, a client using any implementation of XCB may talk with any X server (the same occurs
//  for Xlib, of course). We now give a brief description of the basic XCB notions. They will be detailed
//  later.
//
//  (1) The X Connection
//  The major notion of using XCB is the X Connection. This is a structure representing the connection
//  we have open with a given X server. It hides a queue of messages coming from the server, and a queue
//  of pending requests that our client intends to send to the server. In XCB, this structure is named
//  'xcb_connection_t'. It is analogous to the Xlib Display. When we open a connection to an X server,
//  the library returns a pointer to such a structure. Later, we supply this pointer to any XCB function
//  that should send messages to the X server or receive messages from this server.
//  
//  (2) Requests and replies
//  To ask for information from the X server, we have to make a request and ask for a reply. 
//  With XCB, we can suppress most of the round-trips as the requests and the replies are not
//  locked. We usually send a request, then XCB returns to us a cookie, which is an identifier.
//  Then, later, we ask for a reply using this cookie and XCB returns a pointer to that reply. 
//  Hence, with XCB, we can send a lot of requests, and later in the program, ask for all the
//  replies when we need them.
//  
//  (3) The Graphics Context
//  When we perform various drawing operations (graphics, text, etc), we may specify various
//  options for controlling how the data will be drawn (what foreground and background colors
//  to use, how line edges will be connected, what font to use when drawing some text, etc). 
//  In order to avoid the need to supply hundreds of parameters to each drawing function, a 
//  graphical context structure is used. We set the various drawing options in this structure,
//  and then we pass a pointer to this structure to any drawing routines. This is rather handy,
//  as we often need to perform several drawing requests with the same options. Thus, we would
//  initialize a graphical context, set the desired options, and pass this structure to all 
//  drawing functions.
//
//  Note that graphic contexts have no client-side structure in XCB, they're just XIDs. Xlib
//  has a client-side structure because it caches the GC contents so it can avoid making 
//  redundant requests, but of course XCB doesn't do that.
//
//  (4). Events
//  A structure is used to pass events received from the X server. XCB supports exactly the 
//  events specified in the protocol (33 events). This structure contains the type of event
//  received (including a bit for whether it came from the server or another client), as well
//  as the data associated with the event (e.g. position on the screen where the event was
//  generated, mouse button associated with the event, region of the screen associated with
//  a "redraw" event, etc). The way to read the event's data depends on the event type.
//  



void vk_destroyWindowImpl(void)
{
    // To close a connection, it suffices to use:
    // void xcb_disconnect (xcb_connection_t *c);

    qvkCreateXcbSurfaceKHR = NULL;

    ri.Printf(PRINT_ALL, " Destroy Window Subsystem.\n");

    xcb_destroy_window(s_xcb_win.connection, s_xcb_win.window);

    //xcb_disconnect(connection);
	if (vk_library_handle != NULL)
    {
		ri.Printf(PRINT_ALL, "...unloading Vulkan DLL\n");
		
        dlclose(vk_library_handle);
		
        vk_library_handle = NULL;
	}

}



/*
===============
Minimize the game so that user is back at the desktop
===============
*/
void vk_minimizeWindowImpl( void )
{
    // Hide the window
    xcb_unmap_window(s_xcb_win.connection, s_xcb_win.window);

    // Make sure the unmap window command is sent
    xcb_flush(s_xcb_win.connection);
    //ri.Printf("Not Impled!");
}



////////////////////// doc  ////////////////////////

/*
 * Drawing in a window can be done using various graphical functions
   (drawing pixels, lines, rectangles, etc). In order to draw in a window,
   we first need to define various general drawing parameters, what line 
   width to use, which color to draw with, etc. This is done using a 
   graphical context. A graphical context defines several attributes to be
   used with the various drawing functions. For this, we define a graphical
   context. We can use more than one graphical context with a single window, 
   in order to draw in multiple styles (different colors, line widths, etc).
   In XCB, a Graphics Context is, as a window, characterized by an Id:
*/


/*
static void CreateInstanceImpl(unsigned int numExt, const char* extNames[])
{
	VkApplicationInfo appInfo;
	memset(&appInfo, 0, sizeof(appInfo));
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "OpenArena";
	appInfo.applicationVersion = 1;
	appInfo.pEngineName = "OpenArena";
	appInfo.engineVersion = 1;
	appInfo.apiVersion = VK_API_VERSION_1_0;
	VkInstanceCreateInfo instanceCreateInfo;
	memset(&instanceCreateInfo, 0, sizeof(instanceCreateInfo));
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledExtensionCount = numExt;
	instanceCreateInfo.ppEnabledExtensionNames = extNames;
#ifndef NDEBUG
	const char* validation_layer_name = "VK_LAYER_LUNARG_standard_validation";
	instanceCreateInfo.enabledLayerCount = 1;
	instanceCreateInfo.ppEnabledLayerNames = &validation_layer_name;
#endif
    VkResult e = qvkCreateInstance(&instanceCreateInfo, NULL, &vk.instance);
    if(!e)
    {
        ri.Printf(PRINT_ALL, "---Vulkan create instance success---\n\n");
    }
    else if (e == VK_ERROR_INCOMPATIBLE_DRIVER) {
        ri.Error(ERR_FATAL, 
            "Cannot find a compatible Vulkan installable client driver (ICD).\n" );
    }
    else if (e == VK_ERROR_EXTENSION_NOT_PRESENT)
    {
        ri.Error(ERR_FATAL, "Cannot find a specified extension library.\n");
    }
    else 
    {
        ri.Error(ERR_FATAL, "%d, returned by qvkCreateInstance.\n", e);
    }
}
*/

/*  
void VKimp_CreateInstance(void)
{
    ri.Printf( PRINT_ALL, " VKimp_CreateInstance() \n" );
	// check extensions availability
	unsigned int instance_extension_count = 0;
    VkBool32 surfaceExtFound = 0;
    VkBool32 platformSurfaceExtFound = 0;
     
    const char* extension_names_supported[64] = {0};
    unsigned int enabled_extension_count = 0;
	VK_CHECK(qvkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL));
    if (instance_extension_count > 0)
    {
        VkExtensionProperties *instance_extensions = 
            (VkExtensionProperties *) malloc(sizeof(VkExtensionProperties) * instance_extension_count);
        
        VK_CHECK(qvkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, instance_extensions));
            
        unsigned int i = 0;
        for (i = 0; i < instance_extension_count; i++)
        {
            if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName))
            {
                surfaceExtFound = 1;
                extension_names_supported[enabled_extension_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
            }
            if (!strcmp(VK_KHR_XCB_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName))
            {
                platformSurfaceExtFound = 1;
                extension_names_supported[enabled_extension_count++] = VK_KHR_XCB_SURFACE_EXTENSION_NAME;
            }
#ifndef NDEBUG
            if (!strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, instance_extensions[i].extensionName))
            {
                extension_names_supported[enabled_extension_count++] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
            }
#endif
        }
            
        assert(enabled_extension_count < 64);
            
        free(instance_extensions);
    }
    if (!surfaceExtFound)
		ri.Error(ERR_FATAL, "Vulkan: required instance extension is not available: %s", "surfaceExt");
    if (!platformSurfaceExtFound)
		ri.Error(ERR_FATAL, "Vulkan: required instance extension is not available: %s", "platformSurfaceExt");
    CreateInstanceImpl(enabled_extension_count, extension_names_supported);
}
*/
