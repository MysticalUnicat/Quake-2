#ifndef __GL_THIN_COMPUTE_H__
#define __GL_THIN_COMPUTE_H__

#include "gl_thin_cpp.h"

#define RADIXSORT_BITS_PER_PASS 4
#define RADIXSORT_WORKGROUP_SIZE 128
#define RADIXSORT_NUM_BINS (1 << RADIXSORT_BITS_PER_PASS)

THIN_GL_DECLARE_SNIPPET(sort_key_float)
THIN_GL_DECLARE_SNIPPET(sort_key_uint)
THIN_GL_DECLARE_SNIPPET(sort_key)
THIN_GL_DECLARE_SNIPPET(sort_key_xy)

THIN_GL_DECLARE_SNIPPET(sort_value_uint)
THIN_GL_DECLARE_SNIPPET(sort_value)
THIN_GL_DECLARE_SNIPPET(sort_value_xy)

THIN_GL_DECLARE_SNIPPET(sort_ascending)
THIN_GL_DECLARE_SNIPPET(sort_descending)

THIN_GL_DECLARE_SNIPPET(radixsort_defines)
THIN_GL_DECLARE_SNIPPET(radixsort_key_value)

// sortedmatrix_key and sortedmatrix_key_value require the use snippet sort_key_xy and sort_value_xy
// and their associated functions sort_key_xy_setup and sort_value_xy_setup before calling
THIN_GL_DECLARE_SNIPPET(sortedmatrix_key_value)

#endif