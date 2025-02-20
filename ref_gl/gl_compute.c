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

// -------------------------------------
// random
THIN_GL_SNIPPET(random,
  code(
    // forward
    void random_init(uint x, uint y, uint z);
    void random_advance(uint count);
    uint uint_random();

    float float_random_u() {
      return (uint_random() >> 9) * 0x1.0p-24;
    }

    float float_random_s() {
      return (uint_random() >> 9) * 0x1.0p-23 - 0.5f;
    }
  )
)

// https://arxiv.org/pdf/2004.06278.pdf
THIN_GL_SNIPPET(random_squares32,
  require(random),
  code(
    const uint64_t random_squares32_default_key = 0xEA3742C76BF95D47;

    uint random_squares32(uint64_t counter, uint64_t key) {
      uint64_t x = (counter + 1) * key;
      uint64_t y = x;
      uint64_t z = y + key;
      x = x*x + y; x = (x >> 32) | (x << 32);
      x = x*x + z; x = (x >> 32) | (x << 32);
      x = x*x + y; x = (x >> 32) | (x << 32);
      return uint((x*x + z) >> 32);
    }

    uint64_t global_random_squares32_counter = 0;

    void random_init(uint x, uint y, uint z) {
      global_random_squares32_counter = (uint64_t(x) * 3 + uint64_t(y) * 5 + uint64_t(z) * 7) << 32;
    }

    void random_advance(uint count) {
      global_random_squares32_counter += count;
    }

    uint uint_random() {
      return random_squares32(global_random_squares32_counter++, random_squares32_default_key);
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

    bool sort_key_le(float a, float b) {
      return a <= b;
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

    bool sort_key_le(uint a, uint b) {
      return a <= b;
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
      // width dominated
      case octic_group_e:
        offset = 0;
        x_stride = 1;
        y_stride = int(width);
        break;
      // height dominated
      case octic_group_r:
        offset = int(height * (width - 1));
        x_stride = -int(height);
        y_stride = 1;
        break;
      // width dominated
      case octic_group_r2:
        offset = int(height * width) - 1;
        x_stride = -1;
        y_stride = -int(width);
        break;
      // height dominated
      case octic_group_r3:
        offset = int(height) - 1;
        x_stride = int(height);
        y_stride = -1;
        break;
      // width dominated
      case octic_group_Tx:
        offset = int(width) - 1;
        x_stride = 1;
        y_stride = int(width);
        break;
      // width dominated
      case octic_group_Ty:
        offset = int(height * (width - 1));
        x_stride = 1;
        y_stride = -int(width);
        break;
      // height dominated
      case octic_group_Tac:
        offset = 0;
        x_stride = int(height);
        y_stride = 1;
        break;
      // height dominated
      case octic_group_Tbd:
        offset = int(height * width) - 1;
        x_stride = -1;
        y_stride = -int(height);
        break;
      }
      return Indexer2D(offset, x_stride, y_stride);
    }

    uint Indexer2D_apply(Indexer2D indexer, uint x, uint y) {
      return uint(indexer.offset + indexer.x_stride * int(x) + indexer.y_stride * int(y));
    }
  )
)

THIN_GL_SNIPPET(sort_ascending,
  code(
    uint sort_apply_order(uint index, uint length) {
      return index;
    }
  )
)

THIN_GL_SNIPPET(sort_descending,
  code(
    uint sort_apply_order(uint index, uint length) {
      return length - index - 1;
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

    bool radixsort_key_value(uint rbuf, uint buffer_offset, uint bit_offset, uint length) {
      uint wbuf = rbuf ^ 1;

      uint invalid_key = sort_apply_order(1, 2) * 0xFFFFFFFF;

      // count
      if(gl_LocalInvocationID.x < RADIXSORT_NUM_BINS) {
        radixsort_histogram[gl_LocalInvocationID.x] = 0;
      }
      barrier();

      for(uint src_index = gl_LocalInvocationID.x; src_index < length; src_index += RADIXSORT_WORKGROUP_SIZE) {
        uint key = sort_load_key(rbuf, buffer_offset + src_index);
        uint bin = sort_apply_order((key >> bit_offset) & 0xf, RADIXSORT_NUM_BINS);
        atomicAdd(radixsort_histogram[bin], 1);
      }
      barrier();

      uint max_count = subgroupMax(gl_LocalInvocationID.x < RADIXSORT_NUM_BINS ? radixsort_histogram[gl_LocalInvocationID.x] : 0);
      if(gl_LocalInvocationID.x == 0) {
        workgroup_scratch_uint[0] = max_count;
      }
      barrier();

      if(workgroup_scratch_uint[0] == length) {
        return bool(0);
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

        uint key = src_index < length ? sort_load_key(rbuf, buffer_offset + src_index) : invalid_key;
        radixsort_values[gl_LocalInvocationID.x] = src_index < length ? sort_load_value(rbuf, buffer_offset + src_index) : 0x80808080;
        uint value = gl_LocalInvocationID.x;

        if(gl_LocalInvocationID.x < RADIXSORT_NUM_BINS) {
          radixsort_histogram[gl_LocalInvocationID.x] = 0;
        }

        // sort keys in workgroup
        for(uint bit_shift = 0; bit_shift < RADIXSORT_BITS_PER_PASS; bit_shift += 2) {
          uint key_index = sort_apply_order((key >> bit_offset) & 0xf, RADIXSORT_NUM_BINS);
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

        uint key_index = sort_apply_order((key >> bit_offset) & 0xf, RADIXSORT_NUM_BINS);
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

      return bool(1);
    }
  )
)

// --------------------------------------------------------------------------------------------------------------------
// https://www.sciencedirect.com/science/article/pii/S1877050923001461?ref=cra_js_challenge&fr=RR-1
THIN_GL_SNIPPET(sortedmatrix_internal,
  require(sort_key),
  require(sort_key_xy),
  code(
    /*
        After sorting rows and then columns, for each element consider the sorted matrix to be split into 4 quadrants:

        <- lower             higher ->
        +-------------------+------------+ <- lower
        | Qudrant II        | Quadrant I |
        |              +---+-------------+
        |              | E |             |
        +--------------+---+             |
        | Quadrant III |     Quadrant IV |
        +--------------+-----------------+ <- higher

        The paper shows elements in Quadrant II are lower than E and elements in Quadrant IV are higher.
        Search in Quadrant I and III for elements in those areas less that E

    example sorted matrix 5x5

     1  3 14 14  7
    14 10  8 10  5
     9 14  2  5  9
     4 13  2  8  4
     9  4  5 10 10

     1  3  7 14 14
     5  8 10 10 14
     2  5  9  9 14
     2  4  4  8 13
     4  5  9 10 10

     1  3  4  8 10
     2  4  7  9 13
     2  5  9 10 14
     4  5  9 10 14
     5  8 10 14 14

    */

    uint sortedmatrix_enumerate(uint rbuf, uint width, uint height, uint x, uint y, out SORT_KEY_TYPE key) {
      uint original_x = x;
      uint original_y = y;

      uint count = (x + 1) * (y + 1) - 1;

      key = sort_load_key_xy(rbuf, x, y);

      /* Search in Quadrant I */
      while(x < width - 1 && y > 0) {
        // gather test key, directly adjacent up right
        SORT_KEY_TYPE test = sort_load_key_xy(rbuf, x + 1, y - 1);
        if(sort_key_lt(key, test)) {
          // all elements in this row right of `test` are larger, move up to check the row above
          y--;
        } else {
          // all elements above `test` are smaller, add those elements to enumeration
          count += y;

          // move right
          x++;
        }
      }

      x = original_x;
      y = original_y;

      /* Search in Quadrand III */
      while(x > 0 && y < height - 1) {
        // gather test key, directly adjacent down left
        SORT_KEY_TYPE test = sort_load_key_xy(rbuf, x - 1, y + 1);
        if(sort_key_le(key, test)) {
          // all elements in this column below `test` are larger, move to check the column to the left
          x--;
        } else {
          // all elements left of `test` are smaller, add those elements to enumeration
          count += x;

          // move down
          y++;
        }
      }

      return count;
    }
  )
)

THIN_GL_SNIPPET(sortedmatrix_key_value,
  require(sort_value),
  require(sort_value_xy),
  require(sortedmatrix_internal),
  code(
    void sortedmatrix_key_value(uint rbuf, uint width, uint height, uint length, uint x, uint y) {
      uint wbuf = rbuf ^ 1;

      SORT_KEY_TYPE key;
      uint dst_index = sortedmatrix_enumerate(rbuf, width, height, x, y, key);
      barrier();

      if(dst_index < length) {
        dst_index = sort_apply_order(dst_index, length);

        SORT_VALUE_TYPE value = sort_load_value_xy(rbuf, x, y);

        sort_store_key(wbuf, dst_index, key);
        sort_store_value(wbuf, dst_index, value);
      }
    }

    void sortedmatrix_key_value_drop_key(uint rbuf, uint width, uint height, uint length, uint x, uint y) {
      uint wbuf = rbuf ^ 1;

      SORT_KEY_TYPE key;
      uint dst_index = sortedmatrix_enumerate(rbuf, width, height, x, y, key);
      barrier();

      if(dst_index < length) {
        dst_index = sort_apply_order(dst_index, length);

        SORT_VALUE_TYPE value = sort_load_value_xy(rbuf, x, y);
        sort_store_value(wbuf, dst_index, value);
      }
    }
  )
)
// clang-format on
