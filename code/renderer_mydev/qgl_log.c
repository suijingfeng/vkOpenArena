#include <stdio.h>
#include <stdlib.h>
#include "../qcommon/q_shared.h"
#include "qgl.h"
static FILE* log_fp;

/*
	if (dllname != NULL) {
		// check logging
		QGL_EnableLogging( (qboolean) r_logFile->integer );
	}
*/

const GLubyte * ( APIENTRY * dllGetString )(GLenum name);
GLenum ( APIENTRY * dllGetError )(void);

static void ( APIENTRY * dllAlphaFunc )(GLenum func, GLclampf ref);
static void ( APIENTRY * dllBegin )(GLenum mode);
static void ( APIENTRY * dllBindTexture )(GLenum target, GLuint texture);
static void ( APIENTRY * dllBlendFunc )(GLenum sfactor, GLenum dfactor);
static void ( APIENTRY * dllClear )(GLbitfield mask);
static void ( APIENTRY * dllClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
static void ( APIENTRY * dllClipPlane )(GLenum plane, const GLdouble *equation);
static void ( APIENTRY * dllColor3f )(GLfloat red, GLfloat green, GLfloat blue);
static void ( APIENTRY * dllColorMask )(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
static void ( APIENTRY * dllColorPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllCullFace )(GLenum mode);
static void ( APIENTRY * dllDeleteTextures )(GLsizei n, const GLuint *textures);
static void ( APIENTRY * dllDepthFunc )(GLenum func);
static void ( APIENTRY * dllDepthMask )(GLboolean flag);
static void ( APIENTRY * dllDepthRange )(GLclampd zNear, GLclampd zFar);
static void ( APIENTRY * dllDisable )(GLenum cap);
static void ( APIENTRY * dllDisableClientState )(GLenum array);
static void ( APIENTRY * dllDrawBuffer )(GLenum mode);
static void ( APIENTRY * dllDrawElements )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
static void ( APIENTRY * dllEnable )(GLenum cap);
static void ( APIENTRY * dllEnableClientState )(GLenum array);
static void ( APIENTRY * dllEnd )(void);
static void ( APIENTRY * dllFinish )(void);
static void ( APIENTRY * dllGetIntegerv )(GLenum pname, GLint *params);
static void ( APIENTRY * dllLineWidth )(GLfloat width);
static void ( APIENTRY * dllLoadIdentity )(void);
static void ( APIENTRY * dllLoadMatrixf )(const GLfloat *m);
static void ( APIENTRY * dllMatrixMode )(GLenum mode);
static void ( APIENTRY * dllOrtho )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
static void ( APIENTRY * dllPolygonMode )(GLenum face, GLenum mode);
static void ( APIENTRY * dllPolygonOffset )(GLfloat factor, GLfloat units);
static void ( APIENTRY * dllPopMatrix )(void);
static void ( APIENTRY * dllPushMatrix )(void);
static void ( APIENTRY * dllReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
static void ( APIENTRY * dllScissor )(GLint x, GLint y, GLsizei width, GLsizei height);
static void ( APIENTRY * dllStencilFunc )(GLenum func, GLint ref, GLuint mask);
static void ( APIENTRY * dllStencilOp )(GLenum fail, GLenum zfail, GLenum zpass);
static void ( APIENTRY * dllTexCoord2f )(GLfloat s, GLfloat t);
static void ( APIENTRY * dllTexCoord2fv )(const GLfloat *v);
static void ( APIENTRY * dllTexCoordPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllTexEnvf )(GLenum target, GLenum pname, GLfloat param);
static void ( APIENTRY * dllTexImage2D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
static void ( APIENTRY * dllTexParameterf )(GLenum target, GLenum pname, GLfloat param);
static void ( APIENTRY * dllTexParameterfv )(GLenum target, GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
static void ( APIENTRY * dllVertex2f )(GLfloat x, GLfloat y);
static void ( APIENTRY * dllVertex3f )(GLfloat x, GLfloat y, GLfloat z);
static void ( APIENTRY * dllVertex3fv )(const GLfloat *v);
static void ( APIENTRY * dllVertexPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllViewport )(GLint x, GLint y, GLsizei width, GLsizei height);




static const char * BooleanToString( GLboolean b )
{
	if ( b == GL_FALSE )
		return "GL_FALSE";
	else if ( b == GL_TRUE )
		return "GL_TRUE";
	else
		return "OUT OF RANGE FOR BOOLEAN";
}

static const char * FuncToString( GLenum f )
{
	switch ( f )
	{
	case GL_ALWAYS:
		return "GL_ALWAYS";
	case GL_NEVER:
		return "GL_NEVER";
	case GL_LEQUAL:
		return "GL_LEQUAL";
	case GL_LESS:
		return "GL_LESS";
	case GL_EQUAL:
		return "GL_EQUAL";
	case GL_GREATER:
		return "GL_GREATER";
	case GL_GEQUAL:
		return "GL_GEQUAL";
	case GL_NOTEQUAL:
		return "GL_NOTEQUAL";
	default:
		return "!!! UNKNOWN !!!";
	}
}

static const char * PrimToString( GLenum mode )
{
	static char prim[1024];

	if ( mode == GL_TRIANGLES )
		strcpy( prim, "GL_TRIANGLES" );
	else if ( mode == GL_TRIANGLE_STRIP )
		strcpy( prim, "GL_TRIANGLE_STRIP" );
	else if ( mode == GL_TRIANGLE_FAN )
		strcpy( prim, "GL_TRIANGLE_FAN" );
	else if ( mode == GL_QUADS )
		strcpy( prim, "GL_QUADS" );
	else if ( mode == GL_QUAD_STRIP )
		strcpy( prim, "GL_QUAD_STRIP" );
	else if ( mode == GL_POLYGON )
		strcpy( prim, "GL_POLYGON" );
	else if ( mode == GL_POINTS )
		strcpy( prim, "GL_POINTS" );
	else if ( mode == GL_LINES )
		strcpy( prim, "GL_LINES" );
	else if ( mode == GL_LINE_STRIP )
		strcpy( prim, "GL_LINE_STRIP" );
	else if ( mode == GL_LINE_LOOP )
		strcpy( prim, "GL_LINE_LOOP" );
	else
		sprintf( prim, "0x%x", mode );

	return prim;
}

static const char * CapToString( GLenum cap )
{
	static char buffer[1024];

	switch ( cap )
	{
	case GL_TEXTURE_2D:
		return "GL_TEXTURE_2D";
	case GL_BLEND:
		return "GL_BLEND";
	case GL_DEPTH_TEST:
		return "GL_DEPTH_TEST";
	case GL_CULL_FACE:
		return "GL_CULL_FACE";
	case GL_CLIP_PLANE0:
		return "GL_CLIP_PLANE0";
	case GL_COLOR_ARRAY:
		return "GL_COLOR_ARRAY";
	case GL_TEXTURE_COORD_ARRAY:
		return "GL_TEXTURE_COORD_ARRAY";
	case GL_VERTEX_ARRAY:
		return "GL_VERTEX_ARRAY";
	case GL_ALPHA_TEST:
		return "GL_ALPHA_TEST";
	case GL_STENCIL_TEST:
		return "GL_STENCIL_TEST";
	default:
		sprintf( buffer, "0x%x", cap );
	}

	return buffer;
}

static const char * TypeToString( GLenum t )
{
	switch ( t )
	{
	case GL_BYTE:
		return "GL_BYTE";
	case GL_UNSIGNED_BYTE:
		return "GL_UNSIGNED_BYTE";
	case GL_SHORT:
		return "GL_SHORT";
	case GL_UNSIGNED_SHORT:
		return "GL_UNSIGNED_SHORT";
	case GL_INT:
		return "GL_INT";
	case GL_UNSIGNED_INT:
		return "GL_UNSIGNED_INT";
	case GL_FLOAT:
		return "GL_FLOAT";
	case GL_DOUBLE:
		return "GL_DOUBLE";
	default:
		return "!!! UNKNOWN !!!";
	}
}

static void APIENTRY logAlphaFunc(GLenum func, GLclampf ref)
{
	fprintf( log_fp, "glAlphaFunc( 0x%x, %f )\n", func, ref );
	dllAlphaFunc( func, ref );
}
static void APIENTRY logBegin(GLenum mode)
{
	fprintf( log_fp, "glBegin( %s )\n", PrimToString( mode ));
	dllBegin( mode );
}
static void APIENTRY logBindTexture(GLenum target, GLuint texture)
{
	fprintf( log_fp, "glBindTexture( 0x%x, %u )\n", target, texture );
	dllBindTexture( target, texture );
}
static void BlendToName( char *n, GLenum f )
{
	switch ( f )
	{
	case GL_ONE:
		strcpy( n, "GL_ONE" );
		break;
	case GL_ZERO:
		strcpy( n, "GL_ZERO" );
		break;
	case GL_SRC_ALPHA:
		strcpy( n, "GL_SRC_ALPHA" );
		break;
	case GL_ONE_MINUS_SRC_ALPHA:
		strcpy( n, "GL_ONE_MINUS_SRC_ALPHA" );
		break;
	case GL_DST_COLOR:
		strcpy( n, "GL_DST_COLOR" );
		break;
	case GL_ONE_MINUS_DST_COLOR:
		strcpy( n, "GL_ONE_MINUS_DST_COLOR" );
		break;
	case GL_DST_ALPHA:
		strcpy( n, "GL_DST_ALPHA" );
		break;
	default:
		sprintf( n, "0x%x", f );
	}
}
static void APIENTRY logBlendFunc(GLenum sfactor, GLenum dfactor)
{
	char sf[128], df[128];

	BlendToName( sf, sfactor );
	BlendToName( df, dfactor );

	fprintf( log_fp, "glBlendFunc( %s, %s )\n", sf, df );
	dllBlendFunc( sfactor, dfactor );
}
static void APIENTRY logClear(GLbitfield mask)
{
	fprintf( log_fp, "glClear( 0x%x = ", mask );

	if ( mask & GL_COLOR_BUFFER_BIT )
		fprintf( log_fp, "GL_COLOR_BUFFER_BIT " );
	if ( mask & GL_DEPTH_BUFFER_BIT )
		fprintf( log_fp, "GL_DEPTH_BUFFER_BIT " );
	if ( mask & GL_STENCIL_BUFFER_BIT )
		fprintf( log_fp, "GL_STENCIL_BUFFER_BIT " );
	if ( mask & GL_ACCUM_BUFFER_BIT )
		fprintf( log_fp, "GL_ACCUM_BUFFER_BIT " );

	fprintf( log_fp, ")\n" );
	dllClear( mask );
}
static void APIENTRY logClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	fprintf( log_fp, "glClearColor\n" );
	dllClearColor( red, green, blue, alpha );
}
static void APIENTRY logClipPlane(GLenum plane, const GLdouble *equation)
{
	fprintf( log_fp, "glClipPlane\n" );
	dllClipPlane( plane, equation );
}
static void APIENTRY logColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
	fprintf( log_fp, "glColor3f\n" );
	dllColor3f( red, green, blue );
}

#define SIG( x ) fprintf( log_fp, x "\n" )

static void APIENTRY logColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	SIG( "glColorMask" );
	dllColorMask( red, green, blue, alpha );
}
static void APIENTRY logColorPointer(GLint size, GLenum type, GLsizei stride, const void *pointer)
{
	fprintf( log_fp, "glColorPointer( %d, %s, %d, MEM )\n", size, TypeToString( type ), stride );
	dllColorPointer( size, type, stride, pointer );
}
static void APIENTRY logCullFace(GLenum mode)
{
	fprintf( log_fp, "glCullFace( %s )\n", ( mode == GL_FRONT ) ? "GL_FRONT" : "GL_BACK" );
	dllCullFace( mode );
}
static void APIENTRY logDeleteTextures(GLsizei n, const GLuint *textures)
{
	SIG( "glDeleteTextures" );
	dllDeleteTextures( n, textures );
}
static void APIENTRY logDepthFunc(GLenum func)
{
	fprintf( log_fp, "glDepthFunc( %s )\n", FuncToString( func ) );
	dllDepthFunc( func );
}
static void APIENTRY logDepthMask(GLboolean flag)
{
	fprintf( log_fp, "glDepthMask( %s )\n", BooleanToString( flag ) );
	dllDepthMask( flag );
}
static void APIENTRY logDepthRange(GLclampd zNear, GLclampd zFar)
{
	fprintf( log_fp, "glDepthRange( %f, %f )\n", ( float ) zNear, ( float ) zFar );
	dllDepthRange( zNear, zFar );
}
static void APIENTRY logDisable(GLenum cap)
{
	fprintf( log_fp, "glDisable( %s )\n", CapToString( cap ) );
	dllDisable( cap );
}
static void APIENTRY logDisableClientState(GLenum array)
{
	fprintf( log_fp, "glDisableClientState( %s )\n", CapToString( array ) );
	dllDisableClientState( array );
}
static void APIENTRY logDrawBuffer(GLenum mode)
{
	SIG( "glDrawBuffer" );
	dllDrawBuffer( mode );
}
static void APIENTRY logDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices)
{
	fprintf( log_fp, "glDrawElements( %s, %d, %s, MEM )\n", PrimToString( mode ), count, TypeToString( type ) );
	dllDrawElements( mode, count, type, indices );
}
static void APIENTRY logEnable(GLenum cap)
{
	fprintf( log_fp, "glEnable( %s )\n", CapToString( cap ) );
	dllEnable( cap );
}

static void APIENTRY logEnableClientState(GLenum array)
{
	fprintf( log_fp, "glEnableClientState( %s )\n", CapToString( array ) );
	dllEnableClientState( array );
}

static void APIENTRY logEnd(void)
{
	SIG( "glEnd" );
	dllEnd();
}
static void APIENTRY logFinish(void)
{
	SIG( "glFinish" );
	dllFinish();
}
static GLenum APIENTRY logGetError(void)
{
	SIG( "glGetError" );
	return dllGetError();
}
static void APIENTRY logGetIntegerv(GLenum pname, GLint *params)
{
	SIG( "glGetIntegerv" );
	dllGetIntegerv( pname, params );
}
static const GLubyte * APIENTRY logGetString(GLenum name)
{
	SIG( "glGetString" );
	return dllGetString( name );
}
static void APIENTRY logLineWidth(GLfloat width)
{
	SIG( "glLineWidth" );
	dllLineWidth( width );
}
static void APIENTRY logLoadIdentity(void)
{
	SIG( "glLoadIdentity" );
	dllLoadIdentity();
}
static void APIENTRY logLoadMatrixf(const GLfloat *m)
{
	SIG( "glLoadMatrixf" );
	dllLoadMatrixf( m );
}
static void APIENTRY logMatrixMode(GLenum mode)
{
	SIG( "glMatrixMode" );
	dllMatrixMode( mode );
}
static void APIENTRY logOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	SIG( "glOrtho" );
	dllOrtho( left, right, bottom, top, zNear, zFar );
}
static void APIENTRY logPolygonMode(GLenum face, GLenum mode)
{
	fprintf( log_fp, "glPolygonMode( 0x%x, 0x%x )\n", face, mode );
	dllPolygonMode( face, mode );
}
static void APIENTRY logPolygonOffset(GLfloat factor, GLfloat units)
{
	SIG( "glPolygonOffset" );
	dllPolygonOffset( factor, units );
}
static void APIENTRY logPopMatrix(void)
{
	SIG( "glPopMatrix" );
	dllPopMatrix();
}
static void APIENTRY logPushMatrix(void)
{
	SIG( "glPushMatrix" );
	dllPushMatrix();
}
static void APIENTRY logReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels)
{
	SIG( "glReadPixels" );
	dllReadPixels( x, y, width, height, format, type, pixels );
}
static void APIENTRY logScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	fprintf( log_fp, "glScissor( %d, %d, %d, %d )\n", x, y, width, height );
	dllScissor( x, y, width, height );
}
static void APIENTRY logStencilFunc(GLenum func, GLint ref, GLuint mask)
{
	SIG( "glStencilFunc" );
	dllStencilFunc( func, ref, mask );
}
static void APIENTRY logStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
	SIG( "glStencilOp" );
	dllStencilOp( fail, zfail, zpass );
}
static void APIENTRY logTexCoord2f(GLfloat s, GLfloat t)
{
	SIG( "glTexCoord2f" );
	dllTexCoord2f( s, t );
}
static void APIENTRY logTexCoord2fv(const GLfloat *v)
{
	SIG( "glTexCoord2fv" );
	dllTexCoord2fv( v );
}
static void APIENTRY logTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *pointer)
{
	fprintf( log_fp, "glTexCoordPointer( %d, %s, %d, MEM )\n", size, TypeToString( type ), stride );
	dllTexCoordPointer( size, type, stride, pointer );
}

static void APIENTRY logTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
	fprintf( log_fp, "glTexEnvf( 0x%x, 0x%x, %f )\n", target, pname, param );
	dllTexEnvf( target, pname, param );
}
static void APIENTRY logTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels)
{
	SIG( "glTexImage2D" );
	dllTexImage2D( target, level, internalformat, width, height, border, format, type, pixels );
}
static void APIENTRY logTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
	fprintf( log_fp, "glTexParameterf( 0x%x, 0x%x, %f )\n", target, pname, param );
	dllTexParameterf( target, pname, param );
}
static void APIENTRY logTexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
	SIG( "glTexParameterfv" );
	dllTexParameterfv( target, pname, params );
}
static void APIENTRY logTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
	SIG( "glTexSubImage2D" );
	dllTexSubImage2D( target, level, xoffset, yoffset, width, height, format, type, pixels );
}
static void APIENTRY logVertex2f(GLfloat x, GLfloat y)
{
	SIG( "glVertex2f" );
	dllVertex2f( x, y );
}
static void APIENTRY logVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
	SIG( "glVertex3f" );
	dllVertex3f( x, y, z );
}
static void APIENTRY logVertex3fv(const GLfloat *v)
{
	SIG( "glVertex3fv" );
	dllVertex3fv( v );
}
static void APIENTRY logVertexPointer(GLint size, GLenum type, GLsizei stride, const void *pointer)
{
	fprintf( log_fp, "glVertexPointer( %d, %s, %d, MEM )\n", size, TypeToString( type ), stride );
	dllVertexPointer( size, type, stride, pointer );
}
static void APIENTRY logViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	fprintf( log_fp, "glViewport( %d, %d, %d, %d )\n", x, y, width, height );
	dllViewport( x, y, width, height );
}


void qglEnableLogging( qboolean enable )
{
	static qboolean isEnabled;

	// return if we're already active
	if ( isEnabled ) {
			return;
	}



		if ( !log_fp )
		{
			struct tm *newtime;
			time_t aclock;
			char buffer[1024];

			time( &aclock );
			newtime = localtime( &aclock );

			asctime( newtime );


			log_fp = fopen( buffer, "wt" );

			fprintf( log_fp, "%s\n", asctime( newtime ) );




	dllAlphaFunc            = qglAlphaFunc;
	dllBegin                = qglBegin;
	dllBindTexture          = qglBindTexture;
	dllBlendFunc            = qglBlendFunc;
	dllClear                = qglClear;
    dllClearColor           = qglClearColor;
	dllClipPlane            = qglClipPlane;
	dllColor3f              = qglColor3f;
	dllColorMask            = qglColorMask;
	dllColorPointer         = qglColorPointer;
	dllCullFace             = qglCullFace;
	dllDeleteTextures       = qglDeleteTextures;
	dllDepthFunc            = qglDepthFunc;
	dllDepthMask            = qglDepthMask;
	dllDepthRange           = qglDepthRange;
	dllDisable              = qglDisable;
	dllDisableClientState   = qglDisableClientState;
	dllDrawBuffer           = qglDrawBuffer;
	dllDrawElements         = qglDrawElements;
	dllEnable               = qglEnable;
	dllEnableClientState    = qglEnableClientState;
	dllEnd                  = qglEnd;
	dllFinish               = qglFinish;
	dllGetError             = qglGetError;
	dllGetIntegerv          = qglGetIntegerv;
	dllGetString            = qglGetString;
	dllLineWidth            = qglLineWidth;
	dllLoadIdentity         = qglLoadIdentity;
	dllLoadMatrixf          = qglLoadMatrixf;
	dllMatrixMode           = qglMatrixMode;
	dllOrtho                = qglOrtho;
	dllPolygonMode          = qglPolygonMode;
	dllPolygonOffset        = qglPolygonOffset;
	dllPopMatrix            = qglPopMatrix;
	dllPushMatrix           = qglPushMatrix;
	dllReadPixels           = qglReadPixels;
	dllScissor              = qglScissor;
	dllStencilFunc          = qglStencilFunc;
	dllStencilOp            = qglStencilOp;
	dllTexCoord2f           = qglTexCoord2f;
	dllTexCoord2fv          = qglTexCoord2fv;
	dllTexCoordPointer      = qglTexCoordPointer;
	dllTexEnvf              = qglTexEnvf;
	dllTexImage2D           = qglTexImage2D;
	dllTexParameterf        = qglTexParameterf;
	dllTexParameterfv       = qglTexParameterfv;
	dllTexSubImage2D        = qglTexSubImage2D;
	dllVertex2f             = qglVertex2f;
	dllVertex3f             = qglVertex3f;
	dllVertex3fv            = qglVertex3fv;
	dllVertexPointer        = qglVertexPointer;
	dllViewport             = qglViewport;

        ////////////////////////////////////////////////
		qglAlphaFunc                 = logAlphaFunc;
		qglBegin                     = logBegin;
		qglBindTexture               = logBindTexture;
		qglBlendFunc                 = logBlendFunc;
		qglClear                     = logClear;
		qglClearColor                = logClearColor;
		qglClipPlane                 = logClipPlane;
		qglColor3f                   = logColor3f;
		qglColorMask                 = logColorMask;
		qglColorPointer              = logColorPointer;
		qglCullFace                  = logCullFace;
		qglDeleteTextures            = logDeleteTextures;
		qglDepthFunc                 = logDepthFunc;
		qglDepthMask                 = logDepthMask;
		qglDepthRange                = logDepthRange;
		qglDisable                   = logDisable;
		qglDisableClientState        = logDisableClientState;
		qglDrawBuffer                = logDrawBuffer;
		qglDrawElements              = logDrawElements;
		qglEnable                    = logEnable;
		qglEnableClientState         = logEnableClientState;
		qglEnd                       = logEnd;
		qglFinish                    = logFinish;
		qglGetError                  = logGetError;
		qglGetIntegerv               = logGetIntegerv;
		qglGetString                 = logGetString;
		qglLineWidth                 = logLineWidth;
		qglLoadIdentity              = logLoadIdentity;
		qglLoadMatrixf               = logLoadMatrixf;
		qglMatrixMode                = logMatrixMode;
		qglOrtho                     = logOrtho;
		qglPolygonMode               = logPolygonMode;
		qglPolygonOffset             = logPolygonOffset;
		qglPopMatrix                 = logPopMatrix;
		qglPushMatrix                = logPushMatrix;
		qglReadPixels                = logReadPixels;
		qglScissor                   = logScissor;
		qglStencilFunc               = logStencilFunc;
		qglStencilOp                 = logStencilOp;
		qglTexCoord2f                = logTexCoord2f;
		qglTexCoord2fv               = logTexCoord2fv;
		qglTexCoordPointer           = logTexCoordPointer;
		qglTexEnvf                   = logTexEnvf;
		qglTexImage2D                = logTexImage2D;
		qglTexParameterf             = logTexParameterf;
		qglTexParameterfv            = logTexParameterfv;
		qglTexSubImage2D             = logTexSubImage2D;
		qglVertex2f                  = logVertex2f;
		qglVertex3f                  = logVertex3f;
		qglVertex3fv                 = logVertex3fv;
		qglVertexPointer             = logVertexPointer;
		qglViewport                  = logViewport;
	}
}


void qglDisableLogging( qboolean enable )
{
		if( log_fp )
        {
			fprintf( log_fp, "*** CLOSING LOG ***\n" );
			fclose( log_fp );
			log_fp = NULL;
		}
		qglAlphaFunc                 = dllAlphaFunc;
		qglBegin                     = dllBegin;
		qglBindTexture               = dllBindTexture;
		qglBlendFunc                 = dllBlendFunc;
		qglClear                     = dllClear;
		qglClearColor                = dllClearColor;
		qglClipPlane                 = dllClipPlane;
		qglColor3f                   = dllColor3f;
		qglColorMask                 = dllColorMask;
		qglColorPointer              = dllColorPointer;
		qglCullFace                  = dllCullFace;
		qglDeleteTextures            = dllDeleteTextures;
		qglDepthFunc                 = dllDepthFunc;
		qglDepthMask                 = dllDepthMask;
		qglDepthRange                = dllDepthRange;
		qglDisable                   = dllDisable;
		qglDisableClientState        = dllDisableClientState;
		qglDrawBuffer                = dllDrawBuffer;
		qglDrawElements              = dllDrawElements;
		qglEnable                    = dllEnable;
		qglEnableClientState         = dllEnableClientState;
		qglEnd                       = dllEnd;
		qglFinish                    = dllFinish;
		qglGetError                  = dllGetError;
		qglGetIntegerv               = dllGetIntegerv;
		qglGetString                 = dllGetString;
		qglLineWidth                 = dllLineWidth;
		qglLoadIdentity              = dllLoadIdentity;
		qglLoadMatrixf               = dllLoadMatrixf;
		qglMatrixMode                = dllMatrixMode;
		qglOrtho                     = dllOrtho;
		qglPolygonMode               = dllPolygonMode;
		qglPolygonOffset             = dllPolygonOffset;
		qglPopMatrix                 = dllPopMatrix;
		qglPushMatrix                = dllPushMatrix;
		qglReadPixels                = dllReadPixels;
		qglScissor                   = dllScissor;
		qglStencilFunc               = dllStencilFunc;
		qglStencilOp                 = dllStencilOp;
		qglTexCoord2f                = dllTexCoord2f;
		qglTexCoord2fv               = dllTexCoord2fv;
		qglTexCoordPointer           = dllTexCoordPointer;
		qglTexEnvf                   = dllTexEnvf;
	    qglTexImage2D                = dllTexImage2D;
		qglTexParameterf             = dllTexParameterf;
		qglTexParameterfv            = dllTexParameterfv;
		qglTexSubImage2D             = dllTexSubImage2D;
		qglVertex2f                  = dllVertex2f;
		qglVertex3f                  = dllVertex3f;
		qglVertex3fv                 = dllVertex3fv;
		qglVertexPointer             = dllVertexPointer;
		qglViewport                  = dllViewport;
}


void qglLogComment( char *comment )
{
	if ( log_fp )
	{
		fprintf( log_fp, "%s", comment );
	}
}
