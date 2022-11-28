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

#include "gl_local.h"

union rgba_u32 {
  struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
  };
  uint32_t u;
};

// ================================================================================================================================
struct qoi_decode_state {
  union rgba_u32 previous;
  union rgba_u32 array[64];
  int8_t dg;

  enum {
    qoi_decode_state_initial,
    qoi_decode_state_luma_rb,
    qoi_decode_state_rgb_r,
    qoi_decode_state_rgb_g,
    qoi_decode_state_rgb_b,
    qoi_decode_state_rgba_r,
    qoi_decode_state_rgba_g,
    qoi_decode_state_rgba_b,
    qoi_decode_state_rgba_a,
  } expected;

  void (*emit)(void *ud, union rgba_u32 color);
  void *ud;
};

static inline void qoi_decode_init(struct qoi_decode_state *state, void (*emit)(void *ud, union rgba_u32 color),
                                   void *ud) {
  memset(state, 0, sizeof(*state));
  state->previous.a = 255;
  state->emit = emit;
  state->ud = ud;
}

static inline void qoi_decode_byte(struct qoi_decode_state *state, uint8_t byte) {
  switch(state->expected) {
  case qoi_decode_state_initial:
    if(byte == 0xfe) {
      state->expected = qoi_decode_state_rgb_r;
      return;
    }
    if(byte == 0xff) {
      state->expected = qoi_decode_state_rgba_r;
      return;
    }
    if((byte >> 6) == 0) {
      state->previous = state->array[byte & 0x3f];
    } else if((byte >> 6) == 1) {
      int8_t dr = (byte >> 4) & 0x3;
      int8_t dg = (byte >> 2) & 0x3;
      int8_t db = (byte >> 0) & 0x3;
      state->previous.r += dr - 2;
      state->previous.g += dg - 2;
      state->previous.b += db - 2;
    } else if((byte >> 6) == 2) {
      state->dg = (byte & 0x3f) - 32;
      state->previous.g += state->dg;
      state->expected = qoi_decode_state_luma_rb;
      return;
    } else if((byte >> 6) == 3) {
      uint8_t run = (byte & 0x3f) + 1;
      while(run--) {
        state->emit(state->ud, state->previous);
      }
      return;
    }
    break;
  case qoi_decode_state_luma_rb:
    state->previous.r += state->dg - 8 + (byte >> 4);
    state->previous.b += state->dg - 8 + (byte & 0x0f);
    break;
  case qoi_decode_state_rgb_r:
    state->previous.r = byte;
    state->expected = qoi_decode_state_rgb_g;
    return;
  case qoi_decode_state_rgb_g:
    state->previous.g = byte;
    state->expected = qoi_decode_state_rgb_b;
    return;
  case qoi_decode_state_rgb_b:
    state->previous.b = byte;
    break;
  case qoi_decode_state_rgba_r:
    state->previous.r = byte;
    state->expected = qoi_decode_state_rgba_g;
    return;
  case qoi_decode_state_rgba_g:
    state->previous.g = byte;
    state->expected = qoi_decode_state_rgba_b;
    return;
  case qoi_decode_state_rgba_b:
    state->previous.b = byte;
    state->expected = qoi_decode_state_rgba_a;
    return;
  case qoi_decode_state_rgba_a:
    state->previous.a = byte;
    break;
  }
  state->expected = qoi_decode_state_initial;
  int index_position =
      (state->previous.r * 3 + state->previous.g * 5 + state->previous.b * 7 + state->previous.a * 11) & 63;
  state->array[index_position] = state->previous;
  state->emit(state->ud, state->previous);
}

static inline bool qoi_decode_finish(struct qoi_decode_state *state) {
  return state->expected == qoi_decode_state_initial;
}

struct qoi_encode_state {
  union rgba_u32 previous;
  union rgba_u32 array[64];
  int run;

  void (*emit)(void *ud, const uint8_t *buffer, size_t buffer_length);
  void *ud;
};

static inline void qoi_encode_init(struct qoi_encode_state *state,
                                   void (*emit)(void *ud, const uint8_t *buffer, size_t buffer_length), void *ud) {
  memset(state, 0, sizeof(*state));
  state->previous.a = 255;
  state->emit = emit;
  state->ud = ud;
}

static inline void qoi_encode_pixel(struct qoi_encode_state *state, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  union rgba_u32 pixel;
  pixel.r = r;
  pixel.g = g;
  pixel.b = b;
  pixel.a = a;
  uint8_t buffer[10];
  size_t buffer_length = 0;
  if(pixel.u == state->previous.u) {
    state->run++;
    if(state->run == 62) {
      buffer[buffer_length++] = (3 << 6) | (state->run - 1);
      state->run = 0;
    }
  } else {
    if(state->run > 0) {
      buffer[buffer_length++] = (3 << 6) | (state->run - 1);
      state->run = 0;
    }
    int index_position = (pixel.r * 3 + pixel.g * 5 + pixel.b * 7 + pixel.a * 11) % 64;
    if(state->array[index_position].u == pixel.u) {
      buffer[buffer_length++] = (0 << 6) | index_position;
    } else if(pixel.a != state->previous.a) {
      buffer[buffer_length++] = 0xFF;
      buffer[buffer_length++] = pixel.r;
      buffer[buffer_length++] = pixel.g;
      buffer[buffer_length++] = pixel.b;
      buffer[buffer_length++] = pixel.a;
    } else {
      int8_t dr = pixel.r - state->previous.r;
      int8_t dg = pixel.g - state->previous.g;
      int8_t db = pixel.b - state->previous.b;
      int8_t dr_minus_dg = dr - dg;
      int8_t db_minus_dg = db - dg;
      if(dr > -3 && dr < 2 && dg > -3 && dg < 2 && db > -3 && db < 2) {
        buffer[buffer_length++] = (1 << 6) | (dr + 2) << 4 | (dg + 2) << 2 | (db + 2) << 0;
      } else if(dr_minus_dg > -9 && dr_minus_dg < 8 && dg > -33 && dg < 32 && db_minus_dg > -9 && db_minus_dg < 8) {
        buffer[buffer_length++] = (2 << 6) | (dg + 32);
        buffer[buffer_length++] = (dr_minus_dg + 8) << 4 | (db_minus_dg + 8) << 0;
      } else {
        buffer[buffer_length++] = 0xFE;
        buffer[buffer_length++] = pixel.r;
        buffer[buffer_length++] = pixel.g;
        buffer[buffer_length++] = pixel.b;
      }
    }
    state->array[index_position] = pixel;
    state->previous = pixel;
  }
  if(buffer_length > 0) {
    state->emit(state->ud, buffer, buffer_length);
  }
}

static inline void qoi_encode_finalize(struct qoi_encode_state *state) {
  uint8_t buffer[10];
  size_t buffer_length = 0;
  if(state->run > 0) {
    buffer[buffer_length++] = (3 << 6) | (state->run - 1);
    state->emit(state->ud, buffer, buffer_length);
  }
}

static void encode_qoi_data(int channels, const byte *input, const byte *input_end,
                            void (*emit)(void *ud, const uint8_t *buffer, size_t buffer_length), void *ud) {
  struct qoi_encode_state state;
  qoi_encode_init(&state, emit, ud);
  uint8_t a = 255;
  while(input < input_end) {
    uint8_t r = *input++;
    uint8_t g = *input++;
    uint8_t b = *input++;
    if(channels == 4) {
      a = *input++;
    }
    qoi_encode_pixel(&state, r, g, b, a);
  }
  qoi_encode_finalize(&state);
}

static void check_image_cache_emit_32(void *ud, union rgba_u32 color) {
  uint8_t **out = (uint8_t **)ud;
  *(*out)++ = color.r;
  *(*out)++ = color.g;
  *(*out)++ = color.b;
  *(*out)++ = color.a;
}

static void check_image_cache_emit_24(void *ud, union rgba_u32 color) {
  uint8_t **out = (uint8_t **)ud;
  *(*out)++ = color.r;
  *(*out)++ = color.g;
  *(*out)++ = color.b;
}

static int check_image_cache(const char *name, void **out_image_data, int *out_width, int *out_height) {
  size_t name_length = strlen(name);

  char qoi_path[MAX_QPATH + 12];
  snprintf(qoi_path, sizeof(qoi_path), "cache/%s.qoi", name);

  byte *image;
  int image_length;
  if((image_length = ri.FS_LoadFile(qoi_path, (void **)&image)) < 0) {
    return 0;
  }

  struct {
    char magic[4];
    uint32_t width;
    uint32_t height;
    uint8_t channels;
    uint8_t colorspace;
  } * header;

  *(byte **)&header = image;

  header->width = BigLong(header->width);
  header->height = BigLong(header->height);

  *out_width = header->width;
  *out_height = header->height;

  size_t size = header->width * header->height * header->channels;
  byte *out = malloc(size);

  *out_image_data = out;

  struct qoi_decode_state state;
  qoi_decode_init(&state, header->channels == 3 ? check_image_cache_emit_24 : check_image_cache_emit_32, &out);

  const uint8_t *input = (uint8_t *)(image + 14);
  const uint8_t *input_end = input + (image_length - 14 - 8);

  while(input < input_end) {
    qoi_decode_byte(&state, *input++);
  }

  qoi_decode_finish(&state);

  return header->channels;
}

static void insert_into_image_cache_emit(void *ud, const uint8_t *buffer, size_t buffer_length) {
  FILE *f = (FILE *)ud;
  fwrite(buffer, buffer_length, 1, f);
}

static void insert_into_image_cache(const char *name, const byte *data, uint32_t width, uint32_t height, int channels) {
  extern void FS_CreatePath(const char *path_);

  char path[MAX_QPATH];

  if(channels != 3 && channels != 4)
    return;

  snprintf(path, sizeof(path), "%s/cache/%s.qoi", ri.FS_Gamedir(), name);

  FS_CreatePath(path);

  FILE *f = fopen(path, "wb");
  if(f == NULL) {
    return;
  }

  struct {
    char magic[4];
    uint32_t width;
    uint32_t height;
    uint8_t channels;
    uint8_t colorspace;
  } header;

  byte footer[] = {0, 0, 0, 0, 0, 0, 0, 1};

  header.magic[0] = 'q';
  header.magic[1] = 'o';
  header.magic[2] = 'i';
  header.magic[3] = 'f';
  header.width = BigLong(width);
  header.height = BigLong(height);
  header.channels = channels;
  header.colorspace = 1;

  fwrite(&header.magic, 4, 1, f);
  fwrite(&header.width, 4, 1, f);
  fwrite(&header.height, 4, 1, f);
  fwrite(&header.channels, 1, 1, f);
  fwrite(&header.colorspace, 1, 1, f);

  encode_qoi_data(channels, data, data + width * height * channels, insert_into_image_cache_emit, f);

  fwrite(&footer, sizeof(footer), 1, f);

  fclose(f);
}
// ================================================================================================================================

image_t gltextures[MAX_GLTEXTURES];
int numgltextures;
int base_textureid; // gltextures[i] = base_textureid+i

static byte intensitytable[256];
static unsigned char gammatable[256];

cvar_t *intensity;

unsigned d_8to24table[256];

bool GL_Upload8(byte *data, int width, int height, bool mipmap, bool is_sky);
bool GL_Upload32(unsigned *data, int width, int height, bool mipmap, int channels);

int gl_solid_format = 3;
int gl_alpha_format = 4;

int gl_tex_solid_format = 3;
int gl_tex_alpha_format = 4;

int gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int gl_filter_max = GL_LINEAR;

void GL_SetTexturePalette(unsigned palette[256]) {
  int i;
  unsigned char temptable[768];

  for(i = 0; i < 256; i++) {
    temptable[i * 3 + 0] = (palette[i] >> 0) & 0xff;
    temptable[i * 3 + 1] = (palette[i] >> 8) & 0xff;
    temptable[i * 3 + 2] = (palette[i] >> 16) & 0xff;
  }
}

void GL_EnableMultitexture(bool enable) {
  if(enable) {
    GL_SelectTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    GL_TexEnv(GL_REPLACE);
  } else {
    GL_SelectTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);
    GL_TexEnv(GL_REPLACE);
  }
  GL_SelectTexture(GL_TEXTURE0);
  GL_TexEnv(GL_REPLACE);
}

void GL_SelectTexture(GLenum texture) {
  int tmu = texture - GL_TEXTURE0;

  if(tmu == gl_state.currenttmu)
    return;

  gl_state.currenttmu = tmu;
  glActiveTexture(texture);
}

void GL_TexEnv(GLenum mode) {
  static int lastmodes[2] = {-1, -1};

  if(mode != lastmodes[gl_state.currenttmu]) {
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
    lastmodes[gl_state.currenttmu] = mode;
  }
}

void GL_Bind(int texnum) {
  extern image_t *draw_chars;

  if(gl_nobind->value && draw_chars) // performance evaluation option
    texnum = draw_chars->texnum;
  if(gl_state.currenttextures[gl_state.currenttmu] == texnum)
    return;
  gl_state.currenttextures[gl_state.currenttmu] = texnum;
  glBindTexture(GL_TEXTURE_2D, texnum);
}

void GL_MBind(GLenum target, int texnum) {
  GL_SelectTexture(target);
  GL_Bind(texnum);
}

typedef struct {
  char *name;
  int minimize, maximize;
} glmode_t;

glmode_t modes[] = {{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
                    {"GL_LINEAR", GL_LINEAR, GL_LINEAR},
                    {"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
                    {"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
                    {"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
                    {"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}};

#define NUM_GL_MODES (sizeof(modes) / sizeof(glmode_t))

typedef struct {
  char *name;
  int mode;
} gltmode_t;

gltmode_t gl_alpha_modes[] = {
    {"default", 4},         {"GL_RGBA", GL_RGBA},   {"GL_RGBA8", GL_RGBA8}, {"GL_RGB5_A1", GL_RGB5_A1},
    {"GL_RGBA4", GL_RGBA4}, {"GL_RGBA2", GL_RGBA2},
};

#define NUM_GL_ALPHA_MODES (sizeof(gl_alpha_modes) / sizeof(gltmode_t))

gltmode_t gl_solid_modes[] = {
    {"default", 3},           {"GL_RGB", GL_RGB},   {"GL_RGB8", GL_RGB8},
    {"GL_RGB5", GL_RGB5},     {"GL_RGB4", GL_RGB4}, {"GL_R3_G3_B2", GL_R3_G3_B2},
#ifdef GL_RGB2_EXT
    {"GL_RGB2", GL_RGB2_EXT},
#endif
};

#define NUM_GL_SOLID_MODES (sizeof(gl_solid_modes) / sizeof(gltmode_t))

/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode(char *string) {
  int i;
  image_t *glt;

  for(i = 0; i < NUM_GL_MODES; i++) {
    if(!Q_stricmp(modes[i].name, string))
      break;
  }

  if(i == NUM_GL_MODES) {
    ri.Con_Printf(PRINT_ALL, "bad filter name\n");
    return;
  }

  gl_filter_min = modes[i].minimize;
  gl_filter_max = modes[i].maximize;

  // change all the existing mipmap texture objects
  for(i = 0, glt = gltextures; i < numgltextures; i++, glt++) {
    if(glt->type != it_pic && glt->type != it_sky) {
      GL_Bind(glt->texnum);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
    }
  }
}

/*
===============
GL_TextureAlphaMode
===============
*/
void GL_TextureAlphaMode(char *string) {
  int i;

  for(i = 0; i < NUM_GL_ALPHA_MODES; i++) {
    if(!Q_stricmp(gl_alpha_modes[i].name, string))
      break;
  }

  if(i == NUM_GL_ALPHA_MODES) {
    ri.Con_Printf(PRINT_ALL, "bad alpha texture mode name\n");
    return;
  }

  gl_tex_alpha_format = gl_alpha_modes[i].mode;
}

/*
===============
GL_TextureSolidMode
===============
*/
void GL_TextureSolidMode(char *string) {
  int i;

  for(i = 0; i < NUM_GL_SOLID_MODES; i++) {
    if(!Q_stricmp(gl_solid_modes[i].name, string))
      break;
  }

  if(i == NUM_GL_SOLID_MODES) {
    ri.Con_Printf(PRINT_ALL, "bad solid texture mode name\n");
    return;
  }

  gl_tex_solid_format = gl_solid_modes[i].mode;
}

/*
===============
GL_ImageList_f
===============
*/
void GL_ImageList_f(void) {
  int i;
  image_t *image;
  int texels;
  const char *palstrings[2] = {"RGB", "PAL"};

  ri.Con_Printf(PRINT_ALL, "------------------\n");
  texels = 0;

  for(i = 0, image = gltextures; i < numgltextures; i++, image++) {
    if(image->texnum <= 0)
      continue;
    texels += image->upload_width * image->upload_height;
    switch(image->type) {
    case it_skin:
      ri.Con_Printf(PRINT_ALL, "M");
      break;
    case it_sprite:
      ri.Con_Printf(PRINT_ALL, "S");
      break;
    case it_wall:
      ri.Con_Printf(PRINT_ALL, "W");
      break;
    case it_pic:
      ri.Con_Printf(PRINT_ALL, "P");
      break;
    default:
      ri.Con_Printf(PRINT_ALL, " ");
      break;
    }

    ri.Con_Printf(PRINT_ALL, " %3i %3i %s: %s\n", image->upload_width, image->upload_height,
                  palstrings[image->paletted], image->base.name);
  }
  ri.Con_Printf(PRINT_ALL, "Total texel count (not counting mipmaps): %i\n", texels);
}

/*
=================================================================

PCX LOADING

=================================================================
*/

/*
==============
LoadPCX
==============
*/
void LoadPCX(const char *filename, byte **pic, byte **palette, int *width, int *height) {
  byte *raw;
  pcx_t *pcx;
  int x, y;
  int len;
  int dataByte, runLength;
  byte *out, *pix;

  *pic = NULL;
  *palette = NULL;

  //
  // load the file
  //
  len = ri.FS_LoadFile(filename, (void **)&raw);
  if(!raw) {
    ri.Con_Printf(PRINT_DEVELOPER, "Bad pcx file %s\n", filename);
    return;
  }

  //
  // parse the PCX file
  //
  pcx = (pcx_t *)raw;

  pcx->xmin = LittleShort(pcx->xmin);
  pcx->ymin = LittleShort(pcx->ymin);
  pcx->xmax = LittleShort(pcx->xmax);
  pcx->ymax = LittleShort(pcx->ymax);
  pcx->hres = LittleShort(pcx->hres);
  pcx->vres = LittleShort(pcx->vres);
  pcx->bytes_per_line = LittleShort(pcx->bytes_per_line);
  pcx->palette_type = LittleShort(pcx->palette_type);

  raw = &pcx->data;

  if(pcx->manufacturer != 0x0a || pcx->version != 5 || pcx->encoding != 1 || pcx->bits_per_pixel != 8 ||
     pcx->xmax >= 640 || pcx->ymax >= 480) {
    ri.Con_Printf(PRINT_ALL, "Bad pcx file %s\n", filename);
    return;
  }

  out = malloc((pcx->ymax + 1) * (pcx->xmax + 1));

  *pic = out;

  pix = out;

  if(palette) {
    *palette = malloc(768);
    memcpy(*palette, (byte *)pcx + len - 768, 768);
  }

  if(width)
    *width = pcx->xmax + 1;
  if(height)
    *height = pcx->ymax + 1;

  for(y = 0; y <= pcx->ymax; y++, pix += pcx->xmax + 1) {
    for(x = 0; x <= pcx->xmax;) {
      dataByte = *raw++;

      if((dataByte & 0xC0) == 0xC0) {
        runLength = dataByte & 0x3F;
        dataByte = *raw++;
      } else
        runLength = 1;

      while(runLength-- > 0)
        pix[x++] = dataByte;
    }
  }

  if(raw - (byte *)pcx > len) {
    ri.Con_Printf(PRINT_DEVELOPER, "PCX file %s was malformed", filename);
    free(*pic);
    *pic = NULL;
  }

  ri.FS_FreeFile(pcx);
}

/*
=========================================================

TARGA LOADING

=========================================================
*/

typedef struct _TargaHeader {
  unsigned char id_length, colormap_type, image_type;
  unsigned short colormap_index, colormap_length;
  unsigned char colormap_size;
  unsigned short x_origin, y_origin, width, height;
  unsigned char pixel_size, attributes;
} TargaHeader;

/*
=============
LoadTGA
=============
*/
void LoadTGA(const char *name, byte **pic, int *width, int *height) {
  int columns, rows, numPixels;
  byte *pixbuf;
  int row, column;
  byte *buf_p;
  byte *buffer;
  int length;
  TargaHeader targa_header;
  byte *targa_rgba;
  byte tmp[2];

  *pic = NULL;

  //
  // load the file
  //
  length = ri.FS_LoadFile(name, (void **)&buffer);
  if(!buffer) {
    ri.Con_Printf(PRINT_DEVELOPER, "Bad tga file %s\n", name);
    return;
  }

  buf_p = buffer;

  targa_header.id_length = *buf_p++;
  targa_header.colormap_type = *buf_p++;
  targa_header.image_type = *buf_p++;

  tmp[0] = buf_p[0];
  tmp[1] = buf_p[1];
  targa_header.colormap_index = LittleShort(*((short *)tmp));
  buf_p += 2;
  tmp[0] = buf_p[0];
  tmp[1] = buf_p[1];
  targa_header.colormap_length = LittleShort(*((short *)tmp));
  buf_p += 2;
  targa_header.colormap_size = *buf_p++;
  targa_header.x_origin = LittleShort(*((short *)buf_p));
  buf_p += 2;
  targa_header.y_origin = LittleShort(*((short *)buf_p));
  buf_p += 2;
  targa_header.width = LittleShort(*((short *)buf_p));
  buf_p += 2;
  targa_header.height = LittleShort(*((short *)buf_p));
  buf_p += 2;
  targa_header.pixel_size = *buf_p++;
  targa_header.attributes = *buf_p++;

  if(targa_header.image_type != 2 && targa_header.image_type != 10)
    ri.Sys_Error(ERR_DROP, "LoadTGA: Only type 2 and 10 targa RGB images supported\n");

  if(targa_header.colormap_type != 0 || (targa_header.pixel_size != 32 && targa_header.pixel_size != 24))
    ri.Sys_Error(ERR_DROP, "LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");

  columns = targa_header.width;
  rows = targa_header.height;
  numPixels = columns * rows;

  if(width)
    *width = columns;
  if(height)
    *height = rows;

  targa_rgba = malloc(numPixels * 4);
  *pic = targa_rgba;

  if(targa_header.id_length != 0)
    buf_p += targa_header.id_length; // skip TARGA image comment

  if(targa_header.image_type == 2) { // Uncompressed, RGB images
    for(row = rows - 1; row >= 0; row--) {
      pixbuf = targa_rgba + row * columns * 4;
      for(column = 0; column < columns; column++) {
        unsigned char red, green, blue, alphabyte;
        switch(targa_header.pixel_size) {
        case 24:

          blue = *buf_p++;
          green = *buf_p++;
          red = *buf_p++;
          *pixbuf++ = red;
          *pixbuf++ = green;
          *pixbuf++ = blue;
          *pixbuf++ = 255;
          break;
        case 32:
          blue = *buf_p++;
          green = *buf_p++;
          red = *buf_p++;
          alphabyte = *buf_p++;
          *pixbuf++ = red;
          *pixbuf++ = green;
          *pixbuf++ = blue;
          *pixbuf++ = alphabyte;
          break;
        }
      }
    }
  } else if(targa_header.image_type == 10) { // Runlength encoded RGB images
    unsigned char red, green, blue, alphabyte, packetHeader, packetSize, j;
    for(row = rows - 1; row >= 0; row--) {
      pixbuf = targa_rgba + row * columns * 4;
      for(column = 0; column < columns;) {
        packetHeader = *buf_p++;
        packetSize = 1 + (packetHeader & 0x7f);
        if(packetHeader & 0x80) { // run-length packet
          switch(targa_header.pixel_size) {
          case 24:
            blue = *buf_p++;
            green = *buf_p++;
            red = *buf_p++;
            alphabyte = 255;
            break;
          case 32:
            blue = *buf_p++;
            green = *buf_p++;
            red = *buf_p++;
            alphabyte = *buf_p++;
            break;
          }

          for(j = 0; j < packetSize; j++) {
            *pixbuf++ = red;
            *pixbuf++ = green;
            *pixbuf++ = blue;
            *pixbuf++ = alphabyte;
            column++;
            if(column == columns) { // run spans across rows
              column = 0;
              if(row > 0)
                row--;
              else
                goto breakOut;
              pixbuf = targa_rgba + row * columns * 4;
            }
          }
        } else { // non run-length packet
          for(j = 0; j < packetSize; j++) {
            switch(targa_header.pixel_size) {
            case 24:
              blue = *buf_p++;
              green = *buf_p++;
              red = *buf_p++;
              *pixbuf++ = red;
              *pixbuf++ = green;
              *pixbuf++ = blue;
              *pixbuf++ = 255;
              break;
            case 32:
              blue = *buf_p++;
              green = *buf_p++;
              red = *buf_p++;
              alphabyte = *buf_p++;
              *pixbuf++ = red;
              *pixbuf++ = green;
              *pixbuf++ = blue;
              *pixbuf++ = alphabyte;
              break;
            }
            column++;
            if(column == columns) { // pixel packet run spans across rows
              column = 0;
              if(row > 0)
                row--;
              else
                goto breakOut;
              pixbuf = targa_rgba + row * columns * 4;
            }
          }
        }
      }
    breakOut:;
    }
  }

  ri.FS_FreeFile(buffer);
}

/*
====================================================================

IMAGE FLOOD FILLING

====================================================================
*/

/*
=================
Mod_FloodFillSkin

Fill background pixels so mipmapping doesn't have haloes
=================
*/

typedef struct {
  short x, y;
} floodfill_t;

// must be a power of 2
#define FLOODFILL_FIFO_SIZE 0x1000
#define FLOODFILL_FIFO_MASK (FLOODFILL_FIFO_SIZE - 1)

#define FLOODFILL_STEP(off, dx, dy)                                                                                    \
  {                                                                                                                    \
    if(pos[off] == fillcolor) {                                                                                        \
      pos[off] = 255;                                                                                                  \
      fifo[inpt].x = x + (dx), fifo[inpt].y = y + (dy);                                                                \
      inpt = (inpt + 1) & FLOODFILL_FIFO_MASK;                                                                         \
    } else if(pos[off] != 255)                                                                                         \
      fdc = pos[off];                                                                                                  \
  }

void R_FloodFillSkin(byte *skin, int skinwidth, int skinheight) {
  byte fillcolor = *skin; // assume this is the pixel to fill
  floodfill_t fifo[FLOODFILL_FIFO_SIZE];
  int inpt = 0, outpt = 0;
  int filledcolor = -1;
  int i;

  if(filledcolor == -1) {
    filledcolor = 0;
    // attempt to find opaque black
    for(i = 0; i < 256; ++i)
      if(d_8to24table[i] == (255 << 0)) // alpha 1.0
      {
        filledcolor = i;
        break;
      }
  }

  // can't fill to filled color or to transparent color (used as visited marker)
  if((fillcolor == filledcolor) || (fillcolor == 255)) {
    // printf( "not filling skin from %d to %d\n", fillcolor, filledcolor );
    return;
  }

  fifo[inpt].x = 0, fifo[inpt].y = 0;
  inpt = (inpt + 1) & FLOODFILL_FIFO_MASK;

  while(outpt != inpt) {
    int x = fifo[outpt].x, y = fifo[outpt].y;
    int fdc = filledcolor;
    byte *pos = &skin[x + skinwidth * y];

    outpt = (outpt + 1) & FLOODFILL_FIFO_MASK;

    if(x > 0)
      FLOODFILL_STEP(-1, -1, 0);
    if(x < skinwidth - 1)
      FLOODFILL_STEP(1, 1, 0);
    if(y > 0)
      FLOODFILL_STEP(-skinwidth, 0, -1);
    if(y < skinheight - 1)
      FLOODFILL_STEP(skinwidth, 0, 1);
    skin[x + skinwidth * y] = fdc;
  }
}

//=======================================================

/*
================
GL_ResampleTexture
================
*/
void GL_ResampleTexture(unsigned *in, int inwidth, int inheight, unsigned *out, int outwidth, int outheight) {
  int i, j;
  unsigned *inrow, *inrow2;
  unsigned frac, fracstep;
  unsigned p1[1024], p2[1024];
  byte *pix1, *pix2, *pix3, *pix4;

  fracstep = inwidth * 0x10000 / outwidth;

  frac = fracstep >> 2;
  for(i = 0; i < outwidth; i++) {
    p1[i] = 4 * (frac >> 16);
    frac += fracstep;
  }
  frac = 3 * (fracstep >> 2);
  for(i = 0; i < outwidth; i++) {
    p2[i] = 4 * (frac >> 16);
    frac += fracstep;
  }

  for(i = 0; i < outheight; i++, out += outwidth) {
    inrow = in + inwidth * (int)((i + 0.25) * inheight / outheight);
    inrow2 = in + inwidth * (int)((i + 0.75) * inheight / outheight);
    frac = fracstep >> 1;
    for(j = 0; j < outwidth; j++) {
      pix1 = (byte *)inrow + p1[j];
      pix2 = (byte *)inrow + p2[j];
      pix3 = (byte *)inrow2 + p1[j];
      pix4 = (byte *)inrow2 + p2[j];
      ((byte *)(out + j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0]) >> 2;
      ((byte *)(out + j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1]) >> 2;
      ((byte *)(out + j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2]) >> 2;
      ((byte *)(out + j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3]) >> 2;
    }
  }
}

/*
================
GL_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void GL_LightScaleTexture(unsigned *in, int inwidth, int inheight, bool only_gamma) {
  if(only_gamma) {
    int i, c;
    byte *p;

    p = (byte *)in;

    c = inwidth * inheight;
    for(i = 0; i < c; i++, p += 4) {
      p[0] = gammatable[p[0]];
      p[1] = gammatable[p[1]];
      p[2] = gammatable[p[2]];
    }
  } else {
    int i, c;
    byte *p;

    p = (byte *)in;

    c = inwidth * inheight;
    for(i = 0; i < c; i++, p += 4) {
      p[0] = gammatable[intensitytable[p[0]]];
      p[1] = gammatable[intensitytable[p[1]]];
      p[2] = gammatable[intensitytable[p[2]]];
    }
  }
}

/*
===============
GL_Upload32

Returns has_alpha
===============
*/
void GL_BuildPalettedTexture(unsigned char *paletted_texture, unsigned char *scaled, int scaled_width,
                             int scaled_height) {
  int i;

  for(i = 0; i < scaled_width * scaled_height; i++) {
    unsigned int r, g, b, c;

    r = (scaled[0] >> 3) & 31;
    g = (scaled[1] >> 2) & 63;
    b = (scaled[2] >> 3) & 31;

    c = r | (g << 5) | (b << 11);

    paletted_texture[i] = gl_state.d_16to8table[c];

    scaled += 4;
  }
}

int upload_width, upload_height;
bool uploaded_paletted;

bool GL_Upload32(unsigned *data, int width, int height, bool mipmap, int channels) {
  int i, c;
  byte *scan;
  int comp;
  GLenum format = channels == 3 ? GL_RGB : GL_RGBA;

  if(channels != 3 && channels != 4)
    ri.Sys_Error(ERR_FATAL, "wierd channel count");

  // scan the texture for any non-255 alpha
  comp = gl_tex_solid_format;
  if(channels == 4) {
    c = width * height;
    scan = ((byte *)data) + 3;
    for(i = 0; i < c; i++, scan += 4) {
      if(*scan != 255) {
        comp = gl_tex_alpha_format;
        break;
      }
    }
  }

  upload_width = width;
  upload_height = height;
  uploaded_paletted = false;

  glTexImage2D(GL_TEXTURE_2D, 0, comp, width, height, 0, format, GL_UNSIGNED_BYTE, data);

  if(mipmap) {
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
  } else {
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
  }

  return (comp == gl_tex_alpha_format);
}

/*
================
GL_LoadPic

This is also used as an entry point for the generated r_notexture
================
*/
image_t *GL_LoadPic(const char *name, byte *pic, int width, int height, imagetype_t type, int bits, bool cache) {
  image_t *image;
  int i;

  // find a free image_t
  for(i = 0, image = gltextures; i < numgltextures; i++, image++) {
    if(!image->texnum)
      break;
  }
  if(i == numgltextures) {
    if(numgltextures == MAX_GLTEXTURES)
      ri.Sys_Error(ERR_DROP, "MAX_GLTEXTURES");
    numgltextures++;
  }
  image = &gltextures[i];

  if(strlen(name) >= sizeof(image->base.name))
    ri.Sys_Error(ERR_DROP, "Draw_LoadPic: \"%s\" is too long", name);

  strcpy(image->base.name, name);
  image->registration_sequence = registration_sequence;
  image->base.width = width;
  image->base.height = height;
  image->type = type;

  int channels = bits / 8;

  byte *new_pic = NULL;
  if(channels == 1) {
    channels = memchr(pic, 255, width * height) == NULL ? 3 : 4;
    new_pic = malloc(width * height * channels);

    byte *out = new_pic;
    for(i = 0; i < width * height; i++) {
      union rgba_u32 p;
      p.u = d_8to24table[pic[i]];
      *out++ = p.r;
      *out++ = p.g;
      *out++ = p.b;
      if(channels == 4) {
        *out++ = p.a;
      }
    }

    pic = new_pic;

    bits = channels * 8;
  }

  if(cache) {
    insert_into_image_cache(name, pic, width, height, channels);
  }

  image->scrap = false;
  image->texnum = TEXNUM_IMAGES + (image - gltextures);
  GL_Bind(image->texnum);
  image->has_alpha =
      GL_Upload32((unsigned *)pic, width, height, (image->type != it_pic && image->type != it_sky), channels);
  image->upload_width = upload_width; // after power of 2 and scales
  image->upload_height = upload_height;
  image->paletted = uploaded_paletted;
  image->base.s0 = 0;
  image->base.s1 = 1;
  image->base.t0 = 0;
  image->base.t1 = 1;

  if(new_pic != NULL) {
    free(new_pic);
  }

  return image;
}

/*
================
GL_LoadWal
================
*/
image_t *GL_LoadWal(const char *name) {
  miptex_t *mt;
  int width, height, ofs;
  image_t *image;

  ri.FS_LoadFile(name, (void **)&mt);
  if(!mt) {
    ri.Con_Printf(PRINT_ALL, "GL_FindImage: can't load %s\n", name);
    return r_notexture;
  }

  width = LittleLong(mt->width);
  height = LittleLong(mt->height);
  ofs = LittleLong(mt->offsets[0]);

  image = GL_LoadPic(name, (byte *)mt + ofs, width, height, it_wall, 8, true);

  ri.FS_FreeFile((void *)mt);

  return image;
}

/*
===============
GL_FindImage

Finds or loads the given image
===============
*/
image_t *GL_FindImage(const char *name, imagetype_t type) {
  image_t *image;
  int i, len;
  byte *pic, *palette;
  int width, height;

  if(!name)
    return NULL; //	ri.Sys_Error (ERR_DROP, "GL_FindImage: NULL name");
  len = strlen(name);
  if(len < 5)
    return NULL; //	ri.Sys_Error (ERR_DROP, "GL_FindImage: bad name: %s", name);

  // look for it
  for(i = 0, image = gltextures; i < numgltextures; i++, image++) {
    if(!strcmp(name, image->base.name)) {
      image->registration_sequence = registration_sequence;
      return image;
    }
  }

  if((i = check_image_cache(name, (void **)&pic, &width, &height)) != 0) {
    image = GL_LoadPic(name, pic, width, height, type, i * 8, false);
    free(pic);
    return image;
  }

  //
  // load the pic from disk
  //
  pic = NULL;
  palette = NULL;
  if(!strcmp(name + len - 4, ".pcx")) {
    LoadPCX(name, &pic, &palette, &width, &height);
    if(!pic)
      return NULL; // ri.Sys_Error (ERR_DROP, "GL_FindImage: can't load %s", name);
    image = GL_LoadPic(name, pic, width, height, type, 8, true);
  } else if(!strcmp(name + len - 4, ".wal")) {
    image = GL_LoadWal(name);
  } else if(!strcmp(name + len - 4, ".tga")) {
    LoadTGA(name, &pic, &width, &height);
    if(!pic)
      return NULL; // ri.Sys_Error (ERR_DROP, "GL_FindImage: can't load %s", name);
    image = GL_LoadPic(name, pic, width, height, type, 32, true);
  } else
    return NULL; //	ri.Sys_Error (ERR_DROP, "GL_FindImage: bad extension on: %s", name);

  if(pic)
    free(pic);
  if(palette)
    free(palette);

  return image;
}

/*
===============
R_RegisterSkin
===============
*/
struct BaseImage *R_RegisterSkin(const char *name) {
  return &GL_FindImage(name, it_skin)->base;
}

/*
================
GL_FreeUnusedImages

Any image that was not touched on this registration sequence
will be freed.
================
*/
void GL_FreeUnusedImages(void) {
  int i;
  image_t *image;

  // never free r_notexture or particle texture
  r_notexture->registration_sequence = registration_sequence;
  r_nonormal->registration_sequence = registration_sequence;
  r_whitepcx->registration_sequence = registration_sequence;
  r_particletexture->registration_sequence = registration_sequence;

  for(i = 0, image = gltextures; i < numgltextures; i++, image++) {
    if(image->registration_sequence == registration_sequence)
      continue; // used this sequence
    if(!image->registration_sequence)
      continue; // free image_t slot
    if(image->type == it_pic)
      continue; // don't free pics
    // free it
    glDeleteTextures(1, &image->texnum);
    memset(image, 0, sizeof(*image));
  }
}

/*
===============
Draw_GetPalette
===============
*/
int Draw_GetPalette(void) {
  int i;
  int r, g, b;
  unsigned v;
  byte *pic, *pal;
  int width, height;

  // get the palette

  LoadPCX("pics/colormap.pcx", &pic, &pal, &width, &height);
  if(!pal)
    ri.Sys_Error(ERR_FATAL, "Couldn't load pics/colormap.pcx");

  for(i = 0; i < 256; i++) {
    r = pal[i * 3 + 0];
    g = pal[i * 3 + 1];
    b = pal[i * 3 + 2];

    v = (255 << 24) + (r << 0) + (g << 8) + (b << 16);
    d_8to24table[i] = LittleLong(v);
  }

  d_8to24table[255] &= LittleLong(0xffffff); // 255 is transparent

  free(pic);
  free(pal);

  return 0;
}

/*
===============
GL_InitImages
===============
*/
void GL_InitImages(void) {
  int i, j;
  float g = vid_gamma->value;

  registration_sequence = 1;

  // init intensity conversions
  intensity = ri.Cvar_Get("intensity", "2", 0);

  if(intensity->value <= 1)
    ri.Cvar_Set("intensity", "1");

  gl_state.inverse_intensity = 1 / intensity->value;

  Draw_GetPalette();

  if(gl_config.renderer & (GL_RENDERER_VOODOO | GL_RENDERER_VOODOO2)) {
    g = 1.0F;
  }

  for(i = 0; i < 256; i++) {
    if(g == 1) {
      gammatable[i] = i;
    } else {
      float inf;

      inf = 255 * pow((i + 0.5) / 255.5, g) + 0.5;
      if(inf < 0)
        inf = 0;
      if(inf > 255)
        inf = 255;
      gammatable[i] = inf;
    }
  }

  for(i = 0; i < 256; i++) {
    j = i * intensity->value;
    if(j > 255)
      j = 255;
    intensitytable[i] = j;
  }
}

/*
===============
GL_ShutdownImages
===============
*/
void GL_ShutdownImages(void) {
  int i;
  image_t *image;

  for(i = 0, image = gltextures; i < numgltextures; i++, image++) {
    if(!image->registration_sequence)
      continue; // free image_t slot
    // free it
    glDeleteTextures(1, &image->texnum);
    memset(image, 0, sizeof(*image));
  }
}
