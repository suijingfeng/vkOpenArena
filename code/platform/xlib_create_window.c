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
#include <sys/ioctl.h>

#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>


#include <GL/gl.h>
#include <GL/glext.h>

#include <GL/glx.h>
#include <GL/glxext.h>

#include <dlfcn.h>

#include "../client/client.h"
#include "sys_public.h"
#include "win_public.h"
#include "x11_randr.h"

#include "WinSys_Common.h"

#define OPENGL_DRIVER_NAME	"libGL.so.1"

#define QGL_LinX11_PROCS \
	GLE( XVisualInfo*, glXChooseVisual, Display *dpy, int screen, int *attribList ) \
	GLE( GLXContext, glXCreateContext, Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct ) \
	GLE( void, glXDestroyContext, Display *dpy, GLXContext ctx ) \
	GLE( Bool, glXMakeCurrent, Display *dpy, GLXDrawable drawable, GLXContext ctx) \
	GLE( void, glXCopyContext, Display *dpy, GLXContext src, GLXContext dst, GLuint mask ) \
	GLE( void, glXSwapBuffers, Display *dpy, GLXDrawable drawable )


#define QGL_Swp_PROCS \
	GLE( void,	glXSwapIntervalEXT, Display *dpy, GLXDrawable drawable, int interval ) \
	GLE( int,	glXSwapIntervalMESA, unsigned interval ) \
	GLE( int,	glXSwapIntervalSGI, int interval )


#define GLE( ret, name, ... ) ret ( APIENTRY * q##name )( __VA_ARGS__ );
    QGL_LinX11_PROCS;
    QGL_Swp_PROCS;
#undef GLE


/////////////////////////////

static cvar_t* r_mode;
static cvar_t* r_fullscreen;

static cvar_t* r_swapInterval;


cvar_t	*r_stencilbits;
cvar_t	*r_depthbits;
cvar_t	*r_colorbits;

cvar_t	*r_stereoSeparation;

cvar_t   *vid_xpos;
cvar_t   *vid_ypos;

static cvar_t* r_glDriver;
cvar_t* r_drawBuffer;
///////////////////////////
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


// extern cvar_t *in_dgamouse; // user pref for dga mouse

static GLXContext ctx = NULL;

Atom wmDeleteEvent = None;



void* GLimp_GetProcAddress( const char *symbol )
{
    //void *sym = glXGetProcAddressARB((const unsigned char *)symbol);
    return dlsym(glw_state.hGraphicLib, symbol);
}


/*
** GL_Shutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.
*/
static void GL_Shutdown( qboolean unloadDLL )
{
	Com_Printf( "...shutting down GL\n" );

	if ( glw_state.hGraphicLib && unloadDLL )
	{
		Com_Printf( "...unloading OpenGL DLL\n" );
		// 25/09/05 Tim Angus <tim@ngus.net>
		// Certain combinations of hardware and software, specifically
		// Linux/SMP/Nvidia/agpgart (OK, OK. MY combination of hardware and
		// software), seem to cause a catastrophic (hard reboot required) crash
		// when libGL is dynamically unloaded. I'm unsure of the precise cause,
		// suffice to say I don't see anything in the Q3 code that could cause it.
		// I suspect it's an Nvidia driver bug, but without the source or means to
		// debug I obviously can't prove (or disprove) this. Interestingly (though
		// perhaps not suprisingly), Enemy Territory and Doom 3 both exhibit the
		// same problem.
		//
		// After many, many reboots and prodding here and there, it seems that a
		// placing a short delay before libGL is unloaded works around the problem.
		// This delay is changable via the r_GLlibCoolDownMsec cvar (nice name
		// huh?), and it defaults to 0. For me, 500 seems to work.
		//if( r_GLlibCoolDownMsec->integer )
		//	usleep( r_GLlibCoolDownMsec->integer * 1000 );
		usleep( 250 * 1000 );

		dlclose( glw_state.hGraphicLib );

		glw_state.hGraphicLib = NULL;
	}
}



////////////////////////////////////////////////////////////////////////////////
//about glw
static int GLW_SetMode(int mode, qboolean fullscreen, int type )
{
	XSizeHints sizehints;
	int actualWidth, actualHeight, actualRate;


	int scrnum = DefaultScreen( glw_state.pDisplay );
	glw_state.root = RootWindow( glw_state.pDisplay, scrnum );

	// Init xrandr and get desktop resolution if available
	RandR_Init( vid_xpos->integer, vid_ypos->integer, 640, 480 );


	Com_Printf( "...setting mode %d:", mode );

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

	// these match in the array
	#define ATTR_RED_IDX 2
	#define ATTR_GREEN_IDX 4
	#define ATTR_BLUE_IDX 6
	#define ATTR_DEPTH_IDX 9
	#define ATTR_STENCIL_IDX 11

	static int attrib[] =
	{
		GLX_RGBA,         // 0
		GLX_RED_SIZE, 4,      // 1, 2
		GLX_GREEN_SIZE, 4,      // 3, 4
		GLX_BLUE_SIZE, 4,     // 5, 6
		GLX_DOUBLEBUFFER,     // 7
		GLX_DEPTH_SIZE, 1,      // 8, 9
		GLX_STENCIL_SIZE, 1,    // 10, 11
		None
	};
	

	attrib[ATTR_DEPTH_IDX] = 24; // default to 24 depth
    attrib[ATTR_STENCIL_IDX] = 8;
    attrib[ATTR_RED_IDX] = 8;
    attrib[ATTR_GREEN_IDX] = 8;
    attrib[ATTR_BLUE_IDX] = 8;

    XVisualInfo * visinfo = NULL;
    if(type == 0)
    {
        // OpenGL case
	    visinfo = qglXChooseVisual( glw_state.pDisplay, scrnum, attrib );
	    
        if ( !visinfo )
	    {
		    Com_Printf( "Couldn't get a visual\n" );
	    }

        Com_Printf( "Using %d/%d/%d Color bits, %d depth, %d stencil display.\n", 
            attrib[ATTR_RED_IDX], attrib[ATTR_GREEN_IDX], attrib[ATTR_BLUE_IDX],
            attrib[ATTR_DEPTH_IDX], attrib[ATTR_STENCIL_IDX]);
    }
    else if(type == 1)
    {
        int numberOfVisuals;
        XVisualInfo vInfoTemplate = {};
        vInfoTemplate.screen = DefaultScreen(glw_state.pDisplay);
        // vulkan case
        visinfo = XGetVisualInfo(glw_state.pDisplay, VisualScreenMask, &vInfoTemplate, &numberOfVisuals);

        Com_Printf( "... numberOfVisuals: %d \n", numberOfVisuals);
    }


    glw_state.winWidth = actualWidth;
    glw_state.winHeight = actualHeight;
    glw_state.isFullScreen = fullscreen; 
    

	/* window attributes */
	XSetWindowAttributes attr;

	attr.background_pixel = BlackPixel( glw_state.pDisplay, scrnum );
	attr.border_pixel = 0;

    // The XCreateColormap() function creates a colormap of the specified visual type for the screen
    // on which the specified window resides and returns the colormap ID associated with it. Note that
    // the specified window is only used to determine the screen. 
	attr.colormap = XCreateColormap( glw_state.pDisplay, glw_state.root, visinfo->visual, AllocNone );
	
    attr.event_mask = ( 
            KeyPressMask | KeyReleaseMask | 
            ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ButtonMotionMask |
            VisibilityChangeMask | StructureNotifyMask | FocusChangeMask );

	unsigned long mask = fullscreen ? 
			( CWBackPixel | CWColormap | CWEventMask | CWSaveUnder | CWBackingStore | CWOverrideRedirect ) : 
			( CWBackPixel | CWColormap | CWEventMask | CWBorderPixel );


	if ( fullscreen )
	{
		attr.override_redirect = True;
		attr.backing_store = NotUseful;
		attr.save_under = False;
	}

	glw_state.hWnd = XCreateWindow( glw_state.pDisplay, glw_state.root, 
		0, 0, actualWidth, actualHeight,
		0, visinfo->depth, InputOutput,
		visinfo->visual, mask, &attr );


	XStoreName( glw_state.pDisplay, glw_state.hWnd, CLIENT_WINDOW_TITLE );

    Com_Printf( "... XCreateWindow created. \n");


	/* GH: Don't let the window be resized */
	sizehints.flags = PMinSize | PMaxSize;
	sizehints.min_width = sizehints.max_width = actualWidth;
	sizehints.min_height = sizehints.max_height = actualHeight;

	XSetWMNormalHints( glw_state.pDisplay, glw_state.hWnd, &sizehints );

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
        ctx = qglXCreateContext( glw_state.pDisplay, visinfo, NULL, True );
    
        Com_Printf( "... glX Create context . \n");
    }
    
    XSync( glw_state.pDisplay, False );

	/* GH: Free the visinfo after we're done with it */
	XFree( visinfo );

	return 0;
}



/*
** GLimp_win.c internal function that that attempts to load and use a specific OpenGL DLL.
**
**                 https://www.khronos.org/registry/OpenGL/ABI/
**
** This is responsible for binding our qgl function pointers to the appropriate GL stuff.
** In Windows this means doing a LoadLibrary and a bunch of calls to GetProcAddress.
** On other operating systems we need to do the right thing, whatever that might be.
** 
** There are two link-level libraries. libGL includes the OpenGL and GLX entry points 
** and in general depends on underlying hardware and/or X server dependent code that 
** may or may not be incorporated into this library. 
** The libraries must export all OpenGL 1.2, GLU 1.3, GLX 1.3, and ARB_multitexture 
** entry points statically. It's possible (but unlikely) that additional ARB or vendor
** extensions will be mandated before the ABI is finalized. Applications should not 
** expect to link statically against any entry points not specified here. Because 
** non-ARB extensions vary so widely and are constantly increasing in number, 
** it's infeasible to require that they all be supported, and extensions can always 
** be added to hardware drivers after the base link libraries are released. These 
** drivers are dynamically loaded by libGL, so extensions not in the base library
** must also be obtained dynamically. 
** 
** To perform the dynamic query, libGL also must export an entry point called 
    void (*glXGetProcAddressARB(const GLubyte *))();
** It takes the string name of a GL or GLX entry point and returns a pointer to
** a function implementing that entry point. It is functionally identical to the 
** wglGetProcAddress query defined by the Windows OpenGL library, except that the 
** function pointers returned are context independent, unlike the WGL query. 
** All OpenGL and GLX entry points may be queried with this extension; 

** Thread safety (the ability to issue OpenGL calls to different graphics contexts
** from different application threads) is required. Multithreaded applications must
** use -lpthread. 
** libGL must be transitively linked with any libraries they require in their own 
** internal implementation, so that applications don't fail on some implementations
** due to not pulling in libraries needed not by the app, but by the implementation. 
** 
** The following header files are required:

    <GL/gl.h> --- OpenGL
    <GL/glx.h> --- GLX
    <GL/glext.h> --- OpenGL Extensions
    <GL/glxext.h> --- GLX Extensions

** All OpenGL 1.2 and ARB_multitexture, and GLX 1.3 entry points and enumerants
** must be present in the corresponding header files gl.h, and glx.h, 
** even if only OpenGL 1.1 is implemented at runtime by the associated runtime libraries. 
** Non-ARB OpenGL extensions are defined in glext.h, and non-ARB GLX extensions in glxext.h.

** gl.h must define the symbol GL_OGLBASE_VERSION. This symbol must be an integer
** defining the version of the ABI supported by the headers. Its value is 
** 1000 * major_version + minor_version where major_version and minor_version are
** the major and minor revision numbers of this ABI standard. The primary purpose
** of the symbol is to provide a compile-time test by which application code knows
** whether the ABI guarantees are in force.
*/


static qboolean GLW_LoadOpenGL(const char* dllname)
{
	if ( glw_state.hGraphicLib == NULL )
	{
		glw_state.hGraphicLib = dlopen(dllname, RTLD_NOW);
		Com_Printf( " load %s ...\n", dllname);

        if ( glw_state.hGraphicLib == NULL )
		{
			Com_Error(ERR_FATAL, "GL_Init: failed to load %s from /etc/ld.so.conf: %s\n", dllname, dlerror());
		}
	}
		
    // expand constants before stringifying them
    // load the GLX funs
    #define GLE( ret, name, ... ) \
        q##name = GLimp_GetProcAddress( XSTRING( name ) ); if ( !q##name ) Com_Error(ERR_FATAL, "Error resolving glx core functions\n");
	    QGL_LinX11_PROCS;
    #undef GLE


    #define GLE( ret, name, ... ) \
        q##name = GLimp_GetProcAddress( XSTRING( name ) );
        QGL_Swp_PROCS;
    #undef GLE

	if( qglXSwapIntervalEXT )
	{
		Com_Printf( "...using GLX_EXT_swap_control\n" );
	}
	else
	{
		Com_Printf( "...GLX_EXT_swap_control not found\n" );
	}

    return qtrue;
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
	static char buf[1024];
	XGetErrorText( dpy, ev->error_code, buf, sizeof( buf ) );
	Com_Printf( "X Error of failed request: %s\n", buf) ;
	Com_Printf( "  Major opcode of failed request: %d\n", ev->request_code );
	Com_Printf( "  Minor opcode of failed request: %d\n", ev->minor_code );
	Com_Printf( "  Serial number of failed request: %d\n", (int)ev->serial );
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////

/*
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
void WinSys_EndFrame( void )
{
	// don't flip if drawing to front buffer
	if ( Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) != 0 )
	{
		qglXSwapBuffers( glw_state.pDisplay, glw_state.hWnd );
	}

	//
	// swapinterval stuff
	//
	if ( r_swapInterval->modified )
    {
		r_swapInterval->modified = qfalse;

		if ( qglXSwapIntervalEXT ) {
			qglXSwapIntervalEXT( glw_state.pDisplay, glw_state.hWnd, r_swapInterval->integer );
		} else if ( qglXSwapIntervalMESA ) {
			qglXSwapIntervalMESA( r_swapInterval->integer );
		} else if ( qglXSwapIntervalSGI ) {
			qglXSwapIntervalSGI( r_swapInterval->integer );
		}
	}
}


/*
** GLimp_Shutdown
**
** This routine does all OS specific shutdown procedures for the OpenGL
** subsystem.  Under OpenGL this means NULLing out the current DC and
** HGLRC, deleting the rendering context, and releasing the DC acquired
** for the window.  The state structure is also nulled out.
**
*/
void GLimp_Shutdown( qboolean unloadDLL )
{
	IN_DeactivateMouse();

	if ( glw_state.pDisplay )
	{
		if ( glw_state.randr_gamma && glw_state.gammaSet )
		{
			RandR_RestoreGamma();
			glw_state.gammaSet = qfalse;
		}

		RandR_RestoreMode();

		if ( ctx )
			qglXDestroyContext( glw_state.pDisplay, ctx );

		if ( glw_state.hWnd )
			XDestroyWindow( glw_state.pDisplay, glw_state.hWnd );


		// NOTE TTimo opening/closing the display should be necessary only once per run
		// but it seems GL_Shutdown gets called in a lot of occasion
		// in some cases, this XCloseDisplay is known to raise some X errors
		// ( https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=33 )
		XCloseDisplay( glw_state.pDisplay );
	}

	RandR_Done();

	glw_state.pDisplay = NULL;
	glw_state.hWnd = 0;
	ctx = NULL;

	unsetenv( "vblank_mode" );
	
	if ( glw_state.isFullScreen )
	{
		glw_state.isFullScreen = qfalse;
	}

	GL_Shutdown( unloadDLL );
}


void FileSys_Logging( char *comment )
{
/*   
	if ( glw_state.log_fp )
	{
		fprintf( glw_state.log_fp, "%s", comment );
	}
*/
}



//////////////////////////////////////////////////////////////////////////////////////////////////


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
	r_drawBuffer = Cvar_Get( "r_drawBuffer", "GL_BACK", CVAR_CHEAT );
    

   	r_swapInterval = Cvar_Get( "r_swapInterval", "0", CVAR_ARCHIVE );
	r_glDriver = Cvar_Get( "r_glDriver", OPENGL_DRIVER_NAME, CVAR_ARCHIVE | CVAR_LATCH );


	vid_xpos = Cvar_Get( "vid_xpos", "3", CVAR_ARCHIVE );
	vid_ypos = Cvar_Get( "vid_ypos", "22", CVAR_ARCHIVE );

	if ( r_swapInterval->integer )
		setenv( "vblank_mode", "2", 1 );
	else
		setenv( "vblank_mode", "1", 1 );

	WinSys_ConstructDislayModes();

    Cmd_AddCommand( "minimize", WinMinimize_f );
    
    *pCfg = &glw_state;

/*
** Initializing the OS specific portions of OpenGL / vulkan.
*/
	glw_state.randr_ext = qfalse;

	// set up our custom error handler for X failures
	XSetErrorHandler( &qXErrorHandler );


    if(type == 0)
    {
        //
	    // load and initialize the specific OpenGL driver
	    //
        if ( !GLW_LoadOpenGL( r_glDriver->string ) )
        {
            if ( Q_stricmp( r_glDriver->string, OPENGL_DRIVER_NAME ) != 0 )
            {
                // try default driver
                if ( GLW_LoadOpenGL( OPENGL_DRIVER_NAME ) )
                {
                    Cvar_Set( "r_glDriver", OPENGL_DRIVER_NAME );
                    r_glDriver->modified = qfalse;
                    return;
                }
            }

            Com_Error( ERR_FATAL, "GLW_StartOpenGL() - could not load OpenGL subsystem\n" );
        }
    }
    else if(type == 1)
    {
        // vulkan part
    
    }

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

    // create the window and set up the context

    if( 0 != GLW_SetMode( r_mode->integer, (r_fullscreen->integer != 0), type ))
    {
		Com_Error(ERR_FATAL, "Error setting given display modes\n" );
    }


    if(type == 0)
	{
		qglXMakeCurrent( glw_state.pDisplay, glw_state.hWnd, ctx );
	}

	Key_ClearStates();

	XSetInputFocus( glw_state.pDisplay, glw_state.hWnd, RevertToParent, CurrentTime );


    IN_Init();   // rcg08312005 moved into glimp.
}


void WinSys_Shutdown(void)
{
	Cmd_RemoveCommand( "minimize" );
	WinSys_DestructDislayModes( );
	GLimp_Shutdown( qtrue );
}


