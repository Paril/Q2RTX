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

#ifndef PROTOCOL_H
#define PROTOCOL_H

//
// protocol.h -- communications protocols
//

#define MAX_MSGLEN  0x8000  // max length of a message, 32k

#define PROTOCOL_VERSION_NAC        667

//=========================================

#define UPDATE_BACKUP   16  // copies of entity_state_t to keep buffered
                            // must be power of two
#define UPDATE_MASK     (UPDATE_BACKUP - 1)

#define CMD_BACKUP      128 // allow a lot of command backups for very fast systems
                            // increased from 64
#define CMD_MASK        (CMD_BACKUP - 1)


#define SVCMD_BITS              5
#define SVCMD_MASK              ((1 << SVCMD_BITS) - 1)

#define FRAMENUM_BITS           27
#define FRAMENUM_MASK           ((1 << FRAMENUM_BITS) - 1)

#define SUPPRESSCOUNT_BITS      4
#define SUPPRESSCOUNT_MASK      ((1 << SUPPRESSCOUNT_BITS) - 1)

#define MAX_PARSE_ENTITIES      (MAX_PACKET_ENTITIES * UPDATE_BACKUP)
#define PARSE_ENTITIES_MASK     (MAX_PARSE_ENTITIES - 1)

#define MAX_PACKET_USERCMDS     32
#define MAX_PACKET_FRAMES       4

#define MAX_PACKET_STRINGCMDS   8
#define MAX_PACKET_USERINFOS    8

#define CS_BITMAP_BYTES         (MAX_CONFIGSTRINGS / 8) // 260
#define CS_BITMAP_LONGS         (CS_BITMAP_BYTES / 4)

//
// server to client
//
typedef enum {
    svc_bad,

    // these ops are known to the game dll
    svc_muzzleflash,
    svc_muzzleflash2,
    svc_temp_entity,
    svc_layout,
    svc_inventory,

    // the rest are private to the client and server
    svc_nop,
    svc_disconnect,
    svc_reconnect,
    svc_sound,                  // <see code>
    svc_print,                  // [byte] id [string] null terminated string
    svc_stufftext,              // [string] stuffed into client's console buffer
                                // should be \n terminated
    svc_serverdata,             // [long] protocol ...
    svc_configstring,           // [short] [string]
    svc_centerprint,            // [string] to put in center of the screen
    svc_download,               // [short] size [size bytes]
    svc_playerinfo,             // variable
    svc_frame,

    svc_zpacket,
    svc_zdownload,
    svc_gamestate,
    svc_setting,

    svc_ambient,

    svc_num_types
} svc_ops_t;

//==============================================

//
// client to server
//
typedef enum {
    clc_bad,
    clc_nop,
    clc_userinfo,           // [userinfo string]
    clc_stringcmd,          // [string] message
    clc_ambient,

    // r1q2 specific operations
    clc_setting,

    // q2pro specific operations
    clc_move_nodelta = 10,
    clc_move_batched,
    clc_userinfo_delta
} clc_ops_t;

//==============================================

// player_state_t communication

#define PS_M_TYPE           (1<<0)
#define PS_M_ORIGIN         (1<<1)
#define PS_M_VELOCITY       (1<<2)
#define PS_M_TIME           (1<<3)
#define PS_M_FLAGS          (1<<4)
#define PS_M_GRAVITY        (1<<5)
#define PS_M_DELTA_ANGLES   (1<<6)

#define PS_VIEWOFFSET       (1<<7)
#define PS_VIEWANGLES       (1<<8)
#define PS_KICKANGLES       (1<<9)
#define PS_BLEND            (1<<10)
#define PS_FOV              (1<<11)
#define PS_RDFLAGS          (1<<12)
#define PS_GUNS             (1<<13)
#define PS_REVERB           (1<<14)

#define PS_BITS             15
#define PS_MASK             ((1<<PS_BITS)-1)

#define EPS_M_VELOCITY2     (1<<0)
#define EPS_M_ORIGIN2       (1<<1)
#define EPS_VIEWANGLE2      (1<<2)
#define EPS_STATS           (1<<3)
#define EPS_CLIENTNUM       (1<<4)

#define EPS_BITS            5
#define EPS_MASK            ((1<<EPS_BITS)-1)

#define GB_INDEX0           (1<<0)
#define GB_INDEX1           (1<<1)
#define GB_FRAME0           (1<<2)
#define GB_FRAME1           (1<<3)
#define GB_SPIN0            (1<<4)
#define GB_SPIN1            (1<<5)

#define GB_BITS             6
#define GB_MASK             ((1<<GB_BITS)-1)

//==============================================

// user_cmd_t communication

// ms and light always sent, the others are optional
#define CM_ANGLE1   (1<<0)
#define CM_ANGLE2   (1<<1)
#define CM_ANGLE3   (1<<2)
#define CM_FORWARD  (1<<3)
#define CM_SIDE     (1<<4)
#define CM_UP       (1<<5)
#define CM_BUTTONS  (1<<6)
#define CM_IMPULSE  (1<<7)

// r1q2 button byte hacks
#define BUTTON_MASK     (BUTTON_ATTACK|BUTTON_USE|BUTTON_ANY)
#define BUTTON_FORWARD  4
#define BUTTON_SIDE     8
#define BUTTON_UP       16
#define BUTTON_ANGLE1   32
#define BUTTON_ANGLE2   64

//==============================================

// a sound without an ent or pos will be a local only sound
#define SND_VOLUME          (1<<0)  // a byte
#define SND_ATTENUATION     (1<<1)  // a byte
#define SND_POS             (1<<2)  // three coordinates
#define SND_ENT             (1<<3)  // a short 0-2: channel, 3-12: entity
#define SND_PITCH           (1<<4)  // a byte

#define DEFAULT_SOUND_PACKET_VOLUME         1.0f
#define DEFAULT_SOUND_PACKET_ATTENUATION    1.0f

//==============================================

// entity_state_t communication

// try to pack the common update flags into the first byte
#define U_ORIGIN1   (1<<0)
#define U_ORIGIN2   (1<<1)
#define U_ANGLE2    (1<<2)
#define U_ANGLE3    (1<<3)
#define U_FRAME8    (1<<4)        // frame is a byte
#define U_EVENT     (1<<5)
#define U_REMOVE    (1<<6)        // REMOVE this entity, don't add it
#define U_MOREBITS1 (1<<7)        // read one additional byte

// second byte
#define U_NUMBER16  (1<<8)        // NUMBER8 is implicit if not set
#define U_ORIGIN3   (1<<9)
#define U_ANGLE1    (1<<10)
#define U_MODEL     (1<<11)
#define U_RENDERFX8 (1<<12)        // fullbright, etc
#define U_EFFECTS8  (1<<13)        // autorotate, trails, etc
#define U_SKIN8     (1<<14)
#define U_MOREBITS2 (1<<15)        // read one additional byte

// third byte
#define U_FRAME16       (1<<16)     // frame is a short
#define U_RENDERFX16    (1<<17)     // 8 + 16 = 32
#define U_EFFECTS16     (1<<18)     // 8 + 16 = 32
#define U_MODEL2        (1<<19)     // weapons, flags, etc
#define U_MODEL3        (1<<20)
#define U_MODEL4        (1<<21)
#define U_OLDORIGIN     (1<<22)     // FIXME: get rid of this
#define U_MOREBITS3     (1<<23)     // read one additional byte

// fourth byte
#define U_SKIN16        (1<<24)
#define U_SOUND         (1<<25)
#define U_SOLID         (1<<26)
#define U_SOUNDPITCH    (1<<27)

// ==============================================================

#define CLIENTNUM_NONE        (MAX_CLIENTS - 1)
#define CLIENTNUM_RESERVED    (MAX_CLIENTS - 1)

// a packed bbox will never create this value
#define BBOX_BMODEL     0xffffff

typedef enum {
    // r1q2 specific
    CLS_NOGUN,
    CLS_RECORDING,
    CLS_PLAYERUPDATES,

    // q2pro specific
    CLS_NOPREDICT,

    CLS_MAX
} clientSetting_t;

// q2pro frame flags sent by the server
// only SUPPRESSCOUNT_BITS can be used
#define FF_SUPPRESSED   (1<<0)
#define FF_CLIENTDROP   (1<<1)
#define FF_CLIENTPRED   (1<<2)
#define FF_RESERVED     (1<<3)

#endif // PROTOCOL_H
