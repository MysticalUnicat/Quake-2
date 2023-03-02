#ifndef __GL_THIN_BQN_H__
#define __GL_THIN_BQN_H__

#include "gl_thin_cpp.h"

// https://mlochbaum.github.io/BQN/doc/primitive.html

#define THIN_GL_BQN_GRADE_BITS_PER_PASS 4
#define THIN_GL_BQN_GRADE_ELEMENTS_PER_THREAD 1
#define THIN_GL_BQN_GRADE_THREADGROUP_SIZE 128

THIN_GL_DECLARE_STRUCT(BQN_Grade_Parameters, require(DispatchIndirectCommand),
                       struct(DispatchIndirectCommand, indirect), uint32(num_keys), uint32(num_blocks))
THIN_GL_DECLARE_SNIPPET(BQN_grade)

THIN_GL_DECLARE_STRUCT(BQN_Select_Parameters, require(DispatchIndirectCommand),
                       struct(DispatchIndirectCommand, indirect))
THIN_GL_DECLARE_SNIPPET(BQN_select)

#define THIN_GL_BQN_GRADE_COUNTS_SIZE(MAX_NUM_KEYS)                                                                    \
  (sizeof(uint32_t) *                                                                                                  \
   (MAX_NUM_KEYS + (THIN_GL_BQN_GRADE_THREADGROUP_SIZE * THIN_GL_BQN_GRADE_ELEMENTS_PER_THREAD) - 1) /                 \
   (THIN_GL_BQN_GRADE_THREADGROUP_SIZE * THIN_GL_BQN_GRADE_ELEMENTS_PER_THREAD))

void GL_bqn_grade_nbits(const struct GL_Buffer *parameters, const struct GL_Buffer *input,
                        const struct GL_Buffer *middle, const struct GL_Buffer *counts, const struct GL_Buffer *output,
                        uint32_t num_bits);

void GL_bqn_grade(const struct GL_Buffer *parameters, const struct GL_Buffer *input, const struct GL_Buffer *middle,
                  const struct GL_Buffer *counts, const struct GL_Buffer *output);

void GL_bqn_select(const struct GL_Buffer *parameters, const struct GL_Buffer *haystack,
                   const struct GL_Buffer *indexes, const struct GL_Buffer *output);

#endif

#if 0

https://www.sciencedirect.com/science/article/pii/S1877050923001461?ref=cra_js_challenge&fr=RR-1

Using Parallel Enumeration sort principles

size = ceil(sqrt(num_elements));

input: float[]
output: uint[]
temporary: none

for row_index {

}

for column {
    sort col
}

for p,q in element {
    less_pq = p * q - 1
    X = input[p,q]
    while p >= 1 and q <= size {
        if X < input[p-1,q+1] {
            p--
        } else {
            less_pq += p - 1
            q++
        }
    }

    while p <= size and q >= 1 {
        if X < a[p+1,q-1] {
            q--
        } else {
            less_pq += q - 1
            p++
        }
    }

    output[less_pq + 1] = input[p,q]
}

#endif
#if 0
#define QUICKSORT_KEY_VALUE_WORKGROUP_SIZE 32

// clang-format off
THIN_GL_SNIPPET(workgroup_sum,
  code(
    shared uint workgroup_sum_tile[gl_WorkGroupSize.x * 2];

    uint workgroup_sum(uint value) {
      uint index_0 = gl_LocalInvocationID.x;
      uint index_1 = index_0 + gl_WorkGroupSize.x;

      threadgroup_sum_tile[index_0] = 0;
      threadgroup_sum_tile[index_1] = value;
      threadgroup_sum_tile[index_1] += threadgroup_sum_tile[index - 1];
      threadgroup_sum_tile[index_1] += threadgroup_sum_tile[index - 2];
      threadgroup_sum_tile[index_1] += threadgroup_sum_tile[index - 4];
      threadgroup_sum_tile[index_1] += threadgroup_sum_tile[index - 8];
      threadgroup_sum_tile[index_1] += threadgroup_sum_tile[index - 16];
      uint warp_result = threadgroup_sum_tile[index_1] - value;
      barrier();

      if(index == gl_WorkGroupSize.x - 1) {
        workgroup_sum_tile[0] = warp_result + value;
      }
      barrier();

      return result + workgroup_sum_tile[0] + value;
    }
  )
)

THIN_GL_SNIPPET(sort_key_float,
  string(
    "#define SORT_KEY_TYPE float\n"
    "#define SORT_KEY_VEC2_TYPE vec2\n"
    "#define SORT_KEY_VEC3_TYPE vec3\n"
    "#define SORT_KEY_VEC4_TYPE vec4\n"
  ),
  code(
    uint sort_key_lt(float a, float b) {
      return uint(a < b);
    }

    uint sort_key_gt(float a, float b) {
      return uint(a > b);
    }
  )
)

THIN_GL_SNIPPET(sort_value_uint,
  string(
    "#define SORT_VALUE_TYPE uint\n"
  )
)

THIN_GL_SNIPPET(sort_internal,
  code(
    SORT_KEY_TYPE sort_load_key(uint buffer, uint index) {
      return u_input_keys[buffer].item[index];
    }

    SORT_KEY_TYPE sort_load_key_xy(uint buffer, uint x, uint y) {
      return u_input_keys[buffer].item[y*u_size + x];
    }

    void sort_store_key(uint buffer, uint index, SORT_KEY_TYPE key) {
      u_input_keys[buffer].item[index] = key;
    }

    void sort_store_key_xy(uint buffer, uint x, uint y, SORT_KEY_TYPE key) {
      u_input_keys[buffer].item[y*u_size + x] = value;
    }
  )
)

THIN_GL_SNIPPET(sort_value_internal,
  code(
    SORT_VALUE_TYPE sort_load_value(uint buffer, uint index) {
      return u_input_values[buffer].item[index];
    }

    SORT_VALUE_TYPE sort_load_value_xy(uint buffer, uint x, uint y) {
      return u_input_values[buffer].item[y*u_size + x];
    }

    void sort_load_value(uint buffer, uint index, SORT_VALUE_TYPE value) {
      u_input_values[buffer].item[index] = value;
    }

    void sort_load_value_xy(uint buffer, uint x, uint y, SORT_VALUE_TYPE value) {
      u_input_values[buffer].item[y*u_size + x] = value;
    }
  )
)

THIN_GL_SNIPPET(quicksort_internal,
  require(sort_internal),
  string(
    "#define QUICKSORT_KEY_VALUE_WORKGROUP_SIZE " THIN_GL_TO_STRING(QUICKSORT_KEY_VALUE_WORKGROUP_SIZE) "\n"
  ),
  code(
    shared uvec2 quicksort_workstack[32];
    shared uint quicksort_workstack_counter;

    uvec2 quicksort_workstack_pop() {
      if(gl_LocalInvocationID.x == 0) {
        quicksort_workstack_counter--;
      }
      barrier();
      return quicksort_workstack[quicksort_workstack_counter + 1];
    }

    SORT_KEY_TYPE quicksort_select_pivot(uvec2 offset_stride, uint index_a, uint index_b, uint index_c) {
      SORT_KEY_TYPE a = sort_load_key(offset_strice, index_a);
      SORT_KEY_TYPE b = sort_load_key(offset_strice, index_b);
      SORT_KEY_TYPE c = sort_load_key(offset_strice, index_c);
      uint b_count = uint(sort_key_lt(b, a)) + uint(sort_key_lt(b, c));
      uint c_count = uint(sort_key_lt(c, a)) + uint(sort_key_lt(c, b));
      SORT_KEY_VEC3_TYPE pivot = SORT_KEY_VEC3_TYPE(a, a, a);
      pivot[b_count] = b;
      pivot[c_count] = c;
      return pivot[1];
    }
  )
)

THIN_GL_SNIPPET(quicksort_key_value,
  require(quicksort_internal),
  require(sort_value_internal),
  code(
    shared SORT_KEY_TYPE quicksort_pivot;

    void quicksort_key_value(uint rbuffer, uint start, uint end) {
      if(gl_LocalInvocationID.x == 0) {
        workstack[0] = uvec2(start, (rbuffer << 31) | end);
        quicksort_workstack_counter = 0;
      }
      barrier();

      while(quicksort_workstack_counter > 0) {
        uvec2 range = quicksort_workstack_pop();
        rbuffer = range.y >> 31;
        uint wbuffer = !rbuffer;
        range.y &= 0x7FFFFFFF;
        uint range_length = range.y - range.x;

        /* threshold selected so we can do a full round of counting */
        if(range.y - range.x <= QUICKSORT_KEY_VALUE_WORKGROUP_SIZE * SORT_KEY_TYPE_COALESCED_COUNT) {
          // TODO
          // other in-place sort here
          continue;
        }

        if(gl_LocalInvocationID.x == 0) {
          quicksort_pivot = quicksort_select_pivot(rbuffer, range.x, range.x + range_length / 2, range.y - 1);
        }
        barrier();

        uint local_num_lower = 0;
        uint local_num_greater = 0;

        for(uint i = gl_LocalInvocationID.x; i < range.y; i += QUICKSORT_KEY_VALUE_WORKGROUP_SIZE) {
          SORT_KEY_TYPE key = sort_load_key(rbuffer, range.x + i);
          local_num_lower += sort_key_lt(key, quicksort_pivot);
          local_num_greater += sort_key_gt(key, quicksort_pivot);
        }

        uint global_num_lower = workgroup_sum(local_num_lower);
        barrier();
        uint global_num_greater = workgroup_sum(local_num_greater);
        barrier();

        uint index_lower = range.x + global_lower - local_lower;
        uint index_greater = range.y - global_greater;

        uint last_thread = range_length % QUICKSORT_KEY_VALUE_WORKGROUP_SIZE;
        uint num_elems_previous_threads =
          gl_LocalInvocationID.x * (range_length / QUICKSORT_KEY_VALUE_WORKGROUP_SIZE) +
          min(gl_LocalInvocationID.x, last_thread);
        uint index_pivot = range.x + num_elems_previous_threads - ((global_lower - local_lower) + (global_greater - local_greater));

        for(uint i = gl_LocalInvocationID.x; i < range.y; i += QUICKSORT_KEY_VALUE_WORKGROUP_SIZE) {
          SORT_KEY_TYPE key = sort_load_key(rbuffer, range.x + i);
          SORT_VALUE_TYPE value = sort_load_key(rbuffer, range.x + i);

          if(sort_key_lt(key, pivot)) {
            sort_store_key(wbuffer, index_lower, key);
            sort_store_value(wbuffer, index_lower, value);
            index_lower++;
          } else if(sort_key_gt(key, pivot)) {
            sort_store_key(wbuffer, index_greater, key);
            sort_store_value(wbuffer, index_greater, value);
            index_greater++;
          } else {

          }
        }
      }
    }
  )
)

THIN_GL_SHADER(particle_enumeration_sort_row_pass,
  require(sort_key_float),
  require(sort_value_uint),
  require(quicksort_key_value),
  main(
    quicksort_key_value(0, 0, u_size);
  )
)

THIN_GL_SHADER(particle_enumeration_sort_column_pass,
  require(sort_key_float),
  require(sort_value_uint),
  require(quicksort_key_value),
  main(
    quicksort_key_value(0, 0, u_size);
  )
)

THIN_GL_SHADER(particle_enumeration_sort_final_pass,
  require(sort_key_float),
  require(sort_value_uint),
  main(
    uint p = gl_GlobalInvocationID.x;
    uint q = gl_GlobalInvocationID.y;

    uint less = p * q - 1;
    SORT_KEY_TYPE key = sort_load_key_xy(0, p, q);
    SORT_VALUE_TYPE value = sort_load_key_xy(0, p, q);

    while(p >= 1 && q <= u_size) {
      if(sort_key_lt(key, sort_load_key_xy(0, p - 1, q + 1))) {
        p--;
      } else {
        less += p - 1;
        q++;
      }
    }

    while(p >= u_size && q >= 1) {
      if(sort_key_lt(key, sort_load_key_xy(0, p + 1, q - 1))) {
        q--;
      } else {
        less += q - 1;
        p++;
      }
    }

    sort_store_key(1, less + 1, key);
    sort_store_value(1, less + 1, value);
  )
)
// clang-format on


THIN_GL_IMPL_STRUCT(parallelSort_Parameters, require(DispatchIndirectCommand),
                    struct(DispatchIndirectCommand, on_blocks), struct(DispatchIndirectCommand, on_block_blocks),
                    uint32(num_elements), uint32(num_blocks))

// clang-format off
THIN_GL_SNIPPET(parallelSort,
    require(parallelSort_Parameters),
    string(
        "#define THIN_GL_PARALLELSORT_BITS_PER_PASS " THIN_GL_TO_STRING(THIN_GL_PARALLELSORT_BITS_PER_PASS)  "\n"
        "#define THIN_GL_PARALLELSORT_ELEMENTS_PER_THREAD " THIN_GL_TO_STRING(THIN_GL_PARALLELSORT_ELEMENTS_PER_THREAD) "\n"
        "#define THIN_GL_PARALLELSORT_THREADGROUP_SIZE " THIN_GL_TO_STRING(THIN_GL_PARALLELSORT_THREADGROUP_SIZE) "\n"
        "#define THIN_GL_PARALLELSORT_NUM_BINS (1 << THIN_GL_PARALLELSORT_BITS_PER_PASS)\n"
        "#define THIN_GL_PARALLELSORT_BLOCK_SIZE (THIN_GL_PARALLELSORT_ELEMENTS_PER_THREAD * THIN_GL_PARALLELSORT_THREADGROUP_SIZE)\n"
    ),
    code(
        parallelSort_Parameters init_parallelSort_Parameters(uint num_elements) {
            uint num_blocks = (num_elements + THIN_GL_PARALLELSORT_BLOCK_SIZE - 1) / THIN_GL_PARALLELSORT_BLOCK_SIZE;
            uint num_block_blocks = (num_blocks + THIN_GL_PARALLELSORT_BLOCK_SIZE - 1) / THIN_GL_PARALLELSORT_BLOCK_SIZE;
            return parallelSort_Parameters(
                DispatchIndirectCommand(num_blocks, 1, 1),
                DispatchIndirectCommand(num_block_blocks, 1, 1),
                num_elements,
                num_blocks
            );
        }
    )
)
// clang-format on

void GL_parallelSort_key_value_pairs(const struct GL_parallelSort_scratch *scratch, const struct GL_Buffer *keys,
                                     const struct GL_Buffer *values) {
  for(uint32_t shift = 0; shift < 32; shift += THIN_GL_PARALLELSORT_BITS_PER_PASS) {
    GL_compute_indirect(&parallelSort_count,
                        &(struct GL_ComputeAssets){.buffers[0] = &scratch->parameters,
                                                   .buffers[1] = &scratch->blocks,
                                                   .buffers[2] = keys,
                                                   .uniforms[0]._int = shift},
                        &scratch->parameters, offsetof(struct GL_DispatchIndirectCommand, on_blocks));

    GL_compute_indirect(&parallelSort_reduce,
                        &(struct GL_ComputeAssets){.buffers[0] = &scratch->parameters,
                                                   .buffers[1] = &scratch->blocks,
                                                   .buffers[2] = &scratch->block_blocks},
                        &scratch->parameters, offsetof(struct GL_DispatchIndirectCommand, on_blocks));

    GL_compute(&parallelSort_scan,
               &(struct GL_ComputeAssets){.buffers[0] = &scratch->parameters,
                                          .buffers[1] = &scratch->blocks,
                                          .buffers[2] = &scratch->block_blocks},
               1, 1, 1);

    GL_compute_indirect(&parallelSort_scan_add,
                        &(struct GL_ComputeAssets){.buffers[0] = &scratch->parameters,
                                                   .buffers[1] = &scratch->blocks,
                                                   .buffers[2] = &scratch->block_blocks},
                        &scratch->parameters, offsetof(struct GL_DispatchIndirectCommand, on_block_blocks));

    GL_compute_indirect(&parallelSort_scatter,
                        &(struct GL_ComputeAssets){.buffers[0] = &scratch->parameters,
                                                   .buffers[1] = &scratch->blocks,
                                                   .buffers[2] = &scratch->block_blocks},
                        &scratch->parameters, offsetof(struct GL_DispatchIndirectCommand, on_blocks));
  }
}

// --------------------------------------------------------------------------------------------------------------------

THIN_GL_DECLARE_STRUCT(enumerationSort_Parameters, require(DispatchIndirectCommand),
                       struct(DispatchIndirectCommand, on_row), struct(DispatchIndirectCommand, on_column),
                       struct(DispatchIndirectCommand, on_element))
THIN_GL_DECLARE_SNIPPET(enumerationSort)

// clang-format off
THIN_GL_SNIPPET(enumerationSort,
)

THIN_GL_SHADER()
// clang-format on

void GL_enumerationSort_key_value_pairs(const strucr GL_enumerationSort_scratch *scratch, const struct GL_Buffer *keys,
                                        const struct GL_Buffer *values) {}

#endif

#if 0

#define THIN_GL_PARALLELSORT_BITS_PER_PASS 4
#define THIN_GL_PARALLELSORT_ELEMENTS_PER_THREAD 4
#define THIN_GL_PARALLELSORT_THREADGROUP_SIZE 128
#define THIN_GL_PARALLELSORT_NUM_BINS (1 << THIN_GL_PARALLELSORT_BITS_PER_PASS)
#define THIN_GL_PARALLELSORT_BLOCK_SIZE                                                                                \
  (THIN_GL_PARALLELSORT_ELEMENTS_PER_THREAD * THIN_GL_PARALLELSORT_THREADGROUP_SIZE)

THIN_GL_DECLARE_STRUCT(parallelSort_Parameters, require(DispatchIndirectCommand),
                       struct(DispatchIndirectCommand, on_blocks), struct(DispatchIndirectCommand, on_block_blocks),
                       uint32(num_elements), uint32(num_blocks))
THIN_GL_DECLARE_SNIPPET(parallelSort)

struct GL_parallelSort_scratch {
  struct GL_Buffer parameters;
  struct GL_Buffer blocks;
  struct GL_Buffer block_blocks;
};

#define THIN_GL_PARALLELSORT_INIT_SCRATCH(MAX_ELEMENTS)                                                                \
  {                                                                                                                    \
    .parameters = {.kind = GL_Buffer_GPU, .size = sizeof(uint32_t) * 4}, /* 3 for indirect, one for actual length */   \
        .blocks = {.kind = GL_Buffer_GPU,                                                                              \
                   .size = sizeof(uint32_t) *                                                                          \
                           ((MAX_ELEMENTS + THIN_GL_PARALLELSORT_BLOCK_SIZE - 1) / THIN_GL_PARALLELSORT_BLOCK_SIZE)},  \
    .block_blocks = {                                                                                                  \
        .kind = GL_Buffer_GPU,                                                                                         \
        .size = sizeof(uint32_t) *                                                                                     \
                (((((MAX_ELEMENTS + THIN_GL_PARALLELSORT_BLOCK_SIZE - 1) / THIN_GL_PARALLELSORT_BLOCK_SIZE)) +         \
                  THIN_GL_PARALLELSORT_BLOCK_SIZE - 1) /                                                               \
                 THIN_GL_PARALLELSORT_BLOCK_SIZE)},                                                                    \
  }

void GL_parallelSort_key_value_pairs(const struct GL_parallelSort_scratch *scratch, const struct GL_Buffer *keys,
                                     const struct GL_Buffer *values);

// --------------------------------------------------------------------------------------------------------------------

// destructivly sort keys and values

THIN_GL_DECLARE_STRUCT(enumerationSort_Parameters, require(DispatchIndirectCommand),
                       struct(DispatchIndirectCommand, on_row), struct(DispatchIndirectCommand, on_column),
                       struct(DispatchIndirectCommand, on_element))
THIN_GL_DECLARE_SNIPPET(enumerationSort)

struct GL_enumerationSort_scratch {
  struct GL_Buffer parameters;
};

void GL_enumerationSort_key_value_pairs(const strucr GL_enumerationSort_scratch *scratch, const struct GL_Buffer *keys,
                                        const struct GL_Buffer *values);

#endif