/*
** Internal function that that attempts to load and use a specific OpenGL DLL.
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


A non-NULL return value for glXGetProcAddress does not guarantee that an
extension function is actually supported at runtime. The client must also query
glGetString( GL EXTENSIONS ) or glXQueryExtensionsString to determine if an
extension is supported by a particular context.

GL function pointers returned by glXGetProcAddress are independent of the
currently bound context and may be used by any context which supports the exten-
sion.

glXGetProcAddress may be queried for all of the following functions:

 * All GL and GLX extension functions supported by the implementation
 * (whether those extensions are supported by the current context or not).

 * All core (non-extension) functions in GL and GLX from version 1.0 
 * up to and including the versions of those specifications supported
 * by the implementation, as determined by glGetString( GL VERSION ) 
 * and glXQueryVersion queries.

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

** In the X Window System, OpenGL rendering is made available as an extension 
** to X in the formal X sense: connection and authentication are accomplished
** with the normal X mechanisms. As with other X extensions, there is a defined
** network protocol for the OpenGL rendering commands encapsulated within the X
** byte stream.
**
**
** Allowing for parallel rendering has affected the design of the GLX interface. 
** This has resulted in an added burden on the client to explicitly prevent 
** parallel execution when such execution is inappropriate

** The OpenGL specification is intentionally vague on how a rendering context
** (an abstract OpenGL state machine) is created. One of the purposes of GLX 
** is to provide a means to create an OpenGL context and associate it with a 
** drawing surface.


In X, a rendering surface is called a Drawable. X provides two types of Drawables: 
Windows which are located onscreen and Pixmaps which are maintained offscreen.


The GLX equivalent to a Window is a GLXWindow and the GLX equivalent to a Pixmap 
is a GLXPixmap. GLX introduces a third type of drawable, called a GLXPbuffer, 
for which there is no X equivalent. GLXPbuffers are used for offscreen rendering 
but they have different semantics than GLXPixmaps that make it easier to allocate 
them in non-visible frame buffer memory.


GLXWindows, GLXPixmaps and GLXPbuffers are created with respect to a GLXFBConfig; 
the GLXFBConfig describes the depth of the color buffer components and the types, 
quantities and sizes of the ancillary buffers (i.e., the depth, accumulation, 
auxiliary, multisample, and stencil buffers). Double buffering and stereo 
capability is also fixed by the GLXFBConfig.


Ancillary buffers are associated with a GLXDrawable, not with a rendering context. 
If several rendering contexts are all writing to the same window, they will share 
those buffers. Rendering operations to one window never affect the unobscured 
pixels of another window, or the corresponding pixels of ancillary buffers of that 
window. 


If an Expose event is received by the client, the values in the ancillary buffers 
and in the back buffers for regions corresponding to the exposed region become 
undefined.


A rendering context can be used with any GLXDrawable that it is compatible with 
(subject to the restrictions discussed in the section on address space and
the restrictions discussed under glXCreatePixmap). A drawable and context are
compatible if they.


* support the same type of rendering (e.g., RGBA or color index)

* have color buffers and ancillary buffers of the same depth. 

For example, a GLXDrawable that has a front left buffer and a back left buffer 
with red, green and blue sizes of 4 would not be compatible with a context that
was created with a visual or GLXFBConfig that has only a front left buffer with 
red, green and blue sizes of 8. However, it would be compatible with a context 
that was created with a GLXFBConfig that has only a front left buffer if the 
red, green and blue sizes are 4.

* were created with respect to the same X screen

As long as the compatibility constraint is satisfied (and the address space 
requirement is satisfied), applications can render into the same GLXDrawable, 
using different rendering contexts. It is also possible to use a single context 
to render into multiple GLXDrawables.

For backwards compatibility with GLX versions 1.2 and earlier, a rendering
context can also be used to render into a Window. Thus, a GLXDrawable is the
union {GLXWindow, GLXPixmap, GLXPbuffer, Window}. 

In X, Windows are associated with a Visual. In GLX the definition of Visual 
has been extended to include the types, quantities and sizes of the ancillary 
buffers and information indicating whether or not the Visual is double buffered. 

For backwards compatibility, a GLXPixmap can also be created using a Visual.

======================== Using Rendering Contexts ===========================


OpenGL defines both client state and server state. Thus a rendering context 
consists of two parts: one to hold the client state and one to hold the server 
state.

Each thread can have at most one current rendering context. In addition, 
a rendering context can be current for only one thread at a time. 
The client is responsible for creating a rendering context and a drawable.

Issuing OpenGL commands may cause the X buffer to be flushed. In particular,
calling glFlush when indirect rendering is occurring, will flush both the X and
OpenGL rendering streams.

Some state is shared between the OpenGL and X. The pixel values in the X
frame buffer are shared. The X double buffer extension (DBE) has a definition
for which buffer is currently the displayed buffer. This information is shared 
with GLX. The state of which buffer is displayed tracks in both extensions, 
independent of which extension initiates a buffer swap.


========================= Direct Rendering and Address Spaces ===============

One of the basic assumptions of the X protocol is that if a client can name
an object, then it can manipulate that object. GLX introduces the notion of
an Address Space. A GLX object cannot be used outside of the address space
in which it exists.

In a classic UNIX environment, each process is in its own address space.
In a multi-threaded environment, each of the threads will share a virtual
address space which references a common data region.

An OpenGL client that is rendering to a graphics engine directly connected
to the executing CPU may avoid passing the tokens through the X server.
This generalization is made for performance reasons. The model described
here specifically allows for such optimizations, but does not mandate that
any implementation support it.

When direct rendering is occurring, the address space of the OpenGL
implementation is that of the direct process; when direct rendering
is not being used (i.e., when indirect rendering is occurring), the
address space of the OpenGL implementation is that of the X server.

The client has the ability to reject the use of direct rendering,
but there may be a performance penalty in doing so.

In order to use direct rendering, a client must create a direct rendering context.
Both the client context state and the server context state of a direct rendering
context exist in the client¡¯s address space; this state cannot be shared by a client
in another process.

With indirect rendering contexts, the client context state is kept in the client's
address space and the server context state is kept in the address space of the X server.
In this case the server context state is stored in an X resource; it has an associated
XID and may potentially be used by another client process.

Although direct rendering support is optional, all implementations are required
to support indirect rendering.

*/

#include <unistd.h>

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


#define QGL_X11_PROCS \
    GLE( XVisualInfo*, glXChooseVisual, Display *dpy, int screen, int *attribList ) \
    GLE( GLXContext, glXCreateContext, Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct ) \
    GLE( void, glXDestroyContext, Display *dpy, GLXContext ctx ) \
    GLE( Bool, glXMakeCurrent, Display *dpy, GLXDrawable drawable, GLXContext ctx) \
    GLE( void, glXSwapBuffers, Display *dpy, GLXDrawable drawable ) \
    GLE( Bool, glXQueryExtension, Display * dpy, int * error_base, int * event_base) \
    GLE( Bool, glXQueryVersion, Display * dpy, int * major, int * minor) \
    GLE( const char *, glXQueryExtensionsString, Display * dpy, int screen) \
    GLE( const char *, glXGetClientString, Display * dpy, int name) \
    GLE( const char *, glXQueryServerString, Display * dpy, int screen, int name) \


#define GLE( ret, name, ... ) ret ( APIENTRY * q##name )( __VA_ARGS__ );
    QGL_X11_PROCS;
#undef GLE


extern WinVars_t glw_state;
extern cvar_t* r_swapInterval;

static GLXContext ctx_gl;


void XSys_LoadOpenGL(struct WinData_s * const pWinSys)
{
    void *pGLib; // instance of OpenGL library

    pGLib = dlopen(OPENGL_DRIVER_NAME, RTLD_NOW);
    if (pGLib)
    {
        Com_Printf("%s loaded.\n", OPENGL_DRIVER_NAME);
    }
    else
    {
        cvar_t* r_glDriver = Cvar_Get("r_glDriver", "libGL.so", CVAR_ARCHIVE | CVAR_LATCH);

        pGLib = dlopen(r_glDriver->string, RTLD_NOW);

        if (pGLib == NULL)
        {
            Com_Error(ERR_FATAL, "Failed to load %s from /etc/ld.so.conf: %s\n",
                                 r_glDriver->string, dlerror());
        }
        else
        {
            Com_Printf("%s loaded.\n", r_glDriver->string);
        }
    }

    // Save a copy of that handle for free
    pWinSys->hLibGL = pGLib;

    // expand constants before stringifying them
    // load the GLX funs
#define GLE( ret, name, ... ) \
        q##name = dlsym(pGLib, #name ); \
        if ( !q##name ) { \
            Com_Error(ERR_FATAL, "Error resolving glx core functions\n"); \
        }

        QGL_X11_PROCS;
#undef GLE
}


void XSys_UnloadOpenGL(struct WinData_s * const pWinSys)
{
    if (pWinSys->hLibGL)
    {
        Com_Printf( "Unloading libgl.so ...\n" );

        dlclose( pWinSys->hLibGL );

        pWinSys->hLibGL = NULL;
    }

#define GLE( ret, name, ... ) \
    q##name = NULL;

    QGL_X11_PROCS;
#undef GLE
}


///////////////////////////////////////////////////////////////////////////////

void GLX_AscertainExtension( Display * pDpy )
{
    int error_base;
    int event_base;

    // To ascertain if the GLX extension is defined for an X server
    Bool ret = qglXQueryExtension(pDpy, &error_base, &event_base);
    if( ret == False )
    {
        Com_Error(ERR_FATAL, " GLX extension is not is defined for this X server: %d, %d \n",
            error_base, event_base);
    }
    else
    {
        Com_Printf(" GLX extension defined. \n");
    }
}


void GLX_QueryVersion( Display * pDpy )
{
    // Upon success, major and minor are filled in with 
    // the major and minor versions of the extension implementation.
    int major;
    int minor;

    qglXQueryVersion(pDpy, &major, &minor);

    Com_Printf(" GLX Version: %d.%d. \n", major, minor);
}


const char * GLX_QueryExtensionsString( Display * pDpy, int screen)
{
    const char * ret = qglXQueryExtensionsString( pDpy, screen);

    Com_Printf( " GLX Extensions String on screen %d:\n%s\n\n", screen, ret);

    return ret;
}


const char * GLX_GetVendorString(Display * pDpy)
{
    const char * ret = qglXGetClientString(pDpy, GLX_VENDOR);

    Com_Printf( " GLX Vendor String: %s \n", ret);

    return ret;
}

const char * GLX_GetVersionString(Display * pDpy)
{
    const char * ret = qglXGetClientString(pDpy, GLX_VERSION);

    Com_Printf( " GLX Version String: %s \n", ret);

    return ret;
}

const char * GLX_QueryServerString( Display * pDpy, int screen)
{
    const char * ret1 = qglXQueryServerString( pDpy, screen, GLX_VENDOR);
    const char * ret2 = qglXQueryServerString( pDpy, screen, GLX_VERSION);
    const char * ret3 = qglXQueryServerString( pDpy, screen, GLX_EXTENSIONS);
    Com_Printf( " ================================================== \n" );
    Com_Printf( " GLX Server String on screen %d:\n %s \n %s \n %s\n\n",
            screen, ret1, ret2, ret3);
    Com_Printf( " ================================================== \n" );

    return ret1;
}

/*
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
*/





XVisualInfo * GetXVisualPtrWrapper(void)
{
	// Choose a visual. If you intend to use RGBA mode, specify RGBA in the attribute list when calling glXChooseVisual(). 
	// First decide whether your program will use RGBA or color-index mode. Some operations, 
	// such as texturing and blending, are not supported in color index mode; others, such as lighting, work differently in the two modes. 
	// Because of that, RGBA rendering is usually the right choice. 
	// Remember that RGBA is usually the right choice for OpenGL on a Silicon Graphics system.
	// OpenGL 1.0 and 1.1 and GLX 1.0, 1.1, and 1.2 require an RGBA mode program to use a TrueColor or DirectColor visual, 
	// and require a color index mode program to use a PseudoColor or StaticColor visual. 
	
	XVisualInfo * pRet = NULL;
	int attrib[] =
	{
		GLX_RGBA,         	// 0
		GLX_RED_SIZE, 8,	// 1, 2
		GLX_GREEN_SIZE, 8,	// 3, 4
		GLX_BLUE_SIZE, 8,	// 5, 6
		GLX_ALPHA_SIZE, 8,	// 7, 8
		GLX_DEPTH_SIZE, 24,	// 9, 10
		GLX_STENCIL_SIZE, 8,	// 11, 12
		GLX_DOUBLEBUFFER, True,	// 13, 14
		None
	};

	// GLX_RGBA: 
	// If present, only TrueColor and DirectColor visuals are considered. 
	// Otherwise, only PseudoColor and StaticColor visuals are considered. 
	// If RGBA is not specified in the attribute list, glXChooseVisual() selects a PseudoColor visual to 
	// support color index mode (or a StaticColor visual if no PseudoColor visual is available).
	// Many OpenGL applications use a 24-bit TrueColor visual (by specifying GLX_RGBA in the visual attribute list when choosing a visual). 

	// XVisualInfo *glXChooseVisual(Display* dpy, int screen, int * attribList)
	// dpy: Specifies the connection to the X server.
	// screen: Specifies the screen number.
	// attribList:  Specifies a list of boolean attributes and integer attribute/value pairs. 
	// The last attribute must be None. 
	//
	// glXChooseVisual returns a pointer to an XVisualInfo structure describing 
	// the visual that best meets a minimum specification. 
	// The boolean GLX attributes of the visual that is returned will match the specified values, 
	// and the integer GLX attributes will meet or exceed the specified minimum values. 
	// If all other attributes are equivalent, then TrueColor and PseudoColor visuals have priority 
	// over DirectColor and StaticColor visuals, respectively. If no conforming visual exists, NULL is returned. 
	// To free the data returned by this function, use XFree. 
	//
	// All boolean GLX attributes default to False except GLX_USE_GL, which defaults to True. 
	// All integer GLX attributes default to zero. Default specifications are superseded by attributes included in attribList. 
	// Boolean attributes included in attribList are understood to be True. 
	// Integer attributes and enumerated type attributes are followed immediately by the corresponding desired or minimum value. 
	// The list must be terminated with None.

	// OpenGL case
	// glXChooseVisual is implemented as a client-side utility using only XGetVisualInfo and glXGetConfig.
	// Calls to these two routines can be used to implement selection algorithms other than the generic one implemented by glXChooseVisual. 
	pRet = qglXChooseVisual( glw_state.pDisplay, glw_state.screenIdx, attrib );

	if ( pRet )
	{
		Com_Printf( " Choose visual Using %d/%d/%d Color bits, %d depth, %d stencil display.\n", 
				attrib[2], attrib[4], attrib[6], attrib[10], attrib[12]);
		// then, Create a colormap that can be used with the selected visual. 
		return pRet;
	}
	else
	{
		Com_Printf( "Couldn't get a visual. \n" );
	}
	return NULL;
}


void XSys_CreateContextForGL(XVisualInfo * pVisinfo)
{
	// glXCreateContext fails to create a rendering context, NULL is
	// returned. A thread is one of a set of subprocesses that share
	// a single address space, but maintain separate program counters,
	// stack spaces, and other related global data. A thread that is
	// the only member of its subprocess group is equivalent to a process.
	//
	// GLXContext glXCreateContext(Display * dpy,
	//                             XVisualInfo * vis,
	//                             GLXContext shareList,
	//                             Bool direct);
	//
	// dpy :
	//       Specifies the connection to the X server.
	// vis :
	//       Specifies the visual that defines the frame buffer resources
	//       available to the rendering context. It is a pointer to an
	//       XVisualInfo structure, not a visual ID or a pointer to a Visual.
        // shareList :
	//       Specifies the context with which to share display lists.
	//       NULL indicates that no sharing is to take place.
	// direct :
	//       Specifies whether rendering is to be done with a direct connection
	//       to the graphics system if possible (True) or through the X server (False).

	ctx_gl = qglXCreateContext(glw_state.pDisplay, pVisinfo, NULL, True);
	if( ctx_gl )
	{
		Com_Printf("Context Created for GL.\n");
	}
}

void XSys_SetCurrentContextForGL(void)
{
	qglXMakeCurrent( glw_state.pDisplay, glw_state.hWnd, ctx_gl );
}

void XSys_ClearCurrentContextForGL(void)
{
	if( ctx_gl != NULL )
	{
		qglXDestroyContext( glw_state.pDisplay, ctx_gl );
		ctx_gl = 0;
	}
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
    // dont compare every frame, just draw to back buffer
    // what's the bad doing this ?
    // if ( Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) != 0 )

    // For drawables that are double buffered, the contents of the back buffer 
    // can be made potentially visible ( become the contents of the front buffer) 
    // by calling glXSwapBuffers.
    //
    // glXSwapBuffers performs an implicit glFlush. Subsequent OpenGL commands 
    // can be issued immediately, but will not be executed until the buffer 
    // swapping has completed, typically during vertical retrace of the display 
    // monitor.
    //
    qglXSwapBuffers( glw_state.pDisplay, glw_state.hWnd );
}

//////////////////////////////////////////////////////////////////////////////////////////////////
