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

#ifdef __linux__
  #include <sys/stat.h>
  #include <sys/vt.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>


#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>


#include <X11/extensions/Xxf86dga.h>
#ifdef _XF86DGA_H_
#define HAVE_XF86DGA
#endif

#include "../client/client.h"
#include "sys_public.h"
#include "sys_local.h"

/////////////////////////////

extern cvar_t *in_dgamouse; // user pref for dga mouse

static GLXContext ctx = NULL;
static qboolean ctxErrorOccurred = qfalse;

static cvar_t* r_fullscreen;

static cvar_t* r_customwidth;
static cvar_t* r_customheight;

static cvar_t* r_mode;
static cvar_t* r_glDriver;
cvar_t* r_drawBuffer;

///////////////////////////
 
typedef enum
{
  RSERR_OK,

  RSERR_INVALID_FULLSCREEN,
  RSERR_INVALID_MODE,

  RSERR_UNKNOWN
} rserr_t;

glwstate_t glw_state;

Display *dpy = NULL;
Window win = 0;

int scrnum;
int window_width = 0;
int window_height = 0;


Atom wmDeleteEvent = None;


qboolean window_created = qfalse;



cvar_t   *vid_xpos;
cvar_t   *vid_ypos;


////////////////////////////////////////////////////////////////////////////////////////
typedef struct vidmode_s
{
	const char	*description;
	int			width, height;
	float		pixelAspect;		// pixel width / height
} vidmode_t;

static const vidmode_t cl_vidModes[19] =
{
	{ "Mode  0: 1920x1080",			1920,	1080,	1 },
	{ "Mode  1: 1440x900 (16:10)",	1440,	900,	1 },
	{ "Mode  2: 1366x768",			1366,	768,	1 },
	{ "Mode  3: 640x480",			640,	480,	1 },
	{ "Mode  4: 800x600",			800,	600,	1 },
	{ "Mode  5: 1280x800 (16:10)",	1280,	800,	1 },
	{ "Mode  6: 1024x768",			1024,	768,	1 },
	{ "Mode  7: 1152x864",			1152,	864,	1 },
	{ "Mode  8: 1280x1024 (5:4)",	1280,	1024,	1 },
	{ "Mode  9: 1600x1200",			1600,	1200,	1 },
	{ "Mode 10: 2048x1536",			2048,	1536,	1 },
	{ "Mode 11: 4096x2160 (4K)",	4096,	2160,	1 },
	// extra modes:
	{ "Mode 12: 1280x960",			1280,	960,	1 },
	{ "Mode 13: 1280x720",			1280,	720,	1 },
	{ "Mode 14: 2560x1080 (21:9)",	2560,	1080,	1 },
	{ "Mode 15: 3840x2160",			3840,	2160,	1 },
	{ "Mode 16: 1600x900",			1600,	900,	1 },
    { "Mode 17: 3440x1440 (21:9)",	3440,	1440,	1 },
	{ "Mode 18: 1680x1050 (16:10)",	1680,	1050,	1 },

};
static const int s_numVidModes = ARRAY_LEN( cl_vidModes );

qboolean CL_GetModeInfo( int *width, int *height, int mode, int dw, int dh, qboolean fullscreen )
{
	const	vidmode_t *vm;


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
		vm = &cl_vidModes[ mode ];
		*width  = vm->width;
		*height = vm->height;
	}
	return qtrue;
}


static void ModeList_f( void )
{
	int i;

	printf("\n" );
	for ( i = 0; i < s_numVidModes; i++ )
	{
		printf( "%s\n", cl_vidModes[i].description );
	}
	printf("\n" );
}

////////////////////////////////////////////////////////////////////////////////////////

#define GLX_CONTEXT_MAJOR_VERSION_ARB       0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB       0x2092
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

// Helper to check for extension string presence.  Adapted from:
//   http://www.opengl.org/resources/features/OGLextensions/
static qboolean isExtensionSupported(const char *extList, const char *extension)
{
  const char *start;
  const char *where, *terminator;
  
  /* Extension names should not have spaces. */
  where = strchr(extension, ' ');
  if (where || *extension == '\0')
    return qfalse;

  /* It takes a bit of care to be fool-proof about parsing the
     OpenGL extensions string. Don't be fooled by sub-strings,
     etc. */
  for (start=extList;;) {
    where = strstr(start, extension);

    if (!where)
      break;

    terminator = where + strlen(extension);

    if ( where == start || *(where - 1) == ' ' )
      if ( *terminator == ' ' || *terminator == '\0' )
        return qtrue;

    start = terminator;
  }

  return qfalse;
}


/////////////////////////////////////////////////////////////////////////////////////////
void* GLimp_GetProcAddress(const char *symbol)
{
    return Sys_GetFunAddr(glw_state.OpenGLLib, symbol);
}

/*
** This is responsible for binding our qgl function pointers to 
** the appropriate GL stuff.  In Windows this means doing a 
** LoadLibrary and a bunch of calls to GetProcAddress.  On other
** operating systems we need to do the right thing, whatever that
** might be.
** 
*/



/*
** GL_Shutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.
*/
static void GL_Shutdown( qboolean unloadDLL )
{
	printf( "...shutting down GL\n" );

	if ( glw_state.OpenGLLib && unloadDLL )
	{
		printf( "...unloading OpenGL DLL\n" );
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
		//if( r_GLlibCoolDownMsec->integer )`
		//	usleep( r_GLlibCoolDownMsec->integer * 1000 );
		usleep( 50 * 1000 );

		Sys_UnloadDll( glw_state.OpenGLLib );

		glw_state.OpenGLLib = NULL;
	}
    
}





////////////////////////////////////////////////////////////////////////////////
//about glw
static int GLW_SetMode(int mode, qboolean fullscreen, glconfig_t* config, qboolean context)
{

	XSizeHints sizehints;
	unsigned long mask;
	int actualWidth = 0, actualHeight = 0, actualRate = 60;

	window_width = 0;
	window_height = 0;
	window_created = qfalse;

	glw_state.dga_ext = qfalse;
	glw_state.randr_ext = qfalse;
	glw_state.vidmode_ext = qfalse;

	dpy = XOpenDisplay( NULL );

	if ( dpy == NULL )
	{
		fprintf( stderr, "Error: couldn't open the X display\n" );
		return RSERR_INVALID_MODE;
	}

	scrnum = DefaultScreen( dpy );
	Window root = RootWindow( dpy, scrnum );

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
		printf( " invalid mode\n" );
		return RSERR_INVALID_MODE;
	}

	actualWidth = config->vidWidth;
	actualHeight = config->vidHeight;

	if ( actualRate )
		printf( " %d %d @%iHz\n", actualWidth, actualHeight, actualRate );
	else
		printf( " %d %d\n", actualWidth, actualHeight );

	if ( fullscreen ) // try randr first
	{
		RandR_SetMode( &actualWidth, &actualHeight, &actualRate );
	}

	if ( glw_state.vidmode_ext && !glw_state.randr_active )
	{
		if ( fullscreen )
			VidMode_SetMode( &actualWidth, &actualHeight, &actualRate );
		else
			printf( "XFree86-VidModeExtension: Ignored on non-fullscreen\n" );
	}


/////////////////////////////
    if(context)
    {
     // Get a matching FB config
        static int visual_attribs[] =
        {
          GLX_X_RENDERABLE    , True,
          GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
          GLX_RENDER_TYPE     , GLX_RGBA_BIT,
          GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
          GLX_RED_SIZE        , 8,
          GLX_GREEN_SIZE      , 8,
          GLX_BLUE_SIZE       , 8,
          GLX_ALPHA_SIZE      , 8,
          GLX_DEPTH_SIZE      , 24,
          GLX_STENCIL_SIZE    , 8,
          GLX_DOUBLEBUFFER    , True, 
          //GLX_SAMPLE_BUFFERS  , 1,
          //GLX_SAMPLES         , 4,
          None
        };

        int glx_major, glx_minor;
     
        // FBConfigs were added in GLX version 1.3.
        if ( !glXQueryVersion( dpy, &glx_major, &glx_minor ) || ( ( glx_major == 1 ) && ( glx_minor < 3 ) ) || ( glx_major < 1 ) )
        {
            printf("Invalid GLX version");
            exit(1);
        }
        
        printf( "Getting matching framebuffer configs\n" );
        int fbcount;
        GLXFBConfig* fbc = glXChooseFBConfig(dpy, scrnum, visual_attribs, &fbcount);
        if (!fbc)
        {
            printf( "Failed to retrieve a framebuffer config\n" );
            exit(1);
        }
        printf( "Found %d matching FB configs.\n", fbcount );

        // Pick the FB config/visual with the most samples per pixel
        printf( "Getting XVisualInfos\n" );
        int best_fbc = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = 999;

        int i;
        for (i=0; i<fbcount; ++i)
        {
            XVisualInfo *vi = glXGetVisualFromFBConfig( dpy, fbc[i] );
            if ( vi )
            {
                int samp_buf, samples;
                glXGetFBConfigAttrib(dpy, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf );
                glXGetFBConfigAttrib(dpy, fbc[i], GLX_SAMPLES       , &samples  );
          
                printf( "  Matching fbconfig %d, visual ID 0x%2x: SAMPLE_BUFFERS = %d,"
                  " SAMPLES = %d\n", i, vi -> visualid, samp_buf, samples );
            
                if ( (best_fbc < 0) || samp_buf && (samples > best_num_samp) )
                {
                    best_fbc = i;
                    best_num_samp = samples;
                }
                
                if ( (worst_fbc < 0) || !samp_buf || (samples < worst_num_samp) )
                {
                    worst_fbc = i;
                    worst_num_samp = samples;
                }
            }
            XFree( vi );
        }
        
        GLXFBConfig bestFbc = fbc[ best_fbc ];


        // Get a visual
        XVisualInfo *vi = glXGetVisualFromFBConfig( dpy, bestFbc );
        printf( "Chosen visual ID = 0x%x\n", vi->visualid );

        
///////////        
 
        XSetWindowAttributes attr;
        attr.colormap = XCreateColormap(dpy, root, vi->visual, AllocNone );
        attr.background_pixmap = None ;
        attr.border_pixel = BlackPixel( dpy, scrnum );
        attr.event_mask = (KeyPressMask | KeyReleaseMask)
                | (ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ButtonMotionMask )
                | (VisibilityChangeMask | StructureNotifyMask | FocusChangeMask );        
       
        // Be sure to free the FBConfig list allocated by glXChooseFBConfig()
        XFree( fbc );

        if ( fullscreen )
        {
            mask = CWBackPixel | CWColormap | CWSaveUnder | CWBackingStore | CWEventMask | CWOverrideRedirect;
            attr.override_redirect = True;
            attr.backing_store = NotUseful;
            attr.save_under = False;
        }
        else
        {
            mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
        }
        
        win = XCreateWindow( dpy, root, 0, 0, actualWidth, actualHeight, 0, 
                vi->depth, InputOutput, vi->visual, mask, &attr );
        if ( !win )
        {
            printf( "Failed to create window.\n" );
            exit(1);
        }
             // Done with the visual info data
        XFree( vi );

        XStoreName( dpy, win, CLIENT_WINDOW_TITLE);
        XMapWindow( dpy, win );       
//////////
        glw_state.cdsFullscreen = fullscreen;


        // Get the default screen's GLX extension list
        const char *glxExts = glXQueryExtensionsString( dpy, scrnum);

        // NOTE: It is not necessary to create or make current to a context before
        // calling glXGetProcAddressARB
        glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
        glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc) glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB" );


        // Install an X error handler so the application won't exit if GL 3.0
        // context allocation fails.
        //
        // Note this error handler is global.  All display connections in all threads
        // of a process use the same error handler, so be sure to guard against other
        // threads issuing X commands while this code is running.
        ctxErrorOccurred = qfalse;
        //int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(&ctxErrorHandler);

        // Check for the GLX_ARB_create_context extension string and the function.
        // If either is not present, use GLX 1.3 context creation method.
        if ( !isExtensionSupported( glxExts, "GLX_ARB_create_context" ) || !glXCreateContextAttribsARB )
        {
            printf( "glXCreateContextAttribsARB() not found, using old-style GLX context\n" );
            ctx = glXCreateNewContext( dpy, bestFbc, GLX_RGBA_TYPE, 0, True );
        }
        else
        {
            // If it does, try to get a GL 3.0 context!
            int context_attribs[] =
            {
                GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
                GLX_CONTEXT_MINOR_VERSION_ARB, 3,
                //GLX_CONTEXT_FLAGS_ARB        , 0,
                None
            };

            ctx = glXCreateContextAttribsARB( dpy, bestFbc, 0, True, context_attribs );

            // Sync to ensure any errors generated are processed.
            XSync( dpy, False );
            if( !ctxErrorOccurred && ctx )
                printf( "Created GL 3.2 context\n" );
            else
            {
              // Couldn't create GL 3.0 context.  Fall back to old-style 2.x context.
              // When a context version below 3.0 is requested, implementations will
              // return the newest context version compatible with OpenGL versions less
              // than version 3.0.
              // GLX_CONTEXT_MAJOR_VERSION_ARB = 1
              context_attribs[1] = 1;
              // GLX_CONTEXT_MINOR_VERSION_ARB = 0
              context_attribs[3] = 0;

              ctxErrorOccurred = qfalse;

              printf( "Failed to create GL 3.2 context, using old-style GLX context\n" );
              ctx = glXCreateContextAttribsARB( dpy, bestFbc, 0, True, context_attribs );
            }
        }
          // Sync to ensure any errors generated are processed.
          XSync( dpy, False );

          // Restore the original error handler
          //XSetErrorHandler( oldHandler );

          if( ctxErrorOccurred || !ctx )
          {
            printf( "Failed to create an OpenGL context\n" );
            exit(1);
          }

          // Verifying that context is a direct context
          if( ! glXIsDirect ( dpy, ctx ) )
            printf( "Indirect GLX rendering context obtained\n" );
          else
            printf( "Direct GLX rendering context obtained\n" );

///////////////////////////
        /* GH: Don't let the window be resized */
        sizehints.flags = PMinSize | PMaxSize;
        sizehints.min_width = sizehints.max_width = actualWidth;
        sizehints.min_height = sizehints.max_height = actualHeight;

        XSetWMNormalHints(dpy, win, &sizehints);

        XMapWindow(dpy, win);

        wmDeleteEvent = XInternAtom( dpy, "WM_DELETE_WINDOW", True );

        if ( wmDeleteEvent == BadValue )
            wmDeleteEvent = None;
        if ( wmDeleteEvent != None )
            XSetWMProtocols( dpy, win, &wmDeleteEvent, 1 );

        window_created = qtrue;

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

///////////////////////////
    }
    else
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

        attrib[ATTR_DEPTH_IDX] = 24; // default to 24 depth
        attrib[ATTR_STENCIL_IDX] = 0;
        attrib[ATTR_RED_IDX] = 8;
        attrib[ATTR_GREEN_IDX] = 8;
        attrib[ATTR_BLUE_IDX] = 8;
        
        XVisualInfo* visinfo = glXChooseVisual( dpy, scrnum, attrib );
        if ( !visinfo )
        {
            printf( "Couldn't get a visual\n" );
        }

        printf( "Using %d/%d/%d Color bits, %d depth, %d stencil display.\n", 
            attrib[ATTR_RED_IDX], attrib[ATTR_GREEN_IDX], attrib[ATTR_BLUE_IDX],
            attrib[ATTR_DEPTH_IDX], attrib[ATTR_STENCIL_IDX]);

        window_width = actualWidth;
        window_height = actualHeight;


        /* window attributes */
        XSetWindowAttributes attr;

        attr.background_pixel = BlackPixel( dpy, scrnum );
        attr.border_pixel = 0;
        attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone );
        attr.event_mask = (KeyPressMask | KeyReleaseMask)
                | (ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ButtonMotionMask )
                | (VisibilityChangeMask | StructureNotifyMask | FocusChangeMask );

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

        win = XCreateWindow( dpy, root, 0, 0, actualWidth, actualHeight,
            0, visinfo->depth, InputOutput, visinfo->visual, mask, &attr );

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

        window_created = qtrue;

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
        ctx = glXCreateContext( dpy, visinfo, NULL, True );
        XSync( dpy, False );
        	/* GH: Free the visinfo after we're done with it */
	    XFree( visinfo );

    }


    printf( "Making context current\n" );
	glXMakeCurrent(dpy, win, ctx);

    glXSwapBuffers ( dpy, win );

	Key_ClearStates();

	XSetInputFocus( dpy, win, RevertToParent, CurrentTime );

	return RSERR_OK;
}


static qboolean GLW_StartDriverAndSetMode(int mode, qboolean fullscreen, glconfig_t* config, qboolean context)
{
	rserr_t err = GLW_SetMode(mode, fullscreen, config, context);

	switch ( err )
	{
        case RSERR_INVALID_FULLSCREEN:
            printf( "...WARNING: fullscreen unavailable in this mode\n" );
            return qfalse;

        case RSERR_INVALID_MODE:
            printf( "...WARNING: could not set the given mode (%d)\n", mode );
            return qfalse;

        default:
            break;
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
void GLimp_Init( glconfig_t *config, qboolean context )
{
    r_fullscreen = Cvar_Get( "r_fullscreen", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_mode = Cvar_Get( "r_mode", "-2", CVAR_ARCHIVE | CVAR_LATCH );
    
    r_drawBuffer = Cvar_Get( "r_drawBuffer", "GL_BACK", CVAR_CHEAT );
    
	r_customwidth = Cvar_Get( "r_customwidth", "1920", CVAR_ARCHIVE | CVAR_LATCH );
	r_customheight = Cvar_Get( "r_customheight", "1080", CVAR_ARCHIVE | CVAR_LATCH );
	r_glDriver = Cvar_Get( "r_glDriver", OPENGL_DRIVER_NAME, CVAR_ARCHIVE | CVAR_LATCH );
    
    
    vid_xpos = Cvar_Get( "vid_xpos", "3", CVAR_ARCHIVE );
	vid_ypos = Cvar_Get( "vid_ypos", "22", CVAR_ARCHIVE );
	setenv( "vblank_mode", "1", 1 );
    Cmd_AddCommand( "modelist", ModeList_f );


    IN_Init();   // rcg08312005 moved into glimp.
    //load opengl dll
	glw_state.OpenGLLib = Sys_LoadDll(r_glDriver->string, qtrue);

    
	// set up our custom error handler for X failures
	XSetErrorHandler( &qXErrorHandler );

	// feedback to renderer configuration
    // glw_state.config = config;
    qboolean fullscreen = (r_fullscreen->integer != 0);

	config->isFullscreen = fullscreen;
    

	
	// This values force the UI to disable driver selection
	config->driverType = GLDRV_ICD;
	config->hardwareType = GLHW_GENERIC;
    config->colorBits = 24;
    config->depthBits = 24;
    config->stencilBits = 0;
	// load the GL layer


    // create the window and set up the context
    if ( GLW_StartDriverAndSetMode(r_mode->integer, fullscreen, config, context) )
    {
        return;
    }
    else if ( r_mode->integer != 3 )
    {
        if ( GLW_StartDriverAndSetMode(3, fullscreen, config, context) )
        {
            return;
        }
    }
    else
    {
        GL_Shutdown( qtrue );
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
		glXSwapBuffers( dpy, win );
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

		RandR_RestoreGamma();

		RandR_RestoreMode();

		if ( ctx )
			glXDestroyContext( dpy, ctx );

		if ( win )
			XDestroyWindow( dpy, win );

		VidMode_RestoreGamma();

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
	
    if ( glw_state.cdsFullscreen )
	{
		glw_state.cdsFullscreen = qfalse;
	}

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


