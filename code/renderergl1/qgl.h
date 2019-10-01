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
** QGL.H
*/

#ifndef __QGL_H__
#define __QGL_H__

#ifdef _WIN32
	#include <windows.h>
	#include <gl/gl.h>
#else
	#include <GL/gl.h>
#endif


//===========================================================================

// GL function loader, based on https://gist.github.com/rygorous/16796a0c876cf8a5f542caddb55bce8a
// get missing functions from code/SDL2/include/SDL_opengl.h

// OpenGL 1.0/1.1 and OpenGL ES 1.0
#define QGL_1_1_PROCS \
	GLE(void, AlphaFunc, GLenum func, GLclampf ref) \
	GLE(void, BindTexture, GLenum target, GLuint texture) \
	GLE(void, BlendFunc, GLenum sfactor, GLenum dfactor) \
	GLE(void, ClearColor, GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) \
	GLE(void, Clear, GLbitfield mask) \
	GLE(void, ClearStencil, GLint s) \
	GLE(void, Color4f, GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) \
	GLE(void, ColorMask, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) \
	GLE(void, ColorPointer, GLint size, GLenum type, GLsizei stride, const GLvoid *ptr) \
	GLE(void, CopyTexSubImage2D, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) \
	GLE(void, CullFace, GLenum mode) \
	GLE(void, DeleteTextures, GLsizei n, const GLuint *textures) \
	GLE(void, DepthFunc, GLenum func) \
	GLE(void, DepthMask, GLboolean flag) \
	GLE(void, DisableClientState, GLenum cap) \
	GLE(void, Disable, GLenum cap) \
	GLE(void, DrawArrays, GLenum mode, GLint first, GLsizei count) \
	GLE(void, DrawElements, GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) \
	GLE(void, EnableClientState, GLenum cap) \
	GLE(void, Enable, GLenum cap) \
	GLE(void, Finish, void) \
	GLE(void, Flush, void) \
	GLE(void, GenTextures, GLsizei n, GLuint *textures ) \
	GLE(void, GetBooleanv, GLenum pname, GLboolean *params) \
	GLE(GLenum, GetError, void) \
	GLE(void, GetIntegerv, GLenum pname, GLint *params) \
	GLE(void, GetFloatv, GLenum pname, GLfloat *params) \
	GLE(const GLubyte *, GetString, GLenum name) \
	GLE(void, LineWidth, GLfloat width) \
	GLE(void, LoadIdentity, void) \
	GLE(void, LoadMatrixf, const GLfloat *m) \
	GLE(void, MatrixMode, GLenum mode) \
	GLE(void, PolygonOffset, GLfloat factor, GLfloat units) \
	GLE(void, PopMatrix, void) \
	GLE(void, PushMatrix, void) \
	GLE(void, PushClientAttrib, GLbitfield mask) \
	GLE(void, PopClientAttrib, void) \
	GLE(void, ReadPixels, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) \
	GLE(void, Scissor, GLint x, GLint y, GLsizei width, GLsizei height) \
	GLE(void, ShadeModel, GLenum mode) \
	GLE(void, StencilFunc, GLenum func, GLint ref, GLuint mask) \
	GLE(void, StencilMask, GLuint mask) \
	GLE(void, StencilOp, GLenum fail, GLenum zfail, GLenum zpass) \
	GLE(void, TexCoordPointer, GLint size, GLenum type, GLsizei stride, const GLvoid *ptr) \
	GLE(void, TexEnvf, GLenum target, GLenum pname, GLfloat param) \
	GLE(void, TexImage2D, GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) \
	GLE(void, TexParameterf, GLenum target, GLenum pname, GLfloat param) \
	GLE(void, TexParameterfv, GLenum target, GLenum pname, GLfloat* param) \
	GLE(void, TexParameteri, GLenum target, GLenum pname, GLint param) \
	GLE(void, TexSubImage2D, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) \
	GLE(void, Translatef, GLfloat x, GLfloat y, GLfloat z) \
	GLE(void, VertexPointer, GLint size, GLenum type, GLsizei stride, const GLvoid *ptr) \
	GLE(void, Viewport, GLint x, GLint y, GLsizei width, GLsizei height) \

// OpenGL 1.0/1.1 but not OpenGL ES 1.x
#define QGL_DESKTOP_1_1_PROCS \
	GLE(void, ArrayElement, GLint i) \
	GLE(void, Begin, GLenum mode) \
	GLE(void, ClearDepth, GLclampd depth) \
	GLE(void, ClipPlane, GLenum plane, const GLdouble *equation) \
	GLE(void, Color3f, GLfloat red, GLfloat green, GLfloat blue) \
	GLE(void, Color4ubv, const GLubyte *v) \
	GLE(void, DepthRange, GLclampd near_val, GLclampd far_val) \
	GLE(void, DrawBuffer, GLenum mode) \
	GLE(void, End, void) \
	GLE(void, Frustum, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val) \
	GLE(void, Ortho, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val) \
	GLE(void, PolygonMode, GLenum face, GLenum mode) \
	GLE(void, TexCoord2f, GLfloat s, GLfloat t) \
	GLE(void, TexCoord2fv, const GLfloat *v) \
	GLE(void, Vertex2f, GLfloat x, GLfloat y) \
	GLE(void, Vertex3f, GLfloat x, GLfloat y, GLfloat z) \
	GLE(void, Vertex3fv, const GLfloat *v) 

// OpenGL ES 1.1 but not desktop OpenGL 1.x
#define QGL_ES_1_1_PROCS \
	GLE(void, ClearDepthf, GLclampf depth) \
	GLE(void, ClipPlanef, GLenum plane, const GLfloat *equation) \
	GLE(void, DepthRangef, GLclampf near_val, GLclampf far_val) \
	GLE(void, Frustumf, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val) \
	GLE(void, Orthof, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val) 

// OpenGL 1.3, was GL_ARB_texture_compression
#define QGL_1_3_PROCS \
	GLE(void, ActiveTexture, GLenum texture) \
	GLE(void, CompressedTexImage2D, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data) \
	GLE(void, CompressedTexSubImage2D, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data) 


#define GLE(ret, name, ...) typedef ret APIENTRY name##proc(__VA_ARGS__);
QGL_1_1_PROCS;
QGL_DESKTOP_1_1_PROCS;
//QGL_ES_1_1_PROCS;
QGL_1_3_PROCS;
#undef GLE


#endif
