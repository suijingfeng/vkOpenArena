/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
/*
** This file contains ALL Linux specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
*/

#include <termios.h>

#include <stdarg.h>
#include <stdio.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>

#include "../client/client.h"
#include "sys_public.h"
#include "win_public.h"
#include "x11_randr.h"

#include "WinSys_Common.h"


/////////////////////////////
extern void XSys_LoadOpenGL(void);
extern void XSys_UnloadOpenGL(void);
extern XVisualInfo * GetXVisualPtrWrapper(void);
extern void XSys_CreateContextForGL(XVisualInfo * pVisinfo);
extern void XSys_SetCurrentContextForGL(void);
extern void XSys_ClearCurrentContextForGL(void);

///////////////////////////
static cvar_t* r_mode;
static cvar_t* r_fullscreen;

cvar_t* r_swapInterval;
static cvar_t* r_allowResize; // make window resizable

cvar_t * r_stencilbits;
cvar_t * r_depthbits;
cvar_t * r_colorbits;

cvar_t * vid_xpos;
cvar_t * vid_ypos;



WinVars_t glw_state;


int WinSys_GetWinWidth(void)
{
    return glw_state.winWidth;
}

int WinSys_GetWinHeight(void)
{
    return glw_state.winHeight;
}

int WinSys_IsWinFullscreen(void)
{
    return glw_state.isFullScreen;
}


// 
//   Properties and Atoms
// A property is a collection of named, typed data. The window system has a set of predefined properties 
// (for example, the name of a window, size hints, and so on), and users can define any other arbitrary 
// information and associate it with windows. Each property has a name, which is an ISO Latin-1 string. 
// For each named property, a unique identifier (atom) is associated with it.  
// A property also has a type, for example, string or integer. These types are also indicated using atoms, 
// so arbitrary new types can be defined. Data of only one type may be associated with a single property name. 
// Clients can store and retrieve properties associated with windows. For efficiency reasons, 
// an atom is used rather than a character string. XInternAtom() can be used to obtain the atom for property names.
//
// A property is also stored in one of several possible formats. 
// The X server can store the information as 8-bit quantities, 16-bit quantities, or 32-bit quantities.
// This permits the X server to present the data in the byte order that the client expects. 
// 
// If you define further properties of complex type, you must encode and decode them yourself.
// These functions must be carefully written if they are to be portable. 
// For further information about how to write a library extension, see "Extensions".
//
// The type of a property is defined by an atom, which allows for arbitrary extension in this type scheme. 
//
// Certain property names are predefined in the server for commonly used functions. 
// The atoms for these properties are defined in X11/Xatom.h. To avoid name clashes with user symbols, 
// the #define name for each atom has the XA_ prefix. For definitions of these properties, see below. 
// For an explanation of the functions that let you get and set much of the information stored in these predefined properties,
//  see "Inter-Client Communication Functions". 
//
//
Atom wmDeleteEvent = None;


////////////////////////////////////////////////////////////////////////////////
//about glw
static int CreateWindowForRenderer(int mode, qboolean fullscreen, int type )
{

	int actualWidth, actualHeight, actualRate;


	glw_state.screenIdx = DefaultScreen( glw_state.pDisplay );
	glw_state.root = RootWindow( glw_state.pDisplay, glw_state.screenIdx );

	// Init xrandr and get desktop resolution if available
	RandR_Init( vid_xpos->integer, vid_ypos->integer, 640, 480 );


	Com_Printf( " Setting display mode %d:", mode );

	if ( !CL_GetModeInfo( &actualWidth, &actualHeight, mode, glw_state.desktopWidth, glw_state.desktopHeight, fullscreen ) )
	{
		Com_Error( ERR_FATAL, " invalid mode\n" );
	}


	if ( actualRate )
		Com_Printf( " %d %d @%iHz\n", actualWidth, actualHeight, actualRate );
	else
		Com_Printf( " %d %d\n", actualWidth, actualHeight );

	if ( fullscreen ) // try randr first
	{
		RandR_SetMode( &actualWidth, &actualHeight, &actualRate );
	}

	glw_state.winWidth = actualWidth;
	glw_state.winHeight = actualHeight;
	glw_state.isFullScreen = fullscreen; 
	
	
	XVisualInfo * visinfo = NULL;
	if(type == 0)
	{
		visinfo = GetXVisualPtrWrapper();
		if(visinfo == NULL)
		{
			Com_Error(ERR_FATAL,  "XGetVisualInfo() says no visuals available!\n" );
		}
	}
	else if(type == 1)
	{
		int numberOfVisuals;
		XVisualInfo vInfoTemplate = {};
		vInfoTemplate.screen = glw_state.screenIdx;
		vInfoTemplate.class = TrueColor;
		vInfoTemplate.red_mask = 8;
		vInfoTemplate.green_mask = 8;
		vInfoTemplate.blue_mask = 8;
		vInfoTemplate.depth = 24; 
		// vulkan case
		//
		// XVisualInfo * XGetVisualInfo(Display * display, long vinfo_mask, XVisualInfo * vinfo_template, int * nitems_return)
		//  display: Specifies the connection to the X server;
		//  vinfo_mask: Specifies the visual mask value;
		//  vinfo_template: Specifies the visual attributes that are to be used in matching the visual structures. 
		//  nitems_return: returns the number of matching visual structures
		//
		// The XGetVisualInfo() function returns a list of visual structures that have attributes equal to the attributes
		//  specified by vinfo_template. 
		visinfo = XGetVisualInfo(glw_state.pDisplay, VisualScreenMask | VisualClassMask, 
				&vInfoTemplate, &numberOfVisuals);

		if(visinfo == NULL)
		{
			Com_Printf( "XGetVisualInfo() says no visuals available!\n" );
		}

		Com_Printf( "... numberOfVisuals: %d \n", numberOfVisuals);
	}


	/* window attributes */
	unsigned long win_mask = fullscreen ? 
		( CWBackPixel | CWColormap | CWEventMask | CWSaveUnder | CWBackingStore | CWOverrideRedirect ) : 
		( CWBackPixel | CWColormap | CWEventMask | CWBorderPixel );

	// Window attribute value mask bits 


	XSetWindowAttributes win_attr;

	win_attr.background_pixel = BlackPixel( glw_state.pDisplay, glw_state.screenIdx );
	win_attr.border_pixel = 10;
	
	// The XCreateColormap() function creates a colormap of the specified visual type for the screen
	// on which the specified window resides and returns the colormap ID associated with it. Note that
	// the specified window is only used to determine the screen. 
	//
	// The initial values of the colormap entries are undefined for the visual classes GrayScale, 
	// PseudoColor, and DirectColor. For StaticGray, StaticColor, and TrueColor, the entries have defined values, 
	// but those values are specific to the visual and are not defined by X. 
	// For StaticGray, StaticColor, and TrueColor, alloc must be AllocNone, or a BadMatch error results. 
	// For the other visual classes, if alloc is AllocNone, the colormap initially has no allocated entries, 
	// and clients can allocate them. For information about the visual types. 
	//
	// Xlib requires specification of a colormap when creating a window.
	// The GLX extension, which integrates OpenGL and X, is used by X servers that support OpenGL.
	// GLX is both an API and an X extension protocol for supporting OpenGL. 
	// GLX routines provide basic interaction between X and OpenGL. 
	// Use them, for example, to create a rendering context and bind it to a window. 
	// A standard X visual specifies how the server should map a given pixel value to a color to be displayed on the screen. 
	// Different windows on the screen can have different visuals.
	// GLX overloads X visuals to include both the standard X definition of a visual and 
	// OpenGL specific information about the configuration of the framebuffer and ancillary
	// buffers that might be associated with a drawable. Only those overloaded visuals support 
	// both OpenGL and X rendering—GLX therefore requires that an X server support a 
	// high minimum baseline of OpenGL functionality.
	// 
	// Not all X visuals support OpenGL rendering, but all X servers capable of 
	// OpenGL rendering have at least two OpenGL capable visuals.
	// An RGBA visual is required for any hardware system that supports OpenGL
	//
	// As a rule, a drawable is something X can draw into, either a window or a pixmap 
	// (an exception is pbuffers, which are GLX drawables but cannot be used for X rendering).
	//
	// A GLX drawable is something both OpenGL can draw into, either an OpenGL capable window or a GLX pixmap.
	//  (A GLX pixmap is a handle to an X pixmap that is allocated in a special way;
	//
	//  Another kind of GLX drawable is the pixel buffer (or pbuffer), which permits hardware-accelerated off-screen rendering.
	//
	//  Resources As Server Data
	//
	//  Resources, in X, are data structures maintained by the server rather than by client programs. 
	//  Colormaps (as well as windows, pixmaps, and fonts) are implemented as resources.
	//
	//  Rather than keeping information about a window in the client program and sending an entire window data structure from client to server, 
	//  for instance, window data is stored in the server and given a unique integer ID called an XID. To manipulate or query the window data, 
	//  the client sends the window's ID number; the server can then perform any requested operation on that window. This reduces network traffic.
	// 
	// Because pixmaps and windows are resources, they are part of the X server and can be shared by different processes (or threads). 
	// OpenGL contexts are also resources. In standard OpenGL, they can be shared by threads in the same process but not by separate processes
	//
	// X Window Colormaps
	//
	// A colormap maps pixel values from the framebuffer to intensities on screen.  
	// Each pixel value indexes into the colormap to produce intensities of red, green, and blue for display. 
	// Depending on hardware limitations, one or more colormaps may be installed at one time, 
	// such that windows associated with those maps display with the correct colors. 
	// If there is only one colormap, two windows that load colormaps with different values look correct 
	// only when they have their particular colormap is installed. The X window manager takes care of 
	// colormap installation and tries to make sure that the X client with input focus has its colormaps installed. 
	// On all systems, the colormap is a limited resource.
	//
	// Every X window needs a colormap. If you are using the OpenGL drawing area-widget to render in RGB mode into a TrueColor visual, 
	// you may not need to worry about the colormap. In other cases, you may need to assign one. For additional information, 
	// see “Using Colormaps”. Colormaps are also discussed in detail in O'Reilly, Volume One. 


	// OpenGL supports two rendering modes: RGBA mode and color index mode.
	//
	// In RGBA mode, color buffers store red, green, blue, and alpha components directly.
	//
	// In color-index mode, color buffers store indexes (names) of colors that are dereferenced by the display hardware. 
	// A color index represents a color by name rather than value. A colormap is a table of index-to-RGB mappings.
	
	// OpenGL 1.0 and 1.1 and GLX 1.0, 1.1, and 1.2 require an RGBA mode program to use a TrueColor or DirectColor visual, 
	// and require a color index mode program to use a PseudoColor or StaticColor visual. 
	win_attr.colormap = XCreateColormap( glw_state.pDisplay, glw_state.root, visinfo->visual, AllocNone );
	
	win_attr.event_mask = ( 
            KeyPressMask | KeyReleaseMask | 
            ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ButtonMotionMask |
            VisibilityChangeMask | StructureNotifyMask | FocusChangeMask );


	if ( fullscreen )
	{
		win_attr.override_redirect = True;
		win_attr.backing_store = NotUseful;
		win_attr.save_under = False;
		win_attr.border_pixel = 0;
	}


	// The XCreateWindow function creates an unmapped subwindow for a specified parent window, 
	// returns the window ID of the created window, and causes the X server to generate a CreateNotify event. 
	// The created window is placed on top in the stacking order with respect to siblings. 
	//
	// The coordinate system has the X axis horizontal and the Y axis vertical with the origin [0, 0] at the upper-left corner. 
	// Coordinates are integral, in terms of pixels, and coincide with pixel centers. 
	// Each window and pixmap has its own coordinate system. 
	// For a window, the origin is inside the border at the inside, upper-left corner. 
	//
	// The border_width for an InputOnly window must be zero, or a BadMatch error results. 
	// For class InputOutput, the visual type and depth must be a combination supported for the screen, 
	// or a BadMatch error results. The depth need not be the same as the parent, 
	// but the parent must not be a window of class InputOnly, or a BadMatch error results. 
	// For an InputOnly window, the depth must be zero, and the visual must be one supported by the screen. 
	// If either condition is not met, a BadMatch error results. 
	// The parent window, however, may have any depth and class. 
	// If you specify any invalid window attribute for a window, a BadMatch error results. 
	//
	//
	// Window XCreateWindow ( Display *display, Window parent, 
	// 	int x, int y, unsigned int width, unsigned int height, 
	// 	unsigned int border_width, int depth, unsigned int class, 
	// 	Visual *visual, unsigned long valuemask, XSetWindowAttributes *attributes); 
	//
	// attributes:  Specifies the structure from which the values (as specified by the value mask) are to be taken. 
	// The value mask should have the appropriate bits set to indicate which attributes have been set in the structure.
	//
	// class:  Specifies the created window's class. You can pass InputOutput, InputOnly, or CopyFromParent.
	// A class of CopyFromParent means the class is taken from the parent.  
	//
	// depth:  Specifies the window's depth. A depth of CopyFromParent means the depth is taken from the parent. 
	//
	// valuemask:  Specifies which window attributes are defined in the attributes argument. 
	// This mask is the bitwise inclusive OR of the valid attribute mask bits. 
	// If valuemask is zero, the attributes are ignored and are not referenced.
	//
	// visual:  Specifies the visual type. A visual of CopyFromParent means the visual type is taken from the parent.
	//
	// width, height: Specify the width and height, which are the created window's inside dimensions and
	//  do not include the created window's borders. 
	glw_state.hWnd = XCreateWindow( glw_state.pDisplay, glw_state.root, 
		0, 0, actualWidth, actualHeight,
		0, visinfo->depth, InputOutput,
		visinfo->visual, win_mask, &win_attr );

	// All InputOutput windows have a border width of zero or more pixels, an optional background, 
	// an event suppression mask (which suppresses propagation of events from children), and a property list (see "Properties and Atoms"). 
	// The window border and background can be a solid color or a pattern, called a tile. 
	// All windows except the root have a parent and are clipped by their parent. 
	// If a window is stacked on top of another window, it obscures that other window for the purpose of input. 
	// If a window has a background (almost all do), it obscures the other window for purposes of output. 
	// Attempts to output to the obscured area do nothing, 
	// and no input events (for example, pointer motion) are generated for the obscured area. 


	Com_Printf( " X Window created. \n");

	XStoreName( glw_state.pDisplay, glw_state.hWnd, CLIENT_WINDOW_TITLE );
	
	// Don't let the window be resized.

	if( r_allowResize->integer == 0 )
	{
		XSizeHints sizehints;
		sizehints.flags = PMinSize | PMaxSize;
		sizehints.min_width = sizehints.max_width = actualWidth;
		sizehints.min_height = sizehints.max_height = actualHeight;

		XSetWMNormalHints( glw_state.pDisplay, glw_state.hWnd, &sizehints );
	}

	// The created window is not yet displayed (mapped) on the user's display.
	// To display the window, call XMapWindow. 
	// The new window initially uses the same cursor as its parent. 
	// A new cursor can be defined for the new window by calling XDefineCursor.
	// The window will not be visible on the screen unless it and all of 
	// its ancestors are mapped and it is not obscured by any of its ancestors.
	
	
	XMapWindow( glw_state.pDisplay, glw_state.hWnd );

	wmDeleteEvent = XInternAtom( glw_state.pDisplay, "WM_DELETE_WINDOW", True );
	if ( wmDeleteEvent == BadValue )
		wmDeleteEvent = None;
	if ( wmDeleteEvent != None )
		XSetWMProtocols( glw_state.pDisplay, glw_state.hWnd, &wmDeleteEvent, 1 );


	if ( fullscreen )
	{
		if ( glw_state.randr_active )
			XMoveWindow( glw_state.pDisplay, glw_state.hWnd, glw_state.desktop_x, glw_state.desktop_y );
	}
	else
	{
		XMoveWindow( glw_state.pDisplay, glw_state.hWnd, vid_xpos->integer, vid_ypos->integer );
	}

	XFlush( glw_state.pDisplay );
	XSync( glw_state.pDisplay, False );
	
	if(type == 0)
	{
		XSys_CreateContextForGL( visinfo );
    	}
	

    	XSync( glw_state.pDisplay, False );

	/* GH: Free the visinfo after we're done with it */
	XFree( visinfo );

	return 0;
}



/*
** XErrorHandler
**   the default X error handler exits the application
**   I found out that on some hosts some operations would raise X errors (GLXUnsupportedPrivateRequest)
**   but those don't seem to be fatal .. so the default would be to just ignore them
**   our implementation mimics the default handler behaviour (not completely cause I'm lazy)
*/
static int qXErrorHandler( Display *dpy, XErrorEvent *ev )
{
	char buf[1024];
	XGetErrorText( dpy, ev->error_code, buf, sizeof( buf ) );
	Com_Printf( "X Error of failed request: %s\n", buf) ;
	Com_Printf( "  Major opcode of failed request: %d\n", ev->request_code );
	Com_Printf( "  Minor opcode of failed request: %d\n", ev->minor_code );
	Com_Printf( "  Serial number of failed request: %d\n", (int)ev->serial );
	return 0;
}


/*
=================
Sys_GetClipboardData
=================
*/
char *Sys_GetClipboardData( void )
{
	const Atom xtarget = XInternAtom( glw_state.pDisplay, "UTF8_STRING", 0 );
	unsigned long nitems, rem;
	unsigned char *data;
	Atom type;
	XEvent ev;
	char *buf;
	int format;

	XConvertSelection( glw_state.pDisplay, XA_PRIMARY, xtarget, XA_PRIMARY, glw_state.hWnd, CurrentTime );
	XSync( glw_state.pDisplay, False );
	XNextEvent( glw_state.pDisplay, &ev );
	if ( !XFilterEvent( &ev, None ) && ev.type == SelectionNotify ) {
		if ( XGetWindowProperty( glw_state.pDisplay, glw_state.hWnd, XA_PRIMARY, 0, 8, False, AnyPropertyType,
			&type, &format, &nitems, &rem, &data ) == 0 ) {
			if ( format == 8 ) {
				if ( nitems > 0 ) {
					buf = Z_Malloc( nitems + 1 );
					Q_strncpyz( buf, (char*)data, nitems + 1 );
					strtok( buf, "\n\r\b" );
					return buf;
				}
			} else {
				fprintf( stderr, "Bad clipboard format %i\n", format );
			}
		} else {
			fprintf( stderr, "Clipboard allocation failed\n" );
		}
	}
	return NULL;
}



static qboolean WindowMinimized( void )
{
	unsigned long i, num_items, bytes_after;
	Atom actual_type, *atoms, nws, nwsh;
	int actual_format;

	nws = XInternAtom( glw_state.pDisplay, "_NET_WM_STATE", True );
	if ( nws == BadValue || nws == None )
		return qfalse;

	nwsh = XInternAtom( glw_state.pDisplay, "_NET_WM_STATE_HIDDEN", True );
	if ( nwsh == BadValue || nwsh == None )
		return qfalse;

	atoms = NULL;

	XGetWindowProperty( glw_state.pDisplay, glw_state.hWnd, nws, 0, 0x7FFFFFFF, False, XA_ATOM,
		&actual_type, &actual_format, &num_items,
		&bytes_after, (unsigned char**)&atoms );

	for ( i = 0; i < num_items; i++ )
	{
		if ( atoms[i] == nwsh )
		{
			XFree( atoms );
			return qtrue;
		}
	}

	XFree( atoms );
	return qfalse;
}

//////////////////
void WinMinimize_f(void)
{
    glw_state.isMinimized = WindowMinimized( );
    Com_Printf( " gw_minimized: %i\n", glw_state.isMinimized );
}

/*
type 0: OpenGL
type 1: Vulkan
type 2: directx
*/
void WinSys_Init(void ** pCfg, int type)
{
	Com_Printf( "... Window System Specific Init ...\n" );

	r_fullscreen = Cvar_Get( "r_fullscreen", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_mode = Cvar_Get( "r_mode", "-2", CVAR_ARCHIVE | CVAR_LATCH );

	r_colorbits = Cvar_Get( "r_colorbits", "32", CVAR_ARCHIVE | CVAR_LATCH );
	r_stencilbits = Cvar_Get( "r_stencilbits", "8", CVAR_ARCHIVE | CVAR_LATCH );
	r_depthbits = Cvar_Get( "r_depthbits", "24", CVAR_ARCHIVE | CVAR_LATCH );

   	r_swapInterval = Cvar_Get( "r_swapInterval", "0", CVAR_ARCHIVE );

	// r_glDriver = Cvar_Get( "r_glDriver", "libGL.so.1", CVAR_ARCHIVE | CVAR_LATCH );

	r_allowResize = Cvar_Get( "r_allowResize", "0", CVAR_ARCHIVE | CVAR_LATCH );
	
	vid_xpos = Cvar_Get( "vid_xpos", "3", CVAR_ARCHIVE );
	vid_ypos = Cvar_Get( "vid_ypos", "22", CVAR_ARCHIVE );

	if ( r_swapInterval->integer )
		setenv( "vblank_mode", "2", 1 );
	else
		setenv( "vblank_mode", "1", 1 );

	WinSys_ConstructDislayModes();
	
	Cmd_AddCommand( "minimize", WinMinimize_f );
	
	*pCfg = &glw_state;

	glw_state.randr_ext = qfalse;

	// set up our custom error handler for X failures
	XSetErrorHandler( &qXErrorHandler );

	// To open a connection to the X server that controls a display
	// char *display_name: Specifies the hardware display name, 
	// which determines the display and communications domain to be used. 
	// On a POSIX-conformant system, if the display_name is NULL, 
	// it defaults to the value of the DISPLAY environment variable.
	//
	// The encoding and interpretation of the display name is implementation dependent.
	// Strings in the Host Portable Character Encoding are supported; support for other
	// characters is implementation dependent. On POSIX-conformant systems, the display
	// name or DISPLAY environment variable can be a string in the format:
	//
	// hostname:number.screen_number
	//
	// hostname: Specifies the name of the host machine on which the display is physically attached.
	// You follow the hostname with either a single colon (:) or a double colon (::). 
	//
	// number : Specifies the number of the display server on that host machine. You may optionally 
	// follow this display number with a period (.). A single CPU can have more than one display. 
	// Multiple displays are usually numbered starting with zero. 
	
	glw_state.pDisplay = XOpenDisplay( NULL );

	if ( glw_state.pDisplay == NULL )
	{
		Com_Printf( " Couldn't open the X display. \n" );
	}

	if(type == 0)
	{
		// load and initialize the specific OpenGL driver
		XSys_LoadOpenGL( );
	}
	// create the window and set up the context

	if( 0 != CreateWindowForRenderer( r_mode->integer, (r_fullscreen->integer != 0), type ))
	{
		Com_Error(ERR_FATAL, "Error setting given display modes\n" );
	}

	if(type == 0)
	{
		// load and initialize the specific OpenGL driver
		XSys_SetCurrentContextForGL();
	}
	else if(type == 1)
	{
		// vulkan part
	}

	Key_ClearStates();

	XSetInputFocus( glw_state.pDisplay, glw_state.hWnd, RevertToParent, CurrentTime );

	IN_Init();   // rcg08312005 moved into glimp.
}


/*
** This routine does all OS specific shutdown procedures for the OpenGL
** subsystem.  Under OpenGL this means NULLing out the current DC and
** HGLRC, deleting the rendering context, and releasing the DC acquired
** for the window.  The state structure is also nulled out.
*/
void WinSys_Shutdown(void)
{
	Cmd_RemoveCommand( "minimize" );
	
	WinSys_DestructDislayModes( );
	
	IN_DeactivateMouse();

	if ( glw_state.pDisplay )
	{
		if ( glw_state.randr_gamma && glw_state.gammaSet )
		{
			RandR_RestoreGamma();
			glw_state.gammaSet = qfalse;
		}

		RandR_RestoreMode();


		
		XSys_ClearCurrentContextForGL();

		if ( glw_state.hWnd )
		{
			XDestroyWindow( glw_state.pDisplay, glw_state.hWnd );
			glw_state.hWnd = 0;
		}

		// NOTE TTimo opening/closing the display should be necessary only once per run
		// but it seems GL_Shutdown gets called in a lot of occasion
		// in some cases, this XCloseDisplay is known to raise some X errors
		// ( https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=33 )
		XCloseDisplay( glw_state.pDisplay );
		glw_state.pDisplay = NULL;
	}

	RandR_Done();

	unsetenv( "vblank_mode" );
	
	if ( glw_state.isFullScreen )
	{
		glw_state.isFullScreen = qfalse;
	}

	XSys_UnloadOpenGL();
}
