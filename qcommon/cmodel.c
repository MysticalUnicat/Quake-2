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
// cmodel.c -- model loading

#include "qcommon.h"

typedef struct {
  cplane_t *plane;
  int children[2]; // negative numbers are leafs
} cnode_t;

typedef struct {
  cplane_t *plane;
  mapsurface_t *surface;
} cbrushside_t;

typedef struct {
  int contents;
  int cluster;
  int area;
  unsigned short firstleafbrush;
  unsigned short numleafbrushes;
} cleaf_t;

typedef struct {
  int contents;
  int numsides;
  int firstbrushside;
  int checkcount; // to avoid repeated testings
} cbrush_t;

typedef struct {
  int numareaportals;
  int firstareaportal;
  int floodnum; // if two areas have equal floodnums, they are connected
  int floodvalid;
} carea_t;

struct cmodel {
  int checkcount;
  unsigned int checksum;

  char map_name[MAX_QPATH];

  int numbrushsides;
  cbrushside_t map_brushsides[MAX_MAP_BRUSHSIDES];

  int numtexinfo;
  mapsurface_t map_surfaces[MAX_MAP_TEXINFO];

  int numplanes;
  cplane_t map_planes[MAX_MAP_PLANES + 6]; // extra for box hull

  int numnodes;
  cnode_t map_nodes[MAX_MAP_NODES + 6]; // extra for box hull

  int numleafs;
  cleaf_t map_leafs[MAX_MAP_LEAFS];
  int emptyleaf, solidleaf;

  int numleafbrushes;
  unsigned short map_leafbrushes[MAX_MAP_LEAFBRUSHES];

  int numcmodels;
  cmodel_t map_cmodels[MAX_MAP_MODELS];

  int numbrushes;
  cbrush_t map_brushes[MAX_MAP_BRUSHES];

  int numvisibility;
  byte map_visibility[MAX_MAP_VISIBILITY];
  dvis_t *map_vis;

  int numentitychars;
  char map_entitystring[MAX_MAP_ENTSTRING];

  int numareas;
  carea_t map_areas[MAX_MAP_AREAS];

  int numareaportals;
  dareaportal_t map_areaportals[MAX_MAP_AREAPORTALS];

  int numclusters;

  mapsurface_t nullsurface;

  int floodvalid;

  bool portalopen[MAX_MAP_AREAPORTALS];
};

cvar_t *map_noareas;

void CM_InitBoxHull(struct cmodel *cm);
void FloodAreaConnections(struct cmodel *cm);

int c_pointcontents;
int c_traces, c_brush_traces;

/*
===============================================================================

          MAP LOADING

===============================================================================
*/

byte *cmod_base;

/*
=================
CMod_LoadSubmodels
=================
*/
static void CMod_LoadSubmodels(struct cmodel *cm, lump_t *l) {
  dmodel_t *in;
  cmodel_t *out;
  int i, j, count;

  in = (void *)(cmod_base + l->fileofs);
  if(l->filelen % sizeof(*in))
    Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
  count = l->filelen / sizeof(*in);

  if(count < 1)
    Com_Error(ERR_DROP, "Map with no models");
  if(count > MAX_MAP_MODELS)
    Com_Error(ERR_DROP, "Map has too many models");

  cm->numcmodels = count;

  for(i = 0; i < count; i++, in++, out++) {
    out = &cm->map_cmodels[i];

    for(j = 0; j < 3; j++) { // spread the mins / maxs by a pixel
      out->mins[j] = LittleFloat(in->mins[j]) - 1;
      out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
      out->origin[j] = LittleFloat(in->origin[j]);
    }
    out->headnode = LittleLong(in->headnode);
  }
}

/*
=================
CMod_LoadSurfaces
=================
*/
static void CMod_LoadSurfaces(struct cmodel *cm, lump_t *l) {
  texinfo_t *in;
  mapsurface_t *out;
  int i, count;

  in = (void *)(cmod_base + l->fileofs);
  if(l->filelen % sizeof(*in))
    Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
  count = l->filelen / sizeof(*in);
  if(count < 1)
    Com_Error(ERR_DROP, "Map with no surfaces");
  if(count > MAX_MAP_TEXINFO)
    Com_Error(ERR_DROP, "Map has too many surfaces");

  cm->numtexinfo = count;
  out = cm->map_surfaces;

  for(i = 0; i < count; i++, in++, out++) {
    strncpy(out->c.name, in->texture, sizeof(out->c.name) - 1);
    strncpy(out->rname, in->texture, sizeof(out->rname) - 1);
    out->c.flags = LittleLong(in->flags);
    out->c.value = LittleLong(in->value);
  }
}

/*
=================
CMod_LoadNodes

=================
*/
static void CMod_LoadNodes(struct cmodel *cm, lump_t *l) {
  dnode_t *in;
  int child;
  cnode_t *out;
  int i, j, count;

  in = (void *)(cmod_base + l->fileofs);
  if(l->filelen % sizeof(*in))
    Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
  count = l->filelen / sizeof(*in);

  if(count < 1)
    Com_Error(ERR_DROP, "Map has no nodes");
  if(count > MAX_MAP_NODES)
    Com_Error(ERR_DROP, "Map has too many nodes");

  out = cm->map_nodes;

  cm->numnodes = count;

  for(i = 0; i < count; i++, out++, in++) {
    out->plane = cm->map_planes + LittleLong(in->planenum);
    for(j = 0; j < 2; j++) {
      child = LittleLong(in->children[j]);
      out->children[j] = child;
    }
  }
}

/*
=================
CMod_LoadBrushes

=================
*/
static void CMod_LoadBrushes(struct cmodel *cm, lump_t *l) {
  dbrush_t *in;
  cbrush_t *out;
  int i, count;

  in = (void *)(cmod_base + l->fileofs);
  if(l->filelen % sizeof(*in))
    Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
  count = l->filelen / sizeof(*in);

  if(count > MAX_MAP_BRUSHES)
    Com_Error(ERR_DROP, "Map has too many brushes");

  out = cm->map_brushes;

  cm->numbrushes = count;

  for(i = 0; i < count; i++, out++, in++) {
    out->firstbrushside = LittleLong(in->firstside);
    out->numsides = LittleLong(in->numsides);
    out->contents = LittleLong(in->contents);
  }
}

/*
=================
CMod_LoadLeafs
=================
*/
static void CMod_LoadLeafs(struct cmodel *cm, lump_t *l) {
  int i;
  cleaf_t *out;
  dleaf_t *in;
  int count;

  in = (void *)(cmod_base + l->fileofs);
  if(l->filelen % sizeof(*in))
    Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
  count = l->filelen / sizeof(*in);

  if(count < 1)
    Com_Error(ERR_DROP, "Map with no leafs");
  // need to save space for box planes
  if(count > MAX_MAP_PLANES)
    Com_Error(ERR_DROP, "Map has too many planes");

  out = cm->map_leafs;
  cm->numleafs = count;
  cm->numclusters = 0;

  for(i = 0; i < count; i++, in++, out++) {
    out->contents = LittleLong(in->contents);
    out->cluster = LittleShort(in->cluster);
    out->area = LittleShort(in->area);
    out->firstleafbrush = LittleShort(in->firstleafbrush);
    out->numleafbrushes = LittleShort(in->numleafbrushes);

    if(out->cluster >= cm->numclusters)
      cm->numclusters = out->cluster + 1;
  }

  if(cm->map_leafs[0].contents != CONTENTS_SOLID)
    Com_Error(ERR_DROP, "Map leaf 0 is not CONTENTS_SOLID");
  cm->solidleaf = 0;
  cm->emptyleaf = -1;
  for(i = 1; i < cm->numleafs; i++) {
    if(!cm->map_leafs[i].contents) {
      cm->emptyleaf = i;
      break;
    }
  }
  if(cm->emptyleaf == -1)
    Com_Error(ERR_DROP, "Map does not have an empty leaf");
}

/*
=================
CMod_LoadPlanes
=================
*/
static void CMod_LoadPlanes(struct cmodel *cm, lump_t *l) {
  int i, j;
  cplane_t *out;
  dplane_t *in;
  int count;
  int bits;

  in = (void *)(cmod_base + l->fileofs);
  if(l->filelen % sizeof(*in))
    Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
  count = l->filelen / sizeof(*in);

  if(count < 1)
    Com_Error(ERR_DROP, "Map with no planes");
  // need to save space for box planes
  if(count > MAX_MAP_PLANES)
    Com_Error(ERR_DROP, "Map has too many planes");

  out = cm->map_planes;
  cm->numplanes = count;

  for(i = 0; i < count; i++, in++, out++) {
    bits = 0;
    for(j = 0; j < 3; j++) {
      out->normal[j] = LittleFloat(in->normal[j]);
      if(out->normal[j] < 0)
        bits |= 1 << j;
    }

    out->dist = LittleFloat(in->dist);
    out->type = LittleLong(in->type);
    out->signbits = bits;
  }
}

/*
=================
CMod_LoadLeafBrushes
=================
*/
static void CMod_LoadLeafBrushes(struct cmodel *cm, lump_t *l) {
  int i;
  unsigned short *out;
  unsigned short *in;
  int count;

  in = (void *)(cmod_base + l->fileofs);
  if(l->filelen % sizeof(*in))
    Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
  count = l->filelen / sizeof(*in);

  if(count < 1)
    Com_Error(ERR_DROP, "Map with no planes");
  // need to save space for box planes
  if(count > MAX_MAP_LEAFBRUSHES)
    Com_Error(ERR_DROP, "Map has too many leafbrushes");

  out = cm->map_leafbrushes;
  cm->numleafbrushes = count;

  for(i = 0; i < count; i++, in++, out++)
    *out = LittleShort(*in);
}

/*
=================
CMod_LoadBrushSides
=================
*/
static void CMod_LoadBrushSides(struct cmodel *cm, lump_t *l) {
  int i, j;
  cbrushside_t *out;
  dbrushside_t *in;
  int count;
  int num;

  in = (void *)(cmod_base + l->fileofs);
  if(l->filelen % sizeof(*in))
    Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
  count = l->filelen / sizeof(*in);

  // need to save space for box planes
  if(count > MAX_MAP_BRUSHSIDES)
    Com_Error(ERR_DROP, "Map has too many planes");

  out = cm->map_brushsides;
  cm->numbrushsides = count;

  for(i = 0; i < count; i++, in++, out++) {
    num = LittleShort(in->planenum);
    out->plane = &cm->map_planes[num];
    j = LittleShort(in->texinfo);
    if(j >= cm->numtexinfo)
      Com_Error(ERR_DROP, "Bad brushside texinfo");
    out->surface = &cm->map_surfaces[j];
  }
}

/*
=================
CMod_LoadAreas
=================
*/
static void CMod_LoadAreas(struct cmodel *cm, lump_t *l) {
  int i;
  carea_t *out;
  darea_t *in;
  int count;

  in = (void *)(cmod_base + l->fileofs);
  if(l->filelen % sizeof(*in))
    Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
  count = l->filelen / sizeof(*in);

  if(count > MAX_MAP_AREAS)
    Com_Error(ERR_DROP, "Map has too many areas");

  out = cm->map_areas;
  cm->numareas = count;

  for(i = 0; i < count; i++, in++, out++) {
    out->numareaportals = LittleLong(in->numareaportals);
    out->firstareaportal = LittleLong(in->firstareaportal);
    out->floodvalid = 0;
    out->floodnum = 0;
  }
}

/*
=================
CMod_LoadAreaPortals
=================
*/
static void CMod_LoadAreaPortals(struct cmodel *cm, lump_t *l) {
  int i;
  dareaportal_t *out;
  dareaportal_t *in;
  int count;

  in = (void *)(cmod_base + l->fileofs);
  if(l->filelen % sizeof(*in))
    Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
  count = l->filelen / sizeof(*in);

  if(count > MAX_MAP_AREAS)
    Com_Error(ERR_DROP, "Map has too many areas");

  out = cm->map_areaportals;
  cm->numareaportals = count;

  for(i = 0; i < count; i++, in++, out++) {
    out->portalnum = LittleLong(in->portalnum);
    out->otherarea = LittleLong(in->otherarea);
  }
}

/*
=================
CMod_LoadVisibility
=================
*/
static void CMod_LoadVisibility(struct cmodel *cm, lump_t *l) {
  int i;

  cm->numvisibility = l->filelen;
  if(l->filelen > MAX_MAP_VISIBILITY)
    Com_Error(ERR_DROP, "Map has too large visibility lump");

  memcpy(cm->map_visibility, cmod_base + l->fileofs, l->filelen);

  cm->map_vis->numclusters = LittleLong(cm->map_vis->numclusters);
  for(i = 0; i < cm->map_vis->numclusters; i++) {
    cm->map_vis->bitofs[i][0] = LittleLong(cm->map_vis->bitofs[i][0]);
    cm->map_vis->bitofs[i][1] = LittleLong(cm->map_vis->bitofs[i][1]);
  }
}

/*
=================
CMod_LoadEntityString
=================
*/
static void CMod_LoadEntityString(struct cmodel *cm, lump_t *l) {
  cm->numentitychars = l->filelen;
  if(l->filelen > MAX_MAP_ENTSTRING)
    Com_Error(ERR_DROP, "Map has too large entity lump");

  memcpy(cm->map_entitystring, cmod_base + l->fileofs, l->filelen);
}

/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
static struct cmodel global_cmodels[3];

cmodel_t *CM_LoadMap(int index, char *name, bool clientload, unsigned *checksum) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];

  unsigned *buf;
  int i;
  dheader_t header;
  int length;

  map_noareas = Cvar_Get("map_noareas", "0", 0);

  if(!strcmp(cm->map_name, name) && (clientload || !Cvar_VariableValue("flushmap"))) {
    *checksum = cm->checksum;
    if(!clientload) {
      memset(cm->portalopen, 0, sizeof(cm->portalopen));
      FloodAreaConnections(cm);
    }
    return &cm->map_cmodels[0]; // still have the right version
  }

  // initialize
  cm->numleafs = 1; // allow leaf funcs to be called without a map
  cm->map_vis = (dvis_t *)cm->map_visibility;
  cm->numareas = 1;
  cm->numclusters = 1;

  // free old stuff
  cm->numplanes = 0;
  cm->numnodes = 0;
  cm->numleafs = 0;
  cm->numcmodels = 0;
  cm->numvisibility = 0;
  cm->numentitychars = 0;
  cm->map_entitystring[0] = 0;
  cm->map_name[0] = 0;

  if(!name || !name[0]) {
    cm->numleafs = 1;
    cm->numclusters = 1;
    cm->numareas = 1;
    *checksum = 0;
    return &cm->map_cmodels[0]; // cinematic servers won't have anything at all
  }

  //
  // load the file
  //
  length = FS_LoadFile(name, (void **)&buf);
  if(!buf)
    Com_Error(ERR_DROP, "Couldn't load %s", name);

  cm->checksum = LittleLong(Com_BlockChecksum(buf, length));
  *checksum = cm->checksum;

  header = *(dheader_t *)buf;
  for(i = 0; i < sizeof(dheader_t) / 4; i++)
    ((int *)&header)[i] = LittleLong(((int *)&header)[i]);

  if(header.version != BSPVERSION)
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %s has wrong version number (%i should be %i)", name, header.version,
              BSPVERSION);

  cmod_base = (byte *)buf;

  // load into heap
  CMod_LoadSurfaces(cm, &header.lumps[LUMP_TEXINFO]);
  CMod_LoadLeafs(cm, &header.lumps[LUMP_LEAFS]);
  CMod_LoadLeafBrushes(cm, &header.lumps[LUMP_LEAFBRUSHES]);
  CMod_LoadPlanes(cm, &header.lumps[LUMP_PLANES]);
  CMod_LoadBrushes(cm, &header.lumps[LUMP_BRUSHES]);
  CMod_LoadBrushSides(cm, &header.lumps[LUMP_BRUSHSIDES]);
  CMod_LoadSubmodels(cm, &header.lumps[LUMP_MODELS]);
  CMod_LoadNodes(cm, &header.lumps[LUMP_NODES]);
  CMod_LoadAreas(cm, &header.lumps[LUMP_AREAS]);
  CMod_LoadAreaPortals(cm, &header.lumps[LUMP_AREAPORTALS]);
  CMod_LoadVisibility(cm, &header.lumps[LUMP_VISIBILITY]);
  CMod_LoadEntityString(cm, &header.lumps[LUMP_ENTITIES]);

  FS_FreeFile(buf);

  CM_InitBoxHull(cm);

  memset(cm->portalopen, 0, sizeof(cm->portalopen));
  FloodAreaConnections(cm);

  strcpy(cm->map_name, name);

  return &cm->map_cmodels[0];
}

/*
==================
CM_InlineModel
==================
*/
cmodel_t *CM_InlineModel(int index, char *name) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];

  int num;

  if(!name || name[0] != '*')
    Com_Error(ERR_DROP, "CM_InlineModel: bad name");
  num = atoi(name + 1);
  if(num < 1 || num >= cm->numcmodels)
    Com_Error(ERR_DROP, "CM_InlineModel: bad number");

  return &cm->map_cmodels[num];
}

int CM_NumClusters(int index) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];
  return cm->numclusters;
}

int CM_NumInlineModels(int index) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];
  return cm->numcmodels;
}

char *CM_EntityString(int index) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];
  return cm->map_entitystring;
}

int CM_LeafContents(int index, int leafnum) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];
  if(leafnum < 0 || leafnum >= cm->numleafs)
    Com_Error(ERR_DROP, "CM_LeafContents: bad number");
  return cm->map_leafs[leafnum].contents;
}

int CM_LeafCluster(int index, int leafnum) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];
  if(leafnum < 0 || leafnum >= cm->numleafs)
    Com_Error(ERR_DROP, "CM_LeafCluster: bad number");
  return cm->map_leafs[leafnum].cluster;
}

int CM_LeafArea(int index, int leafnum) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];
  if(leafnum < 0 || leafnum >= cm->numleafs)
    Com_Error(ERR_DROP, "CM_LeafArea: bad number");
  return cm->map_leafs[leafnum].area;
}

//=======================================================================

cplane_t *box_planes;
int box_headnode;
cbrush_t *box_brush;
cleaf_t *box_leaf;

/*
===================
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void CM_InitBoxHull(struct cmodel *cm) {
  int i;
  int side;
  cnode_t *c;
  cplane_t *p;
  cbrushside_t *s;

  box_headnode = cm->numnodes;
  box_planes = &cm->map_planes[cm->numplanes];
  if(cm->numnodes + 6 > MAX_MAP_NODES || cm->numbrushes + 1 > MAX_MAP_BRUSHES ||
     cm->numleafbrushes + 1 > MAX_MAP_LEAFBRUSHES || cm->numbrushsides + 6 > MAX_MAP_BRUSHSIDES ||
     cm->numplanes + 12 > MAX_MAP_PLANES)
    Com_Error(ERR_DROP, "Not enough room for box tree");

  box_brush = &cm->map_brushes[cm->numbrushes];
  box_brush->numsides = 6;
  box_brush->firstbrushside = cm->numbrushsides;
  box_brush->contents = CONTENTS_MONSTER;

  box_leaf = &cm->map_leafs[cm->numleafs];
  box_leaf->contents = CONTENTS_MONSTER;
  box_leaf->firstleafbrush = cm->numleafbrushes;
  box_leaf->numleafbrushes = 1;

  cm->map_leafbrushes[cm->numleafbrushes] = cm->numbrushes;

  for(i = 0; i < 6; i++) {
    side = i & 1;

    // brush sides
    s = &cm->map_brushsides[cm->numbrushsides + i];
    s->plane = cm->map_planes + (cm->numplanes + i * 2 + side);
    s->surface = &cm->nullsurface;

    // nodes
    c = &cm->map_nodes[box_headnode + i];
    c->plane = cm->map_planes + (cm->numplanes + i * 2);
    c->children[side] = -1 - cm->emptyleaf;
    if(i != 5)
      c->children[side ^ 1] = box_headnode + i + 1;
    else
      c->children[side ^ 1] = -1 - cm->numleafs;

    // planes
    p = &box_planes[i * 2];
    p->type = i >> 1;
    p->signbits = 0;
    VectorClear(p->normal);
    p->normal[i >> 1] = 1;

    p = &box_planes[i * 2 + 1];
    p->type = 3 + (i >> 1);
    p->signbits = 0;
    VectorClear(p->normal);
    p->normal[i >> 1] = -1;
  }
}

/*
===================
CM_HeadnodeForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
int CM_HeadnodeForBox(vec3_t mins, vec3_t maxs) {
  box_planes[0].dist = maxs[0];
  box_planes[1].dist = -maxs[0];
  box_planes[2].dist = mins[0];
  box_planes[3].dist = -mins[0];
  box_planes[4].dist = maxs[1];
  box_planes[5].dist = -maxs[1];
  box_planes[6].dist = mins[1];
  box_planes[7].dist = -mins[1];
  box_planes[8].dist = maxs[2];
  box_planes[9].dist = -maxs[2];
  box_planes[10].dist = mins[2];
  box_planes[11].dist = -mins[2];

  return box_headnode;
}

/*
==================
CM_PointLeafnum_r

==================
*/
static int CM_PointLeafnum_r(struct cmodel *cm, vec3_t p, int num) {
  float d;
  cnode_t *node;
  cplane_t *plane;

  while(num >= 0) {
    node = cm->map_nodes + num;
    plane = node->plane;

    if(plane->type < 3)
      d = p[plane->type] - plane->dist;
    else
      d = DotProduct(plane->normal, p) - plane->dist;
    if(d < 0)
      num = node->children[1];
    else
      num = node->children[0];
  }

  c_pointcontents++; // optimize counter

  return -1 - num;
}

int CM_PointLeafnum(int index, vec3_t p) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];
  if(!cm->numplanes)
    return 0; // sound may call this without map loaded
  return CM_PointLeafnum_r(cm, p, 0);
}

/*
=============
CM_BoxLeafnums

Fills in a list of all the leafs touched
=============
*/
struct box_leafnum_state {
  int count;
  int maxcount;
  int *list;
  float *mins;
  float *maxs;
  int topnode;
};

static void CM_BoxLeafnums_r(struct cmodel *cm, struct box_leafnum_state *state, int nodenum) {
  cplane_t *plane;
  cnode_t *node;
  int s;

  while(1) {
    if(nodenum < 0) {
      if(state->count >= state->maxcount) {
        //				Com_Printf ("CM_BoxLeafnums_r: overflow\n");
        return;
      }
      state->list[state->count++] = -1 - nodenum;
      return;
    }

    node = &cm->map_nodes[nodenum];
    plane = node->plane;
    //		s = BoxOnPlaneSide (leaf_mins, leaf_maxs, plane);
    s = BOX_ON_PLANE_SIDE(state->mins, state->maxs, plane);
    if(s == 1)
      nodenum = node->children[0];
    else if(s == 2)
      nodenum = node->children[1];
    else { // go down both
      if(state->topnode == -1)
        state->topnode = nodenum;
      CM_BoxLeafnums_r(cm, state, node->children[0]);
      nodenum = node->children[1];
    }
  }
}

static int CM_BoxLeafnums_headnode(struct cmodel *cm, vec3_t mins, vec3_t maxs, int *list, int listsize, int headnode,
                                   int *topnode) {
  struct box_leafnum_state state;

  state.list = list;
  state.count = 0;
  state.maxcount = listsize;
  state.mins = mins;
  state.maxs = maxs;

  state.topnode = -1;

  CM_BoxLeafnums_r(cm, &state, headnode);

  if(topnode)
    *topnode = state.topnode;

  return state.count;
}

int CM_BoxLeafnums(int index, vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];
  return CM_BoxLeafnums_headnode(cm, mins, maxs, list, listsize, cm->map_cmodels[0].headnode, topnode);
}

/*
==================
CM_PointContents

==================
*/
int CM_PointContents(int index, vec3_t p, int headnode) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];

  int l;

  if(!cm->numnodes) // map not loaded
    return 0;

  l = CM_PointLeafnum_r(cm, p, headnode);

  return cm->map_leafs[l].contents;
}

/*
==================
CM_TransformedPointContents

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
int CM_TransformedPointContents(int index, vec3_t p, int headnode, vec3_t origin, vec3_t angles) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];

  vec3_t p_l;
  vec3_t temp;
  vec3_t forward, right, up;
  int l;

  // subtract origin offset
  VectorSubtract(p, origin, p_l);

  // rotate start and end into the models frame of reference
  if(headnode != box_headnode && (angles[0] || angles[1] || angles[2])) {
    AngleVectors(angles, forward, right, up);

    VectorCopy(p_l, temp);
    p_l[0] = DotProduct(temp, forward);
    p_l[1] = -DotProduct(temp, right);
    p_l[2] = DotProduct(temp, up);
  }

  l = CM_PointLeafnum_r(cm, p_l, headnode);

  return cm->map_leafs[l].contents;
}

/*
===============================================================================

BOX TRACING

===============================================================================
*/

// 1/32 epsilon to keep floating point happy
#define DIST_EPSILON (0.03125)

vec3_t trace_start, trace_end;
vec3_t trace_mins, trace_maxs;
vec3_t trace_extents;

trace_t trace_trace;
int trace_contents;
bool trace_ispoint; // optimized case

/*
================
CM_ClipBoxToBrush
================
*/
void CM_ClipBoxToBrush(struct cmodel *cm, vec3_t mins, vec3_t maxs, vec3_t p1, vec3_t p2, trace_t *trace,
                       cbrush_t *brush) {
  int i, j;
  cplane_t *plane, *clipplane;
  float dist;
  float enterfrac, leavefrac;
  vec3_t ofs;
  float d1, d2;
  bool getout, startout;
  float f;
  cbrushside_t *side, *leadside;

  enterfrac = -1;
  leavefrac = 1;
  clipplane = NULL;

  if(!brush->numsides)
    return;

  c_brush_traces++;

  getout = false;
  startout = false;
  leadside = NULL;

  for(i = 0; i < brush->numsides; i++) {
    side = &cm->map_brushsides[brush->firstbrushside + i];
    plane = side->plane;

    // FIXME: special case for axial

    if(!trace_ispoint) { // general box case

      // push the plane out apropriately for mins/maxs

      // FIXME: use signbits into 8 way lookup for each mins/maxs
      for(j = 0; j < 3; j++) {
        if(plane->normal[j] < 0)
          ofs[j] = maxs[j];
        else
          ofs[j] = mins[j];
      }
      dist = DotProduct(ofs, plane->normal);
      dist = plane->dist - dist;
    } else { // special point case
      dist = plane->dist;
    }

    d1 = DotProduct(p1, plane->normal) - dist;
    d2 = DotProduct(p2, plane->normal) - dist;

    if(d2 > 0)
      getout = true; // endpoint is not in solid
    if(d1 > 0)
      startout = true;

    // if completely in front of face, no intersection
    if(d1 > 0 && d2 >= d1)
      return;

    if(d1 <= 0 && d2 <= 0)
      continue;

    // crosses face
    if(d1 > d2) { // enter
      f = (d1 - DIST_EPSILON) / (d1 - d2);
      if(f > enterfrac) {
        enterfrac = f;
        clipplane = plane;
        leadside = side;
      }
    } else { // leave
      f = (d1 + DIST_EPSILON) / (d1 - d2);
      if(f < leavefrac)
        leavefrac = f;
    }
  }

  if(!startout) { // original point was inside brush
    trace->startsolid = true;
    if(!getout)
      trace->allsolid = true;
    return;
  }
  if(enterfrac < leavefrac) {
    if(enterfrac > -1 && enterfrac < trace->fraction) {
      if(enterfrac < 0)
        enterfrac = 0;
      trace->fraction = enterfrac;
      trace->plane = *clipplane;
      trace->surface = &(leadside->surface->c);
      trace->contents = brush->contents;
    }
  }
}

/*
================
CM_TestBoxInBrush
================
*/
static void CM_TestBoxInBrush(struct cmodel *cm, vec3_t mins, vec3_t maxs, vec3_t p1, trace_t *trace, cbrush_t *brush) {
  int i, j;
  cplane_t *plane;
  float dist;
  vec3_t ofs;
  float d1;
  cbrushside_t *side;

  if(!brush->numsides)
    return;

  for(i = 0; i < brush->numsides; i++) {
    side = &cm->map_brushsides[brush->firstbrushside + i];
    plane = side->plane;

    // FIXME: special case for axial

    // general box case

    // push the plane out apropriately for mins/maxs

    // FIXME: use signbits into 8 way lookup for each mins/maxs
    for(j = 0; j < 3; j++) {
      if(plane->normal[j] < 0)
        ofs[j] = maxs[j];
      else
        ofs[j] = mins[j];
    }
    dist = DotProduct(ofs, plane->normal);
    dist = plane->dist - dist;

    d1 = DotProduct(p1, plane->normal) - dist;

    // if completely in front of face, no intersection
    if(d1 > 0)
      return;
  }

  // inside this brush
  trace->startsolid = trace->allsolid = true;
  trace->fraction = 0;
  trace->contents = brush->contents;
}

/*
================
CM_TraceToLeaf
================
*/
static void CM_TraceToLeaf(struct cmodel *cm, int leafnum) {
  int k;
  int brushnum;
  cleaf_t *leaf;
  cbrush_t *b;

  leaf = &cm->map_leafs[leafnum];
  if(!(leaf->contents & trace_contents))
    return;
  // trace line against all brushes in the leaf
  for(k = 0; k < leaf->numleafbrushes; k++) {
    brushnum = cm->map_leafbrushes[leaf->firstleafbrush + k];
    b = &cm->map_brushes[brushnum];
    if(b->checkcount == cm->checkcount)
      continue; // already checked this brush in another leaf
    b->checkcount = cm->checkcount;

    if(!(b->contents & trace_contents))
      continue;
    CM_ClipBoxToBrush(cm, trace_mins, trace_maxs, trace_start, trace_end, &trace_trace, b);
    if(!trace_trace.fraction)
      return;
  }
}

/*
================
CM_TestInLeaf
================
*/
static void CM_TestInLeaf(struct cmodel *cm, int leafnum) {
  int k;
  int brushnum;
  cleaf_t *leaf;
  cbrush_t *b;

  leaf = &cm->map_leafs[leafnum];
  if(!(leaf->contents & trace_contents))
    return;
  // trace line against all brushes in the leaf
  for(k = 0; k < leaf->numleafbrushes; k++) {
    brushnum = cm->map_leafbrushes[leaf->firstleafbrush + k];
    b = &cm->map_brushes[brushnum];
    if(b->checkcount == cm->checkcount)
      continue; // already checked this brush in another leaf
    b->checkcount = cm->checkcount;

    if(!(b->contents & trace_contents))
      continue;
    CM_TestBoxInBrush(cm, trace_mins, trace_maxs, trace_start, &trace_trace, b);
    if(!trace_trace.fraction)
      return;
  }
}

/*
==================
CM_RecursiveHullCheck

==================
*/
static void CM_RecursiveHullCheck(struct cmodel *cm, int num, float p1f, float p2f, vec3_t p1, vec3_t p2) {
  cnode_t *node;
  cplane_t *plane;
  float t1, t2, offset;
  float frac, frac2;
  float idist;
  int i;
  vec3_t mid;
  int side;
  float midf;

  if(trace_trace.fraction <= p1f)
    return; // already hit something nearer

  // if < 0, we are in a leaf node
  if(num < 0) {
    CM_TraceToLeaf(cm, -1 - num);
    return;
  }

  //
  // find the point distances to the seperating plane
  // and the offset for the size of the box
  //
  node = cm->map_nodes + num;
  plane = node->plane;

  if(plane->type < 3) {
    t1 = p1[plane->type] - plane->dist;
    t2 = p2[plane->type] - plane->dist;
    offset = trace_extents[plane->type];
  } else {
    t1 = DotProduct(plane->normal, p1) - plane->dist;
    t2 = DotProduct(plane->normal, p2) - plane->dist;
    if(trace_ispoint)
      offset = 0;
    else
      offset = fabs(trace_extents[0] * plane->normal[0]) + fabs(trace_extents[1] * plane->normal[1]) +
               fabs(trace_extents[2] * plane->normal[2]);
  }

#if 0
CM_RecursiveHullCheck (node->children[0], p1f, p2f, p1, p2);
CM_RecursiveHullCheck (node->children[1], p1f, p2f, p1, p2);
return;
#endif

  // see which sides we need to consider
  if(t1 >= offset && t2 >= offset) {
    CM_RecursiveHullCheck(cm, node->children[0], p1f, p2f, p1, p2);
    return;
  }
  if(t1 < -offset && t2 < -offset) {
    CM_RecursiveHullCheck(cm, node->children[1], p1f, p2f, p1, p2);
    return;
  }

  // put the crosspoint DIST_EPSILON pixels on the near side
  if(t1 < t2) {
    idist = 1.0 / (t1 - t2);
    side = 1;
    frac2 = (t1 + offset + DIST_EPSILON) * idist;
    frac = (t1 - offset + DIST_EPSILON) * idist;
  } else if(t1 > t2) {
    idist = 1.0 / (t1 - t2);
    side = 0;
    frac2 = (t1 - offset - DIST_EPSILON) * idist;
    frac = (t1 + offset + DIST_EPSILON) * idist;
  } else {
    side = 0;
    frac = 1;
    frac2 = 0;
  }

  // move up to the node
  if(frac < 0)
    frac = 0;
  if(frac > 1)
    frac = 1;

  midf = p1f + (p2f - p1f) * frac;
  for(i = 0; i < 3; i++)
    mid[i] = p1[i] + frac * (p2[i] - p1[i]);

  CM_RecursiveHullCheck(cm, node->children[side], p1f, midf, p1, mid);

  // go past the node
  if(frac2 < 0)
    frac2 = 0;
  if(frac2 > 1)
    frac2 = 1;

  midf = p1f + (p2f - p1f) * frac2;
  for(i = 0; i < 3; i++)
    mid[i] = p1[i] + frac2 * (p2[i] - p1[i]);

  CM_RecursiveHullCheck(cm, node->children[side ^ 1], midf, p2f, mid, p2);
}

//======================================================================

/*
==================
CM_BoxTrace
==================
*/
trace_t CM_BoxTrace(int index, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int headnode, int brushmask) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];

  int i;

  cm->checkcount++; // for multi-check avoidance

  c_traces++; // for statistics, may be zeroed

  // fill in a default trace
  memset(&trace_trace, 0, sizeof(trace_trace));
  trace_trace.fraction = 1;
  trace_trace.surface = &(cm->nullsurface.c);

  if(!cm->numnodes) // map not loaded
    return trace_trace;

  trace_contents = brushmask;
  VectorCopy(start, trace_start);
  VectorCopy(end, trace_end);
  VectorCopy(mins, trace_mins);
  VectorCopy(maxs, trace_maxs);

  //
  // check for position test special case
  //
  if(start[0] == end[0] && start[1] == end[1] && start[2] == end[2]) {
    int leafs[1024];
    int i, numleafs;
    vec3_t c1, c2;
    int topnode;

    VectorAdd(start, mins, c1);
    VectorAdd(start, maxs, c2);
    for(i = 0; i < 3; i++) {
      c1[i] -= 1;
      c2[i] += 1;
    }

    numleafs = CM_BoxLeafnums_headnode(cm, c1, c2, leafs, 1024, headnode, &topnode);
    for(i = 0; i < numleafs; i++) {
      CM_TestInLeaf(cm, leafs[i]);
      if(trace_trace.allsolid)
        break;
    }
    VectorCopy(start, trace_trace.endpos);
    return trace_trace;
  }

  //
  // check for point special case
  //
  if(mins[0] == 0 && mins[1] == 0 && mins[2] == 0 && maxs[0] == 0 && maxs[1] == 0 && maxs[2] == 0) {
    trace_ispoint = true;
    VectorClear(trace_extents);
  } else {
    trace_ispoint = false;
    trace_extents[0] = -mins[0] > maxs[0] ? -mins[0] : maxs[0];
    trace_extents[1] = -mins[1] > maxs[1] ? -mins[1] : maxs[1];
    trace_extents[2] = -mins[2] > maxs[2] ? -mins[2] : maxs[2];
  }

  //
  // general sweeping through world
  //
  CM_RecursiveHullCheck(cm, headnode, 0, 1, start, end);

  if(trace_trace.fraction == 1) {
    VectorCopy(end, trace_trace.endpos);
  } else {
    for(i = 0; i < 3; i++)
      trace_trace.endpos[i] = start[i] + trace_trace.fraction * (end[i] - start[i]);
  }
  return trace_trace;
}

/*
==================
CM_TransformedBoxTrace

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
#ifdef _WIN32
#pragma optimize("", off)
#endif

trace_t CM_TransformedBoxTrace(int index, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int headnode,
                               int brushmask, vec3_t origin, vec3_t angles) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];

  trace_t trace;
  vec3_t start_l, end_l;
  vec3_t a;
  vec3_t forward, right, up;
  vec3_t temp;
  bool rotated;

  // subtract origin offset
  VectorSubtract(start, origin, start_l);
  VectorSubtract(end, origin, end_l);

  // rotate start and end into the models frame of reference
  if(headnode != box_headnode && (angles[0] || angles[1] || angles[2]))
    rotated = true;
  else
    rotated = false;

  if(rotated) {
    AngleVectors(angles, forward, right, up);

    VectorCopy(start_l, temp);
    start_l[0] = DotProduct(temp, forward);
    start_l[1] = -DotProduct(temp, right);
    start_l[2] = DotProduct(temp, up);

    VectorCopy(end_l, temp);
    end_l[0] = DotProduct(temp, forward);
    end_l[1] = -DotProduct(temp, right);
    end_l[2] = DotProduct(temp, up);
  }

  // sweep the box through the model
  trace = CM_BoxTrace(index, start_l, end_l, mins, maxs, headnode, brushmask);

  if(rotated && trace.fraction != 1.0) {
    // FIXME: figure out how to do this with existing angles
    VectorNegate(angles, a);
    AngleVectors(a, forward, right, up);

    VectorCopy(trace.plane.normal, temp);
    trace.plane.normal[0] = DotProduct(temp, forward);
    trace.plane.normal[1] = -DotProduct(temp, right);
    trace.plane.normal[2] = DotProduct(temp, up);
  }

  trace.endpos[0] = start[0] + trace.fraction * (end[0] - start[0]);
  trace.endpos[1] = start[1] + trace.fraction * (end[1] - start[1]);
  trace.endpos[2] = start[2] + trace.fraction * (end[2] - start[2]);

  return trace;
}

#ifdef _WIN32
#pragma optimize("", on)
#endif

/*
===============================================================================

PVS / PHS

===============================================================================
*/

/*
===================
CM_DecompressVis
===================
*/
static void CM_DecompressVis(struct cmodel *cm, byte *in, byte *out) {
  int c;
  byte *out_p;
  int row;

  row = (cm->numclusters + 7) >> 3;
  out_p = out;

  if(!in || !cm->numvisibility) { // no vis info, so make all visible
    while(row) {
      *out_p++ = 0xff;
      row--;
    }
    return;
  }

  do {
    if(*in) {
      *out_p++ = *in++;
      continue;
    }

    c = in[1];
    in += 2;
    if((out_p - out) + c > row) {
      c = row - (out_p - out);
      Com_DPrintf("warning: Vis decompression overrun\n");
    }
    while(c) {
      *out_p++ = 0;
      c--;
    }
  } while(out_p - out < row);
}

static byte pvsrow[MAX_MAP_LEAFS / 8];
static byte phsrow[MAX_MAP_LEAFS / 8];

byte *CM_ClusterPVS(int index, int cluster) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];

  if(cluster == -1)
    memset(pvsrow, 0, (cm->numclusters + 7) >> 3);
  else
    CM_DecompressVis(cm, cm->map_visibility + cm->map_vis->bitofs[cluster][DVIS_PVS], pvsrow);
  return pvsrow;
}

byte *CM_ClusterPHS(int index, int cluster) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];

  if(cluster == -1)
    memset(phsrow, 0, (cm->numclusters + 7) >> 3);
  else
    CM_DecompressVis(cm, cm->map_visibility + cm->map_vis->bitofs[cluster][DVIS_PHS], phsrow);
  return phsrow;
}

/*
===============================================================================

AREAPORTALS

===============================================================================
*/

void FloodArea_r(struct cmodel *cm, carea_t *area, int floodnum) {
  int i;
  dareaportal_t *p;

  if(area->floodvalid == cm->floodvalid) {
    if(area->floodnum == floodnum)
      return;
    Com_Error(ERR_DROP, "FloodArea_r: reflooded");
  }

  area->floodnum = floodnum;
  area->floodvalid = cm->floodvalid;
  p = &cm->map_areaportals[area->firstareaportal];
  for(i = 0; i < area->numareaportals; i++, p++) {
    if(cm->portalopen[p->portalnum])
      FloodArea_r(cm, &cm->map_areas[p->otherarea], floodnum);
  }
}

/*
====================
FloodAreaConnections


====================
*/
void FloodAreaConnections(struct cmodel *cm) {
  int i;
  carea_t *area;
  int floodnum;

  // all current floods are now invalid
  cm->floodvalid++;
  floodnum = 0;

  // area 0 is not used
  for(i = 1; i < cm->numareas; i++) {
    area = &cm->map_areas[i];
    if(area->floodvalid == cm->floodvalid)
      continue; // already flooded into
    floodnum++;
    FloodArea_r(cm, area, floodnum);
  }
}

void CM_SetAreaPortalState(int index, int portalnum, bool open) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];

  if(portalnum > cm->numareaportals)
    Com_Error(ERR_DROP, "areaportal > numareaportals");

  cm->portalopen[portalnum] = open;
  FloodAreaConnections(cm);
}

bool CM_AreasConnected(int index, int area1, int area2) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];

  if(map_noareas->value)
    return true;

  if(area1 > cm->numareas || area2 > cm->numareas)
    Com_Error(ERR_DROP, "area > numareas");

  if(cm->map_areas[area1].floodnum == cm->map_areas[area2].floodnum)
    return true;
  return false;
}

/*
=================
CM_WriteAreaBits

Writes a length byte followed by a bit vector of all the areas
that area in the same flood as the area parameter

This is used by the client refreshes to cull visibility
=================
*/
int CM_WriteAreaBits(int index, byte *buffer, int area) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];

  int i;
  int floodnum;
  int bytes;

  bytes = (cm->numareas + 7) >> 3;

  if(map_noareas->value) { // for debugging, send everything
    memset(buffer, 255, bytes);
  } else {
    memset(buffer, 0, bytes);

    floodnum = cm->map_areas[area].floodnum;
    for(i = 0; i < cm->numareas; i++) {
      if(cm->map_areas[i].floodnum == floodnum || !area)
        buffer[i >> 3] |= 1 << (i & 7);
    }
  }

  return bytes;
}

/*
===================
CM_WritePortalState

Writes the portal state to a savegame file
===================
*/
void CM_WritePortalState(int index, FILE *f) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];

  fwrite(cm->portalopen, sizeof(cm->portalopen), 1, f);
}

/*
===================
CM_ReadPortalState

Reads the portal state from a savegame file
and recalculates the area connections
===================
*/
void CM_ReadPortalState(int index, FILE *f) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];

  FS_Read(cm->portalopen, sizeof(cm->portalopen), f);
  FloodAreaConnections(cm);
}

/*
=============
CM_HeadnodeVisible

Returns true if any leaf under headnode has a cluster that
is potentially visible
=============
*/
bool CM_HeadnodeVisible(int index, int nodenum, byte *visbits) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];

  int leafnum;
  int cluster;
  cnode_t *node;

  if(nodenum < 0) {
    leafnum = -1 - nodenum;
    cluster = cm->map_leafs[leafnum].cluster;
    if(cluster == -1)
      return false;
    if(visbits[cluster >> 3] & (1 << (cluster & 7)))
      return true;
    return false;
  }

  node = &cm->map_nodes[nodenum];
  if(CM_HeadnodeVisible(index, node->children[0], visbits))
    return true;
  return CM_HeadnodeVisible(index, node->children[1], visbits);
}

int CM_NumTextures(int index) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];

  return cm->numtexinfo;
}

const char *CM_GetTexturePrecacheName(int index, int texture) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];

  return cm->map_surfaces[texture].rname;
}

bool CM_IsActive(int index) {
  if(index < 0 || index >= 3) {
    Com_Error(ERR_DROP, "CMod_LoadBrushModel: %i is an invalid index (must be 0, 1, or 2)", index);
  }
  struct cmodel *cm = &global_cmodels[index];

  return cm->numclusters > 0;
}
