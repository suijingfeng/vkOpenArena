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


#define QGL_LinX11_PROCS \
	GLE( XVisualInfo*, glXChooseVisual, Display *dpy, int screen, int *attribList ) \
	GLE( GLXContext, glXCreateContext, Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct ) \
	GLE( void, glXDestroyContext, Display *dpy, GLXContext ctx ) \
	GLE( Bool, glXMakeCurrent, Display *dpy, GLXDrawable drawable, GLXContext ctx) \
	GLE( void, glXSwapBuffers, Display *dpy, GLXDrawable drawable )


#define GLE( ret, name, ... ) ret ( APIENTRY * q##name )( __VA_ARGS__ );
    QGL_LinX11_PROCS;
#undef GLE


extern WinVars_t glw_state;
extern cvar_t* r_swapInterval;


static void * hGraphicLib; // instance of OpenGL library
static GLXContext ctx_gl;


void XSys_LoadOpenGL( void )
{
	if ( hGraphicLib == NULL )
	{
		hGraphicLib = dlopen( OPENGL_DRIVER_NAME, RTLD_NOW);
		if(hGraphicLib) {
			Com_Printf( " %s loaded. \n", OPENGL_DRIVER_NAME);
		}
		else
		{
			cvar_t* r_glDriver = Cvar_Get( "r_glDriver", "libGL.so", CVAR_ARCHIVE | CVAR_LATCH );
			
			dlopen( r_glDriver->string, RTLD_NOW);

			if ( hGraphicLib == NULL ) {
				Com_Error(ERR_FATAL, "Failed to load %s from /etc/ld.so.conf: %s\n",
					r_glDriver->string, dlerror());
			}
			else
			{
				Com_Printf( " %s loaded. \n", r_glDriver->string);
			}
		}
	}

	// expand constants before stringifying them
	// load the GLX funs
#define GLE( ret, name, ... ) \
	q##name = dlsym(hGraphicLib, #name ); \
	if ( !q##name ) { \
		Com_Error(ERR_FATAL, "Error resolving glx core functions\n"); \
	}
	
	QGL_LinX11_PROCS;
#undef GLE

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


void XSys_UnloadOpenGL(void)
{
	if ( hGraphicLib )
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

		dlclose( hGraphicLib );

		hGraphicLib = NULL;
	}
}


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


void XSys_CreateContextForGL( XVisualInfo * pVisinfo )
{
	// glXCreateContext fails to create a rendering context, NULL is returned. 
	// A thread is one of a set of subprocesses that share a single address space, 
	// but maintain separate program counters, stack spaces, and other related global data. 
	// A thread that is the only member of its subprocess group is equivalent to a process. 
	//
	// GLXContext glXCreateContext(Display * dpy,  XVisualInfo * vis,  GLXContext shareList,  Bool direct);
	// 
	// dpy : 
	//       Specifies the connection to the X server.
	// vis : 
	//       Specifies the visual that defines the frame buffer resources available to the rendering context.
	//       It is a pointer to an XVisualInfo structure, not a visual ID or a pointer to a Visual.
        // shareList : 
	//             Specifies the context with which to share display lists.
	//             NULL indicates that no sharing is to take place.
        //        
	// direct : 
        //            Specifies whether rendering is to be done with a direct connection
        //            to the graphics system if possible (True) or through the X server (False).
                
	ctx_gl = qglXCreateContext( glw_state.pDisplay, pVisinfo, NULL, True );
	if( ctx_gl ) {
		Com_Printf( " Context Created for GL. \n");
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
	
	qglXSwapBuffers( glw_state.pDisplay, glw_state.hWnd );

}

//////////////////////////////////////////////////////////////////////////////////////////////////
