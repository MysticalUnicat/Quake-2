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
#define THIN_GL_MAX_BUFFERS 16

#define THIN_GL_VERTEX_BIT 0x01
#define THIN_GL_GEOMETRY_BIT 0x02
#define THIN_GL_TESSC_BIT 0x04
#define THIN_GL_TESSE_BIT 0x08
#define THIN_GL_FRAGMENT_BIT 0x10

#define THIN_GL_LOCAL_GROUP_SIZE_SUBGROUP_SIZE 0xFFFFFFFF

typedef void (*GL_VoidFn)(void);

// --------------------------------------------------------------------------------------------------------------------
// C to OpenGL data format
enum GL_Type {
  GL_Type_Unused,
  GL_Type_Float,
  GL_Type_Float2,
  GL_Type_Float3,
  GL_Type_Float4,
  GL_Type_Double,
  GL_Type_Double2,
  GL_Type_Double3,
  GL_Type_Double4,
  GL_Type_Int,
  GL_Type_Int2,
  GL_Type_Int3,
  GL_Type_Int4,
  GL_Type_Uint,
  GL_Type_Uint2,
  GL_Type_Uint3,
  GL_Type_Uint4,
  GL_Type_Bool,
  GL_Type_Bool2,
  GL_Type_Bool3,
  GL_Type_Bool4,
  GL_Type_Float2x2,
  GL_Type_Float3x3,
  GL_Type_Float4x4,
  GL_Type_Float2x3,
  GL_Type_Float2x4,
  GL_Type_Float3x2,
  GL_Type_Float3x4,
  GL_Type_Float4x2,
  GL_Type_Float4x3,
  GL_Type_Double2x2,
  GL_Type_Double3x3,
  GL_Type_Double4x4,
  GL_Type_Double2x3,
  GL_Type_Double2x4,
  GL_Type_Double3x2,
  GL_Type_Double3x4,
  GL_Type_Double4x2,
  GL_Type_Double4x3,
  GL_Type_Sampler1D,
  GL_Type_Sampler2D,
  GL_Type_Sampler3D,
  GL_Type_SamplerCube,
  GL_Type_Sampler1DShadow,
  GL_Type_Sampler2DShadow,
  GL_Type_Sampler1DArray,
  GL_Type_Sampler2DArray,
  GL_Type_SamplerCubeArray,
  GL_Type_Sampler1DArrayShadow,
  GL_Type_Sampler2DArrayShadow,
  GL_Type_Sampler2DMultisample,
  GL_Type_Sampler2DMultisampleArray,
  GL_Type_SamplerCubeShadow,
  GL_Type_SamplerCubeArrayShadow,
  GL_Type_SamplerBuffer,
  GL_Type_Sampler2DRect,
  GL_Type_Sampler2DRectShadow,
  GL_Type_IntSampler1D,
  GL_Type_IntSampler2D,
  GL_Type_IntSampler3D,
  GL_Type_IntSamplerCube,
  GL_Type_IntSampler1DArray,
  GL_Type_IntSampler2DArray,
  GL_Type_IntSamplerCubeMapArray,
  GL_Type_IntSampler2DMultisample,
  GL_Type_IntSampler2DMultisampleArray,
  GL_Type_IntSamplerBuffer,
  GL_Type_IntSampler2DRect,
  GL_Type_UintSampler1D,
  GL_Type_UintSampler2D,
  GL_Type_UintSampler3D,
  GL_Type_UintSamplerCube,
  GL_Type_UintSampler1DArray,
  GL_Type_UintSampler2DArray,
  GL_Type_UintSamplerCubeMapArray,
  GL_Type_UintSampler2DMultisample,
  GL_Type_UintSampler2DMultisampleArray,
  GL_Type_UintSamplerBuffer,
  GL_Type_UintSampler2DRect,
  GL_Type_Image1D,
  GL_Type_Image2D,
  GL_Type_Image3D,
  GL_Type_Image2DRect,
  GL_Type_ImageCube,
  GL_Type_ImageBuffer,
  GL_Type_Image1DArray,
  GL_Type_Image2DArray,
  GL_Type_ImageCubeArray,
  GL_Type_Image2DMultisample,
  GL_Type_Image2DMultisampleArray,
  GL_Type_IntImage1D,
  GL_Type_IntImage2D,
  GL_Type_IntImage3D,
  GL_Type_IntImage2DRect,
  GL_Type_IntImageCube,
  GL_Type_IntImageBuffer,
  GL_Type_IntImage1DArray,
  GL_Type_IntImage2DArray,
  GL_Type_IntImageCubeArray,
  GL_Type_IntImage2DMultisample,
  GL_Type_IntImage2DMultisampleArray,
  GL_Type_UintImage1D,
  GL_Type_UintImage2D,
  GL_Type_UintImage3D,
  GL_Type_UintImage2DRect,
  GL_Type_UintImageCube,
  GL_Type_UintImageBuffer,
  GL_Type_UintImage1DArray,
  GL_Type_UintImage2DArray,
  GL_Type_UintImageCubeArray,
  GL_Type_UintImage2DMultisample,
  GL_Type_UintImage2DMultisampleArray,
  GL_Type_AtomicUint,
  GL_Type_InlineStructure, // valid in THIN_GL_STRUCT and THIN_GL_BLOCK
  GL_Type_ShaderStorageBuffer
};

struct GL_UniformData {
  const void *pointer;
  GLboolean transpose;
  const struct GL_Buffer *buffer;
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

struct GL_ShaderSnippet {
  const struct GL_ShaderSnippet *requires[8];
  const char *code;

  /* internal */
  uint32_t structure_size;
  GLuint emit_index;
};

struct GL_Shader {
  const struct GL_ShaderSnippet *requires[8];
  const char *code;

  /* internal */
  GLuint object;
};

struct GL_ShaderResource {
  enum GL_Type type;
  const char *name;
  GLsizei count;
  union {
    struct {
      GL_VoidFn prepare;
      uint32_t prepare_draw_index;
      struct GL_UniformData data;
    } uniform;
    struct {
      struct GL_ShaderSnippet *snippet;
      union {
        struct GL_Buffer *buffer;
        struct GL_Buffer *buffers[THIN_GL_MAX_BUFFERS];
      };
    } block;
  };
};

// --------------------------------------------------------------------------------------------------------------------
// state setup for draw/compute
// - interface defined in C ensures GLSL and C expect the same thing

#define THIN_GL_PIPILINE_STATE_FIELDS                                                                                  \
  struct {                                                                                                             \
    GLbitfield stage_bits;                                                                                             \
    enum GL_Type type;                                                                                                 \
    const char *name;                                                                                                  \
    GLsizei count;                                                                                                     \
    const struct GL_ShaderSnippet *struture;                                                                           \
  } uniform[THIN_GL_MAX_UNIFORMS];                                                                                     \
  struct {                                                                                                             \
    GLbitfield stage_bits;                                                                                             \
    struct GL_ShaderResource *resource;                                                                                \
  } global[THIN_GL_MAX_UNIFORMS];                                                                                      \
  struct {                                                                                                             \
    GLbitfield stage_bits;                                                                                             \
    enum GL_Type type;                                                                                                 \
    const char *name;                                                                                                  \
  } image[THIN_GL_MAX_IMAGES];                                                                                         \
  /* internal */                                                                                                       \
  GLuint program_object;

struct GL_PipelineState {
  THIN_GL_PIPILINE_STATE_FIELDS
};

#define THIN_GL_STAGE_FIELDS                                                                                           \
  const char *source;                                                                                                  \
  /* internal */                                                                                                       \
  GLuint shader_object;

struct GL_VertexStage {
  THIN_GL_STAGE_FIELDS

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

  // internal
  GLuint vertex_array_object;
};

struct GL_DrawState {
  THIN_GL_PIPILINE_STATE_FIELDS

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

  const struct GL_Shader *vertex_shader;

  bool depth_test_enable;
  bool depth_mask;
  float depth_range_min;
  float depth_range_max;

  const struct GL_Shader *fragment_shader;

  bool blend_enable;
  GLenum blend_src_factor;
  GLenum blend_dst_factor;

  /* internal */
  GLuint vertex_shader_object;
  GLuint fragment_shader_object;
  GLuint vertex_array_object;
};

struct GL_ComputeState {
  THIN_GL_PIPILINE_STATE_FIELDS

  const struct GL_Shader *shader;

  GLuint local_group_x;
  GLuint local_group_y;
  GLuint local_group_z;

  /* internal */
  GLuint shader_object;
};

// --------------------------------------------------------------------------------------------------------------------
// asset

struct GL_Buffer {
  enum {
    GL_Buffer_Static,    // never changes, lives on the GPU
    GL_Buffer_Temporary, // The buffer used to send information from the CPU to GPU once
    GL_Buffer_GPU,       // only lives on the GPU
    GL_Buffer_CPU,       // A buffer persantly mapped and bound, updated by the CPU many times
  } kind;
  GLuint buffer;
  GLsizei size;
  void *mapping;
  GLintptr offset;
  bool dirty;
};

// --------------------------------------------------------------------------------------------------------------------
// asset reference collection needed for a draw/compute

#define THIN_GL_PIPILINE_ASSETS_FIELDS                                                                                 \
  GLuint image[THIN_GL_MAX_IMAGES];                                                                                    \
  struct GL_UniformData uniforms[THIN_GL_MAX_UNIFORMS];

struct GL_PipelineAssets {
  THIN_GL_PIPILINE_ASSETS_FIELDS
};

struct GL_DrawAssets {
  THIN_GL_PIPILINE_ASSETS_FIELDS

  const struct GL_Buffer *element_buffer;
  uint32_t element_buffer_offset;

  const struct GL_Buffer *vertex_buffers[THIN_GL_MAX_BINDINGS];
};

struct GL_ComputeAssets {
  THIN_GL_PIPILINE_ASSETS_FIELDS
};

// --------------------------------------------------------------------------------------------------------------------
// draw
void GL_initialize_draw_state(const struct GL_DrawState *state);
GLbitfield GL_apply_draw_state(const struct GL_DrawState *state);
GLbitfield GL_apply_draw_assets(const struct GL_DrawState *state, const struct GL_DrawAssets *assets);

void GL_draw_arrays(const struct GL_DrawState *state, const struct GL_DrawAssets *assets, GLint first, GLsizei count,
                    GLsizei instancecount, GLuint baseinstance);

void GL_draw_elements(const struct GL_DrawState *state, const struct GL_DrawAssets *assets, GLsizei count,
                      GLsizei instancecount, GLint basevertex, GLuint baseinstance);

void GL_draw_elements_indirect(const struct GL_DrawState *state, const struct GL_DrawAssets *assets,
                               const struct GL_Buffer *indirect, GLsizei indirect_offset);

// --------------------------------------------------------------------------------------------------------------------
// compute
void GL_initialize_compute_state(const struct GL_ComputeState *state);
GLbitfield GL_apply_compute_state(const struct GL_ComputeState *state);
GLbitfield GL_apply_compute_assets(const struct GL_ComputeState *state, const struct GL_ComputeAssets *assets);

void GL_compute(const struct GL_ComputeState *state, const struct GL_ComputeAssets *assets, GLuint num_groups_x,
                GLuint num_groups_y, GLuint num_groups_z);

void GL_compute_indirect(const struct GL_ComputeState *state, const struct GL_ComputeAssets *assets,
                         const struct GL_Buffer *indirect, GLsizei indirect_offset);

// --------------------------------------------------------------------------------------------------------------------
// resource data
struct GL_Buffer GL_allocate_static_buffer(GLenum type, GLsizei size, const void *data);

struct GL_Buffer GL_allocate_temporary_buffer(GLenum type, GLsizei size);

struct GL_Buffer GL_allocate_temporary_buffer_from(GLenum type, GLsizei size, const void *data);

void *GL_update_buffer_begin(const struct GL_Buffer *buffer, GLintptr offset, GLsizei size);
void GL_update_buffer_end(const struct GL_Buffer *buffer, GLintptr offset, GLsizei size);

GLbitfield GL_flush_buffer(const struct GL_Buffer *buffer, GLbitfield read_barrier_bits);

void GL_free_buffer(const struct GL_Buffer *buffer);

void GL_reset_temporary_buffers(void);

void GL_temporary_buffer_stats(GLenum type, uint32_t *total_allocated, uint32_t *used);

void GL_destroy_buffer(const struct GL_Buffer *buffer);

// --------------------------------------------------------------------------------------------------------------------
// resource
void GL_ShaderResource_prepare(const struct GL_ShaderResource *resource);

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