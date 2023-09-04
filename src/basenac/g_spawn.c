/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2019, NVIDIA CORPORATION. All rights reserved.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "g_local.h"

typedef struct {
    char    *name;
    void (*spawn)(edict_t *ent);
    entity_type_t type;
} spawn_func_t;

typedef struct {
    char    *name;
    unsigned ofs;
    fieldtype_t type;
} spawn_field_t;

void SP_info_player_start(edict_t *ent);
void SP_info_player_deathmatch(edict_t *ent);
void SP_info_player_coop(edict_t *ent);
void SP_info_player_intermission(edict_t *ent);

void SP_func_plat(edict_t *ent);
void SP_func_rotating(edict_t *ent);
void SP_func_button(edict_t *ent);
void SP_func_door(edict_t *ent);
void SP_func_door_secret(edict_t *ent);
void SP_func_door_rotating(edict_t *ent);
void SP_func_water(edict_t *ent);
void SP_func_train(edict_t *ent);
void SP_func_conveyor(edict_t *self);
void SP_func_wall(edict_t *self);
void SP_func_object(edict_t *self);
void SP_func_explosive(edict_t *self);
void SP_func_timer(edict_t *self);
void SP_func_areaportal(edict_t *ent);
void SP_func_clock(edict_t *ent);
void SP_func_killbox(edict_t *ent);

void SP_trigger_always(edict_t *ent);
void SP_trigger_once(edict_t *ent);
void SP_trigger_multiple(edict_t *ent);
void SP_trigger_relay(edict_t *ent);
void SP_trigger_push(edict_t *ent);
void SP_trigger_hurt(edict_t *ent);
void SP_trigger_key(edict_t *ent);
void SP_trigger_counter(edict_t *ent);
void SP_trigger_elevator(edict_t *ent);
void SP_trigger_gravity(edict_t *ent);
void SP_trigger_monsterjump(edict_t *ent);
void SP_trigger_teleport(edict_t *ent);

void SP_target_temp_entity(edict_t *ent);
void SP_target_speaker(edict_t *ent);
void SP_target_explosion(edict_t *ent);
void SP_target_changelevel(edict_t *ent);
void SP_target_secret(edict_t *ent);
void SP_target_goal(edict_t *ent);
void SP_target_splash(edict_t *ent);
void SP_target_spawner(edict_t *ent);
void SP_target_blaster(edict_t *ent);
void SP_target_crosslevel_trigger(edict_t *ent);
void SP_target_crosslevel_target(edict_t *ent);
void SP_target_laser(edict_t *self);
void SP_target_help(edict_t *ent);
void SP_target_lightramp(edict_t *self);
void SP_target_earthquake(edict_t *ent);
void SP_target_character(edict_t *ent);
void SP_target_string(edict_t *ent);
void SP_target_setskill(edict_t *ent);

// Paril: gravity change support
void SP_target_gravity(edict_t *ent);
void SP_target_removeweapons(edict_t *self);

void SP_worldspawn(edict_t *ent);

void SP_light(edict_t *self);
void SP_light_mine1(edict_t *ent);
void SP_light_mine2(edict_t *ent);
void SP_info_null(edict_t *self);
void SP_info_notnull(edict_t *self);
void SP_path_corner(edict_t *self);
void SP_point_combat(edict_t *self);

void SP_misc_explobox(edict_t *self);
void SP_misc_explobox2(edict_t* self);
void SP_misc_banner(edict_t *self);
void SP_misc_satellite_dish(edict_t *self);
void SP_misc_gib_arm(edict_t *self);
void SP_misc_gib_leg(edict_t *self);
void SP_misc_gib_head(edict_t *self);
void SP_misc_insane(edict_t *self);
void SP_misc_deadsoldier(edict_t *self);
void SP_misc_viper(edict_t *self);
void SP_misc_viper_bomb(edict_t *self);
void SP_misc_bigviper(edict_t *self);
void SP_misc_strogg_ship(edict_t *self);
void SP_misc_teleporter(edict_t *self);
void SP_misc_teleporter_dest(edict_t *self);
void SP_misc_blackhole(edict_t *self);
void SP_misc_eastertank(edict_t *self);
void SP_misc_easterchick(edict_t *self);
void SP_misc_easterchick2(edict_t *self);

void SP_monster_berserk(edict_t *self);
void SP_monster_gladiator(edict_t *self);
void SP_monster_gunner(edict_t *self);
void SP_monster_infantry(edict_t *self);
void SP_monster_soldier_light(edict_t *self);
void SP_monster_soldier(edict_t *self);
void SP_monster_soldier_ss(edict_t *self);
void SP_monster_tank(edict_t *self);
void SP_monster_medic(edict_t *self);
void SP_monster_flipper(edict_t *self);
void SP_monster_chick(edict_t *self);
void SP_monster_parasite(edict_t *self);
void SP_monster_flyer(edict_t *self);
void SP_monster_brain(edict_t *self);
void SP_monster_floater(edict_t *self);
void SP_monster_hover(edict_t *self);
void SP_monster_mutant(edict_t *self);
void SP_monster_supertank(edict_t *self);
void SP_monster_boss2(edict_t *self);
void SP_monster_jorg(edict_t *self);
void SP_monster_boss3_stand(edict_t *self);

void SP_monster_commander_body(edict_t *self);

void SP_turret_breach(edict_t *self);
void SP_turret_base(edict_t *self);
void SP_turret_driver(edict_t *self);

// Paril
void SP_model_spawn(edict_t *ent);
void SP_misc_property_swap(edict_t *ent);
void SP_monster_knight(edict_t *self);
void SP_monster_grunt(edict_t *self);
void SP_monster_enforcer(edict_t *self);
void SP_monster_fiend(edict_t *self);
void SP_monster_vore(edict_t *self);
void SP_monster_scrag(edict_t* self);
void SP_env_reverb(edict_t *ent);
void SP_target_spotlight(edict_t *self);

static const spawn_func_t spawn_funcs[] = {
    {"info_player_start", SP_info_player_start, ENT_PRIVATE},
    {"info_player_deathmatch", SP_info_player_deathmatch},
    {"info_player_coop", SP_info_player_coop, ENT_PRIVATE},
    {"info_player_intermission", SP_info_player_intermission, ENT_PRIVATE},

    {"func_plat", SP_func_plat},
    {"func_button", SP_func_button},
    {"func_door", SP_func_door},
    {"func_door_secret", SP_func_door_secret},
    {"func_door_rotating", SP_func_door_rotating},
    {"func_rotating", SP_func_rotating},
    {"func_train", SP_func_train},
    {"func_water", SP_func_water},
    {"func_conveyor", SP_func_conveyor},
    {"func_areaportal", SP_func_areaportal, ENT_PRIVATE},
    {"func_clock", SP_func_clock, ENT_PRIVATE},
    {"func_wall", SP_func_wall, ENT_AMBIENT},
    {"func_object", SP_func_object},
    {"func_timer", SP_func_timer, ENT_PRIVATE},
    {"func_explosive", SP_func_explosive, ENT_AMBIENT},
    {"func_killbox", SP_func_killbox, ENT_PRIVATE},

    {"trigger_always", SP_trigger_always, ENT_PRIVATE},
    {"trigger_once", SP_trigger_once, ENT_PRIVATE},
    {"trigger_multiple", SP_trigger_multiple, ENT_PRIVATE},
    {"trigger_relay", SP_trigger_relay, ENT_PRIVATE},
    {"trigger_push", SP_trigger_push, ENT_PRIVATE},
    {"trigger_hurt", SP_trigger_hurt, ENT_PRIVATE},
    {"trigger_key", SP_trigger_key, ENT_PRIVATE},
    {"trigger_counter", SP_trigger_counter, ENT_PRIVATE},
    {"trigger_elevator", SP_trigger_elevator, ENT_PRIVATE},
    {"trigger_gravity", SP_trigger_gravity, ENT_PRIVATE},
    {"trigger_monsterjump", SP_trigger_monsterjump, ENT_PRIVATE},
    {"trigger_teleport", SP_trigger_teleport},

    {"target_temp_entity", SP_target_temp_entity, ENT_PRIVATE},
    {"target_speaker", SP_target_speaker, ENT_AMBIENT},
    {"target_explosion", SP_target_explosion, ENT_PRIVATE},
    {"target_changelevel", SP_target_changelevel, ENT_PRIVATE},
    {"target_secret", SP_target_secret},
  //{"target_secret", SP_target_secret, ENT_PRIVATE},
    {"target_goal", SP_target_goal, ENT_AMBIENT},
    {"target_splash", SP_target_splash, ENT_PRIVATE},
    {"target_spawner", SP_target_spawner, ENT_PRIVATE},
    {"target_blaster", SP_target_blaster},
    {"target_crosslevel_trigger", SP_target_crosslevel_trigger, ENT_PRIVATE},
    {"target_crosslevel_target", SP_target_crosslevel_target, ENT_PRIVATE},
    {"target_laser", SP_target_laser},
    {"target_spotlight", SP_target_spotlight},
    {"target_help", SP_target_help, ENT_PRIVATE},
    {"target_lightramp", SP_target_lightramp, ENT_PRIVATE},
    {"target_earthquake", SP_target_earthquake, ENT_PRIVATE},
    {"target_character", SP_target_character},
    {"target_string", SP_target_string, ENT_PRIVATE},
    {"target_setskill", SP_target_setskill, ENT_PRIVATE},

    // Paril: gravity change support
    {"target_gravity", SP_target_gravity, ENT_PRIVATE},
    {"target_removeweapons", SP_target_removeweapons, ENT_PRIVATE},

    {"worldspawn", SP_worldspawn},

    {"light", SP_light, ENT_PRIVATE},
    {"light_mine1", SP_light_mine1, ENT_AMBIENT},
    {"light_mine2", SP_light_mine2, ENT_AMBIENT},
    {"info_null", SP_info_null},
    {"func_group", SP_info_null},
    {"info_notnull", SP_info_notnull, ENT_PRIVATE},
    {"path_corner", SP_path_corner, ENT_PRIVATE},
    {"point_combat", SP_point_combat, ENT_PRIVATE},

    {"misc_explobox", SP_misc_explobox},
    {"misc_explobox2", SP_misc_explobox2},
    {"misc_banner", SP_misc_banner},
    {"misc_satellite_dish", SP_misc_satellite_dish},
    {"misc_gib_arm", SP_misc_gib_arm},
    {"misc_gib_leg", SP_misc_gib_leg},
    {"misc_gib_head", SP_misc_gib_head},
    {"misc_insane", SP_misc_insane},
    {"misc_deadsoldier", SP_misc_deadsoldier, ENT_AMBIENT},
    {"misc_viper", SP_misc_viper},
    {"misc_viper_bomb", SP_misc_viper_bomb},
    {"misc_bigviper", SP_misc_bigviper},
    {"misc_strogg_ship", SP_misc_strogg_ship},
    {"misc_teleporter", SP_misc_teleporter},
    {"misc_teleporter_dest", SP_misc_teleporter_dest},
    {"misc_blackhole", SP_misc_blackhole},
    {"misc_eastertank", SP_misc_eastertank},
    {"misc_easterchick", SP_misc_easterchick},
    {"misc_easterchick2", SP_misc_easterchick2},

    {"monster_berserk", SP_monster_berserk},
    {"monster_gladiator", SP_monster_gladiator},
    {"monster_gunner", SP_monster_gunner},
    {"monster_infantry", SP_monster_infantry},
    {"monster_soldier_light", SP_monster_soldier_light},
    {"monster_soldier", SP_monster_soldier},
    {"monster_soldier_ss", SP_monster_soldier_ss},
    {"monster_tank", SP_monster_tank},
    {"monster_tank_commander", SP_monster_tank},
    {"monster_medic", SP_monster_medic},
    {"monster_flipper", SP_monster_flipper},
    {"monster_chick", SP_monster_chick},
    {"monster_parasite", SP_monster_parasite},
    {"monster_flyer", SP_monster_flyer},
    {"monster_brain", SP_monster_brain},
    {"monster_floater", SP_monster_floater},
    {"monster_hover", SP_monster_hover},
    {"monster_mutant", SP_monster_mutant},
    {"monster_supertank", SP_monster_supertank},
    {"monster_boss2", SP_monster_boss2},
    {"monster_boss3_stand", SP_monster_boss3_stand},
    {"monster_jorg", SP_monster_jorg},

    {"monster_commander_body", SP_monster_commander_body},

    {"turret_breach", SP_turret_breach},
    {"turret_base", SP_turret_base},
    {"turret_driver", SP_turret_driver},

    // Paril
    { "model_spawn", SP_model_spawn },
    { "misc_property_swap", SP_misc_property_swap, ENT_PRIVATE },

    // NAC
    { "monster_grunt", SP_monster_grunt },
    { "monster_enforcer", SP_monster_enforcer },
    { "monster_knight", SP_monster_knight },
    { "monster_fiend", SP_monster_fiend },
    { "monster_vore", SP_monster_vore },
    { "monster_scrag", SP_monster_scrag },
    { "env_reverb", SP_env_reverb, ENT_PRIVATE },

    {NULL, NULL}
};

static const spawn_field_t spawn_fields[] = {
    {"classname", FOFS(classname), F_LSTRING},
    {"model", FOFS(model), F_LSTRING},
    {"model2", FOFS(model2), F_LSTRING},
    {"model3", FOFS(model3), F_LSTRING},
    {"model4", FOFS(model4), F_LSTRING},
    {"spawnflags", FOFS(spawnflags), F_INT},
    {"speed", FOFS(speed), F_FLOAT},
    {"accel", FOFS(accel), F_FLOAT},
    {"decel", FOFS(decel), F_FLOAT},
    {"target", FOFS(target), F_LSTRING},
    {"targetname", FOFS(targetname), F_LSTRING},
    {"pathtarget", FOFS(pathtarget), F_LSTRING},
    {"deathtarget", FOFS(deathtarget), F_LSTRING},
    {"killtarget", FOFS(killtarget), F_LSTRING},
    {"combattarget", FOFS(combattarget), F_LSTRING},
    {"message", FOFS(message), F_LSTRING},
    {"team", FOFS(team), F_LSTRING},
    {"wait", FOFS(wait), F_FLOAT},
    {"delay", FOFS(delay), F_FLOAT},
    {"random", FOFS(random), F_FLOAT},
    {"move_origin", FOFS(move_origin), F_VECTOR},
    {"move_angles", FOFS(move_angles), F_VECTOR},
    {"style", FOFS(style), F_INT},
    {"count", FOFS(count), F_INT},
    {"health", FOFS(health), F_INT},
    {"sounds", FOFS(sounds), F_INT},
    {"light", 0, F_IGNORE},
    {"dmg", FOFS(dmg), F_INT},
    {"mass", FOFS(mass), F_INT},
    {"volume", FOFS(volume), F_FLOAT},
    {"attenuation", FOFS(attenuation), F_FLOAT},
    {"map", FOFS(map), F_LSTRING},
    {"origin", FOFS(s.origin), F_VECTOR},
    {"angles", FOFS(s.angles), F_VECTOR},
    {"angle", FOFS(s.angles), F_ANGLEHACK},

    // Paril - entity animation stuff
    {"anim_start", FOFS(anim.start), F_INT},
    {"anim_end", FOFS(anim.end), F_INT},
    {"anim_frame_delay", FOFS(anim.frame_delay), F_INT},
    {"anim_animating", FOFS(anim.animating), F_INT},
    {"anim_reset_on_trigger", FOFS(anim.reset_on_trigger), F_INT},
    {"anim_target", FOFS(anim.target), F_LSTRING},
    {"anim_count", FOFS(anim.count), F_INT},
    {"anim_finished_target", FOFS(anim.finished_target), F_LSTRING},

    // Paril - miscmodel
    {"skinnum", FOFS(s.skinnum), F_INT},
    {"avelocity", FOFS(avelocity), F_VECTOR},
    {"effects", FOFS(s.effects), F_INT},
    {"renderfx", FOFS(s.renderfx), F_INT},
    {"frame", FOFS(s.frame), F_INT},

    // Paril: switchable light style support
    {"style2", FOFS(style2), F_LSTRING},

    // NAC; env_reverb
    {"radius", FOFS(radius), F_FLOAT},

    // lightweight triggers
    {"mins", FOFS(mins), F_VECTOR},
    {"maxs", FOFS(maxs), F_VECTOR},

    {NULL}
};

// temp spawn vars -- only valid when the spawn function is called
static const spawn_field_t temp_fields[] = {
    {"lip", STOFS(lip), F_INT},
    {"distance", STOFS(distance), F_INT},
    {"height", STOFS(height), F_INT},
    {"noise", STOFS(noise), F_LSTRING},
    {"pausetime", STOFS(pausetime), F_FLOAT},
    {"item", STOFS(item), F_LSTRING},

    {"gravity", STOFS(gravity), F_LSTRING},
    {"sky", STOFS(sky), F_LSTRING},
    {"skyrotate", STOFS(skyrotate), F_FLOAT},
    {"skyautorotate", STOFS(skyautorotate), F_INT},
    {"skyaxis", STOFS(skyaxis), F_VECTOR},
    {"minyaw", STOFS(minyaw), F_FLOAT},
    {"maxyaw", STOFS(maxyaw), F_FLOAT},
    {"minpitch", STOFS(minpitch), F_FLOAT},
    {"maxpitch", STOFS(maxpitch), F_FLOAT},
    {"nextmap", STOFS(nextmap), F_LSTRING},

    {"default_reverb", STOFS(default_reverb), F_INT},

    {"color", STOFS(color), F_COLOR},
    {"intensity", STOFS(intensity), F_INT},
    {"width_angle", STOFS(width_angle), F_FLOAT},
    {"falloff_angle", STOFS(falloff_angle), F_FLOAT},

    {NULL}
};

static edict_t *ED_ChangeType(edict_t *ent, entity_type_t type)
{
    edict_t *n = G_SpawnType(type);
    memcpy(n, ent, sizeof(edict_t));
    n->s.number = n - globals.entities;
    G_FreeEdict(ent);
    return n;
}

/*
===============
ED_CallSpawn

Finds the spawn function for the entity and calls it.
Might change entity ptr if the entity type is different.
===============
*/
edict_t *ED_CallSpawn(edict_t *ent)
{
    const spawn_func_t *s;
    gitem_t *item;
    int     i;

    if (!ent->classname) {
        Com_WPrint("ED_CallSpawn: NULL classname\n");
        G_FreeEdict(ent);
        return ent;
    }

    // check item spawn functions
    for (i = 1; i < ITEM_TOTAL; i++, item++) {
        item = GetItemByIndex(i);
        if (!item->classname)
            continue;
        if (!strcmp(item->classname, ent->classname)) {
            // found it
            ent->classname = item->classname;
            SpawnItem(ent, item);
            return ent;
        }
    }

    // check normal spawn functions
    for (s = spawn_funcs ; s->name ; s++) {
        if (!strcmp(s->name, ent->classname)) {
            // found it
            // are we changing entity types?
            if ((s->type == ENT_PACKET && !Ent_IsPacket(ent->s.number)) ||
                (s->type == ENT_AMBIENT && !Ent_IsAmbient(ent->s.number)) ||
                (s->type == ENT_PRIVATE && !Ent_IsPrivate(ent->s.number))) {
                ent = ED_ChangeType(ent, s->type);
            }
            ent->classname = s->name;
            s->spawn(ent);
            return ent;
        }
    }

    Com_WPrintf("%s doesn't have a spawn function\n", ent->classname);
    G_FreeEdict(ent);
    return ent;
}

/*
=============
ED_NewString
=============
*/
static char *ED_NewString(const char *string)
{
    char    *newb, *new_p;
    int     i, l;

    l = strlen(string) + 1;

    newb = Z_TagMallocz(l, TAG_LEVEL);

    new_p = newb;

    for (i = 0 ; i < l ; i++) {
        if (string[i] == '\\' && i < l - 1) {
            i++;
            if (string[i] == 'n')
                *new_p++ = '\n';
            else
                *new_p++ = '\\';
        } else
            *new_p++ = string[i];
    }

    return newb;
}




/*
===============
ED_ParseField

Takes a key/value pair and sets the binary values
in an edict
===============
*/
static bool ED_ParseField(const spawn_field_t *fields, const char *key, const char *value, byte *b)
{
    const spawn_field_t *f;
    float   v;
    vec3_t  vec;

    for (f = fields ; f->name ; f++) {
        if (!Q_stricmp(f->name, key)) {
            // found it
            switch (f->type) {
            case F_LSTRING:
                *(char **)(b + f->ofs) = ED_NewString(value);
                break;
            case F_VECTOR:
            case F_COLOR:
                if (sscanf(value, "%f %f %f", &vec[0], &vec[1], &vec[2]) != 3) {
                    Com_WPrintf("%s: couldn't parse '%s'\n", __func__, key);
                    VectorClear(vec);
                }

                // expand float colors to bytes
                if (f->type == F_COLOR && vec[0] <= 1.f && vec[1] <= 1.f && vec[2] <= 1.f) {
                    vec[0] *= 255.f;
                    vec[1] *= 255.f;
                    vec[2] *= 255.f;
                }

                ((float *)(b + f->ofs))[0] = vec[0];
                ((float *)(b + f->ofs))[1] = vec[1];
                ((float *)(b + f->ofs))[2] = vec[2];
                break;
            case F_INT:
                *(int *)(b + f->ofs) = atoi(value);
                break;
            case F_FLOAT:
                *(float *)(b + f->ofs) = atof(value);
                break;
            case F_ANGLEHACK:
                v = atof(value);
                ((float *)(b + f->ofs))[0] = 0;
                ((float *)(b + f->ofs))[1] = v;
                ((float *)(b + f->ofs))[2] = 0;
                break;
            case F_IGNORE:
                break;
            default:
                break;
            }
            return true;
        }
    }
    return false;
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
====================
*/
void ED_ParseEdict(const char **data, edict_t *ent)
{
    bool        init;
    char        *key, *value;

    init = false;
    memset(&st, 0, sizeof(st));
    st.skyautorotate = 1;

// go through all the dictionary pairs
    while (1) {
        // parse key
        key = COM_Parse(data);
        if (key[0] == '}')
            break;
        if (!*data)
            Com_Errorf(ERR_DROP, "%s: EOF without closing brace", __func__);

        // parse value
        value = COM_Parse(data);
        if (!*data)
            Com_Errorf(ERR_DROP, "%s: EOF without closing brace", __func__);

        if (value[0] == '}')
            Com_Errorf(ERR_DROP, "%s: closing brace without data", __func__);

        init = true;

        // keynames with a leading underscore are used for utility comments,
        // and are immediately discarded by quake
        if (key[0] == '_')
            continue;

        if (!ED_ParseField(spawn_fields, key, value, (byte *)ent)) {
            if (!ED_ParseField(temp_fields, key, value, (byte *)&st)) {
                Com_WPrintf("%s: %s is not a field\n", __func__, key);
            }
        }
    }

    if (!init)
        memset(ent, 0, sizeof(*ent));
}


/*
================
G_FindTeams

Chain together all entities with a matching team field.

All but the first will have the FL_TEAMSLAVE flag set.
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams(void)
{
    int c = 0;
    int c2 = 0;

    for (edict_t *e = globals.entities + game.maxclients + BODY_QUEUE_SIZE + 1; e; e = G_NextEnt(e)) { 
        if (!e->team)
            continue;
        if (e->flags & FL_TEAMSLAVE)
            continue;
        edict_t *chain = e;
        e->teammaster = e;
        c++;
        c2++;
        for (edict_t *e2 = G_NextEnt(e); e2; e2 = G_NextEnt(e2)) {
            if (!e2->inuse)
                continue;
            if (!e2->team)
                continue;
            if (e2->flags & FL_TEAMSLAVE)
                continue;
            if (!strcmp(e->team, e2->team)) {
                c2++;
                chain->teamchain = e2;
                e2->teammaster = e;
                chain = e2;
                e2->flags |= FL_TEAMSLAVE;
            }
        }
    }

    Com_Printf("%i teams with %i entities\n", c, c2);
}

/*
==============
SpawnEntities

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.
==============
*/
void SpawnEntities(const char *mapname, const char *entities, const char *spawnpoint)
{
    edict_t     *ent;
    int         inhibit;
    char        *com_token;
    int         i;

    int skill_level = skill.integer;
    clamp(skill_level, 0, 3);
    if (skill.integer != skill_level)
        Cvar_Set(&skill, skill_level, true);

    SaveClientData();

    Z_FreeTags(TAG_LEVEL);

    memset(&level, 0, sizeof(level));
    G_InitEntityList(globals.entities);

    Q_strlcpy(level.mapname, mapname, sizeof(level.mapname));
    Q_strlcpy(game.spawnpoint, spawnpoint, sizeof(game.spawnpoint));

    // set client fields on player ents
    for (i = 0 ; i < game.maxclients ; i++)
        globals.entities[i + 1].client = game.clients + i;

    ent = NULL;
    inhibit = 0;

// parse ents
    while (1) {
        // parse the opening brace
        com_token = COM_Parse(&entities);
        if (!entities)
            break;
        if (com_token[0] != '{')
            Com_Errorf(ERR_DROP, "ED_LoadFromFile: found %s when expecting {", com_token);

        if (!ent)
            ent = game.world;
        else
            ent = G_Spawn();

        ED_ParseEdict(&entities, ent);

        // yet another map hack
        if (!Q_stricmp(level.mapname, "command") && !Q_stricmp(ent->classname, "trigger_once") && !Q_stricmp(ent->model, "*27"))
            ent->spawnflags &= ~SPAWNFLAG_NOT_HARD;

        // remove things (except the world) from different skill levels or deathmatch
        if (ent != game.world) {
			if (nomonsters.integer && (strstr(ent->classname, "monster") || strstr(ent->classname, "misc_deadsoldier") || strstr(ent->classname, "misc_insane"))) {
				G_FreeEdict(ent);
				inhibit++;
				continue;
			}
            if (deathmatch.integer) {
                if (ent->spawnflags & SPAWNFLAG_NOT_DEATHMATCH) {
                    G_FreeEdict(ent);
                    inhibit++;
                    continue;
                }
            } else {
                if ( /* ((coop->value) && (ent->spawnflags & SPAWNFLAG_NOT_COOP)) || */
                    ((skill.integer == 0) && (ent->spawnflags & SPAWNFLAG_NOT_EASY)) ||
                    ((skill.integer == 1) && (ent->spawnflags & SPAWNFLAG_NOT_MEDIUM)) ||
                    (((skill.integer == 2) || (skill.integer == 3)) && (ent->spawnflags & SPAWNFLAG_NOT_HARD))
                ) {
                    G_FreeEdict(ent);
                    inhibit++;
                    continue;
                }
            }

            ent->spawnflags &= ~(SPAWNFLAG_NOT_EASY | SPAWNFLAG_NOT_MEDIUM | SPAWNFLAG_NOT_HARD | SPAWNFLAG_NOT_COOP | SPAWNFLAG_NOT_DEATHMATCH);

            // Paril - generic animation support
            if (ent->anim.start || ent->anim.end) {
                G_InitAnimation(ent);
            }
        }

        ent = ED_CallSpawn(ent);
    }

    Com_Printf("%i entities inhibited\n", inhibit);

    G_FindTeams();

    PlayerTrail_Init();
}


//===================================================================

/*QUAKED worldspawn (0 0 0) ?

Only used for the world.
"sky"   environment map name
"skyaxis"   vector axis for rotating sky
"skyrotate" speed of rotation in degrees/second
"sounds"    music cd track number
"gravity"   800 is default gravity
"message"   text to print at user logon
*/
void SP_worldspawn(edict_t *ent)
{
    ent->movetype = MOVETYPE_PUSH;
    ent->solid = SOLID_BSP;
    ent->inuse = true;          // since the world doesn't use G_Spawn()
    ent->s.modelindex = 1;      // world model is always index 1

    //---------------

    // reserve some spots for dead player bodies for coop / deathmatch
    InitBodyQue();

    // set configstrings for items
    SetItemNames();

    if (st.nextmap)
        Q_strlcpy(level.nextmap, st.nextmap, sizeof(level.nextmap));

    // make some data visible to the server

    if (ent->message && ent->message[0]) {
        SV_SetConfigString(CS_NAME, ent->message);
        Q_strlcpy(level.level_name, ent->message, sizeof(level.level_name));
    } else
        Q_strlcpy(level.level_name, level.mapname, sizeof(level.level_name));

    if (st.sky && st.sky[0])
        SV_SetConfigString(CS_SKY, st.sky);
    else
        SV_SetConfigString(CS_SKY, "unit1_");

    SV_SetConfigString(CS_NUMITEMS, va("%i", ITEM_TOTAL));

    SV_SetConfigString(CS_SKYROTATE, va("%f", st.skyrotate));

    SV_SetConfigString(CS_SKYAXIS, va("%f %f %f",
                                   st.skyaxis[0], st.skyaxis[1], st.skyaxis[2]));

    SV_SetConfigString(CS_CDTRACK, va("%i", ent->sounds));

    SV_SetConfigString(CS_MAXCLIENTS, va("%i", game.maxclients));

    //---------------


    // help icon for statusbar
    SV_ImageIndex("i_help");
    level.pic_health = SV_ImageIndex("i_health");
    SV_ImageIndex("help");
    SV_ImageIndex("field_3");

    if (!st.gravity)
        Cvar_Set(&sv_gravity, 800, false);
    else
        Cvar_Set(&sv_gravity, st.gravity, false);

    level.default_reverb = st.default_reverb;

    // Level gravity change support
    level.gravity = sv_gravity.value;

    snd_fry = SV_SoundIndex(ASSET_SOUND_LAVA_FRYING);  // standing in lava / slime

    PrecacheItem(GetItemByIndex(ITEM_AXE));

    SV_SoundIndex(ASSET_SOUND_MONSTER_FRY1);
    SV_SoundIndex(ASSET_SOUND_MONSTER_FRY2);

    SV_SoundIndex("misc/pc_up.wav");
    SV_SoundIndex(ASSET_SOUND_GAME_MESSAGE);

    SV_SoundIndex(ASSET_SOUND_GIB);

    // sexed sounds
    SV_SoundIndex("*death1.wav");
    SV_SoundIndex("*death2.wav");
    SV_SoundIndex("*death3.wav");
    SV_SoundIndex("*death4.wav");
    SV_SoundIndex("*fall1.wav");
    SV_SoundIndex("*fall2.wav");
    SV_SoundIndex("*gurp1.wav");        // drowning damage
    SV_SoundIndex("*gurp2.wav");
    SV_SoundIndex("*jump1.wav");        // player jump
    SV_SoundIndex("*pain25_1.wav");
    SV_SoundIndex("*pain25_2.wav");
    SV_SoundIndex("*pain50_1.wav");
    SV_SoundIndex("*pain50_2.wav");
    SV_SoundIndex("*pain75_1.wav");
    SV_SoundIndex("*pain75_2.wav");
    SV_SoundIndex("*pain100_1.wav");
    SV_SoundIndex("*pain100_2.wav");

    // sexed models
    // THIS ORDER MUST MATCH THE DEFINES IN g_local.h
    // you can add more, max 21
    SV_ModelIndex("#w_blaster.md2");
    SV_ModelIndex("#w_shotgun.md2");
    SV_ModelIndex("#w_sshotgun.md2");
    SV_ModelIndex("#w_machinegun.md2");
    SV_ModelIndex("#w_chaingun.md2");
    SV_ModelIndex("#a_grenades.md2");
    SV_ModelIndex("#w_glauncher.md2");
    SV_ModelIndex("#w_rlauncher.md2");
    SV_ModelIndex("#w_hyperblaster.md2");
    SV_ModelIndex("#w_railgun.md2");
    SV_ModelIndex("#w_bfg.md2");

    //-------------------

    SV_SoundIndex("player/gasp1.wav");      // gasping for air
    SV_SoundIndex("player/gasp2.wav");      // head breaking surface, not gasping

    SV_SoundIndex(ASSET_SOUND_WATER_ENTER);    // feet hitting water
    SV_SoundIndex(ASSET_SOUND_WATER_EXIT);   // feet leaving water

    SV_SoundIndex(ASSET_SOUND_WATER_UNDER);    // head going underwater

    SV_SoundIndex(ASSET_SOUND_MONSTER_LAND);        // landing thud
    SV_SoundIndex(ASSET_SOUND_WATER_IMPACT);      // landing splash

    SV_SoundIndex(ASSET_SOUND_QUAD_ACTIVATE);
    SV_SoundIndex(ASSET_SOUND_PENT_ACTIVATE);
    SV_SoundIndex(ASSET_SOUND_PENT_HIT);
    SV_SoundIndex(ASSET_SOUND_OUT_OF_AMMO);

    sm_meat_index = SV_ModelIndex(ASSET_MODEL_GIB_MEAT);
    SV_ModelIndex(ASSET_MODEL_GIB_BONE);
    SV_ModelIndex(ASSET_MODEL_GIB_CHEST);
    SV_ModelIndex(ASSET_MODEL_GIB_SKULL);
    SV_ModelIndex(ASSET_MODEL_GIB_HEAD);

//
// Setup light animation tables. 'a' is total darkness, 'z' is doublebright.
//

    // 0 normal
    SV_SetConfigString(CS_LIGHTS + 0, "m");

    // 1 FLICKER (first variety)
    SV_SetConfigString(CS_LIGHTS + 1, "kkokkrkkrkkorokkrovokkr");

    // 2 SLOW STRONG PULSE
    SV_SetConfigString(CS_LIGHTS + 2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");

    // 3 CANDLE (first variety)
    SV_SetConfigString(CS_LIGHTS + 3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");

    // 4 FAST STROBE
    SV_SetConfigString(CS_LIGHTS + 4, "mamamamamama");

    // 5 GENTLE PULSE 1
    SV_SetConfigString(CS_LIGHTS + 5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj");

    // 6 FLICKER (second variety)
    SV_SetConfigString(CS_LIGHTS + 6, "okrovokrkokrkrkor");

    // 7 CANDLE (second variety)
    SV_SetConfigString(CS_LIGHTS + 7, "mmmaaaabcdefgmmmmaaaammmaamm");

    // 8 CANDLE (third variety)
    SV_SetConfigString(CS_LIGHTS + 8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");

    // 9 SLOW STROBE (fourth variety)
    SV_SetConfigString(CS_LIGHTS + 9, "aaaaaaaazzzzzzzz");

    // 10 FLUORESCENT FLICKER
    SV_SetConfigString(CS_LIGHTS + 10, "mmamammmmammamamaaamammma");

    // 11 SLOW PULSE NOT FADE TO BLACK
    SV_SetConfigString(CS_LIGHTS + 11, "abcdefghijklmnopqrrqponmlkjihgfedcba");

    // styles 32-62 are assigned by the light program for switchable lights

    // 63 testing
    SV_SetConfigString(CS_LIGHTS + 63, "a");
}

