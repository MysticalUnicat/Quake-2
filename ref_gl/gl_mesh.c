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
// gl_mesh.c: triangle model functions

#include "gl_local.h"

#include "gl_thin.h"

#include "render_mesh.h"

/*
=============================================================

  ALIAS MODELS

=============================================================
*/

struct SH1 shadelight;

static struct DrawState draw_state_opaque = {.primitive = GL_TRIANGLES,
                                             .depth_mask = true,
                                             .depth_test_enable = true,
                                             .depth_range_min = 0,
                                             .depth_range_max = 1};
static struct DrawState draw_state_opaque_depthhack = {.primitive = GL_TRIANGLES,
                                                       .depth_mask = true,
                                                       .depth_test_enable = true,
                                                       .depth_range_min = 0,
                                                       .depth_range_max = 0.3};
static struct DrawState draw_state_transparent = {.primitive = GL_TRIANGLES,
                                                  .depth_mask = false,
                                                  .depth_test_enable = true,
                                                  .depth_range_min = 0,
                                                  .depth_range_max = 1,
                                                  .blend_enable = true,
                                                  .blend_src_factor = GL_SRC_ALPHA,
                                                  .blend_dst_factor = GL_ONE_MINUS_SRC_ALPHA};
static struct DrawState draw_state_transparent_depthhack = {.primitive = GL_TRIANGLES,
                                                            .depth_mask = false,
                                                            .depth_test_enable = true,
                                                            .depth_range_min = 0,
                                                            .depth_range_max = 0.3,
                                                            .blend_enable = true,
                                                            .blend_src_factor = GL_SRC_ALPHA,
                                                            .blend_dst_factor = GL_ONE_MINUS_SRC_ALPHA};

/*
** R_CullAliasModel
*/
static bool R_CullAliasModel(vec3_t bbox[8], entity_t *e) {
  int i;
  vec3_t mins, maxs;
  dmdl_t *paliashdr;
  vec3_t vectors[3];
  vec3_t thismins, oldmins, thismaxs, oldmaxs;
  daliasframe_t *pframe, *poldframe;
  vec3_t angles;

  paliashdr = (dmdl_t *)currentmodel->extradata;

  if((e->frame >= paliashdr->num_frames) || (e->frame < 0)) {
    ri.Con_Printf(PRINT_ALL, "R_CullAliasModel %s: no such frame %d\n", currentmodel->name, e->frame);
    e->frame = 0;
  }
  if((e->oldframe >= paliashdr->num_frames) || (e->oldframe < 0)) {
    ri.Con_Printf(PRINT_ALL, "R_CullAliasModel %s: no such oldframe %d\n", currentmodel->name, e->oldframe);
    e->oldframe = 0;
  }

  pframe = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames + e->frame * paliashdr->framesize);

  poldframe = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames + e->oldframe * paliashdr->framesize);

  /*
  ** compute axially aligned mins and maxs
  */
  if(pframe == poldframe) {
    for(i = 0; i < 3; i++) {
      mins[i] = pframe->translate[i];
      maxs[i] = mins[i] + pframe->scale[i] * 255;
    }
  } else {
    for(i = 0; i < 3; i++) {
      thismins[i] = pframe->translate[i];
      thismaxs[i] = thismins[i] + pframe->scale[i] * 255;

      oldmins[i] = poldframe->translate[i];
      oldmaxs[i] = oldmins[i] + poldframe->scale[i] * 255;

      if(thismins[i] < oldmins[i])
        mins[i] = thismins[i];
      else
        mins[i] = oldmins[i];

      if(thismaxs[i] > oldmaxs[i])
        maxs[i] = thismaxs[i];
      else
        maxs[i] = oldmaxs[i];
    }
  }

  /*
  ** compute a full bounding box
  */
  for(i = 0; i < 8; i++) {
    vec3_t tmp;

    if(i & 1)
      tmp[0] = mins[0];
    else
      tmp[0] = maxs[0];

    if(i & 2)
      tmp[1] = mins[1];
    else
      tmp[1] = maxs[1];

    if(i & 4)
      tmp[2] = mins[2];
    else
      tmp[2] = maxs[2];

    VectorCopy(tmp, bbox[i]);
  }

  /*
  ** rotate the bounding box
  */
  VectorCopy(e->angles, angles);
  angles[YAW] = -angles[YAW];
  AngleVectors(angles, vectors[0], vectors[1], vectors[2]);

  for(i = 0; i < 8; i++) {
    vec3_t tmp;

    VectorCopy(bbox[i], tmp);

    bbox[i][0] = DotProduct(vectors[0], tmp);
    bbox[i][1] = -DotProduct(vectors[1], tmp);
    bbox[i][2] = DotProduct(vectors[2], tmp);

    VectorAdd(e->origin, bbox[i], bbox[i]);
  }

  {
    int p, f, aggregatemask = ~0;

    for(p = 0; p < 8; p++) {
      int mask = 0;

      for(f = 0; f < 4; f++) {
        float dp = DotProduct(frustum[f].normal, bbox[p]);

        if((dp - frustum[f].dist) < 0) {
          mask |= (1 << f);
        }
      }

      aggregatemask &= mask;
    }

    if(aggregatemask) {
      return true;
    }

    return false;
  }
}

/*
=================
R_DrawAliasModel

=================
*/
void R_DrawAliasModel(entity_t *e) {
  int i;
  // dmdl_t *paliashdr;
  float an;
  vec3_t bbox[8];
  image_t *skin;

  // if(!(e->flags & RF_WEAPONMODEL)) {
  //   if(R_CullAliasModel(bbox, e))
  //     return;
  // }

  if(e->flags & RF_WEAPONMODEL) {
    if(r_lefthand->value == 2)
      return;
  }

  // paliashdr = (dmdl_t *)currentmodel->extradata;

  //
  // get lighting information
  //
  // PMM - rewrote, reordered to handle new shells & mixing
  //
  vec3_t shell_color;
  if(currententity->flags & (RF_SHELL_HALF_DAM | RF_SHELL_GREEN | RF_SHELL_RED | RF_SHELL_BLUE | RF_SHELL_DOUBLE)) {
    // PMM -special case for godmode
    if((currententity->flags & RF_SHELL_RED) && (currententity->flags & RF_SHELL_BLUE) &&
       (currententity->flags & RF_SHELL_GREEN)) {
      for(i = 0; i < 3; i++)
        shell_color[i] = 1.0;
    } else if(currententity->flags & (RF_SHELL_RED | RF_SHELL_BLUE | RF_SHELL_DOUBLE)) {
      VectorClear(shell_color);

      if(currententity->flags & RF_SHELL_RED) {
        shell_color[0] = 1.0;
        if(currententity->flags & (RF_SHELL_BLUE | RF_SHELL_DOUBLE))
          shell_color[2] = 1.0;
      } else if(currententity->flags & RF_SHELL_BLUE) {
        if(currententity->flags & RF_SHELL_DOUBLE) {
          shell_color[1] = 1.0;
          shell_color[2] = 1.0;
        } else {
          shell_color[2] = 1.0;
        }
      } else if(currententity->flags & RF_SHELL_DOUBLE) {
        shell_color[0] = 0.9;
        shell_color[1] = 0.7;
      }
    } else if(currententity->flags & (RF_SHELL_HALF_DAM | RF_SHELL_GREEN)) {
      VectorClear(shell_color);
      // PMM - new colors
      if(currententity->flags & RF_SHELL_HALF_DAM) {
        shell_color[0] = 0.56;
        shell_color[1] = 0.59;
        shell_color[2] = 0.45;
      }
      if(currententity->flags & RF_SHELL_GREEN) {
        shell_color[1] = 1.0;
      }
    }

    shadelight = SH1_FromDirectionalLight(vec3_origin, shell_color);
  }
  // PMM - ok, now flatten these down to range from 0 to 1.0.
  //		max_shell_val = max(shadelight[0], max(shadelight[1], shadelight[2]));
  //		if (max_shell_val > 0)
  //		{
  //			for (i=0; i<3; i++)
  //			{
  //				shadelight[i] = shadelight[i] / max_shell_val;
  //			}
  //		}
  // pmm
  else if(currententity->flags & RF_FULLBRIGHT) {
    shadelight = SH1_FromDirectionalLight(vec3_origin, (float[]){1, 1, 1});
  } else {
    shadelight = R_LightPoint(r_newrefdef.cmodel_index, currententity->origin);

    // player lighting hack for communication back to server
    // big hack!
    if(currententity->flags & RF_WEAPONMODEL) {
      // pick the greatest component, which should be the same
      // as the mono value returned by software
      SH1_Normalize(shadelight, &r_lightlevel->value);
      r_lightlevel->value *= 255;
    }

    // if(gl_monolightmap->string[0] != '0') {
    //   float s = shadelight[0];

    //   if(s < shadelight[1])
    //     s = shadelight[1];
    //   if(s < shadelight[2])
    //     s = shadelight[2];

    //   shadelight[0] = s;
    //   shadelight[1] = s;
    //   shadelight[2] = s;
    // }
  }

  if(currententity->flags & RF_MINLIGHT) {
    // for(i = 0; i < 3; i++)
    //   if(shadelight[i] > 0.1)
    //     break;
    // if(i == 3) {
    //   shadelight[0] = 0.1;
    //   shadelight[1] = 0.1;
    //   shadelight[2] = 0.1;
    // }
  }

  if(currententity->flags & RF_GLOW) { // bonus items will pulse with time
    // float scale;
    // float min;

    // scale = 0.1 * sin(r_newrefdef.time * 7);
    // for(i = 0; i < 3; i++) {
    //   min = shadelight[i] * 0.8;
    //   shadelight[i] += scale;
    //   if(shadelight[i] < min)
    //     shadelight[i] = min;
    // }
  }

  // =================
  // PGM	ir goggles color override
  if(r_newrefdef.rdflags & RDF_IRGOGGLES && currententity->flags & RF_IR_VISIBLE) {
    // shadelight[0] = 1.0;
    // shadelight[1] = 0.0;
    // shadelight[2] = 0.0;
  }
  // PGM
  // =================

  // an = currententity->angles[1] / 180 * M_PI;
  // shadevector[0] = cos(-an);
  // shadevector[1] = sin(-an);
  // shadevector[2] = 1;
  // VectorNormalize(shadevector);

  //
  // locate the proper data
  //

  // c_alias_polys += paliashdr->num_tris;

  //
  // draw all the triangles
  //
  if((currententity->flags & RF_WEAPONMODEL) && (r_lefthand->value == 1.0F)) {
    extern void MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glScalef(-1, 1, 1);
    MYgluPerspective(r_newrefdef.fov_y, (float)r_newrefdef.width / r_newrefdef.height, 4, 4096);
    glMatrixMode(GL_MODELVIEW);

    glCullFace(GL_BACK);
  }

  glPushMatrix();
  e->angles[PITCH] = -e->angles[PITCH]; // sigh.
  R_RotateForEntity(e);
  e->angles[PITCH] = -e->angles[PITCH]; // sigh.

  // select skin
  if(currententity->skin)
    skin = currententity->skin; // custom player skin
  else {
    if(currententity->skinnum >= MAX_MD2SKINS)
      skin = currentmodel->skins[0];
    else {
      skin = currentmodel->skins[currententity->skinnum];
      if(!skin)
        skin = currentmodel->skins[0];
    }
  }
  if(!skin)
    skin = r_notexture; // fallback...

  // draw it
  const struct RenderMesh *render_mesh = (const struct RenderMesh *)currentmodel->extradata;

  if((currententity->frame >= render_mesh->num_shapes) || (currententity->frame < 0)) {
    ri.Con_Printf(PRINT_ALL, "R_DrawAliasModel %s: no such frame %d\n", currentmodel->name, currententity->frame);
    currententity->frame = 0;
    currententity->oldframe = 0;
  }

  if((currententity->oldframe >= render_mesh->num_shapes) || (currententity->oldframe < 0)) {
    ri.Con_Printf(PRINT_ALL, "R_DrawAliasModel %s: no such oldframe %d\n", currentmodel->name, currententity->oldframe);
    currententity->frame = 0;
    currententity->oldframe = 0;
  }

  if(!r_lerpmodels->value)
    currententity->backlerp = 0;

  // GL_DrawAliasFrameLerp(paliashdr, currententity->backlerp);
  struct DrawState *draw_state =
      (currententity->flags & RF_TRANSLUCENT)
          ? ((currententity->flags & RF_DEPTHHACK) ? &draw_state_transparent_depthhack : &draw_state_transparent)
          : ((currententity->flags & RF_DEPTHHACK) ? &draw_state_opaque_depthhack : &draw_state_opaque);

  {
    static uint32_t index[MAX_TRIANGLES * 3];
    static float position[MAX_TRIANGLES * 3 * 3];
    static float normal[MAX_TRIANGLES * 3 * 3];
    static float texture_coord[MAX_TRIANGLES * 3 * 2];

    struct RenderMesh_output output = {
        .indexes = {.count = MAX_TRIANGLES * 3,
                    .pointer = index,
                    .stride = sizeof(uint32_t),
                    .type_format = alias_memory_Format_Uint32,
                    .type_length = 1},
        .position = {.count = MAX_TRIANGLES * 3,
                     .pointer = position,
                     .stride = sizeof(float) * 3,
                     .type_format = alias_memory_Format_Float32,
                     .type_length = 3},
        .normal = {.count = MAX_TRIANGLES * 3,
                   .pointer = normal,
                   .stride = sizeof(float) * 3,
                   .type_format = alias_memory_Format_Float32,
                   .type_length = 3},
        .texture_coord_1 = {.count = MAX_TRIANGLES * 3,
                            .pointer = texture_coord,
                            .stride = sizeof(float) * 2,
                            .type_format = alias_memory_Format_Float32,
                            .type_length = 2},
    };

    RenderMesh_render_lerp_shaped(render_mesh, &output, currententity->frame, currententity->oldframe,
                                  currententity->backlerp);

    GL_begin_draw(draw_state, &(struct DrawAssets){.images[0] = skin->texnum});

    if(currententity->flags & (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM)) {
      glColor3f(shadelight.f[0], shadelight.f[4], shadelight.f[8]);

      for(uint32_t i = 0; i < render_mesh->num_indexes; i++) {
        uint32_t vertex_index = index[i];
        glTexCoord2fv(&texture_coord[vertex_index * 2]);
        glVertex3f(position[vertex_index * 3 + 0] + normal[vertex_index * 3 + 0],
                   position[vertex_index * 3 + 1] + normal[vertex_index * 3 + 1],
                   position[vertex_index * 3 + 2] + normal[vertex_index * 3 + 2]);
      }
    } else {
      shadelight = SH1_RotateX(SH1_RotateZ(shadelight, -currententity->angles[1]), currententity->angles[0]);

      for(uint32_t i = 0; i < render_mesh->num_indexes; i++) {
        uint32_t vertex_index = index[i];
        glTexCoord2fv(&texture_coord[vertex_index * 2]);

        float color[3];
        SH1_Sample(shadelight, &normal[vertex_index * 3], color);
        glColor3f(color[0], color[1], color[2]);

        glVertex3fv(&position[vertex_index * 3]);
      }
    }

    glEnd();
  }

  glPopMatrix();

  if((currententity->flags & RF_WEAPONMODEL) && (r_lefthand->value == 1.0F)) {
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glCullFace(GL_FRONT);
  }

  glColor4f(1, 1, 1, 1);
}

void GL_MD2Init(void) {
  // clang-format off
  static const char * vsource =
  "#version 460 compatibility\n"
  GL_MSTR(
    layout(location = 0) out vec2 out_main_st;
    layout(location = 1) out vec2 out_lightmap_st;
    layout(location = 2) out vec3 out_texture_space_0;
    layout(location = 3) out vec3 out_texture_space_1;
    layout(location = 4) out vec3 out_texture_space_2;

    void main() {
      gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

      out_main_st = gl_MultiTexCoord0.st;
      out_lightmap_st = gl_MultiTexCoord1.st;

      out_texture_space_0 = gl_MultiTexCoord2.xyz;
      out_texture_space_1 = gl_MultiTexCoord3.xyz;
      out_texture_space_2 = gl_MultiTexCoord4.xyz;
    }
  );
  static const char * fsource =
  "#version 460 compatibility\n"
  GL_MSTR(
    layout(binding = 0) uniform sampler2D u_albedo_map;
    layout(binding = 1) uniform sampler2D u_normal_map;
    //layout(binding = 2) uniform sampler2D u_roughness_metallic_map;

    layout(binding = 3) uniform sampler2D u_lightmap_rgb0;
    layout(binding = 4) uniform sampler2D u_lightmap_r1;
    layout(binding = 5) uniform sampler2D u_lightmap_g1;
    layout(binding = 6) uniform sampler2D u_lightmap_b1;

    layout(location = 0) in vec2 in_main_st;
    layout(location = 1) in vec2 in_lightmap_st;
    layout(location = 2) in vec3 in_texture_space_0;
    layout(location = 3) in vec3 in_texture_space_1;
    layout(location = 4) in vec3 in_texture_space_2;

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
      mat3 texture_space = mat3(in_texture_space_0, in_texture_space_1, in_texture_space_2);

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

      out_color = vec4(albedo_map * lightmap * 2, 1);
    }
  );
  // clang-format on

  glProgram_init(&gl_state.md2_program, vsource, fsource);
}

void GL_MD2_Load(model_t *mod, struct HunkAllocator *hunk, const void *buffer) {
  dmdl_t header = *(const dmdl_t *)buffer;

  header.ident = LittleLong(header.ident);
  header.version = LittleLong(header.version);

  header.skinwidth = LittleLong(header.skinwidth);
  header.skinheight = LittleLong(header.skinheight);
  header.framesize = LittleLong(header.framesize);

  header.num_skins = LittleLong(header.num_skins);
  header.num_xyz = LittleLong(header.num_xyz);
  header.num_st = LittleLong(header.num_st);
  header.num_tris = LittleLong(header.num_tris);
  header.num_glcmds = LittleLong(header.num_glcmds);
  header.num_frames = LittleLong(header.num_frames);

  header.ofs_skins = LittleLong(header.ofs_skins);
  header.ofs_st = LittleLong(header.ofs_st);
  header.ofs_tris = LittleLong(header.ofs_tris);
  header.ofs_frames = LittleLong(header.ofs_frames);
  header.ofs_glcmds = LittleLong(header.ofs_glcmds);
  header.ofs_end = LittleLong(header.ofs_end);

  const dskin_t *in_skins = (const dskin_t *)(buffer + header.ofs_skins);
  const dstvert_t *in_st = (const dstvert_t *)(buffer + header.ofs_st);
  const dtriangle_t *in_tris = (const dtriangle_t *)(buffer + header.ofs_tris);

  uint32_t num_indexes = header.num_tris * 3;
  uint32_t *index_remap = malloc(sizeof(*index_remap) * num_indexes);

  for(uint32_t i = 0; i < header.num_tris * 3; i++) {
    index_remap[i] = UINT32_MAX;
  }

  for(uint32_t i = 0; i < header.num_tris * 3; i++) {
    if(index_remap[i] != UINT32_MAX) {
      continue;
    }

    uint16_t i_index = in_tris[i / 3].index_xyz[i % 3];
    uint16_t i_st = in_tris[i / 3].index_st[i % 3];

    for(uint32_t j = 0; j < header.num_tris * 3; j++) {
      uint16_t j_index = in_tris[j / 3].index_xyz[j % 3];
      uint16_t j_st = in_tris[j / 3].index_st[j % 3];

      if(i != j && j_index == i_index && j_st == i_st) {
        index_remap[j] = i;
      }
    }
  }

  uint32_t num_vertexes = 0;

  for(uint32_t i = 0; i < header.num_tris * 3; i++) {
    if(index_remap[i] != UINT32_MAX) {
      continue;
    }
    num_vertexes++;
  }

  struct RenderMesh *render_mesh =
      allocate_RenderMesh(RENDER_MESH_FLAG_TEXTURE_COORD_1, num_indexes, 1, num_vertexes, header.num_frames, 0, 0);

  num_vertexes = 0;

  for(uint32_t i = 0; i < header.num_tris * 3; i++) {
    if(index_remap[i] != UINT32_MAX) {
      continue;
    }
    render_mesh->indexes[i] = num_vertexes++;
    index_remap[i] = i;
  }

  for(uint32_t i = 0; i < header.num_tris * 3; i++) {
    if(index_remap[i] != i) {
      render_mesh->indexes[i] = render_mesh->indexes[index_remap[i]];
    }
  }

  for(uint32_t j = 0; j < header.num_frames; j++) {
    const daliasframe_t *in_frame = (const daliasframe_t *)(buffer + header.ofs_frames + j * header.framesize);

    float scale[3], translate[3];

    scale[0] = LittleFloat(in_frame->scale[0]);
    scale[1] = LittleFloat(in_frame->scale[1]);
    scale[2] = LittleFloat(in_frame->scale[2]);

    translate[0] = LittleFloat(in_frame->translate[0]);
    translate[1] = LittleFloat(in_frame->translate[1]);
    translate[2] = LittleFloat(in_frame->translate[2]);

    for(uint32_t i = 0; i < num_indexes; i++) {
      uint32_t xyz_index = LittleLong(in_tris[index_remap[i] / 3].index_xyz[index_remap[i] % 3]);

      float position[3];
      position[0] = in_frame->verts[xyz_index].v[0] * scale[0] + translate[0];
      position[1] = in_frame->verts[xyz_index].v[1] * scale[1] + translate[1];
      position[2] = in_frame->verts[xyz_index].v[2] * scale[2] + translate[2];

      RenderMesh_set_position(render_mesh, j, render_mesh->indexes[i], position);
    }
  }

  float one_over_skin_width = 1.0f / (float)header.skinwidth;
  float one_over_skin_height = 1.0f / (float)header.skinheight;

  for(uint32_t i = 0; i < num_indexes; i++) {
    uint32_t st_index = LittleLong(in_tris[index_remap[i] / 3].index_st[index_remap[i] % 3]);

    float st[2];
    st[0] = ((float)LittleShort(in_st[st_index].s) + 0.5f) * one_over_skin_width;
    st[1] = ((float)LittleShort(in_st[st_index].t) + 0.5f) * one_over_skin_height;

    RenderMesh_set_texture_coord_1(render_mesh, render_mesh->indexes[i], st);
  }

  RenderMesh_generate_vertex_orientations(render_mesh);

  for(uint32_t i = 0; i < header.num_skins; i++) {
    mod->skins[i] = GL_FindImage((char *)buffer + header.ofs_skins + i * MAX_SKINNAME, it_skin);
  }

  mod->type = mod_alias;

  mod->extradata = render_mesh;
}
