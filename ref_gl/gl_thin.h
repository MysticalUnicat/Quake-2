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

enum DrawUniformType {
  DrawUniformType_Unused,
  DrawUniformType_Float,
  DrawUniformType_Vec2,
  DrawUniformType_Vec3,
  DrawUniformType_Vec4,
  DrawUniformType_Int,
  DrawUniformType_IVec2,
  DrawUniformType_IVec3,
  DrawUniformType_IVec4,
  DrawUniformType_Uint,
  DrawUniformType_UVec2,
  DrawUniformType_UVec3,
  DrawUniformType_UVec4,
  DrawUniformType_Mat2,
  DrawUniformType_Mat3,
  DrawUniformType_Mat4,
  DrawUniformType_Mat2x3,
  DrawUniformType_Mat3x2,
  DrawUniformType_Mat2x4,
  DrawUniformType_Mat4x2,
  DrawUniformType_Mat3x4,
  DrawUniformType_Mat4x3
};

struct DrawAssets {
  GLuint images[8];

  struct {
    enum DrawUniformType type;
    GLsizei count;
    bool transpose;
    const void *pointer;
    union {
      float _float;
      float vec2[2];
      float vec3[3];
      float vec4[4];
      GLint _int;
      GLint ivec2[2];
      GLint ivec3[3];
      GLint ivec4[4];
      GLuint uint;
      GLuint uvec2[2];
      GLuint uvec3[3];
      GLuint uvec4[4];
    };
  } uniforms[8];

  // element buffer and format info

  // position buffer and format info
  // dito for tex_coord_1
  //          tex_coord_2
  //          normal
  //          tangent
  //          bitangent
};

void GL_initialize_draw_state(const struct DrawState *state);
void GL_apply_draw_state(const struct DrawState *state);
void GL_apply_draw_assets(const struct DrawAssets *assets);

void GL_begin_draw(const struct DrawState *state, const struct DrawAssets *assets);
void GL_end_draw(void);
