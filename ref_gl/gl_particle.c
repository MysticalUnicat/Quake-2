#include "gl_local.h"

#include "gl_thin.h"

// particle idea:
// dual source blending to do this per particle:
// framebuffer = framebuffer * particle.absorb + particle.reflect * scene.light + particle.emit
// dst blend factor set to SRC1_COLOR
// src blend factor set to ONE
// becomes
// dst = (dst * src1) + src0

// clang-format off
static char vertex_shader_source[] =
GL_MSTR(
  layout(location = 0) out vec3 out_albedo;
  layout(location = 1) out vec3 out_emit;

  void main() {
    gl_Position = u_view_projection_matrix * vec4(in_position.xyz, 1);
    float size = u_point_size_sizemin_sizemax.x;
    float size_min = u_point_size_sizemin_sizemax.y;
    float size_max = u_point_size_sizemin_sizemax.z;
    float a = u_point_a_b_c.x;
    float b = u_point_a_b_c.y;
    float c = u_point_a_b_c.z;
    float dist = sqrt(in_position.w);
    float dist_atten = 1 / (a + b * dist + c * dist * dist);
    float derived_size = size * sqrt(dist_atten);
    gl_PointSize = clamp(derived_size, size_min, size_max);
    vec3 albedo_white = mix(vec3(1), in_albedo.rgb, min(in_albedo.a * 2, 1));
    vec3 albedo_black = mix(in_albedo.rgb, vec3(0), max(in_albedo.a * 2 - 1, 0));
    out_albedo = mix(albedo_white, albedo_black, in_albedo.a);
    // TODO: query light space to add to light emitted (multiplied by in_albedo.rgb)
    //       right now, it is being multiplied by in_albedo.a
    out_emit = in_albedo.rgb * in_albedo.a * 0.25 + in_emit.rgb * in_emit.a * in_albedo.a;
  }
);

static char fragment_shader_source[] =
GL_MSTR(
  layout(location = 0) in vec3 in_albedo;
  layout(location = 1) in vec3 in_emit;

  layout(location = 0, index = 0) out vec4 out_emit;
  layout(location = 0, index = 1) out vec4 out_absorb;

  void main() {
    out_absorb = vec4(in_albedo, 1);
    out_emit = vec4(in_emit, 1);
  }
);
// clang-format on

struct ParticleBuffer {
  float origin[3];                 // 0
  float time;                      // 12
  uint16_t velocity[3];            // 16
  uint16_t alpha;                  // 22
  uint16_t acceleration[3];        // 24
  uint16_t alpha_velocity;         // 30
  uint8_t albedo[3];               // 32
  uint8_t unused_1;                // 35
  uint8_t emit[3];                 // 36
  uint8_t unused_2;                // 39
  uint16_t incandescence;          // 40
  uint16_t incandescence_velocity; // 42
};

struct ParticlePositionAttributes {
  float position[3];
  float distance_squared;
  uint8_t albedo[4];
  uint8_t emit[4];
};

static struct DrawState particle_draw_state = {
    .primitive = GL_POINTS,
    .attribute[0] = {0, alias_memory_Format_Float32, 4, "position", 0},
    .attribute[1] = {0, alias_memory_Format_Unorm8, 4, "albedo", 16},
    .attribute[2] = {0, alias_memory_Format_Unorm8, 4, "emit", 20},
    .binding[0] = {sizeof(float) * 4 + sizeof(uint8_t) * 8, 0},
    .uniform[0] = {THIN_GL_VERTEX_BIT, GL_UniformType_Vec3, "point_size_sizemin_sizemax"},
    .uniform[1] = {THIN_GL_VERTEX_BIT, GL_UniformType_Vec3, "point_a_b_c"},
    .global[0] = {THIN_GL_VERTEX_BIT, &u_view_projection_matrix},
    .global[1] = {THIN_GL_VERTEX_BIT, &u_view_matrix},
    .global[2] = {THIN_GL_VERTEX_BIT, &u_time},
    .vertex_shader_source = vertex_shader_source,
    .fragment_shader_source = fragment_shader_source,
    .depth_test_enable = true,
    .depth_range_min = 0,
    .depth_range_max = 1,
    .depth_mask = false,
    .blend_enable = true,
    .blend_src_factor = GL_ONE,
    .blend_dst_factor = GL_SRC1_COLOR};

static int compare_particle(const void *ap, const void *bp) {
  const struct ParticlePositionAttributes *a = (const struct ParticlePositionAttributes *)ap;
  const struct ParticlePositionAttributes *b = (const struct ParticlePositionAttributes *)bp;
  return (int)(a->distance_squared - b->distance_squared);
}

void GL_draw_particles(void) {
  int i;
  const particle_t *p;

  if(r_newrefdef.num_particles == 0)
    return;

  struct GL_Buffer buffer = GL_allocate_temporary_buffer(GL_ARRAY_BUFFER, sizeof(struct ParticlePositionAttributes) *
                                                                              r_newrefdef.num_particles);

  struct ParticlePositionAttributes *output = (struct ParticlePositionAttributes *)buffer.temporary.mapping;

  for(i = 0, p = r_newrefdef.particles; i < r_newrefdef.num_particles; i++, p++) {
    output[i].position[0] = p->origin[0];
    output[i].position[1] = p->origin[1];
    output[i].position[2] = p->origin[2];

    float dist[3] = {p->origin[0] - r_newrefdef.vieworg[0], p->origin[1] - r_newrefdef.vieworg[1],
                     p->origin[2] - r_newrefdef.vieworg[2]};
    output[i].distance_squared = dist[0] * dist[0] + dist[1] * dist[1] + dist[2] * dist[2];

    *(int *)output[i].albedo = d_8to24table[p->albedo];
    output[i].albedo[3] = p->alpha * 255;

    *(int *)output[i].emit = d_8to24table[p->emit];
    output[i].emit[3] = p->incandescence * 255;
  }

  qsort(output, r_newrefdef.num_particles, sizeof(*output), compare_particle);

  struct DrawAssets assets = {.uniforms[0] = {.vec[0] = gl_particle_size->value,
                                              .vec[1] = gl_particle_min_size->value,
                                              .vec[2] = gl_particle_max_size->value},
                              .uniforms[1] = {.vec[0] = gl_particle_att_a->value,
                                              .vec[1] = gl_particle_att_b->value,
                                              .vec[2] = gl_particle_att_c->value},
                              .vertex_buffers[0] = &buffer};

  GL_draw_arrays(&particle_draw_state, &assets, 0, r_newrefdef.num_particles, 1, 0);
}
