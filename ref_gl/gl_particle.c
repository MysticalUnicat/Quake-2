#include "gl_local.h"

#include "gl_thin_cpp.h"
#include "gl_compute.h"

#define NUM_PARTICLES 1000000
#define PARTICLE_WORKGROUP_SIZE 128

THIN_GL_STRUCT(particle_Data, float32x4(origin_time), snorm16x4(velocity_alpha), snorm16x4(acceleration_alphaVelocity),
               unorm8x4(albedo), unorm8x4(emit), snorm16x2(incandescence_incandescenceVelocity),
               snorm16x2(size_sizeVelocity))
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
static struct GL_Buffer distance2_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(uint32_t) * NUM_PARTICLES};

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

THIN_GL_BLOCK(particle_counter, uint32(alive), int32(dead), uint32(simulate))
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

// THIN_GL_STRUCT(particle_EmitParameters, float32x4(velocity_x), float32x4(velocity_y), float32x4(velocity_z),
//                float32x4(albedo_r), float32x4(albedo_g), float32x4(albedo_b), float32x4(emit_r), float32x4(emit_g),
//                float32x4(emit_b), float32x2(alpha), float32x2(alpha_velocity), float32x2(incandescence),
//                float32x2(incandescence_velocity), float32x2(size), float32x2(size_velocity), float(weight))

THIN_GL_BLOCK(sort_parameters, require(DispatchIndirectCommand), uint32(num_particles), uint32(width), uint32(height),
              struct(DispatchIndirectCommand, fill), struct(DispatchIndirectCommand, radix_1),
              struct(DispatchIndirectCommand, rotate), struct(DispatchIndirectCommand, radix_2),
              struct(DispatchIndirectCommand, sortedmatrix))
static struct GL_Buffer sort_parameters_buffer = {.kind = GL_Buffer_GPU, .size = sizeof(struct GL_sort_parameters)};
static struct GL_ShaderResource sort_parameters_resource = {.type = GL_Type_ShaderStorageBuffer,
                                                            .name = "sort_parameters",
                                                            .block.buffer = &sort_parameters_buffer,
                                                            .block.snippet = &GL_sort_parameters_snippet};

THIN_GL_BLOCK(particle_key, unsized_array(uint32(item)))
static struct GL_ShaderResource sort_key_resource = {.type = GL_Type_ShaderStorageBuffer,
                                                     .name = "sort_keys",
                                                     .count = 2,
                                                     .block.buffers[0] = &distance_buffer,
                                                     .block.buffers[1] = &distance2_buffer,
                                                     .block.snippet = &GL_particle_key_snippet};

THIN_GL_BLOCK(particle_value, unsized_array(uint32(item)))
static struct GL_ShaderResource sort_value_resource = {.type = GL_Type_ShaderStorageBuffer,
                                                       .name = "sort_values",
                                                       .count = 2,
                                                       .block.buffers[0] = &emitted_buffer,
                                                       .block.buffers[1] = &alive_buffer,
                                                       .block.snippet = &GL_particle_value_snippet};

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

#if 0
// done on subgroup size
//
// treat the emit upper as target indexes into workgroup_scratch_uint that are scanned over
// on those we add one
// for example if we have a count of `1 2 3 4` we expect `0 1 1 2 2 2 3 3 3 3` back
// assuming a workgroup size of 4, we expect to see `0 1 1 2` first, then `2 3 3 3`, then `3`
// from the count we get were each section ends `1 3 6 10`
// then add to a scratch a 1 where each section ends (if within this group)
//
// clang-format off
THIN_GL_SHADER(emit_on_mesh,
  require(random_squares32),
  require(emit_particle),
  require(particle_EmitParameters),
  code(
    shared uint s_scratch_uint[gl_WorkGroupSize.x];
  ),
  main(
    // buffer Positions { vec3 position[]; } u_positions
    // buffer Triangles { uvec3 triangle[]; } u_triangles;
    // uniform float weight;

    uint num_positions = len(u_positions.position);
    uint num_triangles = len(u_triangles.triangle);

    random_init(u_frame.index, u_particle_counter.dead, gl_SubgroupInvocationID);

    // work in blocks to keep all invocations active
    uint num_blocks = (num_triangles + gl_SubgroupSize - 1) / gl_SubgroupSize;
    for(uint block = 0; block < num_blocks; block++) {
      uint base_triangle_index = block * gl_SubgroupSize;

      //  load one workgroup worth of indices, instead of being inactive invocations at the
      // end of the list get degenerate triangles and always emit 0 particles.
      uint triangle_index = base_triangle_index + gl_SubgroupInvocationID;
      uvec3 indices = triangle_index < num_triangles ? u_triangles.triangle[triangle_index] : uvec3(0);
      subgroupMemoryBarrierBuffer();

      // random load :(
      vec3 a = u_positions.position[indices.x];
      vec3 b = u_positions.position[indices.y];
      vec3 c = u_positions.position[indices.z];
      subgroupMemoryBarrierBuffer();

      vec3 ab = b - a;
      vec3 ac = c - a;
      vec3 cross = cross_product(ab, ac);
      vec3 normal = normalize(cross);

      float area = length(cross) * 0.5;

      uint count = uint(float_random_u() * area * weight);

      uint upper = subgroupInclusiveAdd(count);
      uint total = subgroupBroadcast(upper, gl_SubgroupSize - 1);

      uint num_indices_blocks = (total + gl_SubgoupSize - 1) / gl_SubgroupSize;
      for(uint indices_block = 0; indices_block < num_indices_blocks; indices_block++) {
        uint offset = indices_block * gl_SubgroupSize;

        s_scratch_uint[gl_SubgroupInvocationID] = 0;
        subgroupMemoryBarrierShared();

        uint target = upper;
        if(target >= offset) {
          target -= offset;
          if(target < gl_WorkGroupSize.x) {
            atomicAdd(s_scratch_uint[target], 1);
          }
        } else {
          atomicAdd(s_scratch_uint[0], 1);
        }
        subgroupBarrier();

        uint bits = s_scratch_uint[gl_SubgroupInvocationID];
        uint subgroup_triangle_index = subgroupInclusiveAdd(bits);

        // use `subgroupShuffle` to not load positions again
        vec3 emit_a = subgroupShuffle(a, subgroup_triangle_index);
        vec3 emit_b = subgroupShuffle(b, subgroup_triangle_index);
        vec3 emit_c = subgroupShuffle(c, subgroup_triangle_index);

        if(base_triangle_index + subgroup_triangle_index < num_triangles) {
          float p = float_random_u();
          float q = float_random_u();
          if(p + q > 1) {
            p = 1 - p;
            q = 1 - q;
          }

          vec3 emit_ab = emit_b - emit_a;
          vec3 emit_ac = emit_c - emit_a;
          vec3 emit_cross = cross_product(emit_ab, emit_ac);
          vec3 emit_normal = normalize(emit_cross);
          vec3 emit_position = emit_a + emit_ab*p + emit_ac*q;


        }
      }
    }
  )
)
// clang-format on
#endif

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
  // u_particle_indirect.simulate.num_groups_x = u_particle_counter.alive;
  u_particle_indirect.simulate.num_groups_x = uint(u_particle_counter.alive > 1);
  u_particle_counter.simulate = u_particle_counter.alive;
  u_particle_counter.alive = 0;
))
// clang-format on

struct GL_ComputeState presimulate_compute_state = {
    .shader = &pre_simulate_shader, .global[0] = {0, &indirect_resource}, .global[1] = {0, &counter_resource}};

// clang-format off
THIN_GL_SHADER(simulate, main(
  uint num_simulate = u_particle_counter.simulate;

  for(uint i = gl_LocalInvocationID.x; i < num_simulate; i += gl_WorkGroupSize.x) {
    uint index = u_particle_alive.item[i];

    particle_Data data = particle_Data_unpack(u_particle_data.item[index]);

    float time = u_frame.time - data.origin_time.w;
    float alpha = data.velocity_alpha.w * 256 + data.acceleration_alphaVelocity.w * 256 * time;
    vec3 position = data.origin_time.xyz + data.velocity_alpha.xyz * 256 * time + data.acceleration_alphaVelocity.xyz * 256 * time * time;
    uint distance = uint(length(position - u_frame.viewOrigin) * 4096);

    if(alpha > 0) {
      uint emitted_index = atomicAdd(u_particle_counter.alive, 1);
      u_particle_distance.item[emitted_index] = distance;
      u_particle_emitted.item[emitted_index] = index;
    } else {
      uint dead_index = atomicAdd(u_particle_counter.dead, 1);
      u_particle_dead.item[dead_index] = index;
    }
  }
))
// clang-format on

struct GL_ComputeState simulate_compute_state = {.shader = &simulate_shader,
                                                 .local_group_x = PARTICLE_WORKGROUP_SIZE,
                                                 .global[0] = {0, &alive_resource},
                                                 .global[1] = {0, &data_resource},
                                                 .global[2] = {0, &distance_resource},
                                                 .global[3] = {0, &emitted_resource},
                                                 .global[4] = {0, &dead_resource},
                                                 .global[5] = {0, &counter_resource},
                                                 .global[6] = {0, &u_frame}};

// clang-format off
THIN_GL_SHADER(post_simulate,
  require(radixsort_defines),
  main(
    uint count = u_particle_counter.alive;
    u_particle_indirect.draw.count = count;

    u_sort_parameters.num_particles = count;

    u_sort_parameters.width = uint(ceil(sqrt(count)));
    u_sort_parameters.height = uint(ceil((count + u_sort_parameters.width - 1) / u_sort_parameters.width));

    u_sort_parameters.fill.num_groups_x = 1;
    u_sort_parameters.fill.num_groups_y = 1;
    u_sort_parameters.fill.num_groups_z = 1;

    u_sort_parameters.radix_1.num_groups_x = 1;
    u_sort_parameters.radix_1.num_groups_y = u_sort_parameters.height;
    u_sort_parameters.radix_1.num_groups_z = 1;

    u_sort_parameters.rotate.num_groups_x = 1;
    u_sort_parameters.rotate.num_groups_y = u_sort_parameters.height;
    u_sort_parameters.rotate.num_groups_z = 1;

    u_sort_parameters.radix_2.num_groups_x = 1;
    u_sort_parameters.radix_2.num_groups_y = u_sort_parameters.width;
    u_sort_parameters.radix_2.num_groups_z = 1;

    u_sort_parameters.sortedmatrix.num_groups_x = u_sort_parameters.width;
    u_sort_parameters.sortedmatrix.num_groups_y = u_sort_parameters.height;
    u_sort_parameters.sortedmatrix.num_groups_z = 1;
  )
)
// clang-format on

struct GL_ComputeState postsimulate_compute_state = {.shader = &post_simulate_shader,
                                                     .global[0] = {0, &indirect_resource},
                                                     .global[1] = {0, &counter_resource},
                                                     .global[2] = {0, &alive_resource},
                                                     .global[3] = {0, &emitted_resource},
                                                     .global[4] = {0, &sort_parameters_resource}};

// clang-format off
THIN_GL_SHADER(sort_particles_fill_pass,
  require(sort_key_uint),
  require(sort_key),
  main(
    uint end = u_sort_parameters.width * u_sort_parameters.height;
    for(uint i = u_sort_parameters.num_particles + gl_LocalInvocationID.x; i < end; i++) {
      sort_store_key(0, i, 0xFFFFFFFF);
      sort_store_key(1, i, 0xFFFFFFFF);
    }
  )
)
// clang-format on

static struct GL_ComputeState sort_particles_fill_pass_state = {.shader = &sort_particles_fill_pass_shader,
                                                                .local_group_x = RADIXSORT_WORKGROUP_SIZE,
                                                                .local_group_y = 1,
                                                                .local_group_z = 1,
                                                                .global[0] = {0, &sort_parameters_resource},
                                                                .global[1] = {0, &sort_key_resource},
                                                                .global[2] = {0, &sort_value_resource}};

// clang-format off
THIN_GL_SHADER(sort_particles_radix_pass_1,
  require(sort_key_uint),
  require(sort_value_uint),
  require(sort_ascending),
  require(radixsort_key_value),
  main(
    uint buffer_offset = gl_GlobalInvocationID.y * u_sort_parameters.width;
    uint buffer_length = u_sort_parameters.width;
    uint buffer_end = buffer_offset + buffer_length;

    uint buf = 0;
    for(uint bit_offset = 0; bit_offset < 32; bit_offset += RADIXSORT_BITS_PER_PASS) {
      buf ^= uint(radixsort_key_value(buf, buffer_offset, bit_offset, buffer_length));
    }
    barrier();

    // make sure most recently sorted data is placed in buffers 1
    if(buf != 1) {
      for(uint index = buffer_offset + gl_LocalInvocationID.x; index < buffer_end; index += gl_WorkGroupSize.x) {
        uint key = sort_load_key(0, index);
        uint value = sort_load_value(0, index);

        sort_store_key(1, index, key);
        sort_store_value(1, index, value);
      }
    }
  )
)
// clang-format on

static struct GL_ComputeState sort_particles_radix_pass_1_state = {.shader = &sort_particles_radix_pass_1_shader,
                                                                   .local_group_x = RADIXSORT_WORKGROUP_SIZE,
                                                                   .local_group_y = 1,
                                                                   .local_group_z = 1,
                                                                   .global[0] = {0, &sort_parameters_resource},
                                                                   .global[1] = {0, &sort_key_resource},
                                                                   .global[2] = {0, &sort_value_resource}};

// clang-format off
THIN_GL_SHADER(sort_particles_rotate_pass,
  require(sort_key_uint),
  require(sort_value_uint),
  require(sort_key_xy),
  require(sort_value_xy),
  main(
    sort_key_xy_setup(0, octic_group_r3, u_sort_parameters.width, u_sort_parameters.height);
    sort_value_xy_setup(0, octic_group_r3, u_sort_parameters.width, u_sort_parameters.height);

    uint y = gl_GlobalInvocationID.y;
    uint buffer_offset = y * u_sort_parameters.width;

    for(uint x = gl_LocalInvocationID.x; x < u_sort_parameters.width; x += RADIXSORT_WORKGROUP_SIZE) {
      uint src_index = buffer_offset + x;

      SORT_KEY_TYPE key = sort_load_key(1, src_index);
      SORT_VALUE_TYPE value = sort_load_value(1, src_index);

      sort_store_key_xy(0, x, y, key);
      sort_store_value_xy(0, x, y, value);
    }
  )
)
// clang-format on

static struct GL_ComputeState sort_particles_rotate_pass_state = {.shader = &sort_particles_rotate_pass_shader,
                                                                  .local_group_x = RADIXSORT_WORKGROUP_SIZE,
                                                                  .local_group_y = 1,
                                                                  .local_group_z = 1,
                                                                  .global[0] = {0, &sort_parameters_resource},
                                                                  .global[1] = {0, &sort_key_resource},
                                                                  .global[2] = {0, &sort_value_resource}};

// clang-format off
THIN_GL_SHADER(sort_particles_radix_pass_2,
  require(sort_key_uint),
  require(sort_value_uint),
  require(sort_ascending),
  require(radixsort_key_value),
  main(
    uint buffer_offset = gl_GlobalInvocationID.y * u_sort_parameters.height;
    uint buffer_length = u_sort_parameters.height;
    uint buffer_end = buffer_offset + buffer_length;

    uint buf = 0;
    for(uint bit_offset = 0; bit_offset < 32; bit_offset += RADIXSORT_BITS_PER_PASS) {
      buf ^= uint(radixsort_key_value(buf, buffer_offset, bit_offset, buffer_length));
    }

    // make sure most recently sorted data is placed in buffers 0
    if(buf == 1) {
      for(uint index = buffer_offset + gl_LocalInvocationID.x; index < buffer_end; index += gl_WorkGroupSize.x) {
        uint key = sort_load_key(1, index);
        uint value = sort_load_value(1, index);

        sort_store_key(0, index, key);
        sort_store_value(0, index, value);
      }
    }
  )
)
// clang-format on

static struct GL_ComputeState sort_particles_radix_pass_2_state = {.shader = &sort_particles_radix_pass_2_shader,
                                                                   .local_group_x = RADIXSORT_WORKGROUP_SIZE,
                                                                   .local_group_y = 1,
                                                                   .local_group_z = 1,
                                                                   .global[0] = {0, &sort_parameters_resource},
                                                                   .global[1] = {0, &sort_key_resource},
                                                                   .global[2] = {0, &sort_value_resource}};

// clang-format off
// 1,1,1 over u_size,u_size,1
THIN_GL_SHADER(sort_particles_sortedmatrix_pass,
  require(sort_key_uint),
  require(sort_value_uint),
  require(sort_descending),
  require(sortedmatrix_key_value),
  main(
    sort_key_xy_setup(0, octic_group_Tac, u_sort_parameters.width, u_sort_parameters.height);
    sort_value_xy_setup(0, octic_group_Tac, u_sort_parameters.width, u_sort_parameters.height);

    sortedmatrix_key_value_drop_key(0, u_sort_parameters.width, u_sort_parameters.height, u_sort_parameters.num_particles, gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
  )
)
// clang-format on

static struct GL_ComputeState sort_particles_sortedmatrix_pass_state = {.shader =
                                                                            &sort_particles_sortedmatrix_pass_shader,
                                                                        .local_group_x = 1,
                                                                        .local_group_y = 1,
                                                                        .local_group_z = 1,
                                                                        .global[0] = {0, &sort_parameters_resource},
                                                                        .global[1] = {0, &sort_key_resource},
                                                                        .global[2] = {0, &sort_value_resource}};

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
    float size_max = 80;
    float a = 0.01;
    float b = 0;
    float c = 0.001;
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

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
  GL_compute_indirect(&sort_particles_fill_pass_state, NULL, &sort_parameters_buffer,
                      offsetof(struct GL_sort_parameters, fill));

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  GL_compute_indirect(&sort_particles_radix_pass_1_state, NULL, &sort_parameters_buffer,
                      offsetof(struct GL_sort_parameters, radix_1));

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  GL_compute_indirect(&sort_particles_rotate_pass_state, NULL, &sort_parameters_buffer,
                      offsetof(struct GL_sort_parameters, rotate));

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  GL_compute_indirect(&sort_particles_radix_pass_2_state, NULL, &sort_parameters_buffer,
                      offsetof(struct GL_sort_parameters, radix_2));

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  GL_compute_indirect(&sort_particles_sortedmatrix_pass_state, NULL, &sort_parameters_buffer,
                      offsetof(struct GL_sort_parameters, sortedmatrix));

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
