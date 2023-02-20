#include "gl_local.h"

#include "gl_thin_cpp.h"

#define NUM_PARTICLES 1000000

THIN_GL_STRUCT(particle_Data, float32x4(origin__time), snorm16x4(velocity__alpha),
               snorm16x4(acceleration__alpha_velocity), unorm8x4(albedo), unorm8x4(emit),
               snorm16x2(incandescence__incandescence_velocity))
THIN_GL_BLOCK(particle_data, require(particle_Data), unsized_array(struct(particle_Data, item)))
static struct GL_Buffer data_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(struct GL_particle_Data) * NUM_PARTICLES};
static struct GL_ShaderResource data_resource = {
    .type = GL_Type_ShaderStorageBuffer, .block.buffer = &data_buffer, .block.structure = &GL_particle_data_shader};

THIN_GL_BLOCK(particle_distance, unsized_array(uint32(item)))
static struct GL_Buffer distance_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(uint32_t) * NUM_PARTICLES};
static struct GL_ShaderResource distance_resource = {.type = GL_Type_ShaderStorageBuffer,
                                                     .block.buffer = &distance_buffer,
                                                     .block.structure = &GL_particle_distance_shader};

THIN_GL_BLOCK(particle_dead, unsized_array(uint32(item)))
static struct GL_Buffer dead_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(uint32_t) * NUM_PARTICLES};
static struct GL_ShaderResource dead_resource = {
    .type = GL_Type_ShaderStorageBuffer, .block.buffer = &dead_buffer, .block.structure = &GL_particle_dead_shader};

THIN_GL_BLOCK(particle_alive, unsized_array(uint32(item)))
static struct GL_Buffer alive_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(uint32_t) * NUM_PARTICLES};
static struct GL_ShaderResource alive_resource = {
    .type = GL_Type_ShaderStorageBuffer, .block.buffer = &alive_buffer, .block.structure = &GL_particle_alive_shader};

THIN_GL_BLOCK(particle_sorted, unsized_array(uint32(item)))
static struct GL_Buffer sorted_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(uint32_t) * NUM_PARTICLES};
static struct GL_ShaderResource sorted_resource = {
    .type = GL_Type_ShaderStorageBuffer, .block.buffer = &sorted_buffer, .block.structure = &GL_particle_sorted_shader};

THIN_GL_BLOCK(particle_counter, uint32(alive), uint32(dead), uint32(emitted))
static struct GL_Buffer counter_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(struct GL_particle_counter)};
static struct GL_ShaderResource counter_resource = {.type = GL_Type_ShaderStorageBuffer,
                                                    .block.buffer = &counter_buffer,
                                                    .block.structure = &GL_particle_counter_shader};

THIN_GL_BLOCK(particle_indirect, require(DispatchIndirectCommand), require(DrawElementsIndirectCommand),
              struct(DispatchIndirectCommand, simulate), struct(DispatchIndirectCommand, sort1),
              struct(DispatchIndirectCommand, sort2), struct(DispatchIndirectCommand, sort3),
              struct(DispatchIndirectCommand, sort4), struct(DispatchIndirectCommand, sort5),
              struct(DispatchIndirectCommand, sort6), struct(DispatchIndirectCommand, sort7),
              struct(DispatchIndirectCommand, sort8), struct(DrawElementsIndirectCommand, draw))
static struct GL_Buffer indirect_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(struct GL_particle_indirect)};
static struct GL_ShaderResource indirect_resource = {.type = GL_Type_ShaderStorageBuffer,
                                                     .name = "indirect",
                                                     .block.buffer = &indirect_buffer,
                                                     .block.structure = &GL_particle_indirect_shader};

// clang-format off
static char initialize_compute_shader_source[] =
GL_MSTR(
  void main() {
    u_particle_counter.dead = u_count;

    u_particle_indirect.simulate.num_groups_x = 0;
    u_particle_indirect.simulate.num_groups_y = 1;
    u_particle_indirect.simulate.num_groups_z = 1;

    u_particle_indirect.sort1.num_groups_x = 0;
    u_particle_indirect.sort1.num_groups_y = 1;
    u_particle_indirect.sort1.num_groups_z = 1;

    u_particle_indirect.sort1.num_groups_x = 0;
    u_particle_indirect.sort1.num_groups_y = 1;
    u_particle_indirect.sort1.num_groups_z = 1;

    u_particle_indirect.sort1.num_groups_x = 0;
    u_particle_indirect.sort1.num_groups_y = 1;
    u_particle_indirect.sort1.num_groups_z = 1;

    u_particle_indirect.sort1.num_groups_x = 0;
    u_particle_indirect.sort1.num_groups_y = 1;
    u_particle_indirect.sort1.num_groups_z = 1;

    u_particle_indirect.sort1.num_groups_x = 0;
    u_particle_indirect.sort1.num_groups_y = 1;
    u_particle_indirect.sort1.num_groups_z = 1;

    u_particle_indirect.sort1.num_groups_x = 0;
    u_particle_indirect.sort1.num_groups_y = 1;
    u_particle_indirect.sort1.num_groups_z = 1;

    u_particle_indirect.sort1.num_groups_x = 0;
    u_particle_indirect.sort1.num_groups_y = 1;
    u_particle_indirect.sort1.num_groups_z = 1;

    u_particle_indirect.sort1.num_groups_x = 0;
    u_particle_indirect.sort1.num_groups_y = 1;
    u_particle_indirect.sort1.num_groups_z = 1;

    u_particle_indirect.draw.count = 0;
    u_particle_indirect.draw.instance_count = 1;
    u_particle_indirect.draw.first_index = 0;
    u_particle_indirect.draw.base_vertex = 0;
    u_particle_indirect.draw.base_instance = 0;

    for(uint i = 0; i < u_count; i++)
      u_particle_dead.item[i] = i;
  }
);
// clang-format on

static struct GL_ComputeState initialize_compute_state = {.shader.source = initialize_compute_shader_source,
                                                          .uniform[0] = {0, GL_Type_Uint, "count"},
                                                          .global[0] = {0, &counter_resource},
                                                          .global[1] = {0, &indirect_resource},
                                                          .global[2] = {0, &dead_resource}};

static void ensure_init(void) {
  static bool init = false;
  if(init)
    return;
  init = true;

  GL_compute(&initialize_compute_state, &(struct GL_ComputeAssets){.uniforms[0].uint = NUM_PARTICLES}, 1, 1, 1);
}

// clang-format off
static char compute_emit_shader_source[] =
GL_MSTR(
  void main() {
    uint index = u_DEAD.item[atomicCounterDecrement(u_COUNTER.dead) - 1];
    u_DATA.item[index] = ...;
    u_ALIVE[atomicCounterIncrement(u_COUNTER.alive)] = index;
  }
);
// clang-format on

// clang-format off
static char presimulate_compute_shader_source[] =
GL_MSTR(
  void main() {
    u_INDIRECT.simulate.num_groups_x = u_COUNTER.alive;
    u_COUNTER.emitter = 0;
  }
);
// clang-format on

struct GL_ComputeState presimulate_compute_state = {.shader.source = presimulate_compute_shader_source,
                                                    .global[0] = {0, &indirect_resource},
                                                    .global[1] = {0, &counter_resource}};

// clang-format off
static char simulate_compute_shader_source[] =
GL_MSTR(
  void main() {
    uint work_id = gl_GlobalInvocationID.x;
    uint index = u_ALIVE.item[work_id];
    Data data = Data_unpack(u_DATA.item[index]);
    float time = u_FRAME.time - data.origin__time.w;
    float alpha = data.velocity__alpha.w + data.acceleration__alpha_velocity.w * time;
    if(alpha > 0) {
      vec3 position = data.origin__time.xyz + (data.velocity__alpha.xyz / 256) * time + (data.acceleration__alpha_velocity.xyz / 256) * time * time;
      u_ACTIVE.item[index].position__distance = vec4(position, length(position - u_FRAME.view_origin));
      u_SORTED.item[atomicCounterIncrement(u_COUNTER.emitted)] = index;
    } else {
      u_DEAD.item[atomicCounterIncrement(u_COUNTER.dead)] = index;
    }
  }
);
// clang-format on

struct GL_ComputeState simulate_compute_state = {.shader.source = simulate_compute_shader_source,
                                                 .global[0] = {0, &alive_resource},
                                                 .global[1] = {0, &data_resource},
                                                 .global[2] = {0, &distance_resource},
                                                 .global[3] = {0, &sorted_resource},
                                                 .global[4] = {0, &dead_resource},
                                                 .global[5] = {0, &counter_resource}};

// clang-format off
static char postsimulate_compute_shader_source[] =
GL_MSTR(
  void main() {
    uint count = u_COUNTER.emitted;
    u_INDIRECT.sort1.num_groups_x = count;
    u_INDIRECT.sort2.num_groups_x = count;
    u_INDIRECT.sort3.num_groups_x = count;
    u_INDIRECT.sort4.num_groups_x = count;
    u_INDIRECT.sort5.num_groups_x = count;
    u_INDIRECT.sort6.num_groups_x = count;
    u_INDIRECT.sort7.num_groups_x = count;
    u_INDIRECT.sort8.num_groups_x = count;
    u_INDIRECT.draw.count = count;
  }
);
// clang-format on

struct GL_ComputeState postsimulate_compute_state = {.shader.source = postsimulate_compute_shader_source,
                                                     .global[0] = {0, &indirect_resource},
                                                     .global[1] = {0, &counter_resource}};

// clang-format off
static char vertex_shader_source[] =
GL_MSTR(
  layout(location = 0) out vec3 out_albedo;
  layout(location = 1) out vec3 out_emit;

  void main() {
    float time = u_time - in_time;
    vec3 position = in_origin + (((in_acceleration * time) + in_velocity) * time) / 256;
    float alpha = clamp((in_alpha + in_alpha_velocity * time) / 256, 0, 1);
    float incandescence = clamp((in_incandescence + in_incandescence_velocity * time) / 256, 0, 1);

    gl_Position = u_view_projection_matrix * vec4(position, 1);
    float size = u_point_size_sizemin_sizemax.x;
    float size_min = u_point_size_sizemin_sizemax.y;
    float size_max = u_point_size_sizemin_sizemax.z;
    float a = u_point_a_b_c.x;
    float b = u_point_a_b_c.y;
    float c = u_point_a_b_c.z;
    float dist = length(u_view_matrix * vec4(position, 1));
    float dist_atten = 1 / (a + b * dist + c * dist * dist);
    float derived_size = size * sqrt(dist_atten);
    gl_PointSize = clamp(derived_size, size_min, size_max);
    vec3 albedo_white = mix(vec3(1), in_albedo, min(alpha * 2, 1));
    vec3 albedo_black = mix(in_albedo, vec3(0), max(alpha * 2 - 1, 0));
    out_albedo = mix(albedo_white, albedo_black, alpha);
    // TODO: query light space to add to light emitted (multiplied by in_albedo.rgb)
    //       right now, it is being multiplied by in_albedo.a
    out_emit = in_albedo * alpha * 0.25 + in_emit * incandescence * alpha;
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

#if 0
void GL_AddParticle(float time, const vec3_t origin, const vec3_t velocity, const vec3_t acceleration, int albedo,
                    int emit, float alpha, float alphavel, float incandescence, float incandescencevel) {
  if(_.buffer.buffer == 0) {
    _.buffer = GL_allocate_dynamic_buffer(GL_ARRAY_BUFFER, sizeof(struct ParticleBuffer) * NUM_PARTICLES);
    _.write_index = 0;
    _.read_index = 0;
  }
  ((struct ParticleBuffer *)_.buffer.dynamic.mapping)[_.write_index % NUM_PARTICLES] = (struct ParticleBuffer){
      .time = time,
      .origin[0] = origin[0],
      .origin[1] = origin[1],
      .origin[2] = origin[2],
      .velocity[0] = velocity[0] * 256,
      .velocity[1] = velocity[1] * 256,
      .velocity[2] = velocity[2] * 256,
      .acceleration[0] = acceleration[0] * 256,
      .acceleration[1] = acceleration[1] * 256,
      .acceleration[2] = acceleration[2] * 256,
      .albedo[0] = ((uint8_t *)(d_8to24table + albedo))[0],
      .albedo[1] = ((uint8_t *)(d_8to24table + albedo))[1],
      .albedo[2] = ((uint8_t *)(d_8to24table + albedo))[2],
      .emit[0] = ((uint8_t *)(d_8to24table + emit))[0],
      .emit[1] = ((uint8_t *)(d_8to24table + emit))[1],
      .emit[2] = ((uint8_t *)(d_8to24table + emit))[2],
      .alpha = alpha * 256,
      .alpha_velocity = alphavel * 256,
      .incandescence = incandescence * 256,
      .incandescence_velocity = incandescencevel * 256,
  };
  _.write_index++;
}

struct ParticleBuffer {
  float origin[3];                //  0 float
  float time;                     // 12 float
  int16_t velocity[3];            // 16 snorm
  int16_t alpha;                  // 22 snorm
  int16_t acceleration[3];        // 24 snorm
  int16_t alpha_velocity;         // 30 snorm
  uint8_t albedo[3];              // 32 unorm
  uint8_t unused_1;               // 35 -----
  uint8_t emit[3];                // 36 unorm
  uint8_t unused_2;               // 39 -----
  int16_t incandescence;          // 40 snorm
  int16_t incandescence_velocity; // 42 snorm
};

static void flush_particles(void) {
  // if(_.read_index < _.write_index) {
  //   uint32_t base = _.read_index % NUM_PARTICLES;
  //   uint32_t length = _.write_index - _.read_index;
  //   if(length > NUM_PARTICLES) {
  //     base = 0;
  //     length = NUM_PARTICLES;
  //   }
  //   if(base + length > NUM_PARTICLES) {
  //     GL_flush_dynamic_buffer(_.buffer, base * sizeof(struct ParticleBuffer),
  //                             (NUM_PARTICLES - base) * sizeof(struct ParticleBuffer));
  //     GL_flush_dynamic_buffer(_.buffer, 0, (length - (NUM_PARTICLES - base)) * sizeof(struct ParticleBuffer));
  //   } else {
  //     GL_flush_dynamic_buffer(_.buffer, base * sizeof(struct ParticleBuffer), length * sizeof(struct
  //     ParticleBuffer));
  //   }
  //   _.read_index = _.write_index;
  // }
}

static struct GL_DrawState particle_draw_state = {
    .primitive = GL_POINTS,
    .attribute[0] = {0, alias_memory_Format_Float32, 3, "origin", 0},
    .attribute[1] = {0, alias_memory_Format_Float32, 1, "time", 12},
    .attribute[2] = {0, alias_memory_Format_Snorm16, 3, "velocity", 16},
    .attribute[3] = {0, alias_memory_Format_Snorm16, 1, "alpha", 22},
    .attribute[4] = {0, alias_memory_Format_Snorm16, 3, "acceleration", 24},
    .attribute[5] = {0, alias_memory_Format_Snorm16, 1, "alpha_velocity", 30},
    .attribute[6] = {0, alias_memory_Format_Unorm8, 3, "albedo", 32},
    .attribute[7] = {0, alias_memory_Format_Unorm8, 3, "emit", 36},
    .attribute[8] = {0, alias_memory_Format_Snorm16, 1, "incandescence", 40},
    .attribute[9] = {0, alias_memory_Format_Snorm16, 1, "incandescence_velocity", 42},
    .binding[0] = {sizeof(struct ParticleBuffer), 0},
    .uniform[0] = {THIN_GL_VERTEX_BIT, GL_Type_Float3, "point_size_sizemin_sizemax"},
    .uniform[1] = {THIN_GL_VERTEX_BIT, GL_Type_Float3, "point_a_b_c"},
    .global[0] = {THIN_GL_VERTEX_BIT, &u_view_projection_matrix},
    .global[1] = {THIN_GL_VERTEX_BIT, &u_view_matrix},
    .global[2] = {THIN_GL_VERTEX_BIT, &u_time},
    .vertex_shader.source = vertex_shader_source,
    .fragment_shader.source = fragment_shader_source,
    .depth_test_enable = true,
    .depth_range_min = 0,
    .depth_range_max = 1,
    .depth_mask = false,
    .blend_enable = true,
    .blend_src_factor = GL_ONE,
    .blend_dst_factor = GL_SRC1_COLOR};

struct ParticlePositionAttributes {
  float position[3];
  float distance_squared;
  uint8_t albedo[4];
  uint8_t emit[4];
};

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

  struct GL_DrawAssets assets = {.uniforms[0] = {.vec[0] = gl_particle_size->value,
                                                 .vec[1] = gl_particle_min_size->value,
                                                 .vec[2] = gl_particle_max_size->value},
                                 .uniforms[1] = {.vec[0] = gl_particle_att_a->value,
                                                 .vec[1] = gl_particle_att_b->value,
                                                 .vec[2] = gl_particle_att_c->value},
                                 .vertex_buffers[0] = &buffer};

  GL_draw_arrays(&particle_draw_state, &assets, 0, r_newrefdef.num_particles, 1, 0);
}
#else
void GL_AddParticle(float time, const vec3_t origin, const vec3_t velocity, const vec3_t acceleration, int albedo,
                    int emit, float alpha, float alphavel, float incandescence, float incandescencevel) {
  ensure_init();
}

static struct GL_DrawState particle_draw_state = {
    .primitive = GL_POINTS,
    .attribute[0] = {0, alias_memory_Format_Float32, 3, "origin", offsetof(struct GL_particle_Data, origin__time)},
    .attribute[1] = {0, alias_memory_Format_Float32, 1, "time", offsetof(struct GL_particle_Data, origin__time) + 12},
    .attribute[2] = {0, alias_memory_Format_Snorm16, 3, "velocity", offsetof(struct GL_particle_Data, velocity__alpha)},
    .attribute[3] = {0, alias_memory_Format_Snorm16, 1, "alpha",
                     offsetof(struct GL_particle_Data, velocity__alpha) + 6},
    .attribute[5] = {0, alias_memory_Format_Snorm16, 3, "velocity",
                     offsetof(struct GL_particle_Data, acceleration__alpha_velocity)},
    .attribute[5] = {0, alias_memory_Format_Snorm16, 1, "alpha_velocity",
                     offsetof(struct GL_particle_Data, acceleration__alpha_velocity) + 6},
    .attribute[6] = {0, alias_memory_Format_Unorm8, 3, "albedo", offsetof(struct GL_particle_Data, albedo)},
    .attribute[7] = {0, alias_memory_Format_Unorm8, 3, "emit", offsetof(struct GL_particle_Data, emit)},
    .attribute[8] = {0, alias_memory_Format_Snorm16, 1, "incandescence",
                     offsetof(struct GL_particle_Data, incandescence__incandescence_velocity)},
    .attribute[9] = {0, alias_memory_Format_Snorm16, 1, "incandescence_velocity",
                     offsetof(struct GL_particle_Data, incandescence__incandescence_velocity) + 2},
    .binding[0] = {sizeof(struct GL_particle_Data), 0},
    .uniform[0] = {THIN_GL_VERTEX_BIT, GL_Type_Float3, "point_size_sizemin_sizemax"},
    .uniform[1] = {THIN_GL_VERTEX_BIT, GL_Type_Float3, "point_a_b_c"},
    .global[0] = {THIN_GL_VERTEX_BIT, &u_view_projection_matrix},
    .global[1] = {THIN_GL_VERTEX_BIT, &u_view_matrix},
    .global[2] = {THIN_GL_VERTEX_BIT, &u_time},
    .vertex_shader.source = vertex_shader_source,
    .fragment_shader.source = fragment_shader_source,
    .depth_test_enable = true,
    .depth_range_min = 0,
    .depth_range_max = 1,
    .depth_mask = false,
    .blend_enable = true,
    .blend_src_factor = GL_ONE,
    .blend_dst_factor = GL_SRC1_COLOR};

void GL_draw_particles(void) {
  ensure_init();

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // ensure we see emit evocations
  GL_compute(&presimulate_compute_state, NULL, 1, 1, 1);

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
  GL_compute_indirect(&simulate_compute_state, NULL, &indirect_buffer, offsetof(struct GL_particle_indirect, simulate));

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  GL_compute(&postsimulate_compute_state, NULL, 1, 1, 1);

  // if(0) {
  //   GL_compute_indirect(&sort1_compute_state, NULL, &indirect_buffer, offsetof(struct GL_particle_INDIRECT, sort1));
  //   GL_compute_indirect(&sort2_compute_state, NULL, &indirect_buffer, offsetof(struct GL_particle_INDIRECT, sort2));
  //   GL_compute_indirect(&sort3_compute_state, NULL, &indirect_buffer, offsetof(struct GL_particle_INDIRECT, sort3));
  //   GL_compute_indirect(&sort4_compute_state, NULL, &indirect_buffer, offsetof(struct GL_particle_INDIRECT, sort4));
  //   GL_compute_indirect(&sort5_compute_state, NULL, &indirect_buffer, offsetof(struct GL_particle_INDIRECT, sort5));
  //   GL_compute_indirect(&sort6_compute_state, NULL, &indirect_buffer, offsetof(struct GL_particle_INDIRECT, sort6));
  //   GL_compute_indirect(&sort7_compute_state, NULL, &indirect_buffer, offsetof(struct GL_particle_INDIRECT, sort7));
  //   GL_compute_indirect(&sort8_compute_state, NULL, &indirect_buffer, offsetof(struct GL_particle_INDIRECT, sort8));
  // }

  struct GL_DrawAssets assets = {.uniforms[0] = {.vec[0] = gl_particle_size->value,
                                                 .vec[1] = gl_particle_min_size->value,
                                                 .vec[2] = gl_particle_max_size->value},
                                 .uniforms[1] = {.vec[0] = gl_particle_att_a->value,
                                                 .vec[1] = gl_particle_att_b->value,
                                                 .vec[2] = gl_particle_att_c->value},
                                 .vertex_buffers[0] = &data_buffer};

  glMemoryBarrier(GL_ELEMENT_ARRAY_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
  GL_draw_elements_indirect(&particle_draw_state, &assets, &indirect_buffer,
                            offsetof(struct GL_particle_indirect, draw));
}
#endif