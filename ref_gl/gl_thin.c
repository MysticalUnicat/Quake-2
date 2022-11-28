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
  }

  if(current_draw_state.depth_test_enable != state->depth_test_enable) {
    (state->depth_test_enable ? glEnable : glDisable)(GL_DEPTH_TEST);
  }

  if(current_draw_state.depth_mask != state->depth_mask) {
    glDepthMask(state->depth_mask ? GL_TRUE : GL_FALSE);
  }

  if(current_draw_state.depth_range_min != state->depth_range_min ||
     current_draw_state.depth_range_max != state->depth_range_max) {
    glDepthRangef(state->depth_range_min, state->depth_range_max);
  }

  if(state->blend_enable) {
    if(!current_draw_state.blend_enable || current_draw_state.blend_src_factor != state->blend_src_factor ||
       current_draw_state.blend_dst_factor != state->blend_dst_factor) {
      glEnable(GL_BLEND);
      glBlendFunc(state->blend_src_factor, state->blend_dst_factor);
    }
  } else if(current_draw_state.blend_enable) {
    glDisable(GL_BLEND);
  }

  current_draw_state = *state;
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
}

void GL_begin_draw(const struct DrawState *state, const struct DrawAssets *assets) {
  GL_initialize_draw_state(state);
  GL_apply_draw_state(state);
  GL_apply_draw_assets(assets);

  glad_glBegin(state->primitive);
}

void GL_end_draw(void) { glad_glEnd(); }
