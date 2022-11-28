#include "glad.h"

#include <stdbool.h>

struct DrawState {
  GLenum primitive;

  const char *vertex_shader_source;

  bool depth_test_enable;
  bool depth_mask;
  float depth_range_min;
  float depth_range_max;

  const char *fragment_shader_source;

  bool blend_enable;
  GLenum blend_src_factor;
  GLenum blend_dst_factor;

  // opengl objects
  GLuint vertex_shader_object;
  GLuint fragment_shader_object;
  GLuint program_object;
};

struct DrawAssets {
  GLuint images[8];

  // element buffer and format info

  // position buffer and format info
  // dito for tex_coord_1
  //          tex_coord_2
  //          normal
  //          tangent
  //          bitangent
};

void GL_begin_draw(const struct DrawState *state, const struct DrawAssets *assets);
void GL_end_draw(void);
