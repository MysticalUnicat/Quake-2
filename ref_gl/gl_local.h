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
// disable data conversion warnings

#if 0
#pragma warning(disable : 4244) // MIPS
#pragma warning(disable : 4136) // X86
#pragma warning(disable : 4051) // ALPHA
#endif

#include "../client/ref.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <math.h>

#include "glad.h"

bool QGL_Init(void);
void QGL_Shutdown(void);

#define REF_VERSION "GL 0.01"

// up / down
#define PITCH 0

// left / right
#define YAW 1

// fall over
#define ROLL 2

typedef struct {
  unsigned width, height; // coordinates from main game
} viddef_t;

extern viddef_t vid;

struct SH1 {
  float f[12];
};

static inline struct SH1 SH1_Clear(void) { return (struct SH1){{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}; }

static inline struct SH1 SH1_FromDirectionalLight(const float direction[3], const float color[3]) {
  return (struct SH1){.f[0] = color[0],
                      .f[1] = direction[0] * color[0],
                      .f[2] = direction[1] * color[0],
                      .f[3] = direction[2] * color[0],
                      .f[4] = color[1],
                      .f[5] = direction[0] * color[1],
                      .f[6] = direction[1] * color[1],
                      .f[7] = direction[2] * color[1],
                      .f[8] = color[2],
                      .f[9] = direction[0] * color[2],
                      .f[10] = direction[1] * color[2],
                      .f[11] = direction[2] * color[2]};
}

static inline struct SH1 SH1_Add(const struct SH1 sh1_left, const struct SH1 sh1_right) {
  return (struct SH1){.f[0] = sh1_left.f[0] + sh1_right.f[0],
                      .f[1] = sh1_left.f[1] + sh1_right.f[1],
                      .f[2] = sh1_left.f[2] + sh1_right.f[2],
                      .f[3] = sh1_left.f[3] + sh1_right.f[3],
                      .f[4] = sh1_left.f[4] + sh1_right.f[4],
                      .f[5] = sh1_left.f[5] + sh1_right.f[5],
                      .f[6] = sh1_left.f[6] + sh1_right.f[6],
                      .f[7] = sh1_left.f[7] + sh1_right.f[7],
                      .f[8] = sh1_left.f[8] + sh1_right.f[8],
                      .f[9] = sh1_left.f[9] + sh1_right.f[9],
                      .f[10] = sh1_left.f[10] + sh1_right.f[10],
                      .f[11] = sh1_left.f[11] + sh1_right.f[11]};
}

static inline struct SH1 SH1_Scale(const struct SH1 sh1, float scale) {
  return (struct SH1){.f[0] = sh1.f[0] * scale,
                      .f[1] = sh1.f[1] * scale,
                      .f[2] = sh1.f[2] * scale,
                      .f[3] = sh1.f[3] * scale,
                      .f[4] = sh1.f[4] * scale,
                      .f[5] = sh1.f[5] * scale,
                      .f[6] = sh1.f[6] * scale,
                      .f[7] = sh1.f[7] * scale,
                      .f[8] = sh1.f[8] * scale,
                      .f[9] = sh1.f[9] * scale,
                      .f[10] = sh1.f[10] * scale,
                      .f[11] = sh1.f[11] * scale};
}

static inline struct SH1 SH1_ColorScale(const struct SH1 sh1, const float scale[3]) {
  return (struct SH1){.f[0] = sh1.f[0] * scale[0],
                      .f[1] = sh1.f[1] * scale[0],
                      .f[2] = sh1.f[2] * scale[0],
                      .f[3] = sh1.f[3] * scale[0],
                      .f[4] = sh1.f[4] * scale[1],
                      .f[5] = sh1.f[5] * scale[1],
                      .f[6] = sh1.f[6] * scale[1],
                      .f[7] = sh1.f[7] * scale[1],
                      .f[8] = sh1.f[8] * scale[2],
                      .f[9] = sh1.f[9] * scale[2],
                      .f[10] = sh1.f[10] * scale[2],
                      .f[11] = sh1.f[11] * scale[2]};
}

static inline struct SH1 SH1_Reflect(const struct SH1 sh1, const float normal[3]) {
  float rd = 2 * (sh1.f[1] * normal[0] + sh1.f[2] * normal[1] + sh1.f[3] * normal[2]);
  float gd = 2 * (sh1.f[5] * normal[0] + sh1.f[6] * normal[1] + sh1.f[7] * normal[2]);
  float bd = 2 * (sh1.f[9] * normal[0] + sh1.f[10] * normal[1] + sh1.f[11] * normal[2]);

  return (struct SH1){.f[0] = sh1.f[0],
                      .f[1] = sh1.f[1] - rd * normal[0],
                      .f[2] = sh1.f[2] - rd * normal[1],
                      .f[3] = sh1.f[3] - rd * normal[2],
                      .f[4] = sh1.f[4],
                      .f[5] = sh1.f[5] - gd * normal[0],
                      .f[6] = sh1.f[6] - gd * normal[1],
                      .f[7] = sh1.f[7] - gd * normal[2],
                      .f[8] = sh1.f[8],
                      .f[9] = sh1.f[9] - bd * normal[0],
                      .f[10] = sh1.f[10] - bd * normal[1],
                      .f[11] = sh1.f[11] - bd * normal[2]};
}

static inline struct SH1 SH1_Normalize(const struct SH1 sh1, float *out_intensity) {
#define SH1_MAX(X, Y) ((X) > (Y) ? (X) : (Y))
  float intensity = SH1_MAX(sh1.f[0], SH1_MAX(sh1.f[4], sh1.f[8]));
#undef SH1_MAX
  if(out_intensity)
    *out_intensity = intensity;
  return SH1_Scale(sh1, 1.0f / intensity);
}

static inline struct SH1 SH1_NormalizeMaximum(const struct SH1 sh1, float maximum, float *out_intensity) {
#define SH1_MAX(X, Y) ((X) > (Y) ? (X) : (Y))
  float intensity = SH1_MAX(sh1.f[0], SH1_MAX(sh1.f[4], sh1.f[8]));
#undef SH1_MAX
  if(out_intensity)
    *out_intensity = intensity;
  if(intensity > maximum)
    return SH1_Scale(sh1, maximum / intensity);
  else
    return sh1;
}

static inline void SH1_Sample(const struct SH1 sh1, const float direction[3], float output_color[3]) {
  // https://grahamhazel.com/blog/
  for(int component = 0; component < 3; component++) {
    float r0 = sh1.f[component * 4 + 0];
    float r1[3];
    r1[0] = sh1.f[component * 4 + 1];
    r1[1] = sh1.f[component * 4 + 2];
    r1[2] = sh1.f[component * 4 + 3];
    float r1_length_sq = r1[0] * r1[0] + r1[1] * r1[1] + r1[2] * r1[2];
    if(r1_length_sq <= __FLT_EPSILON__ || r0 <= __FLT_EPSILON__) {
      output_color[component] = r0;
      continue;
    }
    float r1_length = sqrt(r1_length_sq);
    float one_over_r1_length = 1.0f / r1_length;
    float r1_normalized[3];
    r1_normalized[0] = r1[0] * one_over_r1_length;
    r1_normalized[1] = r1[1] * one_over_r1_length;
    r1_normalized[2] = r1[2] * one_over_r1_length;
    float q = 0.5f *
              (1 + r1_normalized[0] * direction[0] + r1_normalized[1] * direction[1] + r1_normalized[2] * direction[2]);
    float r1_length_over_r0 = r1_length / r0;
    float p = 1 + 2 * r1_length_over_r0;
    float a = (1 - r1_length_over_r0) / (1 + r1_length_over_r0);
    output_color[component] = r0 * (1 + (1 - a) * (p + 1) * powf(q, p)) * 0.25;
  }
}

/*

  skins will be outline flood filled and mip mapped
  pics and sprites with alpha will be outline flood filled
  pic won't be mip mapped

  model skin
  sprite frame
  wall texture
  pic

*/

typedef enum { it_skin, it_sprite, it_wall, it_pic, it_sky } imagetype_t;

typedef struct Image {
  struct BaseImage base;

  imagetype_t type;

  int upload_width, upload_height; // after power of two and picmip
  int registration_sequence;       // 0 = free
  struct msurface_s *texturechain; // for sort-by-texture world drawing
  int texnum;                      // gl texture binding
  // float sl, tl, sh, th;            // 0,0 - 1,1 unless part of the scrap
  bool scrap;
  bool has_alpha;

  bool paletted;
} image_t;

struct ImageSet {
  image_t *albedo;
  image_t *normal;
};

struct glProgram {
  GLuint vertex_shader;
  GLuint fragment_shader;
  GLuint program;
};

static inline void glProgram_init(struct glProgram *program, const char *vertex_shader, const char *fragment_shader) {
  program->vertex_shader = glCreateShader(GL_VERTEX_SHADER);
}

#define MAX_LIGHTMAPS 128

#define TEXNUM_LIGHTMAPS 1024
#define TEXNUM_SCRAPS (TEXNUM_LIGHTMAPS + MAX_LIGHTMAPS * CMODEL_COUNT)
#define TEXNUM_IMAGES (TEXNUM_SCRAPS + 1)

#define MAX_GLTEXTURES 1024

//===================================================================

typedef enum {
  rserr_ok,

  rserr_invalid_fullscreen,
  rserr_invalid_mode,

  rserr_unknown
} rserr_t;

#include "gl_model.h"

void GL_BeginRendering(int *x, int *y, int *width, int *height);
void GL_EndRendering(void);

void GL_SetDefaultState(void);
void GL_UpdateSwapInterval(void);

extern float gldepthmin, gldepthmax;

typedef struct {
  float x, y, z;
  float s, t;
  float r, g, b;
} glvert_t;

#define MAX_LBM_HEIGHT 480

#define BACKFACE_EPSILON 0.01

//====================================================

extern image_t gltextures[MAX_GLTEXTURES];
extern int numgltextures;

extern image_t *r_notexture;
extern image_t *r_nonormal;
extern image_t *r_whitepcx;
extern image_t *r_particletexture;
extern entity_t *currententity;
extern model_t *currentmodel;
extern int r_visframecount;
extern int r_framecount;
extern cplane_t frustum[4];
extern int c_brush_polys, c_alias_polys;

extern int gl_filter_min, gl_filter_max;

//
// view origin
//
extern vec3_t vup;
extern vec3_t vpn;
extern vec3_t vright;
extern vec3_t r_origin;

//
// screen size info
//
extern refdef_t r_newrefdef;
extern int r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

extern cvar_t *r_norefresh;
extern cvar_t *r_lefthand;
extern cvar_t *r_drawentities;
extern cvar_t *r_drawworld;
extern cvar_t *r_speeds;
extern cvar_t *r_fullbright;
extern cvar_t *r_novis;
extern cvar_t *r_nocull;
extern cvar_t *r_lerpmodels;

extern cvar_t *r_lightlevel; // FIXME: This is a HACK to get the client's light level

extern cvar_t *gl_vertex_arrays;

extern cvar_t *gl_ext_swapinterval;
extern cvar_t *gl_ext_palettedtexture;
extern cvar_t *gl_ext_multitexture;
extern cvar_t *gl_ext_pointparameters;
extern cvar_t *gl_ext_compiled_vertex_array;

extern cvar_t *gl_particle_min_size;
extern cvar_t *gl_particle_max_size;
extern cvar_t *gl_particle_size;
extern cvar_t *gl_particle_att_a;
extern cvar_t *gl_particle_att_b;
extern cvar_t *gl_particle_att_c;

extern cvar_t *gl_nosubimage;
extern cvar_t *gl_bitdepth;
extern cvar_t *gl_mode;
extern cvar_t *gl_log;
extern cvar_t *gl_lightmap;
extern cvar_t *gl_shadows;
extern cvar_t *gl_dynamic;
extern cvar_t *gl_monolightmap;
extern cvar_t *gl_nobind;
extern cvar_t *gl_round_down;
extern cvar_t *gl_picmip;
extern cvar_t *gl_skymip;
extern cvar_t *gl_showtris;
extern cvar_t *gl_finish;
extern cvar_t *gl_ztrick;
extern cvar_t *gl_clear;
extern cvar_t *gl_cull;
extern cvar_t *gl_poly;
extern cvar_t *gl_texsort;
extern cvar_t *gl_polyblend;
extern cvar_t *gl_flashblend;
extern cvar_t *gl_lightmaptype;
extern cvar_t *gl_modulate;
extern cvar_t *gl_playermip;
extern cvar_t *gl_drawbuffer;
extern cvar_t *gl_3dlabs_broken;
extern cvar_t *gl_driver;
extern cvar_t *gl_swapinterval;
extern cvar_t *gl_texturemode;
extern cvar_t *gl_texturealphamode;
extern cvar_t *gl_texturesolidmode;
extern cvar_t *gl_saturatelighting;
extern cvar_t *gl_lockpvs;

extern cvar_t *vid_fullscreen;
extern cvar_t *vid_gamma;

extern cvar_t *intensity;

extern int gl_lightmap_format;
extern int gl_solid_format;
extern int gl_alpha_format;
extern int gl_tex_solid_format;
extern int gl_tex_alpha_format;

extern int c_visible_lightmaps;
extern int c_visible_textures;

extern float r_world_matrix[16];

void R_TranslatePlayerSkin(int playernum);
void GL_Bind(int texnum);
void GL_MBind(GLenum target, int texnum);
void GL_TexEnv(GLenum value);
void GL_EnableMultitexture(bool enable);
void GL_SelectTexture(GLenum);

struct SH1 R_LightPoint(int cmodel_index, vec3_t p);
void R_PushDlights(int cmodel_index);

//====================================================================

extern model_t *r_worldmodel[CMODEL_COUNT];

extern unsigned d_8to24table[256];

extern int registration_sequence;

void V_AddBlend(float r, float g, float b, float a, float *v_blend);

bool R_Init(void *hinstance, void *hWnd);
void R_Shutdown(void);

void R_RenderView(refdef_t *fd);
void GL_ScreenShot_f(void);
void R_DrawAliasModel(entity_t *e);
void R_DrawBrushModel(entity_t *e);
void R_DrawSpriteModel(entity_t *e);
void R_DrawBeam(entity_t *e);
void R_DrawWorld(int cmodel_index);
void R_RenderDlights(void);
void R_DrawAlphaSurfaces(void);
void R_RenderBrushPoly(msurface_t *fa);
void R_InitParticleTexture(void);
void Draw_InitLocal(void);
void GL_SubdivideSurface(msurface_t *fa, struct HunkAllocator *hunk);
bool R_CullBox(vec3_t mins, vec3_t maxs);
void R_RotateForEntity(entity_t *e);
void R_MarkLeaves(int cmodel_index);

glpoly_t *WaterWarpPolyVerts(glpoly_t *p);
void EmitWaterPolys(msurface_t *fa);
void R_AddSkySurface(msurface_t *fa);
void R_ClearSkyBox(void);
void R_DrawSkyBox(void);
void R_MarkLights(int codel_index, dlight_t *light, int bit, mnode_t *node);

void GL_SurfaceInit(void);

#if 0
short LittleShort (short l);
short BigShort (short l);
int	LittleLong (int l);
float LittleFloat (float f);

char	*va(char *format, ...);
// does a varargs printf into a temp buffer
#endif

void COM_StripExtension(char *in, char *out);

void Draw_GetPicSize(int *w, int *h, const char *name);
void Draw_Pic(int x, int y, const char *name);
void Draw_StretchPic(int x, int y, int w, int h, const char *name);
void Draw_Char(int x, int y, int c);
void Draw_TileClear(int x, int y, int w, int h, const char *name);
void Draw_Fill(int x, int y, int w, int h, int c);
void Draw_FadeScreen(void);
void Draw_StretchRaw(int x, int y, int w, int h, int cols, int rows, byte *data);

void R_BeginFrame(float camera_separation);
void R_SwapBuffers(int);
void R_SetPalette(const unsigned char *palette);

int Draw_GetPalette(void);

void GL_ResampleTexture(unsigned *in, int inwidth, int inheight, unsigned *out, int outwidth, int outheight);

struct BaseImage *R_RegisterSkin(const char *name);

void LoadPCX(const char *filename, byte **pic, byte **palette, int *width, int *height);
image_t *GL_LoadPic(const char *name, byte *pic, int width, int height, imagetype_t type, int bits);
image_t *GL_FindImage(const char *name, imagetype_t type);
void GL_TextureMode(char *string);
void GL_ImageList_f(void);

void GL_SetTexturePalette(unsigned palette[256]);

void GL_InitImages(void);
void GL_ShutdownImages(void);

void GL_FreeUnusedImages(void);

void GL_TextureAlphaMode(char *string);
void GL_TextureSolidMode(char *string);

/*
** GL extension emulation functions
*/
void GL_DrawParticles(int n, const particle_t particles[], const unsigned colortable[768]);

/*
** GL config stuff
*/
#define GL_RENDERER_VOODOO 0x00000001
#define GL_RENDERER_VOODOO2 0x00000002
#define GL_RENDERER_VOODOO_RUSH 0x00000004
#define GL_RENDERER_BANSHEE 0x00000008
#define GL_RENDERER_3DFX 0x0000000F

#define GL_RENDERER_PCX1 0x00000010
#define GL_RENDERER_PCX2 0x00000020
#define GL_RENDERER_PMX 0x00000040
#define GL_RENDERER_POWERVR 0x00000070

#define GL_RENDERER_PERMEDIA2 0x00000100
#define GL_RENDERER_GLINT_MX 0x00000200
#define GL_RENDERER_GLINT_TX 0x00000400
#define GL_RENDERER_3DLABS_MISC 0x00000800
#define GL_RENDERER_3DLABS 0x00000F00

#define GL_RENDERER_REALIZM 0x00001000
#define GL_RENDERER_REALIZM2 0x00002000
#define GL_RENDERER_INTERGRAPH 0x00003000

#define GL_RENDERER_3DPRO 0x00004000
#define GL_RENDERER_REAL3D 0x00008000
#define GL_RENDERER_RIVA128 0x00010000
#define GL_RENDERER_DYPIC 0x00020000

#define GL_RENDERER_V1000 0x00040000
#define GL_RENDERER_V2100 0x00080000
#define GL_RENDERER_V2200 0x00100000
#define GL_RENDERER_RENDITION 0x001C0000

#define GL_RENDERER_O2 0x00100000
#define GL_RENDERER_IMPACT 0x00200000
#define GL_RENDERER_RE 0x00400000
#define GL_RENDERER_IR 0x00800000
#define GL_RENDERER_SGI 0x00F00000

#define GL_RENDERER_MCD 0x01000000
#define GL_RENDERER_OTHER 0x80000000

typedef struct {
  int renderer;
  const char *renderer_string;
  const char *vendor_string;
  const char *version_string;
  const char *extensions_string;

  bool allow_cds;
} glconfig_t;

typedef struct {
  float inverse_intensity;
  bool fullscreen;

  int prev_mode;

  unsigned char *d_16to8table;

  int lightmap_textures;

  int currenttextures[32];
  int currenttmu;

  float camera_separation;
  bool stereo_enabled;

  unsigned char originalRedGammaTable[256];
  unsigned char originalGreenGammaTable[256];
  unsigned char originalBlueGammaTable[256];

  struct glProgram bsp_program;
} glstate_t;

extern glconfig_t gl_config;
extern glstate_t gl_state;

/*
====================================================================

IMPORTED FUNCTIONS

====================================================================
*/

extern refimport_t ri;

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void GLimp_BeginFrame(float camera_separation);
void GLimp_EndFrame(void);
int GLimp_Init(void *hinstance, void *hWnd);
void GLimp_Shutdown(void);
rserr_t GLimp_SetMode(int *pwidth, int *pheight, int mode, bool fullscreen);
void GLimp_AppActivate(bool active);
void GLimp_EnableLogging(bool enable);
void GLimp_LogNewFrame(void);
