#include "gl_thin_cpp.h"

#include "gl_local.h"

THIN_GL_IMPL_STRUCT(DrawArraysIndirectCommand, uint32(count), uint32(instance_count), uint32(first),
                    uint32(base_instance))

THIN_GL_IMPL_STRUCT(DrawElementsIndirectCommand, uint32(count), uint32(instance_count), uint32(first_index),
                    uint32(base_vertex), uint32(base_instance))

THIN_GL_IMPL_STRUCT(DispatchIndirectCommand, uint32(num_groups_x), uint32(num_groups_y), uint32(num_groups_z))

const char shader_prelude[] = "#version 460 core\n";

struct GL_TypeInfo {
  const char *name;
  GLenum target;
};

static struct GL_TypeInfo type_info[] = {
    [GL_Type_Float] = {"float"},
    [GL_Type_Float2] = {"vec2"},
    [GL_Type_Float3] = {"vec3"},
    [GL_Type_Float4] = {"vec4"},
    [GL_Type_Int] = {"int"},
    [GL_Type_Int2] = {"ivec2"},
    [GL_Type_Int3] = {"ivec3"},
    [GL_Type_Int4] = {"ivec4"},
    [GL_Type_Uint] = {"uint"},
    [GL_Type_Uint2] = {"uvec2"},
    [GL_Type_Uint3] = {"uvec3"},
    [GL_Type_Uint4] = {"uvec4"},
    [GL_Type_Float2x2] = {"mat2"},
    [GL_Type_Float3x3] = {"mat3"},
    [GL_Type_Float4x4] = {"mat4"},
    [GL_Type_Float2x3] = {"mat2x3"},
    [GL_Type_Float3x2] = {"mat3x2"},
    [GL_Type_Float2x4] = {"mat2x4"},
    [GL_Type_Float4x2] = {"mat4x2"},
    [GL_Type_Float3x4] = {"mat3x4"},
    [GL_Type_Float4x3] = {"mat4x3"},
    [GL_Type_Sampler1D] =
        {
            .name = "sampler1D",
            .target = GL_TEXTURE_1D,
        },
    [GL_Type_Image1D] =
        {
            .name = "image1D",
            .target = GL_TEXTURE_1D,
        },
    [GL_Type_Sampler1DShadow] =
        {
            .name = "sampler1DShadow",
            .target = GL_TEXTURE_1D_ARRAY,
        },
    [GL_Type_Sampler1DArray] =
        {
            .name = "sampler1DArray",
            .target = GL_TEXTURE_1D_ARRAY,
        },
    [GL_Type_Sampler1DArrayShadow] =
        {
            .name = "sampler1DArrayShadow",
            .target = GL_TEXTURE_1D_ARRAY,
        },
    [GL_Type_Sampler2D] =
        {
            .name = "sampler2D",
            .target = GL_TEXTURE_2D,
        },
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
  uint32_t emit_index;
} _ = {0, 0, 0, 0};

static void script_builder_init(void) {
  _.script_builder_len = 0;
  _.emit_index++;
}

static void script_builder_add(const char *format, ...) {
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

static void script_builder_add_shader(const struct GL_ShaderSnippet *shader);

static void script_builder_add_shader_requisites(const struct GL_ShaderSnippet *shader) {
  for(uint32_t i = 0; i < 8; i++) {
    if(shader->requires[i] == NULL)
      break;
    script_builder_add_shader(shader->requires[i]);
  }
}

static void script_builder_add_shader(const struct GL_ShaderSnippet *shader) {
  if(shader->emit_index == _.emit_index)
    return;
  script_builder_add_shader_requisites(shader);
  script_builder_add("%s\n", shader->code);
  *(uint32_t *)(&shader->emit_index) = _.emit_index;
}

static void script_builder_add_vertex_format(const struct GL_DrawState *draw_state) {
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

static void script_builder_add_uniform_format(const struct GL_PipelineState *pipeline_state, GLbitfield stage_bit) {
  GLuint uniform_location = 0;
  GLuint shader_storage_buffer_binding = 0;

  GLuint location, binding;

  for(uint32_t i = 0; i < THIN_GL_MAX_UNIFORMS; i++) {
    if((stage_bit && !pipeline_state->uniform[i].stage_bits) || pipeline_state->uniform[i].type == GL_Type_Unused)
      continue;
    GLuint location = uniform_location++;
    if((pipeline_state->uniform[i].stage_bits & stage_bit) != stage_bit)
      continue;
    script_builder_add("layout(location=%i) uniform %s u_%s;\n", location,
                       type_info[pipeline_state->uniform[i].type].name, pipeline_state->uniform[i].name);
  }

  for(uint32_t i = 0; i < THIN_GL_MAX_UNIFORMS; i++) {
    if((stage_bit && !pipeline_state->global[i].stage_bits) || pipeline_state->global[i].resource == NULL)
      continue;
    if(pipeline_state->global[i].resource->type == GL_Type_ShaderStorageBuffer) {
      binding = shader_storage_buffer_binding++;
    } else {
      location = uniform_location++;
    }
    if((pipeline_state->global[i].stage_bits & stage_bit) != stage_bit)
      continue;
    if(pipeline_state->global[i].resource->type == GL_Type_ShaderStorageBuffer) {
      script_builder_add_shader_requisites(pipeline_state->global[i].resource->block.snippet);

      script_builder_add("layout(std430, binding=%i) %s u_%s;\n", binding,
                         pipeline_state->global[i].resource->block.snippet->code,
                         pipeline_state->global[i].resource->name);
    } else {
      script_builder_add("layout(location=%i) uniform %s u_%s;\n", location,
                         type_info[pipeline_state->global[i].resource->type].name,
                         pipeline_state->global[i].resource->name);
    }
  }
}

static void script_builder_add_images_format(const struct GL_PipelineState *pipeline_state, GLbitfield stage_bit) {
  for(uint32_t i = 0; i < THIN_GL_MAX_IMAGES; i++) {
    if(!(pipeline_state->image[i].stage_bits & stage_bit))
      continue;
    script_builder_add("layout(binding=%i) uniform %s u_%s;\n", i, type_info[pipeline_state->image[i].type].name,
                       pipeline_state->image[i].name);
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

void GL_initialize_draw_state(const struct GL_DrawState *state) {
  if(state->vertex_shader != NULL && state->vertex_shader_object == 0) {
    script_builder_init();
    script_builder_add(shader_prelude);
    script_builder_add_vertex_format(state);
    script_builder_add_uniform_format((const struct GL_PipelineState *)state, THIN_GL_VERTEX_BIT);
    script_builder_add_images_format((const struct GL_PipelineState *)state, THIN_GL_VERTEX_BIT);

    script_builder_add_shader_requisites((const struct GL_ShaderSnippet *)state->vertex_shader);
    script_builder_add("%s", state->vertex_shader->code);

    GLuint shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(shader, 1, (const char *const *)&_.script_builder_ptr, &_.script_builder_len);
    gl_compileShader(shader);
    *(GLuint *)(&state->vertex_shader_object) = shader;
  }

  if(state->fragment_shader != NULL && state->fragment_shader_object == 0) {
    script_builder_init();
    script_builder_add(shader_prelude);
    script_builder_add_uniform_format((const struct GL_PipelineState *)state, THIN_GL_FRAGMENT_BIT);
    script_builder_add_images_format((const struct GL_PipelineState *)state, THIN_GL_FRAGMENT_BIT);

    script_builder_add_shader_requisites((const struct GL_ShaderSnippet *)state->fragment_shader);
    script_builder_add("%s", state->fragment_shader->code);

    GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shader, 1, (const char *const *)&_.script_builder_ptr, &_.script_builder_len);
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

static GLbitfield apply_pipeline_state(const struct GL_PipelineState *state) {
  static struct GL_PipelineState current_pipeline_state;

  if(current_pipeline_state.program_object != state->program_object) {
    glUseProgram(state->program_object);
    current_pipeline_state.program_object = state->program_object;
  }

  return 0;
}

GLbitfield GL_apply_draw_state(const struct GL_DrawState *state) {
  static struct GL_DrawState current_draw_state;

  GLbitfield barriers = apply_pipeline_state((const struct GL_PipelineState *)state);

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

  return barriers;
}

static inline void GL_apply_uniform(GLuint location, enum GL_Type type, GLsizei count,
                                    const struct GL_UniformData *data) {
  count = count || 1;
  if(data->pointer) {
    switch(type) {
    case GL_Type_Unused:
      break;
    case GL_Type_Float:
      glUniform1fv(location, count, data->pointer);
      break;
    case GL_Type_Float2:
      glUniform2fv(location, count, data->pointer);
      break;
    case GL_Type_Float3:
      glUniform3fv(location, count, data->pointer);
      break;
    case GL_Type_Float4:
      glUniform4fv(location, count, data->pointer);
      break;
    case GL_Type_Int:
      glUniform1iv(location, count, data->pointer);
      break;
    case GL_Type_Int2:
      glUniform2iv(location, count, data->pointer);
      break;
    case GL_Type_Int3:
      glUniform3iv(location, count, data->pointer);
      break;
    case GL_Type_Int4:
      glUniform4iv(location, count, data->pointer);
      break;
    case GL_Type_Uint:
      glUniform1uiv(location, count, data->pointer);
      break;
    case GL_Type_Uint2:
      glUniform2uiv(location, count, data->pointer);
      break;
    case GL_Type_Uint3:
      glUniform3uiv(location, count, data->pointer);
      break;
    case GL_Type_Uint4:
      glUniform4uiv(location, count, data->pointer);
      break;
    case GL_Type_Float2x2:
      glUniformMatrix2fv(location, count, data->transpose, data->pointer);
      break;
    case GL_Type_Float3x3:
      glUniformMatrix3fv(location, count, data->transpose, data->pointer);
      break;
    case GL_Type_Float4x4:
      glUniformMatrix4fv(location, count, data->transpose, data->pointer);
      break;
    case GL_Type_Float2x3:
      glUniformMatrix2x3fv(location, count, data->transpose, data->pointer);
      break;
    case GL_Type_Float3x2:
      glUniformMatrix3x2fv(location, count, data->transpose, data->pointer);
      break;
    case GL_Type_Float2x4:
      glUniformMatrix2x4fv(location, count, data->transpose, data->pointer);
      break;
    case GL_Type_Float4x2:
      glUniformMatrix4x2fv(location, count, data->transpose, data->pointer);
      break;
    case GL_Type_Float3x4:
      glUniformMatrix3x4fv(location, count, data->transpose, data->pointer);
      break;
    case GL_Type_Float4x3:
      glUniformMatrix4x3fv(location, count, data->transpose, data->pointer);
      break;
    }
  } else {
    switch(type) {
    case GL_Type_Unused:
      break;
    case GL_Type_Float:
      glUniform1f(location, data->_float);
      break;
    case GL_Type_Float2:
      glUniform2f(location, data->vec[0], data->vec[1]);
      break;
    case GL_Type_Float3:
      glUniform3f(location, data->vec[0], data->vec[1], data->vec[2]);
      break;
    case GL_Type_Float4:
      glUniform4f(location, data->vec[0], data->vec[1], data->vec[2], data->vec[3]);
      break;
    case GL_Type_Int:
      glUniform1i(location, data->_int);
      break;
    case GL_Type_Int2:
      glUniform2i(location, data->ivec[0], data->ivec[1]);
      break;
    case GL_Type_Int3:
      glUniform3i(location, data->ivec[0], data->ivec[1], data->ivec[2]);
      break;
    case GL_Type_Int4:
      glUniform4i(location, data->ivec[0], data->ivec[1], data->ivec[2], data->ivec[3]);
      break;
    case GL_Type_Uint:
      glUniform1ui(location, data->uint);
      break;
    case GL_Type_Uint2:
      glUniform2ui(location, data->uvec[0], data->uvec[1]);
      break;
    case GL_Type_Uint3:
      glUniform3ui(location, data->uvec[0], data->uvec[1], data->uvec[2]);
      break;
    case GL_Type_Uint4:
      glUniform4ui(location, data->uvec[0], data->uvec[1], data->uvec[2], data->uvec[3]);
      break;
    case GL_Type_Float2x2:
      glUniformMatrix2fv(location, count, data->transpose, data->mat);
      break;
    case GL_Type_Float3x3:
      glUniformMatrix3fv(location, count, data->transpose, data->mat);
      break;
    case GL_Type_Float4x4:
      glUniformMatrix4fv(location, count, data->transpose, data->mat);
      break;
    case GL_Type_Float2x3:
      glUniformMatrix2x3fv(location, count, data->transpose, data->mat);
      break;
    case GL_Type_Float3x2:
      glUniformMatrix3x2fv(location, count, data->transpose, data->mat);
      break;
    case GL_Type_Float2x4:
      glUniformMatrix2x4fv(location, count, data->transpose, data->mat);
      break;
    case GL_Type_Float4x2:
      glUniformMatrix4x2fv(location, count, data->transpose, data->mat);
      break;
    case GL_Type_Float3x4:
      glUniformMatrix3x4fv(location, count, data->transpose, data->mat);
      break;
    case GL_Type_Float4x3:
      glUniformMatrix4x3fv(location, count, data->transpose, data->mat);
      break;
    }
  }
}

static inline GLbitfield apply_pipeline_assets(const struct GL_PipelineState *state,
                                               const struct GL_PipelineAssets *assets, bool compute) {
  static struct GL_PipelineAssets current_assets;

  GLuint uniform_location = 0;
  GLuint shader_storage_buffer_binding = 0;
  GLuint location, binding;

  GLbitfield barriers = 0;

  for(uint32_t i = 0; i < THIN_GL_MAX_IMAGES; i++) {
    if(assets != NULL && assets->image[i]) {
      if(current_assets.image[i] != assets->image[i]) {
        if(type_info[state->image[i].type].target == GL_SAMPLER) {
          glBindSampler(i, assets->image[i]);
        } else {
          glActiveTexture(GL_TEXTURE0 + i);
          glBindTexture(type_info[state->image[i].type].target, assets->image[i]);
        }
      } else {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, assets->image[i]);
      }
      current_assets.image[i] = assets->image[i];
    }
  }

  for(uint32_t i = 0; i < THIN_GL_MAX_UNIFORMS; i++) {
    if((!compute && !state->uniform[i].stage_bits) || state->uniform[i].type == GL_Type_Unused)
      continue;
    GLuint location = uniform_location++;
    if(assets != NULL)
      GL_apply_uniform(location, state->uniform[i].type, state->uniform[i].count, &assets->uniforms[i]);
  }

  for(uint32_t i = 0; i < THIN_GL_MAX_UNIFORMS; i++) {
    if((!compute && !state->global[i].stage_bits) || state->global[i].resource == NULL)
      continue;
    if(state->global[i].resource->type == GL_Type_ShaderStorageBuffer) {
      binding = shader_storage_buffer_binding++;
    } else {
      location = uniform_location++;
    }
    if(state->global[i].resource->type == GL_Type_ShaderStorageBuffer) {
      barriers |= GL_flush_buffer(state->global[i].resource->block.buffer, GL_SHADER_STORAGE_BARRIER_BIT);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, state->global[i].resource->block.buffer->buffer);
    } else {
      GL_ShaderResource_prepare(state->global[i].resource);
      GL_apply_uniform(location, state->global[i].resource->type, state->global[i].resource->count,
                       &state->global[i].resource->uniform.data);
    }
  }

  return barriers;
}

GLbitfield GL_apply_draw_assets(const struct GL_DrawState *state, const struct GL_DrawAssets *assets) {
  static struct GL_DrawAssets current_draw_assets;

  GLbitfield barriers =
      apply_pipeline_assets((const struct GL_PipelineState *)state, (const struct GL_PipelineAssets *)assets, false);

  if(assets != NULL) {
    if(assets->element_buffer != NULL) {
      barriers |= GL_flush_buffer(assets->element_buffer, GL_ELEMENT_ARRAY_BARRIER_BIT);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, assets->element_buffer->buffer);
    }

    for(int i = 0; i < THIN_GL_MAX_BINDINGS; i++) {
      if(assets->vertex_buffers[i] == NULL)
        break;
      barriers |= GL_flush_buffer(assets->vertex_buffers[i], GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
      if(assets->vertex_buffers[i]->kind == GL_Buffer_Temporary) {
        glBindVertexBuffer(i, assets->vertex_buffers[i]->buffer, assets->vertex_buffers[i]->offset,
                           state->binding[i].stride);
      } else {
        glBindVertexBuffer(i, assets->vertex_buffers[i]->buffer, 0, state->binding[i].stride);
      }
    }
  }

  return barriers;
}

static void apply_barriers(GLbitfield barriers) {
  if(barriers)
    glMemoryBarrier(barriers);
}

void GL_draw_arrays(const struct GL_DrawState *state, const struct GL_DrawAssets *assets, GLint first, GLsizei count,
                    GLsizei instancecount, GLuint baseinstance) {
  GL_initialize_draw_state(state);
  GLbitfield barriers = GL_apply_draw_state(state);
  barriers |= GL_apply_draw_assets(state, assets);

  apply_barriers(barriers);

  glDrawArraysInstancedBaseInstance(state->primitive, first, count, instancecount, baseinstance);

  _.draw_index++;
}

void GL_draw_elements(const struct GL_DrawState *state, const struct GL_DrawAssets *assets, GLsizei count,
                      GLsizei instancecount, GLint basevertex, GLuint baseinstance) {
  GL_initialize_draw_state(state);
  GLbitfield barriers = GL_apply_draw_state(state);
  barriers |= GL_apply_draw_assets(state, assets);

  apply_barriers(barriers);

  glDrawElementsInstancedBaseVertexBaseInstance(
      state->primitive, count, GL_UNSIGNED_INT,
      (void *)(assets->element_buffer->kind == GL_Buffer_Temporary ? assets->element_buffer->offset : 0) +
          sizeof(uint32_t) * assets->element_buffer_offset,
      instancecount, basevertex, baseinstance);

  _.draw_index++;
}

void GL_draw_elements_indirect(const struct GL_DrawState *state, const struct GL_DrawAssets *assets,
                               const struct GL_Buffer *indirect, GLsizei indirect_offset) {
  GL_initialize_draw_state(state);
  GLbitfield barriers = GL_apply_draw_state(state);
  barriers |= GL_apply_draw_assets(state, assets);

  barriers |= GL_flush_buffer(indirect, GL_COMMAND_BARRIER_BIT);

  apply_barriers(barriers);

  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect->buffer);
  glDrawElementsIndirect(state->primitive, GL_UNSIGNED_INT, (const void *)(uintptr_t)indirect_offset);

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
      result.mapping = (void *)((GLbyte *)_.temporary_buffers[i].mapped_memory + _.temporary_buffers[i].offset);
      result.offset = _.temporary_buffers[i].offset;
      result.dirty = true;
      _.temporary_buffers[i].offset += size;
      return result;
    }
  }

  // 16 << 20); // 16mb TODO make a cvar
  GLsizei block_size = 4 << 20;
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
  memcpy(result.mapping, ptr, size);
  return result;
}

static void activate_buffer(const struct GL_Buffer *buffer) {
  if(buffer->buffer != 0)
    return;
  switch(buffer->kind) {
  case GL_Buffer_Static:
    break;
  case GL_Buffer_Temporary:
    break;
  case GL_Buffer_CPU:
    glCreateBuffers(1, (GLuint *)&buffer->buffer);
    glNamedBufferStorage(buffer->buffer, buffer->size, NULL, GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT);
    *(void **)&buffer->mapping = glMapNamedBufferRange(
        buffer->buffer, 0, buffer->size, GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
    break;
  case GL_Buffer_GPU:
    glCreateBuffers(1, (GLuint *)&buffer->buffer);
    glNamedBufferStorage(buffer->buffer, buffer->size, NULL, 0);
    break;
  }
}

void *GL_update_buffer_begin(const struct GL_Buffer *buffer, GLintptr offset, GLsizei size) {
  (void)offset;
  (void)size;
  if(buffer->kind == GL_Buffer_CPU) {
    activate_buffer(buffer);
    return buffer->mapping;
  }
  return NULL;
}

void GL_update_buffer_end(const struct GL_Buffer *buffer, GLintptr offset, GLsizei size) {
  if(buffer->kind == GL_Buffer_CPU) {
    glFlushMappedNamedBufferRange(buffer->buffer, offset, size);
    *(bool *)&buffer->dirty = true;
  }
}

GLbitfield GL_flush_buffer(const struct GL_Buffer *buffer, GLbitfield read_barrier_bits) {
  switch(buffer->kind) {
  case GL_Buffer_Static:
    break;
  case GL_Buffer_Temporary:
    if(buffer->dirty) {
      glFlushMappedNamedBufferRange(buffer->buffer, buffer->offset, buffer->size);
      *(bool *)&buffer->dirty = false;
      return GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT;
    }
    break;
  case GL_Buffer_CPU:
    activate_buffer(buffer);
    if(buffer->dirty) {
      *(bool *)&buffer->dirty = false;
      return GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT;
    }
    break;
  case GL_Buffer_GPU:
    activate_buffer(buffer);
    if(buffer->dirty) {
      *(bool *)&buffer->dirty = false;
      return read_barrier_bits;
    }
    break;
  }

  return 0;
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

void GL_ShaderResource_prepare(const struct GL_ShaderResource *resource) {
  if(resource->uniform.prepare_draw_index != _.draw_index) {
    *(uint32_t *)&resource->uniform.prepare_draw_index = _.draw_index;
    if(resource->uniform.prepare != NULL) {
      resource->uniform.prepare();
    }
  }
}

void GL_initialize_compute_state(const struct GL_ComputeState *state) {
  if(state->shader != NULL && state->shader_object == 0) {
    script_builder_init();
    script_builder_add(shader_prelude);
    script_builder_add("layout(local_size_x=%i, local_size_y=%i, local_size_z=%i) in;\n", state->local_group_x || 1,
                       state->local_group_y || 1, state->local_group_z || 1);
    script_builder_add_uniform_format((const struct GL_PipelineState *)state, 0);
    script_builder_add_images_format((const struct GL_PipelineState *)state, 0);
    script_builder_add_shader_requisites((const struct GL_ShaderSnippet *)state->shader);
    script_builder_add("%s", state->shader->code);

    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, (const char *const *)&_.script_builder_ptr, &_.script_builder_len);
    gl_compileShader(shader);
    *(GLuint *)(&state->shader_object) = shader;
  }

  if(state->shader_object != 0 && state->program_object == 0) {
    GLuint program = glCreateProgram();
    glAttachShader(program, state->shader_object);
    gl_linkProgram(program);
    *(GLuint *)(&state->program_object) = program;
  }
}

GLbitfield GL_apply_compute_state(const struct GL_ComputeState *state) {
  return apply_pipeline_state((const struct GL_PipelineState *)state);
}

GLbitfield GL_apply_compute_assets(const struct GL_ComputeState *state, const struct GL_ComputeAssets *assets) {
  static struct GL_ComputeAssets current_compute_assets;

  GLbitfield barriers =
      apply_pipeline_assets((const struct GL_PipelineState *)state, (const struct GL_PipelineAssets *)assets, true);

  if(assets != NULL) {
  }

  return barriers;
}

void GL_compute(const struct GL_ComputeState *state, const struct GL_ComputeAssets *assets, GLuint num_groups_x,
                GLuint num_groups_y, GLuint num_groups_z) {
  GL_initialize_compute_state(state);
  GLbitfield barriers = GL_apply_compute_state(state);
  barriers |= GL_apply_compute_assets(state, assets);

  apply_barriers(barriers);

  glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
}

void GL_compute_indirect(const struct GL_ComputeState *state, const struct GL_ComputeAssets *assets,
                         const struct GL_Buffer *indirect, GLsizei indirect_offset) {
  GL_initialize_compute_state(state);
  GLbitfield barriers = GL_apply_compute_state(state);
  barriers |= GL_apply_compute_assets(state, assets);

  barriers |= GL_flush_buffer(indirect, GL_COMMAND_BARRIER_BIT);

  apply_barriers(barriers);

  glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, indirect->buffer);
  glDispatchComputeIndirect(indirect_offset);
}
