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

struct GL_VertexFormat {
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
};

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

struct GL_UniformsFormat {
  struct {
    GLbitfield stage_bits;
    enum GL_UniformType type;
    const char *name;
    GLsizei count;
  } uniform[THIN_GL_MAX_UNIFORMS];
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

struct GL_ImagesFormat {
  struct {
    GLbitfield stage_bits;
    enum GL_ImageType type;
    const char *name;
  } image[THIN_GL_MAX_IMAGES];
};

struct DrawState {
  GLenum primitive;

  const struct GL_VertexFormat *vertex_format;

  const struct GL_UniformsFormat *uniforms_format;

  const struct GL_ImagesFormat *images_format;

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

struct GL_DrawAssetBinding {
  const struct GL_Buffer *buffer;
};

struct GL_DrawAssetUniform {
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

struct DrawAssets {
  GLuint image[THIN_GL_MAX_IMAGES];

  struct GL_DrawAssetUniform uniforms[THIN_GL_MAX_UNIFORMS];

  const struct GL_Buffer *element_buffer;
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

bool GL_flush_buffer(const struct GL_Buffer *buffer);

void GL_free_buffer(const struct GL_Buffer *buffer);

void GL_reset_temporary_buffers(void);
