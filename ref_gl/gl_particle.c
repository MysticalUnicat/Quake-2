#include "gl_local.h"

#include "gl_thin_cpp.h"
#include "gl_compute.h"

#define NUM_PARTICLES 1000000

THIN_GL_STRUCT(particle_Data, float32x4(origin_time), snorm16x4(velocity_alpha), snorm16x4(acceleration_alphaVelocity),
               unorm8x4(albedo), unorm8x4(emit), snorm16x2(incandescence_incandescenceVelocity))
THIN_GL_BLOCK(particle_data, require(particle_Data), unsized_array(struct(particle_DataPacked, item)))
static struct GL_Buffer data_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(struct GL_particle_Data) * NUM_PARTICLES};
static struct GL_ShaderResource data_resource = {.type = GL_Type_ShaderStorageBuffer,
                                                 .name = "particle_data",
                                                 .block.buffer = &data_buffer,
                                                 .block.snippet = &GL_particle_data_snippet};

THIN_GL_BLOCK(particle_distance, unsized_array(uint32(item)))
static struct GL_Buffer distance_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(uint32_t) * NUM_PARTICLES};
static struct GL_ShaderResource distance_resource = {.type = GL_Type_ShaderStorageBuffer,
                                                     .name = "particle_distance",
                                                     .block.buffer = &distance_buffer,
                                                     .block.snippet = &GL_particle_distance_snippet};

THIN_GL_BLOCK(particle_dead, unsized_array(uint32(item)))
static struct GL_Buffer dead_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(uint32_t) * NUM_PARTICLES};
static struct GL_ShaderResource dead_resource = {.type = GL_Type_ShaderStorageBuffer,
                                                 .name = "particle_dead",
                                                 .block.buffer = &dead_buffer,
                                                 .block.snippet = &GL_particle_dead_snippet};

THIN_GL_BLOCK(particle_alive, unsized_array(uint32(item)))
static struct GL_Buffer alive_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(uint32_t) * NUM_PARTICLES};
static struct GL_ShaderResource alive_resource = {.type = GL_Type_ShaderStorageBuffer,
                                                  .name = "particle_alive",
                                                  .block.buffer = &alive_buffer,
                                                  .block.snippet = &GL_particle_alive_snippet};

THIN_GL_BLOCK(particle_emitted, unsized_array(uint32(item)))
static struct GL_Buffer emitted_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(uint32_t) * NUM_PARTICLES};
static struct GL_ShaderResource emitted_resource = {.type = GL_Type_ShaderStorageBuffer,
                                                    .name = "particle_emitted",
                                                    .block.buffer = &emitted_buffer,
                                                    .block.snippet = &GL_particle_emitted_snippet};

THIN_GL_BLOCK(particle_counter, uint32(alive), int32(dead))
static struct GL_Buffer counter_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(struct GL_particle_counter)};
static struct GL_ShaderResource counter_resource = {.type = GL_Type_ShaderStorageBuffer,
                                                    .name = "particle_counter",
                                                    .block.buffer = &counter_buffer,
                                                    .block.snippet = &GL_particle_counter_snippet};

THIN_GL_BLOCK(particle_indirect, require(DispatchIndirectCommand), require(DrawElementsIndirectCommand),
              struct(DispatchIndirectCommand, simulate), struct(DrawElementsIndirectCommand, draw))
static struct GL_Buffer indirect_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(struct GL_particle_indirect)};
static struct GL_ShaderResource indirect_resource = {.type = GL_Type_ShaderStorageBuffer,
                                                     .name = "particle_indirect",
                                                     .block.buffer = &indirect_buffer,
                                                     .block.snippet = &GL_particle_indirect_snippet};

THIN_GL_BLOCK(sort_parameters, require(DispatchIndirectCommand), uint32(num_particles), uint32(size),
              struct(DispatchIndirectCommand, fill), struct(DispatchIndirectCommand, radix),
              struct(DispatchIndirectCommand, rotate), struct(DispatchIndirectCommand, sortedmatrix))
static struct GL_Buffer sort_parameters_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(struct GL_sort_parameters)};
static struct GL_ShaderResource sort_parameters_resource = {.type = GL_Type_ShaderStorageBuffer,
                                                            .name = "sort_parameters",
                                                            .block.buffer = &sort_parameters_buffer,
                                                            .block.snippet = &GL_sort_parameters_snippet};

// clang-format off
THIN_GL_SHADER(initialize, main(
  u_particle_counter.dead = int(u_count);

  u_particle_indirect.simulate.num_groups_x = 0;
  u_particle_indirect.simulate.num_groups_y = 1;
  u_particle_indirect.simulate.num_groups_z = 1;

  u_particle_indirect.draw.count = 0;
  u_particle_indirect.draw.instance_count = 1;
  u_particle_indirect.draw.first_index = 0;
  u_particle_indirect.draw.base_vertex = 0;
  u_particle_indirect.draw.base_instance = 0;

  for(uint i = 0; i < u_count; i++)
    u_particle_dead.item[i] = (u_count - 1) - i;
))
// clang-format on

static struct GL_ComputeState initialize_compute_state = {.shader = &initialize_shader,
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
THIN_GL_SNIPPET(emit_particle, require(particle_Data), code(
  void emit_particle(vec3 origin, vec3 velocity, vec3 acceleration, vec4 albedo, vec4 emit, float alpha, float alpha_velocity, float incandescence, float incandescence_velocity) {
    int dead_index = atomicAdd(u_particle_counter.dead, -1);
    atomicMax(u_particle_counter.dead, 0);

    if(dead_index > 0) {
      uint index = u_particle_dead.item[dead_index - 1];
      u_particle_data.item[index] = particle_Data_pack(particle_Data(
        vec4(origin, u_frame.time),
        vec4(velocity / 256, alpha / 256),
        vec4(acceleration / 256, alpha_velocity / 256),
        albedo,
        emit,
        vec2(incandescence / 256, incandescence_velocity / 256)
      ));
      u_particle_alive.item[atomicAdd(u_particle_counter.alive, 1)] = index;
    }
  }
))
// clang-format on

// clang-format off
THIN_GL_SHADER(emit_single, require(emit_particle), main(
  emit_particle(u_origin, u_velocity, u_acceleration, u_albedo, u_emit, u_alpha, u_alphaVelocity, u_incandescence, u_incandescenceVelocity);
))
// clang-format on

struct GL_ComputeState emit_single_compute_state = {.shader = &emit_single_shader,
                                                    .uniform[0] = {0, GL_Type_Float3, "origin"},
                                                    .uniform[1] = {0, GL_Type_Float3, "velocity"},
                                                    .uniform[2] = {0, GL_Type_Float3, "acceleration"},
                                                    .uniform[3] = {0, GL_Type_Float, "alpha"},
                                                    .uniform[4] = {0, GL_Type_Float, "alphaVelocity"},
                                                    .uniform[5] = {0, GL_Type_Float4, "albedo"},
                                                    .uniform[6] = {0, GL_Type_Float4, "emit"},
                                                    .uniform[7] = {0, GL_Type_Float, "incandescence"},
                                                    .uniform[8] = {0, GL_Type_Float, "incandescenceVelocity"},
                                                    .global[0] = {0, &dead_resource},
                                                    .global[1] = {0, &counter_resource},
                                                    .global[2] = {0, &data_resource},
                                                    .global[3] = {0, &alive_resource},
                                                    .global[4] = {0, &u_frame}};

// clang-format off
THIN_GL_SHADER(pre_simulate, main(
  u_particle_indirect.simulate.num_groups_x = u_particle_counter.alive;
  u_particle_counter.alive = 0;
))
// clang-format on

struct GL_ComputeState presimulate_compute_state = {
    .shader = &pre_simulate_shader, .global[0] = {0, &indirect_resource}, .global[1] = {0, &counter_resource}};

// clang-format off
THIN_GL_SHADER(simulate, main(
  uint index = u_particle_alive.item[gl_GlobalInvocationID.x];
  particle_Data data = particle_Data_unpack(u_particle_data.item[index]);
  float time = u_frame.time - data.origin_time.w;
  float alpha = data.velocity_alpha.w * 256 + data.acceleration_alphaVelocity.w * 256 * time;
  if(alpha > 0) {
    vec3 position = data.origin_time.xyz + data.velocity_alpha.xyz * 256 * time + data.acceleration_alphaVelocity.xyz * 256 * time * time;
    uint emitted_index = atomicAdd(u_particle_counter.alive, 1);
    u_particle_distance.item[emitted_index] = uint(length(position - u_frame.viewOrigin) * 1024);
    u_particle_emitted.item[emitted_index] = index;
  } else {
    uint dead_index = atomicAdd(u_particle_counter.dead, 1);
    u_particle_dead.item[dead_index] = index;
  }
))
// clang-format on

struct GL_ComputeState simulate_compute_state = {.shader = &simulate_shader,
                                                 .global[0] = {0, &alive_resource},
                                                 .global[1] = {0, &data_resource},
                                                 .global[2] = {0, &distance_resource},
                                                 .global[3] = {0, &emitted_resource},
                                                 .global[4] = {0, &dead_resource},
                                                 .global[5] = {0, &counter_resource},
                                                 .global[6] = {0, &u_frame}};

// clang-format off
THIN_GL_SHADER(post_simulate, main(
  uint count = u_particle_counter.alive;
  u_particle_indirect.draw.count = count;

  u_sort_particles.num_particles = num_particles;
  u_sort_particles.size = uint(ceil(sqrt(num_particles)));

  u_sort_particles.fill.num_local_x = u_sort_particles.size * u_sort_particles.size - num_particles;
  u_sort_particles.fill.num_local_y = 1
  u_sort_particles.fill.num_local_z = 1;

  u_sort_particles.radix.num_local_x = RADIXSORT_WORKGROUP_SIZE;
  u_sort_particles.radix.num_local_y = u_sort_particles.size;
  u_sort_particles.radix.num_local_z = 1;

  u_sort_particles.rotate.num_local_x = u_sort_particles.size;
  u_sort_particles.rotate.num_local_y = u_sort_particles.size;
  u_sort_particles.rotate.num_local_z = 1;

  u_sort_particles.sortedmatrix.num_local_x = u_sort_particles.size;
  u_sort_particles.sortedmatrix.num_local_y = u_sort_particles.size;
  u_sort_particles.sortedmatrix.num_local_z = 1;
))
// clang-format on

struct GL_ComputeState postsimulate_compute_state = {
    .shader = &post_simulate_shader,
    .global[0] = {0, &indirect_resource},
    .global[1] = {0, &counter_resource},
    .global[2] = {0, &alive_resource},
    .global[3] = {0, &emitted_resource},
};

// clang-format off
THIN_GL_SHADER(sort_particles_fill_pass,
  require(sort_key_uint),
  main(
    sort_store_key(0, u_sort_particles.num_particles + gl_LocalInvocationID.x, 0xFFFFFFFF);
  )
)
// clang-format on

static struct GL_ComputeState sort_particles_fill_pass_state = {
    .shader = &GL_sort_particles_fill_pass_state_shader, .local_group_x = 1, .local_group_y = 1, .local_group_z = 1};

// clang-format off
// RADIXSORT_WORKGROUP_SIZE,1,1 over RADIXSORT_WORKGROUP_SIZE,u_size,1
THIN_GL_SHADER(sort_particles_radix_pass_1,
  require(sort_key_uint),
  require(sort_value_uint),
  require(radixsort_key_value),
  main(
    uint buffer_offset = gl_GlobalInvocationID.y * u_sort_particles.size;
    uint buffer = 0;
    for(uint bit_offset = 0, ; bit_offset < u_num_bits; bit_offset += RADIXSORT_BITS_PER_PASS) {
      radixsort_key_value(buffer, buffer_offset, bit_offset, u_sort_particles.size);
      buffer = !buffer;
    }
  )
)
// clang-format on

// clang-format off
// 1,1,1 over u_size,u_size,1
THIN_GL_SHADER(sort_particles_rotate_pass,
  require(sort_key_uint),
  require(sort_value_uint),
  main(
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;
    uint src_index = y*u_sort_particles.size + x;
    uint dst_index = x*u_sort_particles.size + (u_sort_particles.size - 1 - y);
    sort_store_key(1, dst_index, sort_load_key(0, src_index));
    sort_store_value(1, dst_index, sort_load_value(0, src_index));
  )
)
// clang-format on

// clang-format off
// RADIXSORT_WORKGROUP_SIZE,1,1 over RADIXSORT_WORKGROUP_SIZE,u_size,1
THIN_GL_SHADER(sort_particles_radix_pass_2,
  require(sort_key_uint),
  require(sort_value_uint),
  require(radixsort_key_value),
  main(
    uint buffer_offset = gl_GlobalInvocationID.y * u_sort_particles.size;
    uint buffer = 1;
    for(uint bit_offset = 0, ; bit_offset < u_num_bits; bit_offset += RADIXSORT_BITS_PER_PASS) {
      radixsort_key_value(buffer, buffer_offset, bit_offset, u_sort_particles.size);
      buffer = !buffer;
    }
  )
)
// clang-format on

// clang-format off
// 1,1,1 over u_size,u_size,1
THIN_GL_SHADER(sort_particles_unrotate_pass,
  require(sort_key_uint),
  require(sort_value_uint),
  main(
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;
    uint src_index = x*u_sort_particles.size + (u_sort_particles.size - 1 - y);
    uint dst_index = y*u_sort_particles.size + x;
    sort_store_key(0, dst_index, sort_load_key(1, src_index));
    sort_store_value(0, dst_index, sort_load_value(1, src_index));
  )
)
// clang-format on

// clang-format off
// 1,1,1 over u_size,u_size,1
THIN_GL_SHADER(sort_particles_enumerate_pass,
  require(sort_key_uint),
  require(sort_value_uint),
  require(sortedmatrix_key_value)
  main(
    sortedmatrix_key_value(0, gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
  )
)
// clang-format on

static struct GL_ComputeState sort_particles_radix_pass_1_state = {.shader =
                                                                       &GL_sort_particles_radix_pass_1_state_shader,
                                                                   .local_group_x = RADIXSORT_WORKGROUP_SIZE,
                                                                   .local_group_y = 1,
                                                                   .local_group_z = 1};

static struct GL_ComputeState sort_particles_rotate_pass_state = {
    .shader = &GL_sort_particles_rotate_pass_shader, .local_group_x = 1, .local_group_y = 1, .local_group_z = 1};

static struct GL_ComputeState sort_particles_radix_pass_2_state = {.shader =
                                                                       &GL_sort_particles_radix_pass_2_state_shader,
                                                                   .local_group_x = RADIXSORT_WORKGROUP_SIZE,
                                                                   .local_group_y = 1,
                                                                   .local_group_z = 1};

static struct GL_ComputeState sort_particles_unrotate_pass_state = {
    .shader = &GL_sort_particles_unrotate_pass_shader, .local_group_x = 1, .local_group_y = 1, .local_group_z = 1};

static struct GL_ComputeState sort_particles_sortedmatrix_pass_state = {
    .shader = &GL_sort_particles_sortedmatrix_pass_shader, .local_group_x = 1, .local_group_y = 1, .local_group_z = 1};

// clang-format off
THIN_GL_SHADER(vertex,
  code(
    layout(location = 0) out vec3 out_albedo;
    layout(location = 1) out vec3 out_emit;
  ),
  main(
    float time = u_time - in_time;
    vec3 position = in_origin + in_velocity * 256 * time + in_acceleration * 256 * time * time;
    float alpha = clamp(in_alpha * 256 + in_alpha_velocity * 256 * time, 0, 1);
    float incandescence = clamp((in_incandescence * 256 + in_incandescence_velocity * 256 * time) * alpha, 0, 1);

    gl_Position = u_view_projection_matrix * vec4(position, 1);
    float size = 40;
    float size_min = 2;
    float size_max = 40;
    float a = 0.01;
    float b = 0;
    float c = 0.01;
    float dist = length(u_view_matrix * vec4(position, 1));
    float dist_atten = 1 / (a + b * dist + c * dist * dist);
    float derived_size = size * sqrt(dist_atten);
    float clamped_size = clamp(derived_size, size_min, size_max);
    gl_PointSize = mix(1, clamped_size, clamp(time * 2, 0, 1));
    out_albedo = mix(vec3(1 - alpha), in_albedo, sin(alpha * 3.14));
    // TODO: query light space to add to light emitted (multiplied by in_albedo.rgb)
    //       right now, it is being multiplied by in_albedo.a
    out_emit = in_albedo * alpha * 0.25 + in_emit * incandescence;
  )
)

THIN_GL_SHADER(fragment,
  code(
    layout(location = 0) in vec3 in_albedo;
    layout(location = 1) in vec3 in_emit;

    layout(location = 0, index = 0) out vec4 out_emit;
    layout(location = 0, index = 1) out vec4 out_absorb;
  ),
  main(
    out_absorb = vec4(in_albedo, 1);
    out_emit = vec4(in_emit, 1);
  )
)
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
  assets.uniforms[3]._float = alpha;
  assets.uniforms[4]._float = alphavel;
  assets.uniforms[5].vec[0] = albedo_rgba[0] / 256.0f;
  assets.uniforms[5].vec[1] = albedo_rgba[1] / 256.0f;
  assets.uniforms[5].vec[2] = albedo_rgba[2] / 256.0f;
  assets.uniforms[5].vec[3] = 1;
  assets.uniforms[6].vec[0] = emit_rgba[0] / 256.0f;
  assets.uniforms[6].vec[1] = emit_rgba[1] / 256.0f;
  assets.uniforms[6].vec[2] = emit_rgba[2] / 256.0f;
  assets.uniforms[6].vec[3] = 1;
  assets.uniforms[7]._float = incandescence;
  assets.uniforms[8]._float = incandescencevel;

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
    .vertex_shader = &vertex_shader,
    .fragment_shader = &fragment_shader,
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

  void GL_sort_particles(const struct GL_Buffer *parameters, const const struct GL_Buffer *source_keys,
                         const struct GL_Buffer *source_values, const struct GL_Buffer *destination_keys,
                         const struct GL_Buffer *destination_values) {
    struct GL_ComputeAssets assets = {
        .uniforms[0].buffer[0] = parameters,
        .uniforms[1].buffer[0] = source_keys,
        .uniforms[1].buffer[1] = source_values,
        .uniforms[2].buffer[0] = destination_keys,
        .uniforms[2].buffer[1] = destination_values,
    };

    GL_compute_indirect(&sort_particles_fill_pass_state, &assets, &parameters,
                        offsetof(GL_sort_particles_Parameters, fill));

    GL_compute_indirect(&sort_particles_radix_pass_1_state, &assets, &parameters,
                        offsetof(GL_sort_particles_Parameters, radix));

    GL_compute_indirect(&sort_particles_rotate_state, &assets, &parameters,
                        offsetof(GL_sort_particles_Parameters, rotate));

    GL_compute_indirect(&sort_particles_radix_pass_2_state, &assets, &parameters,
                        offsetof(GL_sort_particles_Parameters, radix));

    GL_compute_indirect(&sort_particles_unrotate_state, &assets, &parameters,
                        offsetof(GL_sort_particles_Parameters, rotate));

    GL_compute_indirect(&sort_particles_sortedmatrix_pass_state, &assets, &parameters,
                        offsetof(GL_sort_particles_Parameters, sortedmatrix));
  }

  // GL_parallelSort_key_value_pairs(&parallelSort_scratch, &distance_buffer, &emitted_buffer);

  struct GL_DrawAssets assets = {.uniforms[0] = {.vec[0] = gl_particle_size->value,
                                                 .vec[1] = gl_particle_min_size->value,
                                                 .vec[2] = gl_particle_max_size->value},
                                 .uniforms[1] = {.vec[0] = gl_particle_att_a->value,
                                                 .vec[1] = gl_particle_att_b->value,
                                                 .vec[2] = gl_particle_att_c->value},
                                 .vertex_buffers[0] = &data_buffer,
                                 .element_buffer = &alive_buffer};

  glMemoryBarrier(GL_ELEMENT_ARRAY_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);

  GL_draw_elements_indirect(&particle_draw_state, &assets, &indirect_buffer,
                            offsetof(struct GL_particle_indirect, draw));
}
