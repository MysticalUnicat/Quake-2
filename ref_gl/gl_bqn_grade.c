#include "gl_bqn.h"

// https://mlochbaum.github.io/BQN/doc/order.html#grade

#define BITS_PER_PASS THIN_GL_BQN_GRADE_BITS_PER_PASS
#define ELEMENTS_PER_THREAD THIN_GL_BQN_GRADE_ELEMENTS_PER_THREAD
#define THREADGROUP_SIZE THIN_GL_BQN_GRADE_THREADGROUP_SIZE
#define BIN_COUNT (1 << BITS_PER_PASS)
#define BLOCK_SIZE (THREADGROUP_SIZE * ELEMENTS_PER_THREAD)

#define TO_STR(X) TO_STR_(X)
#define TO_STR_(X) #X

// clang-format off
THIN_GL_IMPL_STRUCT(BQN_Grade_Parameters, uint32(num_keys), uint32(num_blocks))

THIN_GL_IMPL_SNIPPET(BQN_grade,
  require(BQN_Grade_Parameters),
  string(
    "#define GRADE_BITS_PER_PASS " TO_STR(BITS_PER_PASS) "\n"
    "#define GRADE_ELEMENTS_PER_THREAD " TO_STR(ELEMENTS_PER_THREAD) "\n"
    "#define GRADE_THREADGROUP_SIZE " TO_STR(THREADGROUP_SIZE) "\n"
    "#define GRADE_BIN_COUNT (1 << GRADE_BITS_PER_PASS)\n"
    "#define GRADE_BLOCK_SIZE (GRADE_THREADGROUP_SIZE * GRADE_ELEMENTS_PER_THREAD)\n"
  ),
  code(
    void BQN_Grade_Parameters_initialize(uint num_keys) {
      u_grade_parameters.num_keys = num_keys;
      u_grade_parameters.num_blocks = (num_keys + GRADE_BLOCK_SIZE - 1) / GRADE_BLOCK_SIZE;
    }
  )
)

THIN_GL_SHADER(BQN_grade_prepare, require(grade_constants), main(
  u_indirect.local_group_x = u_grade_parameters.num_blocks;
  u_indirect.local_group_y = 1;
  u_indirect.local_group_z = 1;
  for(uint i = 0; i < u_grade_parameters.num_keys; i++)
    u_input.item[i] = i;
))

THIN_GL_SHADER(BQN_grade_count,
  require(grade_constants),
  main(
    uint block_index = gl_GlobalInvocationID.x;

    for(uint i = 0; i < BIN_COUNT; i++)
        u_counts.item[block_index*BIN_COUNT + i] = 0;

    for(uint i = 0; i < BLOCK_SIZE; i++) {
        uint old_index = block_index * BLOCK_SIZE + i;
        if(old_index < len(u_keys.item)) {
            uint key = u_keys.item[u_input.item[old_index]];
            key = (key >> u_bit_offset) & (BIN_COUNT - 1);
            u_input.item[old_index] |= key << (32 - BITS_PER_PASS);
            u_counts.item[block_index*BIN_COUNT + key]++;
        }
    }
  )
)

THIN_GL_SHADER(BQN_grade_accumulate,
  require(grade_constants),
  main(
    float sum = 0;
    for(uint bin_index = 0; bin_index < BIN_COUNT; i++) {
        for(uint block_index = 0; block_index < u_num_blocks; block_index++) {
            uint v = u_counts.item[block_index*BLOCK_SIZE + bin_index];
            u_counts.item[block_index*BLOCK_SIZE + bin_index] = sum;
            sum += v;
        }
    }
  )
)

THIN_GL_SHADER(BQN_grade_distribute,
  require(grade_constants),
  main(
    uint block_index = gl_GlobalInvocationID.x;

    for(uint i = 0; i < BLOCK_SIZE; i++) {
      uint old_index = block_index * BLOCK_SIZE + i;
      if(old_index < u_num_keys) {
        uint key = u_input.item[old_index] >> (32 - BITS_PER_PASS);
        uint new_index = atomicAdd(u_counts.item[block_index*BLOCK_SIZE + key], 1);
        u_output.item[new_index] = u_input.item[old_index] & ((1 << (32 - BITS_PER_PASS)) - 1);
      }
    }
  )
)
// clang-format on

static struct GL_Buffer indirect_buffer;

void GL_bqn_grade_nbits(const struct GL_Buffer *parameters, const struct GL_Buffer *input,
                        const struct GL_Buffer *middle, const struct GL_Buffer *counts, const struct GL_Buffer *output,
                        uint32_t num_bits) {
  uint32_t swap = (num_bits + BITS_PER_PASS - 1) / BITS_PER_PASS;

  GL_compute(&grade_prepare_compute_state,
             &(struct GL_ComputeAssets){.buffers[0] = parameters, .buffers[1] = (swap & 1) ? middle : output}, 1, 1, 1);

  for(uint32_t bit_offset = 0; bit_offset < num_bits; bit_offset += BITS_PER_PASS, swap++) {
    GL_compute_indirect(&grade_count_compute_state,
                        &(struct GL_ComputeAssets){
                            .uniforms[0]._int = bit_offset,
                            .buffers[0] = parameters,
                            .buffers[1] = (swap & 1) ? middle : output,
                            .buffers[2] = (swap & 1) ? output : middle,
                        },
                        &indirect_buffer, 0);

    GL_compute(&grade_accumulate_compute_state, &(struct GL_ComputeAssets){.buffers[0] = parameters}, 1, 1, 1);

    GL_compute_indirect(&grade_distribute_compute_state,
                        &(struct GL_ComputeAssets){
                            .buffers[0] = parameters,
                            .buffers[1] = (swap & 1) ? middle : output,
                            .buffers[2] = (swap & 1) ? output : middle,
                        },
                        &indirect_buffer, 0);
  }
}

void GL_bqn_grade(const struct GL_Buffer *parameters, const struct GL_Buffer *input, const struct GL_Buffer *middle,
                  const struct GL_Buffer *counts, const struct GL_Buffer *output) {
  GL_bqn_grade_nbits(parameters, input, middle, counts, output, 32);
}
