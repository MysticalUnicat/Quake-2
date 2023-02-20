#include "gl_local.h"

#include "gl_thin.h"

struct SpriteVertex {
  float position[3];
  float st[2];
};

// clang-format off
static char vertex_shader_source[] =
  GL_MSTR(
    layout(location = 0) out vec2 out_st;

    void main() {
      gl_Position = u_view_projection_matrix * vec4(in_position, 1);
      out_st = in_st;
    }
  );

static char fragment_shader_source[] =
  GL_MSTR(
    layout(location = 0) in vec2 in_st;

    layout(location = 0) out vec4 out_color;

    void main() {
      out_color = texture(u_img, in_st) * vec4(1, 1, 1, u_alpha);
    }
  );
// clang-format on

static struct GL_DrawState draw_state = {.primitive = GL_TRIANGLES,
                                         .attribute[0] = {0, alias_memory_Format_Float32, 3, "position", 0},
                                         .attribute[1] = {0, alias_memory_Format_Float32, 2, "st", 12},
                                         .binding[0] = {sizeof(struct SpriteVertex)},
                                         .uniform[0] = {THIN_GL_FRAGMENT_BIT, GL_Type_Float, "alpha"},
                                         .global[0] = {THIN_GL_VERTEX_BIT, &u_view_projection_matrix},
                                         .image[0] = {THIN_GL_FRAGMENT_BIT, GL_Type_Sampler2D, "img"},
                                         .vertex_shader.source = vertex_shader_source,
                                         .fragment_shader.source = fragment_shader_source,
                                         .depth_test_enable = true,
                                         .depth_mask = false,
                                         .depth_range_min = 0,
                                         .depth_range_max = 1,
                                         .blend_enable = true,
                                         .blend_src_factor = GL_SRC_ALPHA,
                                         .blend_dst_factor = GL_ONE_MINUS_SRC_ALPHA};

/*
=================
R_DrawSpriteModel

=================
*/
void GL_draw_sprite(entity_t *e) {
  float alpha = 1.0F;
  vec3_t point;
  dsprframe_t *frame;
  float *up, *right;
  dsprite_t *psprite;

  // don't even bother culling, because it's just a single
  // polygon without a surface cache

  psprite = (dsprite_t *)currentmodel->extradata;

  e->frame %= psprite->numframes;

  frame = &psprite->frames[e->frame];

  up = vup;
  right = vright;

  if(e->flags & RF_TRANSLUCENT)
    alpha = e->alpha;

  struct SpriteVertex vertexes[4];

  vertexes[0].st[0] = 0;
  vertexes[0].st[1] = 1;
  VectorMA(e->origin, -frame->origin_y, up, point);
  VectorMA(point, -frame->origin_x, right, vertexes[0].position);

  vertexes[1].st[0] = 0;
  vertexes[1].st[1] = 0;
  VectorMA(e->origin, frame->height - frame->origin_y, up, point);
  VectorMA(point, -frame->origin_x, right, vertexes[1].position);

  vertexes[2].st[0] = 1;
  vertexes[2].st[1] = 0;
  VectorMA(e->origin, frame->height - frame->origin_y, up, point);
  VectorMA(point, frame->width - frame->origin_x, right, vertexes[2].position);

  vertexes[3].st[0] = 1;
  vertexes[3].st[1] = 1;
  VectorMA(e->origin, -frame->origin_y, up, point);
  VectorMA(point, frame->width - frame->origin_x, right, vertexes[3].position);

  uint32_t indexes[6] = {0, 1, 2, 0, 2, 3};

  struct GL_Buffer element_buffer =
      GL_allocate_temporary_buffer_from(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * 6, indexes);
  struct GL_Buffer vertex_buffer =
      GL_allocate_temporary_buffer_from(GL_ARRAY_BUFFER, sizeof(struct DrawVertex) * 4, vertexes);

  GL_draw_elements(&draw_state,
                   &(struct GL_DrawAssets){.image[0] = currentmodel->skins[e->frame]->texnum,
                                           .element_buffer = &element_buffer,
                                           .vertex_buffers[0] = &vertex_buffer,
                                           .uniforms[0] = {._float = e->alpha}},
                   6, 1, 0, 0);
}
