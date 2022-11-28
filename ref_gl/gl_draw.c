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
  GL_Bind(draw_chars->texnum);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

static struct DrawState draw_state_quad = {.primitive = GL_QUADS,
                                           .blend_enable = true,
                                           .blend_src_factor = GL_SRC_ALPHA,
                                           .blend_dst_factor = GL_ONE_MINUS_SRC_ALPHA};

static struct DrawState draw_state_triangle = {.primitive = GL_TRIANGLES,
                                               .blend_enable = true,
                                               .blend_src_factor = GL_SRC_ALPHA,
                                               .blend_dst_factor = GL_ONE_MINUS_SRC_ALPHA};

/*
================
Draw_Char

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Char(int x, int y, int num) {
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

  GL_begin_draw(&draw_state_quad, &(struct DrawAssets){.images[0] = draw_chars->texnum});

  glColor4f(1, 1, 1, 1);

  glTexCoord2f(fcol, frow);
  glVertex2f(x, y);
  glTexCoord2f(fcol + size, frow);
  glVertex2f(x + 8, y);
  glTexCoord2f(fcol + size, frow + size);
  glVertex2f(x + 8, y + 8);
  glTexCoord2f(fcol, frow + size);
  glVertex2f(x, y + 8);

  GL_end_draw();
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

  GL_begin_draw(&draw_state_quad, &(struct DrawAssets){.images[0] = gl->texnum});

  glColor4f(1, 1, 1, 1);

  glTexCoord2f(gl->base.s0, gl->base.t0);
  glVertex2f(x, y);
  glTexCoord2f(gl->base.s1, gl->base.t0);
  glVertex2f(x + w, y);
  glTexCoord2f(gl->base.s1, gl->base.t1);
  glVertex2f(x + w, y + h);
  glTexCoord2f(gl->base.s0, gl->base.t1);
  glVertex2f(x, y + h);

  GL_end_draw();
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

  GL_begin_draw(&draw_state_quad, &(struct DrawAssets){.images[0] = gl->texnum});

  glColor4f(1, 1, 1, 1);

  glTexCoord2f(gl->base.s0, gl->base.t0);
  glVertex2f(x, y);
  glTexCoord2f(gl->base.s1, gl->base.t0);
  glVertex2f(x + gl->base.width, y);
  glTexCoord2f(gl->base.s1, gl->base.t1);
  glVertex2f(x + gl->base.width, y + gl->base.height);
  glTexCoord2f(gl->base.s0, gl->base.t1);
  glVertex2f(x, y + gl->base.height);

  GL_end_draw();
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

  GL_begin_draw(&draw_state_quad, &(struct DrawAssets){.images[0] = image->texnum});

  glColor4f(1, 1, 1, 1);

  glTexCoord2f(x / 64.0, y / 64.0);
  glVertex2f(x, y);
  glTexCoord2f((x + w) / 64.0, y / 64.0);
  glVertex2f(x + w, y);
  glTexCoord2f((x + w) / 64.0, (y + h) / 64.0);
  glVertex2f(x + w, y + h);
  glTexCoord2f(x / 64.0, (y + h) / 64.0);
  glVertex2f(x, y + h);

  GL_end_draw();
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

  GL_begin_draw(&draw_state_quad, &(struct DrawAssets){.images[0] = r_whitepcx->texnum});

  color.c = d_8to24table[c];
  glColor3f(color.v[0] / 255.0, color.v[1] / 255.0, color.v[2] / 255.0);

  glVertex2f(x, y);
  glVertex2f(x + w, y);
  glVertex2f(x + w, y + h);
  glVertex2f(x, y + h);

  GL_end_draw();
}

//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen(void) {
  GL_begin_draw(&draw_state_quad, &(struct DrawAssets){.images[0] = r_whitepcx->texnum});

  glColor4f(0, 0, 0, 0.8);

  glVertex2f(0, 0);
  glVertex2f(vid.width, 0);
  glVertex2f(vid.width, vid.height);
  glVertex2f(0, vid.height);

  GL_end_draw();
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

  GL_Bind(1);

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

  GL_begin_draw(&draw_state_quad, &(struct DrawAssets){.images[0] = 1});

  glTexCoord2f(0, 0);
  glVertex2f(x, y);
  glTexCoord2f(1, 0);
  glVertex2f(x + w, y);
  glTexCoord2f(1, t);
  glVertex2f(x + w, y + h);
  glTexCoord2f(0, t);
  glVertex2f(x, y + h);

  GL_end_draw();
}

void Draw_Triangles(const struct BaseImage *image, const struct DrawVertex *vertexes, uint32_t num_vertexes,
                    const uint32_t *indexes, uint32_t num_indexes) {
  GL_begin_draw(&draw_state_triangle, &(struct DrawAssets){.images[0] = ((image_t *)image)->texnum});

  for(int i = 0; i < num_indexes; i++) {
    uint32_t index = indexes[i];
    glTexCoord2fv(vertexes[index].st);
    glColor4ub(vertexes[index].rgba[0], vertexes[index].rgba[1], vertexes[index].rgba[2], vertexes[index].rgba[3]);
    glVertex2fv(vertexes[index].xy);
  }

  GL_end_draw();
}
