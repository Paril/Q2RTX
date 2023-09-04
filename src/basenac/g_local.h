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
// g_local.h -- local definitions for game module

#include "shared/shared.h"
#include "shared/list.h"

// define GAME_INCLUDE so that game.h does not define the
// short, server-visible gclient_t and edict_t structures,
// because we define the full size ones in this file
#define GAME_INCLUDE
#include "shared/game.h"

// features this game supports
#define G_FEATURES  (GMF_PROPERINUSE|GMF_WANT_ALL_DISCONNECTS|GMF_ENHANCED_SAVEGAMES)

// the "gameversion" client command will print this plus compile date
#define GAMEVERSION "basenac"

// protocol bytes that can be directly added to messages
#define svc_muzzleflash     1
#define svc_muzzleflash2    2
#define svc_temp_entity     3
#define svc_layout          4
#define svc_inventory       5
#define svc_stufftext       11

//==================================================================

// 64-bit milliseconds
typedef int64_t gtime_t;

static inline gtime_t G_SecToMs(float sec)
{
    return sec * 1000;
}

static inline float G_MsToSec(gtime_t millis)
{
    return millis / 1000.f;
}

static inline gtime_t G_MinToMs(float min)
{
    return (min * 60) * 1000;
}

static inline float G_MsToMin(gtime_t millis)
{
    return (millis / 1000.f) / 60.f;
}

static inline gtime_t G_FramesToMs(int frames)
{
    return frames * BASE_FRAMETIME;
}

static inline float G_MsToFrames(gtime_t ms)
{
    return ms / BASE_FRAMETIME;
}

// view pitching times
#define DAMAGE_TIME     500
#define FALL_TIME       300

// edict->spawnflags
// these are set with checkboxes on each entity in the map editor
#define SPAWNFLAG_NOT_EASY          0x00000100
#define SPAWNFLAG_NOT_MEDIUM        0x00000200
#define SPAWNFLAG_NOT_HARD          0x00000400
#define SPAWNFLAG_NOT_DEATHMATCH    0x00000800
#define SPAWNFLAG_NOT_COOP          0x00001000

// edict->flags
#define FL_FLY                  0x00000001
#define FL_SWIM                 0x00000002  // implied immunity to drowining
#define FL_IMMUNE_LASER         0x00000004
#define FL_INWATER              0x00000008
#define FL_GODMODE              0x00000010
#define FL_NOTARGET             0x00000020
#define FL_IMMUNE_SLIME         0x00000040
#define FL_IMMUNE_LAVA          0x00000080
#define FL_PARTIALGROUND        0x00000100  // not all corners are valid
#define FL_WATERJUMP            0x00000200  // player jumping out of water
#define FL_TEAMSLAVE            0x00000400  // not the first on the team
#define FL_NO_KNOCKBACK         0x00000800
#define FL_ACCELERATE           0x00001000  // accelerative movement (plats, etc)
#define FL_RESPAWN              0x80000000  // used for item respawning

#define MELEE_DISTANCE  80

#define BODY_QUEUE_SIZE     8

typedef enum {
    DAMAGE_NO,
    DAMAGE_YES,         // will take damage if hit
    DAMAGE_AIM          // auto targeting recognizes this
} damage_t;

//deadflag
#define DEAD_NO                 0
#define DEAD_DYING              1
#define DEAD_DEAD               2
#define DEAD_RESPAWNABLE        3

//range
#define RANGE_MELEE             0
#define RANGE_NEAR              1
#define RANGE_MID               2
#define RANGE_FAR               3

//gib types
#define GIB_ORGANIC             0
#define GIB_METALLIC            1

//monster ai flags
#define AI_STAND_GROUND         0x00000001
#define AI_TEMP_STAND_GROUND    0x00000002
#define AI_SOUND_TARGET         0x00000004
#define AI_LOST_SIGHT           0x00000008
#define AI_PURSUIT_LAST_SEEN    0x00000010
#define AI_PURSUE_NEXT          0x00000020
#define AI_PURSUE_TEMP          0x00000040
#define AI_HOLD_FRAME           0x00000080
#define AI_GOOD_GUY             0x00000100
#define AI_BRUTAL               0x00000200
#define AI_NOSTEP               0x00000400
#define AI_DUCKED               0x00000800
#define AI_COMBAT_POINT         0x00001000
#define AI_MEDIC                0x00002000
#define AI_RESURRECTING         0x00004000
#define AI_HIGH_TICK_RATE       0x00008000
#define AI_JUMP_IMMEDIATELY     0x00010000
#define AI_ALLOW_BLIND_FIRE     0x00020000
#define AI_DO_BLIND_FIRE        0x00040000

//monster attack state
#define AS_STRAIGHT             1
#define AS_SLIDING              2
#define AS_MELEE                3
#define AS_MISSILE              4

// handedness values
#define RIGHT_HANDED            0
#define LEFT_HANDED             1
#define CENTER_HANDED           2


// game.serverflags values
#define SFL_CROSS_TRIGGER_1     0x00000001
#define SFL_CROSS_TRIGGER_2     0x00000002
#define SFL_CROSS_TRIGGER_3     0x00000004
#define SFL_CROSS_TRIGGER_4     0x00000008
#define SFL_CROSS_TRIGGER_5     0x00000010
#define SFL_CROSS_TRIGGER_6     0x00000020
#define SFL_CROSS_TRIGGER_7     0x00000040
#define SFL_CROSS_TRIGGER_8     0x00000080
#define SFL_CROSS_TRIGGER_MASK  0x000000ff


// noise types for PlayerNoise
#define PNOISE_SELF             0
#define PNOISE_WEAPON           1
#define PNOISE_IMPACT           2


// edict->movetype values
typedef enum {
    MOVETYPE_NONE,          // never moves
    MOVETYPE_NOCLIP,        // origin and angles change with no interaction
    MOVETYPE_PUSH,          // no clip to world, push on box contact
    MOVETYPE_STOP,          // no clip to world, stops on box contact

    MOVETYPE_WALK,          // gravity
    MOVETYPE_STEP,          // gravity, special edge handling
    MOVETYPE_FLY,
    MOVETYPE_TOSS,          // gravity
    MOVETYPE_FLYMISSILE,    // extra size to monsters
    MOVETYPE_BOUNCE
} movetype_t;



// for start/end; allow any value
// for this particular cap
#define WEAPON_EVENT_MINMAX -1

// return false to cancel usual handling
typedef bool (*weapon_func_t) (struct edict_s *ent);

typedef struct weapon_event_s {
    weapon_func_t func; // function; null for end of list
    int32_t start, end; // frame range, or WEAPON_EVENT_ALWAYS
} weapon_event_t;

typedef enum {
    WEAPID_GUN,
    WEAPID_AXE,

    WEAPID_TOTAL
} weapon_id_t;

typedef struct weapon_animation_s {
    int32_t start, end;
    const struct weapon_animation_s *next; // null for looping animations, otherwise you must use a Finished func
    weapon_func_t frame, finished;
    const weapon_event_t *events;
} weapon_animation_t;

typedef struct {
    int     base_count;
    int     max_count;
    float   normal_protection;
    float   energy_protection;
} gitem_armor_t;

// gitem_t->flags
typedef enum {
    IT_WEAPON       = 1 << 0, // use makes active weapon
    IT_AMMO         = 1 << 1,
    IT_ARMOR        = 1 << 2,
    IT_STAY_COOP    = 1 << 3,
    IT_KEY          = 1 << 4,
    IT_POWERUP      = 1 << 5,
    IT_HEALTH       = 1 << 6,
} gitem_flags_t;

// gitem_t->weapmodel for weapons indicates model index
typedef enum {
    WEAP_BLASTER            = 1,
    WEAP_SHOTGUN            = 2,
    WEAP_SUPERSHOTGUN       = 3,
    WEAP_MACHINEGUN         = 4,
    WEAP_CHAINGUN           = 5,
    WEAP_GRENADES           = 6,
    WEAP_GRENADELAUNCHER    = 7,
    WEAP_ROCKETLAUNCHER     = 8,
    WEAP_HYPERBLASTER       = 9,
    WEAP_RAILGUN            = 10,
    WEAP_BFG                = 11,
    WEAP_FLAREGUN           = 12,
} gitem_vwep_t;

// Item ID list. Item list fills out the
// details of every item. 0 must always be NULL,
// and the weapons should be grouped together for
// vwep purposes.
typedef enum {
    // MUST ALWAYS BE AT THE BEGINNING
    ITEM_NULL,

    ITEM_AXE,
    ITEM_BLASTER,
    ITEM_SHOTGUN,
    ITEM_SSHOTGUN,
    ITEM_PERFORATOR,
    ITEM_GRENADE_LAUNCHER,
    ITEM_ROCKET_LAUNCHER,
    ITEM_THUNDERBOLT,

    ITEM_HEALTH_SMALL,
    ITEM_HEALTH_ROTTEN,
    ITEM_HEALTH,
    ITEM_HEALTH_MEGA,

    ITEM_ARMOR_SHARD,
    ITEM_ARMOR_GREEN,
    ITEM_ARMOR_YELLOW,
    ITEM_ARMOR_RED,

    ITEM_SHELLS,
    ITEM_NAILS,
    ITEM_CELLS,
    ITEM_ROCKETS,
    ITEM_GRENADES,

    ITEM_QUAD_DAMAGE,
    ITEM_PENTAGRAM,
    ITEM_RING_OF_SHADOWS,
    ITEM_BIOSUIT,

    ITEM_KEY_SILVER,
    ITEM_KEY_GOLD,

    // MUST ALWAYS BE AT THE END
    ITEM_TOTAL
} gitem_id_t;

typedef struct gitem_s {
    char        *classname; // spawning name
    bool        (*pickup)(struct edict_s *ent, struct edict_s *other);
    void        (*use)(struct edict_s *ent, struct gitem_s *item);
    void        (*drop)(struct edict_s *ent, struct gitem_s *item);
    const       weapon_animation_t *animation; // animation to 'start' this weapon
    char        *pickup_sound;
    char        *world_model;
    int         world_model_flags;
    char        *view_model;
    int         skinnum; 

    // client side info
    char        *icon;
    char        *pickup_name;   // for printing on pickup

    int             quantity;      // for ammo how much, for weapons how much is used per shot
    gitem_id_t      ammo;          // for weapons
    gitem_flags_t   flags;         // IT_* flags

    gitem_vwep_t    weapmodel;      // weapon model index (for weapons)
    gitem_armor_t   *armor;

    char        *precaches;     // string of all models, sounds, and images this item will use

    weapon_id_t weapid;
    gitem_id_t  id; // set by InitItems
} gitem_t;

//
// this structure is left intact through an entire game
// it should be initialized at dll load time, and read/written to
// the server.ssv file for savegames
//
typedef struct {
    char        helpmessage1[512];
    char        helpmessage2[512];
    int         helpchanged;    // flash F1 icon if non 0, play sound
                                // and increment only if 1, 2, or 3

    gclient_t   *clients;       // [maxclients]

    // can't store spawnpoint in level, because
    // it would get overwritten by the savegame restore
    char        spawnpoint[512];    // needed for coop respawns

    // store latched cvars here that we want to get at often
    int         maxclients;

    // cross level triggers
    int         serverflags;

    bool        autosaved;

    // world entity ptr
    edict_t     *world;
} game_locals_t;


//
// this structure is cleared as each map is entered
// it is read/written to the level.sav file for savegames
//
typedef struct {
    gtime_t     time;

    char        level_name[MAX_QPATH];  // the descriptive name (Outer Base, etc)
    char        mapname[MAX_QPATH];     // the server name (base1, etc)
    char        nextmap[MAX_QPATH];     // go here when fraglimit is hit

    // intermission state
    gtime_t     intermission_time;  // time the intermission was started
    char        *changemap;
    int         exitintermission;
    vec3_t      intermission_origin;
    vec3_t      intermission_angle;

    edict_t     *sight_client;  // changed once each frame for coop games

    edict_t     *sight_entity;
    gtime_t     sight_entity_time;
    edict_t     *sound_entity;
    gtime_t     sound_entity_time;
    edict_t     *sound2_entity;
    gtime_t     sound2_entity_time;

    int         pic_health;

    int         total_secrets;
    int         found_secrets;

    int         total_goals;
    int         found_goals;

    int         total_monsters;
    int         killed_monsters;

    edict_t     *current_entity;    // entity running from G_RunFrame
    int         body_que;           // dead bodies

    // Paril: level gravity change support.
    // we have to do this here for save/load support.
    float       gravity;
    int         default_reverb;
    edict_t     *reverb_entities;
} level_locals_t;


// spawn_temp_t is only used to hold entity field values that
// can be set from the editor, but aren't actualy present
// in edict_t during gameplay
typedef struct {
    // world vars
    char        *sky;
    float       skyrotate;
    int         skyautorotate;
    vec3_t      skyaxis;
    char        *nextmap;

    int         lip;
    int         distance;
    int         height;
    char        *noise;
    float       pausetime;
    char        *item;
    char        *gravity;

    float       minyaw;
    float       maxyaw;
    float       minpitch;
    float       maxpitch;

    int         default_reverb;

    vec3_t      color;
    int         intensity;
    float       width_angle, falloff_angle;
} spawn_temp_t;


typedef struct {
    // fixed data
    vec3_t      start_origin;
    vec3_t      start_angles;
    vec3_t      end_origin;
    vec3_t      end_angles;

    int         sound_start;
    int         sound_middle;
    int         sound_end;

    float       accel;
    float       speed;
    float       decel;
    float       distance;

    float       wait;

    // state data
    int         state;
    vec3_t      dir;
    float       current_speed;
    float       move_speed;
    float       next_speed;
    float       remaining_distance;
    float       decel_distance;
    void        (*endfunc)(edict_t *);
    vec3_t      dest;
} moveinfo_t;


typedef struct {
    void    (*aifunc)(edict_t *self, float dist);
    float   dist;
    void    (*thinkfunc)(edict_t *self);
    vec3_t  translate;
} mframe_t;

typedef struct {
    int         firstframe;
    int         lastframe;
    mframe_t    *frame;
    void        (*endfunc)(edict_t *self);
    void        (*default_aifunc)(edict_t *self, float dist);
    void        (*thinkfunc)(edict_t *self);
} mmove_t;

typedef struct {
    mmove_t     *currentmove;
    int         aiflags;
    int         nextframe;
    float       scale;

    void        (*stand)(edict_t *self);
    void        (*idle)(edict_t *self);
    void        (*search)(edict_t *self);
    void        (*walk)(edict_t *self);
    void        (*run)(edict_t *self);
    void        (*dodge)(edict_t *self, edict_t *other, float eta);
    void        (*attack)(edict_t *self);
    void        (*melee)(edict_t *self);
    void        (*sight)(edict_t *self, edict_t *other);
    bool        (*checkattack)(edict_t *self);

    gtime_t     pause_time;
    gtime_t     attack_finished_time;

    vec3_t      saved_goal;
    gtime_t     search_time;
    gtime_t     trail_time;
    vec3_t      last_sighting;
    int         attack_state;
    int         lefty;
    gtime_t     idle_time;
    int         linkcount;

    void        (*load)(edict_t *self);

    float       melee_distance;
} monsterinfo_t;



extern  game_locals_t   game;
extern  level_locals_t  level;
extern  game_export_t   globals;
extern  spawn_temp_t    st;

extern  int sm_meat_index;
extern  int snd_fry;

//extern  int green_armor_index;
//extern  int yellow_armor_index;
//extern  int red_armor_index;


// means of death
#define MOD_UNKNOWN         0
#define MOD_BLASTER         1
#define MOD_SHOTGUN         2
#define MOD_SSHOTGUN        3
#define MOD_MACHINEGUN      4
#define MOD_CHAINGUN        5
#define MOD_GRENADE         6
#define MOD_G_SPLASH        7
#define MOD_ROCKET          8
#define MOD_R_SPLASH        9
#define MOD_HYPERBLASTER    10
#define MOD_RAILGUN         11
#define MOD_BFG_LASER       12
#define MOD_BFG_BLAST       13
#define MOD_BFG_EFFECT      14
#define MOD_HANDGRENADE     15
#define MOD_HG_SPLASH       16
#define MOD_WATER           17
#define MOD_SLIME           18
#define MOD_LAVA            19
#define MOD_CRUSH           20
#define MOD_TELEFRAG        21
#define MOD_FALLING         22
#define MOD_SUICIDE         23
#define MOD_HELD_GRENADE    24
#define MOD_EXPLOSIVE       25
#define MOD_BARREL          26
#define MOD_BOMB            27
#define MOD_EXIT            28
#define MOD_SPLASH          29
#define MOD_TARGET_LASER    30
#define MOD_TRIGGER_HURT    31
#define MOD_HIT             32
#define MOD_TARGET_BLASTER  33
#define MOD_FRIENDLY_FIRE   0x8000000

extern  int meansOfDeath;

#define FOFS(x) q_offsetof(edict_t, x)
#define STOFS(x) q_offsetof(spawn_temp_t, x)
#define LLOFS(x) q_offsetof(level_locals_t, x)
#define GLOFS(x) q_offsetof(game_locals_t, x)
#define CLOFS(x) q_offsetof(gclient_t, x)

#define random()    frand()
#define crandom()   crand()

extern  cvarRef_t  deathmatch;
extern  cvarRef_t  coop;
extern  cvarRef_t  dmflags;
extern  cvarRef_t  skill;
extern  cvarRef_t  fraglimit;
extern  cvarRef_t  timelimit;
extern  cvarRef_t  password;
extern  cvarRef_t  spectator_password;
extern  cvarRef_t  needpass;
extern  cvarRef_t  g_select_empty;
extern  cvarRef_t  dedicated;
extern  cvarRef_t  nomonsters;
extern  cvar_t  *aimfix;

extern  cvarRef_t  filterban;

extern  cvarRef_t  sv_gravity;
extern  cvarRef_t  sv_maxvelocity;

extern  cvarRef_t  sv_rollspeed;
extern  cvarRef_t  sv_rollangle;

extern  cvarRef_t  run_pitch;
extern  cvarRef_t  run_roll;
extern  cvarRef_t  bob_up;
extern  cvarRef_t  bob_pitch;
extern  cvarRef_t  bob_roll;

extern  cvarRef_t  sv_cheats;
extern  cvarRef_t  maxspectators;

extern  cvarRef_t  flood_msgs;
extern  cvarRef_t  flood_persecond;
extern  cvarRef_t  flood_waitdelay;

extern  cvarRef_t  sv_maplist;

extern  cvarRef_t  sv_features;

// item spawnflags
#define ITEM_TRIGGER_SPAWN      0x00000001
#define ITEM_NO_TOUCH           0x00000002
// 6 bits reserved for editor flags
#define DROPPED_ITEM            0x00010000
#define DROPPED_PLAYER_ITEM     0x00020000
#define ITEM_TARGETS_USED       0x00040000

//
// fields are needed for spawning from the entity string
// and saving / loading games
//
typedef enum {
    F_BAD,
    F_BYTE,
    F_SHORT,
    F_INT,
    F_BOOL,
    F_FLOAT,
    F_LSTRING,          // string on disk, pointer in memory, TAG_LEVEL
    F_GSTRING,          // string on disk, pointer in memory, TAG_GAME
    F_ZSTRING,          // string on disk, string in memory
    F_VECTOR,
    F_EDICT,            // index on disk, pointer in memory
    F_ITEM,             // string on disk, pointer in memory
    F_ITEM_ID,          // string on disk, gitem_id_t in memory
    F_CLIENT,           // index on disk, pointer in memory
    F_FUNCTION,
    F_POINTER,
    F_IGNORE,
    F_INT64,

    // parsing only
    F_ANGLEHACK,
    F_COLOR
} fieldtype_t;

//
// g_cmds.c
//
void Cmd_Help_f(edict_t *ent);
void Cmd_Score_f(edict_t *ent);

//
// g_items.c
//
void PrecacheItem(gitem_t *it);
void InitItems(void);
void SetItemNames(void);
gitem_t *FindItem(char *pickup_name);
gitem_t *FindItemByClassname(char *classname);
edict_t *Drop_Item(edict_t *ent, gitem_t *item);
void SetRespawn(edict_t *ent, float delay);
bool ChangeWeapon(edict_t *ent);
void Weapon_Activate(edict_t *ent, bool switched);
void SpawnItem(edict_t *ent, gitem_t *item);
void Think_Weapon(edict_t *ent);
int ArmorIndex(edict_t *ent);
gitem_t *GetItemByIndex(gitem_id_t index);
bool Add_Ammo(edict_t *ent, gitem_t *item, int count);
void Touch_Item(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf);

//
// g_utils.c
//
bool    KillBox(edict_t *ent);
void    G_ProjectSource(const vec3_t point, const vec3_t distance, const vec3_t forward, const vec3_t right, vec3_t result);
edict_t *G_NextEnt(edict_t *from);
edict_t *G_Find(edict_t *from, int fieldofs, char *match);
edict_t *findradius(edict_t *from, vec3_t org, float rad);
edict_t *G_PickTarget(char *targetname);
void    G_UseTargets(edict_t *ent, edict_t *activator);
void    G_SetMovedir(vec3_t angles, vec3_t movedir);

void    G_InitEdict(edict_t *e);
edict_t *G_Spawn(void);
edict_t *G_SpawnType(entity_type_t type);
void    G_FreeEdict(edict_t *e);

void    G_TouchTriggers(edict_t *ent);

float   *tv(float x, float y, float z);

float vectoyaw(vec3_t vec);
void vectoangles(vec3_t vec, vec3_t angles);

//
// g_combat.c
//
bool OnSameTeam(edict_t *ent1, edict_t *ent2);
bool CanDamage(edict_t *targ, edict_t *inflictor);
void T_Damage(edict_t *targ, edict_t *inflictor, edict_t *attacker, const vec3_t dir, vec3_t point, const vec3_t normal, int damage, int knockback, int dflags, int mod);
void T_RadiusDamage(edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int mod);

// damage flags
#define DAMAGE_RADIUS           0x00000001  // damage was indirect
#define DAMAGE_NO_ARMOR         0x00000002  // armour does not protect from this damage
#define DAMAGE_ENERGY           0x00000004  // damage is from an energy based weapon
#define DAMAGE_NO_KNOCKBACK     0x00000008  // do not affect velocity, just view angles
#define DAMAGE_BULLET           0x00000010  // damage is from a bullet (used for ricochets)
#define DAMAGE_NO_PROTECTION    0x00000020  // armor, shields, invulnerability, and godmode have no effect

#define DEFAULT_BULLET_HSPREAD  300
#define DEFAULT_BULLET_VSPREAD  500
#define DEFAULT_SHOTGUN_HSPREAD 1000
#define DEFAULT_SHOTGUN_VSPREAD 500
#define DEFAULT_DEATHMATCH_SHOTGUN_COUNT    12
#define DEFAULT_SHOTGUN_COUNT   12
#define DEFAULT_SSHOTGUN_COUNT  20

//
// g_monster.c
//
void monster_fire_bullet(edict_t *self, vec3_t start, vec3_t dir, int damage, int kick, int hspread, int vspread, int flashtype);
void monster_fire_shotgun(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int count, int flashtype);
void monster_fire_blaster(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashtype, int effect);
void monster_fire_grenade(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int flashtype);
void monster_fire_rocket(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashtype);
void monster_fire_railgun(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int flashtype);
void monster_fire_bfg(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int kick, float damage_radius, int flashtype);
void M_droptofloor(edict_t *ent);
void monster_think(edict_t *self);
void walkmonster_start(edict_t *self);
void swimmonster_start(edict_t *self);
void flymonster_start(edict_t *self);
void AttackFinished(edict_t *self, float time);
void monster_death_use(edict_t *self);
void M_CatagorizePosition(edict_t *ent);
bool M_CheckAttack(edict_t *self);
void M_FlyCheck(edict_t *self);
void M_CheckGround(edict_t *ent);

//
// g_misc.c
//
void ThrowHead(edict_t *self, const char *gibname, int damage, int type);
void ThrowClientHead(edict_t *self, int damage);
void ThrowGib(edict_t *self, const char *gibname, int damage, int type);
void BecomeExplosion1(edict_t *self);

#define CLOCK_MESSAGE_SIZE  16
void func_clock_think(edict_t *self);
void func_clock_use(edict_t *self, edict_t *other, edict_t *activator);

//
// g_ai.c
//
void AI_SetSightClient(void);

void ai_stand(edict_t *self, float dist);
void ai_move(edict_t *self, float dist);
void ai_walk(edict_t *self, float dist);
void ai_turn(edict_t *self, float dist);
void ai_run(edict_t *self, float dist);
void ai_charge(edict_t *self, float dist);
int range(edict_t *self, edict_t *other);

void FoundTarget(edict_t *self);
bool infront(edict_t *self, edict_t *other);
bool visible(edict_t *self, edict_t *other);
bool FacingIdeal(edict_t *self);

//
// g_weapon.c
//
void ThrowDebris(edict_t *self, const char *modelname, float speed, vec3_t origin);
bool fire_hit(edict_t *self, vec3_t aim, int damage, int kick);
void fire_bullet(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int mod);
void fire_shotgun(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int count, int mod);
void fire_blaster(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int effect, bool hyper);
void fire_grenade(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, float timer, float damage_radius);
void fire_grenade2(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, float timer, float damage_radius, bool held);
void fire_rocket(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius, int radius_damage);
void fire_nail(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed);
void fire_rail(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick);
void fire_bfg(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius);

//
// g_ptrail.c
//
void PlayerTrail_Init(void);
void PlayerTrail_Add(vec3_t spot);
void PlayerTrail_New(vec3_t spot);
edict_t *PlayerTrail_PickFirst(edict_t *self);
edict_t *PlayerTrail_PickNext(edict_t *self);
edict_t *PlayerTrail_LastSpot(void);

//
// g_client.c
//
void respawn(edict_t *ent);
void BeginIntermission(edict_t *targ);
void PutClientInServer(edict_t *ent);
void InitClientPersistant(gclient_t *client);
void InitClientResp(gclient_t *client);
void InitBodyQue(void);
void ClientBeginServerFrame(edict_t *ent);

//
// g_player.c
//
void player_pain(edict_t *self, edict_t *other, float kick, int damage);
void player_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

//
// g_svcmds.c
//
void ServerCommand(void);
bool SV_FilterPacket(char *from);

//
// p_view.c
//
void ClientEndServerFrame(edict_t *ent);

//
// p_hud.c
//
void MoveClientToIntermission(edict_t *client);
void G_SetStats(edict_t *ent);
void G_SetSpectatorStats(edict_t *ent);
void G_CheckChaseStats(edict_t *ent);
void ValidateSelectedItem(edict_t *ent);
void DeathmatchScoreboardMessage(edict_t *client, edict_t *killer);

//
// p_weapon.c
//
void PlayerNoise(edict_t *who, vec3_t where, int type);
void Weapon_SetAnimationFrame(edict_t *ent, const weapon_animation_t *animation, int32_t frame);
void Weapon_SetAnimation(edict_t *ent, const weapon_animation_t *animation);
void Weapon_RunAnimation(edict_t *ent);
void P_ProjectSource(gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result);
void P_ProjectSource2(gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t up, vec3_t result);
bool Weapon_AmmoCheck(edict_t *ent);

// 20% chance of inspect after idle
#define WEAPON_RANDOM_INSPECT_CHANCE    0.8f

//
// m_move.c
//
bool M_CheckBottom(edict_t *ent);
bool M_walkmove(edict_t *ent, float yaw, float dist);
void M_MoveToGoal(edict_t *ent, float dist);
void M_ChangeYaw(edict_t *ent);

//
// g_phys.c
//
void G_RunEntity(edict_t *ent);
void G_InitAnimation(edict_t *ent);
void SV_Impact(edict_t *e1, trace_t *trace);

//
// g_main.c
//
void SaveClientData(void);
void FetchClientEntData(edict_t *ent);
edict_t *G_CreateEntityList(void);
void G_InitEntityList(edict_t *list);

//
// g_chase.c
//
void UpdateChaseCam(edict_t *ent);
void ChaseNext(edict_t *ent);
void ChasePrev(edict_t *ent);
void GetChaseTarget(edict_t *ent);

// NAC
#include "monsters/nac_monster.h"

//============================================================================

// client_t->anim_priority
#define ANIM_BASIC      0       // stand / run
#define ANIM_WAVE       1
#define ANIM_JUMP       2
#define ANIM_PAIN       3
#define ANIM_ATTACK     4
#define ANIM_DEATH      5
#define ANIM_REVERSE    6

// client data that stays across multiple level loads
typedef struct {
    char        userinfo[MAX_INFO_STRING];
    char        netname[16];
    int         hand;

    bool        connected;  // a loadgame will leave valid entities that
                            // just don't have a connection yet

    // values saved and restored from edicts when changing levels
    int         health;
    int         max_health;
    int         savedFlags;

    gitem_id_t  selected_item;
    int         inventory[ITEM_TOTAL];

    // ammo capacities
    int         max_nails;
    int         max_shells;
    int         max_rockets;
    int         max_grenades;
    int         max_cells;

    gitem_t     *weapon;
    gitem_t     *lastweapon;

    int         score;          // for calculating total unit score in coop games

    int         game_helpchanged;
    int         helpchanged;

    bool        spectator;      // client is a spectator
} client_persistant_t;

// client data that stays across deathmatch respawns
typedef struct {
    client_persistant_t coop_respawn;   // what to set client->pers to on a respawn
    gtime_t     entertime;         // level.time the client entered the game
    int         score;              // frags, etc
    vec3_t      cmd_angles;         // angles sent over in the last command

    bool        spectator;          // client is a spectator
} client_respawn_t;

// this structure is cleared on each PutClientInServer(),
// except for 'client->pers'
struct gclient_s {
    // known to server
    player_state_t  ps;             // communicated by server to clients
    int             ping;

    // private to game
    client_persistant_t pers;
    client_respawn_t    resp;
    pmove_state_t       old_pmove;  // for detecting out-of-pmove changes

    bool        showscores;         // set layout stat
    bool        showinventory;      // set layout stat
    bool        showhelp;
    bool        showhelpicon;

    gitem_id_t  ammo_index;

    int         buttons;
    int         oldbuttons;
    int         latched_buttons;

    bool        weapon_thunk;

    gitem_t     *newweapon;

    // sum up damage over an entire frame, so
    // shotgun blasts give a single big kick
    int         damage_armor;       // damage absorbed by armor
    int         damage_blood;       // damage taken out of health
    int         damage_knockback;   // impact damage
    vec3_t      damage_from;        // origin for vector calculation

    float       killer_yaw;         // when dead, look at killer

    const weapon_animation_t *weapanim[WEAPID_TOTAL];

    vec3_t      kick_angles;    // weapon kicks
    float       v_dmg_roll, v_dmg_pitch;
    gtime_t     v_dmg_time;    // damage kicks
    gtime_t     fall_time;
    float       fall_value;      // for view drop on fall
    float       damage_alpha;
    float       bonus_alpha;
    vec3_t      damage_blend;
    vec3_t      v_angle;            // aiming direction
    float       bobtime;            // so off-ground doesn't change it
    vec3_t      oldviewangles;
    vec3_t      oldvelocity;

    gtime_t     next_drown_time;
    int         old_waterlevel;
    int         breather_sound;

    // animation vars
    int         anim_end;
    int         anim_priority;
    bool        anim_duck;
    bool        anim_run;

    // powerup timers
    gtime_t     quad_time;
    gtime_t     invincible_time;
    gtime_t     enviro_time;

    int         weapon_sound;

    gtime_t     pickup_msg_time;

    gtime_t     flood_locktill_time;     // locked from talking
    gtime_t     flood_when_times[10];     // when messages were said
    int         flood_whenhead;     // head pointer for when said

    gtime_t     respawn_time;   // can respawn when time > this

    edict_t     *chase_target;      // player we are chasing
    bool        update_chase;       // need to update chase info?

    // N&C
    bool        axe_attack, can_charge_axe, can_release_charge;
    bool        inspect;
};

struct edict_s {
    entity_state_t  s;
    struct gclient_s    *client;    // NULL if not a player
                                    // the server expects the first part
                                    // of gclient_s to be a player_state_t
                                    // but the rest of it is opaque

    bool    inuse;
    int     linkcount;

    // FIXME: move these fields to a server private sv_entity_t

    int         areanum, areanum2;

    //================================

    int         svflags;
    vec3_t      mins, maxs;
    vec3_t      absmin, absmax, size;
    solid_t     solid;
    int         clipmask;
    edict_t     *owner;


    // DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
    // EXPECTS THE FIELDS IN THAT ORDER!

    //================================
    int         movetype;
    int         flags;

    char        *model, *model2, *model3, *model4;
    gtime_t     free_time;           // time when the object was freed

    //
    // only used locally in game, not by server
    //
    char        *message;
    char        *classname;
    int         spawnflags;

    gtime_t     timestamp;

    float       angle;          // set in qe3, -1 = up, -2 = down
    char        *target;
    char        *targetname;
    char        *killtarget;
    char        *team;
    char        *pathtarget;
    char        *deathtarget;
    char        *combattarget;
    edict_t     *target_ent;

    float       speed, accel, decel;
    vec3_t      movedir;
    vec3_t      pos1, pos2;

    vec3_t      velocity;
    vec3_t      avelocity;
    int         mass;
    gtime_t     air_finished_time;
    float       gravity;        // per entity gravity multiplier (1.0 is normal)
                                // use for lowgrav artifact, flares

    edict_t     *goalentity;
    edict_t     *movetarget;
    float       yaw_speed;
    float       ideal_yaw;

    gtime_t     nextthink;
    void        (*prethink)(edict_t *ent);
    void        (*think)(edict_t *self);
    void        (*blocked)(edict_t *self, edict_t *other);         // move to moveinfo?
    void        (*touch)(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);
    void        (*use)(edict_t *self, edict_t *other, edict_t *activator);
    void        (*pain)(edict_t *self, edict_t *other, float kick, int damage);
    void        (*die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

    gtime_t     touch_debounce_time;        // are all these legit?  do we need more/less of them?
    gtime_t     pain_debounce_time;
    gtime_t     damage_debounce_time;
    gtime_t     fly_sound_debounce_time;    // move to clientinfo
    gtime_t     last_move_time;

    int         health;
    int         max_health;
    int         gib_health;
    int         deadflag;
    gtime_t     show_hostile_time;

    char        *map;           // target_changelevel

    int         viewheight;     // height above origin where eyesight is determined
    int         takedamage;
    int         dmg;
    int         radius_dmg;
    float       dmg_radius;
    int         sounds;         // make this a spawntemp var?
    int         count;

    edict_t     *chain;
    edict_t     *enemy;
    edict_t     *oldenemy;
    edict_t     *activator;
    edict_t     *groundentity;
    int         groundentity_linkcount;
    edict_t     *teamchain;
    edict_t     *teammaster;

    edict_t     *mynoise;       // can go in client only
    edict_t     *mynoise2;

    int         noise_index;
    int         noise_index2;
    float       volume;
    float       attenuation;

    // timing variables
    float       wait;
    float       delay;          // before firing targets
    float       random;

    gtime_t     last_sound_time;

    int         watertype;
    int         waterlevel;

    vec3_t      move_origin;
    vec3_t      move_angles;

    // move this to clientinfo?
    int         light_level;

    int         style;          // also used as areaportal number

    gitem_t     *item;          // for bonus items

    // common data blocks
    moveinfo_t      moveinfo;
    monsterinfo_t   monsterinfo;

    // Paril - animated entity stuff
    struct {
        // entity keys
        int start, end;
        int frame_delay;
        bool animating, reset_on_trigger;
        char *target, *finished_target;
        int count;

        // state only
        bool is_active;
        int next_frame;
        int count_left;
    } anim;

    // Paril: switchable light style
    char        *style2;

    // radius key for map stuff
    float       radius;
    edict_t     *next_reverb;
};

// API wrappers

void SV_CenterPrint(edict_t *ent, const char *message);
void SV_CenterPrintf(edict_t *ent, const char *fmt, ...);
void SV_BroadcastPrint(client_print_type_t level, const char *message);
void SV_BroadcastPrintf(client_print_type_t level, const char *fmt, ...);
void SV_ClientPrint(edict_t *ent, client_print_type_t level, const char *message);
void SV_ClientPrintf(edict_t *ent, client_print_type_t level, const char *fmt, ...);
void SV_StartSound(edict_t *entity, int channel,
                   int soundindex, float volume,
                   float attenuation, int pitch_shift);
void SV_PositionedSound(const vec3_t origin, edict_t *entity, int channel,
                        int soundindex, float volume,
                        float attenuation, int pitch_shift);

int SV_ModelIndex(const char *name);
int SV_SoundIndex(const char *name);
int SV_ImageIndex(const char *name);

bool SV_InVis(const vec3_t p1, const vec3_t p2, vis_set_t vis, bool ignore_areas);
void SV_SetAreaPortalState(int portalnum, bool open);
bool SV_GetAreaPortalState(int portalnum);
bool SV_AreasConnected(int area1, int area2);
void SV_SetBrushModel(edict_t *ent, const char *model);
void SV_LinkEntity(edict_t *ent);
bool SV_EntityLinked(edict_t *ent);
void SV_UnlinkEntity(edict_t *ent);
trace_t SV_Trace(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
                 edict_t *passedict, int contentmask);
int SV_PointContents(const vec3_t p);
size_t SV_AreaEdicts(const vec3_t mins, const vec3_t maxs, edict_t **list, size_t maxcount, int areatype);
bool SV_EntityCollide(const vec3_t mins, const vec3_t maxs, edict_t *ent);

int Cmd_Argc(void);
char *Cmd_Argv(int arg);
char *Cmd_RawArgs(void);
char *Cmd_Args(void);

void Cbuf_AddText(const char *text);

typedef enum {
    TAG_COMMON,
    TAG_GAME,
    TAG_LEVEL,
    TAG_MODEL,
    TAG_MAX
} memtag_t;

void Z_Free(void *ptr);
void *Z_Realloc(void *ptr, size_t size);
void *Z_TagMalloc(size_t size, memtag_t tag) q_malloc;
void *Z_TagMallocz(size_t size, memtag_t tag) q_malloc;
char *Z_TagCopyString(const char *in, memtag_t tag) q_malloc;
void Z_FreeTags(memtag_t tag);

void SV_SetConfigString(uint32_t num, const char *string);
size_t SV_GetConfigString(uint32_t num, char *buffer, size_t len);

void SV_DropClient(edict_t *ent, const char *reason);
void SV_Multicast(const vec3_t origin, multicast_t to, bool reliable);
void SV_Unicast(edict_t *ent, bool reliable);
void SV_WriteChar(int c);
void SV_WriteByte(int c);
void SV_WriteShort(int c);
void SV_WriteEntity(edict_t *ent);
void SV_WriteLong(int c);
void SV_WriteString(const char *s);
void SV_WritePos(const vec3_t pos);
void SV_WriteDir(const vec3_t pos);
void SV_WriteAngle(float f);
void SV_WriteAngle16(float f);
void SV_WriteData(const void *data, size_t len);

void Pm_Move(pmove_t *pm);

void Cvar_Get(cvarRef_t *cvar, const char *var_name, const char *value, int flags);
bool Cvar_Update(cvarRef_t *cvar);
void Cvar_SetString(cvarRef_t *cvar, const char *value, bool force);
void Cvar_SetInteger(cvarRef_t *cvar, int value, bool force);
void Cvar_SetFloat(cvarRef_t *cvar, float value, bool force);

int FS_LoadFileEx(const char *path, void **buffer, unsigned flags, unsigned tag);

#define G_LoadFile(path, buf)  FS_LoadFileEx(path, buf, 0, TAG_GAME)
#define G_FreeFile(buf)        Z_Free(buf)

#define Cvar_Set(cvar, X, force) \
_Generic((X), \
    default: Cvar_SetInteger, \
    float: Cvar_SetFloat, \
    const char *: Cvar_SetString, \
    char *: Cvar_SetString \
)(cvar, X, force)