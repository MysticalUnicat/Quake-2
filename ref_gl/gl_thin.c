#include "gl_thin.h"

#include "gl_local.h"

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
    GLuint shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(shader, 1, &state->vertex_shader_source, NULL);
    gl_compileShader(shader);
    *(GLuint *)(&state->vertex_shader_object) = shader;
  }

  if(state->fragment_shader_source != NULL && state->fragment_shader_object == 0) {
    GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shader, 1, &state->fragment_shader_source, NULL);
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
}

void GL_apply_draw_state(const struct DrawState *state) {
  static struct DrawState current_draw_state;

  if(current_draw_state.program_object != state->program_object) {
    glUseProgram(state->program_object);
    current_draw_state.program_object = state->program_object;
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

void GL_apply_draw_assets(const struct DrawAssets *assets) {
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
    if(assets->uniforms[i].pointer) {
      switch(assets->uniforms[i].type) {
      case DrawUniformType_Unused:
        break;
      case DrawUniformType_Float:
        glUniform1fv(i, assets->uniforms[i].count, assets->uniforms[i].pointer);
        break;
      case DrawUniformType_Vec2:
        glUniform2fv(i, assets->uniforms[i].count, assets->uniforms[i].pointer);
        break;
      case DrawUniformType_Vec3:
        glUniform3fv(i, assets->uniforms[i].count, assets->uniforms[i].pointer);
        break;
      case DrawUniformType_Vec4:
        glUniform4fv(i, assets->uniforms[i].count, assets->uniforms[i].pointer);
        break;
      case DrawUniformType_Int:
        glUniform1iv(i, assets->uniforms[i].count, assets->uniforms[i].pointer);
        break;
      case DrawUniformType_IVec2:
        glUniform2iv(i, assets->uniforms[i].count, assets->uniforms[i].pointer);
        break;
      case DrawUniformType_IVec3:
        glUniform3iv(i, assets->uniforms[i].count, assets->uniforms[i].pointer);
        break;
      case DrawUniformType_IVec4:
        glUniform4iv(i, assets->uniforms[i].count, assets->uniforms[i].pointer);
        break;
      case DrawUniformType_Uint:
        glUniform1uiv(i, assets->uniforms[i].count, assets->uniforms[i].pointer);
        break;
      case DrawUniformType_UVec2:
        glUniform2uiv(i, assets->uniforms[i].count, assets->uniforms[i].pointer);
        break;
      case DrawUniformType_UVec3:
        glUniform3uiv(i, assets->uniforms[i].count, assets->uniforms[i].pointer);
        break;
      case DrawUniformType_UVec4:
        glUniform4uiv(i, assets->uniforms[i].count, assets->uniforms[i].pointer);
        break;
      case DrawUniformType_Mat2:
        glUniformMatrix2fv(i, assets->uniforms[i].count, assets->uniforms[i].transpose, assets->uniforms[i].pointer);
        break;
      case DrawUniformType_Mat3:
        glUniformMatrix3fv(i, assets->uniforms[i].count, assets->uniforms[i].transpose, assets->uniforms[i].pointer);
        break;
      case DrawUniformType_Mat4:
        glUniformMatrix4fv(i, assets->uniforms[i].count, assets->uniforms[i].transpose, assets->uniforms[i].pointer);
        break;
      case DrawUniformType_Mat2x3:
        glUniformMatrix2x3fv(i, assets->uniforms[i].count, assets->uniforms[i].transpose, assets->uniforms[i].pointer);
        break;
      case DrawUniformType_Mat3x2:
        glUniformMatrix3x2fv(i, assets->uniforms[i].count, assets->uniforms[i].transpose, assets->uniforms[i].pointer);
        break;
      case DrawUniformType_Mat2x4:
        glUniformMatrix2x4fv(i, assets->uniforms[i].count, assets->uniforms[i].transpose, assets->uniforms[i].pointer);
        break;
      case DrawUniformType_Mat4x2:
        glUniformMatrix4x2fv(i, assets->uniforms[i].count, assets->uniforms[i].transpose, assets->uniforms[i].pointer);
        break;
      case DrawUniformType_Mat3x4:
        glUniformMatrix3x4fv(i, assets->uniforms[i].count, assets->uniforms[i].transpose, assets->uniforms[i].pointer);
        break;
      case DrawUniformType_Mat4x3:
        glUniformMatrix4x3fv(i, assets->uniforms[i].count, assets->uniforms[i].transpose, assets->uniforms[i].pointer);
        break;
      }
    } else {
      switch(assets->uniforms[i].type) {
      case DrawUniformType_Unused:
        break;
      case DrawUniformType_Float:
        glUniform1f(i, assets->uniforms[i]._float);
        break;
      case DrawUniformType_Vec2:
        glUniform2f(i, assets->uniforms[i].vec2[0], assets->uniforms[i].vec2[1]);
        break;
      case DrawUniformType_Vec3:
        glUniform3f(i, assets->uniforms[i].vec3[0], assets->uniforms[i].vec3[1], assets->uniforms[i].vec3[2]);
        break;
      case DrawUniformType_Vec4:
        glUniform4f(i, assets->uniforms[i].vec4[0], assets->uniforms[i].vec4[1], assets->uniforms[i].vec4[2],
                    assets->uniforms[i].vec4[3]);
        break;
      case DrawUniformType_Int:
        glUniform1i(i, assets->uniforms[i]._int);
        break;
      case DrawUniformType_IVec2:
        glUniform2i(i, assets->uniforms[i].ivec2[0], assets->uniforms[i].ivec2[1]);
        break;
      case DrawUniformType_IVec3:
        glUniform3i(i, assets->uniforms[i].ivec3[0], assets->uniforms[i].ivec3[1], assets->uniforms[i].ivec3[2]);
        break;
      case DrawUniformType_IVec4:
        glUniform4i(i, assets->uniforms[i].ivec4[0], assets->uniforms[i].ivec4[1], assets->uniforms[i].ivec4[2],
                    assets->uniforms[i].ivec4[3]);
        break;
      case DrawUniformType_Uint:
        glUniform1ui(i, assets->uniforms[i].uint);
        break;
      case DrawUniformType_UVec2:
        glUniform2ui(i, assets->uniforms[i].uvec2[0], assets->uniforms[i].uvec2[1]);
        break;
      case DrawUniformType_UVec3:
        glUniform3ui(i, assets->uniforms[i].uvec3[0], assets->uniforms[i].uvec3[1], assets->uniforms[i].uvec3[2]);
        break;
      case DrawUniformType_UVec4:
        glUniform4ui(i, assets->uniforms[i].uvec4[0], assets->uniforms[i].uvec4[1], assets->uniforms[i].uvec4[2],
                     assets->uniforms[i].uvec4[3]);
        break;
      case DrawUniformType_Mat2:
      case DrawUniformType_Mat3:
      case DrawUniformType_Mat4:
      case DrawUniformType_Mat2x3:
      case DrawUniformType_Mat3x2:
      case DrawUniformType_Mat2x4:
      case DrawUniformType_Mat4x2:
      case DrawUniformType_Mat3x4:
      case DrawUniformType_Mat4x3:
        break;
      }
    }
  }
}

void GL_begin_draw(const struct DrawState *state, const struct DrawAssets *assets) {
  GL_initialize_draw_state(state);
  GL_apply_draw_state(state);
  GL_apply_draw_assets(assets);

  glad_glBegin(state->primitive);
}

void GL_end_draw(void) { glad_glEnd(); }
