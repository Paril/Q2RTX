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
// g_weapon.c

#include "g_local.h"
#include "m_player.h"


static bool     is_quad;
static byte     is_silenced;


void G_ProjectSource2(const vec3_t point, const vec3_t distance, const vec3_t forward, const vec3_t right, const vec3_t up, vec3_t result)
{
    result[0] = point[0] + forward[0] * distance[0] + right[0] * distance[1] + up[0] * distance[2];
    result[1] = point[1] + forward[1] * distance[0] + right[1] * distance[1] + up[1] * distance[2];
    result[2] = point[2] + forward[2] * distance[0] + right[2] * distance[1] + up[2] * distance[2];
}

void P_ProjectSource(gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
    vec3_t  _distance;

    VectorCopy(distance, _distance);
    if (client->pers.hand == LEFT_HANDED)
        _distance[1] *= -1;
    else if (client->pers.hand == CENTER_HANDED)
        _distance[1] = 0;
    G_ProjectSource(point, _distance, forward, right, result);
}

void P_ProjectSource2(gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t up, vec3_t result)
{
    vec3_t  _distance;

    VectorCopy(distance, _distance);
    if (client->pers.hand == LEFT_HANDED)
        _distance[1] *= -1;
    else if (client->pers.hand == CENTER_HANDED)
        _distance[1] = 0;
    G_ProjectSource2(point, _distance, forward, right, up, result);
}


/*
===============
PlayerNoise

Each player can have two noise objects associated with it:
a personal noise (jumping, pain, weapon firing), and a weapon
target noise (bullet wall impacts)

Monsters that don't directly see the player can move
to a noise in hopes of seeing the player from there.
===============
*/
void PlayerNoise(edict_t *who, vec3_t where, int type)
{
    edict_t     *noise;

    if (type == PNOISE_WEAPON) {
        if (who->client->silencer_shots) {
            who->client->silencer_shots--;
            return;
        }
    }

    if (deathmatch.integer)
        return;

    if (who->flags & FL_NOTARGET)
        return;


    if (!who->mynoise) {
        noise = G_Spawn();
        noise->classname = "player_noise";
        VectorSet(noise->mins, -8, -8, -8);
        VectorSet(noise->maxs, 8, 8, 8);
        noise->owner = who;
        noise->svflags = SVF_NOCLIENT;
        who->mynoise = noise;

        noise = G_Spawn();
        noise->classname = "player_noise";
        VectorSet(noise->mins, -8, -8, -8);
        VectorSet(noise->maxs, 8, 8, 8);
        noise->owner = who;
        noise->svflags = SVF_NOCLIENT;
        who->mynoise2 = noise;
    }

    if (type == PNOISE_SELF || type == PNOISE_WEAPON) {
        noise = who->mynoise;
        level.sound_entity = noise;
        level.sound_entity_time = level.time;
    } else { // type == PNOISE_IMPACT
        noise = who->mynoise2;
        level.sound2_entity = noise;
        level.sound2_entity_time = level.time;
    }

    VectorCopy(where, noise->s.origin);
    VectorSubtract(where, noise->maxs, noise->absmin);
    VectorAdd(where, noise->maxs, noise->absmax);
    noise->last_sound_time = level.time;
    SV_LinkEntity(noise);
}


bool Pickup_Weapon(edict_t *ent, edict_t *other)
{
    int         index;
    gitem_t     *ammo;

    index = ITEM_INDEX(ent->item);

    if (((dmflags.integer & DF_WEAPONS_STAY) || coop.integer)
        && other->client->pers.inventory[index]) {
        if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
            return false;   // leave the weapon for others to pickup
    }

    if (!other->client->pers.inventory[index]) {
        other->client->inspect = true;
    }

    other->client->pers.inventory[index]++;

    if (!(ent->spawnflags & DROPPED_ITEM)) {
        // give them some ammo with it
        ammo = FindItem(ent->item->ammo);
        if (dmflags.integer & DF_INFINITE_AMMO)
            Add_Ammo(other, ammo, 1000);
        else
            Add_Ammo(other, ammo, ammo->quantity);

        if (!(ent->spawnflags & DROPPED_PLAYER_ITEM)) {
            if (deathmatch.integer) {
                if (dmflags.integer & DF_WEAPONS_STAY)
                    ent->flags |= FL_RESPAWN;
                else
                    SetRespawn(ent, 30);
            }
            if (coop.integer)
                ent->flags |= FL_RESPAWN;
        }
    }

    if (other->client->pers.weapon != ent->item &&
        (other->client->pers.inventory[index] == 1) &&
        (!deathmatch.integer || other->client->pers.weapon == FindItem("Axe")))
        other->client->newweapon = ent->item;

    return true;
}


/*
===============
ChangeWeapon

The old weapon has been dropped all the way, so make the new one
current
===============
*/
bool ChangeWeapon(edict_t *ent)
{
    int i;

    ent->client->pers.lastweapon = ent->client->pers.weapon;
    ent->client->pers.weapon = ent->client->newweapon;
    ent->client->newweapon = NULL;
    ent->client->machinegun_shots = 0;

    // set visible model
    if (ent->s.modelindex == 255) {
        if (ent->client->pers.weapon)
            i = ((ent->client->pers.weapon->weapmodel & 0xff) << 8);
        else
            i = 0;
        ent->s.skinnum = (ent - g_edicts - 1) | i;
    }

    if (ent->client->pers.weapon && ent->client->pers.weapon->ammo)
        ent->client->ammo_index = ITEM_INDEX(FindItem(ent->client->pers.weapon->ammo));
    else
        ent->client->ammo_index = 0;

    ent->client->weapon_sound = 0;
    ent->s.sound_pitch = 0;
    ent->client->ps.gun[0].spin = 0;

    if (!ent->client->pers.weapon) {
        // dead
        ent->client->ps.gun[0].index = 0;
        return false;
    }

    if (ent->client->pers.weapon->animation)
        Weapon_SetAnimation(ent, ent->client->pers.weapon->animation);
    else
    {
        ent->client->weaponstate = WEAPON_ACTIVATING;
        ent->client->ps.gun[0].frame = 0;
    }
    ent->client->ps.gun[0].index = SV_ModelIndex(ent->client->pers.weapon->view_model);

    ent->client->anim_priority = ANIM_PAIN;
    if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
        ent->s.frame = FRAME_crpain1;
        ent->client->anim_end = FRAME_crpain4;
    } else {
        ent->s.frame = FRAME_pain301;
        ent->client->anim_end = FRAME_pain304;
    }

    return false;
}

/*
=================
NoAmmoWeaponChange
=================
*/
static void NoAmmoWeaponChange(edict_t *ent)
{
    if (ent->client->pers.inventory[ITEM_INDEX(FindItem("bullets"))]
        &&  ent->client->pers.inventory[ITEM_INDEX(FindItem("perforator"))]) {
        ent->client->newweapon = FindItem("perforator");
        return;
    }
    if (ent->client->pers.inventory[ITEM_INDEX(FindItem("shells"))]
        &&  ent->client->pers.inventory[ITEM_INDEX(FindItem("shotgun"))]) {
        ent->client->newweapon = FindItem("shotgun");
        return;
    }
    ent->client->newweapon = FindItem("axe");
}

// not animation function; returns true if
// we can keep firing
bool Weapon_AmmoCheck(edict_t *ent)
{
    if (ent->client->newweapon)
        return false;

    if (ent->client->pers.inventory[ent->client->ammo_index] >= ent->client->pers.weapon->quantity) {
        return true;
    }

    SV_StartSound(ent, CHAN_VOICE, SV_SoundIndex("weapons/noammo.wav"), 1, ATTN_NORM, 0);

    NoAmmoWeaponChange(ent);

    return false;
}

/*
=================
Think_Weapon

Called by ClientBeginServerFrame and ClientThink
=================
*/
void Think_Weapon(edict_t *ent)
{
    // if just died, put the weapon away
    if (ent->health < 1) {
        ent->client->newweapon = NULL;
        ChangeWeapon(ent);
    }

    // call active weapon think routine
    if (ent->client->pers.weapon) {
        is_quad = (ent->client->quad_time > level.time);
        if (ent->client->silencer_shots)
            is_silenced = MZ_SILENCED;
        else
            is_silenced = 0;

        if (ent->client->pers.weapon->weaponthink)
            ent->client->pers.weapon->weaponthink(ent);
        else
            Weapon_RunAnimation(ent);
    }
}


/*
================
Use_Weapon

Make the weapon ready if there is ammo
================
*/
void Use_Weapon(edict_t *ent, gitem_t *item)
{
    int         ammo_index;
    gitem_t     *ammo_item;

    // see if we're already using it
    if (item == ent->client->pers.weapon)
        return;

    if (item->ammo && !g_select_empty.integer && !(item->flags & IT_AMMO)) {
        ammo_item = FindItem(item->ammo);
        ammo_index = ITEM_INDEX(ammo_item);

        if (!ent->client->pers.inventory[ammo_index]) {
            SV_ClientPrintf(ent, PRINT_HIGH, "No %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
            return;
        }

        if (ent->client->pers.inventory[ammo_index] < item->quantity) {
            SV_ClientPrintf(ent, PRINT_HIGH, "Not enough %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
            return;
        }
    }

    // change to this weapon when down
    ent->client->newweapon = item;
}

/*
================
Drop_Weapon
================
*/
void Drop_Weapon(edict_t *ent, gitem_t *item)
{
    int     index;

    if (dmflags.integer & DF_WEAPONS_STAY)
        return;

    index = ITEM_INDEX(item);
    // see if we're already using it
    if (((item == ent->client->pers.weapon) || (item == ent->client->newweapon)) && (ent->client->pers.inventory[index] == 1)) {
        SV_ClientPrint(ent, PRINT_HIGH, "Can't drop current weapon\n");
        return;
    }

    Drop_Item(ent, item);
    ent->client->pers.inventory[index]--;
}

/*
======================================================================

AXE

======================================================================
*/

void Weapon_SetAnimationFrame(edict_t *ent, const weapon_animation_t *animation, int32_t frame)
{
    ent->client->weaponanimation = animation;

    if (frame < animation->start || frame > animation->end)
    {
        Com_Print("Bad start frame\n");
        ent->client->ps.gun[0].frame = animation->start;
    }
    else
        ent->client->ps.gun[0].frame = frame;
}

void Weapon_SetAnimation(edict_t *ent, const weapon_animation_t *animation)
{
    Weapon_SetAnimationFrame(ent, animation, animation->start);
}

void Weapon_RunAnimation(edict_t *ent)
{
    int32_t currentFrame = ent->client->ps.gun[0].frame;
    const weapon_animation_t *animation = ent->client->weaponanimation;

    // assertion...
    if (currentFrame < animation->start || currentFrame > animation->end)
    {
        currentFrame = animation->start;
        Com_Print("animation out of range; restarting\n");
    }
    // will the next frame bring us to the end?
    else if (currentFrame + 1 > animation->end)
    {
        if (animation->finished && !animation->finished(ent))
            return;

        if (animation->next)
            Weapon_SetAnimation(ent, animation->next);
        else
            ent->client->ps.gun[0].frame = animation->start;

        return;
    }
    // regular frame, so we're going up by 1
    else
        currentFrame++;

    // run frame func first
    if (animation->frame && !animation->frame(ent))
        return;

    // run events
    for (const weapon_event_t *event = animation->events; event && event->func; event++)
        if ((event->start == WEAPON_EVENT_MINMAX || currentFrame >= event->start) &&
            (event->end == WEAPON_EVENT_MINMAX || currentFrame <= event->end))
            if (!event->func(ent))
                return;

    // copy over to visual model
    ent->client->ps.gun[0].frame = currentFrame;
}

/*
======================================================================

SHOTGUN / SUPERSHOTGUN

======================================================================
*/

void weapon_shotgun_fire(edict_t *ent)
{
    vec3_t      start;
    vec3_t      forward, right;
    vec3_t      offset;
    int         damage = 4;
    int         kick = 8;

    AngleVectors(ent->client->v_angle, forward, right, NULL);

    ent->client->kick_angles[0] = -2;

    VectorSet(offset, 0, 0, ent->viewheight);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    if (is_quad) {
        damage *= 4;
        kick *= 4;
    }

    if (deathmatch.integer)
        fire_shotgun(ent, start, forward, damage, kick, 200, 200, DEFAULT_DEATHMATCH_SHOTGUN_COUNT, MOD_SHOTGUN);
    else
        fire_shotgun(ent, start, forward, damage, kick, 200, 200, DEFAULT_SHOTGUN_COUNT, MOD_SHOTGUN);

    // send muzzle flash
    SV_WriteByte(svc_muzzleflash);
    SV_WriteShort(ent - g_edicts);
    SV_WriteByte(MZ_SHOTGUN | is_silenced);
    SV_Multicast(ent->s.origin, MULTICAST_PVS, false);
}

enum {
    EQUIP_FIRST,
    EQUIP_LAST,
    IDLE_FIRST,
    IDLE_LAST,
    ATTACK_FIRST,
    ATTACK_LAST,
    PUTAWAY_FIRST,
    PUTAWAY_LAST,
    INSPECT_FIRST,
    INSPECT_LAST,

    FRAMES_TOTAL
};

// shared generic entry point for basic weapons
inline void Weapon_Generic(edict_t *ent, const int frames[FRAMES_TOTAL], void (*fire) (edict_t *ent))
{
    ent->client->ps.gun[0].frame++;

    switch (ent->client->weaponstate)
    {
    case WEAPON_ACTIVATING:
        if (ent->client->ps.gun[0].frame == frames[EQUIP_LAST])
            ent->client->weaponstate = WEAPON_READY;
        break;
    case WEAPON_READY:
        if (ent->client->newweapon) {
            ent->client->weaponstate = WEAPON_DROPPING;
            ent->client->ps.gun[0].frame = frames[PUTAWAY_FIRST];
            break;
        } else if (((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK)) {

            if (frames[INSPECT_FIRST] && ent->client->ps.gun[0].frame >= frames[INSPECT_FIRST]) {
                ent->client->ps.gun[0].frame = frames[IDLE_FIRST];
            }

            ent->client->latched_buttons &= ~BUTTON_ATTACK;

            if (ent->client->pers.inventory[ent->client->ammo_index] >= ent->client->pers.weapon->quantity) {
                ent->client->anim_priority = ANIM_ATTACK;
                if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
                    ent->s.frame = FRAME_crattak1 - 1;
                    ent->client->anim_end = FRAME_crattak9;
                } else {
                    ent->s.frame = FRAME_attack1 - 1;
                    ent->client->anim_end = FRAME_attack8;
                }

                ent->client->weaponstate = WEAPON_FIRING;
                ent->client->ps.gun[0].frame = frames[ATTACK_FIRST];

                fire(ent);

                PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

                if (!(dmflags.integer & DF_INFINITE_AMMO))
                    ent->client->pers.inventory[ent->client->ammo_index] -= ent->client->pers.weapon->quantity;
            } else {
                if (level.time >= ent->pain_debounce_time) {
                    SV_StartSound(ent, CHAN_VOICE, SV_SoundIndex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
                    ent->pain_debounce_time = level.time + 1000;
                }
                NoAmmoWeaponChange(ent);
            }
        } else if (ent->client->inspect) {
            ent->client->inspect = false;
            ent->client->ps.gun[0].frame = frames[INSPECT_FIRST];
        } else if (ent->client->ps.gun[0].frame == frames[IDLE_LAST]) {
            if (random() < 0.1) {
                ent->client->ps.gun[0].frame = frames[INSPECT_FIRST];
            } else {
                ent->client->ps.gun[0].frame = frames[IDLE_FIRST];
            }
        } else if (ent->client->ps.gun[0].frame == frames[INSPECT_LAST]) {
            ent->client->ps.gun[0].frame = frames[IDLE_FIRST];
        }
        break;
    case WEAPON_FIRING:
        if (ent->client->ps.gun[0].frame == frames[ATTACK_LAST]) {
            ent->client->ps.gun[0].frame = frames[IDLE_FIRST];
            ent->client->weaponstate = WEAPON_READY;
        }
        break;
    case WEAPON_DROPPING:
        if (ent->client->ps.gun[0].frame == frames[PUTAWAY_LAST]) {
            ChangeWeapon(ent);
        }
        break;
    }
}

void Weapon_Shotgun(edict_t *ent)
{
    static const int frames[] = {
        [EQUIP_FIRST]    = 0,
        [EQUIP_LAST]     = 8,
        [IDLE_FIRST]     = 9,
        [IDLE_LAST]      = 68,
        [ATTACK_FIRST]   = 69,
        [ATTACK_LAST]    = 78,
        [PUTAWAY_FIRST]  = 79,
        [PUTAWAY_LAST]   = 81,
        [INSPECT_FIRST]  = 84,
        [INSPECT_LAST]   = 135
    };
    Weapon_Generic(ent, frames, weapon_shotgun_fire);
}

void weapon_supershotgun_fire(edict_t *ent)
{
    vec3_t      start;
    vec3_t      forward, right;
    vec3_t      offset;
    int         damage = 4;
    int         kick = 8;

    if (is_quad) {
        damage *= 4;
        kick *= 4;
    }

    AngleVectors(ent->client->v_angle, forward, right, NULL);

    ent->client->kick_angles[0] = -4;
    
    VectorSet(offset, 0, 0, ent->viewheight);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    if (deathmatch.integer)
        fire_shotgun(ent, start, forward, damage, kick, 350 * 2.2f, 350 * 1.1f, DEFAULT_DEATHMATCH_SHOTGUN_COUNT * 2, MOD_SHOTGUN);
    else
        fire_shotgun(ent, start, forward, damage, kick, 350 * 2.2f, 350 * 1.1f, DEFAULT_SHOTGUN_COUNT * 2, MOD_SHOTGUN);

    // send muzzle flash
    SV_WriteByte(svc_muzzleflash);
    SV_WriteShort(ent - g_edicts);
    SV_WriteByte(MZ_SHOTGUN | is_silenced);
    SV_Multicast(ent->s.origin, MULTICAST_PVS, false);
}

void Weapon_SuperShotgun(edict_t *ent)
{
    static const int frames[] = {
        [EQUIP_FIRST]    = 0,
        [EQUIP_LAST]     = 8,
        [IDLE_FIRST]     = 9,
        [IDLE_LAST]      = 68,
        [ATTACK_FIRST]   = 69,
        [ATTACK_LAST]    = 78,
        [PUTAWAY_FIRST]  = 79,
        [PUTAWAY_LAST]   = 81,
        [INSPECT_FIRST]  = 0,
        [INSPECT_LAST]   = 0
    };
    Weapon_Generic(ent, frames, weapon_supershotgun_fire);
}

void weapon_nailgun_fire(edict_t *ent)
{
    vec3_t      start;
    vec3_t      forward, right;
    vec3_t      offset;

    AngleVectors(ent->client->v_angle, forward, right, NULL);

    ent->client->kick_angles[0] = -1;
    
    VectorSet(offset, 0, 0, ent->viewheight);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    fire_nail(ent, start, forward, 12, 2000);

    // send muzzle flash
    SV_WriteByte(svc_muzzleflash);
    SV_WriteShort(ent - g_edicts);
    SV_WriteByte(MZ_SHOTGUN | is_silenced);
    SV_Multicast(ent->s.origin, MULTICAST_PVS, false);
}

void Weapon_Nailgun(edict_t *ent)
{
    static const int frames[] = {
        [EQUIP_FIRST]    = 0,
        [EQUIP_LAST]     = 8,
        [IDLE_FIRST]     = 9,
        [IDLE_LAST]      = 68,
        [ATTACK_FIRST]   = 69,
        [ATTACK_LAST]    = 78,
        [PUTAWAY_FIRST]  = 79,
        [PUTAWAY_LAST]   = 81,
        [INSPECT_FIRST]  = 0,
        [INSPECT_LAST]   = 0
    };
    Weapon_Generic(ent, frames, weapon_nailgun_fire);
}

void weapon_grenadelauncher_fire(edict_t *ent)
{
    vec3_t      start;
    vec3_t      forward, right;
    vec3_t      offset;

    AngleVectors(ent->client->v_angle, forward, right, NULL);

    ent->client->kick_angles[0] = -1;
    
    VectorSet(offset, 0, 0, ent->viewheight);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    fire_grenade(ent, start, forward, 100, 800, 2.5f, 75);

    // send muzzle flash
    SV_WriteByte(svc_muzzleflash);
    SV_WriteShort(ent - g_edicts);
    SV_WriteByte(MZ_SHOTGUN | is_silenced);
    SV_Multicast(ent->s.origin, MULTICAST_PVS, false);
}

void Weapon_GrenadeLauncher(edict_t *ent)
{
    static const int frames[] = {
        [EQUIP_FIRST]    = 0,
        [EQUIP_LAST]     = 8,
        [IDLE_FIRST]     = 9,
        [IDLE_LAST]      = 68,
        [ATTACK_FIRST]   = 69,
        [ATTACK_LAST]    = 78,
        [PUTAWAY_FIRST]  = 79,
        [PUTAWAY_LAST]   = 81,
        [INSPECT_FIRST]  = 0,
        [INSPECT_LAST]   = 0
    };
    Weapon_Generic(ent, frames, weapon_grenadelauncher_fire);
}

void weapon_rocketlauncher_fire(edict_t *ent)
{
    vec3_t      start;
    vec3_t      forward, right;
    vec3_t      offset;

    AngleVectors(ent->client->v_angle, forward, right, NULL);

    ent->client->kick_angles[0] = -1;
    
    VectorSet(offset, 0, 0, ent->viewheight);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    fire_rocket(ent, start, forward, 100, 800, 120, 75);

    // send muzzle flash
    SV_WriteByte(svc_muzzleflash);
    SV_WriteShort(ent - g_edicts);
    SV_WriteByte(MZ_SHOTGUN | is_silenced);
    SV_Multicast(ent->s.origin, MULTICAST_PVS, false);
}

void Weapon_RocketLauncher(edict_t *ent)
{
    static const int frames[] = {
        [EQUIP_FIRST]    = 0,
        [EQUIP_LAST]     = 8,
        [IDLE_FIRST]     = 9,
        [IDLE_LAST]      = 68,
        [ATTACK_FIRST]   = 69,
        [ATTACK_LAST]    = 78,
        [PUTAWAY_FIRST]  = 79,
        [PUTAWAY_LAST]   = 81,
        [INSPECT_FIRST]  = 0,
        [INSPECT_LAST]   = 0
    };
    Weapon_Generic(ent, frames, weapon_rocketlauncher_fire);
}

void weapon_thunderbolt_fire(edict_t *ent)
{
    vec3_t      start;
    vec3_t      forward, right;
    vec3_t      offset;

    AngleVectors(ent->client->v_angle, forward, right, NULL);

    ent->client->kick_angles[0] = -1;
    
    VectorSet(offset, 0, 0, ent->viewheight);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    if (SV_PointContents(start) & MASK_WATER) {
        T_RadiusDamage(ent, ent, 9999, NULL, 9999, MOD_RAILGUN);
        return;
    }

    vec3_t end;
    VectorMA(start, 8192, forward, end);

    trace_t tr = SV_Trace(start, vec3_origin, vec3_origin, end, ent, MASK_SHOT);

    if (tr.ent->takedamage)
        T_Damage(tr.ent, ent, ent, forward, tr.endpos, tr.plane.normal, 7, 15, DAMAGE_ENERGY, MOD_RAILGUN);

    SV_WriteByte(svc_temp_entity);
    SV_WriteByte(TE_LIGHTNING);
    SV_WriteShort(ent->s.number);
    SV_WriteShort(-1);
    SV_WritePos(start);
    SV_WritePos(tr.endpos);
    SV_Multicast(tr.endpos, MULTICAST_PVS, false);

    // send muzzle flash
    SV_WriteByte(svc_muzzleflash);
    SV_WriteShort(ent - g_edicts);
    SV_WriteByte(MZ_SHOTGUN | is_silenced);
    SV_Multicast(ent->s.origin, MULTICAST_PVS, false);
}

void Weapon_Thunderbolt(edict_t *ent)
{
    enum {
        ANIM_EQUIP_FIRST      = 0,
        ANIM_EQUIP_LAST       = 8,
        ANIM_IDLE_FIRST       = 9,
        ANIM_IDLE_LAST        = 68,
        ANIM_ATTACK_FIRST     = 69,
        ANIM_ATTACK_LAST      = 78,
        ANIM_PUTAWAY_FIRST    = 79,
        ANIM_PUTAWAY_LAST     = 81,
        ANIM_INSPECT_FIRST    = 9,
        ANIM_INSPECT_LAST     = 68
    };
    
    ent->client->ps.gun[0].frame++;

    switch (ent->client->weaponstate)
    {
    case WEAPON_ACTIVATING:
        if (ent->client->ps.gun[0].frame == ANIM_EQUIP_LAST)
            ent->client->weaponstate = WEAPON_READY;
        break;
    case WEAPON_READY:
        if (((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK)) {
            ent->client->latched_buttons &= ~BUTTON_ATTACK;

            if (ent->client->ps.gun[0].frame >= ANIM_INSPECT_FIRST) {
                ent->client->ps.gun[0].frame = ANIM_IDLE_FIRST;
            }

            if (ent->client->pers.inventory[ent->client->ammo_index] >= ent->client->pers.weapon->quantity) {
                ent->client->weaponstate = WEAPON_FIRING;
            } else {
                if (level.time >= ent->pain_debounce_time) {
                    SV_StartSound(ent, CHAN_VOICE, SV_SoundIndex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
                    ent->pain_debounce_time = level.time + 1000;
                }
                NoAmmoWeaponChange(ent);
                goto ready_noammo;
            }

            // intentional fall-through
        } else {
ready_noammo:
            if (ent->client->inspect) {
                ent->client->inspect = false;
                ent->client->ps.gun[0].frame = ANIM_INSPECT_FIRST;
            } else if (ent->client->ps.gun[0].frame == ANIM_IDLE_LAST) {
                if (random() < 0.1) {
                    ent->client->ps.gun[0].frame = ANIM_INSPECT_FIRST;
                } else {
                    ent->client->ps.gun[0].frame = ANIM_IDLE_FIRST;
                }
            } else if (ent->client->ps.gun[0].frame == ANIM_INSPECT_LAST) {
                ent->client->ps.gun[0].frame = ANIM_IDLE_FIRST;
            }

            if (ent->client->newweapon) {
                ent->client->weaponstate = WEAPON_DROPPING;
                ent->client->ps.gun[0].frame = ANIM_PUTAWAY_FIRST;
                break;
            }
        }
    case WEAPON_FIRING:
        if ((ent->client->buttons | ent->client->latched_buttons) & BUTTON_ATTACK) {
            ent->client->latched_buttons &= ~BUTTON_ATTACK;

            if (ent->client->pers.inventory[ent->client->ammo_index] >= ent->client->pers.weapon->quantity) {

                ent->client->ps.gun[0].frame = ANIM_ATTACK_FIRST;

                weapon_thunderbolt_fire(ent);

                if (ent->deadflag) {
                    return;
                }

                ent->client->anim_priority = ANIM_ATTACK;
                if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
                    ent->s.frame = (FRAME_crattak1 - 1) + (level.time % (int)(BASE_FRAMETIME * 2));
                    ent->client->anim_end = FRAME_crattak9;
                } else {
                    ent->s.frame = (FRAME_attack1 - 1) + (level.time % (int)(BASE_FRAMETIME * 2));
                    ent->client->anim_end = FRAME_attack8;
                }

                if (!(dmflags.integer & DF_INFINITE_AMMO))
                    ent->client->pers.inventory[ent->client->ammo_index]--;
            } else {
                if (level.time >= ent->pain_debounce_time) {
                    SV_StartSound(ent, CHAN_VOICE, SV_SoundIndex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
                    ent->pain_debounce_time = level.time + 1000;
                }
                NoAmmoWeaponChange(ent);
            }

            if (ent->client->ps.gun[0].frame == ANIM_IDLE_LAST) {
                ent->client->ps.gun[0].frame = ANIM_IDLE_FIRST;
            } else if (ent->client->ps.gun[0].frame == ANIM_ATTACK_LAST) {
                ent->client->ps.gun[0].frame = ANIM_ATTACK_FIRST;
            }
        } else {
            if (ent->client->weaponstate == WEAPON_FIRING) {
                ent->client->ps.gun[0].frame = ANIM_IDLE_FIRST;
                ent->client->weaponstate = WEAPON_READY;
            }
        }
        break;
    case WEAPON_DROPPING:
        if (ent->client->ps.gun[0].frame == ANIM_PUTAWAY_LAST)
            ChangeWeapon(ent);
        break;
    }
}
