#ifndef __QGL_H__
#define __QGL_H__


#if defined( _WIN32 )

#define NOMINMAX
#include <windows.h>
#include <gl/gl.h>
#endif

/*
typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef float GLclampf;
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLuint;
typedef double GLdouble;
typedef float GLfloat;
typedef void GLvoid;
typedef double GLclampd;
*/
//===========================================================================
static inline void qglAlphaFunc(GLenum func, GLclampf ref) {}
static inline void qglBegin(GLenum mode) {}
static inline void qglBindTexture(GLenum target, GLuint texture) {}
static inline void qglBlendFunc(GLenum sfactor, GLenum dfactor) {}
static inline void qglClear(GLbitfield mask) {}
static inline void qglClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {}
static inline void qglClipPlane(GLenum plane, const GLdouble *equation){ }
static inline void qglColor3f(GLfloat red, GLfloat green, GLfloat blue){ }
static inline void qglColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha){ }
static inline void qglColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer){ }
static inline void qglCullFace(GLenum mode){ }
static inline void qglDeleteTextures(GLsizei n, const GLuint *textures){ }
static inline void qglDepthFunc(GLenum func){ }
static inline void qglDepthMask(GLboolean flag){ }
static inline void qglDepthRange(GLclampd zNear, GLclampd zFar){ }
static inline void qglDisable(GLenum cap){ }
static inline void qglDisableClientState(GLenum array){ }
static inline void qglDrawBuffer(GLenum mode){ }
static inline void qglDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices){ }
static inline void qglEnable(GLenum cap){ }
static inline void qglEnableClientState(GLenum array){ }
static inline void qglEnd(void){ }
static inline void qglFinish(void){ }
static inline GLenum qglGetError(void) { return 0; }
static inline void qglGetIntegerv(GLenum pname, GLint *params){ }
// const GLubyte * qglGetString(GLenum name) { return APP_MANIFEST_TABLE; }
static inline void qglLineWidth(GLfloat width){ }
static inline void qglLoadIdentity(void){ }
static inline void qglLoadMatrixf(const GLfloat *m){ }
static inline void qglMatrixMode(GLenum mode){ }
static inline void qglOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar){ }
static inline void qglPolygonMode(GLenum face, GLenum mode){ }
static inline void qglPolygonOffset(GLfloat factor, GLfloat units){ }
static inline void qglPopMatrix(void){ }
static inline void qglPushMatrix(void){ }
static inline void qglReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels){ }
static inline void qglScissor(GLint x, GLint y, GLsizei width, GLsizei height){ }
static inline void qglStencilFunc(GLenum func, GLint ref, GLuint mask){ }
static inline void qglStencilOp(GLenum fail, GLenum zfail, GLenum zpass){ }
static inline void qglTexCoord2f(GLfloat s, GLfloat t){ }
static inline void qglTexCoord2fv(const GLfloat *v){ }
static inline void qglTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer){ }
static inline void qglTexEnvf(GLenum target, GLenum pname, GLfloat param){ }
static inline void qglTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels){ }
static inline void qglTexParameterf(GLenum target, GLenum pname, GLfloat param){ }
static inline void qglTexParameterfv(GLenum target, GLenum pname, const GLfloat *params){ }
static inline void qglTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels){ }
static inline void qglVertex2f(GLfloat x, GLfloat y){ }
static inline void qglVertex3f(GLfloat x, GLfloat y, GLfloat z){ }
static inline void qglVertex3fv(const GLfloat *v){ }
static inline void qglVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer){ }
static inline void qglViewport(GLint x, GLint y, GLsizei width, GLsizei height){ }

#endif
