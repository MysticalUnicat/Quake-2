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
// r_light.c

#include "gl_local.h"

int r_dlightframecount;

#define DLIGHT_CUTOFF 64

/*
=============================================================================

DYNAMIC LIGHTS BLEND RENDERING

=============================================================================
*/

void R_RenderDlight(dlight_t *light) {
  int i, j;
  float a;
  vec3_t v;
  float rad;

  rad = light->intensity * 0.35;

  VectorSubtract(light->origin, r_origin, v);
#if 0
	// FIXME?
	if (VectorLength (v) < rad)
	{	// view is inside the dlight
		V_AddBlend (light->color[0], light->color[1], light->color[2], light->intensity * 0.0003, v_blend);
		return;
	}
#endif

  glBegin(GL_TRIANGLE_FAN);
  glColor3f(light->color[0] * 0.2, light->color[1] * 0.2, light->color[2] * 0.2);
  for(i = 0; i < 3; i++)
    v[i] = light->origin[i] - vpn[i] * rad;
  glVertex3fv(v);
  glColor3f(0, 0, 0);
  for(i = 16; i >= 0; i--) {
    a = i / 16.0 * M_PI * 2;
    for(j = 0; j < 3; j++)
      v[j] = light->origin[j] + vright[j] * cos(a) * rad + vup[j] * sin(a) * rad;
    glVertex3fv(v);
  }
  glEnd();
}

/*
=============
R_RenderDlights
=============
*/
void R_RenderDlights(void) {
  int i;
  dlight_t *l;

  if(!gl_flashblend->value)
    return;

  r_dlightframecount = r_framecount + 1; // because the count hasn't
                                         //  advanced yet for this frame
  glDepthMask(0);
  glDisable(GL_TEXTURE_2D);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);

  l = r_newrefdef.dlights;
  for(i = 0; i < r_newrefdef.num_dlights; i++, l++)
    R_RenderDlight(l);

  glColor3f(1, 1, 1);
  glDisable(GL_BLEND);
  glEnable(GL_TEXTURE_2D);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(1);
}

/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
R_MarkLights
=============
*/
void R_MarkLights(int cmodel_index, dlight_t *light, int bit, mnode_t *node) {
  cplane_t *splitplane;
  float dist;
  msurface_t *surf;
  int i;

  if(r_worldmodel[cmodel_index]->type != mod_brush)
    return;

  if(node->contents != -1)
    return;

  splitplane = node->plane;
  dist = DotProduct(light->origin, splitplane->normal) - splitplane->dist;

  if(dist > light->intensity - DLIGHT_CUTOFF) {
    R_MarkLights(cmodel_index, light, bit, node->children[0]);
    return;
  }
  if(dist < -light->intensity + DLIGHT_CUTOFF) {
    R_MarkLights(cmodel_index, light, bit, node->children[1]);
    return;
  }

  // mark the polygons
  surf = r_worldmodel[cmodel_index]->surfaces + node->firstsurface;
  for(i = 0; i < node->numsurfaces; i++, surf++) {
    if(surf->dlightframe != r_dlightframecount) {
      surf->dlightbits = 0;
      surf->dlightframe = r_dlightframecount;
    }
    surf->dlightbits |= bit;
  }

  R_MarkLights(cmodel_index, light, bit, node->children[0]);
  R_MarkLights(cmodel_index, light, bit, node->children[1]);
}

/*
=============
R_PushDlights
=============
*/
void R_PushDlights(int cmodel_index) {
  int i;
  dlight_t *l;

  if(gl_flashblend->value)
    return;

  r_dlightframecount = r_framecount + 1; // because the count hasn't
                                         //  advanced yet for this frame
  l = r_newrefdef.dlights;
  for(i = 0; i < r_newrefdef.num_dlights; i++, l++)
    R_MarkLights(cmodel_index, l, 1 << i, r_worldmodel[cmodel_index]->nodes);
}

/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

struct SH1 pointcolor;
cplane_t *lightplane; // used as shadow plane
vec3_t lightspot;

int RecursiveLightPoint(int cmodel_index, mnode_t *node, vec3_t start, vec3_t end) {
  float front, back, frac;
  int side;
  cplane_t *plane;
  vec3_t mid;
  msurface_t *surf;
  int s, t, ds, dt;
  int i;
  mtexinfo_t *tex;
  byte *lightmap;
  int maps;
  int r;

  if(node->contents != -1)
    return -1; // didn't hit anything

  // calculate mid point

  // FIXME: optimize for axial
  plane = node->plane;
  front = DotProduct(start, plane->normal) - plane->dist;
  back = DotProduct(end, plane->normal) - plane->dist;
  side = front < 0;

  if((back < 0) == side)
    return RecursiveLightPoint(cmodel_index, node->children[side], start, end);

  frac = front / (front - back);
  mid[0] = start[0] + (end[0] - start[0]) * frac;
  mid[1] = start[1] + (end[1] - start[1]) * frac;
  mid[2] = start[2] + (end[2] - start[2]) * frac;

  // go down front side
  r = RecursiveLightPoint(cmodel_index, node->children[side], start, mid);
  if(r >= 0)
    return r; // hit something

  if((back < 0) == side)
    return -1; // didn't hit anuthing

  // check for impact on this node
  VectorCopy(mid, lightspot);
  lightplane = plane;

  surf = r_worldmodel[cmodel_index]->surfaces + node->firstsurface;
  for(i = 0; i < node->numsurfaces; i++, surf++) {
    if(surf->flags & (SURF_DRAWTURB | SURF_DRAWSKY))
      continue; // no lightmaps

    tex = surf->texinfo;

    s = DotProduct(mid, tex->vecs[0]) + tex->vecs[0][3];
    t = DotProduct(mid, tex->vecs[1]) + tex->vecs[1][3];
    ;

    if(s < surf->texturemins[0] || t < surf->texturemins[1])
      continue;

    ds = s - surf->texturemins[0];
    dt = t - surf->texturemins[1];

    if(ds > surf->extents[0] || dt > surf->extents[1])
      continue;

    if(!surf->samples)
      return 0;

    ds >>= 4;
    dt >>= 4;

    lightmap = surf->samples;
    pointcolor = SH1_Clear();
    if(lightmap) {
      vec3_t scale;

      lightmap += 3 * (dt * ((surf->extents[0] >> 4) + 1) + ds);

      for(maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++) {
        for(i = 0; i < 3; i++)
          scale[i] = gl_modulate->value * r_newrefdef.lightstyles[surf->styles[maps]].rgb[i];

        pointcolor.f[0] += lightmap[0] * scale[0] * (1.0 / 255);
        pointcolor.f[4] += lightmap[1] * scale[1] * (1.0 / 255);
        pointcolor.f[8] += lightmap[2] * scale[2] * (1.0 / 255);
        lightmap += 3 * ((surf->extents[0] >> 4) + 1) * ((surf->extents[1] >> 4) + 1);

        pointcolor.f[1] += lightmap[0] * scale[0] * (1.0 / 255) * 2.0f - 1.0f;
        pointcolor.f[2] += lightmap[1] * scale[1] * (1.0 / 255) * 2.0f - 1.0f;
        pointcolor.f[3] += lightmap[2] * scale[2] * (1.0 / 255) * 2.0f - 1.0f;
        lightmap += 3 * ((surf->extents[0] >> 4) + 1) * ((surf->extents[1] >> 4) + 1);

        pointcolor.f[5] += lightmap[0] * scale[0] * (1.0 / 255) * 2.0f - 1.0f;
        pointcolor.f[6] += lightmap[1] * scale[1] * (1.0 / 255) * 2.0f - 1.0f;
        pointcolor.f[7] += lightmap[2] * scale[2] * (1.0 / 255) * 2.0f - 1.0f;
        lightmap += 3 * ((surf->extents[0] >> 4) + 1) * ((surf->extents[1] >> 4) + 1);

        pointcolor.f[9] += (lightmap[0] * scale[0] * (1.0 / 255)) * 2.0f - 1.0f;
        pointcolor.f[10] += (lightmap[1] * scale[1] * (1.0 / 255)) * 2.0f - 1.0f;
        pointcolor.f[11] += (lightmap[2] * scale[2] * (1.0 / 255)) * 2.0f - 1.0f;
        lightmap += 3 * ((surf->extents[0] >> 4) + 1) * ((surf->extents[1] >> 4) + 1);
      }
    }

    return 1;
  }

  // go down back side
  return RecursiveLightPoint(cmodel_index, node->children[!side], mid, end);
}

/*
===============
R_LightPoint
===============
*/
struct SH1 R_LightPoint(int cmodel_index, vec3_t p) {
  vec3_t end;
  float r;
  int lnum;
  dlight_t *dl;
  float light;
  vec3_t dist;
  float add;
  struct SH1 color;

  if(!r_worldmodel[cmodel_index]->lightdata) {
    return SH1_FromDirectionalLight(vec3_origin, (float[]){1, 1, 1});
  }

  end[0] = p[0];
  end[1] = p[1];
  end[2] = p[2] - 2048;

  r = RecursiveLightPoint(cmodel_index, r_worldmodel[cmodel_index]->nodes, p, end);

  if(r == -1) {
    return SH1_Clear();
  } else {
    color = pointcolor;
  }

  //
  // add dynamic lights
  //
  light = 0;
  dl = r_newrefdef.dlights;
  for(lnum = 0; lnum < r_newrefdef.num_dlights; lnum++, dl++) {
    VectorSubtract(currententity->origin, dl->origin, dist);
    float length = VectorNormalize(dist);
    add = dl->intensity - length;
    add *= (1.0 / 256);
    if(add > 0) {
      color = SH1_Add(color, SH1_FromDirectionalLight(dist, dl->color));
      // VectorMA(color, add, dl->color, color);
    }
  }

  // VectorScale(color, gl_modulate->value, color);
  return SH1_Scale(color, gl_modulate->value);
}

//===================================================================

static struct SH1 s_blocklights[34 * 34];
/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLights(msurface_t *surf) {
  int lnum;
  int sd, td;
  float fdist, frad, fminlight;
  vec3_t impact, local;
  int s, t;
  int i;
  int smax, tmax;
  mtexinfo_t *tex;
  dlight_t *dl;
  struct SH1 *pfBL;
  float fsacc, ftacc;

  smax = (surf->extents[0] >> 4) + 1;
  tmax = (surf->extents[1] >> 4) + 1;
  tex = surf->texinfo;

  for(lnum = 0; lnum < r_newrefdef.num_dlights; lnum++) {
    if(!(surf->dlightbits & (1 << lnum)))
      continue; // not lit by this light

    dl = &r_newrefdef.dlights[lnum];
    frad = dl->intensity;
    fdist = DotProduct(dl->origin, surf->plane->normal) - surf->plane->dist;
    frad -= fabs(fdist);
    // rad is now the highest intensity on the plane

    fminlight = DLIGHT_CUTOFF; // FIXME: make configurable?
    if(frad < fminlight)
      continue;
    fminlight = frad - fminlight;

    for(i = 0; i < 3; i++) {
      impact[i] = dl->origin[i] - surf->plane->normal[i] * fdist;
    }

    local[0] = DotProduct(impact, tex->vecs[0]) + tex->vecs[0][3] - surf->texturemins[0];
    local[1] = DotProduct(impact, tex->vecs[1]) + tex->vecs[1][3] - surf->texturemins[1];

    pfBL = s_blocklights;
    for(t = 0, ftacc = 0; t < tmax; t++, ftacc += 16) {
      td = local[1] - ftacc;
      if(td < 0)
        td = -td;

      for(s = 0, fsacc = 0; s < smax; s++, fsacc += 16, pfBL += 3) {
        sd = (int)(local[0] - fsacc);

        if(sd < 0)
          sd = -sd;

        if(sd > td)
          fdist = sd + (td >> 1);
        else
          fdist = td + (sd >> 1);

        if(fdist < fminlight) {
          *pfBL = SH1_Add(*pfBL, SH1_Scale(SH1_FromDirectionalLight(vec3_origin, dl->color), frad - fdist));
        }
      }
    }
  }
}

/*
** R_SetCacheState
*/
void R_SetCacheState(msurface_t *surf) {
  int maps;

  for(maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++) {
    surf->cached_light[maps] = r_newrefdef.lightstyles[surf->styles[maps]].white;
  }
}

/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the floating format in blocklights
===============
*/
void R_BuildLightMap(msurface_t *surf, byte *dest_rgb0, byte *dest_r1, byte *dest_g1, byte *dest_b1, int stride) {
  int smax, tmax;
  float max;
  int i, j, size;
  byte *lightmap_rgb0, *lightmap_r1, *lightmap_g1, *lightmap_b1;
  float scale[4];
  int nummaps;
  struct SH1 *bl;
  lightstyle_t *style;
  int monolightmap;

  if(surf->texinfo->flags & (SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP))
    ri.Sys_Error(ERR_DROP, "R_BuildLightMap called for non-lit surface");

  smax = (surf->extents[0] >> 4) + 1;
  tmax = (surf->extents[1] >> 4) + 1;
  size = smax * tmax;
  if(size > (sizeof(s_blocklights) / sizeof(s_blocklights[0])))
    ri.Sys_Error(ERR_DROP, "Bad s_blocklights size");

  // set to full bright if no light data
  if(!surf->samples) {
    int maps;

    for(i = 0; i < size; i++) {
      s_blocklights[i].f[0] = s_blocklights[i].f[4] = s_blocklights[i].f[8] = 255;
      s_blocklights[i].f[1] = s_blocklights[i].f[2] = s_blocklights[i].f[3] = 0;
      s_blocklights[i].f[5] = s_blocklights[i].f[6] = s_blocklights[i].f[7] = 0;
      s_blocklights[i].f[9] = s_blocklights[i].f[10] = s_blocklights[i].f[11] = 0;
    }

    for(maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++) {
      style = &r_newrefdef.lightstyles[surf->styles[maps]];
    }
    goto store;
  }

  // count the # of maps
  for(nummaps = 0; nummaps < MAXLIGHTMAPS && surf->styles[nummaps] != 255; nummaps++)
    ;

  lightmap_rgb0 = surf->samples + (size * 3) * 0;
  lightmap_r1 = surf->samples + (size * 3) * 1;
  lightmap_g1 = surf->samples + (size * 3) * 2;
  lightmap_b1 = surf->samples + (size * 3) * 3;

  // add all the lightmaps
  if(nummaps == 1) {
    int maps;

    for(maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++) {
      bl = s_blocklights;

      for(i = 0; i < 3; i++)
        scale[i] = gl_modulate->value * r_newrefdef.lightstyles[surf->styles[maps]].rgb[i];

      for(i = 0; i < size; i++) {
        struct SH1 color;

        color.f[0] = lightmap_rgb0[i * 3 + 0] - 0.5f;
        color.f[4] = lightmap_rgb0[i * 3 + 1] - 0.5f;
        color.f[8] = lightmap_rgb0[i * 3 + 2] - 0.5f;
        color.f[1] = lightmap_r1[i * 3 + 0] - 0.5f;
        color.f[2] = lightmap_r1[i * 3 + 1] - 0.5f;
        color.f[3] = lightmap_r1[i * 3 + 2] - 0.5f;
        color.f[5] = lightmap_g1[i * 3 + 0] - 0.5f;
        color.f[6] = lightmap_g1[i * 3 + 1] - 0.5f;
        color.f[7] = lightmap_g1[i * 3 + 2] - 0.5f;
        color.f[9] = lightmap_b1[i * 3 + 0] - 0.5f;
        color.f[10] = lightmap_b1[i * 3 + 1] - 0.5f;
        color.f[11] = lightmap_b1[i * 3 + 2] - 0.5f;

        for(int component = 0; component < 3; component++) {
          for(int basis = 0; basis < 3; basis++) {
            color.f[component * 4 + basis + 1] -= 127.0f;
            color.f[component * 4 + basis + 1] *= 2;
          }
        }

        bl[i] = SH1_ColorScale(color, scale);
      }

      // skip to next lightmap
      lightmap_rgb0 += size * 3 * 4;
      lightmap_r1 += size * 3 * 4;
      lightmap_g1 += size * 3 * 4;
      lightmap_b1 += size * 3 * 4;
    }
  } else {
    int maps;

    memset(s_blocklights, 0, sizeof(s_blocklights[0]) * size);

    for(maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++) {
      bl = s_blocklights;

      for(i = 0; i < 3; i++)
        scale[i] = gl_modulate->value * r_newrefdef.lightstyles[surf->styles[maps]].rgb[i];

      for(i = 0; i < size; i++) {
        struct SH1 color;

        color.f[0] = lightmap_rgb0[i * 3 + 0] - 0.5f;
        color.f[4] = lightmap_rgb0[i * 3 + 1] - 0.5f;
        color.f[8] = lightmap_rgb0[i * 3 + 2] - 0.5f;
        color.f[1] = lightmap_r1[i * 3 + 0] - 0.5f;
        color.f[2] = lightmap_r1[i * 3 + 1] - 0.5f;
        color.f[3] = lightmap_r1[i * 3 + 2] - 0.5f;
        color.f[5] = lightmap_g1[i * 3 + 0] - 0.5f;
        color.f[6] = lightmap_g1[i * 3 + 1] - 0.5f;
        color.f[7] = lightmap_g1[i * 3 + 2] - 0.5f;
        color.f[9] = lightmap_b1[i * 3 + 0] - 0.5f;
        color.f[10] = lightmap_b1[i * 3 + 1] - 0.5f;
        color.f[11] = lightmap_b1[i * 3 + 2] - 0.5f;

        for(int component = 0; component < 3; component++) {
          for(int basis = 0; basis < 3; basis++) {
            color.f[component * 4 + basis + 1] -= 127.0f;
            color.f[component * 4 + basis + 1] *= 2;
          }
        }

        bl[i] = SH1_ColorScale(color, scale);
      }
    }
    // skip to next lightmap
    lightmap_rgb0 += size * 3 * 4;
    lightmap_r1 += size * 3 * 4;
    lightmap_g1 += size * 3 * 4;
    lightmap_b1 += size * 3 * 4;
  }

  // add all the dynamic lights
  if(surf->dlightframe == r_framecount)
    R_AddDynamicLights(surf);

// put into texture format
store:
  // stride -= (smax << 2);
  bl = s_blocklights;

  for(i = 0; i < tmax; i++, dest_rgb0 += stride, dest_r1 += stride, dest_g1 += stride, dest_b1 += stride) {
    for(j = 0; j < smax; j++) {
      struct SH1 color = SH1_NormalizeMaximum(*bl, 254.5f, NULL);

      for(int component = 0; component < 3; component++) {
        if(color.f[component * 4 + 0] > 255)
          assert(0);

        for(int basis = 0; basis < 3; basis++) {
          color.f[component * 4 + basis + 1] *= 0.5f;
          color.f[component * 4 + basis + 1] += 127.0f;

          if(color.f[component * 4 + basis + 1] > 255)
            assert(0);
        }
      }

      dest_rgb0[j * 3 + 0] = (byte)(color.f[0] + 0.5f);
      dest_rgb0[j * 3 + 1] = (byte)(color.f[4] + 0.5f);
      dest_rgb0[j * 3 + 2] = (byte)(color.f[8] + 0.5f);

      dest_r1[j * 3 + 0] = (byte)(color.f[1] + 0.5f);
      dest_r1[j * 3 + 1] = (byte)(color.f[2] + 0.5f);
      dest_r1[j * 3 + 2] = (byte)(color.f[3] + 0.5f);

      dest_g1[j * 3 + 0] = (byte)(color.f[5] + 0.5f);
      dest_g1[j * 3 + 1] = (byte)(color.f[6] + 0.5f);
      dest_g1[j * 3 + 2] = (byte)(color.f[7] + 0.5f);

      dest_b1[j * 3 + 0] = (byte)(color.f[9] + 0.5f);
      dest_b1[j * 3 + 1] = (byte)(color.f[10] + 0.5f);
      dest_b1[j * 3 + 2] = (byte)(color.f[11] + 0.5f);

      bl++;
    }
  }
}
