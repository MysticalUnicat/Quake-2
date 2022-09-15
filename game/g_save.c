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

#include "g_local.h"

// #define Function(f) {#f, f}

// mmove_t mmove_reloc;

static void set_classname(edict_t *e, struct field_value value) { e->classname = value.string; }
static struct field_value get_classname(const edict_t *e) {
  return (struct field_value){.type = F_LSTRING, .string = e->classname};
}

static void set_model(edict_t *e, struct field_value value) { e->model = value.string; }
static struct field_value get_model(const edict_t *e) {
  return (struct field_value){.type = F_LSTRING, .string = e->model};
}

static void set_spawnflags(edict_t *e, struct field_value value) { e->spawnflags = value.integer; }
static struct field_value get_spawnflags(const edict_t *e) {
  return (struct field_value){.type = F_INT, .integer = e->spawnflags};
}

static void set_speed(edict_t *e, struct field_value value) { e->speed = value.floating; }
static struct field_value get_speed(const edict_t *e) {
  return (struct field_value){.type = F_FLOAT, .floating = e->speed};
}

static void set_accel(edict_t *e, struct field_value value) { e->accel = value.floating; }
static struct field_value get_accel(const edict_t *e) {
  return (struct field_value){.type = F_FLOAT, .floating = e->accel};
}

static void set_decel(edict_t *e, struct field_value value) { e->decel = value.floating; }
static struct field_value get_decel(const edict_t *e) {
  return (struct field_value){.type = F_FLOAT, .floating = e->decel};
}

static void set_target(edict_t *e, struct field_value value) { e->target = value.string; }
static struct field_value get_target(const edict_t *e) {
  return (struct field_value){.type = F_LSTRING, .string = e->target};
}

static void set_targetname(edict_t *e, struct field_value value) { e->targetname = value.string; }
static struct field_value get_targetname(const edict_t *e) {
  return (struct field_value){.type = F_LSTRING, .string = e->targetname};
}

static void set_pathtarget(edict_t *e, struct field_value value) { e->pathtarget = value.string; }
static struct field_value get_pathtarget(const edict_t *e) {
  return (struct field_value){.type = F_LSTRING, .string = e->pathtarget};
}

static void set_deathtarget(edict_t *e, struct field_value value) { e->deathtarget = value.string; }
static struct field_value get_deathtarget(const edict_t *e) {
  return (struct field_value){.type = F_LSTRING, .string = e->deathtarget};
}

static void set_killtarget(edict_t *e, struct field_value value) { e->killtarget = value.string; }
static struct field_value get_killtarget(const edict_t *e) {
  return (struct field_value){.type = F_LSTRING, .string = e->killtarget};
}

static void set_combattarget(edict_t *e, struct field_value value) { e->combattarget = value.string; }
static struct field_value get_combattarget(const edict_t *e) {
  return (struct field_value){.type = F_LSTRING, .string = e->combattarget};
}

static void set_message(edict_t *e, struct field_value value) { e->message = value.string; }
static struct field_value get_message(const edict_t *e) {
  return (struct field_value){.type = F_LSTRING, .string = e->message};
}

static void set_team(edict_t *e, struct field_value value) { e->team = value.string; }
static struct field_value get_team(const edict_t *e) {
  return (struct field_value){.type = F_LSTRING, .string = e->team};
}

static void set_wait(edict_t *e, struct field_value value) { e->wait = value.floating; }
static struct field_value get_wait(const edict_t *e) {
  return (struct field_value){.type = F_FLOAT, .floating = e->wait};
}

static void set_delay(edict_t *e, struct field_value value) { e->delay = value.floating; }
static struct field_value get_delay(const edict_t *e) {
  return (struct field_value){.type = F_FLOAT, .floating = e->delay};
}

static void set_random(edict_t *e, struct field_value value) { e->random = value.floating; }
static struct field_value get_random(const edict_t *e) {
  return (struct field_value){.type = F_FLOAT, .floating = e->random};
}

static void set_move_origin(edict_t *e, struct field_value value) { VectorCopy(value.vector, e->move_origin); }
static struct field_value get_move_origin(const edict_t *e) {
  struct field_value v;
  v.type = F_VECTOR;
  VectorCopy(e->move_origin, v.vector);
  return v;
}

static void set_move_angles(edict_t *e, struct field_value value) { VectorCopy(value.vector, e->move_angles); }
static struct field_value get_move_angles(const edict_t *e) {
  struct field_value v;
  v.type = F_VECTOR;
  VectorCopy(e->move_angles, v.vector);
  return v;
}

static void set_style(edict_t *e, struct field_value value) { e->style = value.integer; }
static struct field_value get_style(const edict_t *e) {
  return (struct field_value){.type = F_INT, .integer = e->style};
}

static void set_count(edict_t *e, struct field_value value) { e->count = value.integer; }
static struct field_value get_count(const edict_t *e) {
  return (struct field_value){.type = F_INT, .integer = e->count};
}

static void set_health(edict_t *e, struct field_value value) { dv_set(ent_write_health(e), value.integer); }
static struct field_value get_health(const edict_t *e) {
  return (struct field_value){.type = F_INT, .integer = ent_read_health(e)->value};
}

static void set_sounds(edict_t *e, struct field_value value) { e->sounds = value.integer; }
static struct field_value get_sounds(const edict_t *e) {
  return (struct field_value){.type = F_INT, .integer = e->sounds};
}

static void set_dmg(edict_t *e, struct field_value value) { e->dmg = value.integer; }
static struct field_value get_dmg(const edict_t *e) { return (struct field_value){.type = F_INT, .integer = e->dmg}; }

static void set_mass(edict_t *e, struct field_value value) { e->mass = value.integer; }
static struct field_value get_mass(const edict_t *e) { return (struct field_value){.type = F_INT, .integer = e->mass}; }

static void set_volume(edict_t *e, struct field_value value) { e->volume = value.floating; }
static struct field_value get_volume(const edict_t *e) {
  return (struct field_value){.type = F_FLOAT, .floating = e->volume};
}

static void set_attenuation(edict_t *e, struct field_value value) { e->attenuation = value.floating; }
static struct field_value get_attenuation(const edict_t *e) {
  return (struct field_value){.type = F_FLOAT, .floating = e->attenuation};
}

static void set_map(edict_t *e, struct field_value value) { e->map = value.string; }
static struct field_value get_map(const edict_t *e) {
  return (struct field_value){.type = F_LSTRING, .string = e->map};
}

static void set_origin(edict_t *e, struct field_value value) { VectorCopy(value.vector, e->s.origin); }
static struct field_value get_origin(const edict_t *e) {
  struct field_value v;
  v.type = F_VECTOR;
  VectorCopy(e->s.origin, v.vector);
  return v;
}

static void set_angles(edict_t *e, struct field_value value) { VectorCopy(value.vector, e->s.angles); }
static struct field_value get_angles(const edict_t *e) {
  struct field_value v;
  v.type = F_VECTOR;
  VectorCopy(e->s.angles, v.vector);
  return v;
}

static void set_angle(edict_t *e, struct field_value value) { VectorSet(e->s.angles, 0, value.floating, 0); }
static struct field_value get_angle(const edict_t *e) {
  return (struct field_value){.type = F_ANGLEHACK, .floating = e->s.angles[1]};
}

// {"goalentity", FOFS(goalentity), F_EDICT, FFL_NOSPAWN}, {"movetarget", FOFS(movetarget), F_EDICT, FFL_NOSPAWN},
// {"enemy", FOFS(enemy), F_EDICT, FFL_NOSPAWN}, {"oldenemy", FOFS(oldenemy), F_EDICT, FFL_NOSPAWN},
// {"activator", FOFS(activator), F_EDICT, FFL_NOSPAWN}, {"groundentity", FOFS(groundentity), F_EDICT, FFL_NOSPAWN},
// {"teamchain", FOFS(teamchain), F_EDICT, FFL_NOSPAWN}, {"teammaster", FOFS(teammaster), F_EDICT, FFL_NOSPAWN},
// {"owner", FOFS(owner), F_EDICT, FFL_NOSPAWN}, {"mynoise", FOFS(mynoise), F_EDICT, FFL_NOSPAWN},
// {"mynoise2", FOFS(mynoise2), F_EDICT, FFL_NOSPAWN}, {"target_ent", FOFS(target_ent), F_EDICT, FFL_NOSPAWN},
// {"chain", FOFS(chain), F_EDICT, FFL_NOSPAWN},

// {"prethink", FOFS(prethink), F_FUNCTION, FFL_NOSPAWN}, {"think", FOFS(think), F_FUNCTION, FFL_NOSPAWN},
// {"blocked", FOFS(blocked), F_FUNCTION, FFL_NOSPAWN}, {"touch", FOFS(touch), F_FUNCTION, FFL_NOSPAWN},
// {"use", FOFS(use), F_FUNCTION, FFL_NOSPAWN}, {"pain", FOFS(pain), F_FUNCTION, FFL_NOSPAWN},
// {"die", FOFS(die), F_FUNCTION, FFL_NOSPAWN},

// {"stand", FOFS(monsterinfo.stand), F_FUNCTION, FFL_NOSPAWN},
// {"idle", FOFS(monsterinfo.idle), F_FUNCTION, FFL_NOSPAWN},
// {"search", FOFS(monsterinfo.search), F_FUNCTION, FFL_NOSPAWN},
// {"walk", FOFS(monsterinfo.walk), F_FUNCTION, FFL_NOSPAWN}, {"run", FOFS(monsterinfo.run), F_FUNCTION, FFL_NOSPAWN},
// {"dodge", FOFS(monsterinfo.dodge), F_FUNCTION, FFL_NOSPAWN},
// {"attack", FOFS(monsterinfo.attack), F_FUNCTION, FFL_NOSPAWN},
// {"melee", FOFS(monsterinfo.melee), F_FUNCTION, FFL_NOSPAWN},
// {"sight", FOFS(monsterinfo.sight), F_FUNCTION, FFL_NOSPAWN},
// {"checkattack", FOFS(monsterinfo.checkattack), F_FUNCTION, FFL_NOSPAWN},
// {"currentmove", FOFS(monsterinfo.currentmove), F_MMOVE, FFL_NOSPAWN},

// {"endfunc", FOFS(moveinfo.endfunc), F_FUNCTION, FFL_NOSPAWN},

static void set_item_spawn(edict_t *e, struct field_value value) { e->item = value.item; }

// temp spawn vars -- only valid when the spawn function is called
static void set_lip(spawn_temp_t *st, struct field_value value) { st->lip = value.integer; }
static void set_distance(spawn_temp_t *st, struct field_value value) { st->distance = value.integer; }
static void set_height(spawn_temp_t *st, struct field_value value) { st->height = value.integer; }
static void set_noise(spawn_temp_t *st, struct field_value value) { st->noise = value.string; }
static void set_pausetime(spawn_temp_t *st, struct field_value value) { st->pausetime = value.floating; }
static void set_item_temp(spawn_temp_t *st, struct field_value value) { st->item = value.string; }
static void set_gravity(spawn_temp_t *st, struct field_value value) { st->gravity = value.string; }
static void set_sky(spawn_temp_t *st, struct field_value value) { st->sky = value.string; }
static void set_skyrotate(spawn_temp_t *st, struct field_value value) { st->skyrotate = value.floating; }
static void set_skyaxis(spawn_temp_t *st, struct field_value value) { VectorCopy(value.vector, st->skyaxis); }
static void set_minyaw(spawn_temp_t *st, struct field_value value) { st->minyaw = value.floating; }
static void set_maxyaw(spawn_temp_t *st, struct field_value value) { st->maxyaw = value.floating; }
static void set_minpitch(spawn_temp_t *st, struct field_value value) { st->minpitch = value.floating; }
static void set_maxpitch(spawn_temp_t *st, struct field_value value) { st->maxpitch = value.floating; }
static void set_nextmap(spawn_temp_t *st, struct field_value value) { st->nextmap = value.string; }

field_t fields[] = {{"classname", F_LSTRING, 0, set_classname, get_classname},
                    {"model", F_LSTRING, 0, set_model, get_model},
                    {"spawnflags", F_INT, 0, set_spawnflags, get_spawnflags},
                    {"speed", F_FLOAT, 0, set_speed, get_speed},
                    {"accel", F_FLOAT, 0, set_accel, get_accel},
                    {"decel", F_FLOAT, 0, set_decel, get_decel},
                    {"target", F_LSTRING, 0, set_target, get_target},
                    {"targetname", F_LSTRING, 0, set_targetname, get_targetname},
                    {"pathtarget", F_LSTRING, 0, set_pathtarget, get_pathtarget},
                    {"deathtarget", F_LSTRING, 0, set_deathtarget, get_deathtarget},
                    {"killtarget", F_LSTRING, 0, set_killtarget, get_killtarget},
                    {"combattarget", F_LSTRING, 0, set_combattarget, get_combattarget},
                    {"message", F_LSTRING, 0, set_message, get_message},
                    {"team", F_LSTRING, 0, set_team, get_team},
                    {"wait", F_FLOAT, 0, set_wait, get_wait},
                    {"delay", F_FLOAT, 0, set_delay, get_delay},
                    {"random", F_FLOAT, 0, set_random, get_random},
                    {"move_origin", F_VECTOR, 0, set_move_origin, get_move_origin},
                    {"move_angles", F_VECTOR, 0, set_move_angles, get_move_angles},
                    {"style", F_INT, 0, set_style, get_style},
                    {"count", F_INT, 0, set_count, get_count},
                    {"health", F_INT, 0, set_health, get_health},
                    {"sounds", F_INT, 0, set_sounds, get_sounds},
                    {"light", F_IGNORE},
                    {"dmg", F_INT, 0, set_dmg, get_dmg},
                    {"mass", F_INT, 0, set_mass, get_mass},
                    {"volume", F_FLOAT, 0, set_volume, get_volume},
                    {"attenuation", F_FLOAT, 0, set_attenuation, get_attenuation},
                    {"map", F_LSTRING, 0, set_map, get_map},
                    {"origin", F_VECTOR, 0, set_origin, get_origin},
                    {"angles", F_VECTOR, 0, set_angles, get_angles},
                    {"angle", F_ANGLEHACK, 0, set_angle, get_angle},

                    // {"goalentity", FOFS(goalentity), F_EDICT, FFL_NOSPAWN},
                    // {"movetarget", FOFS(movetarget), F_EDICT, FFL_NOSPAWN},
                    // {"enemy", FOFS(enemy), F_EDICT, FFL_NOSPAWN},
                    // {"oldenemy", FOFS(oldenemy), F_EDICT, FFL_NOSPAWN},
                    // {"activator", FOFS(activator), F_EDICT, FFL_NOSPAWN},
                    // {"groundentity", FOFS(groundentity), F_EDICT, FFL_NOSPAWN},
                    // {"teamchain", FOFS(teamchain), F_EDICT, FFL_NOSPAWN},
                    // {"teammaster", FOFS(teammaster), F_EDICT, FFL_NOSPAWN},
                    // {"owner", FOFS(owner), F_EDICT, FFL_NOSPAWN},
                    // {"mynoise", FOFS(mynoise), F_EDICT, FFL_NOSPAWN},
                    // {"mynoise2", FOFS(mynoise2), F_EDICT, FFL_NOSPAWN},
                    // {"target_ent", FOFS(target_ent), F_EDICT, FFL_NOSPAWN},
                    // {"chain", FOFS(chain), F_EDICT, FFL_NOSPAWN},

                    // {"prethink", FOFS(prethink), F_FUNCTION, FFL_NOSPAWN},
                    // {"think", FOFS(think), F_FUNCTION, FFL_NOSPAWN},
                    // {"blocked", FOFS(blocked), F_FUNCTION, FFL_NOSPAWN},
                    // {"touch", FOFS(touch), F_FUNCTION, FFL_NOSPAWN},
                    // {"use", FOFS(use), F_FUNCTION, FFL_NOSPAWN},
                    // {"pain", FOFS(pain), F_FUNCTION, FFL_NOSPAWN},
                    // {"die", FOFS(die), F_FUNCTION, FFL_NOSPAWN},

                    // {"stand", FOFS(monsterinfo.stand), F_FUNCTION, FFL_NOSPAWN},
                    // {"idle", FOFS(monsterinfo.idle), F_FUNCTION, FFL_NOSPAWN},
                    // {"search", FOFS(monsterinfo.search), F_FUNCTION, FFL_NOSPAWN},
                    // {"walk", FOFS(monsterinfo.walk), F_FUNCTION, FFL_NOSPAWN},
                    // {"run", FOFS(monsterinfo.run), F_FUNCTION, FFL_NOSPAWN},
                    // {"dodge", FOFS(monsterinfo.dodge), F_FUNCTION, FFL_NOSPAWN},
                    // {"attack", FOFS(monsterinfo.attack), F_FUNCTION, FFL_NOSPAWN},
                    // {"melee", FOFS(monsterinfo.melee), F_FUNCTION, FFL_NOSPAWN},
                    // {"sight", FOFS(monsterinfo.sight), F_FUNCTION, FFL_NOSPAWN},
                    // {"checkattack", FOFS(monsterinfo.checkattack), F_FUNCTION, FFL_NOSPAWN},
                    // {"currentmove", FOFS(monsterinfo.currentmove), F_MMOVE, FFL_NOSPAWN},

                    // {"endfunc", FOFS(moveinfo.endfunc), F_FUNCTION, FFL_NOSPAWN},

                    // temp spawn vars -- only valid when the spawn function is called
                    {"lip", F_INT, FFL_SPAWNTEMP, NULL, NULL, set_lip},
                    {"distance", F_INT, FFL_SPAWNTEMP, NULL, NULL, set_distance},
                    {"height", F_INT, FFL_SPAWNTEMP, NULL, NULL, set_height},
                    {"noise", F_LSTRING, FFL_SPAWNTEMP, NULL, NULL, set_noise},
                    {"pausetime", F_FLOAT, FFL_SPAWNTEMP, NULL, NULL, set_pausetime},
                    {"item", F_LSTRING, FFL_SPAWNTEMP, NULL, NULL, set_item_temp},

                    // need for item field in edict struct, FFL_SPAWNTEMP item will be skipped on saves
                    {"item", F_ITEM, 0, set_item_spawn, NULL},

                    {"gravity", F_LSTRING, FFL_SPAWNTEMP, NULL, NULL, set_gravity},
                    {"sky", F_LSTRING, FFL_SPAWNTEMP, NULL, NULL, set_sky},
                    {"skyrotate", F_FLOAT, FFL_SPAWNTEMP, NULL, NULL, set_skyrotate},
                    {"skyaxis", F_VECTOR, FFL_SPAWNTEMP, NULL, NULL, set_skyaxis},
                    {"minyaw", F_FLOAT, FFL_SPAWNTEMP, NULL, NULL, set_minyaw},
                    {"maxyaw", F_FLOAT, FFL_SPAWNTEMP, NULL, NULL, set_maxyaw},
                    {"minpitch", F_FLOAT, FFL_SPAWNTEMP, NULL, NULL, set_minpitch},
                    {"maxpitch", F_FLOAT, FFL_SPAWNTEMP, NULL, NULL, set_maxpitch},
                    {"nextmap", F_LSTRING, FFL_SPAWNTEMP, NULL, NULL, set_nextmap},

                    {0, 0, 0, 0}

};

// field_t		levelfields[] =
// {
// 	{"changemap", LLOFS(changemap), F_LSTRING},

// 	{"sight_client", LLOFS(sight_client), F_EDICT},
// 	{"sight_entity", LLOFS(sight_entity), F_EDICT},
// 	{"sound_entity", LLOFS(sound_entity), F_EDICT},
// 	{"sound2_entity", LLOFS(sound2_entity), F_EDICT},

// 	{NULL, 0, F_INT}
// };

// field_t		clientfields[] =
// {
// 	{"pers.weapon", CLOFS(pers.weapon), F_ITEM},
// 	{"pers.lastweapon", CLOFS(pers.lastweapon), F_ITEM},
// 	{"newweapon", CLOFS(newweapon), F_ITEM},

// 	{NULL, 0, F_INT}
// };

/*
============
InitGame

This will be called when the dll is first loaded, which
only happens when a new game is started or a save game
is loaded.
============
*/
void InitGame(void) {
  gi.dprintf("==== InitGame ====\n");

  gun_x = gi.cvar("gun_x", "0", 0);
  gun_y = gi.cvar("gun_y", "0", 0);
  gun_z = gi.cvar("gun_z", "0", 0);

  // FIXME: sv_ prefix is wrong for these
  sv_rollspeed = gi.cvar("sv_rollspeed", "200", 0);
  sv_rollangle = gi.cvar("sv_rollangle", "2", 0);
  sv_maxvelocity = gi.cvar("sv_maxvelocity", "2000", 0);
  sv_gravity = gi.cvar("sv_gravity", "800", 0);

  // noset vars
  dedicated = gi.cvar("dedicated", "0", CVAR_NOSET);

  // latched vars
  sv_cheats = gi.cvar("cheats", "0", CVAR_SERVERINFO | CVAR_LATCH);
  gi.cvar("gamename", GAMEVERSION, CVAR_SERVERINFO | CVAR_LATCH);
  gi.cvar("gamedate", __DATE__, CVAR_SERVERINFO | CVAR_LATCH);

  maxclients = gi.cvar("maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH);
  maxspectators = gi.cvar("maxspectators", "4", CVAR_SERVERINFO);
  deathmatch = gi.cvar("deathmatch", "0", CVAR_LATCH);
  coop = gi.cvar("coop", "0", CVAR_LATCH);
  skill = gi.cvar("skill", "1", CVAR_LATCH);
  maxentities = gi.cvar("maxentities", "1024", CVAR_LATCH);

  // change anytime vars
  dmflags = gi.cvar("dmflags", "0", CVAR_SERVERINFO);
  fraglimit = gi.cvar("fraglimit", "0", CVAR_SERVERINFO);
  timelimit = gi.cvar("timelimit", "0", CVAR_SERVERINFO);
  password = gi.cvar("password", "", CVAR_USERINFO);
  spectator_password = gi.cvar("spectator_password", "", CVAR_USERINFO);
  filterban = gi.cvar("filterban", "1", 0);

  g_select_empty = gi.cvar("g_select_empty", "0", CVAR_ARCHIVE);

  run_pitch = gi.cvar("run_pitch", "0.002", 0);
  run_roll = gi.cvar("run_roll", "0.005", 0);
  bob_up = gi.cvar("bob_up", "0.005", 0);
  bob_pitch = gi.cvar("bob_pitch", "0.002", 0);
  bob_roll = gi.cvar("bob_roll", "0.002", 0);

  // flood control
  flood_msgs = gi.cvar("flood_msgs", "4", 0);
  flood_persecond = gi.cvar("flood_persecond", "4", 0);
  flood_waitdelay = gi.cvar("flood_waitdelay", "10", 0);

  // dm map list
  sv_maplist = gi.cvar("sv_maplist", "", 0);

  // items
  InitItems();

  Com_sprintf(game.helpmessage1, sizeof(game.helpmessage1), "");

  Com_sprintf(game.helpmessage2, sizeof(game.helpmessage2), "");

  // initialize all entities for this game
  game.maxentities = maxentities->value;
  g_edicts = gi.TagMalloc(game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
  globals.edicts = g_edicts;
  globals.max_edicts = game.maxentities;

  // initialize all clients for this game
  game.maxclients = maxclients->value;
  game.clients = gi.TagMalloc(game.maxclients * sizeof(game.clients[0]), TAG_GAME);
  globals.num_edicts = game.maxclients + 1;
}

//=========================================================

// void WriteField1(FILE *f, field_t *field, byte *base) {
//   void *p;
//   int len;
//   int index;

//   if(field->flags & FFL_SPAWNTEMP)
//     return;

//   p = (void *)(base + field->ofs);
//   switch(field->type) {
//   case F_INT:
//   case F_FLOAT:
//   case F_ANGLEHACK:
//   case F_VECTOR:
//   case F_IGNORE:
//     break;

//   case F_LSTRING:
//   case F_GSTRING:
//     if(*(char **)p)
//       len = strlen(*(char **)p) + 1;
//     else
//       len = 0;
//     *(int *)p = len;
//     break;
//   case F_EDICT:
//     if(*(edict_t **)p == NULL)
//       index = -1;
//     else
//       index = *(edict_t **)p - g_edicts;
//     *(int *)p = index;
//     break;
//   case F_CLIENT:
//     if(*(gclient_t **)p == NULL)
//       index = -1;
//     else
//       index = *(gclient_t **)p - game.clients;
//     *(int *)p = index;
//     break;
//   case F_ITEM:
//     if(*(edict_t **)p == NULL)
//       index = -1;
//     else
//       index = *(gitem_t **)p - itemlist;
//     *(int *)p = index;
//     break;

//   // relative to code segment
//   case F_FUNCTION:
//     if(*(byte **)p == NULL)
//       index = 0;
//     else
//       index = *(byte **)p - ((byte *)InitGame);
//     *(int *)p = index;
//     break;

//   // relative to data segment
//   case F_MMOVE:
//     if(*(byte **)p == NULL)
//       index = 0;
//     else
//       index = *(byte **)p - (byte *)&mmove_reloc;
//     *(int *)p = index;
//     break;

//   default:
//     gi.error("WriteEdict: unknown field type");
//   }
// }

// void WriteField2(FILE *f, field_t *field, byte *base) {
//   int len;
//   void *p;

//   if(field->flags & FFL_SPAWNTEMP)
//     return;

//   p = (void *)(base + field->ofs);
//   switch(field->type) {
//   case F_LSTRING:
//     if(*(char **)p) {
//       len = strlen(*(char **)p) + 1;
//       fwrite(*(char **)p, len, 1, f);
//     }
//     break;
//   }
// }

// void ReadField(FILE *f, field_t *field, byte *base) {
//   void *p;
//   int len;
//   int index;

//   if(field->flags & FFL_SPAWNTEMP)
//     return;

//   p = (void *)(base + field->ofs);
//   switch(field->type) {
//   case F_INT:
//   case F_FLOAT:
//   case F_ANGLEHACK:
//   case F_VECTOR:
//   case F_IGNORE:
//     break;

//   case F_LSTRING:
//     len = *(int *)p;
//     if(!len)
//       *(char **)p = NULL;
//     else {
//       *(char **)p = gi.TagMalloc(len, TAG_LEVEL);
//       fread(*(char **)p, len, 1, f);
//     }
//     break;
//   case F_EDICT:
//     index = *(int *)p;
//     if(index == -1)
//       *(edict_t **)p = NULL;
//     else
//       *(edict_t **)p = &g_edicts[index];
//     break;
//   case F_CLIENT:
//     index = *(int *)p;
//     if(index == -1)
//       *(gclient_t **)p = NULL;
//     else
//       *(gclient_t **)p = &game.clients[index];
//     break;
//   case F_ITEM:
//     index = *(int *)p;
//     if(index == -1)
//       *(gitem_t **)p = NULL;
//     else
//       *(gitem_t **)p = &itemlist[index];
//     break;

//   // relative to code segment
//   case F_FUNCTION:
//     index = *(int *)p;
//     if(index == 0)
//       *(byte **)p = NULL;
//     else
//       *(byte **)p = ((byte *)InitGame) + index;
//     break;

//   // relative to data segment
//   case F_MMOVE:
//     index = *(int *)p;
//     if(index == 0)
//       *(byte **)p = NULL;
//     else
//       *(byte **)p = (byte *)&mmove_reloc + index;
//     break;

//   default:
//     gi.error("ReadEdict: unknown field type");
//   }
// }

//=========================================================

// /*
// ==============
// WriteClient

// All pointer variables (except function pointers) must be handled specially.
// ==============
// */
// void WriteClient(FILE *f, gclient_t *client) {
//   field_t *field;
//   gclient_t temp;

//   // all of the ints, floats, and vectors stay as they are
//   temp = *client;

//   // change the pointers to lengths or indexes
//   for(field = clientfields; field->name; field++) {
//     WriteField1(f, field, (byte *)&temp);
//   }

//   // write the block
//   fwrite(&temp, sizeof(temp), 1, f);

//   // now write any allocated data following the edict
//   for(field = clientfields; field->name; field++) {
//     WriteField2(f, field, (byte *)client);
//   }
// }

// /*
// ==============
// ReadClient

// All pointer variables (except function pointers) must be handled specially.
// ==============
// */
// void ReadClient(FILE *f, gclient_t *client) {
//   field_t *field;

//   fread(client, sizeof(*client), 1, f);

//   for(field = clientfields; field->name; field++) {
//     ReadField(f, field, (byte *)client);
//   }
// }

/*
============
WriteGame

This will be called whenever the game goes to a new level,
and when the user explicitly saves the game.

Game information include cross level data, like multi level
triggers, help computer info, and all client states.

A single player death will automatically restore from the
last save position.
============
*/
void WriteGame(char *filename, qboolean autosave) {
  gi.dprintf("unimplemented (WriteGame)");
  //   FILE *f;
  //   int i;
  //   char str[16];

  //   if(!autosave)
  //     SaveClientData();

  //   f = fopen(filename, "wb");
  //   if(!f)
  //     gi.error("Couldn't open %s", filename);

  //   memset(str, 0, sizeof(str));
  //   strcpy(str, __DATE__);
  //   fwrite(str, sizeof(str), 1, f);

  //   game.autosaved = autosave;
  //   fwrite(&game, sizeof(game), 1, f);
  //   game.autosaved = false;

  //   for(i = 0; i < game.maxclients; i++)
  //     WriteClient(f, &game.clients[i]);

  //   fclose(f);
}

void ReadGame(char *filename) {
  gi.error("unimplemented (ReadGame)");
  //   FILE *f;
  //   int i;
  //   char str[16];

  //   gi.FreeTags(TAG_GAME);

  //   f = fopen(filename, "rb");
  //   if(!f)
  //     gi.error("Couldn't open %s", filename);

  //   fread(str, sizeof(str), 1, f);
  //   if(strcmp(str, __DATE__)) {
  //     fclose(f);
  //     gi.error("Savegame from an older version.\n");
  //   }

  //   g_edicts = gi.TagMalloc(game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
  //   globals.edicts = g_edicts;

  //   fread(&game, sizeof(game), 1, f);
  //   game.clients = gi.TagMalloc(game.maxclients * sizeof(game.clients[0]), TAG_GAME);
  //   for(i = 0; i < game.maxclients; i++)
  //     ReadClient(f, &game.clients[i]);

  //   fclose(f);
}

//==========================================================

// /*
// ==============
// WriteEdict

// All pointer variables (except function pointers) must be handled specially.
// ==============
// */
// void WriteEdict(FILE *f, edict_t *ent) {
//   field_t *field;
//   edict_t temp;

//   // all of the ints, floats, and vectors stay as they are
//   temp = *ent;

//   // change the pointers to lengths or indexes
//   for(field = fields; field->name; field++) {
//     WriteField1(f, field, (byte *)&temp);
//   }

//   // write the block
//   fwrite(&temp, sizeof(temp), 1, f);

//   // now write any allocated data following the edict
//   for(field = fields; field->name; field++) {
//     WriteField2(f, field, (byte *)ent);
//   }
// }

// /*
// ==============
// WriteLevelLocals

// All pointer variables (except function pointers) must be handled specially.
// ==============
// */
// void WriteLevelLocals(FILE *f) {
//   field_t *field;
//   level_locals_t temp;

//   // all of the ints, floats, and vectors stay as they are
//   temp = level;

//   // change the pointers to lengths or indexes
//   for(field = levelfields; field->name; field++) {
//     WriteField1(f, field, (byte *)&temp);
//   }

//   // write the block
//   fwrite(&temp, sizeof(temp), 1, f);

//   // now write any allocated data following the edict
//   for(field = levelfields; field->name; field++) {
//     WriteField2(f, field, (byte *)&level);
//   }
// }

// /*
// ==============
// ReadEdict

// All pointer variables (except function pointers) must be handled specially.
// ==============
// */
// void ReadEdict(FILE *f, edict_t *ent) {
//   field_t *field;

//   fread(ent, sizeof(*ent), 1, f);

//   for(field = fields; field->name; field++) {
//     ReadField(f, field, (byte *)ent);
//   }
// }

// /*
// ==============
// ReadLevelLocals

// All pointer variables (except function pointers) must be handled specially.
// ==============
// */
// void ReadLevelLocals(FILE *f) {
//   field_t *field;

//   fread(&level, sizeof(level), 1, f);

//   for(field = levelfields; field->name; field++) {
//     ReadField(f, field, (byte *)&level);
//   }
// }

/*
=================
WriteLevel

=================
*/
void WriteLevel(char *filename) {
  gi.dprintf("unimplemented (WriteLevel)");
  //   int i;
  //   edict_t *ent;
  //   FILE *f;
  //   void *base;

  //   f = fopen(filename, "wb");
  //   if(!f)
  //     gi.error("Couldn't open %s", filename);

  //   // write out edict size for checking
  //   i = sizeof(edict_t);
  //   fwrite(&i, sizeof(i), 1, f);

  //   // write out a function pointer for checking
  //   base = (void *)InitGame;
  //   fwrite(&base, sizeof(base), 1, f);

  //   // write out level_locals_t
  //   WriteLevelLocals(f);

  //   // write out all the entities
  //   for(i = 0; i < globals.num_edicts; i++) {
  //     ent = &g_edicts[i];
  //     if(!ent->inuse)
  //       continue;
  //     fwrite(&i, sizeof(i), 1, f);
  //     WriteEdict(f, ent);
  //   }
  //   i = -1;
  //   fwrite(&i, sizeof(i), 1, f);

  //   fclose(f);
}

/*
=================
ReadLevel

SpawnEntities will allready have been called on the
level the same way it was when the level was saved.

That is necessary to get the baselines
set up identically.

The server will have cleared all of the world links before
calling ReadLevel.

No clients are connected yet.
=================
*/
void ReadLevel(char *filename) {
  gi.error("unimplemented (ReadLevel)");

  //   int entnum;
  //   FILE *f;
  //   int i;
  //   void *base;
  //   edict_t *ent;

  //   f = fopen(filename, "rb");
  //   if(!f)
  //     gi.error("Couldn't open %s", filename);

  //   // free any dynamic memory allocated by loading the level
  //   // base state
  //   gi.FreeTags(TAG_LEVEL);

  //   // wipe all the entities
  //   memset(g_edicts, 0, game.maxentities * sizeof(g_edicts[0]));
  //   globals.num_edicts = maxclients->value + 1;

  //   // check edict size
  //   fread(&i, sizeof(i), 1, f);
  //   if(i != sizeof(edict_t)) {
  //     fclose(f);
  //     gi.error("ReadLevel: mismatched edict size");
  //   }

  //   // check function pointer base address
  //   fread(&base, sizeof(base), 1, f);
  // #ifdef _WIN32
  //   if(base != (void *)InitGame) {
  //     fclose(f);
  //     gi.error("ReadLevel: function pointers have moved");
  //   }
  // #else
  //   gi.dprintf("Function offsets %d\n", ((byte *)base) - ((byte *)InitGame));
  // #endif

  //   // load the level locals
  //   ReadLevelLocals(f);

  //   // load all the entities
  //   while(1) {
  //     if(fread(&entnum, sizeof(entnum), 1, f) != 1) {
  //       fclose(f);
  //       gi.error("ReadLevel: failed to read entnum");
  //     }
  //     if(entnum == -1)
  //       break;
  //     if(entnum >= globals.num_edicts)
  //       globals.num_edicts = entnum + 1;

  //     ent = &g_edicts[entnum];
  //     ReadEdict(f, ent);

  //     // let the server rebuild world links for this ent
  //     memset(&ent->area, 0, sizeof(ent->area));
  //     gi.linkentity(ent);
  //   }

  //   fclose(f);

  //   // mark all clients as unconnected
  //   for(i = 0; i < maxclients->value; i++) {
  //     ent = &g_edicts[i + 1];
  //     ent->client = game.clients + i;
  //     ent->client->pers.connected = false;
  //   }

  //   // do any load time things at this point
  //   for(i = 0; i < globals.num_edicts; i++) {
  //     ent = &g_edicts[i];

  //     if(!ent->inuse)
  //       continue;

  //     // fire any cross-level triggers
  //     if(ent->classname)
  //       if(strcmp(ent->classname, "target_crosslevel_target") == 0)
  //         ent->nextthink = level.time + ent->delay;
  //   }
}
