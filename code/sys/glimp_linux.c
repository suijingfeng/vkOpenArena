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
** GLW_IMP.C
**
** This file contains ALL Linux specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_Shutdown
** GLimp_SetGamma
**
*/

#include <termios.h>
#include <sys/ioctl.h>
#ifdef __linux__
  #include <sys/stat.h>
  #include <sys/vt.h>
#endif
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

/*
#if !defined(__sun)
#include <X11/extensions/Xxf86dga.h>
#endif
*/


#include <dlfcn.h>


#ifdef _XF86DGA_H_
#define HAVE_XF86DGA
#endif

#include "../client/client.h"
#include "local.h"
#include "inputs.h"


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


static cvar_t* r_fullscreen;
static cvar_t* r_availableModes;

static cvar_t* r_customwidth;
static cvar_t* r_customheight;

static cvar_t* r_swapInterval;
static cvar_t* r_mode;

cvar_t	*r_stencilbits;
cvar_t	*r_depthbits;
cvar_t	*r_colorbits;

cvar_t	*r_stereoSeparation;

cvar_t   *vid_xpos;
cvar_t   *vid_ypos;

static cvar_t* r_glDriver;
cvar_t* r_drawBuffer;
///////////////////////////
 
glwstate_t glw_state;

Display *dpy = NULL;
Window win = 0;

int scrnum;
int window_width = 0;
int window_height = 0;


extern cvar_t *in_dgamouse; // user pref for dga mouse

static GLXContext ctx = NULL;

Atom wmDeleteEvent = None;


qboolean window_created = qfalse;



////////////////////////////////////////////////////////////////////////////////////////
typedef struct vidmode_s
{
	const char	*description;
	int			width, height;
	float		pixelAspect;		// pixel width / height
} vidmode_t;

static const vidmode_t r_vidModes[] = {
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
	{ "Mode 11: 856x480",		856,	480,	1 },		// Q3 MODES END HERE AND EXTENDED MODES BEGIN
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
	{ "Mode 26: 2560x1600",		2560,	1600,	1 },
	{ "Mode 27: 3840x2160 (4K)",	3840,	2160,	1 }
};
static const int s_numVidModes = ARRAY_LEN( r_vidModes );

qboolean CL_GetModeInfo( int *width, int *height, int mode, int dw, int dh, qboolean fullscreen )
{
	if ( mode < -2 )
		return qfalse;

	if ( mode >= s_numVidModes )
		return qfalse;

	// fix unknown desktop resolution
	if ( mode == -2 && (dw == 0 || dh == 0) )
		mode = 3;

	if ( mode == -2 ) { // desktop resolution
		*width = dw;
		*height = dh;
	} else if ( mode == -1 ) { // custom resolution
		*width = r_customwidth->integer;
		*height = r_customheight->integer;
	} else { // predefined resolution
		*width  = r_vidModes[ mode ].width;
		*height = r_vidModes[ mode ].height;
	}
	return qtrue;
}

/*
static void GLimp_DetectAvailableModes(void)
{
	int i, j;
	char buf[ MAX_STRING_CHARS ] = { 0 };


	SDL_DisplayMode windowMode;
    
	// If a window exists, note its display index
	if( SDL_window != NULL )
	{
		r_displayIndex->integer = SDL_GetWindowDisplayIndex( SDL_window );
		if( r_displayIndex->integer < 0 )
		{
			Com_Printf("SDL_GetWindowDisplayIndex() failed: %s\n", SDL_GetError() );
            return;
		}
	}

	int numSDLModes = SDL_GetNumDisplayModes( r_displayIndex->integer );

	if( SDL_GetWindowDisplayMode( SDL_window, &windowMode ) < 0 || numSDLModes <= 0 )
	{
		Com_Printf("Couldn't get window display mode, no resolutions detected: %s\n", SDL_GetError() );
		return;
	}

	int numModes = 0;
	SDL_Rect* modes = SDL_calloc(numSDLModes, sizeof( SDL_Rect ));
	if ( !modes )
	{
        ////////////////////////////////////
		Com_Error(ERR_FATAL, "Out of memory" );
        ////////////////////////////////////
	}

	for( i = 0; i < numSDLModes; i++ )
	{
		SDL_DisplayMode mode;

		if( SDL_GetDisplayMode( r_displayIndex->integer, i, &mode ) < 0 )
			continue;

		if( !mode.w || !mode.h )
		{
			Com_Printf( "Display supports any resolution\n" );
			SDL_free( modes );
			return;
		}

		if( windowMode.format != mode.format )
			continue;

		// SDL can give the same resolution with different refresh rates.
		// Only list resolution once.
		for( j = 0; j < numModes; j++ )
		{
			if( (mode.w == modes[ j ].w) && (mode.h == modes[ j ].h) )
				break;
		}

		if( j != numModes )
			continue;

		modes[ numModes ].w = mode.w;
		modes[ numModes ].h = mode.h;
		numModes++;
	}

	for( i = 0; i < numModes; i++ )
	{
		const char *newModeString = va( "%ux%u ", modes[ i ].w, modes[ i ].h );

		if( strlen( newModeString ) < (int)sizeof( buf ) - strlen( buf ) )
			Q_strcat( buf, sizeof( buf ), newModeString );
		else
			Com_Printf( "Skipping mode %ux%u, buffer too small\n", modes[ i ].w, modes[ i ].h );
	}

	if( *buf )
	{
		buf[ strlen( buf ) - 1 ] = 0;
		Com_Printf("Available modes: '%s'\n", buf );
		Cvar_Set( "r_availableModes", buf );
	}
	SDL_free( modes );
}
*/

static void ModeList_f( void )
{
	int i;

	Com_Printf("\n" );
	for ( i = 0; i < s_numVidModes; i++ )
	{
		Com_Printf( "%s\n", r_vidModes[i].description );
	}
	Com_Printf("\n" );
}

////////////////////////////////////////////////////////////////////////////////////////

void* GLimp_GetProcAddress( const char *symbol )
{
	void *sym = dlsym(glw_state.OpenGLLib, symbol);
//    void *sym = glXGetProcAddressARB((const unsigned char *)symbol);
    return sym;
}




/*
** GL_Shutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.
*/
static void GL_Shutdown( qboolean unloadDLL )
{
	Com_Printf( "...shutting down GL\n" );

	if ( glw_state.OpenGLLib && unloadDLL )
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

		dlclose( glw_state.OpenGLLib );

		glw_state.OpenGLLib = NULL;
	}
}



////////////////////////////////////////////////////////////////////////////////
//about glw
static int GLW_SetMode(glconfig_t *config, int mode, qboolean fullscreen )
{
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


	Window root;
	XVisualInfo *visinfo;

	XSetWindowAttributes attr;
	XSizeHints sizehints;
	unsigned long mask;
	int actualWidth, actualHeight, actualRate;


	window_width = 0;
	window_height = 0;
	window_created = qfalse;

	glw_state.dga_ext = qfalse;
	glw_state.randr_ext = qfalse;
	glw_state.vidmode_ext = qfalse;

	dpy = XOpenDisplay( NULL );

	if ( dpy == NULL )
	{
		Com_Printf( "Couldn't open the X display\n" );
	}

	scrnum = DefaultScreen( dpy );
	root = RootWindow( dpy, scrnum );

	// Init xrandr and get desktop resolution if available
	RandR_Init( vid_xpos->integer, vid_ypos->integer, 640, 480 );

	if ( !glw_state.randr_ext )
	{
		VidMode_Init();
	}

#ifdef HAVE_XF86DGA
	if ( in_dgamouse && in_dgamouse->integer )
	{
		if ( !DGA_Init( dpy ) )
		{
			Cvar_Set( "in_dgamouse", "0" );
		}
	}
#endif
	printf( "...setting mode %d:", mode );

	if ( !CL_GetModeInfo( &config->vidWidth, &config->vidHeight, mode, glw_state.desktop_width, glw_state.desktop_height, fullscreen ) )
	{
        Com_Error( ERR_FATAL, " invalid mode\n" );
	}

	actualWidth = config->vidWidth;
	actualHeight = config->vidHeight;

	if ( actualRate )
		Com_Printf( " %d %d @%iHz\n", actualWidth, actualHeight, actualRate );
	else
		Com_Printf( " %d %d\n", actualWidth, actualHeight );

	if ( fullscreen ) // try randr first
	{
		RandR_SetMode( &actualWidth, &actualHeight, &actualRate );
	}

	if ( glw_state.vidmode_ext && !glw_state.randr_active )
	{
		if ( fullscreen )
			VidMode_SetMode( &actualWidth, &actualHeight, &actualRate );
		else
			Com_Printf( "XFree86-VidModeExtension: Ignored on non-fullscreen\n" );
	}


    attrib[ATTR_DEPTH_IDX] = 24; // default to 24 depth
    attrib[ATTR_STENCIL_IDX] = 0;
    attrib[ATTR_RED_IDX] = 8;
    attrib[ATTR_GREEN_IDX] = 8;
    attrib[ATTR_BLUE_IDX] = 8;
    visinfo = qglXChooseVisual( dpy, scrnum, attrib );
	if ( !visinfo )
	{
		Com_Printf( "Couldn't get a visual\n" );
	}

    printf( "Using %d/%d/%d Color bits, %d depth, %d stencil display.\n", 
        attrib[ATTR_RED_IDX], attrib[ATTR_GREEN_IDX], attrib[ATTR_BLUE_IDX],
        attrib[ATTR_DEPTH_IDX], attrib[ATTR_STENCIL_IDX]);

    config->colorBits = 24;
    config->depthBits = 24;
    config->stencilBits = 0;


    config->vidWidth = window_width = actualWidth;
    config->vidHeight = window_height = actualHeight;
    config->displayFrequency = actualRate;

    config->windowAspect = (float) window_width / (float) window_height;
    config->isFullscreen = glw_state.cdsFullscreen = fullscreen;



	/* window attributes */
	attr.background_pixel = BlackPixel( dpy, scrnum );
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone );
	attr.event_mask = X_MASK;

	if ( fullscreen )
	{
		mask = CWBackPixel | CWColormap | CWSaveUnder | CWBackingStore |
			CWEventMask | CWOverrideRedirect;
		attr.override_redirect = True;
		attr.backing_store = NotUseful;
		attr.save_under = False;
	}
	else
	{
		mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
	}

	win = XCreateWindow( dpy, root, 0, 0,
		actualWidth, actualHeight,
		0, visinfo->depth, InputOutput,
		visinfo->visual, mask, &attr );

	XStoreName( dpy, win, CLIENT_WINDOW_TITLE );

	/* GH: Don't let the window be resized */
	sizehints.flags = PMinSize | PMaxSize;
	sizehints.min_width = sizehints.max_width = actualWidth;
	sizehints.min_height = sizehints.max_height = actualHeight;

	XSetWMNormalHints( dpy, win, &sizehints );

	XMapWindow( dpy, win );

	wmDeleteEvent = XInternAtom( dpy, "WM_DELETE_WINDOW", True );
	if ( wmDeleteEvent == BadValue )
		wmDeleteEvent = None;
	if ( wmDeleteEvent != None )
		XSetWMProtocols( dpy, win, &wmDeleteEvent, 1 );

    if(win)
	{
        window_created = qtrue;
        //GLimp_DetectAvailableModes();
    }
	if ( fullscreen )
	{
		if ( glw_state.randr_active || glw_state.vidmode_active )
			XMoveWindow( dpy, win, glw_state.desktop_x, glw_state.desktop_y );
	}
	else
	{
		XMoveWindow( dpy, win, vid_xpos->integer, vid_ypos->integer );
	}

	XFlush( dpy );
	XSync( dpy, False );
	ctx = qglXCreateContext( dpy, visinfo, NULL, True );
	XSync( dpy, False );

	/* GH: Free the visinfo after we're done with it */
	XFree( visinfo );

	qglXMakeCurrent( dpy, win, ctx );


	Key_ClearStates();

	XSetInputFocus( dpy, win, RevertToParent, CurrentTime );

	return 0;
}



/*
** GLW_LoadOpenGL
**
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
	if ( glw_state.OpenGLLib == NULL )
	{
		glw_state.OpenGLLib = Sys_LoadLibrary( dllname );
		Com_Printf( "load %s\n", dllname);

        if ( glw_state.OpenGLLib == NULL )
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

	if( qglXSwapIntervalEXT || qglXSwapIntervalMESA || qglXSwapIntervalSGI )
	{
		Com_Printf( "...using GLX_EXT_swap_control\n" );
		Cvar_SetModified( "r_swapInterval", qtrue ); // force a set next frame
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
	printf( "X Error of failed request: %s\n", buf) ;
	printf( "  Major opcode of failed request: %d\n", ev->request_code );
	printf( "  Minor opcode of failed request: %d\n", ev->minor_code );
	printf( "  Serial number of failed request: %d\n", (int)ev->serial );
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////

/*
** GLimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of OpenGL.
*/
void GLimp_Init( glconfig_t *config )
{
    r_fullscreen = Cvar_Get( "r_fullscreen", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_mode = Cvar_Get( "r_mode", "-2", CVAR_ARCHIVE | CVAR_LATCH );

	r_colorbits = Cvar_Get( "r_colorbits", "24", CVAR_ARCHIVE | CVAR_LATCH );
	r_stencilbits = Cvar_Get( "r_stencilbits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_depthbits = Cvar_Get( "r_depthbits", "24", CVAR_ARCHIVE | CVAR_LATCH ); 
	r_stereoSeparation = Cvar_Get( "r_stereoSeparation", "64", CVAR_ARCHIVE );
    r_drawBuffer = Cvar_Get( "r_drawBuffer", "GL_BACK", CVAR_CHEAT );
    
	r_customwidth = Cvar_Get( "r_customwidth", "1920", CVAR_ARCHIVE | CVAR_LATCH );
	r_customheight = Cvar_Get( "r_customheight", "1080", CVAR_ARCHIVE | CVAR_LATCH );
   	r_swapInterval = Cvar_Get( "r_swapInterval", "0", CVAR_ARCHIVE );
	r_glDriver = Cvar_Get( "r_glDriver", OPENGL_DRIVER_NAME, CVAR_ARCHIVE | CVAR_LATCH );


    vid_xpos = Cvar_Get( "vid_xpos", "3", CVAR_ARCHIVE );
	vid_ypos = Cvar_Get( "vid_ypos", "22", CVAR_ARCHIVE );


	if ( r_swapInterval->integer )
		setenv( "vblank_mode", "2", 1 );
	else
		setenv( "vblank_mode", "1", 1 );

	// load the GL layer


    Cmd_AddCommand( "modelist", ModeList_f );

	// This values force the UI to disable driver selection
	config->driverType = GLDRV_ICD;
	config->hardwareType = GLHW_GENERIC;

    IN_Init();   // rcg08312005 moved into glimp.

	// set up our custom error handler for X failures
	XSetErrorHandler( &qXErrorHandler );

	//
	// load and initialize the specific OpenGL driver
	//
	printf( "...GLW_StartOpenGL...\n" );
	// load and initialize the specific OpenGL driver
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


    // create the window and set up the context

    if( 0 != GLW_SetMode( config, r_mode->integer, (r_fullscreen->integer != 0) ))
    {
        r_mode->integer = 3;
        if(0 != GLW_SetMode( config, 3, qfalse ))
            Com_Error(ERR_FATAL, "Error setting given display modes\n" );
    }
}


/*
** GLimp_EndFrame
** 
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
void GLimp_EndFrame( void )
{
	// don't flip if drawing to front buffer
	if ( Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) != 0 )
	{
		qglXSwapBuffers( dpy, win );
	}

	//
	// swapinterval stuff
	//
	if ( r_swapInterval->modified ) {
		r_swapInterval->modified = qfalse;

		if ( qglXSwapIntervalEXT ) {
			qglXSwapIntervalEXT( dpy, win, r_swapInterval->integer );
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

	if ( dpy )
	{
		if ( glw_state.randr_gamma && glw_state.gammaSet )
		{
			RandR_RestoreGamma();
			glw_state.gammaSet = qfalse;
		}

		RandR_RestoreMode();

		if ( ctx )
			qglXDestroyContext( dpy, ctx );

		if ( win )
			XDestroyWindow( dpy, win );

		if ( glw_state.gammaSet )
		{
			VidMode_RestoreGamma();
			glw_state.gammaSet = qfalse;
		}

		if ( glw_state.vidmode_active )
			VidMode_RestoreMode();

		// NOTE TTimo opening/closing the display should be necessary only once per run
		// but it seems GL_Shutdown gets called in a lot of occasion
		// in some cases, this XCloseDisplay is known to raise some X errors
		// ( https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=33 )
		XCloseDisplay( dpy );
	}

	RandR_Done();
	VidMode_Done();

	glw_state.desktop_ok = qfalse;

	dpy = NULL;
	win = 0;
	ctx = NULL;

	unsetenv( "vblank_mode" );
	
	//if ( glw_state.cdsFullscreen )
	{
		glw_state.cdsFullscreen = qfalse;
	}
    Cmd_RemoveCommand("modelist");

	GL_Shutdown( unloadDLL );
}


/*
** GLimp_LogComment
*/
void GLimp_LogComment( char *comment )
{
	if ( glw_state.log_fp )
	{
		fprintf( glw_state.log_fp, "%s", comment );
	}
}



//////////////////////////////////////////////////////////////////////////////////////////////////


/*
=================
Sys_GetClipboardData
=================
*/
char *Sys_GetClipboardData( void )
{
	const Atom xtarget = XInternAtom( dpy, "UTF8_STRING", 0 );
	unsigned long nitems, rem;
	unsigned char *data;
	Atom type;
	XEvent ev;
	char *buf;
	int format;

	XConvertSelection( dpy, XA_PRIMARY, xtarget, XA_PRIMARY, win, CurrentTime );
	XSync( dpy, False );
	XNextEvent( dpy, &ev );
	if ( !XFilterEvent( &ev, None ) && ev.type == SelectionNotify ) {
		if ( XGetWindowProperty( dpy, win, XA_PRIMARY, 0, 8, False, AnyPropertyType,
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


