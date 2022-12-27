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

// draw.c

#include "gl_local.h"

#include "gl_thin.h"

image_t *draw_chars;

/*
===============
Draw_InitLocal
===============
*/
void Draw_InitLocal(void) {
  // load console characters (don't bilerp characters)
  draw_chars = GL_FindImage("pics/conchars.pcx", it_pic);
  glTextureParameterf(draw_chars->texnum, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTextureParameterf(draw_chars->texnum, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

static struct GL_VertexFormat vertex_format = {
    .attribute[0] = {0, alias_memory_Format_Float32, 2, "xy", 0},
    .attribute[1] = {0, alias_memory_Format_Float32, 2, "st", 8},
    .attribute[2] = {0, alias_memory_Format_Unorm8, 4, "rgba", 16},
    .binding[0] = {sizeof(struct DrawVertex)},
};

static struct GL_ImagesFormat images_format = {
    .image[0] = {THIN_GL_FRAGMENT_BIT, GL_ImageType_Sampler2D, "img"},
};

// clang-format off
static char vertex_shader_source[] =
  GL_MSTR(
    layout(location = 0) out vec2 out_st;
    layout(location = 1) out vec4 out_rgba;

    void main() {
      gl_Position = gl_ModelViewProjectionMatrix * vec4(in_xy, 0, 1);
      out_st = in_st;
      out_rgba = in_rgba;
    }
  );

static char fragment_shader_source[] =
  GL_MSTR(
    layout(location = 0) in vec2 in_st;
    layout(location = 1) in vec4 in_rgba;

    layout(location = 0) out vec4 out_color;

    void main() {
      out_color = texture(u_img, in_st) * in_rgba;
    }
  );
// clang-format on

static struct DrawState draw_state = {.primitive = GL_TRIANGLES,
                                      .vertex_format = &vertex_format,
                                      .images_format = &images_format,
                                      .vertex_shader_source = vertex_shader_source,
                                      .fragment_shader_source = fragment_shader_source,
                                      .depth_range_min = 0,
                                      .depth_range_max = 1,
                                      .blend_enable = true,
                                      .blend_src_factor = GL_SRC_ALPHA,
                                      .blend_dst_factor = GL_ONE_MINUS_SRC_ALPHA};

static void draw_triangles_internal(GLuint image, const struct DrawVertex *vertexes, uint32_t num_vertexes,
                                    const uint32_t *indexes, uint32_t num_indexes) {
  struct GL_Buffer element_buffer =
      GL_allocate_temporary_buffer_from(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * num_indexes, indexes);
  struct GL_Buffer vertex_buffer =
      GL_allocate_temporary_buffer_from(GL_ARRAY_BUFFER, sizeof(struct DrawVertex) * num_vertexes, vertexes);

  GL_draw_elements(
      &draw_state,
      &(struct DrawAssets){.image[0] = image, .element_buffer = &element_buffer, .vertex_buffers[0] = &vertex_buffer},
      num_indexes, 1, 0, 0);
}

void Draw_Triangles(const struct BaseImage *image, const struct DrawVertex *vertexes, uint32_t num_vertexes,
                    const uint32_t *indexes, uint32_t num_indexes) {
  draw_triangles_internal(((image_t *)image)->texnum, vertexes, num_vertexes, indexes, num_indexes);
}

void GL_draw_2d_quad(GLuint image, float x1, float y1, float x2, float y2, float s1, float t1, float s2, float t2,
                     float r, float g, float b, float a) {
  struct DrawVertex vertexes[4] = {
      {{x1, y1}, {s1, t1}, {r * 255, g * 255, b * 255, a * 255}},
      {{x1, y2}, {s1, t2}, {r * 255, g * 255, b * 255, a * 255}},
      {{x2, y2}, {s2, t2}, {r * 255, g * 255, b * 255, a * 255}},
      {{x2, y1}, {s2, t1}, {r * 255, g * 255, b * 255, a * 255}},
  };
  uint32_t indexes[6] = {0, 1, 2, 0, 2, 3};
  draw_triangles_internal(image, vertexes, 4, indexes, 6);
}

/*
================
Draw_Char

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Char(int x, int y, int num) {
  struct DrawVertex vertexes[4];

  int row, col;
  float frow, fcol, size;

  num &= 255;

  if((num & 127) == 32)
    return; // space

  if(y <= -8)
    return; // totally off screen

  row = num >> 4;
  col = num & 15;

  frow = row * 0.0625;
  fcol = col * 0.0625;
  size = 0.0625;

  GL_draw_2d_quad(draw_chars->texnum, x, y, x + 8, y + 8, fcol, frow, fcol + size, frow + size, 1, 1, 1, 1);
}

/*
=============
Draw_FindPic
=============
*/
image_t *Draw_FindPic(const char *name) {
  image_t *gl;
  char fullname[MAX_QPATH];

  if(name[0] != '/' && name[0] != '\\') {
    Com_sprintf(fullname, sizeof(fullname), "pics/%s.pcx", name);
    gl = GL_FindImage(fullname, it_pic);
  } else
    gl = GL_FindImage(name + 1, it_pic);

  return gl;
}

/*
=============
Draw_GetPicSize
=============
*/
void Draw_GetPicSize(int *w, int *h, const char *pic) {
  image_t *gl;

  gl = Draw_FindPic(pic);
  if(!gl) {
    *w = *h = -1;
    return;
  }
  *w = gl->base.width;
  *h = gl->base.height;
}

/*
=============
Draw_StretchPic
=============
*/
void Draw_StretchPic(int x, int y, int w, int h, const char *pic) {
  image_t *gl;

  gl = Draw_FindPic(pic);
  if(!gl) {
    ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
    return;
  }

  GL_draw_2d_quad(gl->texnum, x, y, x + w, y + h, gl->base.s0, gl->base.t0, gl->base.s1, gl->base.t1, 1, 1, 1, 1);
}

/*
=============
Draw_Pic
=============
*/
void Draw_Pic(int x, int y, const char *pic) {
  image_t *gl;

  gl = Draw_FindPic(pic);
  if(!gl) {
    ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
    return;
  }

  GL_draw_2d_quad(gl->texnum, x, y, x + gl->base.width, y + gl->base.height, gl->base.s0, gl->base.t0, gl->base.s1,
                  gl->base.t1, 1, 1, 1, 1);
}

/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear(int x, int y, int w, int h, const char *pic) {
  image_t *image;

  image = Draw_FindPic(pic);
  if(!image) {
    ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
    return;
  }

  GL_draw_2d_quad(image->texnum, x, y, x + w, y + h, x / 64.0, y / 64.0, (x + w) / 64.0, (y + h) / 64.0, 1, 1, 1, 1);
}

/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill(int x, int y, int w, int h, int c) {
  union {
    unsigned c;
    byte v[4];
  } color;

  if((unsigned)c > 255)
    ri.Sys_Error(ERR_FATAL, "Draw_Fill: bad color");

  color.c = d_8to24table[c];

  GL_draw_2d_quad(r_whitepcx->texnum, x, y, x + w, y + h, 0, 0, 1, 1, color.v[0] / 255.0, color.v[1] / 255.0,
                  color.v[2] / 255.0, 1);
}

//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen(void) {
  GL_draw_2d_quad(r_whitepcx->texnum, 0, 0, vid.width, vid.height, 0, 0, 1, 1, 0, 0, 0, 0.8);
}

//====================================================================

/*
=============
Draw_StretchRaw
=============
*/
extern unsigned r_rawpalette[256];

void Draw_StretchRaw(int x, int y, int w, int h, int cols, int rows, byte *data) {
  unsigned image32[256 * 256];
  unsigned char image8[256 * 256];
  int i, j, trows;
  byte *source;
  int frac, fracstep;
  float hscale;
  int row;
  float t;

  glBindTexture(GL_TEXTURE_2D, 1);

  if(rows <= 256) {
    hscale = 1;
    trows = rows;
  } else {
    hscale = rows / 256.0;
    trows = 256;
  }
  t = rows * hscale / 256;

  unsigned *dest;

  for(i = 0; i < trows; i++) {
    row = (int)(i * hscale);
    if(row > rows)
      break;
    source = data + cols * row;
    dest = &image32[i * 256];
    fracstep = cols * 0x10000 / 256;
    frac = fracstep >> 1;
    for(j = 0; j < 256; j++) {
      dest[j] = r_rawpalette[source[frac >> 16]];
      frac += fracstep;
    }
  }

  glTexImage2D(GL_TEXTURE_2D, 0, gl_tex_solid_format, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, image32);

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  GL_draw_2d_quad(1, x, y, x + w, y + h, 0, 0, 1, t, 1, 1, 1, 1);
}
