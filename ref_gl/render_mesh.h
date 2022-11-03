#ifndef RENDER_MESH_H
#define RENDER_MESH_H

#include <alias/pga3d.h>
#include <alias/memory.h>

#define RENDER_MESH_FLAG_TEXTURE_COORD_1 0x01
#define RENDER_MESH_FLAG_TEXTURE_COORD_2 0x02
#define RENDER_MESH_FLAG_TEXTURE_COLOR 0x04

struct RenderMesh_vertex {
  float motor[8];
};

struct RenderMesh_vertex_st {
  float st[2];
};

struct RenderMesh_vertex_bone {
  uint8_t index[4];
  uint8_t weight[4];
};

struct RenderMesh_vertex_color {
  uint8_t rgba[4];
};

struct RenderMesh_joint {
  uint8_t parent;
};

struct RenderMesh_pose {
  float motor[8];
};

struct RenderMesh_submesh {
  uint32_t num_indexes;
  uint32_t first_index;
};

struct RenderMesh {
  uint32_t flags;

  uint32_t num_indexes;
  uint32_t num_submeshes;
  uint32_t num_vertexes;
  uint32_t num_shapes;
  uint32_t num_joints;
  uint32_t num_poses;

  uint32_t *indexes;                    // num_indexes
  struct RenderMesh_submesh *submeshes; // num_submeshes

  struct RenderMesh_vertex *vertexes;             // num_vertexes * num_shapes
  struct RenderMesh_vertex_st *vertexes_st_1;     // num_vertexes
  struct RenderMesh_vertex_st *vertexes_st_2;     // num_vertexes
  struct RenderMesh_vertex_bone *vertexes_bone;   // num_vertexes
  struct RenderMesh_vertex_color *vertexes_color; // num_vertexes

  struct RenderMesh_joint *joints; // num_joints
  struct RenderMesh_pose *poses;   // num_joints * num_poses
};

struct RenderMesh_output {
  alias_memory_SubBuffer indexes;
  alias_memory_SubBuffer position;
  alias_memory_SubBuffer normal;
  alias_memory_SubBuffer tangent;
  alias_memory_SubBuffer bitangent;
  alias_memory_SubBuffer texture_coord_1;
  alias_memory_SubBuffer texture_coord_2;
  alias_memory_SubBuffer color;
};

struct RenderMesh *allocate_RenderMesh(uint32_t flags, uint32_t num_indexes, uint32_t num_submeshes,
                                       uint32_t num_vertexes, uint32_t num_shapes, uint32_t num_joints,
                                       uint32_t num_poses);

void RenderMesh_render(const struct RenderMesh *render_mesh, struct RenderMesh_output *output);

void RenderMesh_render_skinned(const struct RenderMesh *render_mesh, struct RenderMesh_output *output,
                               uint32_t pose_index);

void RenderMesh_render_lerp_skinned(const struct RenderMesh *render_mesh, struct RenderMesh_output *output,
                                    uint32_t pose_index_a, uint32_t pose_index_b, float t);

void RenderMesh_render_shaped(const struct RenderMesh *render_mesh, struct RenderMesh_output *output,
                              uint32_t shape_index);

void RenderMesh_render_lerp_shaped(const struct RenderMesh *render_mesh, struct RenderMesh_output *output,
                                   uint32_t shape_index_a, uint32_t shape_index_b, float t);

void RenderMesh_render_lerp_shaped_skinned(const struct RenderMesh *render_mesh, struct RenderMesh_output *output,
                                           uint32_t pose_index, uint32_t shape_index_a, uint32_t shape_index_b,
                                           float t);

void RenderMesh_render_lerp_shaped_lerp_skinned(const struct RenderMesh *render_mesh, struct RenderMesh_output *output,
                                                uint32_t pose_index_a, uint32_t pose_index_b, float pose_t,
                                                uint32_t shape_index_a, uint32_t shape_index_b, float shape_t);

void RenderMesh_set_position(struct RenderMesh *render_mesh, uint32_t shape_index, uint32_t vertex_index,
                             const float position[3]);

void RenderMesh_set_texture_coord_1(struct RenderMesh *render_mesh, uint32_t vertex_index, const float position[2]);

void RenderMesh_set_texture_coord_2(struct RenderMesh *render_mesh, uint32_t vertex_index, const float position[2]);

void RenderMesh_generate_vertex_orientations(struct RenderMesh *render_mesh);

#endif
