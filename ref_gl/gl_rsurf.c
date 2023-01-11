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
// GL_RSURF.C: surface-related refresh code

#include "gl_local.h"

#include "gl_thin.h"

#include <assert.h>

static vec3_t modelorg; // relative to viewpoint

msurface_t *r_alpha_surfaces;

int c_visible_lightmaps;
int c_visible_textures;

typedef struct {
  int current_lightmap_texture;

  msurface_t *lightmap_surfaces[MAX_LIGHTMAPS * CMODEL_COUNT];

  int allocated[LIGHTMAP_WIDTH];

  // the lightmap texture data needs to be kept in
  // main memory so texsubimage can update properly
  byte lightmap_buffer_rgb0[LIGHTMAP_WIDTH * LIGHTMAP_HEIGHT * 3];
  byte lightmap_buffer_r1[LIGHTMAP_WIDTH * LIGHTMAP_HEIGHT * 3];
  byte lightmap_buffer_g1[LIGHTMAP_WIDTH * LIGHTMAP_HEIGHT * 3];
  byte lightmap_buffer_b1[LIGHTMAP_WIDTH * LIGHTMAP_HEIGHT * 3];
} gllightmapstate_t;

static void update_lightmap(uint32_t index, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                            const void *pixels) {
  if(gl_state.lightmap_textures[index] == 0) {
    glGenTextures(1, &gl_state.lightmap_textures[index]);
    glBindTexture(GL_TEXTURE_2D, gl_state.lightmap_textures[index]);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  } else {
    glBindTexture(GL_TEXTURE_2D, gl_state.lightmap_textures[index]);
  }
  glTexSubImage2D(GL_TEXTURE_2D, 0, xoffset, yoffset, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
}

static gllightmapstate_t gl_lms;

static void LM_InitBlock(void);
static void LM_UploadBlock(bool dynamic);
static bool LM_AllocBlock(int w, int h, int *x, int *y);

extern void R_SetCacheState(msurface_t *surf);
extern void R_BuildLightMap(msurface_t *surf, byte *dest_rgb0, byte *dest_r1, byte *dest_g1, byte *dest_b1, int stride);

/*
=============================================================

  BRUSH MODELS

=============================================================
*/

// clang-format off
static const char bsp_vertex_shader_source[] =
GL_MSTR(
  layout(location = 0) out vec2 out_main_st;
  layout(location = 1) out vec2 out_lightmap_st;

  void main() {
    gl_Position = u_model_view_projection_matrix * vec4(in_position, 1);

    out_main_st = in_main_st;
    out_lightmap_st = in_lightmap_st;
  }
);
static const char bsp_fragment_shader_source[] =
GL_MSTR(
  layout(location = 0) in vec2 in_main_st;
  layout(location = 1) in vec2 in_lightmap_st;

  layout(location = 0) out vec4 out_color;

  float do_sh1(float r0, vec3 r1, vec3 normal) {
    float r1_length = length(r1);
    vec3 r1_normalized = r1 / r1_length;
    float q = 0.5 * (1 + dot(r1_normalized, normal));
    float r1_length_over_r0 = r1_length / r0;
    float p = 1 + 2 * r1_length_over_r0;
    float a = (1 - r1_length_over_r0) / (1 + r1_length_over_r0);
    return r0 * (1 + (1 - a) * (p + 1) * pow(q, p));
  }

  void main() {
    mat3 texture_space = mat3(u_tangent, u_bitangent, u_normal);

    vec3 albedo_map = texture(u_albedo_map, in_main_st).rgb;
    vec3 normal_map = texture(u_normal_map, in_main_st).rgb;
    vec3 normal = texture_space * normalize(normal_map * 2 - 1);

    vec3 lightmap_rgb0 = texture(u_lightmap_rgb0, in_lightmap_st).rgb;
    vec3 lightmap_r1 = texture(u_lightmap_r1, in_lightmap_st).rgb;
    vec3 lightmap_g1 = texture(u_lightmap_g1, in_lightmap_st).rgb;
    vec3 lightmap_b1 = texture(u_lightmap_b1, in_lightmap_st).rgb;

    vec3 lightmap = vec3(
      do_sh1(lightmap_rgb0.r, lightmap_r1 * 2 - 1, normal),
      do_sh1(lightmap_rgb0.g, lightmap_g1 * 2 - 1, normal),
      do_sh1(lightmap_rgb0.b, lightmap_b1 * 2 - 1, normal)
    );

    out_color = vec4(albedo_map * lightmap * 2, u_alpha_time.x);
  }
);
static const char bsp_turbulent_fragment_shader_source[] =
GL_MSTR(
  layout(location = 0) in vec2 in_main_st;
  layout(location = 1) in vec2 in_lightmap_st;

  layout(location = 0) out vec4 out_color;

  float do_sh1(float r0, vec3 r1, vec3 normal) {
    float r1_length = length(r1);
    vec3 r1_normalized = r1 / r1_length;
    float q = 0.5 * (1 + dot(r1_normalized, normal));
    float r1_length_over_r0 = r1_length / r0;
    float p = 1 + 2 * r1_length_over_r0;
    float a = (1 - r1_length_over_r0) / (1 + r1_length_over_r0);
    return r0 * (1 + (1 - a) * (p + 1) * pow(q, p));
  }

  void main() {
    mat3 texture_space = mat3(u_tangent, u_bitangent, u_normal);

    vec2 turbuluent_st = sin(in_main_st.ts * 4 + vec2(u_alpha_time.y, u_alpha_time.y)) * 0.0625;
    vec2 main_st = in_main_st + turbuluent_st;

    vec3 albedo_map = texture(u_albedo_map, main_st).rgb;
    vec3 normal_map = texture(u_normal_map, main_st).rgb;
    vec3 normal = texture_space * normalize(normal_map * 2 - 1);

    vec3 lightmap_rgb0 = texture(u_lightmap_rgb0, in_lightmap_st).rgb;
    vec3 lightmap_r1 = texture(u_lightmap_r1, in_lightmap_st).rgb;
    vec3 lightmap_g1 = texture(u_lightmap_g1, in_lightmap_st).rgb;
    vec3 lightmap_b1 = texture(u_lightmap_b1, in_lightmap_st).rgb;

    vec3 lightmap = vec3(
      do_sh1(lightmap_rgb0.r, lightmap_r1 * 2 - 1, normal),
      do_sh1(lightmap_rgb0.g, lightmap_g1 * 2 - 1, normal),
      do_sh1(lightmap_rgb0.b, lightmap_b1 * 2 - 1, normal)
    );

    out_color = vec4(albedo_map * lightmap * 2, u_alpha_time.x);
  }
);
// clang-format on

#define VERTEX_FORMAT                                                                                                  \
  .attribute[0] = {0, alias_memory_Format_Float32, 3, "position"},                                                     \
  .attribute[1] = {1, alias_memory_Format_Float32, 2, "main_st"},                                                      \
  .attribute[2] = {1, alias_memory_Format_Float32, 2, "lightmap_st", sizeof(float) * 2},                               \
  .binding[0] = {sizeof(float) * 3}, .binding[1] = {sizeof(float) * 4}

#define UNIFORMS_FORMAT                                                                                                \
  .uniform[0] = {THIN_GL_FRAGMENT_BIT, GL_UniformType_Vec3, "tangent"},                                                \
  .uniform[1] = {THIN_GL_FRAGMENT_BIT, GL_UniformType_Vec3, "bitangent"},                                              \
  .uniform[2] = {THIN_GL_FRAGMENT_BIT, GL_UniformType_Vec3, "normal"},                                                 \
  .uniform[3] = {THIN_GL_FRAGMENT_BIT, GL_UniformType_Vec2, "alpha_time"},                                             \
  .global[0] = {THIN_GL_VERTEX_BIT, &u_model_view_projection_matrix}

#define IMAGES_FORMAT                                                                                                  \
  .image[0] = {THIN_GL_FRAGMENT_BIT, GL_ImageType_Sampler2D, "albedo_map"},                                            \
  .image[1] = {THIN_GL_FRAGMENT_BIT, GL_ImageType_Sampler2D, "normal_map"},                                            \
  .image[3] = {THIN_GL_FRAGMENT_BIT, GL_ImageType_Sampler2D, "lightmap_rgb0"},                                         \
  .image[4] = {THIN_GL_FRAGMENT_BIT, GL_ImageType_Sampler2D, "lightmap_r1"},                                           \
  .image[5] = {THIN_GL_FRAGMENT_BIT, GL_ImageType_Sampler2D, "lightmap_g1"},                                           \
  .image[6] = {THIN_GL_FRAGMENT_BIT, GL_ImageType_Sampler2D, "lightmap_b1"}

static struct DrawState draw_state_opaque = {.primitive = GL_TRIANGLES,
                                             VERTEX_FORMAT,
                                             UNIFORMS_FORMAT,
                                             IMAGES_FORMAT,
                                             .vertex_shader_source = bsp_vertex_shader_source,
                                             .fragment_shader_source = bsp_fragment_shader_source,
                                             .depth_range_min = 0,
                                             .depth_range_max = 1,
                                             .depth_test_enable = true,
                                             .depth_mask = true};

static struct DrawState draw_state_transparent = {.primitive = GL_TRIANGLES,
                                                  VERTEX_FORMAT,
                                                  UNIFORMS_FORMAT,
                                                  IMAGES_FORMAT,
                                                  .vertex_shader_source = bsp_vertex_shader_source,
                                                  .fragment_shader_source = bsp_fragment_shader_source,
                                                  .depth_range_min = 0,
                                                  .depth_range_max = 1,
                                                  .depth_test_enable = true,
                                                  .depth_mask = false,
                                                  .blend_enable = true,
                                                  .blend_src_factor = GL_SRC_ALPHA,
                                                  .blend_dst_factor = GL_ONE_MINUS_SRC_ALPHA};

static struct DrawState draw_state_turbulent_opaque = {.primitive = GL_TRIANGLES,
                                                       VERTEX_FORMAT,
                                                       UNIFORMS_FORMAT,
                                                       IMAGES_FORMAT,
                                                       .vertex_shader_source = bsp_vertex_shader_source,
                                                       .fragment_shader_source = bsp_turbulent_fragment_shader_source,
                                                       .depth_range_min = 0,
                                                       .depth_range_max = 1,
                                                       .depth_test_enable = true,
                                                       .depth_mask = true};

static struct DrawState draw_state_turbulent_transparent = {.primitive = GL_TRIANGLES,
                                                            VERTEX_FORMAT,
                                                            UNIFORMS_FORMAT,
                                                            IMAGES_FORMAT,
                                                            .vertex_shader_source = bsp_vertex_shader_source,
                                                            .fragment_shader_source =
                                                                bsp_turbulent_fragment_shader_source,
                                                            .depth_range_min = 0,
                                                            .depth_range_max = 1,
                                                            .depth_test_enable = true,
                                                            .depth_mask = false,
                                                            .blend_enable = true,
                                                            .blend_src_factor = GL_SRC_ALPHA,
                                                            .blend_dst_factor = GL_ONE_MINUS_SRC_ALPHA};

/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
struct ImageSet R_TextureAnimation(mtexinfo_t *tex) {
  int c;

  if(tex->next) {
    c = currententity->frame % tex->numframes;
    while(c) {
      tex = tex->next;
      c--;
    }
  }

  struct ImageSet result;
  result.albedo = tex->albedo_image;
  result.normal = tex->normal_image;
  return result;
}

static void GL_RenderLightmappedPoly(msurface_t *surf, float alpha) {
  int i;
  int map;
  float *v;
  bool is_dynamic = false;
  unsigned lmtex = surf->lightmaptexturenum;
  glpoly_t *p;

  struct ImageSet image_set = R_TextureAnimation(surf->texinfo);

  for(map = 0; map < MAXLIGHTMAPS && surf->styles[map] != 255; map++) {
    if(r_newrefdef.lightstyles[surf->styles[map]].white != surf->cached_light[map])
      goto dynamic;
  }

  // dynamic this frame or dynamic previously
  if((surf->dlightframe == r_framecount)) {
  dynamic:
    if(gl_dynamic->value) {
      if(!(surf->texinfo->flags & (SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP))) {
        is_dynamic = true;
      }
    }
  }

  if(is_dynamic) {
    uint8_t temp_rgb0[128 * 128 * 3];
    uint8_t temp_r1[128 * 128 * 3];
    uint8_t temp_g1[128 * 128 * 3];
    uint8_t temp_b1[128 * 128 * 3];

    int smax, tmax;

    smax = (surf->extents[0] >> 4) + 1;
    tmax = (surf->extents[1] >> 4) + 1;

    if((surf->styles[map] >= 32 || surf->styles[map] == 0) && (surf->dlightframe != r_framecount)) {
      lmtex = surf->lightmaptexturenum;
    } else {
      lmtex = 0;
    }

    R_BuildLightMap(surf, (void *)temp_rgb0, (void *)temp_r1, (void *)temp_g1, (void *)temp_b1, smax * 3);

    if(lmtex != 0) {
      R_SetCacheState(surf);
    }

    update_lightmap(lmtex + 0, surf->light_s, surf->light_t, smax, tmax, temp_rgb0);
    update_lightmap(lmtex + 1, surf->light_s, surf->light_t, smax, tmax, temp_r1);
    update_lightmap(lmtex + 2, surf->light_s, surf->light_t, smax, tmax, temp_g1);
    update_lightmap(lmtex + 3, surf->light_s, surf->light_t, smax, tmax, temp_b1);
  }

  c_brush_polys++;

  struct DrawState *draw_state;

  struct DrawAssets assets = {.image[0] = image_set.albedo->texnum,
                              .image[1] = image_set.normal->texnum,
                              .image[3] = gl_state.lightmap_textures[lmtex + 0],
                              .image[4] = gl_state.lightmap_textures[lmtex + 1],
                              .image[5] = gl_state.lightmap_textures[lmtex + 2],
                              .image[6] = gl_state.lightmap_textures[lmtex + 3],
                              .element_buffer = &currentmodel->element_buffer,
                              .element_buffer_offset = surf->elements_offset,
                              .vertex_buffers[0] = &currentmodel->position_buffer,
                              .vertex_buffers[1] = &currentmodel->attribute_buffer,
                              .uniforms[0] = {.vec[0] = surf->texture_space_mat3[0][0],
                                              .vec[1] = surf->texture_space_mat3[0][1],
                                              .vec[2] = surf->texture_space_mat3[0][2]},
                              .uniforms[1] = {.vec[0] = surf->texture_space_mat3[1][0],
                                              .vec[1] = surf->texture_space_mat3[1][1],
                                              .vec[2] = surf->texture_space_mat3[1][2]},
                              .uniforms[2] = {.vec[0] = surf->texture_space_mat3[2][0],
                                              .vec[1] = surf->texture_space_mat3[2][1],
                                              .vec[2] = surf->texture_space_mat3[2][2]},
                              .uniforms[3] = {.vec[0] = alpha, .vec[1] = r_newrefdef.time}};

  if(surf->texinfo->flags & SURF_WARP) {
    draw_state = alpha < 1 ? &draw_state_turbulent_transparent : &draw_state_turbulent_opaque;
    // assets.uniforms[0].type = DrawUniformType_Float;
    // assets.uniforms[0]._float = r_newrefdef.time;
  } else {
    draw_state = alpha < 1 ? &draw_state_transparent : &draw_state_opaque;
  }

  float scroll =
      (surf->texinfo->flags & SURF_FLOWING) ? -64 * ((r_newrefdef.time / 40.0) - (int)(r_newrefdef.time / 40.0)) : 0;

  if(scroll == 0.0)
    scroll = -64.0;

  GL_draw_elements(draw_state, &assets, surf->elements_count, 1, 0, 0);
}

/*
================
R_DrawAlphaSurfaces

Draw water surfaces and windows.
The BSP tree is waled front to back, so unwinding the chain
of alpha_surfaces will draw back to front, giving proper ordering.
================
*/
void R_DrawAlphaSurfaces(void) {
  msurface_t *s;
  float intens;

  for(s = r_alpha_surfaces; s; s = s->texturechain) {
    c_brush_polys++;

    float alpha = (s->texinfo->flags & SURF_TRANS33) ? 0.33 : (s->texinfo->flags & SURF_TRANS66) ? 0.66 : 1;

    GL_RenderLightmappedPoly(s, alpha);
  }

  r_alpha_surfaces = NULL;
}

/*
=================
R_DrawInlineBModel
=================
*/
void R_DrawInlineBModel(int cmodel_index) {
  int i, k;
  cplane_t *pplane;
  float dot;
  msurface_t *psurf;
  dlight_t *lt;

  // calculate dynamic lighting for bmodel
  if(!gl_flashblend->value) {
    lt = r_newrefdef.dlights;
    for(k = 0; k < r_newrefdef.num_dlights; k++, lt++) {
      R_MarkLights(cmodel_index, lt, 1 << k, currentmodel->nodes + currentmodel->firstnode);
    }
  }

  psurf = &currentmodel->surfaces[currentmodel->firstmodelsurface];

  float alpha = (currententity->flags & RF_TRANSLUCENT) ? 0.25 : 1;

  //
  // draw texture
  //
  for(i = 0; i < currentmodel->nummodelsurfaces; i++, psurf++) {
    // find which side of the node we are on
    pplane = psurf->plane;

    dot = DotProduct(modelorg, pplane->normal) - pplane->dist;

    // draw the polygon
    if(((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
       (!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON))) {
      if(psurf->texinfo->flags & (SURF_TRANS33 | SURF_TRANS66)) { // add to the translucent chain
        psurf->texturechain = r_alpha_surfaces;
        r_alpha_surfaces = psurf;
      } else {
        GL_RenderLightmappedPoly(psurf, alpha);
      }
    }
  }
}

/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel(entity_t *e) {
  vec3_t mins, maxs;
  int i;
  bool rotated;

  if(currentmodel->nummodelsurfaces == 0)
    return;

  currententity = e;

  if(e->angles[0] || e->angles[1] || e->angles[2]) {
    rotated = true;
    for(i = 0; i < 3; i++) {
      mins[i] = e->origin[i] - currentmodel->radius;
      maxs[i] = e->origin[i] + currentmodel->radius;
    }
  } else {
    rotated = false;
    VectorAdd(e->origin, currentmodel->mins, mins);
    VectorAdd(e->origin, currentmodel->maxs, maxs);
  }

  if(R_CullBox(mins, maxs))
    return;

  memset(gl_lms.lightmap_surfaces, 0, sizeof(gl_lms.lightmap_surfaces));

  VectorSubtract(r_newrefdef.vieworg, e->origin, modelorg);
  if(rotated) {
    vec3_t temp;
    vec3_t forward, right, up;

    VectorCopy(modelorg, temp);
    AngleVectors(e->angles, forward, right, up);
    modelorg[0] = DotProduct(temp, forward);
    modelorg[1] = -DotProduct(temp, right);
    modelorg[2] = DotProduct(temp, up);
  }

  GL_matrix_identity(u_model_matrix.data.mat);
  e->angles[0] = -e->angles[0]; // stupid quake bug
  e->angles[2] = -e->angles[2]; // stupid quake bug
  GL_TransformForEntity(e);
  e->angles[0] = -e->angles[0]; // stupid quake bug
  e->angles[2] = -e->angles[2]; // stupid quake bug

  R_DrawInlineBModel(r_newrefdef.cmodel_index);
}

/*
=============================================================

  WORLD MODEL

=============================================================
*/

/*
================
R_RecursiveWorldNode
================
*/
void R_RecursiveWorldNode(int cmodel_index, mnode_t *node) {
  int c, side, sidebit;
  cplane_t *plane;
  msurface_t *surf, **mark;
  mleaf_t *pleaf;
  float dot;
  image_t *image;

  if(node->contents == CONTENTS_SOLID)
    return; // solid

  if(node->visframe != r_visframecount)
    return;
  if(R_CullBox(node->minmaxs, node->minmaxs + 3))
    return;

  // if a leaf node, draw stuff
  if(node->contents != -1) {
    pleaf = (mleaf_t *)node;

    // check for door connected areas
    if(r_newrefdef.areabits) {
      if(!(r_newrefdef.areabits[pleaf->area >> 3] & (1 << (pleaf->area & 7))))
        return; // not visible
    }

    mark = pleaf->firstmarksurface;
    c = pleaf->nummarksurfaces;

    if(c) {
      do {
        (*mark)->visframe = r_framecount;
        mark++;
      } while(--c);
    }

    return;
  }

  // node is just a decision point, so go down the apropriate sides

  // find which side of the node we are on
  plane = node->plane;

  switch(plane->type) {
  case PLANE_X:
    dot = modelorg[0] - plane->dist;
    break;
  case PLANE_Y:
    dot = modelorg[1] - plane->dist;
    break;
  case PLANE_Z:
    dot = modelorg[2] - plane->dist;
    break;
  default:
    dot = DotProduct(modelorg, plane->normal) - plane->dist;
    break;
  }

  if(dot >= 0) {
    side = 0;
    sidebit = 0;
  } else {
    side = 1;
    sidebit = SURF_PLANEBACK;
  }

  // recurse down the children, front side first
  R_RecursiveWorldNode(cmodel_index, node->children[side]);

  // draw stuff
  for(c = node->numsurfaces, surf = r_worldmodel[cmodel_index]->surfaces + node->firstsurface; c; c--, surf++) {
    if(surf->visframe != r_framecount)
      continue;

    if((surf->flags & SURF_PLANEBACK) != sidebit)
      continue; // wrong side

    if(surf->texinfo->flags & SURF_SKY) {                             // just adds to visible sky bounds
    } else if(surf->texinfo->flags & (SURF_TRANS33 | SURF_TRANS66)) { // add to the translucent chain
      surf->texturechain = r_alpha_surfaces;
      r_alpha_surfaces = surf;
    } else {
      GL_RenderLightmappedPoly(surf, 1);
    }
  }

  // recurse down the back side
  R_RecursiveWorldNode(cmodel_index, node->children[!side]);
}

/*
=============
R_DrawWorld
=============
*/
void R_DrawWorld(int cmodel_index) {
  entity_t ent;

  if(!r_drawworld->value)
    return;

  if(r_newrefdef.rdflags & RDF_NOWORLDMODEL)
    return;

  if(r_worldmodel[cmodel_index]->type != mod_brush)
    return;

  currentmodel = r_worldmodel[cmodel_index];

  VectorCopy(r_newrefdef.vieworg, modelorg);

  // auto cycle the world frame for texture animation
  memset(&ent, 0, sizeof(ent));
  ent.frame = (int)(r_newrefdef.time * 2);
  currententity = &ent;

  memset(gl_lms.lightmap_surfaces, 0, sizeof(gl_lms.lightmap_surfaces));

  GL_matrix_identity(u_model_matrix.data.mat);

  R_RecursiveWorldNode(cmodel_index, r_worldmodel[cmodel_index]->nodes);

  // R_DrawSkyBox();
  {
    extern void GL_draw_sky(uint32_t cmodel_index);
    GL_draw_sky(cmodel_index);
  }
}

/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
void R_MarkLeaves(int cmodel_index) {
  byte *vis;
  byte fatvis[MAX_MAP_LEAFS / 8];
  mnode_t *node;
  int i, c;
  mleaf_t *leaf;
  int cluster;

  if(r_oldviewcluster == r_viewcluster && r_oldviewcluster2 == r_viewcluster2 && !r_novis->value && r_viewcluster != -1)
    return;

  // development aid to let you run around and see exactly where
  // the pvs ends
  if(gl_lockpvs->value)
    return;

  r_visframecount++;
  r_oldviewcluster = r_viewcluster;
  r_oldviewcluster2 = r_viewcluster2;

  if(r_novis->value || r_viewcluster == -1 || !r_worldmodel[cmodel_index]->vis) {
    // mark everything
    for(i = 0; i < r_worldmodel[cmodel_index]->numleafs; i++)
      r_worldmodel[cmodel_index]->leafs[i].visframe = r_visframecount;
    for(i = 0; i < r_worldmodel[cmodel_index]->numnodes; i++)
      r_worldmodel[cmodel_index]->nodes[i].visframe = r_visframecount;
    return;
  }

  vis = Mod_ClusterPVS(r_viewcluster, r_worldmodel[cmodel_index]);
  // may have to combine two clusters because of solid water boundaries
  if(r_viewcluster2 != r_viewcluster) {
    memcpy(fatvis, vis, (r_worldmodel[cmodel_index]->numleafs + 7) / 8);
    vis = Mod_ClusterPVS(r_viewcluster2, r_worldmodel[cmodel_index]);
    c = (r_worldmodel[cmodel_index]->numleafs + 31) / 32;
    for(i = 0; i < c; i++)
      ((int *)fatvis)[i] |= ((int *)vis)[i];
    vis = fatvis;
  }

  for(i = 0, leaf = r_worldmodel[cmodel_index]->leafs; i < r_worldmodel[cmodel_index]->numleafs; i++, leaf++) {
    cluster = leaf->cluster;
    if(cluster == -1)
      continue;
    if(vis[cluster >> 3] & (1 << (cluster & 7))) {
      node = (mnode_t *)leaf;
      do {
        if(node->visframe == r_visframecount)
          break;
        node->visframe = r_visframecount;
        node = node->parent;
      } while(node);
    }
  }
}

/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

static void LM_InitBlock(void) { memset(gl_lms.allocated, 0, sizeof(gl_lms.allocated)); }

static void LM_UploadBlock(bool dynamic) {
  int texture;
  int height = 0;

  if(dynamic) {
    texture = 0;
  } else {
    texture = gl_lms.current_lightmap_texture;
  }

  if(dynamic) {
    int i;

    for(i = 0; i < LIGHTMAP_WIDTH; i++) {
      if(gl_lms.allocated[i] > height)
        height = gl_lms.allocated[i];
    }

    update_lightmap(texture + 0, 0, 0, LIGHTMAP_WIDTH, height, gl_lms.lightmap_buffer_rgb0);
    update_lightmap(texture + 1, 0, 0, LIGHTMAP_WIDTH, height, gl_lms.lightmap_buffer_r1);
    update_lightmap(texture + 2, 0, 0, LIGHTMAP_WIDTH, height, gl_lms.lightmap_buffer_g1);
    update_lightmap(texture + 3, 0, 0, LIGHTMAP_WIDTH, height, gl_lms.lightmap_buffer_b1);
  } else {
    update_lightmap(texture + 0, 0, 0, LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT, gl_lms.lightmap_buffer_rgb0);
    update_lightmap(texture + 1, 0, 0, LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT, gl_lms.lightmap_buffer_r1);
    update_lightmap(texture + 2, 0, 0, LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT, gl_lms.lightmap_buffer_g1);
    update_lightmap(texture + 3, 0, 0, LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT, gl_lms.lightmap_buffer_b1);

    if((gl_lms.current_lightmap_texture += 4) >= MAX_LIGHTMAPS * CMODEL_COUNT)
      ri.Sys_Error(ERR_DROP, "LM_UploadBlock() - MAX_LIGHTMAPS exceeded\n");
  }
}

// returns a texture number and the position inside it
static bool LM_AllocBlock(int w, int h, int *x, int *y) {
  int i, j;
  int best, best2;

  best = LIGHTMAP_HEIGHT;

  for(i = 0; i < LIGHTMAP_WIDTH - w; i++) {
    best2 = 0;

    for(j = 0; j < w; j++) {
      if(gl_lms.allocated[i + j] >= best)
        break;
      if(gl_lms.allocated[i + j] > best2)
        best2 = gl_lms.allocated[i + j];
    }
    if(j == w) { // this is a valid spot
      *x = i;
      *y = best = best2;
    }
  }

  if(best + h > LIGHTMAP_HEIGHT)
    return false;

  for(i = 0; i < w; i++)
    gl_lms.allocated[*x + i] = best + h;

  return true;
}

/*
========================
GL_CreateSurfaceLightmap
========================
*/
void GL_CreateSurfaceLightmap(msurface_t *surf) {
  int smax, tmax;
  byte *base_rgb0, *base_r1, *base_g1, *base_b1;

  if(surf->flags & SURF_DRAWSKY)
    return;

  smax = (surf->extents[0] >> 4) + 1;
  tmax = (surf->extents[1] >> 4) + 1;

  if(!LM_AllocBlock(smax, tmax, &surf->light_s, &surf->light_t)) {
    LM_UploadBlock(false);
    LM_InitBlock();
    if(!LM_AllocBlock(smax, tmax, &surf->light_s, &surf->light_t)) {
      ri.Sys_Error(ERR_FATAL, "Consecutive calls to LM_AllocBlock(%d,%d) failed\n", smax, tmax);
    }
  }

  surf->lightmaptexturenum = gl_lms.current_lightmap_texture;

  base_rgb0 = gl_lms.lightmap_buffer_rgb0;
  base_rgb0 += (surf->light_t * LIGHTMAP_WIDTH + surf->light_s) * 3;

  base_r1 = gl_lms.lightmap_buffer_r1;
  base_r1 += (surf->light_t * LIGHTMAP_WIDTH + surf->light_s) * 3;

  base_g1 = gl_lms.lightmap_buffer_g1;
  base_g1 += (surf->light_t * LIGHTMAP_WIDTH + surf->light_s) * 3;

  base_b1 = gl_lms.lightmap_buffer_b1;
  base_b1 += (surf->light_t * LIGHTMAP_WIDTH + surf->light_s) * 3;

  R_SetCacheState(surf);
  R_BuildLightMap(surf, base_rgb0, base_r1, base_g1, base_b1, LIGHTMAP_WIDTH * 3);
}

/*
==================
GL_BeginBuildingLightmaps

==================
*/
void GL_BeginBuildingLightmaps(model_t *m) {
  static lightstyle_t lightstyles[MAX_LIGHTSTYLES];
  int i;

  memset(gl_lms.allocated, 0, sizeof(gl_lms.allocated));

  r_framecount = 1; // no dlightcache

  /*
  ** setup the base lightstyles so the lightmaps won't have to be regenerated
  ** the first time they're seen
  */
  for(i = 0; i < MAX_LIGHTSTYLES; i++) {
    lightstyles[i].rgb[0] = 1;
    lightstyles[i].rgb[1] = 1;
    lightstyles[i].rgb[2] = 1;
    lightstyles[i].white = 3;
  }
  r_newrefdef.lightstyles = lightstyles;

  gl_lms.current_lightmap_texture = 4 + m->cmodel_index * MAX_LIGHTMAPS;

  update_lightmap(0, 0, 0, LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT, gl_lms.lightmap_buffer_rgb0);
  update_lightmap(1, 0, 0, LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT, gl_lms.lightmap_buffer_r1);
  update_lightmap(2, 0, 0, LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT, gl_lms.lightmap_buffer_g1);
  update_lightmap(3, 0, 0, LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT, gl_lms.lightmap_buffer_b1);
}

/*
=======================
GL_EndBuildingLightmaps
=======================
*/
void GL_EndBuildingLightmaps(void) { LM_UploadBlock(false); }
