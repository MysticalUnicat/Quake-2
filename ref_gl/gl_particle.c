#include "gl_local.h"

#include "gl_thin_cpp.h"

#define NUM_PARTICLES 1000000

THIN_GL_STRUCT(particle_Data, float32x4(origin_time), snorm16x4(velocity_alpha), snorm16x4(acceleration_alphaVelocity),
               unorm8x4(albedo), unorm8x4(emit), snorm16x2(incandescence_incandescenceVelocity))
THIN_GL_BLOCK(particle_data, require(particle_Data), unsized_array(struct(particle_DataPacked, item)))
static struct GL_Buffer data_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(struct GL_particle_Data) * NUM_PARTICLES};
static struct GL_ShaderResource data_resource = {.type = GL_Type_ShaderStorageBuffer,
                                                 .name = "particle_data",
                                                 .block.buffer = &data_buffer,
                                                 .block.structure = &GL_particle_data_shader};

THIN_GL_BLOCK(particle_distance, unsized_array(uint32(item)))
static struct GL_Buffer distance_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(uint32_t) * NUM_PARTICLES};
static struct GL_ShaderResource distance_resource = {.type = GL_Type_ShaderStorageBuffer,
                                                     .name = "particle_distance",
                                                     .block.buffer = &distance_buffer,
                                                     .block.structure = &GL_particle_distance_shader};

THIN_GL_BLOCK(particle_dead, unsized_array(uint32(item)))
static struct GL_Buffer dead_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(uint32_t) * NUM_PARTICLES};
static struct GL_ShaderResource dead_resource = {.type = GL_Type_ShaderStorageBuffer,
                                                 .name = "particle_dead",
                                                 .block.buffer = &dead_buffer,
                                                 .block.structure = &GL_particle_dead_shader};

THIN_GL_BLOCK(particle_alive, unsized_array(uint32(item)))
static struct GL_Buffer alive_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(uint32_t) * NUM_PARTICLES};
static struct GL_ShaderResource alive_resource = {.type = GL_Type_ShaderStorageBuffer,
                                                  .name = "particle_alive",
                                                  .block.buffer = &alive_buffer,
                                                  .block.structure = &GL_particle_alive_shader};

THIN_GL_BLOCK(particle_sorted, unsized_array(uint32(item)))
static struct GL_Buffer sorted_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(uint32_t) * NUM_PARTICLES};
static struct GL_ShaderResource sorted_resource = {.type = GL_Type_ShaderStorageBuffer,
                                                   .name = "particle_sorted",
                                                   .block.buffer = &sorted_buffer,
                                                   .block.structure = &GL_particle_sorted_shader};

THIN_GL_BLOCK(particle_counter, uint32(alive), int32(dead))
static struct GL_Buffer counter_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(struct GL_particle_counter)};
static struct GL_ShaderResource counter_resource = {.type = GL_Type_ShaderStorageBuffer,
                                                    .name = "particle_counter",
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
                                                     .name = "particle_indirect",
                                                     .block.buffer = &indirect_buffer,
                                                     .block.structure = &GL_particle_indirect_shader};

// clang-format off
static char initialize_compute_shader_source[] =
GL_MSTR(
  void main() {
    u_particle_counter.dead = int(u_count);

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
      u_particle_dead.item[i] = (u_count - 1) - i;
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
static char emit_single_compute_shader_source[] =
GL_MSTR(
  void main() {
    int dead_index = atomicAdd(u_particle_counter.dead, -1);
    atomicMax(u_particle_counter.dead, 0);

    if(dead_index > 0) {
      uint index = u_particle_dead.item[dead_index - 1];
      u_particle_data.item[index] = particle_Data_pack(particle_Data(
        vec4(u_origin, u_time),
        vec4(u_velocity / 256, u_alpha / 256),
        vec4(u_acceleration / 256, u_alphaVelocity / 256),
        u_albedo,
        u_emit,
        vec2(u_incandescnence / 256, u_incandescenceVelocity / 256)
      ));
      u_particle_alive.item[atomicAdd(u_particle_counter.alive, 1)] = index;
    }
  }
);
// clang-format on

struct GL_ComputeState emit_single_compute_state = {.shader.source = emit_single_compute_shader_source,
                                                    .uniform[0] = {0, GL_Type_Float3, "origin"},
                                                    .uniform[1] = {0, GL_Type_Float3, "velocity"},
                                                    .uniform[2] = {0, GL_Type_Float3, "acceleration"},
                                                    .uniform[3] = {0, GL_Type_Float, "time"},
                                                    .uniform[4] = {0, GL_Type_Float, "alpha"},
                                                    .uniform[5] = {0, GL_Type_Float, "alphaVelocity"},
                                                    .uniform[6] = {0, GL_Type_Float4, "albedo"},
                                                    .uniform[7] = {0, GL_Type_Float4, "emit"},
                                                    .uniform[8] = {0, GL_Type_Float, "incandescnence"},
                                                    .uniform[9] = {0, GL_Type_Float, "incandescenceVelocity"},
                                                    .global[0] = {0, &dead_resource},
                                                    .global[1] = {0, &counter_resource},
                                                    .global[2] = {0, &data_resource},
                                                    .global[3] = {0, &alive_resource}};

// clang-format off
static char presimulate_compute_shader_source[] =
GL_MSTR(
  void main() {
    u_particle_indirect.simulate.num_groups_x = u_particle_counter.alive;
    u_particle_counter.alive = 0;
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
    uint index = u_particle_alive.item[gl_GlobalInvocationID.x];
    particle_Data data = particle_Data_unpack(u_particle_data.item[index]);
    float time = u_frame.time - data.origin_time.w;
    float alpha = data.velocity_alpha.w * 256 + data.acceleration_alphaVelocity.w * 256 * time;
    if(alpha > 0) {
      vec3 position = data.origin_time.xyz + data.velocity_alpha.xyz * 256 * time + data.acceleration_alphaVelocity.xyz * 256 * time * time;
      uint sorted_index = atomicAdd(u_particle_counter.alive, 1);
      u_particle_distance.item[sorted_index] = uint(length(position - u_frame.viewOrigin) * 1024);
      u_particle_sorted.item[sorted_index] = index;
    } else {
      uint dead_index = atomicAdd(u_particle_counter.dead, 1);
      u_particle_dead.item[dead_index] = index;
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
                                                 .global[5] = {0, &counter_resource},
                                                 .global[6] = {0, &u_frame}};

// clang-format off
static char postsimulate_compute_shader_source[] =
GL_MSTR(
  void main() {
    uint count = u_particle_counter.alive;
    u_particle_indirect.sort1.num_groups_x = count;
    u_particle_indirect.sort2.num_groups_x = count;
    u_particle_indirect.sort3.num_groups_x = count;
    u_particle_indirect.sort4.num_groups_x = count;
    u_particle_indirect.sort5.num_groups_x = count;
    u_particle_indirect.sort6.num_groups_x = count;
    u_particle_indirect.sort7.num_groups_x = count;
    u_particle_indirect.sort8.num_groups_x = count;
    u_particle_indirect.draw.count = count;

    /* make a buffer copy? */
    for(uint i = 0; i < count; i++)
      u_particle_alive.item[i] = u_particle_sorted.item[i];
  }
);
// clang-format on

struct GL_ComputeState postsimulate_compute_state = {
    .shader.source = postsimulate_compute_shader_source,
    .global[0] = {0, &indirect_resource},
    .global[1] = {0, &counter_resource},
    .global[2] = {0, &alive_resource},
    .global[3] = {0, &sorted_resource},
};

// clang-format off
static char vertex_shader_source[] =
GL_MSTR(
  layout(location = 0) out vec3 out_albedo;
  layout(location = 1) out vec3 out_emit;

  void main() {
    float time = u_time - in_time;
    vec3 position = in_origin + in_velocity * 256 * time + in_acceleration * 256 * time * time;
    float alpha = clamp(in_alpha * 256 + in_alpha_velocity * 256 * time, 0, 1);
    float incandescence = clamp(in_incandescence * 256 + in_incandescence_velocity * 256 * time, 0, 1);

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

void GL_AddParticle(float time, const vec3_t origin, const vec3_t velocity, const vec3_t acceleration, int albedo,
                    int emit, float alpha, float alphavel, float incandescence, float incandescencevel) {
  ensure_init();

  uint8_t albedo_rgba[4];
  uint8_t emit_rgba[4];

  *(uint32_t *)albedo_rgba = d_8to24table[albedo];
  *(uint32_t *)emit_rgba = d_8to24table[emit];

  struct GL_ComputeAssets assets;
  memset(&assets, 0, sizeof(assets));

  VectorCopy(origin, assets.uniforms[0].vec);
  VectorCopy(velocity, assets.uniforms[1].vec);
  VectorCopy(acceleration, assets.uniforms[2].vec);
  assets.uniforms[3]._float = time;
  assets.uniforms[4]._float = alpha;
  assets.uniforms[5]._float = alphavel;
  assets.uniforms[6].vec[0] = albedo_rgba[0] / 256.0f;
  assets.uniforms[6].vec[1] = albedo_rgba[1] / 256.0f;
  assets.uniforms[6].vec[2] = albedo_rgba[2] / 256.0f;
  assets.uniforms[6].vec[3] = 1;
  assets.uniforms[7].vec[0] = emit_rgba[0] / 256.0f;
  assets.uniforms[7].vec[1] = emit_rgba[1] / 256.0f;
  assets.uniforms[7].vec[2] = emit_rgba[2] / 256.0f;
  assets.uniforms[7].vec[3] = 1;
  assets.uniforms[8]._float = incandescence;
  assets.uniforms[9]._float = incandescencevel;

  GL_compute(&emit_single_compute_state, &assets, 1, 1, 1);
}

static struct GL_DrawState particle_draw_state = {
    .primitive = GL_POINTS,
    .attribute[0] = {0, alias_memory_Format_Float32, 3, "origin", offsetof(struct GL_particle_Data, origin_time)},
    .attribute[1] = {0, alias_memory_Format_Float32, 1, "time", offsetof(struct GL_particle_Data, origin_time) + 12},
    .attribute[2] = {0, alias_memory_Format_Snorm16, 3, "velocity", offsetof(struct GL_particle_Data, velocity_alpha)},
    .attribute[3] = {0, alias_memory_Format_Snorm16, 1, "alpha", offsetof(struct GL_particle_Data, velocity_alpha) + 6},
    .attribute[4] = {0, alias_memory_Format_Snorm16, 3, "acceleration",
                     offsetof(struct GL_particle_Data, acceleration_alphaVelocity)},
    .attribute[5] = {0, alias_memory_Format_Snorm16, 1, "alpha_velocity",
                     offsetof(struct GL_particle_Data, acceleration_alphaVelocity) + 6},
    .attribute[6] = {0, alias_memory_Format_Unorm8, 3, "albedo", offsetof(struct GL_particle_Data, albedo)},
    .attribute[7] = {0, alias_memory_Format_Unorm8, 3, "emit", offsetof(struct GL_particle_Data, emit)},
    .attribute[8] = {0, alias_memory_Format_Snorm16, 1, "incandescence",
                     offsetof(struct GL_particle_Data, incandescence_incandescenceVelocity)},
    .attribute[9] = {0, alias_memory_Format_Snorm16, 1, "incandescence_velocity",
                     offsetof(struct GL_particle_Data, incandescence_incandescenceVelocity) + 2},
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
                                 .vertex_buffers[0] = &data_buffer,
                                 .element_buffer = &sorted_buffer};

  glMemoryBarrier(GL_ELEMENT_ARRAY_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);

  GL_draw_elements_indirect(&particle_draw_state, &assets, &indirect_buffer,
                            offsetof(struct GL_particle_indirect, draw));
}
