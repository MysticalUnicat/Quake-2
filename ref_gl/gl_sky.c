#include "gl_local.h"

#include "gl_thin.h"

// clang-format off
static char vertex_shader_source[] =
GL_MSTR(
  layout(location = 0) out vec2 out_st;

  void main() {
    gl_Position = u_model_view_projection_matrix * vec4(in_position, 1);
    out_st = in_st;
  }
);

static char fragment_shader_source[] =
GL_MSTR(
  layout(location = 0) in vec2 in_st;

  layout(location = 0) out vec4 out_color;

  void main() {
    out_color = texture(u_sky_map, in_st);
  }
);
// clang-format on

static struct DrawState draw_state = {.primitive = GL_TRIANGLES,
                                      .attribute[0] = {0, alias_memory_Format_Float32, 3, "position", 0},
                                      .attribute[1] = {0, alias_memory_Format_Float32, 2, "st", 12},
                                      .binding[0] = {sizeof(float) * 3 + sizeof(float) * 2, 0},
                                      .image[0] = {THIN_GL_FRAGMENT_BIT, GL_ImageType_Sampler2D, "sky_map"},
                                      .global[0] = {THIN_GL_VERTEX_BIT, &u_model_view_projection_matrix},
                                      .vertex_shader_source = vertex_shader_source,
                                      .fragment_shader_source = fragment_shader_source,
                                      .depth_mask = false,
                                      .depth_test_enable = true,
                                      .depth_range_min = 0,
                                      .depth_range_max = 1};

static bool init = false;
static struct GL_Buffer vertex_buffer;
static struct GL_Buffer index_buffer;

static struct SideInfo { const char *suffix; } side_info[6] = {{"rt"}, {"lf"}, {"bk"}, {"ft"}, {"up"}, {"dn"}};

struct Sky {
  char name[MAX_QPATH];
  float rotate;
  vec3_t axis;
  image_t *image[6];
} sky[CMODEL_COUNT];

void GL_draw_sky(uint32_t cmodel_index) {
  GL_matrix_translation(r_origin[0], r_origin[1], r_origin[2], u_model_matrix.data.mat);
  GL_rotate(u_model_matrix.data.mat, r_newrefdef.time * sky[cmodel_index].rotate, sky[cmodel_index].axis[0],
            sky[cmodel_index].axis[1], sky[cmodel_index].axis[2]);

  for(uint32_t i = 0; i < 6; i++) {
    // if(i == 3)
    //   continue;
    GL_draw_elements(&draw_state,
                     &(struct DrawAssets){.element_buffer = &index_buffer,
                                          .vertex_buffers[0] = &vertex_buffer,
                                          .image[0] = sky[cmodel_index].image[i]->texnum},
                     6, 1, 4 * i, 0);
  }
}

void GL_set_sky(uint32_t cmodel_index, const char *name, float rotate, vec3_t axis) {
  if(!init) {
    float min_st = 1.0f / 512.0f;
    float max_st = 1.0f - min_st;

    struct {
      float x;
      float y;
      float z;
      float s;
      float t;
    } vertex_data[] = {
        // axis 0
        {2300, 2300, -2300, min_st, max_st},
        {2300, 2300, 2300, min_st, min_st},
        {2300, -2300, 2300, max_st, min_st},
        {2300, -2300, -2300, max_st, max_st},

        // axis 1
        {-2300, -2300, -2300, min_st, max_st},
        {-2300, -2300, 2300, min_st, min_st},
        {-2300, 2300, 2300, max_st, min_st},
        {-2300, 2300, -2300, max_st, max_st},

        // axis 2
        {-2300, 2300, -2300, min_st, max_st},
        {-2300, 2300, 2300, min_st, min_st},
        {2300, 2300, 2300, max_st, min_st},
        {2300, 2300, -2300, max_st, max_st},

        // axis 3
        {2300, -2300, -2300, min_st, max_st},
        {2300, -2300, 2300, min_st, min_st},
        {-2300, -2300, 2300, max_st, min_st},
        {-2300, -2300, -2300, max_st, max_st},

        // axis 4
        {2300, 2300, 2300, min_st, max_st},
        {-2300, 2300, 2300, min_st, min_st},
        {-2300, -2300, 2300, max_st, min_st},
        {2300, -2300, 2300, max_st, max_st},

        // axis 5
        {-2300, 2300, -2300, min_st, max_st},
        {2300, 2300, -2300, min_st, min_st},
        {2300, -2300, -2300, max_st, min_st},
        {-2300, -2300, -2300, max_st, max_st},
    };

    uint32_t index_data[] = {
        0, 1, 2, 0, 2, 3,
    };

    index_buffer = GL_allocate_static_buffer(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_data), index_data);
    vertex_buffer = GL_allocate_static_buffer(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data);

    init = true;
  }

  strncpy(sky[cmodel_index].name, name, sizeof(sky[0].name) - 1);
  sky[cmodel_index].rotate = rotate;
  VectorCopy(axis, sky[cmodel_index].axis);

  for(uint32_t i = 0; i < 6; i++) {
    char path[MAX_QPATH];

    snprintf(path, sizeof(path), "env/%s%s.tga", sky[cmodel_index].name, side_info[i].suffix);

    sky[cmodel_index].image[i] = GL_FindImage(path, it_sky);
    if(sky[cmodel_index].image[i] == NULL) {
      sky[cmodel_index].image[i] = r_notexture;
    }
  }
}
