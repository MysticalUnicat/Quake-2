#include "gl_local.h"

#include "gl_thin.h"

// clang-format off
static struct GL_VertexFormat particle_vertex_format = {
  .attribute[0] = {0, alias_memory_Format_Float32, 3, "in_position", 0},
  .attribute[1] = {0, alias_memory_Format_Unorm8, 4, "in_color", 12},
  .binding[0] = {sizeof(float) * 3 + sizeof(uint8_t) * 4, 0},
};

static struct GL_UniformFormat particle_unfiform_format = {
  .uniform[0] = {THIN_GL_VERTEX_BIT, GL_UniformType_Vec3, "u_point_size_sizemin_sizemax"},
  .uniform[1] = {THIN_GL_VERTEX_BIT, GL_UniformType_Vec3, "u_point_a_b_c"},
};

static char particle_vertex_shader_source[] =
GL_MSTR(
  layout(location = 0) out vec4 out_color;

  void main() {
    gl_Position = gl_ModelViewProjectionMatrix * vec4(in_position, 1);
    float size = u_point_size_sizemin_sizemax.x;
    float size_min = u_point_size_sizemin_sizemax.y;
    float size_max = u_point_size_sizemin_sizemax.z;
    float a = u_point_a_b_c.x;
    float b = u_point_a_b_c.y;
    float c = u_point_a_b_c.z;
    float d = length((gl_ModelViewMatrix * vec4(in_position, 1)).xyz);
    float dist_atten = 1 / (a + b * d + c * d * d);
    gl_PointSize = clamp(size * dist_atten, size_min, size_max);
    out_color = in_color;
  }
);

static char particle_fragment_shader_source[] =
GL_MSTR(
  layout(location = 0) in vec4 in_color;

  layout(location = 0) out vec4 out_color;

  void main() {
    vec4 color = in_color;

    if(dot(gl_PointCoord - 0.5, gl_PointCoord - 0.5) > 0.25) {
      color.a = 0;
    }

    out_color = in_color;
  }
);
// clang-format on

static struct DrawState particle_draw_state = {.primitive = GL_POINTS,
                                               .vertex_format = &particle_vertex_format,
                                               .uniform_format = &particle_unfiform_format,
                                               .vertex_shader_source = particle_vertex_shader_source,
                                               .fragment_shader_source = particle_fragment_shader_source,
                                               .depth_test_enable = true,
                                               .depth_range_min = 0,
                                               .depth_range_max = 1,
                                               .depth_mask = false,
                                               .blend_enable = true,
                                               .blend_src_factor = GL_SRC_ALPHA,
                                               .blend_dst_factor = GL_ONE_MINUS_SRC_ALPHA};

void GL_draw_particles(void) {
  int i;
  const particle_t *p;

  if(r_newrefdef.num_particles == 0)
    return;

  struct ParticlePositionAttributes {
    float position[3];
    uint8_t color[4];
  };

  struct GL_Buffer buffer = GL_allocate_temporary_buffer(GL_ARRAY_BUFFER, sizeof(struct ParticlePositionAttributes) *
                                                                              r_newrefdef.num_particles);

  struct ParticlePositionAttributes *output = (struct ParticlePositionAttributes *)buffer.temporary.mapping;

  for(i = 0, p = r_newrefdef.particles; i < r_newrefdef.num_particles; i++, p++) {
    output[i].position[0] = p->origin[0];
    output[i].position[1] = p->origin[1];
    output[i].position[2] = p->origin[2];

    *(int *)output[i].color = d_8to24table[p->color];
    output[i].color[3] = p->alpha * 255;
  }

  struct DrawAssets assets = {.uniforms[0] = {.vec3[0] = gl_particle_size->value,
                                              .vec3[1] = gl_particle_min_size->value,
                                              .vec3[2] = gl_particle_max_size->value},
                              .uniforms[1] = {.vec3[0] = gl_particle_att_a->value,
                                              .vec3[1] = gl_particle_att_b->value,
                                              .vec3[2] = gl_particle_att_c->value},
                              .vertex_buffers[0] = &buffer};

  GL_draw_arrays(&particle_draw_state, &assets, 0, r_newrefdef.num_particles, 1, 0);
}
