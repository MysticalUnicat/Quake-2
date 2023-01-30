#ifndef __GL_THIN_H__
#define __GL_THIN_H__

#include "glad.h"

#include <alias/memory.h>
#include <alias/math.h>
#include <stdbool.h>

#define THIN_GL_MAX_ATTRIBUTES 16
#define THIN_GL_MAX_BINDINGS 2
#define THIN_GL_MAX_UNIFORMS 16
#define THIN_GL_MAX_IMAGES 16

#define THIN_GL_VERTEX_BIT 0x01
#define THIN_GL_GEOMETRY_BIT 0x02
#define THIN_GL_TESSC_BIT 0x04
#define THIN_GL_TESSE_BIT 0x08
#define THIN_GL_FRAGMENT_BIT 0x10

typedef void (*GL_VoidFn)(void);

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
    float vec[4];
    float mat[16];

    GLint _int;
    GLint ivec[4];
    GLint imat[16];

    GLuint uint;
    GLuint uvec[4];
    GLuint umat[16];
  };
};

struct GL_Uniform {
  enum GL_UniformType type;
  const char *name;
  GLsizei count;
  struct GL_UniformData data;
  GL_VoidFn prepare;
  uint32_t prepare_draw_index;
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
    GLuint location;
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

void GL_Uniform_prepare(const struct GL_Uniform *uniform);

// mimic OpenGL's matrix
static inline void GL_matrix_construct(float xx, float yx, float zx, float wx, float xy, float yy, float zy, float wy,
                                       float xz, float yz, float zz, float wz, float xw, float yw, float zw, float ww,
                                       float result[16]) {
  result[0] = xx;
  result[1] = xy;
  result[2] = xz;
  result[3] = xw;
  result[4] = yx;
  result[5] = yy;
  result[6] = yz;
  result[7] = yw;
  result[8] = zx;
  result[9] = zy;
  result[10] = zz;
  result[11] = zw;
  result[12] = wx;
  result[13] = wy;
  result[14] = wz;
  result[15] = ww;
}

static inline void GL_matrix_identity(float result[16]) {
  // clang-format off
  GL_matrix_construct(
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
    result
  );
  // clang-format on
}

static inline void GL_matrix_translation(float x, float y, float z, float result[16]) {
  // clang-format off
  GL_matrix_construct(
    1, 0, 0, x,
    0, 1, 0, y,
    0, 0, 1, z,
    0, 0, 0, 1,
    result
  );
  // clang-format on
}

static inline void GL_matrix_rotation(float angle, float x, float y, float z, float result[16]) {
  double magnitude = sqrt(x * x + y * y + z * z);
  if(magnitude < alias_R_MIN) {
    GL_matrix_identity(result);
    return;
  }
  double one_over_magnitude = 1.0 / magnitude;
  double a = angle * 0.01745329251;
  x *= one_over_magnitude;
  y *= one_over_magnitude;
  z *= one_over_magnitude;
  double s = sin(a);
  double c = cos(a);
  double one_minus_c = 1.0 - c;
  double xs = x * s;
  double ys = y * s;
  double zs = z * s;
  double xc = x * one_minus_c;
  double yc = y * one_minus_c;
  double zc = z * one_minus_c;
  double m_xx = (xc * x) + c;
  double m_xy = (xc * y) + zs;
  double m_xz = (xc * z) - ys;
  double m_yx = (yc * x) - zs;
  double m_yy = (yc * y) + c;
  double m_yz = (yc * z) + xs;
  double m_zx = (zc * x) + ys;
  double m_zy = (zc * y) - xs;
  double m_zz = (zc * z) + c;
  // clang-format off
  GL_matrix_construct(
    m_xx, m_yx, m_zx, 0,
    m_xy, m_yy, m_zy, 0,
    m_xz, m_yz, m_zz, 0,
       0,   0,     0, 1,
    result
  );
  // clang-format on
}

static inline void GL_matrix_rotation_x(float angle, float result[16]) {
  float a = angle * 0.01745329251;
  float s = sin(a);
  float c = cos(a);
  // clang-format off
  GL_matrix_construct(
    1, 0,  0, 0,
    0, c, -s, 0,
    0, s,  c, 0,
    0, 0,  0, 1,
    result
  );
  // clang-format on
}

static inline void GL_matrix_rotation_y(float angle, float result[16]) {
  float a = angle * 0.01745329251;
  float s = sin(a);
  float c = cos(a);
  // clang-format off
  GL_matrix_construct(
     c, 0, s, 0,
     0, 1, 0, 0,
    -s, 0, c, 0,
     0, 0, 0, 1,
    result
  );
  // clang-format on
}

static inline void GL_matrix_rotation_z(float angle, float result[16]) {
  float a = angle * 0.01745329251;
  float s = sin(a);
  float c = cos(a);
  // clang-format off
  GL_matrix_construct(
    c, -s, 0, 0,
    s,  c, 0, 0,
    0,  0, 1, 0,
    0,  0, 0, 1,
    result
  );
  // clang-format on
}

static inline void GL_matrix_frustum(float left, float right, float bottom, float top, float near, float far,
                                     float result[16]) {
  float x = (2 * near) / (right - left);
  float y = (2 * near) / (top - bottom);
  float a = (right + left) / (right - left);
  float b = (top + bottom) / (top - bottom);
  float c = -(far + near) / (far - near);
  float d = -(2 * far * near) / (far - near);
  // clang-format off
  GL_matrix_construct(
      x, 0,  a,  0,
      0, y,  b,  0,
      0, 0,  c,  d,
      0, 0, -1,  0,
    result
  );
  // clang-format on
}

static inline void GL_matrix_ortho(float left, float right, float bottom, float top, float near, float far,
                                   float result[16]) {
  float x = 2 / (right - left);
  float y = 2 / (top - bottom);
  float z = -2 / (far - near);
  float a = -(right + left) / (right - left);
  float b = -(top + bottom) / (top - bottom);
  float c = -(far + near) / (far - near);
  // clang-format off
  GL_matrix_construct(
      x, 0, 0, a,
      0, y, 0, b,
      0, 0, x, c,
      0, 0, 0, 1,
    result
  );
  // clang-format on
}

static inline void GL_matrix_multiply(const float a[16], const float b[16], float result[16]) {
  float m[16];
  for(uint32_t i = 0; i < 4; i++) {
    float row[4] = {a[0 + i], a[4 + i], a[8 + i], a[12 + i]};
    // clang-format off
    m[ 0 + i] = row[0] * b[ 0 + 0] + row[1] * b[ 0 + 1] + row[2] * b[ 0 + 2] + row[3] * b[ 0 + 3];
    m[ 4 + i] = row[0] * b[ 4 + 0] + row[1] * b[ 4 + 1] + row[2] * b[ 4 + 2] + row[3] * b[ 4 + 3];
    m[ 8 + i] = row[0] * b[ 8 + 0] + row[1] * b[ 8 + 1] + row[2] * b[ 8 + 2] + row[3] * b[ 8 + 3];
    m[12 + i] = row[0] * b[12 + 0] + row[1] * b[12 + 1] + row[2] * b[12 + 2] + row[3] * b[12 + 3];
    // clang-format on
  }
  alias_memory_copy(result, sizeof(float) * 16, m, sizeof(float) * 16);
}

static inline void GL_translate(float matrix[16], float x, float y, float z) {
  float temp[16];
  GL_matrix_translation(x, y, z, temp);
  GL_matrix_multiply(matrix, temp, matrix);
}

static inline void GL_rotate(float matrix[16], float angle, float x, float y, float z) {
  float temp[16];
  GL_matrix_rotation(angle, x, y, z, temp);
  GL_matrix_multiply(matrix, temp, matrix);
}

static inline void GL_rotate_x(float matrix[16], float angle) {
  float temp[16];
  GL_matrix_rotation_x(angle, temp);
  GL_matrix_multiply(matrix, temp, matrix);
}

static inline void GL_rotate_y(float matrix[16], float angle) {
  float temp[16];
  GL_matrix_rotation_y(angle, temp);
  GL_matrix_multiply(matrix, temp, matrix);
}

static inline void GL_rotate_z(float matrix[16], float angle) {
  float temp[16];
  GL_matrix_rotation_z(angle, temp);
  GL_matrix_multiply(matrix, temp, matrix);
}

#endif