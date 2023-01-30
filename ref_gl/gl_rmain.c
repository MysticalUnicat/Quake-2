/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// r_main.c
#include "gl_local.h"

#include "gl_thin.h"

#include <GLFW/glfw3.h>

#include "../client/keys.h"

extern void GL_draw_model_not_found(void);
extern void GL_draw_beam(entity_t *e);
extern void GL_draw_sprite(entity_t *e);
extern void GL_draw_2d_quad(GLuint image, float x1, float y1, float x2, float y2, float s1, float t1, float s2,
                            float t2, float r, float g, float b, float a);

// -------------------------------------------------

// projection * view * model * vertex
static inline void prepare_model_view(void) {
  GL_matrix_multiply(u_view_matrix.data.mat, u_model_matrix.data.mat, u_model_view_matrix.data.mat);
}

static inline void prepare_view_projection(void) {
  GL_matrix_multiply(u_projection_matrix.data.mat, u_view_matrix.data.mat, u_view_projection_matrix.data.mat);
}

static inline void prepare_model_view_projection(void) {
  GL_Uniform_prepare(&u_view_projection_matrix);
  GL_matrix_multiply(u_view_projection_matrix.data.mat, u_model_matrix.data.mat,
                     u_model_view_projection_matrix.data.mat);
}

struct GL_Uniform u_time = {.type = GL_UniformType_Float, .name = "time"};
struct GL_Uniform u_model_matrix = {.type = GL_UniformType_Mat4, .name = "model_matrix"};
struct GL_Uniform u_view_matrix = {.type = GL_UniformType_Mat4, .name = "view_matrix"};
struct GL_Uniform u_model_view_matrix = {
    .type = GL_UniformType_Mat4, .name = "model_view_matrix", .prepare = prepare_model_view};
struct GL_Uniform u_projection_matrix = {.type = GL_UniformType_Mat4, .name = "projection_matrix"};
struct GL_Uniform u_view_projection_matrix = {
    .type = GL_UniformType_Mat4, .name = "view_projection_matrix", .prepare = prepare_view_projection};
struct GL_Uniform u_model_view_projection_matrix = {
    .type = GL_UniformType_Mat4, .name = "model_view_projection_matrix", .prepare = prepare_model_view_projection};

void R_Clear(void);

viddef_t vid;

refimport_t ri;

model_t *r_worldmodel[CMODEL_COUNT];

float gldepthmin, gldepthmax;

glconfig_t gl_config;
glstate_t gl_state;

image_t *r_notexture; // use for bad textures
image_t *r_nonormal;
image_t *r_whitepcx;
image_t *r_particletexture; // little dot for particles

entity_t *currententity;
model_t *currentmodel;

cplane_t frustum[4];

int r_visframecount; // bumped when going to a new PVS
int r_framecount;    // used for dlight push checking

int c_brush_polys, c_alias_polys;

float v_blend[4]; // final blending color

void GL_Strings_f(void);

//
// view origin
//
vec3_t vup;
vec3_t vpn;
vec3_t vright;
vec3_t r_origin;

float r_world_matrix[16];
float r_base_world_matrix[16];

//
// screen size info
//
refdef_t r_newrefdef;

int r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

cvar_t *r_norefresh;
cvar_t *r_drawentities;
cvar_t *r_drawworld;
cvar_t *r_speeds;
cvar_t *r_fullbright;
cvar_t *r_novis;
cvar_t *r_nocull;
cvar_t *r_lerpmodels;
cvar_t *r_lefthand;

cvar_t *r_lightlevel; // FIXME: This is a HACK to get the client's light level

cvar_t *gl_nosubimage;
cvar_t *gl_allow_software;

cvar_t *gl_vertex_arrays;

cvar_t *gl_particle_min_size;
cvar_t *gl_particle_max_size;
cvar_t *gl_particle_size;
cvar_t *gl_particle_att_a;
cvar_t *gl_particle_att_b;
cvar_t *gl_particle_att_c;

cvar_t *gl_ext_swapinterval;
cvar_t *gl_ext_palettedtexture;
cvar_t *gl_ext_multitexture;
cvar_t *gl_ext_pointparameters;
cvar_t *gl_ext_compiled_vertex_array;

cvar_t *gl_log;
cvar_t *gl_bitdepth;
cvar_t *gl_driver;
cvar_t *gl_lightmap;
cvar_t *gl_shadows;
cvar_t *gl_mode;
cvar_t *gl_dynamic;
cvar_t *gl_monolightmap;
cvar_t *gl_modulate;
cvar_t *gl_nobind;
cvar_t *gl_round_down;
cvar_t *gl_picmip;
cvar_t *gl_skymip;
cvar_t *gl_showtris;
cvar_t *gl_ztrick;
cvar_t *gl_finish;
cvar_t *gl_clear;
cvar_t *gl_cull;
cvar_t *gl_polyblend;
cvar_t *gl_flashblend;
cvar_t *gl_playermip;
cvar_t *gl_saturatelighting;
cvar_t *gl_swapinterval;
cvar_t *gl_texturemode;
cvar_t *gl_lockpvs;

cvar_t *gl_3dlabs_broken;

extern cvar_t *vid_fullscreen;
extern cvar_t *vid_gamma;
extern cvar_t *vid_ref;

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
bool R_CullBox(vec3_t mins, vec3_t maxs) {
  int i;

  if(r_nocull->value)
    return false;

  for(i = 0; i < 4; i++)
    if(BOX_ON_PLANE_SIDE(mins, maxs, &frustum[i]) == 2)
      return true;
  return false;
}

void GL_TransformForEntity(entity_t *e) {
  GL_translate(u_model_matrix.data.mat, e->origin[0], e->origin[1], e->origin[2]);
  GL_rotate_z(u_model_matrix.data.mat, e->angles[1]);
  GL_rotate_y(u_model_matrix.data.mat, -e->angles[0]);
  GL_rotate_x(u_model_matrix.data.mat, -e->angles[2]);
}

//==================================================================================

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList(void) {
  int i;

  if(!r_drawentities->value)
    return;

  // draw non-transparent first
  for(i = 0; i < r_newrefdef.num_entities; i++) {
    currententity = &r_newrefdef.entities[i];
    if(currententity->flags & RF_TRANSLUCENT)
      continue; // solid

    if(currententity->flags & RF_BEAM) {
      GL_draw_beam(currententity);
    } else {
      currentmodel = currententity->model;
      if(!currentmodel) {
        GL_draw_model_not_found();
        continue;
      }
      switch(currentmodel->type) {
      case mod_alias:
        R_DrawAliasModel(currententity);
        break;
      case mod_brush:
        R_DrawBrushModel(currententity);
        break;
      case mod_sprite:
        GL_draw_sprite(currententity);
        break;
      default:
        GL_draw_model_not_found();
        // ri.Sys_Error(ERR_DROP, "Bad modeltype");
        break;
      }
    }
  }

  // draw transparent entities
  // we could sort these if it ever becomes a problem...
  for(i = 0; i < r_newrefdef.num_entities; i++) {
    currententity = &r_newrefdef.entities[i];
    if(!(currententity->flags & RF_TRANSLUCENT))
      continue; // solid

    if(currententity->flags & RF_BEAM) {
      GL_draw_beam(currententity);
    } else {
      currentmodel = currententity->model;

      if(!currentmodel) {
        GL_draw_model_not_found();
        continue;
      }
      switch(currentmodel->type) {
      case mod_alias:
        R_DrawAliasModel(currententity);
        break;
      case mod_brush:
        R_DrawBrushModel(currententity);
        break;
      case mod_sprite:
        GL_draw_sprite(currententity);
        break;
      default:
        GL_draw_model_not_found();
        // ri.Sys_Error(ERR_DROP, "Bad modeltype");
        break;
      }
    }
  }
}

//=======================================================================

int SignbitsForPlane(cplane_t *out) {
  int bits, j;

  // for fast box on planeside test

  bits = 0;
  for(j = 0; j < 3; j++) {
    if(out->normal[j] < 0)
      bits |= 1 << j;
  }
  return bits;
}

void R_SetFrustum(void) {
  int i;

  // rotate VPN right by FOV_X/2 degrees
  RotatePointAroundVector(frustum[0].normal, vup, vpn, -(90 - r_newrefdef.fov_x / 2));
  // rotate VPN left by FOV_X/2 degrees
  RotatePointAroundVector(frustum[1].normal, vup, vpn, 90 - r_newrefdef.fov_x / 2);
  // rotate VPN up by FOV_X/2 degrees
  RotatePointAroundVector(frustum[2].normal, vright, vpn, 90 - r_newrefdef.fov_y / 2);
  // rotate VPN down by FOV_X/2 degrees
  RotatePointAroundVector(frustum[3].normal, vright, vpn, -(90 - r_newrefdef.fov_y / 2));

  for(i = 0; i < 4; i++) {
    frustum[i].type = PLANE_ANYZ;
    frustum[i].dist = DotProduct(r_origin, frustum[i].normal);
    frustum[i].signbits = SignbitsForPlane(&frustum[i]);
  }
}

//=======================================================================

/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame(void) {
  int i;
  mleaf_t *leaf;

  r_framecount++;

  u_time.data._float = r_newrefdef.time;

  // build the transformation matrix for the given view angles
  VectorCopy(r_newrefdef.vieworg, r_origin);

  AngleVectors(r_newrefdef.viewangles, vpn, vright, vup);

  // current viewcluster
  if(!(r_newrefdef.rdflags & RDF_NOWORLDMODEL) && r_worldmodel[r_newrefdef.cmodel_index]->type == mod_brush) {
    r_oldviewcluster = r_viewcluster;
    r_oldviewcluster2 = r_viewcluster2;
    leaf = Mod_PointInLeaf(r_origin, r_worldmodel[r_newrefdef.cmodel_index]);
    r_viewcluster = r_viewcluster2 = leaf->cluster;

    // check above and below so crossing solid water doesn't draw wrong
    if(!leaf->contents) { // look down a bit
      vec3_t temp;

      VectorCopy(r_origin, temp);
      temp[2] -= 16;
      leaf = Mod_PointInLeaf(temp, r_worldmodel[r_newrefdef.cmodel_index]);
      if(!(leaf->contents & CONTENTS_SOLID) && (leaf->cluster != r_viewcluster2))
        r_viewcluster2 = leaf->cluster;
    } else { // look up a bit
      vec3_t temp;

      VectorCopy(r_origin, temp);
      temp[2] += 16;
      leaf = Mod_PointInLeaf(temp, r_worldmodel[r_newrefdef.cmodel_index]);
      if(!(leaf->contents & CONTENTS_SOLID) && (leaf->cluster != r_viewcluster2))
        r_viewcluster2 = leaf->cluster;
    }
  }

  for(i = 0; i < 4; i++)
    v_blend[i] = r_newrefdef.blend[i];

  c_brush_polys = 0;
  c_alias_polys = 0;

  // clear out the portion of the screen that the NOWORLDMODEL defines
  if(r_newrefdef.rdflags & RDF_NOWORLDMODEL) {
    glEnable(GL_SCISSOR_TEST);
    glClearColor(0.3, 0.3, 0.3, 1);
    glScissor(r_newrefdef.x, vid.height - r_newrefdef.height - r_newrefdef.y, r_newrefdef.width, r_newrefdef.height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(1, 0, 0.5, 0.5);
    glDisable(GL_SCISSOR_TEST);
  }
}

void MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar) {
  GLdouble xmin, xmax, ymin, ymax;

  ymax = zNear * tan(fovy * M_PI / 360.0);
  ymin = -ymax;

  xmin = ymin * aspect;
  xmax = ymax * aspect;

  xmin += -(2 * gl_state.camera_separation) / zNear;
  xmax += -(2 * gl_state.camera_separation) / zNear;

  GL_matrix_frustum(xmin, xmax, ymin, ymax, zNear, zFar, u_projection_matrix.data.mat);
}

/*
=============
R_SetupGL
=============
*/
void R_SetupGL(void) {
  float screenaspect;
  //	float	yfov;
  int x, x2, y2, y, w, h;

  //
  // set up viewport
  //
  x = floor(r_newrefdef.x * vid.width / vid.width);
  x2 = ceil((r_newrefdef.x + r_newrefdef.width) * vid.width / vid.width);
  y = floor(vid.height - r_newrefdef.y * vid.height / vid.height);
  y2 = ceil(vid.height - (r_newrefdef.y + r_newrefdef.height) * vid.height / vid.height);

  w = x2 - x;
  h = y - y2;

  glViewport(x, y2, w, h);

  //
  // set up projection matrix
  //
  screenaspect = (float)r_newrefdef.width / r_newrefdef.height;
  //	yfov = 2*atan((float)r_newrefdef.height/r_newrefdef.width)*180/M_PI;
  MYgluPerspective(r_newrefdef.fov_y, screenaspect, 4, 4096);

  glCullFace(GL_FRONT);

  // GL_matrix_rotation_x(-90, u_view_matrix.data.mat);
  GL_matrix_identity(u_view_matrix.data.mat);
  GL_rotate_x(u_view_matrix.data.mat, -90);
  GL_rotate_z(u_view_matrix.data.mat, 90);
  GL_rotate_x(u_view_matrix.data.mat, -r_newrefdef.viewangles[2]);
  GL_rotate_y(u_view_matrix.data.mat, -r_newrefdef.viewangles[0]);
  GL_rotate_z(u_view_matrix.data.mat, -r_newrefdef.viewangles[1]);
  GL_translate(u_view_matrix.data.mat, -r_newrefdef.vieworg[0], -r_newrefdef.vieworg[1], -r_newrefdef.vieworg[2]);

  //
  // set drawing parms
  //
  if(gl_cull->value)
    glEnable(GL_CULL_FACE);
  else
    glDisable(GL_CULL_FACE);
}

/*
=============
R_Clear
=============
*/
void R_Clear(void) {
  GLboolean old_mask;
  glGetBooleanv(GL_DEPTH_WRITEMASK, &old_mask);

  glDepthMask(GL_TRUE);

  if(gl_clear->value || true)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  else
    glClear(GL_DEPTH_BUFFER_BIT);

  glDepthFunc(GL_LEQUAL);

  glDepthMask(old_mask);
}

void R_Flash(void) {
  GL_draw_2d_quad(r_whitepcx->texnum, 0, 0, vid.width, vid.height, 0, 0, 1, 1, r_newrefdef.blend[0],
                  r_newrefdef.blend[1], r_newrefdef.blend[2], r_newrefdef.blend[3]);
}

/*
================
R_RenderView

r_newrefdef must be set before the first call
================
*/
void R_RenderView(refdef_t *fd) {
  if(r_norefresh->value)
    return;

  r_newrefdef = *fd;

  if(!r_worldmodel[r_newrefdef.cmodel_index] && !(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
    ri.Sys_Error(ERR_DROP, "R_RenderView: NULL worldmodel");

  if(r_speeds->value) {
    c_brush_polys = 0;
    c_alias_polys = 0;
  }

  R_PushDlights(r_newrefdef.cmodel_index);

  if(gl_finish->value)
    glFinish();

  R_SetupFrame();

  R_SetFrustum();

  R_SetupGL();

  R_MarkLeaves(r_newrefdef.cmodel_index); // done here so we know if we're in water

  R_DrawWorld(r_newrefdef.cmodel_index);

  R_DrawEntitiesOnList();

  GL_draw_particles();

  R_DrawAlphaSurfaces();

  if(r_speeds->value) {
    ri.Con_Printf(PRINT_ALL, "%4i wpoly %4i epoly %i tex %i lmaps\n", c_brush_polys, c_alias_polys, c_visible_textures,
                  c_visible_lightmaps);
  }
}

void R_SetGL2D(void) {
  GL_matrix_ortho(0, vid.width, vid.height, 0, -99999, 99999, u_projection_matrix.data.mat);
  GL_matrix_identity(u_view_matrix.data.mat);

  // TODO remove
  glDisable(GL_CULL_FACE);
}

/*
====================
R_SetLightLevel

====================
*/
void R_SetLightLevel(void) {
  struct SH1 shadelight;

  if(r_newrefdef.rdflags & RDF_NOWORLDMODEL)
    return;

  // save off light value for server to look at (BIG HACK!)

  shadelight = R_LightPoint(r_newrefdef.cmodel_index, r_newrefdef.vieworg);

  // pick the greatest component, which should be the same
  // as the mono value returned by software
  SH1_Normalize(shadelight, &r_lightlevel->value);
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
void R_RenderFrame(refdef_t *fd) {
  R_RenderView(fd);
  R_SetLightLevel();
  R_SetGL2D();
  R_Flash();
}

void R_Register(void) {
  r_lefthand = ri.Cvar_Get("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE);
  r_norefresh = ri.Cvar_Get("r_norefresh", "0", 0);
  r_fullbright = ri.Cvar_Get("r_fullbright", "0", 0);
  r_drawentities = ri.Cvar_Get("r_drawentities", "1", 0);
  r_drawworld = ri.Cvar_Get("r_drawworld", "1", 0);
  r_novis = ri.Cvar_Get("r_novis", "0", 0);
  r_nocull = ri.Cvar_Get("r_nocull", "0", 0);
  r_lerpmodels = ri.Cvar_Get("r_lerpmodels", "1", 0);
  r_speeds = ri.Cvar_Get("r_speeds", "0", 0);

  r_lightlevel = ri.Cvar_Get("r_lightlevel", "0", 0);

  gl_nosubimage = ri.Cvar_Get("gl_nosubimage", "0", 0);
  gl_allow_software = ri.Cvar_Get("gl_allow_software", "0", 0);

  gl_particle_min_size = ri.Cvar_Get("gl_particle_min_size", "2", CVAR_ARCHIVE);
  gl_particle_max_size = ri.Cvar_Get("gl_particle_max_size", "40", CVAR_ARCHIVE);
  gl_particle_size = ri.Cvar_Get("gl_particle_size", "40", CVAR_ARCHIVE);
  gl_particle_att_a = ri.Cvar_Get("gl_particle_att_a", "0.01", CVAR_ARCHIVE);
  gl_particle_att_b = ri.Cvar_Get("gl_particle_att_b", "0.0", CVAR_ARCHIVE);
  gl_particle_att_c = ri.Cvar_Get("gl_particle_att_c", "0.01", CVAR_ARCHIVE);

  gl_modulate = ri.Cvar_Get("gl_modulate", "1", CVAR_ARCHIVE);
  gl_log = ri.Cvar_Get("gl_log", "0", 0);
  gl_bitdepth = ri.Cvar_Get("gl_bitdepth", "0", 0);
  gl_mode = ri.Cvar_Get("gl_mode", "3", CVAR_ARCHIVE);
  gl_lightmap = ri.Cvar_Get("gl_lightmap", "0", 0);
  gl_shadows = ri.Cvar_Get("gl_shadows", "0", CVAR_ARCHIVE);
  gl_dynamic = ri.Cvar_Get("gl_dynamic", "1", 0);
  gl_nobind = ri.Cvar_Get("gl_nobind", "0", 0);
  gl_round_down = ri.Cvar_Get("gl_round_down", "1", 0);
  gl_picmip = ri.Cvar_Get("gl_picmip", "0", 0);
  gl_skymip = ri.Cvar_Get("gl_skymip", "0", 0);
  gl_showtris = ri.Cvar_Get("gl_showtris", "0", 0);
  gl_ztrick = ri.Cvar_Get("gl_ztrick", "0", 0);
  gl_finish = ri.Cvar_Get("gl_finish", "0", CVAR_ARCHIVE);
  gl_clear = ri.Cvar_Get("gl_clear", "0", 0);
  gl_cull = ri.Cvar_Get("gl_cull", "1", 0);
  gl_polyblend = ri.Cvar_Get("gl_polyblend", "1", 0);
  gl_flashblend = ri.Cvar_Get("gl_flashblend", "0", 0);
  gl_playermip = ri.Cvar_Get("gl_playermip", "0", 0);
  gl_monolightmap = ri.Cvar_Get("gl_monolightmap", "0", 0);
  gl_driver = ri.Cvar_Get("gl_driver", "opengl32", CVAR_ARCHIVE);
  gl_texturemode = ri.Cvar_Get("gl_texturemode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE);
  gl_lockpvs = ri.Cvar_Get("gl_lockpvs", "0", 0);

  gl_vertex_arrays = ri.Cvar_Get("gl_vertex_arrays", "0", CVAR_ARCHIVE);

  gl_ext_swapinterval = ri.Cvar_Get("gl_ext_swapinterval", "1", CVAR_ARCHIVE);
  gl_ext_palettedtexture = ri.Cvar_Get("gl_ext_palettedtexture", "1", CVAR_ARCHIVE);
  gl_ext_multitexture = ri.Cvar_Get("gl_ext_multitexture", "1", CVAR_ARCHIVE);
  gl_ext_pointparameters = ri.Cvar_Get("gl_ext_pointparameters", "1", CVAR_ARCHIVE);
  gl_ext_compiled_vertex_array = ri.Cvar_Get("gl_ext_compiled_vertex_array", "1", CVAR_ARCHIVE);

  gl_swapinterval = ri.Cvar_Get("gl_swapinterval", "1", CVAR_ARCHIVE);

  gl_saturatelighting = ri.Cvar_Get("gl_saturatelighting", "0", 0);

  gl_3dlabs_broken = ri.Cvar_Get("gl_3dlabs_broken", "1", CVAR_ARCHIVE);

  vid_fullscreen = ri.Cvar_Get("vid_fullscreen", "0", CVAR_ARCHIVE);
  vid_gamma = ri.Cvar_Get("vid_gamma", "1.0", CVAR_ARCHIVE);
  vid_ref = ri.Cvar_Get("vid_ref", "soft", CVAR_ARCHIVE);

  ri.Cmd_AddCommand("imagelist", GL_ImageList_f);
  ri.Cmd_AddCommand("screenshot", GL_ScreenShot_f);
  ri.Cmd_AddCommand("modellist", Mod_Modellist_f);
  ri.Cmd_AddCommand("gl_strings", GL_Strings_f);
}

/*
==================
R_SetMode
==================
*/
bool R_SetMode(void) {
  rserr_t err;
  bool fullscreen;

  if(vid_fullscreen->modified && !gl_config.allow_cds) {
    ri.Con_Printf(PRINT_ALL, "R_SetMode() - CDS not allowed with this driver\n");
    ri.Cvar_SetValue("vid_fullscreen", !vid_fullscreen->value);
    vid_fullscreen->modified = false;
  }

  fullscreen = vid_fullscreen->value;

  vid_fullscreen->modified = false;
  gl_mode->modified = false;

  if((err = GLimp_SetMode(&vid.width, &vid.height, gl_mode->value, fullscreen)) == rserr_ok) {
    gl_state.prev_mode = gl_mode->value;
  } else {
    if(err == rserr_invalid_fullscreen) {
      ri.Cvar_SetValue("vid_fullscreen", 0);
      vid_fullscreen->modified = false;
      ri.Con_Printf(PRINT_ALL, "ref_gl::R_SetMode() - fullscreen unavailable in this mode\n");
      if((err = GLimp_SetMode(&vid.width, &vid.height, gl_mode->value, false)) == rserr_ok)
        return true;
    } else if(err == rserr_invalid_mode) {
      ri.Cvar_SetValue("gl_mode", gl_state.prev_mode);
      gl_mode->modified = false;
      ri.Con_Printf(PRINT_ALL, "ref_gl::R_SetMode() - invalid mode\n");
    }

    // try setting it back to something safe
    if((err = GLimp_SetMode(&vid.width, &vid.height, gl_state.prev_mode, false)) != rserr_ok) {
      ri.Con_Printf(PRINT_ALL, "ref_gl::R_SetMode() - could not revert to safe mode\n");
      return false;
    }
  }
  return true;
}

static inline void debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                  const GLchar *message, const void *userParam) {
  if(type == GL_DEBUG_TYPE_ERROR || type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR ||
     type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR || type == GL_DEBUG_TYPE_PORTABILITY) {
    ri.Con_Printf(PRINT_ALL, "OPENGL debug error - %s\n",
                  message); // 0x7ffde58c97d0 "GL_INVALID_OPERATION error generated. <vaobj> does not refer to an
                            // existing vertex array object."
  }
  if(type == GL_DEBUG_TYPE_PERFORMANCE) {
    ri.Con_Printf(PRINT_ALL, "OPENGL performance note - %s\n", message);
  }
  if(type == GL_DEBUG_TYPE_OTHER) {
    ri.Con_Printf(PRINT_ALL, "OPENGL other note - %s\n", message);
  }
  if(type == GL_DEBUG_TYPE_MARKER) {
    ri.Con_Printf(PRINT_ALL, "OPENGL marker - %s\n", message);
  }
}

/*
===============
R_Init
===============
*/
bool R_Init(void *hinstance, void *hWnd) {
  char renderer_buffer[1000];
  char vendor_buffer[1000];
  int err;
  int j;

  ri.Con_Printf(PRINT_ALL, "ref_gl version: " REF_VERSION "\n");

  Draw_GetPalette();

  R_Register();

  // initialize OS-specific parts of OpenGL
  if(!GLimp_Init(hinstance, hWnd)) {
    return -1;
  }

  // set our "safe" modes
  gl_state.prev_mode = 3;

  // create the window and set up the context
  if(!R_SetMode()) {
    ri.Con_Printf(PRINT_ALL, "ref_gl::R_Init() - could not R_SetMode()\n");
    return -1;
  }

  // initialize our QGL dynamic bindings
  if(!QGL_Init()) {
    ri.Con_Printf(PRINT_ALL, "ref_gl::R_Init() - could not load \"%s\"\n", gl_driver->string);
    return -1;
  }

  if(glDebugMessageCallback != NULL) {
    glDebugMessageCallback(debug_callback, NULL);
  }

  ri.Vid_MenuInit();

  /*
  ** get our various GL strings
  */
  gl_config.vendor_string = glGetString(GL_VENDOR);
  ri.Con_Printf(PRINT_ALL, "GL_VENDOR: %s\n", gl_config.vendor_string);
  gl_config.renderer_string = glGetString(GL_RENDERER);
  ri.Con_Printf(PRINT_ALL, "GL_RENDERER: %s\n", gl_config.renderer_string);
  gl_config.version_string = glGetString(GL_VERSION);
  ri.Con_Printf(PRINT_ALL, "GL_VERSION: %s\n", gl_config.version_string);

  ri.Con_Printf(PRINT_ALL, "GL_EXTENSIONS:");
  GLint num_extensions;
  glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
  for(GLint i = 0; i < num_extensions; i++) {
    const char *name = glGetStringi(GL_EXTENSIONS, i);
    ri.Con_Printf(PRINT_ALL, " %s", name);
  }

  strcpy(renderer_buffer, gl_config.renderer_string);
  strlwr(renderer_buffer);

  strcpy(vendor_buffer, gl_config.vendor_string);
  strlwr(vendor_buffer);

  GL_SetDefaultState();

  GL_InitImages();
  Mod_Init();
  R_InitParticleTexture();
  Draw_InitLocal();

  err = glGetError();
  if(err != GL_NO_ERROR)
    ri.Con_Printf(PRINT_ALL, "glGetError() = 0x%x\n", err);
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown(void) {
  ri.Cmd_RemoveCommand("modellist");
  ri.Cmd_RemoveCommand("screenshot");
  ri.Cmd_RemoveCommand("imagelist");
  ri.Cmd_RemoveCommand("gl_strings");

  Mod_FreeAll();

  GL_ShutdownImages();

  /*
  ** shut down OS specific OpenGL stuff like contexts, etc.
  */
  GLimp_Shutdown();

  /*
  ** shutdown our QGL subsystem
  */
  QGL_Shutdown();
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginFrame
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginFrame(float camera_separation) {

  gl_state.camera_separation = camera_separation;

  /*
  ** change modes if necessary
  */
  if(gl_mode->modified || vid_fullscreen->modified) { // FIXME: only restart if CDS is required
    cvar_t *ref;

    ref = ri.Cvar_Get("vid_ref", "gl", 0);
    ref->modified = true;
  }

  if(gl_log->modified) {
    GLimp_EnableLogging(gl_log->value);
    gl_log->modified = false;
  }

  if(gl_log->value) {
    GLimp_LogNewFrame();
  }

  /*
  ** update 3Dfx gamma -- it is expected that a user will do a vid_restart
  ** after tweaking this value
  */
  if(vid_gamma->modified) {
    vid_gamma->modified = false;

    if(gl_config.renderer & (GL_RENDERER_VOODOO)) {
      char envbuffer[1024];
      float g;

      g = 2.00 * (0.8 - (vid_gamma->value - 0.5)) + 1.0F;
      Com_sprintf(envbuffer, sizeof(envbuffer), "SSTV2_GAMMA=%f", g);
      putenv(envbuffer);
      Com_sprintf(envbuffer, sizeof(envbuffer), "SST_GAMMA=%f", g);
      putenv(envbuffer);
    }
  }

  GLimp_BeginFrame(camera_separation);

  /*
  ** go into 2D mode
  */
  glViewport(0, 0, vid.width, vid.height);
  GL_matrix_ortho(0, vid.width, vid.height, 0, -99999, 99999, u_projection_matrix.data.mat);
  GL_matrix_identity(u_view_matrix.data.mat);

  // TODO
  glDisable(GL_CULL_FACE);

  /*
  ** texturemode stuff
  */
  if(gl_texturemode->modified) {
    GL_TextureMode(gl_texturemode->string);
    gl_texturemode->modified = false;
  }

  //
  // clear screen if desired
  //
  R_Clear();

  GL_reset_temporary_buffers();
}

/*
=============
R_SetPalette
=============
*/
unsigned r_rawpalette[256];

void R_SetPalette(const unsigned char *palette) {
  int i;

  byte *rp = (byte *)r_rawpalette;

  if(palette) {
    for(i = 0; i < 256; i++) {
      rp[i * 4 + 0] = palette[i * 3 + 0];
      rp[i * 4 + 1] = palette[i * 3 + 1];
      rp[i * 4 + 2] = palette[i * 3 + 2];
      rp[i * 4 + 3] = 0xff;
    }
  } else {
    for(i = 0; i < 256; i++) {
      rp[i * 4 + 0] = d_8to24table[i] & 0xff;
      rp[i * 4 + 1] = (d_8to24table[i] >> 8) & 0xff;
      rp[i * 4 + 2] = (d_8to24table[i] >> 16) & 0xff;
      rp[i * 4 + 3] = 0xff;
    }
  }
  GL_SetTexturePalette(r_rawpalette);

  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(1, 0, 0.5, 0.5);
}

//===================================================================

void R_BeginRegistration(const char *map);
struct model_s *R_RegisterModel(int cmodel_index, const char *name);
struct BaseImage *R_RegisterSkin(const char *name);
void R_SetSky(const char *name, float rotate, vec3_t axis);
void R_EndRegistration(void);

void R_RenderFrame(refdef_t *fd);

struct BaseImage *Draw_FindPic(const char *name);

void Draw_Pic(int x, int y, const char *name);
void Draw_Char(int x, int y, int c);
void Draw_TileClear(int x, int y, int w, int h, const char *name);
void Draw_Fill(int x, int y, int w, int h, int c);
void Draw_FadeScreen(void);
void Draw_Triangles(const struct BaseImage *image, const struct DrawVertex *vertexes, uint32_t num_vertexes,
                    const uint32_t *indexes, uint32_t num_indexes);

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
refexport_t GetRefAPI(refimport_t rimp) {
  extern void GL_set_sky(uint32_t cmodel_index, const char *name, float rotate, vec3_t axis);

  refexport_t re;

  ri = rimp;

  re.api_version = API_VERSION;

  re.BeginRegistration = R_BeginRegistration;
  re.RegisterModel = R_RegisterModel;
  re.RegisterSkin = R_RegisterSkin;
  re.RegisterPic = Draw_FindPic;
  re.SetSky = GL_set_sky;
  re.EndRegistration = R_EndRegistration;

  re.RenderFrame = R_RenderFrame;

  re.DrawGetPicSize = Draw_GetPicSize;
  re.DrawPic = Draw_Pic;
  re.DrawStretchPic = Draw_StretchPic;
  re.DrawChar = Draw_Char;
  re.DrawTileClear = Draw_TileClear;
  re.DrawFill = Draw_Fill;
  re.DrawFadeScreen = Draw_FadeScreen;

  re.DrawStretchRaw = Draw_StretchRaw;
  re.DrawTriangles = Draw_Triangles;

  re.Init = R_Init;
  re.Shutdown = R_Shutdown;

  re.CinematicSetPalette = R_SetPalette;
  re.BeginFrame = R_BeginFrame;
  re.EndFrame = GLimp_EndFrame;

  re.AppActivate = GLimp_AppActivate;

  Swap_Init();

  return re;
}

#ifndef REF_HARD_LINKED
// this is only here so the functions in q_shared.c and q_shwin.c can link
void Sys_Error(char *error, ...) {
  va_list argptr;
  char text[1024];

  va_start(argptr, error);
  vsprintf(text, error, argptr);
  va_end(argptr);

  ri.Sys_Error(ERR_FATAL, "%s", text);
}

void Com_Printf(char *fmt, ...) {
  va_list argptr;
  char text[1024];

  va_start(argptr, fmt);
  vsprintf(text, fmt, argptr);
  va_end(argptr);

  ri.Con_Printf(PRINT_ALL, "%s", text);
}

#endif
