#include "gl_local.h"
#include "gl_thin.h"

#define NUM_BEAM_SEGS 6

static struct GL_VertexFormat vertex_format = {
    .attribute[0] = {0, alias_memory_Format_Float32, 3, "position"},
    .binding[0] = {sizeof(float) * 3},
};

static struct GL_UniformsFormat uniforms_format = {
    .uniform[0] = {THIN_GL_FRAGMENT_BIT, GL_UniformType_Vec4, "color"},
};

// clang-format off
static const char vertex_shader_source[] =
  GL_MSTR(
    void main() {
      gl_Position = gl_ModelViewProjectionMatrix * vec4(in_position, 1);
    }
  );

static const char fragment_shader_source[] =
  GL_MSTR(
    layout(location = 0) out vec4 out_color;
    void main() {
      out_color = u_color;
    }
  );
// clang-format on

static struct DrawState draw_state = {.primitive = GL_TRIANGLE_STRIP,
                                      .vertex_format = &vertex_format,
                                      .uniforms_format = &uniforms_format,
                                      .vertex_shader_source = vertex_shader_source,
                                      .fragment_shader_source = fragment_shader_source,
                                      .depth_test_enable = true,
                                      .depth_range_min = 0,
                                      .depth_range_max = 1,
                                      .depth_mask = false,
                                      .blend_enable = true,
                                      .blend_src_factor = GL_SRC_ALPHA,
                                      .blend_dst_factor = GL_ONE_MINUS_SRC_ALPHA};

void GL_draw_beam(entity_t *e) {
  int i;
  float r, g, b;

  vec3_t perpvec;
  vec3_t direction, normalized_direction;
  vec3_t start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
  vec3_t oldorigin, origin;

  oldorigin[0] = e->oldorigin[0];
  oldorigin[1] = e->oldorigin[1];
  oldorigin[2] = e->oldorigin[2];

  origin[0] = e->origin[0];
  origin[1] = e->origin[1];
  origin[2] = e->origin[2];

  normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
  normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
  normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

  if(VectorNormalize(normalized_direction) == 0)
    return;

  PerpendicularVector(perpvec, normalized_direction);
  VectorScale(perpvec, e->frame / 2, perpvec);

  for(i = 0; i < 6; i++) {
    RotatePointAroundVector(start_points[i], normalized_direction, perpvec, (360.0 / NUM_BEAM_SEGS) * i);
    VectorAdd(start_points[i], origin, start_points[i]);
    VectorAdd(start_points[i], direction, end_points[i]);
  }

  r = (d_8to24table[e->skinnum & 0xFF]) & 0xFF;
  g = (d_8to24table[e->skinnum & 0xFF] >> 8) & 0xFF;
  b = (d_8to24table[e->skinnum & 0xFF] >> 16) & 0xFF;

  struct GL_Buffer vertex_buffer = GL_allocate_temporary_buffer(GL_ARRAY_BUFFER, sizeof(float) * 3 * 4 * NUM_BEAM_SEGS);

  float *xyz = vertex_buffer.temporary.mapping;

  for(i = 0; i < NUM_BEAM_SEGS; i++) {
    VectorCopy(start_points[i], xyz);
    xyz += 3;
    VectorCopy(end_points[i], xyz);
    xyz += 3;
    VectorCopy(start_points[(i + 1) % NUM_BEAM_SEGS], xyz);
    xyz += 3;
    VectorCopy(end_points[(i + 1) % NUM_BEAM_SEGS], xyz);
    xyz += 3;
  }

  GL_draw_arrays(
      &draw_state,
      &(struct DrawAssets){
          .uniforms[0] = {.vec4[0] = r / 255.0f, .vec4[1] = g / 255.0f, .vec4[2] = b / 255.0f, .vec4[3] = e->alpha},
          .vertex_buffers[0] = &vertex_buffer,
      },
      0, 4 * NUM_BEAM_SEGS, 1, 0);
}