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
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
/*
** QGL_WIN.C
**
** This file implements the operating system binding of GL to QGL function
** pointers.  When doing a port of Quake3 you must implement the following
** two functions:
**
*/
#include "tr_local.h"
#include "qgl.h"
#include "../qcommon/sys_loadlib.h"



static cvar_t* r_ext_texture_filter_anisotropic;
static cvar_t* r_ext_max_anisotropy;


void ( APIENTRY * qglAlphaFunc )(GLenum func, GLclampf ref);
void ( APIENTRY * qglBegin )(GLenum mode);
void ( APIENTRY * qglBindTexture )(GLenum target, GLuint texture);
void ( APIENTRY * qglBlendFunc )(GLenum sfactor, GLenum dfactor);
void ( APIENTRY * qglClear )(GLbitfield mask);
void ( APIENTRY * qglClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void ( APIENTRY * qglClipPlane )(GLenum plane, const GLdouble *equation);
void ( APIENTRY * qglColor3f )(GLfloat red, GLfloat green, GLfloat blue);
void ( APIENTRY * qglColorMask )(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void ( APIENTRY * qglColorPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglCullFace )(GLenum mode);
void ( APIENTRY * qglDeleteTextures )(GLsizei n, const GLuint *textures);
void ( APIENTRY * qglDepthFunc )(GLenum func);
void ( APIENTRY * qglDepthMask )(GLboolean flag);
void ( APIENTRY * qglDepthRange )(GLclampd zNear, GLclampd zFar);
void ( APIENTRY * qglDisable )(GLenum cap);
void ( APIENTRY * qglDisableClientState )(GLenum array);
void ( APIENTRY * qglDrawBuffer )(GLenum mode);
void ( APIENTRY * qglDrawElements )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void ( APIENTRY * qglEnable )(GLenum cap);
void ( APIENTRY * qglEnableClientState )(GLenum array);
void ( APIENTRY * qglEnd )(void);
void ( APIENTRY * qglFinish )(void);
GLenum ( APIENTRY * qglGetError )(void);
void ( APIENTRY * qglGetIntegerv )(GLenum pname, GLint *params);
const GLubyte * ( APIENTRY * qglGetString )(GLenum name);
void ( APIENTRY * qglLineWidth )(GLfloat width);
void ( APIENTRY * qglLoadIdentity )(void);
void ( APIENTRY * qglLoadMatrixf )(const GLfloat *m);
void ( APIENTRY * qglMatrixMode )(GLenum mode);
void ( APIENTRY * qglOrtho )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void ( APIENTRY * qglPolygonMode )(GLenum face, GLenum mode);
void ( APIENTRY * qglPolygonOffset )(GLfloat factor, GLfloat units);
void ( APIENTRY * qglPopMatrix )(void);
void ( APIENTRY * qglPushMatrix )(void);
void ( APIENTRY * qglReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void ( APIENTRY * qglScissor )(GLint x, GLint y, GLsizei width, GLsizei height);
void ( APIENTRY * qglStencilFunc )(GLenum func, GLint ref, GLuint mask);
void ( APIENTRY * qglStencilOp )(GLenum fail, GLenum zfail, GLenum zpass);
void ( APIENTRY * qglTexCoord2f )(GLfloat s, GLfloat t);
void ( APIENTRY * qglTexCoord2fv )(const GLfloat *v);
void ( APIENTRY * qglTexCoordPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglTexEnvf )(GLenum target, GLenum pname, GLfloat param);
void ( APIENTRY * qglTexImage2D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglTexParameterf )(GLenum target, GLenum pname, GLfloat param);
void ( APIENTRY * qglTexParameterfv )(GLenum target, GLenum pname, const GLfloat *params);
void ( APIENTRY * qglTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglVertex2f )(GLfloat x, GLfloat y);
void ( APIENTRY * qglVertex3f )(GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglVertex3fv )(const GLfloat *v);
void ( APIENTRY * qglVertexPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglViewport )(GLint x, GLint y, GLsizei width, GLsizei height);


void (APIENTRY * qglActiveTextureARB) (GLenum texture);
void (APIENTRY * qglClientActiveTextureARB) (GLenum texture);
void (APIENTRY * qglMultiTexCoord2fARB) (GLenum target, GLfloat s, GLfloat t);
void (APIENTRY * qglLockArraysEXT) (GLint first, GLsizei count);
void (APIENTRY * qglUnlockArraysEXT) (void);



// Placeholder functions to replace OpenGL calls when Vulkan renderer is active.

static void noglActiveTextureARB ( GLenum texture ) {}
static void noglClientActiveTextureARB( GLenum texture ) {}
static void noglMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t){}
static void noglLockArraysEXT (GLint first, GLsizei count) {}
static void noglUnlockArraysEXT (void) {}
static void noglAlphaFunc(GLenum func, GLclampf ref) {}
static void noglBegin(GLenum mode) {}
static void noglBindTexture(GLenum target, GLuint texture) {}
static void noglBlendFunc(GLenum sfactor, GLenum dfactor) {}
static void noglClear(GLbitfield mask) {}
static void noglClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {}
static void noglClipPlane(GLenum plane, const GLdouble *equation) {}
static void noglColor3f(GLfloat red, GLfloat green, GLfloat blue) {}
static void noglColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {}
static void noglColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {}
static void noglCullFace(GLenum mode) {}
static void noglDeleteTextures(GLsizei n, const GLuint *textures) {}
static void noglDepthFunc(GLenum func) {}
static void noglDepthMask(GLboolean flag) {}
static void noglDepthRange(GLclampd zNear, GLclampd zFar) {}
static void noglDisable(GLenum cap) {}
static void noglDisableClientState(GLenum array) {}
static void noglDrawBuffer(GLenum mode) {}
static void noglDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {}
static void noglEnable(GLenum cap) {}
static void noglEnableClientState(GLenum array) {}
static void noglEnd(void) {}
static void noglFinish(void) {}
static GLenum noglGetError(void) { return GL_NO_ERROR; }
static void noglGetIntegerv(GLenum pname, GLint *params) {}
static const GLubyte* noglGetString(GLenum name) { static char* s = ""; return (GLubyte*)s;}
static void noglLineWidth(GLfloat width) {}
static void noglLoadIdentity(void) {}
static void noglLoadMatrixf(const GLfloat *m) {}
static void noglMatrixMode(GLenum mode) {}
static void noglOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {}
static void noglPolygonMode(GLenum face, GLenum mode) {}
static void noglPolygonOffset(GLfloat factor, GLfloat units) {}
static void noglPopMatrix(void) {}
static void noglPushMatrix(void) {}
static void noglReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) {}
static void noglScissor(GLint x, GLint y, GLsizei width, GLsizei height) {}
static void noglStencilFunc(GLenum func, GLint ref, GLuint mask) {}
static void noglStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {}
static void noglTexCoord2f(GLfloat s, GLfloat t) {}
static void noglTexCoord2fv(const GLfloat *v) {}
static void noglTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {}
static void noglTexEnvf(GLenum target, GLenum pname, GLfloat param) {}
static void noglTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {}
static void noglTexParameterf(GLenum target, GLenum pname, GLfloat param) {}
static void noglTexParameterfv(GLenum target, GLenum pname, const GLfloat *params) {}
static void noglTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {}
static void noglVertex2f(GLfloat x, GLfloat y) {}
static void noglVertex3f(GLfloat x, GLfloat y, GLfloat z) {}
static void noglVertex3fv(const GLfloat *v) {}
static void noglVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {}
static void noglViewport(GLint x, GLint y, GLsizei width, GLsizei height) {}



/*
** qglShutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.  This
** is only called during a hard shutdown of the OGL subsystem (e.g. vid_restart).
*/
void qglShutdown( qboolean unloadDLL )
{
    ri.Printf( PRINT_ALL, "...shutting down QGL\n" );

	qglAlphaFunc                 = NULL;
	qglBegin                     = NULL;
	qglBindTexture               = NULL;
	qglBlendFunc                 = NULL;
	qglClear                     = NULL;
	qglClearColor                = NULL;
	qglClipPlane                 = NULL;
	qglColor3f                   = NULL;
	qglColorMask                 = NULL;
	qglColorPointer              = NULL;
	qglCullFace                  = NULL;
	qglDeleteTextures            = NULL;
	qglDepthFunc                 = NULL;
	qglDepthMask                 = NULL;
	qglDepthRange                = NULL;
	qglDisable                   = NULL;
	qglDisableClientState        = NULL;
	qglDrawBuffer                = NULL;
	qglDrawElements              = NULL;
	qglEnable                    = NULL;
	qglEnableClientState         = NULL;
	qglEnd                       = NULL;
	qglFinish                    = NULL;
	qglGetError                  = NULL;
	qglGetIntegerv               = NULL;
	qglGetString                 = NULL;
	qglLineWidth                 = NULL;
	qglLoadIdentity              = NULL;
	qglLoadMatrixf               = NULL;
	qglMatrixMode                = NULL;
	qglOrtho                     = NULL;
	qglPolygonMode               = NULL;
	qglPolygonOffset             = NULL;
	qglPopMatrix                 = NULL;
	qglPushMatrix                = NULL;
	qglReadPixels                = NULL;
	qglScissor                   = NULL;
	qglStencilFunc               = NULL;
	qglStencilOp                 = NULL;
	qglTexCoord2f                = NULL;
	qglTexCoord2fv               = NULL;
	qglTexCoordPointer           = NULL;
	qglTexEnvf                   = NULL;
	qglTexImage2D                = NULL;
	qglTexParameterf             = NULL;
	qglTexParameterfv            = NULL;
	qglTexSubImage2D             = NULL;
	qglVertex2f                  = NULL;
	qglVertex3f                  = NULL;
	qglVertex3fv                 = NULL;
	qglVertexPointer             = NULL;
	qglViewport                  = NULL;
}



static qboolean GLimp_HaveExtension(const char *ext)
{
	const char *ptr = Q_stristr( glConfig.extensions_string, ext );
	if (ptr == NULL)
		return qfalse;
	ptr += strlen(ext);
	return ((*ptr == ' ') || (*ptr == '\0'));  // verify it's complete string.
}


/*
** This is responsible for binding our qgl function pointers to 
** the appropriate GL stuff.  In Windows this means doing a 
** LoadLibrary and a bunch of calls to GetProcAddress.  On other
** operating systems we need to do the right thing, whatever that
** might be.
*/
qboolean qglInit( void )
{

    ri.Printf( PRINT_ALL, "...initializing QGL\n" );
/*
    const char *dllname = OPENGL_DRIVER_NAME;
	if ( hinstOpenGL == NULL )
	{
		hinstOpenGL = Sys_LoadLibrary( dllname );
		ri.Printf(PRINT_ALL, "Loading %s ... ", dllname);

        if ( hinstOpenGL == NULL )
		{
			ri.Error(ERR_FATAL, "LoadOpenGLDll: failed to load %s from /etc/ld.so.conf: %s\n", dllname, Sys_LibraryError());
		    return qfalse;
        }
        
        ri.Cvar_Set( "r_glDriver", dllname );

        ri.Printf( PRINT_ALL, "succeeded\n" );
	}
*/

	qglAlphaFunc            = GLimp_GetProcAddress("glAlphaFunc");
	qglBegin                = GLimp_GetProcAddress("glBegin");
	qglBindTexture          = GLimp_GetProcAddress("glBindTexture");
	qglBlendFunc            = GLimp_GetProcAddress("glBlendFunc");
	qglClear                = GLimp_GetProcAddress("glClear");
	qglClearColor           = GLimp_GetProcAddress("glClearColor");
	qglClipPlane            = GLimp_GetProcAddress("glClipPlane");
	qglColor3f              = GLimp_GetProcAddress("glColor3f");
	qglColorMask            = GLimp_GetProcAddress("glColorMask");
	qglColorPointer         = GLimp_GetProcAddress("glColorPointer");
	qglCullFace             = GLimp_GetProcAddress("glCullFace");
	qglDeleteTextures       = GLimp_GetProcAddress("glDeleteTextures");
	qglDepthFunc            = GLimp_GetProcAddress("glDepthFunc");
	qglDepthMask            = GLimp_GetProcAddress("glDepthMask");
	qglDepthRange           = GLimp_GetProcAddress("glDepthRange");
	qglDisable              = GLimp_GetProcAddress("glDisable");
	qglDisableClientState   = GLimp_GetProcAddress("glDisableClientState");
	qglDrawBuffer           = GLimp_GetProcAddress("glDrawBuffer");
	qglDrawElements         = GLimp_GetProcAddress("glDrawElements");
	qglEnable               = GLimp_GetProcAddress("glEnable");
	qglEnableClientState    = GLimp_GetProcAddress("glEnableClientState");
	qglEnd                  = GLimp_GetProcAddress("glEnd");
	qglFinish               = GLimp_GetProcAddress("glFinish");
	qglGetError             = GLimp_GetProcAddress("glGetError");
	qglGetIntegerv          = GLimp_GetProcAddress("glGetIntegerv");
	qglGetString            = GLimp_GetProcAddress("glGetString");
	qglLineWidth            = GLimp_GetProcAddress("glLineWidth");
	qglLoadIdentity         = GLimp_GetProcAddress("glLoadIdentity");
	qglLoadMatrixf          = GLimp_GetProcAddress("glLoadMatrixf");
	qglMatrixMode           = GLimp_GetProcAddress("glMatrixMode");
	qglOrtho                = GLimp_GetProcAddress("glOrtho");
	qglPolygonMode          = GLimp_GetProcAddress("glPolygonMode");
	qglPolygonOffset        = GLimp_GetProcAddress("glPolygonOffset");
	qglPopMatrix            = GLimp_GetProcAddress("glPopMatrix");
	qglPushMatrix           = GLimp_GetProcAddress("glPushMatrix");
	qglReadPixels           = GLimp_GetProcAddress("glReadPixels");
	qglScissor              = GLimp_GetProcAddress("glScissor");
	qglStencilFunc          = GLimp_GetProcAddress("glStencilFunc");
	qglStencilOp            = GLimp_GetProcAddress("glStencilOp");
	qglTexCoord2f           = GLimp_GetProcAddress("glTexCoord2f");
	qglTexCoord2fv          = GLimp_GetProcAddress("glTexCoord2fv");
	qglTexCoordPointer      = GLimp_GetProcAddress("glTexCoordPointer");
	qglTexEnvf              = GLimp_GetProcAddress("glTexEnvf");
	qglTexImage2D           = GLimp_GetProcAddress("glTexImage2D");
	qglTexParameterf        = GLimp_GetProcAddress("glTexParameterf");
	qglTexParameterfv       = GLimp_GetProcAddress("glTexParameterfv");
	qglTexSubImage2D        = GLimp_GetProcAddress("glTexSubImage2D");
	qglVertex2f             = GLimp_GetProcAddress("glVertex2f");
	qglVertex3f             = GLimp_GetProcAddress("glVertex3f");
	qglVertex3fv            = GLimp_GetProcAddress("glVertex3fv");
	qglVertexPointer        = GLimp_GetProcAddress("glVertexPointer");
	qglViewport             = GLimp_GetProcAddress("glViewport");


	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;
    qglMultiTexCoord2fARB = NULL;
	qglLockArraysEXT = NULL;
	qglUnlockArraysEXT = NULL;

	ri.Printf( PRINT_ALL,  "\n...Initializing OpenGL extensions\n" );


    Q_strncpyz( glConfig.vendor_string, (char *) qglGetString (GL_VENDOR), sizeof( glConfig.vendor_string ) );
    Q_strncpyz( glConfig.renderer_string, (char *) qglGetString (GL_RENDERER), sizeof( glConfig.renderer_string ) );
    if (*glConfig.renderer_string && glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] == '\n')
        glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] = 0;
    Q_strncpyz( glConfig.version_string, (char *) qglGetString (GL_VERSION), sizeof( glConfig.version_string ) );
    Q_strncpyz( glConfig.extensions_string, (char *)qglGetString(GL_EXTENSIONS), sizeof( glConfig.extensions_string ) );

	r_ext_max_anisotropy = ri.Cvar_Get( "r_ext_max_anisotropy", "2", CVAR_ARCHIVE | CVAR_LATCH );
    r_ext_texture_filter_anisotropic = ri.Cvar_Get( "r_ext_texture_filter_anisotropic", "0", CVAR_ARCHIVE | CVAR_LATCH );

	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qfalse;
	if ( GLimp_HaveExtension( "EXT_texture_env_add" ) )
	{
		if ( GLimp_HaveExtension( "GL_EXT_texture_env_add" ) )
		{
			glConfig.textureEnvAddAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_env_add\n" );
		}
		else
		{
			glConfig.textureEnvAddAvailable = qfalse;
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_env_add\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_env_add not found\n" );
	}

	// GL_ARB_multitexture
	qglMultiTexCoord2fARB = NULL;
	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;
	if ( GLimp_HaveExtension( "GL_ARB_multitexture" ) )
	{
		qglMultiTexCoord2fARB = (void ( APIENTRY * ) (GLenum, GLfloat, GLfloat)) GLimp_GetProcAddress( "glMultiTexCoord2fARB" );
		qglActiveTextureARB = (void ( APIENTRY * ) (GLenum )) GLimp_GetProcAddress( "glActiveTextureARB" );
		qglClientActiveTextureARB = (void ( APIENTRY * ) (GLenum )) GLimp_GetProcAddress( "glClientActiveTextureARB" );

		if ( qglActiveTextureARB )
		{
			GLint glint = 0;
			qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &glint );
			glConfig.numTextureUnits = (int) glint;
			if ( glConfig.numTextureUnits > 1 )
			{
				ri.Printf( PRINT_ALL, "...using GL_ARB_multitexture\n" );
			}
			else
			{
				qglMultiTexCoord2fARB = NULL;
				qglActiveTextureARB = NULL;
				qglClientActiveTextureARB = NULL;
				ri.Printf( PRINT_ALL, "...not using GL_ARB_multitexture, < 2 texture units\n" );
			}
		}
	}
	else
	{
		ri.Printf( PRINT_ALL,  "...GL_ARB_multitexture not found\n" );
	}

	// GL_EXT_compiled_vertex_array
	if ( GLimp_HaveExtension( "GL_EXT_compiled_vertex_array" ) )
	{
		ri.Printf( PRINT_ALL,  "...using GL_EXT_compiled_vertex_array\n" );
		qglLockArraysEXT = ( void ( APIENTRY * )( GLint, GLint ) ) GLimp_GetProcAddress( "glLockArraysEXT" );
		qglUnlockArraysEXT = ( void ( APIENTRY * )( void ) ) GLimp_GetProcAddress( "glUnlockArraysEXT" );
		if (!qglLockArraysEXT || !qglUnlockArraysEXT)
		{
			ri.Error(ERR_FATAL, "bad getprocaddress");
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n" );
	}

	if ( GLimp_HaveExtension( "GL_EXT_texture_filter_anisotropic" ) )
	{
		if ( r_ext_texture_filter_anisotropic->integer )
        {
            int maxAnisotropy = 0;
            char target_string[4] = {0};

            #ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
            #define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
            #endif
			qglGetIntegerv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, (GLint *)&maxAnisotropy );
			if ( maxAnisotropy <= 0 ) {
				ri.Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not properly supported!\n" );
				maxAnisotropy = 0;
			}
			else
			{
                sprintf(target_string, "%d", maxAnisotropy);
				ri.Printf( PRINT_ALL,  "...using GL_EXT_texture_filter_anisotropic (max: %i)\n", maxAnisotropy );
                ri.Cvar_Set( "r_ext_max_anisotropy", target_string);

			}
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not found\n" );
	}

	return qtrue;
}


void qglDumb(void)
{
	ri.Printf( PRINT_ALL, "...qglDumb()... \n" );


#define QGL_DUMB( a )       (void*)(&no ## a)
    qglAlphaFunc            = QGL_DUMB(glAlphaFunc);
    qglBegin                = QGL_DUMB(glBegin);
    qglBindTexture          = QGL_DUMB(glBindTexture);
    qglBlendFunc            = QGL_DUMB(glBlendFunc);
    qglClear                = QGL_DUMB(glClear);
    qglClearColor           = QGL_DUMB(glClearColor);
    qglClipPlane            = QGL_DUMB(glClipPlane);
    qglColor3f              = QGL_DUMB(glColor3f);
    qglColorMask            = QGL_DUMB(glColorMask);
    qglColorPointer         = QGL_DUMB(glColorPointer);
    qglCullFace             = QGL_DUMB(glCullFace);
    qglDeleteTextures       = QGL_DUMB(glDeleteTextures);
    qglDepthFunc            = QGL_DUMB(glDepthFunc);
    qglDepthMask            = QGL_DUMB(glDepthMask);
    qglDepthRange           = QGL_DUMB(glDepthRange);
    qglDisable              = QGL_DUMB(glDisable);
    qglDisableClientState   = QGL_DUMB(glDisableClientState);
    qglDrawBuffer           = QGL_DUMB(glDrawBuffer);
    qglDrawElements         = QGL_DUMB(glDrawElements);
    qglEnable               = QGL_DUMB(glEnable);
    qglEnableClientState    = QGL_DUMB(glEnableClientState);
    qglEnd                  = QGL_DUMB(glEnd);
    qglFinish               = QGL_DUMB(glFinish);
    qglGetError             = QGL_DUMB(glGetError);
    qglGetIntegerv          = QGL_DUMB(glGetIntegerv);
    qglGetString            = QGL_DUMB(glGetString);
    qglLineWidth            = QGL_DUMB(glLineWidth);
    qglLoadIdentity         = QGL_DUMB(glLoadIdentity);
    qglLoadMatrixf          = QGL_DUMB(glLoadMatrixf);
    qglMatrixMode           = QGL_DUMB(glMatrixMode);
    qglOrtho                = QGL_DUMB(glOrtho);
    qglPolygonMode          = QGL_DUMB(glPolygonMode);
    qglPolygonOffset        = QGL_DUMB(glPolygonOffset);
    qglPopMatrix            = QGL_DUMB(glPopMatrix);
    qglPushMatrix           = QGL_DUMB(glPushMatrix);
    qglReadPixels           = QGL_DUMB(glReadPixels);
    qglScissor              = QGL_DUMB(glScissor);
    qglStencilFunc          = QGL_DUMB(glStencilFunc);
    qglStencilOp            = QGL_DUMB(glStencilOp);
    qglTexCoord2f           = QGL_DUMB(glTexCoord2f);
    qglTexCoord2fv          = QGL_DUMB(glTexCoord2fv);
    qglTexCoordPointer      = QGL_DUMB(glTexCoordPointer);
    qglTexEnvf              = QGL_DUMB(glTexEnvf);
    qglTexImage2D           = QGL_DUMB(glTexImage2D);
    qglTexParameterf        = QGL_DUMB(glTexParameterf);
    qglTexParameterfv       = QGL_DUMB(glTexParameterfv);
    qglTexSubImage2D        = QGL_DUMB(glTexSubImage2D);
    qglVertex2f             = QGL_DUMB(glVertex2f);
    qglVertex3f             = QGL_DUMB(glVertex3f);
    qglVertex3fv            = QGL_DUMB(glVertex3fv);
    qglVertexPointer        = QGL_DUMB(glVertexPointer);
    qglViewport             = QGL_DUMB(glViewport);

    qglLockArraysEXT        = QGL_DUMB(glLockArraysEXT);
    qglUnlockArraysEXT      = QGL_DUMB(glUnlockArraysEXT);
    qglMultiTexCoord2fARB   = QGL_DUMB(glMultiTexCoord2fARB);
    qglActiveTextureARB     = QGL_DUMB(glActiveTextureARB);
    qglClientActiveTextureARB = QGL_DUMB(glClientActiveTextureARB);
#undef QGL_DUMB
}
