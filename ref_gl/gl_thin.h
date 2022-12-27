#ifndef __GL_THIN_H__
#define __GL_THIN_H__

#include "glad.h"

#include <alias/memory.h>
#include <stdbool.h>

#define THIN_GL_MAX_ATTRIBUTES 8
#define THIN_GL_MAX_BINDINGS 2
#define THIN_GL_MAX_UNIFORMS 16
#define THIN_GL_MAX_IMAGES 16

#define THIN_GL_VERTEX_BIT 0x01
#define THIN_GL_GEOMETRY_BIT 0x02
#define THIN_GL_TESSC_BIT 0x04
#define THIN_GL_TESSE_BIT 0x08
#define THIN_GL_FRAGMENT_BIT 0x10

enum GL_UniformType {
  GL_UniformType_Unused,
  GL_UniformType_Float,
  GL_UniformType_Vec2,
  GL_UniformType_Vec3,
  GL_UniformType_Vec4,
  GL_UniformType_Int,
  GL_UniformType_IVec2,
  GL_UniformType_IVec3,
  GL_UniformType_IVec4,
  GL_UniformType_Uint,
  GL_UniformType_UVec2,
  GL_UniformType_UVec3,
  GL_UniformType_UVec4,
  GL_UniformType_Mat2,
  GL_UniformType_Mat3,
  GL_UniformType_Mat4,
  GL_UniformType_Mat2x3,
  GL_UniformType_Mat3x2,
  GL_UniformType_Mat2x4,
  GL_UniformType_Mat4x2,
  GL_UniformType_Mat3x4,
  GL_UniformType_Mat4x3,
};

struct GL_UniformData {
  const void *pointer;
  GLboolean transpose;
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
};

struct GL_Uniform {
  enum GL_UniformType type;
  const char *name;
  GLsizei count;
  GLboolean transpose;
  union {
    float mat4[16];
    GLint imat4[16];
    GLuint umat4[16];
  };
};

// NOTE: up to sampler 2D (that was needed at the time)
enum GL_ImageType {
  GL_ImageType_Unknown,
  GL_ImageType_Sampler1D,
  GL_ImageType_Texture1D,
  GL_ImageType_Image1D,
  GL_ImageType_Sampler1DShadow,
  GL_ImageType_Sampler1DArray,
  GL_ImageType_Texture1DArray,
  GL_ImageType_Sampler1DArrayShadow,
  GL_ImageType_Sampler2D,
};

struct DrawState {
  GLenum primitive;

  struct {
    GLuint binding;
    alias_memory_Format format;
    GLint size;
    const GLchar *name;
    GLuint offset;
  } attribute[THIN_GL_MAX_ATTRIBUTES];

  struct {
    GLsizei stride;
    GLuint divisor;
  } binding[THIN_GL_MAX_BINDINGS];

  struct {
    GLbitfield stage_bits;
    enum GL_UniformType type;
    const char *name;
    GLsizei count;
  } uniform[THIN_GL_MAX_UNIFORMS];

  struct {
    GLbitfield stage_bits;
    struct GL_Uniform *uniform;
  } global[THIN_GL_MAX_UNIFORMS];

  struct {
    GLbitfield stage_bits;
    enum GL_ImageType type;
    const char *name;
  } image[THIN_GL_MAX_IMAGES];

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
  GLuint vertex_array_object;
};

struct GL_Buffer {
  enum { GL_Buffer_Static, GL_Buffer_Temporary } kind;
  GLuint buffer;
  GLsizei size;
  union {
    struct {
      void *mapping;
      GLintptr offset;
      bool dirty;
    } temporary;
  };
};

struct DrawAssets {
  GLuint image[THIN_GL_MAX_IMAGES];

  struct GL_UniformData uniforms[THIN_GL_MAX_UNIFORMS];

  const struct GL_Buffer *element_buffer;
  uint32_t element_buffer_offset;

  const struct GL_Buffer *vertex_buffers[THIN_GL_MAX_BINDINGS];
};

void GL_initialize_draw_state(const struct DrawState *state);
void GL_apply_draw_state(const struct DrawState *state);
void GL_apply_draw_assets(const struct DrawState *state, const struct DrawAssets *assets);

void GL_begin_draw(const struct DrawState *state, const struct DrawAssets *assets);
void GL_end_draw(void);

void GL_draw_arrays(const struct DrawState *state, const struct DrawAssets *assets, GLint first, GLsizei count,
                    GLsizei instancecount, GLuint baseinstance);

void GL_draw_elements(const struct DrawState *state, const struct DrawAssets *assets, GLsizei count,
                      GLsizei instancecount, GLint basevertex, GLuint baseinstance);

struct GL_Buffer GL_allocate_static_buffer(GLenum type, GLsizei size, const void *data);

struct GL_Buffer GL_allocate_temporary_buffer(GLenum type, GLsizei size);

struct GL_Buffer GL_allocate_temporary_buffer_from(GLenum type, GLsizei size, const void *data);

bool GL_flush_buffer(const struct GL_Buffer *buffer);

void GL_free_buffer(const struct GL_Buffer *buffer);

void GL_reset_temporary_buffers(void);

void GL_temporary_buffer_stats(GLenum type, uint32_t *total_allocated, uint32_t *used);

// mimic OpenGL's matrix
static inline void GL_matrix_identity(float result[16]) {
  // clang-format off
  memcpy(result, (float[16]){
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
  }, sizeof(float) * 16);
  // clang-format on
}

static inline void GL_matrix_translation(float x, float y, float z, float result[16]) {
  // clang-format off
  memcpy(result, (float[16]){
      1, 0, 0, x,
      0, 1, 0, y,
      0, 0, 1, z,
      0, 0, 0, 1,
  }, sizeof(float) * 16);
  // clang-format on
}

static inline void GL_matrix_rotation(float angle, float x, float y, float z, float result[16]) {
  float c = cos(angle * 0.0174533);
  float s = sin(angle * 0.0174533);
  float xyz = 1.0 / sqrt(x * x + y * y + z * z);
  x *= xyz;
  y *= xyz;
  z *= xyz;
  float xx = x * x;
  float yy = y * y;
  float zz = z * z;
  float xy = x * y;
  float xz = x * z;
  float yz = y * z;
  // clang-format off
  memcpy(result, (float[16]){
    xx - c +   c, xy - c - z*c, xz - c + y*s, 0,
    xy - c + z*s, yy - c +   c, yz - c - x*s, 0,
    xz - c + y*s, yz - c + x*s, zz - c +   c, 0,
               0,            0,            0, 1,
  }, sizeof(float) * 16);
  // clang-format on
}

static inline void GL_matrix_frustum(float left, float right, float bottom, float top, float near, float far,
                                     float result[16]) {
  float xx = (2 * near) / (right - left);
  float yy = (2 * near) / (top - bottom);
  float a = (right + left) / (right - left);
  float b = (top + bottom) / (top - bottom);
  float c = -(far + near) / (far - near);
  float d = -(2 * far * near) / (far - near);
  // clang-format off
  memcpy(result, (float[16]){
      xx,  0,  a, 0,
       0, yy,  b, 0,
       0,  0,  c, d,
       0,  0, -1, 0,
  }, sizeof(float) * 16);
  // clang-format on
}

static inline void GL_matrix_ortho(float left, float right, float bottom, float top, float near, float far,
                                   float result[16]) {
  float xx = 2 / (right - left);
  float yy = 2 / (top - bottom);
  float zz = -2 / (far - near);
  float x = -(right + left) / (right - left);
  float y = -(top + bottom) / (top - bottom);
  float z = -(far + near) / (far - near);
  // clang-format off
  memcpy(result, (float[16]){
      xx,  0,  0, x,
       0, yy,  0, y,
       0,  0, zz, z,
       0,  0,  0, 1,
  }, sizeof(float) * 16);
  // clang-format on
}

static inline void GL_matrix_mulitply(const float a[16], const float b[16], float result[16]) {
  // clang-format off
  memcpy(result, (float[16]){
    a[0 + 0*4] * b[0 + 0*4] + a[0 + 1*4] * b[1 + 0*4] + a[0 + 2*4] * b[2 + 0*4] + a[0 + 3*4] * b[3 + 0*4],
    a[0 + 0*4] * b[0 + 1*4] + a[0 + 1*4] * b[1 + 1*4] + a[0 + 2*4] * b[2 + 1*4] + a[0 + 3*4] * b[3 + 1*4],
    a[0 + 0*4] * b[0 + 2*4] + a[0 + 1*4] * b[1 + 2*4] + a[0 + 2*4] * b[2 + 2*4] + a[0 + 3*4] * b[3 + 2*4],
    a[0 + 0*4] * b[0 + 3*4] + a[0 + 1*4] * b[1 + 3*4] + a[0 + 2*4] * b[2 + 3*4] + a[0 + 3*4] * b[3 + 3*4],

    a[1 + 0*4] * b[0 + 0*4] + a[1 + 1*4] * b[1 + 0*4] + a[1 + 2*4] * b[2 + 0*4] + a[1 + 3*4] * b[3 + 0*4],
    a[1 + 0*4] * b[0 + 1*4] + a[1 + 1*4] * b[1 + 1*4] + a[1 + 2*4] * b[2 + 1*4] + a[1 + 3*4] * b[3 + 1*4],
    a[1 + 0*4] * b[0 + 2*4] + a[1 + 1*4] * b[1 + 2*4] + a[1 + 2*4] * b[2 + 2*4] + a[1 + 3*4] * b[3 + 2*4],
    a[1 + 0*4] * b[0 + 3*4] + a[1 + 1*4] * b[1 + 3*4] + a[1 + 2*4] * b[2 + 3*4] + a[1 + 3*4] * b[3 + 3*4],

    a[2 + 0*4] * b[0 + 0*4] + a[2 + 1*4] * b[1 + 0*4] + a[2 + 2*4] * b[2 + 0*4] + a[2 + 3*4] * b[3 + 0*4],
    a[2 + 0*4] * b[0 + 1*4] + a[2 + 1*4] * b[1 + 1*4] + a[2 + 2*4] * b[2 + 1*4] + a[2 + 3*4] * b[3 + 1*4],
    a[2 + 0*4] * b[0 + 2*4] + a[2 + 1*4] * b[1 + 2*4] + a[2 + 2*4] * b[2 + 2*4] + a[2 + 3*4] * b[3 + 2*4],
    a[2 + 0*4] * b[0 + 3*4] + a[2 + 1*4] * b[1 + 3*4] + a[2 + 2*4] * b[2 + 3*4] + a[2 + 3*4] * b[3 + 3*4],

    a[3 + 0*4] * b[0 + 0*4] + a[3 + 1*4] * b[1 + 0*4] + a[3 + 2*4] * b[2 + 0*4] + a[3 + 3*4] * b[3 + 0*4],
    a[3 + 0*4] * b[0 + 1*4] + a[3 + 1*4] * b[1 + 1*4] + a[3 + 2*4] * b[2 + 1*4] + a[3 + 3*4] * b[3 + 1*4],
    a[3 + 0*4] * b[0 + 2*4] + a[3 + 1*4] * b[1 + 2*4] + a[3 + 2*4] * b[2 + 2*4] + a[3 + 3*4] * b[3 + 2*4],
    a[3 + 0*4] * b[0 + 3*4] + a[3 + 1*4] * b[1 + 3*4] + a[3 + 2*4] * b[2 + 3*4] + a[3 + 3*4] * b[3 + 3*4],
  }, sizeof(float) * 16);
  // clang-format on
}

static inline void GL_translate(float matrix[16], float x, float y, float z) {
  float temp[16];
  GL_matrix_translation(x, y, z, temp);
  GL_matrix_mulitply(matrix, temp, matrix);
}

static inline void GL_rotate(float matrix[16], float angle, float x, float y, float z) {
  float temp[16];
  GL_matrix_rotation(angle, x, y, z, temp);
  GL_matrix_mulitply(matrix, temp, matrix);
}

#endif