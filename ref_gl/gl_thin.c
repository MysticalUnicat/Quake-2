#include "gl_thin.h"

#include "gl_local.h"

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

static void script_builder_add_vertex_format(const struct GL_VertexFormat *vertex_format) {
  if(vertex_format != NULL) {
    for(int i = 0; i < THIN_GL_MAX_ATTRIBUTES; i++) {
      if(vertex_format->attribute[i].format == 0)
        break;

      script_builder_add("layout(location=%i) in ", i);
      const char *type_name, *vec_name;
      switch(vertex_format->attribute[i].format) {
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
      if(vertex_format->attribute[i].size > 1) {
        script_builder_add("%s%i %s;\n", vec_name, vertex_format->attribute[i].size, vertex_format->attribute[i].name);
      } else {
        script_builder_add("%s %s;\n", type_name, vertex_format->attribute[i].name);
      }
    }
  }
}

static void script_builder_add_uniform_format(const struct GL_UniformFormat *uniform_format, GLbitfield stage_bit) {
  if(uniform_format == NULL)
    return;
  for(uint32_t i = 0; i < THIN_GL_MAX_UNIFORMS; i++) {
    if(uniform_format->uniform[i].type == 0)
      break;
    if(!(uniform_format->uniform[i].stage_bits & stage_bit))
      continue;
    script_builder_add("layout(location=%i) uniform ", i);
    switch(uniform_format->uniform[i].type) {
    case GL_UniformType_Unused:
      break;
    case GL_UniformType_Float:
      script_builder_add("float ");
      break;
    case GL_UniformType_Vec2:
      script_builder_add("vec2 ");
      break;
    case GL_UniformType_Vec3:
      script_builder_add("vec3 ");
      break;
    case GL_UniformType_Vec4:
      script_builder_add("float ");
      break;
    case GL_UniformType_Int:
      script_builder_add("float ");
      break;
    case GL_UniformType_IVec2:
      script_builder_add("ivec2 ");
      break;
    case GL_UniformType_IVec3:
      script_builder_add("ivec3 ");
      break;
    case GL_UniformType_IVec4:
      script_builder_add("ivec4 ");
      break;
    case GL_UniformType_Uint:
      script_builder_add("uint ");
      break;
    case GL_UniformType_UVec2:
      script_builder_add("uvec2 ");
      break;
    case GL_UniformType_UVec3:
      script_builder_add("uvec3 ");
      break;
    case GL_UniformType_UVec4:
      script_builder_add("uvec4 ");
      break;
    case GL_UniformType_Mat2:
      script_builder_add("mat2 ");
      break;
    case GL_UniformType_Mat3:
      script_builder_add("mat3 ");
      break;
    case GL_UniformType_Mat4:
      script_builder_add("mat4 ");
      break;
    case GL_UniformType_Mat2x3:
      script_builder_add("mat2x3 ");
      break;
    case GL_UniformType_Mat3x2:
      script_builder_add("mat3x2 ");
      break;
    case GL_UniformType_Mat2x4:
      script_builder_add("mat2x4 ");
      break;
    case GL_UniformType_Mat4x2:
      script_builder_add("mat4x2 ");
      break;
    case GL_UniformType_Mat3x4:
      script_builder_add("mat3x4 ");
      break;
    case GL_UniformType_Mat4x3:
      script_builder_add("mat4x3 ");
      break;
    }
    script_builder_add("%s;\n", uniform_format->uniform[i].name);
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
    script_builder_add("#version 460 compatibility\n");
    script_builder_add_vertex_format(state->vertex_format);
    script_builder_add_uniform_format(state->uniform_format, THIN_GL_VERTEX_BIT);

    GLuint shader = glCreateShader(GL_VERTEX_SHADER);
    const char *sources[2] = {_.script_builder_ptr, state->vertex_shader_source};
    GLint lengths[2] = {_.script_builder_len, strlen(state->vertex_shader_source)};
    glShaderSource(shader, 2, sources, lengths);
    gl_compileShader(shader);
    *(GLuint *)(&state->vertex_shader_object) = shader;
  }

  if(state->fragment_shader_source != NULL && state->fragment_shader_object == 0) {
    script_builder_init();
    script_builder_add("#version 460 compatibility\n");
    script_builder_add_uniform_format(state->uniform_format, THIN_GL_FRAGMENT_BIT);

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

  if(state->vertex_format != NULL && state->vertex_format->attribute[0].format != 0 &&
     state->vertex_array_object == 0) {
    GLuint vao;
    glGenVertexArrays(1, &vao);
    for(int i = 0; i < THIN_GL_MAX_ATTRIBUTES; i++) {
      if(state->vertex_format->attribute[i].format == 0)
        break;
      glEnableVertexArrayAttrib(vao, i);
      switch(state->vertex_format->attribute[i].format) {
      case alias_memory_Format_Uint8:
        glVertexArrayAttribIFormat(vao, i, state->vertex_format->attribute[i].size, GL_UNSIGNED_BYTE,
                                   state->vertex_format->attribute[i].offset);
        break;
      case alias_memory_Format_Uint16:
        glVertexArrayAttribIFormat(vao, i, state->vertex_format->attribute[i].size, GL_UNSIGNED_SHORT,
                                   state->vertex_format->attribute[i].offset);
        break;
      case alias_memory_Format_Uint32:
        glVertexArrayAttribIFormat(vao, i, state->vertex_format->attribute[i].size, GL_UNSIGNED_INT,
                                   state->vertex_format->attribute[i].offset);
        break;
      case alias_memory_Format_Uint64:
        //
        break;
      case alias_memory_Format_Sint8:
        glVertexArrayAttribIFormat(vao, i, state->vertex_format->attribute[i].size, GL_BYTE,
                                   state->vertex_format->attribute[i].offset);
        break;
      case alias_memory_Format_Sint16:
        glVertexArrayAttribIFormat(vao, i, state->vertex_format->attribute[i].size, GL_SHORT,
                                   state->vertex_format->attribute[i].offset);
        break;
      case alias_memory_Format_Sint32:
        glVertexArrayAttribIFormat(vao, i, state->vertex_format->attribute[i].size, GL_INT,
                                   state->vertex_format->attribute[i].offset);
        break;
      case alias_memory_Format_Sint64:
        //
        break;
      case alias_memory_Format_Unorm8:
        glVertexArrayAttribFormat(vao, i, state->vertex_format->attribute[i].size, GL_UNSIGNED_BYTE, GL_TRUE,
                                  state->vertex_format->attribute[i].offset);
        break;
      case alias_memory_Format_Unorm16:
        glVertexArrayAttribFormat(vao, i, state->vertex_format->attribute[i].size, GL_UNSIGNED_SHORT, GL_TRUE,
                                  state->vertex_format->attribute[i].offset);
        break;
      case alias_memory_Format_Snorm8:
        glVertexArrayAttribFormat(vao, i, state->vertex_format->attribute[i].size, GL_BYTE, GL_TRUE,
                                  state->vertex_format->attribute[i].offset);
        break;
      case alias_memory_Format_Snorm16:
        glVertexArrayAttribFormat(vao, i, state->vertex_format->attribute[i].size, GL_SHORT, GL_TRUE,
                                  state->vertex_format->attribute[i].offset);
        break;
      case alias_memory_Format_Uscaled8:
        glVertexArrayAttribFormat(vao, i, state->vertex_format->attribute[i].size, GL_UNSIGNED_BYTE, GL_FALSE,
                                  state->vertex_format->attribute[i].offset);
        break;
      case alias_memory_Format_Uscaled16:
        glVertexArrayAttribFormat(vao, i, state->vertex_format->attribute[i].size, GL_UNSIGNED_SHORT, GL_FALSE,
                                  state->vertex_format->attribute[i].offset);
        break;
      case alias_memory_Format_Sscaled8:
        glVertexArrayAttribFormat(vao, i, state->vertex_format->attribute[i].size, GL_BYTE, GL_FALSE,
                                  state->vertex_format->attribute[i].offset);
        break;
      case alias_memory_Format_Sscaled16:
        glVertexArrayAttribFormat(vao, i, state->vertex_format->attribute[i].size, GL_SHORT, GL_FALSE,
                                  state->vertex_format->attribute[i].offset);
        break;
      case alias_memory_Format_Urgb8:
        //
        break;
      case alias_memory_Format_Float16:
        glVertexArrayAttribFormat(vao, i, state->vertex_format->attribute[i].size, GL_HALF_FLOAT, GL_FALSE,
                                  state->vertex_format->attribute[i].offset);
        break;
      case alias_memory_Format_Float32:
        glVertexArrayAttribFormat(vao, i, state->vertex_format->attribute[i].size, GL_FLOAT, GL_FALSE,
                                  state->vertex_format->attribute[i].offset);
        break;
      case alias_memory_Format_Float64:
        glVertexArrayAttribLFormat(vao, i, state->vertex_format->attribute[i].size, GL_DOUBLE,
                                   state->vertex_format->attribute[i].offset);
        break;
      }
      glVertexArrayAttribBinding(vao, i, state->vertex_format->attribute[i].binding);
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

void GL_apply_draw_assets(const struct DrawState *state, const struct DrawAssets *assets) {
  static struct DrawAssets current_draw_assets;

  if(assets == NULL)
    return;

  for(uint32_t i = 0; i < 8; i++) {
    if(assets->images[i]) {
      if(current_draw_assets.images[i] != assets->images[i]) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, assets->images[i]);
        current_draw_assets.images[i] = assets->images[i];
      }
    }
  }

  for(uint32_t i = 0; i < 8; i++) {
    if(state->uniform_format == NULL || state->uniform_format->uniform[i].type == 0)
      break;
    if(assets->uniforms[i].pointer) {
      switch(state->uniform_format->uniform[i].type) {
      case GL_UniformType_Unused:
        break;
      case GL_UniformType_Float:
        glUniform1fv(i, state->uniform_format->uniform[i].count, assets->uniforms[i].pointer);
        break;
      case GL_UniformType_Vec2:
        glUniform2fv(i, state->uniform_format->uniform[i].count, assets->uniforms[i].pointer);
        break;
      case GL_UniformType_Vec3:
        glUniform3fv(i, state->uniform_format->uniform[i].count, assets->uniforms[i].pointer);
        break;
      case GL_UniformType_Vec4:
        glUniform4fv(i, state->uniform_format->uniform[i].count, assets->uniforms[i].pointer);
        break;
      case GL_UniformType_Int:
        glUniform1iv(i, state->uniform_format->uniform[i].count, assets->uniforms[i].pointer);
        break;
      case GL_UniformType_IVec2:
        glUniform2iv(i, state->uniform_format->uniform[i].count, assets->uniforms[i].pointer);
        break;
      case GL_UniformType_IVec3:
        glUniform3iv(i, state->uniform_format->uniform[i].count, assets->uniforms[i].pointer);
        break;
      case GL_UniformType_IVec4:
        glUniform4iv(i, state->uniform_format->uniform[i].count, assets->uniforms[i].pointer);
        break;
      case GL_UniformType_Uint:
        glUniform1uiv(i, state->uniform_format->uniform[i].count, assets->uniforms[i].pointer);
        break;
      case GL_UniformType_UVec2:
        glUniform2uiv(i, state->uniform_format->uniform[i].count, assets->uniforms[i].pointer);
        break;
      case GL_UniformType_UVec3:
        glUniform3uiv(i, state->uniform_format->uniform[i].count, assets->uniforms[i].pointer);
        break;
      case GL_UniformType_UVec4:
        glUniform4uiv(i, state->uniform_format->uniform[i].count, assets->uniforms[i].pointer);
        break;
      case GL_UniformType_Mat2:
        glUniformMatrix2fv(i, state->uniform_format->uniform[i].count, assets->uniforms[i].transpose,
                           assets->uniforms[i].pointer);
        break;
      case GL_UniformType_Mat3:
        glUniformMatrix3fv(i, state->uniform_format->uniform[i].count, assets->uniforms[i].transpose,
                           assets->uniforms[i].pointer);
        break;
      case GL_UniformType_Mat4:
        glUniformMatrix4fv(i, state->uniform_format->uniform[i].count, assets->uniforms[i].transpose,
                           assets->uniforms[i].pointer);
        break;
      case GL_UniformType_Mat2x3:
        glUniformMatrix2x3fv(i, state->uniform_format->uniform[i].count, assets->uniforms[i].transpose,
                             assets->uniforms[i].pointer);
        break;
      case GL_UniformType_Mat3x2:
        glUniformMatrix3x2fv(i, state->uniform_format->uniform[i].count, assets->uniforms[i].transpose,
                             assets->uniforms[i].pointer);
        break;
      case GL_UniformType_Mat2x4:
        glUniformMatrix2x4fv(i, state->uniform_format->uniform[i].count, assets->uniforms[i].transpose,
                             assets->uniforms[i].pointer);
        break;
      case GL_UniformType_Mat4x2:
        glUniformMatrix4x2fv(i, state->uniform_format->uniform[i].count, assets->uniforms[i].transpose,
                             assets->uniforms[i].pointer);
        break;
      case GL_UniformType_Mat3x4:
        glUniformMatrix3x4fv(i, state->uniform_format->uniform[i].count, assets->uniforms[i].transpose,
                             assets->uniforms[i].pointer);
        break;
      case GL_UniformType_Mat4x3:
        glUniformMatrix4x3fv(i, state->uniform_format->uniform[i].count, assets->uniforms[i].transpose,
                             assets->uniforms[i].pointer);
        break;
      }
    } else {
      switch(state->uniform_format->uniform[i].type) {
      case GL_UniformType_Unused:
        break;
      case GL_UniformType_Float:
        glUniform1f(i, assets->uniforms[i]._float);
        break;
      case GL_UniformType_Vec2:
        glUniform2f(i, assets->uniforms[i].vec2[0], assets->uniforms[i].vec2[1]);
        break;
      case GL_UniformType_Vec3:
        glUniform3f(i, assets->uniforms[i].vec3[0], assets->uniforms[i].vec3[1], assets->uniforms[i].vec3[2]);
        break;
      case GL_UniformType_Vec4:
        glUniform4f(i, assets->uniforms[i].vec4[0], assets->uniforms[i].vec4[1], assets->uniforms[i].vec4[2],
                    assets->uniforms[i].vec4[3]);
        break;
      case GL_UniformType_Int:
        glUniform1i(i, assets->uniforms[i]._int);
        break;
      case GL_UniformType_IVec2:
        glUniform2i(i, assets->uniforms[i].ivec2[0], assets->uniforms[i].ivec2[1]);
        break;
      case GL_UniformType_IVec3:
        glUniform3i(i, assets->uniforms[i].ivec3[0], assets->uniforms[i].ivec3[1], assets->uniforms[i].ivec3[2]);
        break;
      case GL_UniformType_IVec4:
        glUniform4i(i, assets->uniforms[i].ivec4[0], assets->uniforms[i].ivec4[1], assets->uniforms[i].ivec4[2],
                    assets->uniforms[i].ivec4[3]);
        break;
      case GL_UniformType_Uint:
        glUniform1ui(i, assets->uniforms[i].uint);
        break;
      case GL_UniformType_UVec2:
        glUniform2ui(i, assets->uniforms[i].uvec2[0], assets->uniforms[i].uvec2[1]);
        break;
      case GL_UniformType_UVec3:
        glUniform3ui(i, assets->uniforms[i].uvec3[0], assets->uniforms[i].uvec3[1], assets->uniforms[i].uvec3[2]);
        break;
      case GL_UniformType_UVec4:
        glUniform4ui(i, assets->uniforms[i].uvec4[0], assets->uniforms[i].uvec4[1], assets->uniforms[i].uvec4[2],
                     assets->uniforms[i].uvec4[3]);
        break;
      case GL_UniformType_Mat2:
      case GL_UniformType_Mat3:
      case GL_UniformType_Mat4:
      case GL_UniformType_Mat2x3:
      case GL_UniformType_Mat3x2:
      case GL_UniformType_Mat2x4:
      case GL_UniformType_Mat4x2:
      case GL_UniformType_Mat3x4:
      case GL_UniformType_Mat4x3:
        break;
      }
    }
  }

  GLbitfield barriers = 0;

  if(assets->element_buffer != NULL) {
    // barriers |= GL_flush_buffer(assets->element_buffer) ? GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT : 0;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, assets->element_buffer->buffer);
  }

  for(int i = 0; i < THIN_GL_MAX_BINDINGS; i++) {
    if(assets->vertex_buffers[i] == NULL)
      break;
    // barriers |= GL_flush_buffer(assets->vertex_buffers[i]) ? GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT : 0;
    switch(assets->vertex_buffers[i]->kind) {
    case GL_Buffer_Static:
      glBindVertexBuffer(i, assets->vertex_buffers[i]->buffer, 0, state->vertex_format->binding[i].stride);
      break;
    case GL_Buffer_Temporary:
      glBindVertexBuffer(i, assets->vertex_buffers[i]->buffer, assets->vertex_buffers[i]->temporary.offset,
                         state->vertex_format->binding[i].stride);
      break;
    }
  }

  if(barriers != 0) {
    glMemoryBarrier(barriers);
  }
}

void GL_begin_draw(const struct DrawState *state, const struct DrawAssets *assets) {
  GL_initialize_draw_state(state);
  GL_apply_draw_state(state);
  GL_apply_draw_assets(state, assets);

  glad_glBegin(state->primitive);
}

void GL_end_draw(void) { glad_glEnd(); }

void GL_draw_arrays(const struct DrawState *state, const struct DrawAssets *assets, GLint first, GLsizei count,
                    GLsizei instancecount, GLuint baseinstance) {
  GL_initialize_draw_state(state);
  GL_apply_draw_state(state);
  GL_apply_draw_assets(state, assets);

  glDrawArraysInstancedBaseInstance(state->primitive, first, count, instancecount, baseinstance);
}

void GL_draw_elements(const struct DrawState *state, const struct DrawAssets *assets, GLsizei count,
                      GLsizei instancecount, GLint basevertex, GLuint baseinstance) {
  GL_initialize_draw_state(state);
  GL_apply_draw_state(state);
  GL_apply_draw_assets(state, assets);

  glDrawElementsInstancedBaseVertexBaseInstance(
      state->primitive, count, GL_UNSIGNED_INT,
      (void *)(assets->element_buffer->kind == GL_Buffer_Temporary ? assets->element_buffer->temporary.offset : 0),
      instancecount, basevertex, baseinstance);
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
                       GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT);
  _.temporary_buffers[_.num_temporary_buffers].type = type;
  _.temporary_buffers[_.num_temporary_buffers].size = allocation_size;
  _.temporary_buffers[_.num_temporary_buffers].offset = 0;
  _.temporary_buffers[_.num_temporary_buffers].mapped_memory =
      glMapNamedBufferRange(_.temporary_buffers[_.num_temporary_buffers].buffer, 0, allocation_size,
                            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
  _.num_temporary_buffers++;

  goto again;
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
