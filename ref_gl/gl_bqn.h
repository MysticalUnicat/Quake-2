#ifndef __GL_THIN_BQN_H__
#define __GL_THIN_BQN_H__

#include "gl_thin_cpp.h"

// https://mlochbaum.github.io/BQN/doc/primitive.html

#define THIN_GL_BQN_GRADE_BITS_PER_PASS 4
#define THIN_GL_BQN_GRADE_ELEMENTS_PER_THREAD 1
#define THIN_GL_BQN_GRADE_THREADGROUP_SIZE 128

THIN_GL_DECLARE_STRUCT(BQN_Grade_Parameters, uint32(num_keys), uint32(num_blocks))
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