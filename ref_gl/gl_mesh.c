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

// clang-format off
static const char vertex_shader_source[] =
  GL_MSTR(
    layout(location = 0) out vec2 out_st;
    layout(location = 1) out vec3 out_normal;

    mat3 quaternion_to_matrix(vec4 q) {
      vec4 qn = normalize(q);

      vec4 q2 = qn * 2;
      float wy = q2.y * qn.w;
      float wz = q2.z * qn.w;
      float xy = q2.y * qn.x;
      float xz = q2.z * qn.x;
      float yy = q2.y * qn.y;
      float zz = q2.z * qn.z;
      vec3 normal = vec3(1 - (yy + zz), xy + wz, xz - wy);

      float wx = q2.x * qn.w;
      float xx = q2.x * qn.x;
      float yz = q2.z * qn.y;
      vec3 tangent = vec3(xy - wz, 1 - (xx + zz), yz + wx);

      vec3 bitangent = sign(q.w) * cross(normal, tangent);

      return mat3(tangent, bitangent, normal);
    }

    void main() {
      gl_Position = u_projection_matrix * u_view_matrix * u_model_matrix * vec4(in_position, 1);

      out_st = in_st;

      mat3 tbn = quaternion_to_matrix(in_quaternion);

      out_normal = tbn[2];
    }
  );

// static const char vertex_shader_shell_source[] =
//   GL_MSTR(
//     layout(location = 0) out vec2 out_st;
//     layout(location = 2) out vec3 out_tbn;

//     void main() {
//       gl_Position = u_model_view_projection_matrix * in_position + in_normal * 5;

//       out_st = in_st;

//       out_tbn =  mat3(in_tangent, in_bitangent, in_normal);
//     }
//   );

static const char fragment_shader_source[] =
  GL_MSTR(
    layout(location = 0) in vec2 in_st;
    layout(location = 1) in vec3 in_normal;

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
      vec3 albedo_map = texture(u_albedo_map, in_st).rgb;
      // vec3 normal_map = texture(u_normal_map, in_st).rgb;
      // vec3 normal = in_tbn * normalize(normal_map * 2 - 1);
      vec3 normal = in_normal;

      vec3 lightmap = vec3(do_sh1(u_light_rgb0.r, u_light_r1, normal),
                           do_sh1(u_light_rgb0.g, u_light_g1, normal),
                           do_sh1(u_light_rgb0.b, u_light_b1, normal));

      out_color = vec4(albedo_map * lightmap * 2, 1);
      // out_color = vec4(in_normal * 2 + 1, 1);
    }
  );
// clang-format on

#define VERTEX_FORMAT                                                                                                  \
  .attribute[0] = {0, alias_memory_Format_Float32, 3, "position", 0},                                                  \
  .attribute[1] = {1, alias_memory_Format_Unorm16, 2, "st", 0},                                                        \
  .attribute[2] = {1, alias_memory_Format_Snorm16, 4, "quaternion", 4}, .binding[0] = {sizeof(float) * 3, 0},          \
  .binding[1] = {sizeof(uint16_t) * 2 + sizeof(int16_t) * 4, 0}

#define UNIFORMS_FORMAT                                                                                                \
  .uniform[0] = {THIN_GL_FRAGMENT_BIT, GL_UniformType_Vec3, "light_rgb0"},                                             \
  .uniform[1] = {THIN_GL_FRAGMENT_BIT, GL_UniformType_Vec3, "light_r1"},                                               \
  .uniform[2] = {THIN_GL_FRAGMENT_BIT, GL_UniformType_Vec3, "light_g1"},                                               \
  .uniform[3] = {THIN_GL_FRAGMENT_BIT, GL_UniformType_Vec3, "light_b1"},                                               \
  .global[0] = {THIN_GL_VERTEX_BIT, &u_model_matrix}, .global[1] = {THIN_GL_VERTEX_BIT, &u_view_matrix},               \
  .global[2] = {THIN_GL_VERTEX_BIT, &u_projection_matrix}

#define IMAGES_FORMAT .image[0] = {THIN_GL_FRAGMENT_BIT, GL_ImageType_Sampler2D, "albedo_map"}

#define DRAW_STATE                                                                                                     \
  .primitive = GL_TRIANGLES, VERTEX_FORMAT, UNIFORMS_FORMAT, IMAGES_FORMAT,                                            \
  .fragment_shader_source = fragment_shader_source

#define NO_SHELL .vertex_shader_source = vertex_shader_source

#define SHELL .vertex_shader_source = vertex_shader_shell_source

#define OPAQUE .depth_mask = true, .depth_test_enable = true

#define TRANSPARENT                                                                                                    \
  .depth_mask = false, .depth_test_enable = true, .blend_enable = true, .blend_src_factor = GL_SRC_ALPHA,              \
  .blend_dst_factor = GL_ONE_MINUS_SRC_ALPHA

#define NO_DEPTH_HACK .depth_range_min = 0, .depth_range_max = 1

#define DEPTH_HACK .depth_range_min = 0, .depth_range_max = 0.3

static struct DrawState draw_state_opaque = {DRAW_STATE, NO_SHELL, OPAQUE, NO_DEPTH_HACK};
static struct DrawState draw_state_opaque_depthhack = {DRAW_STATE, NO_SHELL, OPAQUE, DEPTH_HACK};
static struct DrawState draw_state_transparent = {DRAW_STATE, NO_SHELL, TRANSPARENT, NO_DEPTH_HACK};
static struct DrawState draw_state_transparent_depthhack = {DRAW_STATE, NO_SHELL, TRANSPARENT, DEPTH_HACK};

/*
=================
R_DrawAliasModel

=================
*/
void R_DrawAliasModel(entity_t *e) {
  int i;
  float an;
  vec3_t bbox[8];
  image_t *skin;

  if(e->flags & RF_WEAPONMODEL && r_lefthand->value == 2)
    return;

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
    // extern void MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);

    // glMatrixMode(GL_PROJECTION);
    // glPushMatrix();
    // glLoadIdentity();
    // glScalef(-1, 1, 1);
    // MYgluPerspective(r_newrefdef.fov_y, (float)r_newrefdef.width / r_newrefdef.height, 4, 4096);
    // glMatrixMode(GL_MODELVIEW);

    // glCullFace(GL_BACK);
  }

  GL_matrix_translation((currententity->oldorigin[0] - currententity->origin[0]) * currententity->backlerp,
                        (currententity->oldorigin[1] - currententity->origin[1]) * currententity->backlerp,
                        (currententity->oldorigin[2] - currententity->origin[2]) * currententity->backlerp,
                        u_model_matrix.data.mat);

  e->angles[PITCH] = -e->angles[PITCH]; // sigh.
  GL_TransformForEntity(e);
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
    struct VertexPosition {
      float position[3];
    };

    struct VertexAttribute {
      uint16_t st[2];
      // int16_t normal[3];
      int16_t quaternion[4];
    };

    struct GL_Buffer element_vbo =
        GL_allocate_temporary_buffer(GL_ELEMENT_ARRAY_BUFFER, render_mesh->num_indexes * sizeof(uint32_t));
    struct GL_Buffer position_vbo =
        GL_allocate_temporary_buffer(GL_ARRAY_BUFFER, render_mesh->num_vertexes * sizeof(struct VertexPosition));
    struct GL_Buffer attribute_vbo =
        GL_allocate_temporary_buffer(GL_ARRAY_BUFFER, render_mesh->num_vertexes * sizeof(struct VertexAttribute));

    struct RenderMesh_output output = {
        .indexes = {.count = render_mesh->num_indexes,
                    .pointer = (uint32_t *)element_vbo.temporary.mapping,
                    .stride = sizeof(uint32_t),
                    .type_format = alias_memory_Format_Uint32,
                    .type_length = 1},
        .position = {.count = render_mesh->num_vertexes,
                     .pointer = &((struct VertexPosition *)position_vbo.temporary.mapping)->position[0],
                     .stride = sizeof(struct VertexPosition),
                     .type_format = alias_memory_Format_Float32,
                     .type_length = 3},
        .texture_coord_1 = {.count = render_mesh->num_vertexes,
                            .pointer = &((struct VertexAttribute *)attribute_vbo.temporary.mapping)->st[0],
                            .stride = sizeof(struct VertexAttribute),
                            .type_format = alias_memory_Format_Unorm16,
                            .type_length = 2},
        // .normal = {.count = MAX_TRIANGLES * 3,
        //            .pointer = &((struct VertexAttribute *)attribute_vbo.temporary.mapping)->normal[0],
        //            .stride = sizeof(struct VertexAttribute),
        //            .type_format = alias_memory_Format_Snorm16,
        //            .type_length = 3},
        .quaternion = {.count = render_mesh->num_vertexes,
                       .pointer = &((struct VertexAttribute *)attribute_vbo.temporary.mapping)->quaternion[0],
                       .stride = sizeof(struct VertexAttribute),
                       .type_format = alias_memory_Format_Snorm16,
                       .type_length = 4},
    };

    RenderMesh_render_lerp_shaped(render_mesh, &output, currententity->frame, currententity->oldframe,
                                  currententity->backlerp);

    float alpha = (currententity->flags & RF_TRANSLUCENT) ? currententity->alpha : 1;

    struct DrawAssets assets;

    alias_memory_clear(&assets, sizeof(assets));

    // shadelight = SH1_RotateX(SH1_RotateZ(shadelight, -currententity->angles[1]), currententity->angles[0]);

    assets.image[0] = skin->texnum;
    assets.uniforms[0] =
        (struct GL_UniformData){.vec[0] = shadelight.f[0], .vec[1] = shadelight.f[4], .vec[2] = shadelight.f[8]};
    assets.uniforms[1] =
        (struct GL_UniformData){.vec[0] = shadelight.f[1], .vec[1] = shadelight.f[2], .vec[2] = shadelight.f[3]};
    assets.uniforms[2] =
        (struct GL_UniformData){.vec[0] = shadelight.f[5], .vec[1] = shadelight.f[6], .vec[2] = shadelight.f[7]};
    assets.uniforms[3] =
        (struct GL_UniformData){.vec[0] = shadelight.f[9], .vec[1] = shadelight.f[10], .vec[2] = shadelight.f[11]};
    assets.element_buffer = &element_vbo;
    assets.vertex_buffers[0] = &position_vbo;
    assets.vertex_buffers[1] = &attribute_vbo;

    GL_draw_elements(draw_state, &assets, render_mesh->num_indexes, 1, 0, 0);
  }

  if((currententity->flags & RF_WEAPONMODEL) && (r_lefthand->value == 1.0F)) {
    // glMatrixMode(GL_PROJECTION);
    // glPopMatrix();
    // glMatrixMode(GL_MODELVIEW);
    // glCullFace(GL_FRONT);
  }
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
