#include "gl_thin.h"

#include "gl_local.h"

struct GL_UniformTypeInfo {
  const char *name;
};

static struct GL_UniformTypeInfo uniform_type_info[] = {
    [GL_UniformType_Float] = {"float"},   [GL_UniformType_Vec2] = {"vec2"},     [GL_UniformType_Vec3] = {"vec3"},
    [GL_UniformType_Vec4] = {"vec4"},     [GL_UniformType_Int] = {"int"},       [GL_UniformType_IVec2] = {"ivec2"},
    [GL_UniformType_IVec3] = {"ivec3"},   [GL_UniformType_IVec4] = {"ivec4"},   [GL_UniformType_Uint] = {"uint"},
    [GL_UniformType_UVec2] = {"uvec2"},   [GL_UniformType_UVec3] = {"uvec3"},   [GL_UniformType_UVec4] = {"uvec4"},
    [GL_UniformType_Mat2] = {"mat2"},     [GL_UniformType_Mat3] = {"mat3"},     [GL_UniformType_Mat4] = {"mat4"},
    [GL_UniformType_Mat2x3] = {"mat2x3"}, [GL_UniformType_Mat3x2] = {"mat3x2"}, [GL_UniformType_Mat2x4] = {"mat2x4"},
    [GL_UniformType_Mat4x2] = {"mat4x2"}, [GL_UniformType_Mat3x4] = {"mat3x4"}, [GL_UniformType_Mat4x3] = {"mat4x3"},
};

struct GL_ImageTypeInfo {
  GLenum target;
  const char *name;
};

static struct GL_ImageTypeInfo image_type_info[] = {
    [GL_ImageType_Sampler1D] = {GL_TEXTURE_1D, "sampler1D"},
    [GL_ImageType_Texture1D] = {GL_TEXTURE_1D, "texture1D"},
    [GL_ImageType_Image1D] = {GL_TEXTURE_1D, "image1D"},
    [GL_ImageType_Sampler1DShadow] = {GL_TEXTURE_1D_ARRAY, "sampler1DShadow"},
    [GL_ImageType_Sampler1DArray] = {GL_TEXTURE_1D_ARRAY, "sampler1DArray"},
    [GL_ImageType_Texture1DArray] = {GL_TEXTURE_1D_ARRAY, "texture1DArray"},
    [GL_ImageType_Sampler1DArrayShadow] = {GL_TEXTURE_1D_ARRAY, "sampler1DArrayShadow"},
    [GL_ImageType_Sampler2D] = {GL_TEXTURE_2D, "sampler2D"},
};

struct TemporaryBuffer {
  GLuint buffer;
  GLenum type;
  GLsizei size;
  GLsizei offset;
  void *mapped_memory;
};

static struct {
  uint32_t num_temporary_buffers;
  struct TemporaryBuffer *temporary_buffers;

  char *script_builder_ptr;
  uint32_t script_builder_cap;
  uint32_t script_builder_len;

  uint32_t draw_index;
} _ = {0, 0, 0, 0};

void script_builder_init(void) { _.script_builder_len = 0; }

void script_builder_add(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  uint32_t len = vsnprintf(NULL, 0, format, ap);
  va_end(ap);
  if(_.script_builder_len + len > _.script_builder_cap) {
    _.script_builder_cap = _.script_builder_len + len + 1;
    _.script_builder_cap += _.script_builder_cap >> 1;
    _.script_builder_ptr = realloc(_.script_builder_ptr, sizeof(*_.script_builder_ptr) * _.script_builder_cap);
  }
  va_start(ap, format);
  vsnprintf(_.script_builder_ptr + _.script_builder_len, _.script_builder_cap - _.script_builder_len, format, ap);
  _.script_builder_len += len;
  _.script_builder_ptr[_.script_builder_len] = 0;
  va_end(ap);
}

static void script_builder_add_vertex_format(const struct DrawState *draw_state) {
  for(int i = 0; i < THIN_GL_MAX_ATTRIBUTES; i++) {
    if(draw_state->attribute[i].format == 0)
      break;

    script_builder_add("layout(location=%i) in ", i);
    const char *type_name, *vec_name;
    switch(draw_state->attribute[i].format) {
    case alias_memory_Format_Uint8:
    case alias_memory_Format_Uint16:
    case alias_memory_Format_Uint32:
    case alias_memory_Format_Uint64:
      type_name = "uint";
      vec_name = "uvec";
      break;
    case alias_memory_Format_Sint8:
    case alias_memory_Format_Sint16:
    case alias_memory_Format_Sint32:
    case alias_memory_Format_Sint64:
      type_name = "int";
      vec_name = "ivec";
      break;
    case alias_memory_Format_Unorm8:
    case alias_memory_Format_Unorm16:
    case alias_memory_Format_Snorm8:
    case alias_memory_Format_Snorm16:
    case alias_memory_Format_Uscaled8:
    case alias_memory_Format_Uscaled16:
    case alias_memory_Format_Sscaled8:
    case alias_memory_Format_Sscaled16:
    case alias_memory_Format_Urgb8:
    case alias_memory_Format_Float32:
      type_name = "float";
      vec_name = "vec";
      break;
    case alias_memory_Format_Float16:
      type_name = "half_float";
      vec_name = "hvec";
      break;
    case alias_memory_Format_Float64:
      type_name = "double";
      vec_name = "dvec";
      break;
    }
    if(draw_state->attribute[i].size > 1) {
      script_builder_add("%s%i in_%s;\n", vec_name, draw_state->attribute[i].size, draw_state->attribute[i].name);
    } else {
      script_builder_add("%s in_%s;\n", type_name, draw_state->attribute[i].name);
    }
  }
}

static void script_builder_add_uniform_format(const struct DrawState *draw_state, GLbitfield stage_bit) {
  GLuint location = 0;
  for(uint32_t i = 0; i < THIN_GL_MAX_UNIFORMS; i++) {
    if(!draw_state->uniform[i].stage_bits)
      continue;
    location = i + 1;
    if(!(draw_state->uniform[i].stage_bits & stage_bit))
      continue;
    script_builder_add("layout(location=%i) uniform %s u_%s;\n", i, uniform_type_info[draw_state->uniform[i].type].name,
                       draw_state->uniform[i].name);
  }

  for(uint32_t i = 0; i < THIN_GL_MAX_UNIFORMS; i++) {
    if(!draw_state->global[i].stage_bits)
      continue;
    GLuint loc = *(GLuint *)(&draw_state->global[i].location) = location++;
    if(!(draw_state->global[i].stage_bits & stage_bit))
      continue;
    script_builder_add("layout(location=%i) uniform %s u_%s;\n", loc,
                       uniform_type_info[draw_state->global[i].uniform->type].name,
                       draw_state->global[i].uniform->name);
  }
}

static void script_builder_add_images_format(const struct DrawState *draw_state, GLbitfield stage_bit) {
  for(uint32_t i = 0; i < THIN_GL_MAX_IMAGES; i++) {
    if(!(draw_state->image[i].stage_bits & stage_bit))
      continue;
    script_builder_add("layout(binding=%i) uniform %s u_%s;\n", i, image_type_info[draw_state->image[i].type].name,
                       draw_state->image[i].name);
  }
}

void gl_compileShader(GLuint shader) {
  glCompileShader(shader);

  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if(status != GL_TRUE) {
    GLint length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    GLchar *info_log = malloc(length + 1);
    glGetShaderInfoLog(shader, length + 1, NULL, info_log);

    Com_Error(ERR_FATAL, "shader compile error: %s", info_log);

    free(info_log);
  }
}

void gl_linkProgram(GLuint program) {
  glLinkProgram(program);
  GLint status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if(status != GL_TRUE) {
    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

    GLchar *info_log = malloc(length + 1);
    glGetProgramInfoLog(program, length + 1, NULL, info_log);

    Com_Error(ERR_FATAL, "program link error: %s", info_log);

    free(info_log);
  }
}

void glProgram_init(struct glProgram *prog, const char *vsource, const char *fsource) {
  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

  glShaderSource(vertex_shader, 1, &vsource, NULL);
  glShaderSource(fragment_shader, 1, &fsource, NULL);

  gl_compileShader(vertex_shader);
  gl_compileShader(fragment_shader);

  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  gl_linkProgram(program);

  prog->vertex_shader = vertex_shader;
  prog->fragment_shader = fragment_shader;
  prog->program = program;
}

void GL_initialize_draw_state(const struct DrawState *state) {
  if(state->vertex_shader_source != NULL && state->vertex_shader_object == 0) {
    script_builder_init();
    script_builder_add("#version 460 core\n");
    script_builder_add_vertex_format(state);
    script_builder_add_uniform_format(state, THIN_GL_VERTEX_BIT);
    script_builder_add_images_format(state, THIN_GL_VERTEX_BIT);

    GLuint shader = glCreateShader(GL_VERTEX_SHADER);
    const char *sources[2] = {_.script_builder_ptr, state->vertex_shader_source};
    GLint lengths[2] = {_.script_builder_len, strlen(state->vertex_shader_source)};
    glShaderSource(shader, 2, sources, lengths);
    gl_compileShader(shader);
    *(GLuint *)(&state->vertex_shader_object) = shader;
  }

  if(state->fragment_shader_source != NULL && state->fragment_shader_object == 0) {
    script_builder_init();
    script_builder_add("#version 460 core\n");
    script_builder_add_uniform_format(state, THIN_GL_FRAGMENT_BIT);
    script_builder_add_images_format(state, THIN_GL_FRAGMENT_BIT);

    GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);
    const char *sources[2] = {_.script_builder_ptr, state->fragment_shader_source};
    GLint lengths[2] = {_.script_builder_len, strlen(state->fragment_shader_source)};
    glShaderSource(shader, 2, sources, lengths);
    gl_compileShader(shader);
    *(GLuint *)(&state->fragment_shader_object) = shader;
  }

  if(state->vertex_shader_object != 0 && state->fragment_shader_object != 0 && state->program_object == 0) {
    GLuint program = glCreateProgram();
    glAttachShader(program, state->vertex_shader_object);
    glAttachShader(program, state->fragment_shader_object);
    gl_linkProgram(program);
    *(GLuint *)(&state->program_object) = program;
  }

  if(state->attribute[0].format != 0 && state->vertex_array_object == 0) {
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    for(int i = 0; i < THIN_GL_MAX_ATTRIBUTES; i++) {
      if(state->attribute[i].format == 0)
        break;
      glEnableVertexArrayAttrib(vao, i);
      switch(state->attribute[i].format) {
      case alias_memory_Format_Uint8:
        glVertexArrayAttribIFormat(vao, i, state->attribute[i].size, GL_UNSIGNED_BYTE, state->attribute[i].offset);
        break;
      case alias_memory_Format_Uint16:
        glVertexArrayAttribIFormat(vao, i, state->attribute[i].size, GL_UNSIGNED_SHORT, state->attribute[i].offset);
        break;
      case alias_memory_Format_Uint32:
        glVertexArrayAttribIFormat(vao, i, state->attribute[i].size, GL_UNSIGNED_INT, state->attribute[i].offset);
        break;
      case alias_memory_Format_Uint64:
        //
        break;
      case alias_memory_Format_Sint8:
        glVertexArrayAttribIFormat(vao, i, state->attribute[i].size, GL_BYTE, state->attribute[i].offset);
        break;
      case alias_memory_Format_Sint16:
        glVertexArrayAttribIFormat(vao, i, state->attribute[i].size, GL_SHORT, state->attribute[i].offset);
        break;
      case alias_memory_Format_Sint32:
        glVertexArrayAttribIFormat(vao, i, state->attribute[i].size, GL_INT, state->attribute[i].offset);
        break;
      case alias_memory_Format_Sint64:
        //
        break;
      case alias_memory_Format_Unorm8:
        glVertexArrayAttribFormat(vao, i, state->attribute[i].size, GL_UNSIGNED_BYTE, GL_TRUE,
                                  state->attribute[i].offset);
        break;
      case alias_memory_Format_Unorm16:
        glVertexArrayAttribFormat(vao, i, state->attribute[i].size, GL_UNSIGNED_SHORT, GL_TRUE,
                                  state->attribute[i].offset);
        break;
      case alias_memory_Format_Snorm8:
        glVertexArrayAttribFormat(vao, i, state->attribute[i].size, GL_BYTE, GL_TRUE, state->attribute[i].offset);
        break;
      case alias_memory_Format_Snorm16:
        glVertexArrayAttribFormat(vao, i, state->attribute[i].size, GL_SHORT, GL_TRUE, state->attribute[i].offset);
        break;
      case alias_memory_Format_Uscaled8:
        glVertexArrayAttribFormat(vao, i, state->attribute[i].size, GL_UNSIGNED_BYTE, GL_FALSE,
                                  state->attribute[i].offset);
        break;
      case alias_memory_Format_Uscaled16:
        glVertexArrayAttribFormat(vao, i, state->attribute[i].size, GL_UNSIGNED_SHORT, GL_FALSE,
                                  state->attribute[i].offset);
        break;
      case alias_memory_Format_Sscaled8:
        glVertexArrayAttribFormat(vao, i, state->attribute[i].size, GL_BYTE, GL_FALSE, state->attribute[i].offset);
        break;
      case alias_memory_Format_Sscaled16:
        glVertexArrayAttribFormat(vao, i, state->attribute[i].size, GL_SHORT, GL_FALSE, state->attribute[i].offset);
        break;
      case alias_memory_Format_Urgb8:
        //
        break;
      case alias_memory_Format_Float16:
        glVertexArrayAttribFormat(vao, i, state->attribute[i].size, GL_HALF_FLOAT, GL_FALSE,
                                  state->attribute[i].offset);
        break;
      case alias_memory_Format_Float32:
        glVertexArrayAttribFormat(vao, i, state->attribute[i].size, GL_FLOAT, GL_FALSE, state->attribute[i].offset);
        break;
      case alias_memory_Format_Float64:
        glVertexArrayAttribLFormat(vao, i, state->attribute[i].size, GL_DOUBLE, state->attribute[i].offset);
        break;
      }
      glVertexArrayAttribBinding(vao, i, state->attribute[i].binding);
    }
    *(GLuint *)(&state->vertex_array_object) = vao;
  }
}

void GL_apply_draw_state(const struct DrawState *state) {
  static struct DrawState current_draw_state;

  if(current_draw_state.program_object != state->program_object) {
    glUseProgram(state->program_object);
    current_draw_state.program_object = state->program_object;
  }

  if(current_draw_state.vertex_array_object != state->vertex_array_object) {
    glBindVertexArray(state->vertex_array_object);
    current_draw_state.vertex_array_object = state->vertex_array_object;
  }

  if(current_draw_state.depth_test_enable != state->depth_test_enable) {
    if(state->depth_test_enable) {
      glEnable(GL_DEPTH_TEST);
      current_draw_state.depth_test_enable = true;
    } else {
      glDisable(GL_DEPTH_TEST);
      current_draw_state.depth_test_enable = false;
    }
  }

  if(current_draw_state.depth_mask != state->depth_mask) {
    if(state->depth_mask) {
      glDepthMask(GL_TRUE);
      current_draw_state.depth_mask = true;
    } else {
      glDepthMask(GL_FALSE);
      current_draw_state.depth_mask = false;
    }
  }

  if(current_draw_state.depth_range_min != state->depth_range_min ||
     current_draw_state.depth_range_max != state->depth_range_max) {
    glDepthRange(state->depth_range_min, state->depth_range_max);
    current_draw_state.depth_range_min = state->depth_range_min;
    current_draw_state.depth_range_max = state->depth_range_max;
  }

  if(state->blend_enable) {
    if(!current_draw_state.blend_enable || current_draw_state.blend_src_factor != state->blend_src_factor ||
       current_draw_state.blend_dst_factor != state->blend_dst_factor) {
      glEnable(GL_BLEND);
      glBlendFunc(state->blend_src_factor, state->blend_dst_factor);
      current_draw_state.blend_enable = true;
      current_draw_state.blend_src_factor = state->blend_src_factor;
      current_draw_state.blend_dst_factor = state->blend_dst_factor;
    }
  } else if(current_draw_state.blend_enable) {
    glDisable(GL_BLEND);
    current_draw_state.blend_enable = false;
  }
}

static inline void GL_apply_uniform(GLuint location, enum GL_UniformType type, GLsizei count,
                                    const struct GL_UniformData *data) {
  count = count || 1;
  if(data->pointer) {
    switch(type) {
    case GL_UniformType_Unused:
      break;
    case GL_UniformType_Float:
      glUniform1fv(location, count, data->pointer);
      break;
    case GL_UniformType_Vec2:
      glUniform2fv(location, count, data->pointer);
      break;
    case GL_UniformType_Vec3:
      glUniform3fv(location, count, data->pointer);
      break;
    case GL_UniformType_Vec4:
      glUniform4fv(location, count, data->pointer);
      break;
    case GL_UniformType_Int:
      glUniform1iv(location, count, data->pointer);
      break;
    case GL_UniformType_IVec2:
      glUniform2iv(location, count, data->pointer);
      break;
    case GL_UniformType_IVec3:
      glUniform3iv(location, count, data->pointer);
      break;
    case GL_UniformType_IVec4:
      glUniform4iv(location, count, data->pointer);
      break;
    case GL_UniformType_Uint:
      glUniform1uiv(location, count, data->pointer);
      break;
    case GL_UniformType_UVec2:
      glUniform2uiv(location, count, data->pointer);
      break;
    case GL_UniformType_UVec3:
      glUniform3uiv(location, count, data->pointer);
      break;
    case GL_UniformType_UVec4:
      glUniform4uiv(location, count, data->pointer);
      break;
    case GL_UniformType_Mat2:
      glUniformMatrix2fv(location, count, data->transpose, data->pointer);
      break;
    case GL_UniformType_Mat3:
      glUniformMatrix3fv(location, count, data->transpose, data->pointer);
      break;
    case GL_UniformType_Mat4:
      glUniformMatrix4fv(location, count, data->transpose, data->pointer);
      break;
    case GL_UniformType_Mat2x3:
      glUniformMatrix2x3fv(location, count, data->transpose, data->pointer);
      break;
    case GL_UniformType_Mat3x2:
      glUniformMatrix3x2fv(location, count, data->transpose, data->pointer);
      break;
    case GL_UniformType_Mat2x4:
      glUniformMatrix2x4fv(location, count, data->transpose, data->pointer);
      break;
    case GL_UniformType_Mat4x2:
      glUniformMatrix4x2fv(location, count, data->transpose, data->pointer);
      break;
    case GL_UniformType_Mat3x4:
      glUniformMatrix3x4fv(location, count, data->transpose, data->pointer);
      break;
    case GL_UniformType_Mat4x3:
      glUniformMatrix4x3fv(location, count, data->transpose, data->pointer);
      break;
    }
  } else {
    switch(type) {
    case GL_UniformType_Unused:
      break;
    case GL_UniformType_Float:
      glUniform1f(location, data->_float);
      break;
    case GL_UniformType_Vec2:
      glUniform2f(location, data->vec[0], data->vec[1]);
      break;
    case GL_UniformType_Vec3:
      glUniform3f(location, data->vec[0], data->vec[1], data->vec[2]);
      break;
    case GL_UniformType_Vec4:
      glUniform4f(location, data->vec[0], data->vec[1], data->vec[2], data->vec[3]);
      break;
    case GL_UniformType_Int:
      glUniform1i(location, data->_int);
      break;
    case GL_UniformType_IVec2:
      glUniform2i(location, data->ivec[0], data->ivec[1]);
      break;
    case GL_UniformType_IVec3:
      glUniform3i(location, data->ivec[0], data->ivec[1], data->ivec[2]);
      break;
    case GL_UniformType_IVec4:
      glUniform4i(location, data->ivec[0], data->ivec[1], data->ivec[2], data->ivec[3]);
      break;
    case GL_UniformType_Uint:
      glUniform1ui(location, data->uint);
      break;
    case GL_UniformType_UVec2:
      glUniform2ui(location, data->uvec[0], data->uvec[1]);
      break;
    case GL_UniformType_UVec3:
      glUniform3ui(location, data->uvec[0], data->uvec[1], data->uvec[2]);
      break;
    case GL_UniformType_UVec4:
      glUniform4ui(location, data->uvec[0], data->uvec[1], data->uvec[2], data->uvec[3]);
      break;
    case GL_UniformType_Mat2:
      glUniformMatrix2fv(location, count, data->transpose, data->mat);
      break;
    case GL_UniformType_Mat3:
      glUniformMatrix3fv(location, count, data->transpose, data->mat);
      break;
    case GL_UniformType_Mat4:
      glUniformMatrix4fv(location, count, data->transpose, data->mat);
      break;
    case GL_UniformType_Mat2x3:
      glUniformMatrix2x3fv(location, count, data->transpose, data->mat);
      break;
    case GL_UniformType_Mat3x2:
      glUniformMatrix3x2fv(location, count, data->transpose, data->mat);
      break;
    case GL_UniformType_Mat2x4:
      glUniformMatrix2x4fv(location, count, data->transpose, data->mat);
      break;
    case GL_UniformType_Mat4x2:
      glUniformMatrix4x2fv(location, count, data->transpose, data->mat);
      break;
    case GL_UniformType_Mat3x4:
      glUniformMatrix3x4fv(location, count, data->transpose, data->mat);
      break;
    case GL_UniformType_Mat4x3:
      glUniformMatrix4x3fv(location, count, data->transpose, data->mat);
      break;
    }
  }
}

void GL_apply_draw_assets(const struct DrawState *state, const struct DrawAssets *assets) {
  static struct DrawAssets current_draw_assets;

  if(assets == NULL)
    return;

  for(uint32_t i = 0; i < THIN_GL_MAX_IMAGES; i++) {
    if(assets->image[i]) {
      if(current_draw_assets.image[i] != assets->image[i]) {
        if(image_type_info[state->image[i].type].target == GL_SAMPLER) {
          glBindSampler(i, assets->image[i]);
        } else {
          glActiveTexture(GL_TEXTURE0 + i);
          glBindTexture(image_type_info[state->image[i].type].target, assets->image[i]);
        }
      } else {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, assets->image[i]);
      }
      current_draw_assets.image[i] = assets->image[i];
    }
  }

  for(uint32_t i = 0; i < THIN_GL_MAX_UNIFORMS; i++) {
    if(state->uniform[i].type != 0) {
      GL_apply_uniform(i, state->uniform[i].type, state->uniform[i].count, &assets->uniforms[i]);
    }
    if(state->global[i].stage_bits && state->global[i].uniform != NULL) {
      GL_Uniform_prepare(state->global[i].uniform);
      GL_apply_uniform(state->global[i].location, state->global[i].uniform->type, state->global[i].uniform->count,
                       &state->global[i].uniform->data);
    }
  }

  GLbitfield barriers = 0;

  if(assets->element_buffer != NULL) {
    barriers |= GL_flush_buffer(assets->element_buffer) ? GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT : 0;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, assets->element_buffer->buffer);
  }

  for(int i = 0; i < THIN_GL_MAX_BINDINGS; i++) {
    if(assets->vertex_buffers[i] == NULL)
      break;
    barriers |= GL_flush_buffer(assets->vertex_buffers[i]) ? GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT : 0;
    switch(assets->vertex_buffers[i]->kind) {
    case GL_Buffer_Static:
      glBindVertexBuffer(i, assets->vertex_buffers[i]->buffer, 0, state->binding[i].stride);
      break;
    case GL_Buffer_Temporary:
      glBindVertexBuffer(i, assets->vertex_buffers[i]->buffer, assets->vertex_buffers[i]->temporary.offset,
                         state->binding[i].stride);
      break;
    }
  }

  if(barriers != 0) {
    glMemoryBarrier(barriers);
  }
}

void GL_draw_arrays(const struct DrawState *state, const struct DrawAssets *assets, GLint first, GLsizei count,
                    GLsizei instancecount, GLuint baseinstance) {
  GL_initialize_draw_state(state);
  GL_apply_draw_state(state);
  GL_apply_draw_assets(state, assets);

  glDrawArraysInstancedBaseInstance(state->primitive, first, count, instancecount, baseinstance);

  _.draw_index++;
}

void GL_draw_elements(const struct DrawState *state, const struct DrawAssets *assets, GLsizei count,
                      GLsizei instancecount, GLint basevertex, GLuint baseinstance) {
  GL_initialize_draw_state(state);
  GL_apply_draw_state(state);
  GL_apply_draw_assets(state, assets);

  glDrawElementsInstancedBaseVertexBaseInstance(
      state->primitive, count, GL_UNSIGNED_INT,
      (void *)(assets->element_buffer->kind == GL_Buffer_Temporary ? assets->element_buffer->temporary.offset : 0) +
          sizeof(uint32_t) * assets->element_buffer_offset,
      instancecount, basevertex, baseinstance);

  _.draw_index++;
}

struct GL_Buffer GL_allocate_static_buffer(GLenum type, GLsizei size, const void *data) {
  struct GL_Buffer result;
  result.kind = GL_Buffer_Static;
  glCreateBuffers(1, &result.buffer);
  glNamedBufferStorage(result.buffer, size, data, 0);
  result.size = size;
  return result;
}

struct GL_Buffer GL_allocate_temporary_buffer(GLenum type, GLsizei size) {
again:
  for(uint32_t i = 0; i < _.num_temporary_buffers; i++) {
    if(_.temporary_buffers[i].type == type && _.temporary_buffers[i].size - _.temporary_buffers[i].offset > size) {
      struct GL_Buffer result;
      result.kind = GL_Buffer_Temporary;
      result.buffer = _.temporary_buffers[i].buffer;
      result.size = size;
      result.temporary.mapping =
          (void *)((GLbyte *)_.temporary_buffers[i].mapped_memory + _.temporary_buffers[i].offset);
      result.temporary.offset = _.temporary_buffers[i].offset;
      result.temporary.dirty = true;
      _.temporary_buffers[i].offset += size;
      return result;
    }
  }

  // 16 << 20); // 16mb TODO make a cvar
  GLsizei block_size = 16 << 20;
  GLsizei allocation_size = ((size + block_size) / block_size) * block_size;

  _.temporary_buffers = realloc(_.temporary_buffers, sizeof(*_.temporary_buffers) * (_.num_temporary_buffers + 1));
  glCreateBuffers(1, &_.temporary_buffers[_.num_temporary_buffers].buffer);
  glNamedBufferStorage(_.temporary_buffers[_.num_temporary_buffers].buffer, allocation_size, NULL,
                       GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT);
  _.temporary_buffers[_.num_temporary_buffers].type = type;
  _.temporary_buffers[_.num_temporary_buffers].size = allocation_size;
  _.temporary_buffers[_.num_temporary_buffers].offset = 0;
  _.temporary_buffers[_.num_temporary_buffers].mapped_memory =
      glMapNamedBufferRange(_.temporary_buffers[_.num_temporary_buffers].buffer, 0, allocation_size,
                            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
  _.num_temporary_buffers++;

  goto again;
}

struct GL_Buffer GL_allocate_temporary_buffer_from(GLenum type, GLsizei size, const void *ptr) {
  struct GL_Buffer result = GL_allocate_temporary_buffer(type, size);
  memcpy(result.temporary.mapping, ptr, size);
  return result;
}

bool GL_flush_buffer(const struct GL_Buffer *buffer) {
  switch(buffer->kind) {
  case GL_Buffer_Static:
    break;
  case GL_Buffer_Temporary:
    if(buffer->temporary.dirty) {
      glFlushMappedNamedBufferRange(buffer->buffer, buffer->temporary.offset, buffer->size);
      *(bool *)&buffer->temporary.dirty = false;
      return true;
    }
    return false;
  }
}

void GL_free_buffer(const struct GL_Buffer *buffer) {
  switch(buffer->kind) {
  case GL_Buffer_Static:
    glDeleteBuffers(1, &buffer->buffer);
    break;
  case GL_Buffer_Temporary:
    break;
  }
}

void GL_reset_temporary_buffers(void) {
  for(uint32_t i = 0; i < _.num_temporary_buffers; i++) {
    _.temporary_buffers[i].offset = 0;
  }
}

void GL_temporary_buffer_stats(GLenum type, uint32_t *total_allocated, uint32_t *used) {
  *total_allocated = 0;
  *used = 0;

  for(uint32_t i = 0; i < _.num_temporary_buffers; i++) {
    if(_.temporary_buffers[i].type == type) {
      *total_allocated += _.temporary_buffers[i].size;
      *used += _.temporary_buffers[i].offset;
    }
  }
}

void GL_Uniform_prepare(const struct GL_Uniform *uniform) {
  if(uniform->prepare_draw_index != _.draw_index) {
    *(uint32_t *)&uniform->prepare_draw_index = _.draw_index;
    if(uniform->prepare != NULL) {
      uniform->prepare();
    }
  }
}
