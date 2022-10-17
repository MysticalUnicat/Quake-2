/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/*
** QGL.H
*/

#ifndef __QGL_H__
#define __QGL_H__

#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/gl.h>

bool QGL_Init(const char *dllname);
void QGL_Shutdown(void);

#ifndef APIENTRY
#define APIENTRY
#endif

#define QGL_FUNCTIONS(X)                                                                                               \
  X(void, Accum, (GLenum op, GLfloat value))                                                                           \
  X(void, AlphaFunc, (GLenum func, GLclampf ref))                                                                      \
  X(GLboolean, AreTexturesResident, (GLsizei n, const GLuint *textures, GLboolean *residences))                        \
  X(void, ArrayElement, (GLint i))                                                                                     \
  X(void, Begin, (GLenum mode))                                                                                        \
  X(void, BindTexture, (GLenum target, GLuint texture))                                                                \
  X(void, Bitmap,                                                                                                      \
    (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove,                        \
     const GLubyte *bitmap))                                                                                           \
  X(void, BlendFunc, (GLenum sfactor, GLenum dfactor))                                                                 \
  X(void, CallList, (GLuint list))                                                                                     \
  X(void, CallLists, (GLsizei n, GLenum type, const GLvoid *lists))                                                    \
  X(void, Clear, (GLbitfield mask))                                                                                    \
  X(void, ClearAccum, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha))                                       \
  X(void, ClearColor, (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha))                                   \
  X(void, ClearDepth, (GLclampd depth))                                                                                \
  X(void, ClearIndex, (GLfloat c))                                                                                     \
  X(void, ClearStencil, (GLint s))                                                                                     \
  X(void, ClipPlane, (GLenum plane, const GLdouble *equation))                                                         \
  X(void, Color3b, (GLbyte red, GLbyte green, GLbyte blue))                                                            \
  X(void, Color3bv, (const GLbyte *v))                                                                                 \
  X(void, Color3d, (GLdouble red, GLdouble green, GLdouble blue))                                                      \
  X(void, Color3dv, (const GLdouble *v))                                                                               \
  X(void, Color3f, (GLfloat red, GLfloat green, GLfloat blue))                                                         \
  X(void, Color3fv, (const GLfloat *v))                                                                                \
  X(void, Color3i, (GLint red, GLint green, GLint blue))                                                               \
  X(void, Color3iv, (const GLint *v))                                                                                  \
  X(void, Color3s, (GLshort red, GLshort green, GLshort blue))                                                         \
  X(void, Color3sv, (const GLshort *v))                                                                                \
  X(void, Color3ub, (GLubyte red, GLubyte green, GLubyte blue))                                                        \
  X(void, Color3ubv, (const GLubyte *v))                                                                               \
  X(void, Color3ui, (GLuint red, GLuint green, GLuint blue))                                                           \
  X(void, Color3uiv, (const GLuint *v))                                                                                \
  X(void, Color3us, (GLushort red, GLushort green, GLushort blue))                                                     \
  X(void, Color3usv, (const GLushort *v))                                                                              \
  X(void, Color4b, (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha))                                              \
  X(void, Color4bv, (const GLbyte *v))                                                                                 \
  X(void, Color4d, (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha))                                      \
  X(void, Color4dv, (const GLdouble *v))                                                                               \
  X(void, Color4f, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha))                                          \
  X(void, Color4fv, (const GLfloat *v))                                                                                \
  X(void, Color4i, (GLint red, GLint green, GLint blue, GLint alpha))                                                  \
  X(void, Color4iv, (const GLint *v))                                                                                  \
  X(void, Color4s, (GLshort red, GLshort green, GLshort blue, GLshort alpha))                                          \
  X(void, Color4sv, (const GLshort *v))                                                                                \
  X(void, Color4ub, (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha))                                         \
  X(void, Color4ubv, (const GLubyte *v))                                                                               \
  X(void, Color4ui, (GLuint red, GLuint green, GLuint blue, GLuint alpha))                                             \
  X(void, Color4uiv, (const GLuint *v))                                                                                \
  X(void, Color4us, (GLushort red, GLushort green, GLushort blue, GLushort alpha))                                     \
  X(void, Color4usv, (const GLushort *v))                                                                              \
  X(void, ColorMask, (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha))                                \
  X(void, ColorMaterial, (GLenum face, GLenum mode))                                                                   \
  X(void, ColorPointer, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer))                              \
  X(void, CopyPixels, (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type))                                  \
  X(void, CopyTexImage1D,                                                                                              \
    (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border))                \
  X(void, CopyTexImage2D,                                                                                              \
    (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height,               \
     GLint border))                                                                                                    \
  X(void, CopyTexSubImage1D, (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width))             \
  X(void, CopyTexSubImage2D,                                                                                           \
    (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height))       \
  X(void, CullFace, (GLenum mode))                                                                                     \
  X(void, DeleteLists, (GLuint list, GLsizei range))                                                                   \
  X(void, DeleteTextures, (GLsizei n, const GLuint *textures))                                                         \
  X(void, DepthFunc, (GLenum func))                                                                                    \
  X(void, DepthMask, (GLboolean flag))                                                                                 \
  X(void, DepthRange, (GLclampd zNear, GLclampd zFar))                                                                 \
  X(void, Disable, (GLenum cap))                                                                                       \
  X(void, DisableClientState, (GLenum array))                                                                          \
  X(void, DrawArrays, (GLenum mode, GLint first, GLsizei count))                                                       \
  X(void, DrawBuffer, (GLenum mode))                                                                                   \
  X(void, DrawElements, (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices))                              \
  X(void, DrawPixels, (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels))               \
  X(void, EdgeFlag, (GLboolean flag))                                                                                  \
  X(void, EdgeFlagPointer, (GLsizei stride, const GLvoid *pointer))                                                    \
  X(void, EdgeFlagv, (const GLboolean *flag))                                                                          \
  X(void, Enable, (GLenum cap))                                                                                        \
  X(void, EnableClientState, (GLenum array))                                                                           \
  X(void, End, (void))                                                                                                 \
  X(void, EndList, (void))                                                                                             \
  X(void, EvalCoord1d, (GLdouble u))                                                                                   \
  X(void, EvalCoord1dv, (const GLdouble *u))                                                                           \
  X(void, EvalCoord1f, (GLfloat u))                                                                                    \
  X(void, EvalCoord1fv, (const GLfloat *u))                                                                            \
  X(void, EvalCoord2d, (GLdouble u, GLdouble v))                                                                       \
  X(void, EvalCoord2dv, (const GLdouble *u))                                                                           \
  X(void, EvalCoord2f, (GLfloat u, GLfloat v))                                                                         \
  X(void, EvalCoord2fv, (const GLfloat *u))                                                                            \
  X(void, EvalMesh1, (GLenum mode, GLint i1, GLint i2))                                                                \
  X(void, EvalMesh2, (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2))                                            \
  X(void, EvalPoint1, (GLint i))                                                                                       \
  X(void, EvalPoint2, (GLint i, GLint j))                                                                              \
  X(void, FeedbackBuffer, (GLsizei size, GLenum type, GLfloat * buffer))                                               \
  X(void, Finish, (void))                                                                                              \
  X(void, Flush, (void))                                                                                               \
  X(void, Fogf, (GLenum pname, GLfloat param))                                                                         \
  X(void, Fogfv, (GLenum pname, const GLfloat *params))                                                                \
  X(void, Fogi, (GLenum pname, GLint param))                                                                           \
  X(void, Fogiv, (GLenum pname, const GLint *params))                                                                  \
  X(void, FrontFace, (GLenum mode))                                                                                    \
  X(void, Frustum, (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar))      \
  X(GLuint, GenLists, (GLsizei range))                                                                                 \
  X(void, GenTextures, (GLsizei n, GLuint * textures))                                                                 \
  X(void, GetBooleanv, (GLenum pname, GLboolean * params))                                                             \
  X(void, GetClipPlane, (GLenum plane, GLdouble * equation))                                                           \
  X(void, GetDoublev, (GLenum pname, GLdouble * params))                                                               \
  X(GLenum, GetError, (void))                                                                                          \
  X(void, GetFloatv, (GLenum pname, GLfloat * params))                                                                 \
  X(void, GetIntegerv, (GLenum pname, GLint * params))                                                                 \
  X(void, GetLightfv, (GLenum light, GLenum pname, GLfloat * params))                                                  \
  X(void, GetLightiv, (GLenum light, GLenum pname, GLint * params))                                                    \
  X(void, GetMapdv, (GLenum target, GLenum query, GLdouble * v))                                                       \
  X(void, GetMapfv, (GLenum target, GLenum query, GLfloat * v))                                                        \
  X(void, GetMapiv, (GLenum target, GLenum query, GLint * v))                                                          \
  X(void, GetMaterialfv, (GLenum face, GLenum pname, GLfloat * params))                                                \
  X(void, GetMaterialiv, (GLenum face, GLenum pname, GLint * params))                                                  \
  X(void, GetPixelMapfv, (GLenum map, GLfloat * values))                                                               \
  X(void, GetPixelMapuiv, (GLenum map, GLuint * values))                                                               \
  X(void, GetPixelMapusv, (GLenum map, GLushort * values))                                                             \
  X(void, GetPointerv, (GLenum pname, GLvoid * *params))                                                               \
  X(void, GetPolygonStipple, (GLubyte * mask))                                                                         \
  X(const GLubyte *, GetString, (GLenum name))                                                                         \
  X(void, GetTexEnvfv, (GLenum target, GLenum pname, GLfloat * params))                                                \
  X(void, GetTexEnviv, (GLenum target, GLenum pname, GLint * params))                                                  \
  X(void, GetTexGendv, (GLenum coord, GLenum pname, GLdouble * params))                                                \
  X(void, GetTexGenfv, (GLenum coord, GLenum pname, GLfloat * params))                                                 \
  X(void, GetTexGeniv, (GLenum coord, GLenum pname, GLint * params))                                                   \
  X(void, GetTexImage, (GLenum target, GLint level, GLenum format, GLenum type, GLvoid * pixels))                      \
  X(void, GetTexLevelParameterfv, (GLenum target, GLint level, GLenum pname, GLfloat * params))                        \
  X(void, GetTexLevelParameteriv, (GLenum target, GLint level, GLenum pname, GLint * params))                          \
  X(void, GetTexParameterfv, (GLenum target, GLenum pname, GLfloat * params))                                          \
  X(void, GetTexParameteriv, (GLenum target, GLenum pname, GLint * params))                                            \
  X(void, Hint, (GLenum target, GLenum mode))                                                                          \
  X(void, IndexMask, (GLuint mask))                                                                                    \
  X(void, IndexPointer, (GLenum type, GLsizei stride, const GLvoid *pointer))                                          \
  X(void, Indexd, (GLdouble c))                                                                                        \
  X(void, Indexdv, (const GLdouble *c))                                                                                \
  X(void, Indexf, (GLfloat c))                                                                                         \
  X(void, Indexfv, (const GLfloat *c))                                                                                 \
  X(void, Indexi, (GLint c))                                                                                           \
  X(void, Indexiv, (const GLint *c))                                                                                   \
  X(void, Indexs, (GLshort c))                                                                                         \
  X(void, Indexsv, (const GLshort *c))                                                                                 \
  X(void, Indexub, (GLubyte c))                                                                                        \
  X(void, Indexubv, (const GLubyte *c))                                                                                \
  X(void, InitNames, (void))                                                                                           \
  X(void, InterleavedArrays, (GLenum format, GLsizei stride, const GLvoid *pointer))                                   \
  X(GLboolean, IsEnabled, (GLenum cap))                                                                                \
  X(GLboolean, IsList, (GLuint list))                                                                                  \
  X(GLboolean, IsTexture, (GLuint texture))                                                                            \
  X(void, LightModelf, (GLenum pname, GLfloat param))                                                                  \
  X(void, LightModelfv, (GLenum pname, const GLfloat *params))                                                         \
  X(void, LightModeli, (GLenum pname, GLint param))                                                                    \
  X(void, LightModeliv, (GLenum pname, const GLint *params))                                                           \
  X(void, Lightf, (GLenum light, GLenum pname, GLfloat param))                                                         \
  X(void, Lightfv, (GLenum light, GLenum pname, const GLfloat *params))                                                \
  X(void, Lighti, (GLenum light, GLenum pname, GLint param))                                                           \
  X(void, Lightiv, (GLenum light, GLenum pname, const GLint *params))                                                  \
  X(void, LineStipple, (GLint factor, GLushort pattern))                                                               \
  X(void, LineWidth, (GLfloat width))                                                                                  \
  X(void, ListBase, (GLuint base))                                                                                     \
  X(void, LoadIdentity, (void))                                                                                        \
  X(void, LoadMatrixd, (const GLdouble *m))                                                                            \
  X(void, LoadMatrixf, (const GLfloat *m))                                                                             \
  X(void, LoadName, (GLuint name))                                                                                     \
  X(void, LogicOp, (GLenum opcode))                                                                                    \
  X(void, Map1d, (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points))         \
  X(void, Map1f, (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points))            \
  X(void, Map2d,                                                                                                       \
    (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride,    \
     GLint vorder, const GLdouble *points))                                                                            \
  X(void, Map2f,                                                                                                       \
    (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride,        \
     GLint vorder, const GLfloat *points))                                                                             \
  X(void, MapGrid1d, (GLint un, GLdouble u1, GLdouble u2))                                                             \
  X(void, MapGrid1f, (GLint un, GLfloat u1, GLfloat u2))                                                               \
  X(void, MapGrid2d, (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2))                         \
  X(void, MapGrid2f, (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2))                             \
  X(void, Materialf, (GLenum face, GLenum pname, GLfloat param))                                                       \
  X(void, Materialfv, (GLenum face, GLenum pname, const GLfloat *params))                                              \
  X(void, Materiali, (GLenum face, GLenum pname, GLint param))                                                         \
  X(void, Materialiv, (GLenum face, GLenum pname, const GLint *params))                                                \
  X(void, MatrixMode, (GLenum mode))                                                                                   \
  X(void, MultMatrixd, (const GLdouble *m))                                                                            \
  X(void, MultMatrixf, (const GLfloat *m))                                                                             \
  X(void, NewList, (GLuint list, GLenum mode))                                                                         \
  X(void, Normal3b, (GLbyte nx, GLbyte ny, GLbyte nz))                                                                 \
  X(void, Normal3bv, (const GLbyte *v))                                                                                \
  X(void, Normal3d, (GLdouble nx, GLdouble ny, GLdouble nz))                                                           \
  X(void, Normal3dv, (const GLdouble *v))                                                                              \
  X(void, Normal3f, (GLfloat nx, GLfloat ny, GLfloat nz))                                                              \
  X(void, Normal3fv, (const GLfloat *v))                                                                               \
  X(void, Normal3i, (GLint nx, GLint ny, GLint nz))                                                                    \
  X(void, Normal3iv, (const GLint *v))                                                                                 \
  X(void, Normal3s, (GLshort nx, GLshort ny, GLshort nz))                                                              \
  X(void, Normal3sv, (const GLshort *v))                                                                               \
  X(void, NormalPointer, (GLenum type, GLsizei stride, const GLvoid *pointer))                                         \
  X(void, Ortho, (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar))        \
  X(void, PassThrough, (GLfloat token))                                                                                \
  X(void, PixelMapfv, (GLenum map, GLsizei mapsize, const GLfloat *values))                                            \
  X(void, PixelMapuiv, (GLenum map, GLsizei mapsize, const GLuint *values))                                            \
  X(void, PixelMapusv, (GLenum map, GLsizei mapsize, const GLushort *values))                                          \
  X(void, PixelStoref, (GLenum pname, GLfloat param))                                                                  \
  X(void, PixelStorei, (GLenum pname, GLint param))                                                                    \
  X(void, PixelTransferf, (GLenum pname, GLfloat param))                                                               \
  X(void, PixelTransferi, (GLenum pname, GLint param))                                                                 \
  X(void, PixelZoom, (GLfloat xfactor, GLfloat yfactor))                                                               \
  X(void, PointSize, (GLfloat size))                                                                                   \
  X(void, PolygonMode, (GLenum face, GLenum mode))                                                                     \
  X(void, PolygonOffset, (GLfloat factor, GLfloat units))                                                              \
  X(void, PolygonStipple, (const GLubyte *mask))                                                                       \
  X(void, PopAttrib, (void))                                                                                           \
  X(void, PopClientAttrib, (void))                                                                                     \
  X(void, PopMatrix, (void))                                                                                           \
  X(void, PopName, (void))                                                                                             \
  X(void, PrioritizeTextures, (GLsizei n, const GLuint *textures, const GLclampf *priorities))                         \
  X(void, PushAttrib, (GLbitfield mask))                                                                               \
  X(void, PushClientAttrib, (GLbitfield mask))                                                                         \
  X(void, PushMatrix, (void))                                                                                          \
  X(void, PushName, (GLuint name))                                                                                     \
  X(void, RasterPos2d, (GLdouble x, GLdouble y))                                                                       \
  X(void, RasterPos2dv, (const GLdouble *v))                                                                           \
  X(void, RasterPos2f, (GLfloat x, GLfloat y))                                                                         \
  X(void, RasterPos2fv, (const GLfloat *v))                                                                            \
  X(void, RasterPos2i, (GLint x, GLint y))                                                                             \
  X(void, RasterPos2iv, (const GLint *v))                                                                              \
  X(void, RasterPos2s, (GLshort x, GLshort y))                                                                         \
  X(void, RasterPos2sv, (const GLshort *v))                                                                            \
  X(void, RasterPos3d, (GLdouble x, GLdouble y, GLdouble z))                                                           \
  X(void, RasterPos3dv, (const GLdouble *v))                                                                           \
  X(void, RasterPos3f, (GLfloat x, GLfloat y, GLfloat z))                                                              \
  X(void, RasterPos3fv, (const GLfloat *v))                                                                            \
  X(void, RasterPos3i, (GLint x, GLint y, GLint z))                                                                    \
  X(void, RasterPos3iv, (const GLint *v))                                                                              \
  X(void, RasterPos3s, (GLshort x, GLshort y, GLshort z))                                                              \
  X(void, RasterPos3sv, (const GLshort *v))                                                                            \
  X(void, RasterPos4d, (GLdouble x, GLdouble y, GLdouble z, GLdouble w))                                               \
  X(void, RasterPos4dv, (const GLdouble *v))                                                                           \
  X(void, RasterPos4f, (GLfloat x, GLfloat y, GLfloat z, GLfloat w))                                                   \
  X(void, RasterPos4fv, (const GLfloat *v))                                                                            \
  X(void, RasterPos4i, (GLint x, GLint y, GLint z, GLint w))                                                           \
  X(void, RasterPos4iv, (const GLint *v))                                                                              \
  X(void, RasterPos4s, (GLshort x, GLshort y, GLshort z, GLshort w))                                                   \
  X(void, RasterPos4sv, (const GLshort *v))                                                                            \
  X(void, ReadBuffer, (GLenum mode))                                                                                   \
  X(void, ReadPixels, (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels))  \
  X(void, Rectd, (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2))                                                 \
  X(void, Rectdv, (const GLdouble *v1, const GLdouble *v2))                                                            \
  X(void, Rectf, (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2))                                                     \
  X(void, Rectfv, (const GLfloat *v1, const GLfloat *v2))                                                              \
  X(void, Recti, (GLint x1, GLint y1, GLint x2, GLint y2))                                                             \
  X(void, Rectiv, (const GLint *v1, const GLint *v2))                                                                  \
  X(void, Rects, (GLshort x1, GLshort y1, GLshort x2, GLshort y2))                                                     \
  X(void, Rectsv, (const GLshort *v1, const GLshort *v2))                                                              \
  X(GLint, RenderMode, (GLenum mode))                                                                                  \
  X(void, Rotated, (GLdouble angle, GLdouble x, GLdouble y, GLdouble z))                                               \
  X(void, Rotatef, (GLfloat angle, GLfloat x, GLfloat y, GLfloat z))                                                   \
  X(void, Scaled, (GLdouble x, GLdouble y, GLdouble z))                                                                \
  X(void, Scalef, (GLfloat x, GLfloat y, GLfloat z))                                                                   \
  X(void, Scissor, (GLint x, GLint y, GLsizei width, GLsizei height))                                                  \
  X(void, SelectBuffer, (GLsizei size, GLuint * buffer))                                                               \
  X(void, ShadeModel, (GLenum mode))                                                                                   \
  X(void, StencilFunc, (GLenum func, GLint ref, GLuint mask))                                                          \
  X(void, StencilMask, (GLuint mask))                                                                                  \
  X(void, StencilOp, (GLenum fail, GLenum zfail, GLenum zpass))                                                        \
  X(void, TexCoord1d, (GLdouble s))                                                                                    \
  X(void, TexCoord1dv, (const GLdouble *v))                                                                            \
  X(void, TexCoord1f, (GLfloat s))                                                                                     \
  X(void, TexCoord1fv, (const GLfloat *v))                                                                             \
  X(void, TexCoord1i, (GLint s))                                                                                       \
  X(void, TexCoord1iv, (const GLint *v))                                                                               \
  X(void, TexCoord1s, (GLshort s))                                                                                     \
  X(void, TexCoord1sv, (const GLshort *v))                                                                             \
  X(void, TexCoord2d, (GLdouble s, GLdouble t))                                                                        \
  X(void, TexCoord2dv, (const GLdouble *v))                                                                            \
  X(void, TexCoord2f, (GLfloat s, GLfloat t))                                                                          \
  X(void, TexCoord2fv, (const GLfloat *v))                                                                             \
  X(void, TexCoord2i, (GLint s, GLint t))                                                                              \
  X(void, TexCoord2iv, (const GLint *v))                                                                               \
  X(void, TexCoord2s, (GLshort s, GLshort t))                                                                          \
  X(void, TexCoord2sv, (const GLshort *v))                                                                             \
  X(void, TexCoord3d, (GLdouble s, GLdouble t, GLdouble r))                                                            \
  X(void, TexCoord3dv, (const GLdouble *v))                                                                            \
  X(void, TexCoord3f, (GLfloat s, GLfloat t, GLfloat r))                                                               \
  X(void, TexCoord3fv, (const GLfloat *v))                                                                             \
  X(void, TexCoord3i, (GLint s, GLint t, GLint r))                                                                     \
  X(void, TexCoord3iv, (const GLint *v))                                                                               \
  X(void, TexCoord3s, (GLshort s, GLshort t, GLshort r))                                                               \
  X(void, TexCoord3sv, (const GLshort *v))                                                                             \
  X(void, TexCoord4d, (GLdouble s, GLdouble t, GLdouble r, GLdouble q))                                                \
  X(void, TexCoord4dv, (const GLdouble *v))                                                                            \
  X(void, TexCoord4f, (GLfloat s, GLfloat t, GLfloat r, GLfloat q))                                                    \
  X(void, TexCoord4fv, (const GLfloat *v))                                                                             \
  X(void, TexCoord4i, (GLint s, GLint t, GLint r, GLint q))                                                            \
  X(void, TexCoord4iv, (const GLint *v))                                                                               \
  X(void, TexCoord4s, (GLshort s, GLshort t, GLshort r, GLshort q))                                                    \
  X(void, TexCoord4sv, (const GLshort *v))                                                                             \
  X(void, TexCoordPointer, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer))                           \
  X(void, TexEnvf, (GLenum target, GLenum pname, GLfloat param))                                                       \
  X(void, TexEnvfv, (GLenum target, GLenum pname, const GLfloat *params))                                              \
  X(void, TexEnvi, (GLenum target, GLenum pname, GLint param))                                                         \
  X(void, TexEnviv, (GLenum target, GLenum pname, const GLint *params))                                                \
  X(void, TexGend, (GLenum coord, GLenum pname, GLdouble param))                                                       \
  X(void, TexGendv, (GLenum coord, GLenum pname, const GLdouble *params))                                              \
  X(void, TexGenf, (GLenum coord, GLenum pname, GLfloat param))                                                        \
  X(void, TexGenfv, (GLenum coord, GLenum pname, const GLfloat *params))                                               \
  X(void, TexGeni, (GLenum coord, GLenum pname, GLint param))                                                          \
  X(void, TexGeniv, (GLenum coord, GLenum pname, const GLint *params))                                                 \
  X(void, TexImage1D,                                                                                                  \
    (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type,        \
     const GLvoid *pixels))                                                                                            \
  X(void, TexImage2D,                                                                                                  \
    (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format,     \
     GLenum type, const GLvoid *pixels))                                                                               \
  X(void, TexParameterf, (GLenum target, GLenum pname, GLfloat param))                                                 \
  X(void, TexParameterfv, (GLenum target, GLenum pname, const GLfloat *params))                                        \
  X(void, TexParameteri, (GLenum target, GLenum pname, GLint param))                                                   \
  X(void, TexParameteriv, (GLenum target, GLenum pname, const GLint *params))                                          \
  X(void, TexSubImage1D,                                                                                               \
    (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels))      \
  X(void, TexSubImage2D,                                                                                               \
    (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format,           \
     GLenum type, const GLvoid *pixels))                                                                               \
  X(void, Translated, (GLdouble x, GLdouble y, GLdouble z))                                                            \
  X(void, Translatef, (GLfloat x, GLfloat y, GLfloat z))                                                               \
  X(void, Vertex2d, (GLdouble x, GLdouble y))                                                                          \
  X(void, Vertex2dv, (const GLdouble *v))                                                                              \
  X(void, Vertex2f, (GLfloat x, GLfloat y))                                                                            \
  X(void, Vertex2fv, (const GLfloat *v))                                                                               \
  X(void, Vertex2i, (GLint x, GLint y))                                                                                \
  X(void, Vertex2iv, (const GLint *v))                                                                                 \
  X(void, Vertex2s, (GLshort x, GLshort y))                                                                            \
  X(void, Vertex2sv, (const GLshort *v))                                                                               \
  X(void, Vertex3d, (GLdouble x, GLdouble y, GLdouble z))                                                              \
  X(void, Vertex3dv, (const GLdouble *v))                                                                              \
  X(void, Vertex3f, (GLfloat x, GLfloat y, GLfloat z))                                                                 \
  X(void, Vertex3fv, (const GLfloat *v))                                                                               \
  X(void, Vertex3i, (GLint x, GLint y, GLint z))                                                                       \
  X(void, Vertex3iv, (const GLint *v))                                                                                 \
  X(void, Vertex3s, (GLshort x, GLshort y, GLshort z))                                                                 \
  X(void, Vertex3sv, (const GLshort *v))                                                                               \
  X(void, Vertex4d, (GLdouble x, GLdouble y, GLdouble z, GLdouble w))                                                  \
  X(void, Vertex4dv, (const GLdouble *v))                                                                              \
  X(void, Vertex4f, (GLfloat x, GLfloat y, GLfloat z, GLfloat w))                                                      \
  X(void, Vertex4fv, (const GLfloat *v))                                                                               \
  X(void, Vertex4i, (GLint x, GLint y, GLint z, GLint w))                                                              \
  X(void, Vertex4iv, (const GLint *v))                                                                                 \
  X(void, Vertex4s, (GLshort x, GLshort y, GLshort z, GLshort w))                                                      \
  X(void, Vertex4sv, (const GLshort *v))                                                                               \
  X(void, VertexPointer, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer))                             \
  X(void, Viewport, (GLint x, GLint y, GLsizei width, GLsizei height))                                                 \
  X(void, PointParameterfEXT, (GLenum param, GLfloat value))                                                           \
  X(void, PointParameterfvEXT, (GLenum param, const GLfloat *value))                                                   \
  X(void, ColorTableEXT, (int, int, int, int, int, const void *))                                                      \
  X(void, LockArraysEXT, (int, int))                                                                                   \
  X(void, UnlockArraysEXT, (void))                                                                                     \
  X(void, MTexCoord2fSGIS, (GLenum, GLfloat, GLfloat))                                                                 \
  X(void, SelectTextureSGIS, (GLenum))                                                                                 \
                                                                                                                       \
  X(GLuint, CreateShader, (GLenum shaderType))                                                                         \
  X(void, ShaderSource, (GLuint shader, GLsizei count, const GLchar *const *string, const GLint *length))              \
  X(void, CompileShader, (GLuint shader))                                                                              \
  X(void, ReleaseShaderCompiler, (void))                                                                               \
  X(void, DeleteShader, (GLuint shader))                                                                               \
  X(GLboolean, IsShader, (GLuint shader))                                                                              \
  X(void, ShaderBinary,                                                                                                \
    (GLsizei count, const GLuint *shaders, GLenum binary_format, const void *binary, GLsizei length))                  \
                                                                                                                       \
  X(GLuint, CreateProgram, (void))                                                                                     \
  X(void, AttachShader, (GLuint program, GLuint shader))                                                               \
  X(void, DetachShader, (GLuint program, GLuint shader))                                                               \
  X(void, LinkProgram, (GLuint program))                                                                               \
  X(void, UseProgram, (GLuint program))                                                                                \
  X(GLuint, CreateShaderProgramv, (GLenum type, GLsizei count, const char *const *strings))                            \
  X(void, ProgramParameteri, (GLuint program, GLenum pname, GLint value))                                              \
  X(void, DeleteProgram, (GLuint program))                                                                             \
  X(GLboolean, IsProgram, (GLuint program))

#define QGL_EXTERN_X(RESULT, NAME, ARGUMENTS) extern RESULT(APIENTRY *qgl##NAME) ARGUMENTS;
QGL_FUNCTIONS(QGL_EXTERN_X)
#undef QGL_EXTERN_X

/*
** extension constants
*/
#define GL_POINT_SIZE_MIN_EXT 0x8126
#define GL_POINT_SIZE_MAX_EXT 0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE_EXT 0x8128
#define GL_DISTANCE_ATTENUATION_EXT 0x8129

#ifdef __sgi
#define GL_SHARED_TEXTURE_PALETTE_EXT GL_TEXTURE_COLOR_TABLE_SGI
#else
#define GL_SHARED_TEXTURE_PALETTE_EXT 0x81FB
#endif

#define GL_TEXTURE0_SGIS 0x835E
#define GL_TEXTURE1_SGIS 0x835F

#endif
