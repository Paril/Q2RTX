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
// g_utils.c -- misc utility functions for game module

#include "g_local.h"


void G_ProjectSource(const vec3_t point, const vec3_t distance, const vec3_t forward, const vec3_t right, vec3_t result)
{
    result[0] = point[0] + forward[0] * distance[0] + right[0] * distance[1];
    result[1] = point[1] + forward[1] * distance[0] + right[1] * distance[1];
    result[2] = point[2] + forward[2] * distance[0] + right[2] * distance[1] + distance[2];
}

// Paril: multi-target support
#define MULTI_TARGET_CHAR   ','

// Fetch the next inuse entity from the appropriate
// parts of the entity list. returns null when we have
// nothing left to consume.
edict_t *G_NextEnt(edict_t *from)
{
    if (!from)
        return globals.entities; // world is always valid

    from++;

    while (true)
    {
        int n = from - globals.entities;

        if (n >= MAX_EDICTS)
            return NULL; // nothing left

        // check if we've stepped into an unallocated range
        if (n < OFFSET_AMBIENT_ENTITIES && n >= globals.num_entities[ENT_PACKET])
        {
            from = globals.entities + OFFSET_AMBIENT_ENTITIES; // start from ambient entities
            n = from - globals.entities;
        }
        
        if (n < OFFSET_PRIVATE_ENTITIES && n >= OFFSET_AMBIENT_ENTITIES + globals.num_entities[ENT_AMBIENT])
        {
            from = globals.entities + OFFSET_PRIVATE_ENTITIES; // start from private entities
            n = from - globals.entities;
        }

        if (n >= OFFSET_PRIVATE_ENTITIES + globals.num_entities[ENT_PRIVATE])
            return NULL; // end of private entities

        // if we're in-use, good to go
        if (from->inuse)
            return from;

        // check next entity
        from++;
    }
}

/*
=============
G_Find

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the edict after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.

=============
*/
edict_t *G_Find(edict_t *from, int fieldofs, char *match)
{
    for (from = G_NextEnt(from); from; from = G_NextEnt(from))
    {
        const char *s = *(char **)((byte *)from + fieldofs);
        
        if (!s)
            continue;

        // Paril: multi-target support
        const char *start = match, *end = strchr(match, MULTI_TARGET_CHAR);

        while (true)
        {
            if (!start || !*start)
                break;

            if (end)
            {
                if (!Q_strncasecmp(s, start, end - start))
                    return from;
            }
            else
            {
                if (!Q_strcasecmp(s, start))
                    return from;
            }

            start = ((end && *end) ? end + 1 : end);

            if (start)
                end = strchr(start, MULTI_TARGET_CHAR);
            else
                end = NULL;
        }
    }

    return NULL;
}


/*
=================
findradius

Returns entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
edict_t *findradius(edict_t *from, vec3_t org, float rad)
{
    vec3_t eorg;

    while (from = G_NextEnt(from))
    {
        if (from->solid == SOLID_NOT)
            continue;
        for (int j = 0 ; j < 3 ; j++)
            eorg[j] = org[j] - (from->s.origin[j] + (from->mins[j] + from->maxs[j]) * 0.5f);
        if (VectorLength(eorg) > rad)
            continue;
        return from;
    }

    return NULL;
}


/*
=============
G_PickTarget

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the edict after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.

=============
*/
#define MAXCHOICES  8

edict_t *G_PickTarget(char *targetname)
{
    edict_t *ent = NULL;
    int     num_choices = 0;
    edict_t *choice[MAXCHOICES];

    if (!targetname) {
        Com_WPrintf("G_PickTarget called with NULL targetname\n");
        return NULL;
    }

    while (1) {
        ent = G_Find(ent, FOFS(targetname), targetname);
        if (!ent)
            break;
        choice[num_choices++] = ent;
        if (num_choices == MAXCHOICES)
            break;
    }

    if (!num_choices) {
        Com_WPrintf("G_PickTarget: target %s not found\n", targetname);
        return NULL;
    }

    return choice[Q_rand_uniform(num_choices)];
}



void Think_Delay(edict_t *ent)
{
    G_UseTargets(ent, ent->activator);
    G_FreeEdict(ent);
}

/*
==============================
G_UseTargets

the global "activator" should be set to the entity that initiated the firing.

If self.delay is set, a DelayedUse entity will be created that will actually
do the SUB_UseTargets after that many seconds have passed.

Centerprints any self.message to the activator.

Search for (string)targetname in all entities that
match (string)self.target and call their .use function

==============================
*/
void G_UseTargets(edict_t *ent, edict_t *activator)
{
    edict_t     *t;

//
// check for a delay
//
    if (ent->delay) {
        // create a temp object to fire at a later time
        t = G_Spawn();
        t->classname = "DelayedUse";
        t->nextthink = level.time + G_SecToMs(ent->delay);
        t->think = Think_Delay;
        t->activator = activator;
        if (!activator)
            Com_WPrintf("Think_Delay with no activator\n");
        t->message = ent->message;
        t->target = ent->target;
        t->killtarget = ent->killtarget;
        t->anim.target = ent->anim.target;
        return;
    }


//
// print the message
//
    if ((ent->message) && !(activator->svflags & SVF_MONSTER)) {
        SV_CenterPrint(activator, ent->message);
        if (ent->noise_index >= 0)
            SV_StartSound(activator, CHAN_AUTO, ent->noise_index, 1, ATTN_NORM, 0);
        else
            SV_StartSound(activator, CHAN_AUTO, SV_SoundIndex(ASSET_SOUND_GAME_MESSAGE), 1, ATTN_NORM, 0);
    }

//
// kill killtargets
//
    if (ent->killtarget) {
        t = NULL;
        while ((t = G_Find(t, FOFS(targetname), ent->killtarget))) {
            G_FreeEdict(t);
            if (!ent->inuse) {
                Com_WPrint("entity was removed while using killtargets\n");
                return;
            }
        }
    }

//
// fire targets
//
    if (ent->target) {
        t = NULL;
        while ((t = G_Find(t, FOFS(targetname), ent->target))) {
            // doors fire area portals in a specific way
            if (!Q_stricmp(t->classname, "func_areaportal") &&
                (!Q_stricmp(ent->classname, "func_door") || !Q_stricmp(ent->classname, "func_door_rotating")))
                continue;

            if (t == ent) {
                Com_WPrint("WARNING: Entity used itself.\n");
            } else {
                if (t->use)
                    t->use(t, ent, activator);
            }
            if (!ent->inuse) {
                Com_WPrint("entity was removed while using targets\n");
                return;
            }
        }
    }

    // Paril - entity animation
    if (ent->anim.target) {
        t = NULL;
        while ((t = G_Find(t, FOFS(targetname), ent->anim.target))) {
            if (t->anim.count && !t->anim.count_left && !t->anim.reset_on_trigger) {
                continue;
            }

            t->activator = activator;
          
            if (t->anim.reset_on_trigger) {
                t->s.frame = t->anim.start;

                if (!t->anim.count_left) {
                    t->anim.count_left = t->anim.count;
                }

                t->anim.animating = true;
            } else {
                t->anim.animating = !t->anim.animating;
            }

            t->anim.next_frame++;
        }
    }
}


/*
=============
TempVector

This is just a convenience function
for making temporary vectors for function calls
=============
*/
float   *tv(float x, float y, float z)
{
    static  int     index;
    static  vec3_t  vecs[8];
    float   *v;

    // use an array so that multiple tempvectors won't collide
    // for a while
    v = vecs[index];
    index = (index + 1) & 7;

    v[0] = x;
    v[1] = y;
    v[2] = z;

    return v;
}


vec3_t VEC_UP       = {0, -1, 0};
vec3_t MOVEDIR_UP   = {0, 0, 1};
vec3_t VEC_DOWN     = {0, -2, 0};
vec3_t MOVEDIR_DOWN = {0, 0, -1};

void G_SetMovedir(vec3_t angles, vec3_t movedir)
{
    if (VectorCompare(angles, VEC_UP)) {
        VectorCopy(MOVEDIR_UP, movedir);
    } else if (VectorCompare(angles, VEC_DOWN)) {
        VectorCopy(MOVEDIR_DOWN, movedir);
    } else {
        AngleVectors(angles, movedir, NULL, NULL);
    }

    VectorClear(angles);
}


float vectoyaw(vec3_t vec)
{
    float   yaw;

    if (/*vec[YAW] == 0 &&*/ vec[PITCH] == 0) {
        yaw = 0;
        if (vec[YAW] > 0)
            yaw = 90;
        else if (vec[YAW] < 0)
            yaw = -90;
    } else {
        yaw = (int)RAD2DEG(atan2(vec[YAW], vec[PITCH]));
        if (yaw < 0)
            yaw += 360;
    }

    return yaw;
}


void vectoangles(vec3_t value1, vec3_t angles)
{
    float   forward;
    float   yaw, pitch;

    if (value1[1] == 0 && value1[0] == 0) {
        yaw = 0;
        if (value1[2] > 0)
            pitch = 90;
        else
            pitch = 270;
    } else {
        if (value1[0])
            yaw = (int)RAD2DEG(atan2(value1[1], value1[0]));
        else if (value1[1] > 0)
            yaw = 90;
        else
            yaw = -90;
        if (yaw < 0)
            yaw += 360;

        forward = sqrtf(value1[0] * value1[0] + value1[1] * value1[1]);
        pitch = (int)RAD2DEG(atan2(value1[2], forward));
        if (pitch < 0)
            pitch += 360;
    }

    angles[PITCH] = -pitch;
    angles[YAW] = yaw;
    angles[ROLL] = 0;
}

void G_InitEdict(edict_t *e)
{
    e->inuse = true;
    e->classname = "noclass";
    e->gravity = 1.0f;
    e->s.number = e - globals.entities;
}

/*
=================
G_Spawn

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
edict_t *G_SpawnType(entity_type_t type)
{
    int         i;
    int         start, end;

    switch (type)
    {
    case ENT_PACKET:
        i = game.maxclients + 1;
        start = OFFSET_PACKET_ENTITIES;
        end = OFFSET_AMBIENT_ENTITIES;
        break;
    case ENT_AMBIENT:
        i = start = OFFSET_AMBIENT_ENTITIES;
        end = OFFSET_PRIVATE_ENTITIES;
        break;
    case ENT_PRIVATE:
        i = start = OFFSET_PRIVATE_ENTITIES;
        end = MAX_EDICTS;
        break;
    default:
        Com_Error(ERR_DROP, "G_SpawnType: bad type");
    }

    edict_t *e;

    for (e = &globals.entities[i]; i < start + globals.num_entities[type]; i++, e++) {
        // the first couple seconds of server time can involve a lot of
        // freeing and allocating, so relax the replacement policy
        if (!e->inuse && (e->free_time < 2000 || level.time - e->free_time > 500)) {
            G_InitEdict(e);
            return e;
        }
    }

    if (i == end)
        Com_Error(ERR_DROP, "ED_Alloc: no free edicts");

    globals.num_entities[type]++;
    G_InitEdict(e);
    return e;
}

/*
=================
G_Spawn

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
edict_t *G_Spawn(void) { return G_SpawnType(ENT_PACKET); }

/*
=================
G_FreeEdict

Marks the edict as free
=================
*/
void G_FreeEdict(edict_t *ed)
{
    SV_UnlinkEntity(ed);        // unlink from world

    if ((ed - globals.entities) <= (game.maxclients + BODY_QUEUE_SIZE)) {
        Com_WPrint("tried to free special edict\n");
        return;
    }

    memset(ed, 0, sizeof(*ed));
    ed->classname = "freed";
    ed->free_time = level.time;
    ed->inuse = false;
    ed->s.number = ed - globals.entities;
}


/*
============
G_TouchTriggers

============
*/
void G_TouchTriggers(edict_t *ent)
{
    int         i, num;
    edict_t     *touch[MAXTOUCH * 4], *hit;

    // dead things don't activate triggers!
    if ((ent->client || (ent->svflags & SVF_MONSTER)) && (ent->health <= 0))
        return;

    num = SV_AreaEdicts(ent->absmin, ent->absmax, touch
                        , q_countof(touch), AREA_TRIGGERS);

    if (!num)
        return;

    vec3_t mins, maxs;
    VectorAdd(ent->s.origin, ent->mins, mins);
    VectorAdd(ent->s.origin, ent->maxs, maxs);

    // be careful, it is possible to have an entity in this
    // list removed before we get to it (killtriggered)
    for (i = 0 ; i < num ; i++) {
        hit = touch[i];
        if (!hit->inuse)
            continue;
        if (!hit->touch)
            continue;

        // check full collision
        if (!SV_EntityCollide(mins, maxs, hit))
            continue;

        hit->touch(hit, ent, NULL, NULL);
    }
}

/*
==============================================================================

Kill box

==============================================================================
*/

/*
=================
KillBox

Kills all entities that would touch the proposed new positioning
of ent.  Ent should be unlinked before calling this!
=================
*/
bool KillBox(edict_t *ent)
{
    trace_t     tr;

    while (1) {
        tr = SV_Trace(ent->s.origin, ent->mins, ent->maxs, ent->s.origin, NULL, MASK_PLAYERSOLID);
        if (!tr.ent || tr.ent == globals.entities)
            break;

        // nail it
        T_Damage(tr.ent, ent, ent, vec3_origin, ent->s.origin, vec3_origin, 100000, 0, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);

        // if we didn't kill it, fail
        if (tr.ent->solid)
            return false;
    }

    return true;        // all clear
}
