#include "gl_compute.h"

#define THIN_GL_TO_STRING(X) THIN_GL_TO_STRING_(X)
#define THIN_GL_TO_STRING_(X) #X

// clang-format off
// --------------------------------------------------------------------------------------------------------------------
// primitives
THIN_GL_SNIPPET(workgroup_scratch_uint,
  code(
    shared uint workgroup_scratch_uint[gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z];
  )
)

THIN_GL_SNIPPET(workgroupExclusiveAdd_uint,
  require(workgroup_scratch_uint),
  code(
    uint workgroupExclusiveAdd_uint(uint value) {
      uint subgroup_result = subgroupExclusiveAdd(value);

      if(gl_SubgroupInvocationID == gl_SubgroupSize - 1) {
        workgroup_scratch_uint[gl_SubgroupID] = subgroup_result + value;
      }
      barrier();

      if(gl_SubgroupID == 0) {
        workgroup_scratch_uint[gl_LocalInvocationIndex] = subgroupExclusiveAdd(workgroup_scratch_uint[gl_LocalInvocationIndex]);
      }
      barrier();

      subgroup_result += workgroup_scratch_uint[gl_SubgroupID];

      return subgroup_result;
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
    bool sort_key_lt(float a, float b) {
      return a < b;
    }

    bool sort_key_gt(float a, float b) {
      return a > b;
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
    bool sort_key_lt(uint a, uint b) {
      return a < b;
    }

    bool sort_key_gt(uint a, uint b) {
      return a > b;
    }
  )
)

THIN_GL_SNIPPET(sort_key,
  code(
    SORT_KEY_TYPE sort_load_key(uint buf, uint index) {
      return u_sort_keys[buf].item[index];
    }

    void sort_store_key(uint buf, uint index, SORT_KEY_TYPE key) {
      u_sort_keys[buf].item[index] = key;
    }
  )
)

// https://proofwiki.org/wiki/Definition:Symmetry_Group_of_Square
// dihedral group of order 8
//
// e | r | r2 | r3 | tx | ty | tac | tbd
//
// okay, okay. a rectangle is not a square.
// its fine when talking about transformations.

THIN_GL_SNIPPET(octic_group,
  code(
    struct Indexer2D {
      int offset;
      int x_stride;
      int y_stride;
    };

    const uint octic_group_e = 0;
    const uint octic_group_r = 1;
    const uint octic_group_r2 = 2;
    const uint octic_group_r3 = 3;
    const uint octic_group_Tx = 4;
    const uint octic_group_Ty = 5;
    const uint octic_group_Tac = 6;
    const uint octic_group_Tbd = 7;

    Indexer2D Indexer2D_init(uint octic_group, uint width, uint height) {
      int offset;
      int x_stride;
      int y_stride;
      switch(octic_group) {
      case octic_group_e:
        offset = 0;
        x_stride = 1;
        y_stride = int(width);
        break;
      case octic_group_r:
        offset = int(width * height) - 1;
        x_stride = -int(width);
        y_stride = 1;
        break;
      case octic_group_r3:
        offset = int(width) - 1;
        x_stride = int(width);
        y_stride = -1;
        break;
      }
      return Indexer2D(offset, x_stride, y_stride);
    }

    uint Indexer2D_apply(Indexer2D indexer, uint x, uint y) {
      return uint(indexer.offset + indexer.x_stride * int(x) + indexer.y_stride * int(y));
    }
  )
)

THIN_GL_SNIPPET(sort_key_xy,
  require(sort_key),
  require(octic_group),
  code(
    shared Indexer2D sort_key_indexer[2];

    void sort_key_xy_setup(uint buf, uint octic_group_transform, uint width, uint height) {
      sort_key_indexer[buf] = Indexer2D_init(octic_group_transform, width, height);
    }

    SORT_KEY_TYPE sort_load_key_xy(uint buf, uint x, uint y) {
      return sort_load_key(buf, Indexer2D_apply(sort_key_indexer[buf], x, y));
    }

    void sort_store_key_xy(uint buf, uint x, uint y, SORT_KEY_TYPE key) {
      sort_store_key(buf, Indexer2D_apply(sort_key_indexer[buf], x, y), key);
    }
  )
)

THIN_GL_SNIPPET(sort_value_uint,
  string(
    "#define SORT_VALUE_TYPE uint\n"
  )
)

THIN_GL_SNIPPET(sort_value,
  code(
    SORT_VALUE_TYPE sort_load_value(uint buf, uint index) {
      return u_sort_values[buf].item[index];
    }

    void sort_store_value(uint buf, uint index, SORT_VALUE_TYPE value) {
      u_sort_values[buf].item[index] = value;
    }
  )
)

THIN_GL_SNIPPET(sort_value_xy,
  require(sort_value),
  require(octic_group),
  code(
    shared Indexer2D sort_value_indexer[2];

    void sort_value_xy_setup(uint buf, uint octic_group_transform, uint width, uint height) {
      sort_value_indexer[buf] = Indexer2D_init(octic_group_transform, width, height);
    }

    SORT_VALUE_TYPE sort_load_value_xy(uint buf, uint x, uint y) {
      return sort_load_value(buf, Indexer2D_apply(sort_value_indexer[buf], x, y));
    }

    void sort_store_value_xy(uint buf, uint x, uint y, SORT_VALUE_TYPE value) {
      sort_store_value(buf, Indexer2D_apply(sort_value_indexer[buf], x, y), value);
    }
  )
)

// --------------------------------------------------------------------------------------------------------------------
// algo from https://github.com/GPUOpen-Effects/FidelityFX-ParallelSort
// changed to early exit, require no temporary buffer storage but processes fewer elements

THIN_GL_SNIPPET(radixsort_defines,
  string(
    "#define RADIXSORT_BITS_PER_PASS " THIN_GL_TO_STRING(RADIXSORT_BITS_PER_PASS) "\n"
    "#define RADIXSORT_WORKGROUP_SIZE " THIN_GL_TO_STRING(RADIXSORT_WORKGROUP_SIZE) "\n"
    "#define RADIXSORT_NUM_BINS (1 << RADIXSORT_BITS_PER_PASS)\n"
  ),
)

THIN_GL_SNIPPET(radixsort_key_value,
  require(sort_key),
  require(sort_value),
  require(radixsort_defines),
  require(workgroup_scratch_uint),
  require(workgroupExclusiveAdd_uint),
  code(
    shared uint radixsort_histogram[RADIXSORT_NUM_BINS];
    shared uint radixsort_offsets[RADIXSORT_NUM_BINS];
    shared SORT_VALUE_TYPE radixsort_values[RADIXSORT_WORKGROUP_SIZE];

    void radixsort_key_value(uint rbuf, uint buffer_offset, uint bit_offset, uint length) {
       uint wbuf = rbuf ^ 1;

      // count
      if(gl_LocalInvocationID.x < RADIXSORT_NUM_BINS) {
        radixsort_histogram[gl_LocalInvocationID.x] = 0;
      }
      barrier();

      for(uint src_index = gl_LocalInvocationID.x; src_index < length; src_index += RADIXSORT_WORKGROUP_SIZE) {
        uint key = sort_load_key(rbuf, buffer_offset + src_index);
        uint bin = (key >> bit_offset) & 0xf;
        atomicAdd(radixsort_histogram[bin], 1);
      }
      barrier();

      if(radixsort_histogram[0] == length) {
        return;
      }

      // scan
      uint histogram_prefix_sum = subgroupExclusiveAdd(gl_LocalInvocationID.x < RADIXSORT_NUM_BINS ? radixsort_histogram[gl_LocalInvocationID.x] : 0);
      if(gl_LocalInvocationID.x < RADIXSORT_NUM_BINS) {
        radixsort_offsets[gl_LocalInvocationID.x] = histogram_prefix_sum;
      }
      barrier();

      // scatter
      uint num_blocks = (length + RADIXSORT_WORKGROUP_SIZE - 1) / RADIXSORT_WORKGROUP_SIZE;

      for(uint block = 0; block < num_blocks; block++) {
        uint src_index = block * RADIXSORT_WORKGROUP_SIZE + gl_LocalInvocationID.x;

        uint key = src_index < length ? sort_load_key(rbuf, buffer_offset + src_index) : 0xFFFFFFFF;
        radixsort_values[gl_LocalInvocationID.x] = src_index < length ? sort_load_value(rbuf, buffer_offset + src_index) : 0x80808080;
        uint value = gl_LocalInvocationID.x;

        if(gl_LocalInvocationID.x < RADIXSORT_NUM_BINS) {
          radixsort_histogram[gl_LocalInvocationID.x] = 0;
        }

        // sort keys in workgroup
        for(uint bit_shift = 0; bit_shift < RADIXSORT_BITS_PER_PASS; bit_shift += 2) {
          uint key_index = (key >> bit_offset) & 0xf;
          uint bit_key = (key_index >> bit_shift) & 0x3;

          uint packed_histogram = 1 << (bit_key * 8);

          uint local_sum = workgroupExclusiveAdd_uint(packed_histogram);

          if(gl_LocalInvocationID.x == (RADIXSORT_WORKGROUP_SIZE - 1)) {
            workgroup_scratch_uint[0] = local_sum + packed_histogram;
          }
          barrier();
          packed_histogram = workgroup_scratch_uint[0];

          packed_histogram = (packed_histogram << 8) + (packed_histogram << 16) + (packed_histogram << 24);

          local_sum += packed_histogram;

          uint key_offset = (local_sum >> (bit_key * 8)) & 0xff;

          workgroup_scratch_uint[key_offset] = key;
          barrier();
          key = workgroup_scratch_uint[gl_LocalInvocationID.x];
          barrier();

          workgroup_scratch_uint[key_offset] = value;
          barrier();
          value = workgroup_scratch_uint[gl_LocalInvocationID.x];
          barrier();
        }

        uint key_index = (key >> bit_offset) & 0xf;
        atomicAdd(radixsort_histogram[key_index], 1);
        barrier();

        histogram_prefix_sum = subgroupExclusiveAdd(gl_LocalInvocationID.x < RADIXSORT_NUM_BINS ? radixsort_histogram[gl_LocalInvocationID.x] : 0);
        if(gl_LocalInvocationID.x < RADIXSORT_NUM_BINS) {
          workgroup_scratch_uint[gl_LocalInvocationID.x] = histogram_prefix_sum;
        }

        uint global_offset = radixsort_offsets[key_index];
        barrier();

        uint local_offset = gl_LocalInvocationID.x - workgroup_scratch_uint[key_index];

        uint total_offset = global_offset + local_offset;

        if(total_offset < length) {
          sort_store_key(wbuf, buffer_offset + total_offset, key);
          sort_store_value(wbuf, buffer_offset + total_offset, radixsort_values[value]);
        }
        barrier();

        if(gl_LocalInvocationID.x < RADIXSORT_NUM_BINS) {
          atomicAdd(radixsort_offsets[gl_LocalInvocationID.x], radixsort_histogram[gl_LocalInvocationID.x]);
        }
      }
      barrier();
    }
  )
)

// --------------------------------------------------------------------------------------------------------------------
THIN_GL_SNIPPET(sortedmatrix_internal,
  require(sort_key),
  require(sort_key_xy),
  code(
    uint sortedmatrix_locate(uint rbuf, uint width, uint height, uint x, uint y, SORT_KEY_TYPE key) {
      uint wbuf = rbuf ^ 1;
      uint p = x;
      uint q = y;
      int less = int(p * q) - 1;

      while(p >= 1 && q <= height) {
        if(sort_key_lt(key, sort_load_key_xy(rbuf, p - 1, q + 1))) {
          p--;
        } else {
          less += int(p) - 1;
          q++;
        }
      }

      while(p >= width && q >= 1) {
        if(sort_key_lt(key, sort_load_key_xy(rbuf, p + 1, q - 1))) {
          q--;
        } else {
          less += int(q) - 1;
          p++;
        }
      }

      return uint(less + 1);
    }
  )
)

THIN_GL_SNIPPET(sortedmatrix_key_value,
  require(sort_value),
  require(sort_value_xy),
  require(sortedmatrix_internal),
  code(
    void sortedmatrix_key_value(uint rbuf, uint width, uint height, uint x, uint y) {
      uint wbuf = rbuf ^ 1;

      SORT_KEY_TYPE key = sort_load_key_xy(rbuf, x, y);
      SORT_VALUE_TYPE value = sort_load_value_xy(rbuf, x, x);

      uint dst_index = sortedmatrix_locate(rbuf, width, height, x, y, key);

      sort_store_key(wbuf, dst_index, key);
      sort_store_value(wbuf, dst_index, value);
    }

    void sortedmatrix_key_value_drop_key(uint rbuf, uint width, uint height, uint x, uint y) {
      uint wbuf = rbuf ^ 1;

      SORT_KEY_TYPE key = sort_load_key_xy(rbuf, x, y);
      SORT_VALUE_TYPE value = sort_load_value_xy(rbuf, x, x);

      uint dst_index = sortedmatrix_locate(rbuf, width, height, x, y, key);

      sort_store_value(wbuf, dst_index, value);
    }
  )
)
// clang-format on
