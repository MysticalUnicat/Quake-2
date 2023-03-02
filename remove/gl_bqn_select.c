#include "gl_bqn.h"

// https://mlochbaum.github.io/BQN/doc/select.html

// clang-format off
THIN_GL_IMPL_STRUCT(BQN_Select_Parameters, require(DispatchIndirectCommand), struct(DispatchIndirectCommand, indirect))

THIN_GL_IMPL_SNIPPET(BQN_select,
  require(BQN_Select_Parameters),
  code(
    BQN_Select_Parameters BQN_Select_Parameters_initialize(uint haystack_length, uint indexes_length) {
      return BQN_Select_Parameters(
        DispatchIndirectCommand(indexes_length, 1, 1)
      );
    }
  )
)

THIN_GL_SHADER(select,
  require(BQN_Select_Parameters),
  main(
    uint index = gl_GlobalInvocationID.x;
    u_output.item[index] = u_haystack.item[u_indexes[index]];
  )
)
// clang-format on

static struct GL_ComputeState compute_state = {.shader = &select_shader,
                                               .buffer = {{0, GL_Type_ShaderStorageBuffer, "haystack"},
                                                          {0, GL_Type_ShaderStorageBuffer, "indexes"},
                                                          {0, GL_Type_ShaderStorageBuffer, "output"}}};

void GL_bqn_select(const struct GL_Buffer *parameters, const struct GL_Buffer *haystack,
                   const struct GL_Buffer *indexes, const struct GL_Buffer *output) {
  GL_compute_indirect(&compute_state,
                      &(struct GL_ComputeAssets){
                          .buffers[0] = haystack,
                          .buffers[1] = indexes,
                          .buffers[2] = output,
                      },
                      parameters, 0);
}