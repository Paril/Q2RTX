/*
Copyright (C) 1997-2001 Id Software, Inc.

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
#include "g_ptrs.h"

typedef struct {
    fieldtype_t type;
#if USE_DEBUG
    char *name;
#endif
    unsigned ofs;
    unsigned size;
} save_field_t;

#if USE_DEBUG
#define _FA(type, name, size) { type, #name, _OFS(name), size }
#else
#define _FA(type, name, size) { type, _OFS(name), size }
#endif
#define _F(type, name) _FA(type, name, 1)
#define SZ(name, size) _FA(F_ZSTRING, name, size)
#define BA(name, size) _FA(F_BYTE, name, size)
#define B(name) BA(name, 1)
#define SA(name, size) _FA(F_SHORT, name, size)
#define S(name) SA(name, 1)
#define IA(name, size) _FA(F_INT, name, size)
#define I(name) IA(name, 1)
#define OA(name, size) _FA(F_BOOL, name, size)
#define O(name) OA(name, 1)
#define FA(name, size) _FA(F_FLOAT, name, size)
#define F(name) FA(name, 1)
#define L(name) _F(F_LSTRING, name)
#define V(name) _F(F_VECTOR, name)
#define T(name) _F(F_ITEM, name)
#define TI(name) _F(F_ITEM_ID, name)
#define E(name) _F(F_EDICT, name)
#define P(name, type) _FA(F_POINTER, name, type)
#define I64A(name, size) _FA(F_INT64, name, size)
#define I64(name) IA(name, 1)

static const save_field_t entityfields[] = {
#define _OFS FOFS
    V(s.origin),
    V(s.angles),
    V(s.old_origin),
    I(s.modelindex),
    I(s.modelindex2),
    I(s.modelindex3),
    I(s.modelindex4),
    I(s.frame),
    I(s.skinnum),
    I(s.effects),
    I(s.renderfx),
    I(s.bbox),
    I(s.sound),
    I(s.event),
    I(s.sound_pitch),

    // [...]

    I(svflags),
    V(mins),
    V(maxs),
    V(absmin),
    V(absmax),
    V(size),
    I(solid),
    I(clipmask),
    E(owner),

    I(movetype),
    I(flags),
    
    L(model),
    L(model2),
    L(model3),
    L(model4),
    I64(free_time),

    L(message),
    L(classname),
    I(spawnflags),

    I64(timestamp),

    L(target),
    L(targetname),
    L(killtarget),
    L(team),
    L(pathtarget),
    L(deathtarget),
    L(combattarget),
    E(target_ent),

    F(speed),
    F(accel),
    F(decel),
    V(movedir),
    V(pos1),
    V(pos2),

    V(velocity),
    V(avelocity),
    I(mass),
    I64(air_finished_time),
    F(gravity),

    E(goalentity),
    E(movetarget),
    F(yaw_speed),
    F(ideal_yaw),

    I64(nextthink),
    P(prethink, P_prethink),
    P(think, P_think),
    P(blocked, P_blocked),
    P(touch, P_touch),
    P(use, P_use),
    P(pain, P_pain),
    P(die, P_die),

    I64(touch_debounce_time),
    I64(pain_debounce_time),
    I64(damage_debounce_time),
    I64(fly_sound_debounce_time),
    I64(last_move_time),

    I(health),
    I(max_health),
    I(gib_health),
    I(deadflag),
    I64(show_hostile_time),

    L(map),

    I(viewheight),
    I(takedamage),
    I(dmg),
    I(radius_dmg),
    F(dmg_radius),
    I(sounds),
    I(count),

    E(chain),
    E(enemy),
    E(oldenemy),
    E(activator),
    E(groundentity),
    I(groundentity_linkcount),
    E(teamchain),
    E(teammaster),

    E(mynoise),
    E(mynoise2),

    I(noise_index),
    I(noise_index2),
    F(volume),
    F(attenuation),

    F(wait),
    F(delay),
    F(random),

    I64(last_sound_time),

    I(watertype),
    I(waterlevel),

    V(move_origin),
    V(move_angles),

    I(light_level),

    I(style),

    T(item),

    V(moveinfo.start_origin),
    V(moveinfo.start_angles),
    V(moveinfo.end_origin),
    V(moveinfo.end_angles),

    I(moveinfo.sound_start),
    I(moveinfo.sound_middle),
    I(moveinfo.sound_end),

    F(moveinfo.accel),
    F(moveinfo.speed),
    F(moveinfo.decel),
    F(moveinfo.distance),

    F(moveinfo.wait),

    I(moveinfo.state),
    V(moveinfo.dir),
    F(moveinfo.current_speed),
    F(moveinfo.move_speed),
    F(moveinfo.next_speed),
    F(moveinfo.remaining_distance),
    F(moveinfo.decel_distance),
    P(moveinfo.endfunc, P_moveinfo_endfunc),

    P(monsterinfo.currentmove, P_monsterinfo_currentmove),
    I(monsterinfo.aiflags),
    I(monsterinfo.nextframe),
    F(monsterinfo.scale),

    P(monsterinfo.stand, P_monsterinfo_stand),
    P(monsterinfo.idle, P_monsterinfo_idle),
    P(monsterinfo.search, P_monsterinfo_search),
    P(monsterinfo.walk, P_monsterinfo_walk),
    P(monsterinfo.run, P_monsterinfo_run),
    P(monsterinfo.dodge, P_monsterinfo_dodge),
    P(monsterinfo.attack, P_monsterinfo_attack),
    P(monsterinfo.melee, P_monsterinfo_melee),
    P(monsterinfo.sight, P_monsterinfo_sight),
    P(monsterinfo.checkattack, P_monsterinfo_checkattack),
    P(monsterinfo.load, P_monsterinfo_load),

    I64(monsterinfo.pause_time),
    I64(monsterinfo.attack_finished_time),

    V(monsterinfo.saved_goal),
    I64(monsterinfo.search_time),
    I64(monsterinfo.trail_time),
    V(monsterinfo.last_sighting),
    I(monsterinfo.attack_state),
    I(monsterinfo.lefty),
    I64(monsterinfo.idle_time),
    I(monsterinfo.linkcount),

    // Paril - entity animation stuff
    L(anim.target),
    I(anim.start),
    I(anim.end),
    I(anim.frame_delay),
    B(anim.animating),
    B(anim.reset_on_trigger),
    B(anim.is_active),
    I(anim.next_frame),
    I(anim.count),
    I(anim.count_left),

    I(radius),
    E(next_reverb),

    {0}
#undef _OFS
};

static const save_field_t levelfields[] = {
#define _OFS LLOFS
    I64(time),

    SZ(level_name, MAX_QPATH),
    SZ(mapname, MAX_QPATH),
    SZ(nextmap, MAX_QPATH),

    I64(intermission_time),
    L(changemap),
    I(exitintermission),
    V(intermission_origin),
    V(intermission_angle),

    E(sight_client),

    E(sight_entity),
    I64(sight_entity_time),
    E(sound_entity),
    I64(sound_entity_time),
    E(sound2_entity),
    I64(sound2_entity_time),

    I(pic_health),

    I(total_secrets),
    I(found_secrets),

    I(total_goals),
    I(found_goals),

    I(total_monsters),
    I(killed_monsters),

    I(body_que),

    F(gravity),
    I(default_reverb),
    E(reverb_entities),

    {0}
#undef _OFS
};

static const save_field_t clientfields[] = {
#define _OFS CLOFS
    I(ps.pmove.pm_type),

    SA(ps.pmove.origin, 3),
    V(ps.pmove.velocity),
    B(ps.pmove.pm_flags),
    B(ps.pmove.pm_time),
    F(ps.pmove.gravity),
    V(ps.pmove.delta_angles),

    V(ps.viewangles),
    V(ps.viewoffset),
    V(ps.kick_angles),
    
    I(ps.gun[0].index),
    I(ps.gun[0].frame),
    F(ps.gun[0].spin),

    I(ps.gun[1].index),
    I(ps.gun[1].frame),
    F(ps.gun[1].spin),

    FA(ps.blend, 4),

    F(ps.fov),

    I(ps.rdflags),
    I(ps.reverb),

    SA(ps.stats, MAX_STATS),

    SZ(pers.userinfo, MAX_INFO_STRING),
    SZ(pers.netname, 16),
    I(pers.hand),

    O(pers.connected),

    I(pers.health),
    I(pers.max_health),
    I(pers.savedFlags),

    TI(pers.selected_item),
    IA(pers.inventory, MAX_ITEMS),

    I(pers.max_nails),
    I(pers.max_shells),
    I(pers.max_rockets),
    I(pers.max_cells),

    T(pers.weapon),
    T(pers.lastweapon),

    I(pers.score),

    I(pers.game_helpchanged),
    I(pers.helpchanged),

    O(pers.spectator),

    O(showscores),
    O(showinventory),
    O(showhelp),
    O(showhelpicon),

    TI(ammo_index),

    T(newweapon),

    I(damage_armor),
    I(damage_blood),
    I(damage_knockback),
    V(damage_from),

    F(killer_yaw),

    V(kick_angles),
    F(v_dmg_roll),
    F(v_dmg_pitch),
    I64(v_dmg_time),
    I64(fall_time),
    F(fall_value),
    F(damage_alpha),
    F(bonus_alpha),
    V(damage_blend),
    V(v_angle),
    F(bobtime),
    V(oldviewangles),
    V(oldvelocity),

    I64(next_drown_time),
    I(old_waterlevel),
    I(breather_sound),

    I(anim_end),
    I(anim_priority),
    O(anim_duck),
    O(anim_run),

    // powerup timers
    I64(quad_time),
    I64(invincible_time),
    I64(enviro_time),

    I(weapon_sound),

    I64(pickup_msg_time),

    {0}
#undef _OFS
};

static const save_field_t gamefields[] = {
#define _OFS GLOFS
    SZ(helpmessage1, 512),
    SZ(helpmessage2, 512),

    I(maxclients),

    I(serverflags),

    O(autosaved),

    {0}
#undef _OFS
};

//=========================================================

static void write_data(void *buf, size_t len, FILE *f)
{
    if (fwrite(buf, 1, len, f) != len) {
        fclose(f);
        Com_Errorf(ERR_DROP, "%s: couldn't write %zu bytes", __func__, len);
    }
}

static void write_short(FILE *f, short v)
{
    v = LittleShort(v);
    write_data(&v, sizeof(v), f);
}

static void write_int(FILE *f, int v)
{
    v = LittleLong(v);
    write_data(&v, sizeof(v), f);
}

static void write_int64(FILE *f, int64_t v)
{
    v = LongLongSwap(v);
    write_data(&v, sizeof(v), f);
}

static void write_float(FILE *f, float v)
{
    v = LittleFloat(v);
    write_data(&v, sizeof(v), f);
}

static void write_string(FILE *f, char *s)
{
    size_t len;

    if (!s) {
        write_int(f, -1);
        return;
    }

    len = strlen(s);
    write_int(f, len);
    write_data(s, len, f);
}

static void write_vector(FILE *f, vec_t *v)
{
    write_float(f, v[0]);
    write_float(f, v[1]);
    write_float(f, v[2]);
}

static void write_index(FILE *f, void *p, size_t size, void *start, int max_index)
{
    size_t diff;

    if (!p) {
        write_int(f, -1);
        return;
    }

    if (p < start || (byte *)p > (byte *)start + max_index * size) {
        fclose(f);
        Com_Errorf(ERR_DROP, "%s: pointer out of range: %p", __func__, p);
    }

    diff = (byte *)p - (byte *)start;
    if (diff % size) {
        fclose(f);
        Com_Errorf(ERR_DROP, "%s: misaligned pointer: %p", __func__, p);
    }
    write_int(f, (int)(diff / size));
}

static void write_pointer(FILE *f, void *p, ptr_type_t type)
{
    const save_ptr_t *ptr;
    int i;

    if (!p) {
        write_int(f, -1);
        return;
    }

    for (i = 0, ptr = save_ptrs; i < num_save_ptrs; i++, ptr++) {
        if (ptr->type == type && ptr->ptr == p) {
            write_int(f, i);
            return;
        }
    }

    fclose(f);
    Com_Errorf(ERR_DROP, "%s: unknown pointer: %p", __func__, p);
}

static void write_item(FILE *fp, const gitem_t *item)
{
    if (!item || item->id == ITEM_NULL || !item->classname) {
        write_int(fp, -1);
        return;
    }

    write_string(fp, item->classname);
}

static void write_field(FILE *f, const save_field_t *field, void *base)
{
    void *p = (byte *)base + field->ofs;
    int i;

    switch (field->type) {
    case F_BYTE:
        write_data(p, field->size, f);
        break;
    case F_SHORT:
        for (i = 0; i < field->size; i++) {
            write_short(f, ((short *)p)[i]);
        }
        break;
    case F_INT:
        for (i = 0; i < field->size; i++) {
            write_int(f, ((int *)p)[i]);
        }
        break;
    case F_BOOL:
        for (i = 0; i < field->size; i++) {
            write_int(f, ((bool *)p)[i]);
        }
        break;
    case F_FLOAT:
        for (i = 0; i < field->size; i++) {
            write_float(f, ((float *)p)[i]);
        }
        break;
    case F_VECTOR:
        write_vector(f, (vec_t *)p);
        break;

    case F_ZSTRING:
        write_string(f, (char *)p);
        break;
    case F_LSTRING:
        write_string(f, *(char **)p);
        break;

    case F_EDICT:
        write_index(f, *(void **)p, sizeof(edict_t), globals.entities, MAX_EDICTS - 1);
        break;
    case F_CLIENT:
        write_index(f, *(void **)p, sizeof(gclient_t), game.clients, game.maxclients - 1);
        break;
    case F_ITEM:
        write_item(f, *(gitem_t **)p);
        break;
    case F_ITEM_ID:
        write_item(f, GetItemByIndex(*(gitem_id_t *)p));
        break;

    case F_POINTER:
        write_pointer(f, *(void **)p, field->size);
        break;

    case F_INT64:
        for (i = 0; i < field->size; i++) {
            write_int64(f, ((int64_t *)p)[i]);
        }
        break;

    default:
        Com_Errorf(ERR_DROP, "%s: unknown field type", __func__);
    }
}

static void write_fields(FILE *f, const save_field_t *fields, void *base)
{
    const save_field_t *field;

    for (field = fields; field->type; field++) {
        write_field(f, field, base);
    }
}

static void read_data(void *buf, size_t len, FILE *f)
{
    if (fread(buf, 1, len, f) != len) {
        fclose(f);
        Com_Errorf(ERR_DROP, "%s: couldn't read %zu bytes", __func__, len);
    }
}

static int read_short(FILE *f)
{
    short v;

    read_data(&v, sizeof(v), f);
    v = LittleShort(v);

    return v;
}

static int read_int(FILE *f)
{
    int v;

    read_data(&v, sizeof(v), f);
    v = LittleLong(v);

    return v;
}

static int read_int64(FILE *f)
{
    int64_t v;

    read_data(&v, sizeof(v), f);
    v = LittleLongLong(v);

    return v;
}

static float read_float(FILE *f)
{
    float v;

    read_data(&v, sizeof(v), f);
    v = LittleFloat(v);

    return v;
}

static char *read_string(FILE *f)
{
    int len;
    char *s;

    len = read_int(f);
    if (len == -1) {
        return NULL;
    }

    if (len < 0 || len > 65536) {
        fclose(f);
        Com_Errorf(ERR_DROP, "%s: bad length", __func__);
    }

    s = Z_TagMalloc(len + 1, TAG_LEVEL);
    read_data(s, len, f);
    s[len] = 0;

    return s;
}

static void read_zstring(FILE *f, char *s, size_t size)
{
    int len;

    len = read_int(f);
    if (len < 0 || len >= size) {
        fclose(f);
        Com_Errorf(ERR_DROP, "%s: bad length", __func__);
    }

    read_data(s, len, f);
    s[len] = 0;
}

static gitem_t *read_item(FILE *f)
{
    int len = read_int(f);

    if (len == -1) {
        return NULL;
    }

    static char item_name[256];

    if (len >= q_countof(item_name) - 1) {
        Com_Errorf(ERR_DROP, "%s: bad length", __func__);
    }

    read_data(&item_name, len, f);
    item_name[len] = 0;

    return FindItemByClassname(item_name);
}

static void read_vector(FILE *f, vec_t *v)
{
    v[0] = read_float(f);
    v[1] = read_float(f);
    v[2] = read_float(f);
}

static void *read_index(FILE *f, size_t size, void *start, int max_index)
{
    int index;
    byte *p;

    index = read_int(f);
    if (index == -1) {
        return NULL;
    }

    if (index < 0 || index > max_index) {
        fclose(f);
        Com_Errorf(ERR_DROP, "%s: bad index", __func__);
    }

    p = (byte *)start + index * size;
    return p;
}

static void *read_pointer(FILE *f, ptr_type_t type)
{
    int index;
    const save_ptr_t *ptr;

    index = read_int(f);
    if (index == -1) {
        return NULL;
    }

    if (index < 0 || index >= num_save_ptrs) {
        fclose(f);
        Com_Errorf(ERR_DROP, "%s: bad index", __func__);
    }

    ptr = &save_ptrs[index];
    if (ptr->type != type) {
        fclose(f);
        Com_Errorf(ERR_DROP, "%s: type mismatch", __func__);
    }

    return ptr->ptr;
}

static void read_field(FILE *f, const save_field_t *field, void *base)
{
    void *p = (byte *)base + field->ofs;
    int i;

    switch (field->type) {
    case F_BYTE:
        read_data(p, field->size, f);
        break;
    case F_SHORT:
        for (i = 0; i < field->size; i++) {
            ((short *)p)[i] = read_short(f);
        }
        break;
    case F_INT:
        for (i = 0; i < field->size; i++) {
            ((int *)p)[i] = read_int(f);
        }
        break;
    case F_BOOL:
        for (i = 0; i < field->size; i++) {
            ((bool *)p)[i] = read_int(f);
        }
        break;
    case F_FLOAT:
        for (i = 0; i < field->size; i++) {
            ((float *)p)[i] = read_float(f);
        }
        break;
    case F_VECTOR:
        read_vector(f, (vec_t *)p);
        break;

    case F_LSTRING:
        *(char **)p = read_string(f);
        break;
    case F_ZSTRING:
        read_zstring(f, (char *)p, field->size);
        break;

    case F_EDICT:
        *(edict_t **)p = read_index(f, sizeof(edict_t), globals.entities, MAX_EDICTS - 1);
        break;
    case F_CLIENT:
        *(gclient_t **)p = read_index(f, sizeof(gclient_t), game.clients, game.maxclients - 1);
        break;
    case F_ITEM:
        *(gitem_t **)p = read_item(f);
        break;
    case F_ITEM_ID: {
        gitem_t *item = read_item(f);
        *(gitem_id_t *)p = item ? item->id : 0;
        break;
    }

    case F_POINTER:
        *(void **)p = read_pointer(f, field->size);
        break;

    case F_INT64:
        for (i = 0; i < field->size; i++) {
            ((int64_t *)p)[i] = read_int64(f);
        }
        break;

    default:
        Com_Errorf(ERR_DROP, "%s: unknown field type", __func__);
    }
}

static void read_fields(FILE *f, const save_field_t *fields, void *base)
{
    const save_field_t *field;

    for (field = fields; field->type; field++) {
        read_field(f, field, base);
    }
}

//=========================================================

#define SAVE_MAGIC1     MakeLittleLong('S','S','V','1')
#define SAVE_MAGIC2     MakeLittleLong('S','A','V','1')
#define SAVE_VERSION    8

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
void WriteGame(const char *filename, bool autosave)
{
    FILE    *f;
    int     i;

    if (!autosave)
        SaveClientData();

    f = fopen(filename, "wb");
    if (!f)
        Com_Errorf(ERR_DROP, "Couldn't open %s", filename);

    write_int(f, SAVE_MAGIC1);
    write_int(f, SAVE_VERSION);

    game.autosaved = autosave;
    write_fields(f, gamefields, &game);
    game.autosaved = false;

    for (i = 0; i < game.maxclients; i++) {
        write_fields(f, clientfields, &game.clients[i]);
    }

    if (fclose(f))
        Com_Errorf(ERR_DROP, "Couldn't write %s", filename);
}

void ReadGame(const char *filename)
{
    FILE    *f;
    int     i;

    Z_FreeTags(TAG_GAME);

    f = fopen(filename, "rb");
    if (!f)
        Com_Errorf(ERR_DROP, "Couldn't open %s", filename);

    i = read_int(f);
    if (i != SAVE_MAGIC1) {
        fclose(f);
        Com_Error(ERR_DROP, "Not a save game");
    }

    i = read_int(f);
    if (i != SAVE_VERSION) {
        fclose(f);
        Com_Errorf(ERR_DROP, "Savegame from different version (got %d, expected %d)", i, SAVE_VERSION);
    }

    read_fields(f, gamefields, &game);

    // should agree with server's version
    cvarRef_t maxclients;
    Cvar_Get(&maxclients, "maxclients", NULL, 0);

    if (game.maxclients != maxclients.integer) {
        fclose(f);
        Com_Error(ERR_DROP, "Savegame has bad maxclients");
    }

    globals.entities = G_CreateEntityList();
    game.world = globals.entities;
    globals.num_entities[ENT_PACKET] = game.maxclients + 1;
    globals.num_entities[ENT_AMBIENT] = globals.num_entities[ENT_PRIVATE] = 0;

    game.clients = Z_TagMallocz(game.maxclients * sizeof(game.clients[0]), TAG_GAME);
    for (i = 0; i < game.maxclients; i++) {
        read_fields(f, clientfields, &game.clients[i]);
        game.clients[i].weapanim[0] = game.clients[i].weapanim[1] = NULL;
    }

    fclose(f);
}

//==========================================================


/*
=================
WriteLevel

=================
*/
void WriteLevel(const char *filename)
{
    FILE    *f;

    f = fopen(filename, "wb");
    if (!f)
        Com_Errorf(ERR_DROP, "Couldn't open %s", filename);

    write_int(f, SAVE_MAGIC2);
    write_int(f, SAVE_VERSION);

    // write out level_locals_t
    write_fields(f, levelfields, &level);

    // write out all the entities
    for (edict_t *ent = globals.entities; ent; ent = G_NextEnt(ent)) {
        write_int(f, ent->s.number);
        write_fields(f, entityfields, ent);
    }
    write_int(f, -1);

    if (fclose(f))
        Com_Errorf(ERR_DROP, "Couldn't write %s", filename);
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
void ReadLevel(const char *filename)
{
    int     entnum;
    FILE    *f;
    int     i;
    edict_t *ent;

    // free any dynamic memory allocated by loading the level
    // base state
    Z_FreeTags(TAG_LEVEL);

    f = fopen(filename, "rb");
    if (!f)
        Com_Errorf(ERR_DROP, "Couldn't open %s", filename);

    // wipe all the entities
    G_InitEntityList(globals.entities);

    globals.num_entities[ENT_PACKET] = game.maxclients + 1;
    globals.num_entities[ENT_AMBIENT] = globals.num_entities[ENT_PRIVATE] = 0;

    i = read_int(f);
    if (i != SAVE_MAGIC2) {
        fclose(f);
        Com_Error(ERR_DROP, "Not a save game");
    }

    i = read_int(f);
    if (i != SAVE_VERSION) {
        fclose(f);
        Com_Errorf(ERR_DROP, "Savegame from different version (got %d, expected %d)", i, SAVE_VERSION);
    }

    // load the level locals
    read_fields(f, levelfields, &level);

    // load all the entities
    while (1) {
        entnum = read_int(f);
        if (entnum == -1)
            break;
        if (entnum < 0 || entnum >= MAX_EDICTS) {
            fclose(f);
            Com_Errorf(ERR_DROP, "%s: bad entity number", __func__);
        }

        int *num, start;

        if (Ent_IsPacket(entnum)) {
            start = OFFSET_PACKET_ENTITIES;
            num = &globals.num_entities[ENT_PACKET];
        } else if (Ent_IsAmbient(entnum)) {
            start = OFFSET_AMBIENT_ENTITIES;
            num = &globals.num_entities[ENT_AMBIENT];
        } else if (Ent_IsPrivate(entnum)) {
            start = OFFSET_PRIVATE_ENTITIES;
            num = &globals.num_entities[ENT_PRIVATE];
        } else {
            Com_Errorf(ERR_DROP, "Entity number out of range (%i)\n", entnum);
        }

        *num = max(*num, (entnum - start) + 1);

        ent = &globals.entities[entnum];
        read_fields(f, entityfields, ent);
        ent->inuse = true;
        ent->s.number = entnum;

        // let the server rebuild world links for this ent
        SV_LinkEntity(ent);
    }

    fclose(f);

    // mark all clients as unconnected
    for (i = 0 ; i < game.maxclients ; i++) {
        ent = &globals.entities[i + 1];
        ent->client = game.clients + i;
        ent->client->pers.connected = false;
        ent->client->weapanim[0] = ent->client->weapanim[1] = NULL;
    }

    // do any load time things at this point
    for (ent = globals.entities; ent; ent = G_NextEnt(ent)) {
        // fire any cross-level triggers
        if (ent->classname)
            if (strcmp(ent->classname, "target_crosslevel_target") == 0)
                ent->nextthink = level.time + G_SecToMs(ent->delay);

        if (ent->think == func_clock_think || ent->use == func_clock_use) {
            char *msg = ent->message;
            ent->message = Z_TagMalloc(CLOCK_MESSAGE_SIZE, TAG_LEVEL);
            if (msg) {
                Q_strlcpy(ent->message, msg, CLOCK_MESSAGE_SIZE);
                Z_Free(msg);
            }
        }

        // load monster-specific model stuff
        if (ent->monsterinfo.load)
            ent->monsterinfo.load(ent);
    }
}

