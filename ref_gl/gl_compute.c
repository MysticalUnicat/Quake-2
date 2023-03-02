#include "gl_compute.h"

#define THIN_GL_TO_STRING(X) THIN_GL_TO_STRING_(X)
#define THIN_GL_TO_STRING_(X) #X

// --------------------------------------------------------------------------------------------------------------------
// parameters
#define RADIXSORT_BITS_PER_PASS 4
#define RADIXSORT_WORKGROUP_SIZE 128
#define RADIXSORT_NUM_BINS (1 << RADIXSORT_BITS_PER_PASS)

// clang-format off
// --------------------------------------------------------------------------------------------------------------------
// primitives
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

// --------------------------------------------------------------------------------------------------------------------
// sort primitives
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

THIN_GL_SNIPPET(sort_key_uint,
  string(
    "#define SORT_KEY_TYPE uint\n"
    "#define SORT_KEY_VEC2_TYPE uvec2\n"
    "#define SORT_KEY_VEC3_TYPE uvec3\n"
    "#define SORT_KEY_VEC4_TYPE uvec4\n"
  ),
  code(
    uint sort_key_lt(uint a, uint b) {
      return uint(a < b);
    }

    uint sort_key_gt(uint a, uint b) {
      return uint(a > b);
    }
  )
)

THIN_GL_SNIPPET(sort_key_internal,
  code(
    SORT_KEY_TYPE sort_load_key(uint buffer, uint index) {
      return u_keys[buffer].item[index];
    }

    SORT_KEY_TYPE sort_load_key_xy(uint buffer, uint x, uint y) {
      return u_keys[buffer].item[y*u_size + x];
    }

    void sort_store_key(uint buffer, uint index, SORT_KEY_TYPE key) {
      u_keys[buffer].item[index] = key;
    }

    void sort_store_key_xy(uint buffer, uint x, uint y, SORT_KEY_TYPE key) {
      u_keys[buffer].item[y*u_size + x] = value;
    }
  )
)

THIN_GL_SNIPPET(sort_value_uint,
  string(
    "#define SORT_VALUE_TYPE uint\n"
  )
)

THIN_GL_SNIPPET(sort_value_internal,
  code(
    SORT_VALUE_TYPE sort_load_value(uint buffer, uint index) {
      return u_values[buffer].item[index];
    }

    SORT_VALUE_TYPE sort_load_value_xy(uint buffer, uint x, uint y) {
      return u_values[buffer].item[y*u_size + x];
    }

    void sort_load_value(uint buffer, uint index, SORT_VALUE_TYPE value) {
      u_values[buffer].item[index] = value;
    }

    void sort_load_value_xy(uint buffer, uint x, uint y, SORT_VALUE_TYPE value) {
      u_values[buffer].item[y*u_size + x] = value;
    }
  )
)

// --------------------------------------------------------------------------------------------------------------------
// a radix sort designed for a low amount of elements
THIN_GL_SNIPPET(radixsort_defines,
  string(
    "#define RADIXSORT_BITS_PER_PASS " THIN_GL_TO_STRING(RADIXSORT_BITS_PER_PASS) "\n"
    "#define RADIXSORT_WORKGROUP_SIZE " THIN_GL_TO_STRING(RADIXSORT_WORKGROUP_SIZE) "\n"
    "#define RADIXSORT_NUM_BINS (1 << RADIXSORT_BITS_PER_PASS)\n"
  ),
)

THIN_GL_SNIPPET(radixsort_key_value,
  require(sort_key_internal),
  require(sort_value_internal),
  require(radixsort_defines),
  code(
    shared uint radixsort_sum_table[RADIXSORT_WORKGROUP_SIZE * RADIXSORT_NUM_BINS];
    shared uint radixsort_reduce_table[RADIXSORT_NUM_BINS];

    void radixsort_key_value(uint rbuffer, uint buffer_offset, uint bit_offset, uint length) {
      // count
      for(uint i = 0; i < RADIXSORT_NUM_BINS; i++) {
        radixsort_sum_table[i * RADIXSORT_WORKGROUP_SIZE + gl_LocalInvocationID.x] = 0;
      }
      barrier();

      uint buffer_end = buffer_offset + length;

      for(uint src_index = buffer_offset + gl_LocalInvocationID.x; src_index < buffer_end; src_index += RADIXSORT_WORKGROUP_SIZE) {
        uint key = sort_load_key(rbuffer, buffer_offset + src_index);
        uint bin = (key >> bit_offset) & 0xf;

        atomicAdd(radixsort_sum_table[bin * RADIXSORT_WORKGROUP_SIZE + gl_LocalInvocationID.x], 1);
      }
      barrier();

      // reduce
      for(uint bin = 0; i < RADIXSORT_NUM_BINS; bin++) {
        barrier();
        uint sum = radixsort_sum_table[bin * RADIXSORT_WORKGROUP_SIZE + gl_LocalInvocationID.x];
        sum = workgroup_sum(sum);

        if(!gl_LocalInvocationID.x) {
          radixsort_reduce_table[bin] = sum;
        }
      }
      barrier();

      // scan
      uint sum = 0;
      for(uint bin = 0; bin < RADIXSORT_NUM_BINS; bin++) {
        sum += atomicExchange(radixsort_sum_table[bin * RADIXSORT_WORKGROUP_SIZE + gl_LocalInvocationID.x], sum);
      }
      barrier();

      // scan add
      if(gl_LocalInvocationID.x == 0) {
        uint sum = 0;
        for(uint bin = 0; bin < RADIXSORT_NUM_BINS; i++) {
          sum += atomicExchange(radixsort_reduce_table[bin], sum);
        }
      }
      barrier();

      // scatter
      uint wbuffer = !rbuffer;

      for(uint src_index = buffer_offset + gl_LocalInvocationID.x; src_index < buffer_end; src_index += RADIXSORT_WORKGROUP_SIZE) {
        uint key = sort_load_key(rbuffer, src_index);
        uint value = sort_load_value(rbuffer, src_index);

        uint bin = (key >> bit_offset) & 0xf;
        uint dst_index = buffer_offset + radixsort_reduce_table[bin] + atomicAdd(radixsort_sum_table[bin * RADIXSORT_WORKGROUP_SIZE + gl_LocalInvocationID.x], 1);

        sort_store_key(wbuffer, dst_index, key);
        sort_store_value(wbuffer, dst_index, value);
      }
    }
  )
)

// --------------------------------------------------------------------------------------------------------------------
THIN_GL_SNIPPET(sortedmatrix_key_value,
  require(sort_key_internal),
  require(sort_value_internal),
  code(
    void sortedmatrix_key_value(uint rbuffer, uint x, uint y) {
      uint wbuffer = !rbuffer;
      uint p = x;
      uint q = y;

      uint less = p * q - 1;
      SORT_KEY_TYPE key = sort_load_key_xy(rbuffer, p, q);
      SORT_VALUE_TYPE value = sort_load_key_xy(rbuffer, p, q);

      while(p >= 1 && q <= u_size) {
        if(sort_key_lt(key, sort_load_key_xy(rbuffer, p - 1, q + 1))) {
          p--;
        } else {
          less += p - 1;
          q++;
        }
      }

      while(p >= u_size && q >= 1) {
        if(sort_key_lt(key, sort_load_key_xy(rbuffer, p + 1, q - 1))) {
          q--;
        } else {
          less += q - 1;
          p++;
        }
      }

      sort_store_key(wbuffer, less + 1, key);
      sort_store_value(wbuffer, less + 1, value);
    }
  )
)
// clang-format on
