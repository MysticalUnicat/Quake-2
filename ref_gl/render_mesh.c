#include "render_mesh.h"

#include <alias/math.h>

static inline alias_pga3d_10101 decompress_motor(const float s[8]) {
  alias_pga3d_10101 result;
  result.one = s[0];
  result.e01 = s[1];
  result.e02 = s[2];
  result.e03 = s[3];
  result.e12 = s[4];
  result.e13 = s[5];
  result.e23 = s[6];
  result.e0123 = s[7];
  return result;
}

static inline void compress_motor(float d[8], alias_pga3d_10101 s) {
  d[0] = s.one;
  d[1] = s.e01;
  d[2] = s.e02;
  d[3] = s.e03;
  d[4] = s.e12;
  d[5] = s.e13;
  d[6] = s.e23;
  d[7] = s.e0123;
}

static inline size_t allocation_size(uint32_t flags, uint32_t num_indexes, uint32_t num_submeshes,
                                     uint32_t num_vertexes, uint32_t num_shapes, uint32_t num_joints,
                                     uint32_t num_poses) {
  return sizeof(struct RenderMesh) + (sizeof(uint32_t) * num_indexes) +
         (sizeof(struct RenderMesh_submesh) * num_submeshes) +
         (sizeof(struct RenderMesh_vertex) * num_vertexes * num_shapes) +
         (sizeof(struct RenderMesh_vertex_st) * num_vertexes *
          (((flags & RENDER_MESH_FLAG_TEXTURE_COORD_1) ? 1 : 0) +
           ((flags & RENDER_MESH_FLAG_TEXTURE_COORD_2) ? 1 : 0))) +
         (sizeof(struct RenderMesh_vertex_bone) * num_vertexes * (num_poses > 0 && num_joints > 0 ? 1 : 0)) +
         (sizeof(struct RenderMesh_vertex_color) * num_vertexes * ((flags & RENDER_MESH_FLAG_TEXTURE_COLOR) ? 1 : 0)) +
         (sizeof(struct RenderMesh_joint) * num_joints * (num_poses > 0 && num_joints > 0 ? 1 : 0)) +
         (sizeof(struct RenderMesh_pose) * num_joints * num_poses * (num_poses > 0 && num_joints > 0 ? 1 : 0));
}

struct RenderMesh *allocate_RenderMesh(uint32_t flags, uint32_t num_indexes, uint32_t num_submeshes,
                                       uint32_t num_vertexes, uint32_t num_shapes, uint32_t num_joints,
                                       uint32_t num_poses) {
  if(num_indexes == 0 || num_vertexes == 0 || num_shapes == 0 || num_submeshes == 0)
    return NULL;

  size_t size = allocation_size(flags, num_indexes, num_submeshes, num_vertexes, num_shapes, num_joints, num_poses);
  struct RenderMesh *result = alias_malloc(NULL, size, 4);
  result->flags = flags;
  result->num_indexes = num_indexes;
  result->num_submeshes = num_submeshes;
  result->num_vertexes = num_vertexes;
  result->num_shapes = num_shapes;
  result->num_joints = num_joints;
  result->num_poses = num_poses;

  uint8_t *ptr = (uint8_t *)(result + 1);

  result->indexes = (uint32_t *)ptr;
  ptr += sizeof(uint32_t) * num_indexes;

  result->submeshes = (struct RenderMesh_submesh *)ptr;
  ptr += sizeof(struct RenderMesh_submesh) * num_submeshes;

  result->vertexes = (struct RenderMesh_vertex *)ptr;
  ptr += sizeof(struct RenderMesh_vertex) * num_vertexes * num_shapes;

  if(flags & RENDER_MESH_FLAG_TEXTURE_COORD_1) {
    result->vertexes_st_1 = (struct RenderMesh_vertex_st *)ptr;
    ptr += sizeof(struct RenderMesh_vertex_st) * num_vertexes;
  } else {
    result->vertexes_st_1 = NULL;
  }

  if(flags & RENDER_MESH_FLAG_TEXTURE_COORD_2) {
    result->vertexes_st_2 = (struct RenderMesh_vertex_st *)ptr;
    ptr += sizeof(struct RenderMesh_vertex_st) * num_vertexes;
  } else {
    result->vertexes_st_2 = NULL;
  }

  if(num_poses > 0 && num_joints > 0) {
    result->vertexes_bone = (struct RenderMesh_vertex_bone *)ptr;
    ptr += sizeof(struct RenderMesh_vertex_bone) * num_vertexes;
  } else {
    result->vertexes_bone = NULL;
  }

  if(flags & RENDER_MESH_FLAG_TEXTURE_COLOR) {
    result->vertexes_color = (struct RenderMesh_vertex_color *)ptr;
    ptr += sizeof(struct RenderMesh_vertex_color) * num_vertexes;
  } else {
    result->vertexes_color = NULL;
  }

  if(num_joints > 0 && num_poses > 0) {
    result->joints = (struct RenderMesh_joint *)ptr;
    ptr += sizeof(struct RenderMesh_joint) * num_joints;
  } else {
    result->joints = NULL;
  }

  if(num_joints > 0 && num_poses > 0) {
    result->poses = (struct RenderMesh_pose *)ptr;
    ptr += sizeof(struct RenderMesh_pose) * num_joints * num_poses;
  } else {
    result->poses = NULL;
  }

  if(ptr != (uint8_t *)result + size) {
    return NULL;
  }

  return result;
}

static inline void RenderMesh_init_pose(const struct RenderMesh *render_mesh, alias_pga3d_10101 poses[256],
                                        uint32_t pose_index) {
  const struct RenderMesh_pose *in_pose = &render_mesh->poses[pose_index * render_mesh->num_joints];
  for(int i = 0; i < render_mesh->num_joints; i++) {
    poses[i] = decompress_motor(in_pose[i].motor);
  }
}

static inline void RenderMesh_init_pose_weight(const struct RenderMesh *render_mesh, alias_pga3d_10101 poses[256],
                                               uint32_t pose_index, float weight) {
  const struct RenderMesh_pose *in_pose = &render_mesh->poses[pose_index * render_mesh->num_joints];
  for(int i = 0; i < render_mesh->num_joints; i++) {
    alias_pga3d_10101 pose = decompress_motor(in_pose[i].motor);
    poses[i] = alias_pga3d_mul_sm(weight, pose);
  }
}

static inline void RenderMesh_init_pose_masked(const struct RenderMesh *render_mesh, alias_pga3d_10101 poses[256],
                                               uint32_t pose_index, const uint32_t mask[256 >> 5], float weight) {
  const struct RenderMesh_pose *in_pose = &render_mesh->poses[pose_index * render_mesh->num_joints];
  for(int i = 0; i < render_mesh->num_joints; i++) {
    if(!(mask[i >> 5] & (1 << (i & 31)))) {
      alias_memory_clear(&poses[i], sizeof(poses[0]));
      continue;
    }
    alias_pga3d_10101 pose = decompress_motor(in_pose[i].motor);
    poses[i] = alias_pga3d_mul_sm(weight, pose);
  }
}

static inline void RenderMesh_add_pose(const struct RenderMesh *render_mesh, alias_pga3d_10101 poses[256],
                                       uint32_t pose_index, float weight) {
  const struct RenderMesh_pose *in_pose = &render_mesh->poses[pose_index * render_mesh->num_joints];

  for(int i = 0; i < render_mesh->num_joints; i++) {
    alias_pga3d_10101 pose = decompress_motor(in_pose[i].motor);
    poses[i] = alias_pga3d_add(alias_pga3d_m(poses[i]), alias_pga3d_mul_sm(weight, pose));
  }
}

static inline void RenderMesh_add_pose_masked(const struct RenderMesh *render_mesh, alias_pga3d_10101 poses[256],
                                              uint32_t pose_index, const uint32_t mask[256 >> 5], float weight) {
  const struct RenderMesh_pose *in_pose = &render_mesh->poses[pose_index * render_mesh->num_joints];

  for(int i = 0; i < render_mesh->num_joints; i++) {
    if(!(mask[i >> 5] & (1 << (i & 31))))
      continue;
    alias_pga3d_10101 pose = decompress_motor(in_pose[i].motor);
    poses[i] = alias_pga3d_add(alias_pga3d_m(poses[i]), alias_pga3d_mul_sm(weight, pose));
  }
}

static inline void RenderMesh_finalize_bones(const struct RenderMesh *render_mesh, alias_pga3d_10101 poses[256],
                                             alias_pga3d_10101 bones[256]) {
  alias_memory_clear(&bones[255], sizeof(bones[0]));
  bones[255].one = 1.0f;

  for(int i = 0; i < render_mesh->num_joints; i++) {
    alias_pga3d_10101 pose = poses[i];
    alias_pga3d_10101 parent = bones[render_mesh->joints[i].parent];
    bones[i] = alias_pga3d_mul_mm(parent, pose);
  }
}

static inline void RenderMesh_init_vertexes_static(const struct RenderMesh *render_mesh, alias_pga3d_10101 *vertexes,
                                                   uint32_t shape_index) {
  const struct RenderMesh_vertex *in_vertex = &render_mesh->vertexes[shape_index * render_mesh->num_vertexes];
  for(int i = 0; i < render_mesh->num_vertexes; i++) {
    vertexes[i] = decompress_motor(in_vertex[i].motor);
  }
}

static inline void RenderMesh_init_vertexes_weight(const struct RenderMesh *render_mesh, alias_pga3d_10101 *vertexes,
                                                   uint32_t shape_index, float weight) {
  const struct RenderMesh_vertex *in_vertex = &render_mesh->vertexes[shape_index * render_mesh->num_vertexes];
  for(int i = 0; i < render_mesh->num_vertexes; i++) {
    alias_pga3d_10101 vertex = decompress_motor(in_vertex[i].motor);
    vertexes[i] = alias_pga3d_mul_sm(weight, vertex);
  }
}

static inline void RenderMesh_init_vertexes_masked(const struct RenderMesh *render_mesh, alias_pga3d_10101 *vertexes,
                                                   const uint8_t *mask, uint32_t shape_index, float weight) {
  const struct RenderMesh_vertex *in_vertex = &render_mesh->vertexes[shape_index * render_mesh->num_vertexes];
  for(int i = 0; i < render_mesh->num_vertexes; i++) {
    if(!(mask[i >> 3] & (1 << (i & 7))))
      continue;
    alias_pga3d_10101 vertex = decompress_motor(in_vertex[i].motor);
    vertexes[i] = alias_pga3d_mul_sm(weight, vertex);
  }
}

static inline void RenderMesh_add_vertexes_weight(const struct RenderMesh *render_mesh, alias_pga3d_10101 *vertexes,
                                                  uint32_t shape_index, float weight) {
  const struct RenderMesh_vertex *in_vertex = &render_mesh->vertexes[shape_index * render_mesh->num_vertexes];
  for(int i = 0; i < render_mesh->num_vertexes; i++) {
    alias_pga3d_10101 vertex = decompress_motor(in_vertex[i].motor);
    vertexes[i] = alias_pga3d_add(alias_pga3d_m(vertexes[i]), alias_pga3d_mul_sm(weight, vertex));
  }
}

static inline void RenderMesh_add_vertexes_masked(const struct RenderMesh *render_mesh, alias_pga3d_10101 *vertexes,
                                                  const uint8_t *mask, uint32_t shape_index, float weight) {
  const struct RenderMesh_vertex *in_vertex = &render_mesh->vertexes[shape_index * render_mesh->num_vertexes];
  for(int i = 0; i < render_mesh->num_vertexes; i++) {
    if(!(mask[i >> 3] & (1 << (i & 7))))
      continue;
    alias_pga3d_10101 vertex = decompress_motor(in_vertex[i].motor);
    vertexes[i] = alias_pga3d_add(alias_pga3d_m(vertexes[i]), alias_pga3d_mul_sm(weight, vertex));
  }
}

static inline void RenderMesh_skin_vertexes(const struct RenderMesh *render_mesh, const alias_pga3d_10101 bones[256],
                                            alias_pga3d_10101 *vertexes) {
  for(int i = 0; i < render_mesh->num_vertexes; i++) {
    alias_pga3d_10101 bone = {.one = 0.0f};
    for(int b = 0; b < 4; b++) {
      float w = render_mesh->vertexes_bone[i].weight[b] / 255.0f;
      bone = alias_pga3d_add(alias_pga3d_m(bone), alias_pga3d_mul_sm(w, bones[render_mesh->vertexes_bone[i].index[b]]));
    }
    vertexes[i] = alias_pga3d_mul_mm(bone, vertexes[i]);
  }
}

static inline void RenderMesh_output_indexes(const struct RenderMesh *render_mesh, alias_memory_SubBuffer *subbuffer) {
  alias_memory_SubBuffer_write(subbuffer, 0, render_mesh->num_indexes, alias_memory_Format_Uint32, 0,
                               render_mesh->indexes);
}

static inline void RenderMesh_output_position(const struct RenderMesh *render_mesh, const alias_pga3d_10101 *vertexes,
                                              alias_memory_SubBuffer *subbuffer) {
  for(int i = 0; i < render_mesh->num_vertexes; i++) {
    alias_pga3d_00010 position_1 = alias_pga3d_point(0, 0, 0);
    position_1 = alias_pga3d_grade_3(alias_pga3d_sandwich(alias_pga3d_t(position_1), alias_pga3d_m(vertexes[i])));
    float position_2[3];
    position_2[0] = alias_pga3d_point_x(position_1);
    position_2[1] = alias_pga3d_point_y(position_1);
    position_2[2] = alias_pga3d_point_z(position_1);
    alias_memory_SubBuffer_write(subbuffer, i, 1, alias_memory_Format_Float32, 0, &position_2[0]);
  }
}

static inline void RenderMesh_output_normal(const struct RenderMesh *render_mesh, const alias_pga3d_10101 *vertexes,
                                            alias_memory_SubBuffer *subbuffer) {
  for(int i = 0; i < render_mesh->num_vertexes; i++) {
    alias_pga3d_00010 normal_1 = alias_pga3d_direction(0, 0, 1);
    normal_1 = alias_pga3d_grade_3(alias_pga3d_sandwich(alias_pga3d_t(normal_1), alias_pga3d_m(vertexes[i])));
    float normal_2[3];
    normal_2[0] = alias_pga3d_direction_x(normal_1);
    normal_2[1] = alias_pga3d_direction_y(normal_1);
    normal_2[2] = alias_pga3d_direction_z(normal_1);
    alias_memory_SubBuffer_write(subbuffer, i, 1, alias_memory_Format_Float32, 0, &normal_2[0]);
  }
}

static inline void RenderMesh_output_tangent(const struct RenderMesh *render_mesh, const alias_pga3d_10101 *vertexes,
                                             alias_memory_SubBuffer *subbuffer) {
  for(int i = 0; i < render_mesh->num_vertexes; i++) {
    alias_pga3d_00010 tangent_1 = alias_pga3d_direction(1, 0, 0);
    tangent_1 = alias_pga3d_grade_3(alias_pga3d_sandwich(alias_pga3d_t(tangent_1), alias_pga3d_m(vertexes[i])));
    float tangent_2[3];
    tangent_2[0] = alias_pga3d_direction_x(tangent_1);
    tangent_2[1] = alias_pga3d_direction_y(tangent_1);
    tangent_2[2] = alias_pga3d_direction_z(tangent_1);
    alias_memory_SubBuffer_write(subbuffer, i, 1, alias_memory_Format_Float32, 0, &tangent_2[0]);
  }
}

static inline void RenderMesh_output_bitangent(const struct RenderMesh *render_mesh, const alias_pga3d_10101 *vertexes,
                                               alias_memory_SubBuffer *subbuffer) {
  for(int i = 0; i < render_mesh->num_vertexes; i++) {
    alias_pga3d_00010 bitangent_1 = alias_pga3d_direction(0, 1, 0);
    bitangent_1 = alias_pga3d_grade_3(alias_pga3d_sandwich(alias_pga3d_t(bitangent_1), alias_pga3d_m(vertexes[i])));
    float bitangent_2[3];
    bitangent_2[0] = alias_pga3d_direction_x(bitangent_1);
    bitangent_2[1] = alias_pga3d_direction_y(bitangent_1);
    bitangent_2[2] = alias_pga3d_direction_z(bitangent_1);
    alias_memory_SubBuffer_write(subbuffer, i, 1, alias_memory_Format_Float32, 0, &bitangent_2[0]);
  }
}

static inline void RenderMesh_output_texture_coord(const struct RenderMesh *render_mesh,
                                                   const struct RenderMesh_vertex_st *st,
                                                   alias_memory_SubBuffer *subbuffer) {
  alias_memory_SubBuffer_write(subbuffer, 0, render_mesh->num_vertexes, alias_memory_Format_Float32, 0, st);
}

static inline void RenderMesh_output_color(const struct RenderMesh *render_mesh, alias_memory_SubBuffer *subbuffer) {
  alias_memory_SubBuffer_write(subbuffer, 0, render_mesh->num_vertexes, alias_memory_Format_Unorm8, 0,
                               render_mesh->vertexes_color);
}

static inline void RenderMesh_output_all(const struct RenderMesh *render_mesh, const alias_pga3d_10101 *vertexes,
                                         struct RenderMesh_output *output) {
  if(output->indexes.pointer) {
    RenderMesh_output_indexes(render_mesh, &output->indexes);
  }
  if(output->position.pointer) {
    RenderMesh_output_position(render_mesh, vertexes, &output->position);
  }
  if(output->normal.pointer) {
    RenderMesh_output_normal(render_mesh, vertexes, &output->normal);
  }
  if(output->tangent.pointer) {
    RenderMesh_output_tangent(render_mesh, vertexes, &output->tangent);
  }
  if(output->bitangent.pointer) {
    RenderMesh_output_bitangent(render_mesh, vertexes, &output->bitangent);
  }
  if(output->texture_coord_1.pointer) {
    RenderMesh_output_texture_coord(render_mesh, render_mesh->vertexes_st_1, &output->texture_coord_1);
  }
  if(output->texture_coord_2.pointer) {
    RenderMesh_output_texture_coord(render_mesh, render_mesh->vertexes_st_2, &output->texture_coord_2);
  }
  if(output->color.pointer) {
    RenderMesh_output_color(render_mesh, &output->color);
  }
}

void RenderMesh_render(const struct RenderMesh *render_mesh, struct RenderMesh_output *output) {
  alias_pga3d_10101 *vertexes = alias_malloc(NULL, sizeof(*vertexes) * render_mesh->num_vertexes, 4);
  RenderMesh_init_vertexes_static(render_mesh, vertexes, 0);
  RenderMesh_output_all(render_mesh, vertexes, output);
  alias_free(NULL, vertexes, sizeof(*vertexes) * render_mesh->num_vertexes, 4);
}

void RenderMesh_render_skinned(const struct RenderMesh *render_mesh, struct RenderMesh_output *output,
                               uint32_t pose_index) {
  alias_pga3d_10101 bones[256], poses[256];

  RenderMesh_init_pose(render_mesh, poses, pose_index);
  RenderMesh_finalize_bones(render_mesh, poses, bones);

  alias_pga3d_10101 *vertexes = alias_malloc(NULL, sizeof(*vertexes) * render_mesh->num_vertexes, 4);
  RenderMesh_init_vertexes_static(render_mesh, vertexes, 0);
  RenderMesh_skin_vertexes(render_mesh, bones, vertexes);

  RenderMesh_output_all(render_mesh, vertexes, output);
  alias_free(NULL, vertexes, sizeof(*vertexes) * render_mesh->num_vertexes, 4);
}

void RenderMesh_render_lerp_skinned(const struct RenderMesh *render_mesh, struct RenderMesh_output *output,
                                    uint32_t pose_index_a, uint32_t pose_index_b, float t) {
  alias_pga3d_10101 bones[256], poses[256];

  RenderMesh_init_pose_weight(render_mesh, poses, pose_index_a, 1.0f - t);
  RenderMesh_add_pose(render_mesh, poses, pose_index_b, t);
  RenderMesh_finalize_bones(render_mesh, poses, bones);

  alias_pga3d_10101 *vertexes = alias_malloc(NULL, sizeof(*vertexes) * render_mesh->num_vertexes, 4);
  RenderMesh_init_vertexes_static(render_mesh, vertexes, 0);
  RenderMesh_skin_vertexes(render_mesh, bones, vertexes);

  RenderMesh_output_all(render_mesh, vertexes, output);
  alias_free(NULL, vertexes, sizeof(*vertexes) * render_mesh->num_vertexes, 4);
}

void RenderMesh_render_shaped(const struct RenderMesh *render_mesh, struct RenderMesh_output *output,
                              uint32_t shape_index) {

  alias_pga3d_10101 *vertexes = alias_malloc(NULL, sizeof(*vertexes) * render_mesh->num_vertexes, 4);
  RenderMesh_init_vertexes_static(render_mesh, vertexes, shape_index);

  RenderMesh_output_all(render_mesh, vertexes, output);
  alias_free(NULL, vertexes, sizeof(*vertexes) * render_mesh->num_vertexes, 4);
}

void RenderMesh_render_lerp_shaped(const struct RenderMesh *render_mesh, struct RenderMesh_output *output,
                                   uint32_t shape_index_a, uint32_t shape_index_b, float t) {
  if(shape_index_a == shape_index_b || t <= alias_R_EPSILON) {
    RenderMesh_render_shaped(render_mesh, output, shape_index_a);
    return;
  }

  if((t - 1.0f) >= 0.0f) {
    RenderMesh_render_shaped(render_mesh, output, shape_index_b);
    return;
  }

  alias_pga3d_10101 *vertexes = alias_malloc(NULL, sizeof(*vertexes) * render_mesh->num_vertexes, 4);
  RenderMesh_init_vertexes_weight(render_mesh, vertexes, shape_index_a, 1.0f - t);
  RenderMesh_add_vertexes_weight(render_mesh, vertexes, shape_index_b, t);

  RenderMesh_output_all(render_mesh, vertexes, output);
  alias_free(NULL, vertexes, sizeof(*vertexes) * render_mesh->num_vertexes, 4);
}

void RenderMesh_render_lerp_shaped_skinned(const struct RenderMesh *render_mesh, struct RenderMesh_output *output,
                                           uint32_t pose_index, uint32_t shape_index_a, uint32_t shape_index_b,
                                           float t) {
  alias_pga3d_10101 bones[256], poses[256];

  RenderMesh_init_pose(render_mesh, poses, pose_index);
  RenderMesh_finalize_bones(render_mesh, poses, bones);

  alias_pga3d_10101 *vertexes = alias_malloc(NULL, sizeof(*vertexes) * render_mesh->num_vertexes, 4);
  RenderMesh_init_vertexes_weight(render_mesh, vertexes, shape_index_a, 1.0f - t);
  RenderMesh_add_vertexes_weight(render_mesh, vertexes, shape_index_b, t);
  RenderMesh_skin_vertexes(render_mesh, bones, vertexes);

  RenderMesh_output_all(render_mesh, vertexes, output);
  alias_free(NULL, vertexes, sizeof(*vertexes) * render_mesh->num_vertexes, 4);
}

void RenderMesh_render_lerp_shaped_lerp_skinned(const struct RenderMesh *render_mesh, struct RenderMesh_output *output,
                                                uint32_t pose_index_a, uint32_t pose_index_b, float pose_t,
                                                uint32_t shape_index_a, uint32_t shape_index_b, float shape_t) {
  alias_pga3d_10101 bones[256], poses[256];

  RenderMesh_init_pose_weight(render_mesh, poses, pose_index_a, 1.0f - pose_t);
  RenderMesh_add_pose(render_mesh, poses, pose_index_b, pose_t);
  RenderMesh_finalize_bones(render_mesh, poses, bones);

  alias_pga3d_10101 *vertexes = alias_malloc(NULL, sizeof(*vertexes) * render_mesh->num_vertexes, 4);
  RenderMesh_init_vertexes_weight(render_mesh, vertexes, shape_index_a, 1.0f - shape_t);
  RenderMesh_add_vertexes_weight(render_mesh, vertexes, shape_index_b, shape_t);
  RenderMesh_skin_vertexes(render_mesh, bones, vertexes);

  RenderMesh_output_all(render_mesh, vertexes, output);
  alias_free(NULL, vertexes, sizeof(*vertexes) * render_mesh->num_vertexes, 4);
}

void RenderMesh_set_position(struct RenderMesh *render_mesh, uint32_t shape_index, uint32_t vertex_index,
                             const float position[3]) {
  alias_pga3d_Point point = alias_pga3d_point(position[0], position[1], position[2]);
  alias_pga3d_Motor motor = alias_pga3d_translator_to(point);
  compress_motor(render_mesh->vertexes[shape_index * render_mesh->num_vertexes + vertex_index].motor, motor);
}

void RenderMesh_set_texture_coord_1(struct RenderMesh *render_mesh, uint32_t vertex_index, const float position[2]) {
  render_mesh->vertexes_st_1[vertex_index].st[0] = position[0];
  render_mesh->vertexes_st_1[vertex_index].st[1] = position[1];
}

void RenderMesh_set_texture_coord_2(struct RenderMesh *render_mesh, uint32_t vertex_index, const float position[2]) {
  render_mesh->vertexes_st_2[vertex_index].st[0] = position[0];
  render_mesh->vertexes_st_2[vertex_index].st[1] = position[1];
}

void RenderMesh_generate_vertex_orientations(struct RenderMesh *render_mesh) {
  alias_pga3d_Plane *triangle_plane = alias_malloc(NULL, sizeof(*triangle_plane) * (render_mesh->num_indexes / 3), 4);
  uint32_t *unique_vertex_id = alias_malloc(NULL, sizeof(*unique_vertex_id) * render_mesh->num_vertexes, 4);
  alias_pga3d_Line *vertex_line = alias_malloc(NULL, sizeof(*vertex_line) * render_mesh->num_vertexes, 4);

  for(uint32_t shape_index = 0; shape_index < render_mesh->num_shapes; shape_index++) {
    // naively count unique vertexes positions
    for(uint32_t vertex_index_a = 0; vertex_index_a < render_mesh->num_vertexes; vertex_index_a++) {
      unique_vertex_id[vertex_index_a] = UINT32_MAX;
    }
    alias_memory_clear(vertex_line, sizeof(*vertex_line) * render_mesh->num_vertexes);

    for(uint32_t vertex_index_a = 0; vertex_index_a < render_mesh->num_vertexes; vertex_index_a++) {
      if(unique_vertex_id[vertex_index_a] != UINT32_MAX)
        continue;
      unique_vertex_id[vertex_index_a] = vertex_index_a;
      for(uint32_t vertex_index_b = vertex_index_a + 1; vertex_index_b < render_mesh->num_vertexes; vertex_index_b++) {
        if(!alias_R_fuzzy_eq(render_mesh->vertexes[shape_index * render_mesh->num_vertexes + vertex_index_a].motor[1],
                             render_mesh->vertexes[shape_index * render_mesh->num_vertexes + vertex_index_b].motor[1]))
          continue;
        if(!alias_R_fuzzy_eq(render_mesh->vertexes[shape_index * render_mesh->num_vertexes + vertex_index_a].motor[2],
                             render_mesh->vertexes[shape_index * render_mesh->num_vertexes + vertex_index_b].motor[2]))
          continue;
        if(!alias_R_fuzzy_eq(render_mesh->vertexes[shape_index * render_mesh->num_vertexes + vertex_index_a].motor[3],
                             render_mesh->vertexes[shape_index * render_mesh->num_vertexes + vertex_index_b].motor[3]))
          continue;
        if(!alias_R_fuzzy_eq(render_mesh->vertexes[shape_index * render_mesh->num_vertexes + vertex_index_a].motor[7],
                             render_mesh->vertexes[shape_index * render_mesh->num_vertexes + vertex_index_b].motor[7]))
          continue;
        unique_vertex_id[vertex_index_b] = vertex_index_a;
      }
    }

    // generate normal planes for each triangle
    for(uint32_t triangle_index = 0; triangle_index < render_mesh->num_indexes / 3; triangle_index++) {
      uint32_t index_a = render_mesh->indexes[triangle_index * 3 + 0];
      uint32_t index_b = render_mesh->indexes[triangle_index * 3 + 1];
      uint32_t index_c = render_mesh->indexes[triangle_index * 3 + 2];

      uint32_t unique_index_a = unique_vertex_id[index_a];
      uint32_t unique_index_b = unique_vertex_id[index_b];
      uint32_t unique_index_c = unique_vertex_id[index_c];

      // decompress the vertex motor
      alias_pga3d_Motor vertex_a =
          decompress_motor(render_mesh->vertexes[shape_index * render_mesh->num_vertexes + index_a].motor);
      alias_pga3d_Motor vertex_b =
          decompress_motor(render_mesh->vertexes[shape_index * render_mesh->num_vertexes + index_b].motor);
      alias_pga3d_Motor vertex_c =
          decompress_motor(render_mesh->vertexes[shape_index * render_mesh->num_vertexes + index_c].motor);

      // use vertex motor and point 0,0,0 to get vertex position
      alias_pga3d_Point point_a = alias_pga3d_point(0, 0, 0);
      alias_pga3d_Point point_b = alias_pga3d_point(0, 0, 0);
      alias_pga3d_Point point_c = alias_pga3d_point(0, 0, 0);
      point_a = alias_pga3d_grade_3(alias_pga3d_sandwich(alias_pga3d_t(point_a), alias_pga3d_m(vertex_a)));
      point_b = alias_pga3d_grade_3(alias_pga3d_sandwich(alias_pga3d_t(point_b), alias_pga3d_m(vertex_b)));
      point_c = alias_pga3d_grade_3(alias_pga3d_sandwich(alias_pga3d_t(point_c), alias_pga3d_m(vertex_c)));

      // join points into plane
      triangle_plane[triangle_index] = alias_pga3d_join(alias_pga3d_t(point_a), alias_pga3d_join_tt(point_b, point_c));

      // adding vertex lines
      vertex_line[unique_index_a] =
          alias_pga3d_add(alias_pga3d_b(vertex_line[unique_index_a]),
                          alias_pga3d_inner_product_tv(point_a, triangle_plane[triangle_index]));

      vertex_line[unique_index_b] =
          alias_pga3d_add(alias_pga3d_b(vertex_line[unique_index_b]),
                          alias_pga3d_inner_product_tv(point_b, triangle_plane[triangle_index]));

      vertex_line[unique_index_c] =
          alias_pga3d_add(alias_pga3d_b(vertex_line[unique_index_c]),
                          alias_pga3d_inner_product_tv(point_c, triangle_plane[triangle_index]));
    }

    for(uint32_t vertex_index = 0; vertex_index < render_mesh->num_vertexes; vertex_index++) {
      uint32_t unique_index = unique_vertex_id[vertex_index];

      alias_pga3d_Motor vertex =
          decompress_motor(render_mesh->vertexes[shape_index * render_mesh->num_vertexes + vertex_index].motor);

      alias_pga3d_Point point = alias_pga3d_point(0, 0, 0);
      point = alias_pga3d_grade_3(alias_pga3d_sandwich(alias_pga3d_t(point), alias_pga3d_m(vertex)));

      alias_pga3d_Line line = vertex_line[unique_index];

      line = alias_pga3d_mul(alias_pga3d_s(1.0f / alias_pga3d_norm(alias_pga3d_b(line))), alias_pga3d_b(line));

      alias_pga3d_Motor motor = alias_pga3d_motor_to(0, line, point);

      compress_motor(render_mesh->vertexes[shape_index * render_mesh->num_vertexes + vertex_index].motor, motor);
    }
  }

  alias_free(NULL, triangle_plane, sizeof(*triangle_plane) * (render_mesh->num_indexes / 3), 4);
  alias_free(NULL, unique_vertex_id, sizeof(*unique_vertex_id) * render_mesh->num_vertexes, 4);
  alias_free(NULL, vertex_line, sizeof(*vertex_line) * render_mesh->num_vertexes, 4);
}