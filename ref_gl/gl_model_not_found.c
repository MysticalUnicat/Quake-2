#include "gl_local.h"

#include "gl_thin.h"

// clang-format off
THIN_GL_SHADER(vertex,
  code(
    layout(location = 0) out vec3 out_normal;
  ),
  main(
    gl_Position = u_model_view_projection_matrix * vec4(in_position, 1);
    out_normal = normalize(in_position);
  )
)

THIN_GL_SHADER(fragment,
  code(
    layout(location = 0) in vec3 in_normal;

    layout(location = 0) out vec4 out_color;

    float do_sh1(float r0, vec3 r1, vec3 normal) {
      float r1_length = length(r1);
      vec3 r1_normalized = r1 / r1_length;
      float q = 0.5 * (1 + dot(r1_normalized, normal));
      float r1_length_over_r0 = r1_length / r0;
      float p = 1 + 2 * r1_length_over_r0;
      float a = (1 - r1_length_over_r0) / (1 + r1_length_over_r0);
      return r0 * (1 + (1 - a) * (p + 1) * pow(q, p));
    }
  ),
  main(
      vec3 normal = in_normal;

      vec3 lightmap = vec3(do_sh1(u_light_rgb0.r, u_light_r1, normal),
                           do_sh1(u_light_rgb0.g, u_light_g1, normal),
                           do_sh1(u_light_rgb0.b, u_light_b1, normal));

      out_color = vec4(lightmap * 2, 0.33);
  )
)
// clang-format on

#define VERTEX_FORMAT                                                                                                  \
  .attribute[0] = {0, alias_memory_Format_Float32, 3, "position", 0}, .binding[0] = {sizeof(float) * 3, 0}

#define UNIFORMS_FORMAT                                                                                                \
  .uniform[0] = {THIN_GL_FRAGMENT_BIT, GL_Type_Float3, "light_rgb0"},                                                  \
  .uniform[1] = {THIN_GL_FRAGMENT_BIT, GL_Type_Float3, "light_r1"},                                                    \
  .uniform[2] = {THIN_GL_FRAGMENT_BIT, GL_Type_Float3, "light_g1"},                                                    \
  .uniform[3] = {THIN_GL_FRAGMENT_BIT, GL_Type_Float3, "light_b1"},                                                    \
  .global[0] = {THIN_GL_VERTEX_BIT, &u_model_view_projection_matrix}

static struct GL_DrawState draw_state_opaque = {.primitive = GL_TRIANGLES,
                                                VERTEX_FORMAT,
                                                UNIFORMS_FORMAT,
                                                .vertex_shader = &vertex_shader,
                                                .fragment_shader = &fragment_shader,
                                                .depth_mask = true,
                                                .depth_test_enable = true,
                                                .depth_range_min = 0,
                                                .depth_range_max = 1};
static struct GL_DrawState draw_state_transparent = {.primitive = GL_TRIANGLES,
                                                     VERTEX_FORMAT,
                                                     UNIFORMS_FORMAT,
                                                     .vertex_shader = &vertex_shader,
                                                     .fragment_shader = &fragment_shader,
                                                     .depth_mask = false,
                                                     .depth_test_enable = true,
                                                     .depth_range_min = 0,
                                                     .depth_range_max = 1,
                                                     .blend_enable = true,
                                                     .blend_src_factor = GL_SRC_ALPHA,
                                                     .blend_dst_factor = GL_ONE_MINUS_SRC_ALPHA};

static bool init = false;
static struct GL_Buffer element_buffer;
static struct GL_Buffer vertex_buffer;

void GL_draw_model_not_found(void) {
  struct SH1 shadelight;
  int i;

  if(!init) {
    float vertex_data[] = {
        16, 0, 0, -16, 0, 0, 0, 16, 0, 0, -16, 0, 0, 0, 16, 0, 0, -16,
    };
    uint32_t element_data[] = {
        0, 4, 2, 0, 2, 5, 0, 5, 3, 0, 3, 4, 1, 2, 4, 1, 5, 2, 1, 3, 5, 1, 4, 3,
    };
    element_buffer = GL_allocate_static_buffer(GL_ELEMENT_ARRAY_BUFFER, sizeof(element_data), element_data);
    vertex_buffer = GL_allocate_static_buffer(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data);
    init = true;
  }

  if(currententity->flags & RF_FULLBRIGHT) {
    shadelight = SH1_FromDirectionalLight(vec3_origin, (float[]){1, 1, 1});
  } else
    shadelight = R_LightPoint(r_newrefdef.cmodel_index, currententity->origin);

  GL_matrix_identity(u_model_matrix.uniform.data.mat);
  GL_TransformForEntity(currententity);

  GL_draw_elements(
      (currententity->flags & RF_TRANSLUCENT) ? &draw_state_transparent : &draw_state_opaque,
      &(struct GL_DrawAssets){
          .element_buffer = &element_buffer,
          .vertex_buffers[0] = &vertex_buffer,
          .uniforms[0] =
              (struct GL_UniformData){.vec[0] = shadelight.f[0], .vec[1] = shadelight.f[4], .vec[2] = shadelight.f[8]},
          .uniforms[1] =
              (struct GL_UniformData){.vec[0] = shadelight.f[1], .vec[1] = shadelight.f[2], .vec[2] = shadelight.f[3]},
          .uniforms[2] =
              (struct GL_UniformData){.vec[0] = shadelight.f[5], .vec[1] = shadelight.f[6], .vec[2] = shadelight.f[7]},
          .uniforms[3] = (struct GL_UniformData){.vec[0] = shadelight.f[9],
                                                 .vec[1] = shadelight.f[10],
                                                 .vec[2] = shadelight.f[11]},
      },
      24, 1, 0, 0);
}
