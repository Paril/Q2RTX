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

#include "shared/shared.h"
#include "common/msg.h"
#include "common/protocol.h"
#include "common/sizebuf.h"
#include "common/math.h"
#include "common/intreadwrite.h"

/*
==============================================================================

            MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/

sizebuf_t   msg_write;
byte        msg_write_buffer[MAX_MSGLEN];

sizebuf_t   msg_read;
byte        msg_read_buffer[MAX_MSGLEN];

const entity_state_t   nullEntityState;
const player_state_t   nullPlayerState;
const usercmd_t        nullUserCmd;

/*
=============
MSG_Init

Initialize default buffers, clearing allow overflow/underflow flags.

This is the only place where writing buffer is initialized. Writing buffer is
never allowed to overflow.

Reading buffer is reinitialized in many other places. Reinitializing will set
the allow underflow flag as appropriate.
=============
*/
void MSG_Init(void)
{
    SZ_TagInit(&msg_read, msg_read_buffer, MAX_MSGLEN, SZ_MSG_READ);
    SZ_TagInit(&msg_write, msg_write_buffer, MAX_MSGLEN, SZ_MSG_WRITE);
}


/*
==============================================================================

            WRITING

==============================================================================
*/

/*
=============
MSG_BeginWriting
=============
*/
void MSG_BeginWriting(void)
{
    msg_write.cursize = 0;
    msg_write.bitpos = 0;
    msg_write.overflowed = false;
}

/*
=============
MSG_WriteChar
=============
*/
void MSG_WriteChar(int c)
{
    byte    *buf;

#ifdef PARANOID
    Q_assert(c >= -128 && c <= 127);
#endif

    buf = SZ_GetSpace(&msg_write, 1);
    buf[0] = c;
}

/*
=============
MSG_WriteByte
=============
*/
void MSG_WriteByte(int c)
{
    byte    *buf;

#ifdef PARANOID
    Q_assert(c >= 0 && c <= 255);
#endif

    buf = SZ_GetSpace(&msg_write, 1);
    buf[0] = c;
}

/*
=============
MSG_WriteShort
=============
*/
void MSG_WriteShort(int c)
{
    byte    *buf;

#ifdef PARANOID
    Q_assert(c >= -0x8000 && c <= 0x7fff);
#endif

    buf = SZ_GetSpace(&msg_write, 2);
    WL16(buf, c);
}

/*
=============
MSG_WriteLong
=============
*/
void MSG_WriteLong(int c)
{
    byte    *buf;

    buf = SZ_GetSpace(&msg_write, 4);
    WL32(buf, c);
}

/*
=============
MSG_WriteLong
=============
*/
void MSG_WriteFloat(float c)
{
    byte    *buf;
    union
    {
        float v;
        struct { uint8_t b[4]; };
    } f2i = { c };

    buf = SZ_GetSpace(&msg_write, 4);
    buf[0] = f2i.b[0];
    buf[1] = f2i.b[1];
    buf[2] = f2i.b[2];
    buf[3] = f2i.b[3];
}

void MSG_WriteVarInt(uint64_t c)
{
    byte ba[8] = { 0 };
    byte byteOffset = 0;
    uint64_t e = c;

    do {
        if (byteOffset == 8) {
            Com_Error(ERR_FATAL, "MSG_WriteVarInt: range error");
        }
        byte b = c & 0b01111111;
        c >>= 7;
        if (c) {
            b |= 0b10000000;
        }
        ba[byteOffset++] = b;
    } while (c != 0);

    Com_Printf("Compressed %llu into %u bytes\n", e, byteOffset);

    byte *buf = SZ_GetSpace(&msg_write, byteOffset);
    memcpy(buf, ba, byteOffset);
}

uint64_t MSG_ReadVarInt(void)
{
    uint64_t result = 0;
    byte shift = 0;

    while (true) {
        byte b = MSG_ReadByte();
        result |= (b & 0b01111111ull) << shift;
        if (!(b & 0b10000000)) {
            break;
        }
        shift += 7;
    }

    return result;
}

/*
=============
MSG_WriteString
=============
*/
void MSG_WriteString(const char *string)
{
    size_t length;

    if (!string) {
        MSG_WriteByte(0);
        return;
    }

    length = strlen(string);
    if (length >= MAX_NET_STRING) {
        Com_WPrintf("%s: overflow: %zu chars", __func__, length);
        MSG_WriteByte(0);
        return;
    }

    MSG_WriteData(string, length + 1);
}

/*
=============
MSG_WriteCoord
=============
*/
void MSG_WriteCoord(float f)
{
    MSG_WriteLong(COORD2SHORT(f));
}

/*
=============
MSG_WritePos
=============
*/
void MSG_WritePos(const vec3_t pos)
{
    MSG_WriteCoord(pos[0]);
    MSG_WriteCoord(pos[1]);
    MSG_WriteCoord(pos[2]);
}

#define CoordCompare(x, y) \
    (COORD2SHORT((x)[0]) == COORD2SHORT((y)[0]) && \
    COORD2SHORT((x)[1]) == COORD2SHORT((y)[1]) && \
    COORD2SHORT((x)[2]) == COORD2SHORT((y)[2]))

/*
=============
MSG_WriteAngle
=============
*/

#define ANGLE2BYTE(x)   ((int)((x)*256.0f/360)&255)
#define BYTE2ANGLE(x)   ((x)*(360.0f/256))

void MSG_WriteAngle(float f)
{
    MSG_WriteByte(ANGLE2BYTE(f));
}

void MSG_WriteAngle16(float f)
{
    MSG_WriteShort(ANGLE2SHORT(f));
}

#if USE_CLIENT

/*
=============
MSG_WriteBits
=============
*/
void MSG_WriteBits(int value, int bits)
{
    int i;
    size_t bitpos;

    if (bits == 0 || bits < -31 || bits > 32) {
        Com_Errorf(ERR_FATAL, "MSG_WriteBits: bad bits: %d", bits);
    }

    if (msg_write.maxsize - msg_write.cursize < 4) {
        Com_Error(ERR_FATAL, "MSG_WriteBits: overflow");
    }

    if (bits < 0) {
        bits = -bits;
    }

    bitpos = msg_write.bitpos;
    if ((bitpos & 7) == 0) {
        // optimized case
        switch (bits) {
        case 8:
            MSG_WriteByte(value);
            return;
        case 16:
            MSG_WriteShort(value);
            return;
        case 32:
            MSG_WriteLong(value);
            return;
        default:
            break;
        }
    }
    for (i = 0; i < bits; i++, bitpos++) {
        if ((bitpos & 7) == 0) {
            msg_write.data[bitpos >> 3] = 0;
        }
        msg_write.data[bitpos >> 3] |= (value & 1) << (bitpos & 7);
        value >>= 1;
    }
    msg_write.bitpos = bitpos;
    msg_write.cursize = (bitpos + 7) >> 3;
}

/*
=============
MSG_WriteDeltaUsercmd
=============
*/
int MSG_WriteDeltaUsercmd(const usercmd_t *from,
                          const usercmd_t *cmd)
{
    int     bits, delta, count;

    if (!from) {
        from = &nullUserCmd;
    }

//
// send the movement message
//
    bits = 0;
    if (ANGLE2SHORT(cmd->angles[0]) != ANGLE2SHORT(from->angles[0]))
        bits |= CM_ANGLE1;
    if (ANGLE2SHORT(cmd->angles[1]) != ANGLE2SHORT(from->angles[1]))
        bits |= CM_ANGLE2;
    if (ANGLE2SHORT(cmd->angles[2]) != ANGLE2SHORT(from->angles[2]))
        bits |= CM_ANGLE3;
    if (cmd->forwardmove != from->forwardmove)
        bits |= CM_FORWARD;
    if (cmd->sidemove != from->sidemove)
        bits |= CM_SIDE;
    if (cmd->upmove != from->upmove)
        bits |= CM_UP;
    if (cmd->buttons != from->buttons)
        bits |= CM_BUTTONS;
    if (cmd->msec != from->msec)
        bits |= CM_IMPULSE;

    if (!bits) {
        MSG_WriteBits(0, 1);
        return 0;
    }

    MSG_WriteBits(1, 1);
    MSG_WriteBits(bits, 8);

    if (bits & CM_ANGLE1) {
        delta = ANGLE2SHORT(cmd->angles[0] - from->angles[0]);
        if (delta >= -128 && delta <= 127) {
            MSG_WriteBits(1, 1);
            MSG_WriteBits(delta, -8);
        } else {
            MSG_WriteBits(0, 1);
            MSG_WriteBits(ANGLE2SHORT(cmd->angles[0]), -16);
        }
    }
    if (bits & CM_ANGLE2) {
        delta = ANGLE2SHORT(cmd->angles[1] - from->angles[1]);
        if (delta >= -128 && delta <= 127) {
            MSG_WriteBits(1, 1);
            MSG_WriteBits(delta, -8);
        } else {
            MSG_WriteBits(0, 1);
            MSG_WriteBits(ANGLE2SHORT(cmd->angles[1]), -16);
        }
    }
    if (bits & CM_ANGLE3) {
        MSG_WriteBits(ANGLE2SHORT(cmd->angles[2]), -16);
    }

    count = -10;

    if (bits & CM_FORWARD) {
        MSG_WriteBits(cmd->forwardmove, count);
    }
    if (bits & CM_SIDE) {
        MSG_WriteBits(cmd->sidemove, count);
    }
    if (bits & CM_UP) {
        MSG_WriteBits(cmd->upmove, count);
    }

    if (bits & CM_BUTTONS) {
        int buttons = (cmd->buttons & 3) | (cmd->buttons >> 5);
        MSG_WriteBits(buttons, 3);
    }
    if (bits & CM_IMPULSE) {
        MSG_WriteBits(cmd->msec, 8);
    }

    return bits;
}

#endif // USE_CLIENT

void MSG_WriteDir(const vec3_t dir)
{
    MSG_WriteByte(DirToByte(dir));
}

uint32_t MSG_EntityWillWrite(const entity_state_t *from,
                             const entity_state_t *to,
                             msgEsFlags_t         flags)
{
    uint32_t bits;

    // send an update
    bits = 0;

    if (!(flags & MSG_ES_FIRSTPERSON)) {
        if (COORD2SHORT(to->origin[0]) != COORD2SHORT(from->origin[0]))
            bits |= U_ORIGIN1;
        if (COORD2SHORT(to->origin[1]) != COORD2SHORT(from->origin[1]))
            bits |= U_ORIGIN2;
        if (COORD2SHORT(to->origin[2]) != COORD2SHORT(from->origin[2]))
            bits |= U_ORIGIN3;

        if (ANGLE2SHORT(to->angles[0]) != ANGLE2SHORT(from->angles[0]))
            bits |= U_ANGLE1;
        if (ANGLE2SHORT(to->angles[1]) != ANGLE2SHORT(from->angles[1]))
            bits |= U_ANGLE2;
        if (ANGLE2SHORT(to->angles[2]) != ANGLE2SHORT(from->angles[2]))
            bits |= U_ANGLE3;

        if ((flags & MSG_ES_NEWENTITY) && !CoordCompare(to->old_origin, from->origin))
            bits |= U_OLDORIGIN;
    }

    if (to->skinnum != from->skinnum) {
        if (to->skinnum & 0xffff0000)
            bits |= U_SKIN8 | U_SKIN16;
        else if (to->skinnum & 0x0000ff00)
            bits |= U_SKIN16;
        else
            bits |= U_SKIN8;
    }

    if (to->frame != from->frame) {
        if (to->frame & 0xff00)
            bits |= U_FRAME16;
        else
            bits |= U_FRAME8;
    }

    if (to->effects != from->effects) {
        if (to->effects & 0xffff0000)
            bits |= U_EFFECTS8 | U_EFFECTS16;
        else if (to->effects & 0x0000ff00)
            bits |= U_EFFECTS16;
        else
            bits |= U_EFFECTS8;
    }

    if (to->renderfx != from->renderfx) {
        if (to->renderfx & 0xffff0000)
            bits |= U_RENDERFX8 | U_RENDERFX16;
        else if (to->renderfx & 0x0000ff00)
            bits |= U_RENDERFX16;
        else
            bits |= U_RENDERFX8;
    }

    if (to->bbox != from->bbox)
        bits |= U_SOLID;

    // event is not delta compressed, just 0 compressed
    if (to->event)
        bits |= U_EVENT;

    if (to->modelindex != from->modelindex)
        bits |= U_MODEL;
    if (to->modelindex2 != from->modelindex2)
        bits |= U_MODEL2;
    if (to->modelindex3 != from->modelindex3)
        bits |= U_MODEL3;
    if (to->modelindex4 != from->modelindex4)
        bits |= U_MODEL4;

    if (to->sound != from->sound)
        bits |= U_SOUND;

    if (to->renderfx & RF_FRAMELERP) {
        bits |= U_OLDORIGIN;
    } else if (to->renderfx & RF_MASK_BEAMLIKE) {
        if (!VectorCompare(to->old_origin, from->old_origin))
            bits |= U_OLDORIGIN;
    }

    if (to->sound_pitch != from->sound_pitch)
        bits |= U_SOUNDPITCH;

    return bits;
}

void MSG_WriteDeltaEntity(const entity_state_t *from,
    const entity_state_t *to,
    msgEsFlags_t         flags)
{
    uint32_t bits = MSG_EntityWillWrite(from, to, flags);

    //
    // write the message
    //
    if (!bits && !(flags & MSG_ES_FORCE))
        return;     // nothing to send!

    if (to->number & 0xff00 || (flags & MSG_ES_AMBIENT))
        bits |= U_NUMBER16;     // number8 is implicit otherwise

    if (bits & 0xff000000)
        bits |= U_MOREBITS3 | U_MOREBITS2 | U_MOREBITS1;
    else if (bits & 0x00ff0000)
        bits |= U_MOREBITS2 | U_MOREBITS1;
    else if (bits & 0x0000ff00)
        bits |= U_MOREBITS1;

    MSG_WriteByte(bits & 255);

    if (bits & 0xff000000) {
        MSG_WriteByte((bits >> 8) & 255);
        MSG_WriteByte((bits >> 16) & 255);
        MSG_WriteByte((bits >> 24) & 255);
    } else if (bits & 0x00ff0000) {
        MSG_WriteByte((bits >> 8) & 255);
        MSG_WriteByte((bits >> 16) & 255);
    } else if (bits & 0x0000ff00) {
        MSG_WriteByte((bits >> 8) & 255);
    }

    //----------

    if (bits & U_NUMBER16)
        MSG_WriteShort(to->number);
    else
        MSG_WriteByte(to->number);

    if (bits & U_MODEL)
        MSG_WriteByte(to->modelindex);
    if (bits & U_MODEL2)
        MSG_WriteByte(to->modelindex2);
    if (bits & U_MODEL3)
        MSG_WriteByte(to->modelindex3);
    if (bits & U_MODEL4)
        MSG_WriteByte(to->modelindex4);

    if (bits & U_FRAME8)
        MSG_WriteByte(to->frame);
    else if (bits & U_FRAME16)
        MSG_WriteShort(to->frame);

    if ((bits & (U_SKIN8 | U_SKIN16)) == (U_SKIN8 | U_SKIN16))  //used for laser colors
        MSG_WriteLong(to->skinnum);
    else if (bits & U_SKIN8)
        MSG_WriteByte(to->skinnum);
    else if (bits & U_SKIN16)
        MSG_WriteShort(to->skinnum);

    if ((bits & (U_EFFECTS8 | U_EFFECTS16)) == (U_EFFECTS8 | U_EFFECTS16))
        MSG_WriteLong(to->effects);
    else if (bits & U_EFFECTS8)
        MSG_WriteByte(to->effects);
    else if (bits & U_EFFECTS16)
        MSG_WriteShort(to->effects);

    if ((bits & (U_RENDERFX8 | U_RENDERFX16)) == (U_RENDERFX8 | U_RENDERFX16))
        MSG_WriteLong(to->renderfx);
    else if (bits & U_RENDERFX8)
        MSG_WriteByte(to->renderfx);
    else if (bits & U_RENDERFX16)
        MSG_WriteShort(to->renderfx);

    if (bits & U_ORIGIN1)
        MSG_WriteCoord(to->origin[0]);
    if (bits & U_ORIGIN2)
        MSG_WriteCoord(to->origin[1]);
    if (bits & U_ORIGIN3)
        MSG_WriteCoord(to->origin[2]);

    if (bits & U_ANGLE1)
        MSG_WriteAngle16(to->angles[0]);
    if (bits & U_ANGLE2)
        MSG_WriteAngle16(to->angles[1]);
    if (bits & U_ANGLE3)
        MSG_WriteAngle16(to->angles[2]);

    if (bits & U_OLDORIGIN)
        MSG_WritePos(to->old_origin);

    if (bits & U_SOUND)
        MSG_WriteByte(to->sound);
    if (bits & U_EVENT)
        MSG_WriteByte(to->event);
    if (bits & U_SOLID)
        MSG_WriteLong(to->bbox);

    if (bits & U_SOUNDPITCH)
        MSG_WriteChar(to->sound_pitch);
}

void MSG_WriteDeltaPacketEntity(const entity_state_t *from,
    const entity_state_t *to,
    msgEsFlags_t         flags)
{
    if (!to) {
        if (!from)
            Com_Errorf(ERR_DROP, "%s: NULL", __func__);

        if (from->number < 1 || from->number >= MAX_PACKET_ENTITIES)
            Com_Errorf(ERR_DROP, "%s: bad number: %d", __func__, from->number);

        // remove entity
        uint32_t bits = U_REMOVE;
        if (from->number & 0xff00)
            bits |= U_NUMBER16 | U_MOREBITS1;

        MSG_WriteByte(bits & 255);
        if (bits & 0x0000ff00)
            MSG_WriteByte((bits >> 8) & 255);

        if (bits & U_NUMBER16)
            MSG_WriteShort(from->number);
        else
            MSG_WriteByte(from->number);

        return;
    }

    if (to->number < 1 || to->number >= MAX_PACKET_ENTITIES)
        Com_Errorf(ERR_DROP, "%s: bad number: %d", __func__, to->number);

    if (!from)
        from = &nullEntityState;

    MSG_WriteDeltaEntity(from, to, flags);
}

void MSG_WriteDeltaAmbientEntity(const entity_state_t *from,
    const entity_state_t *to,
    msgEsFlags_t         flags)
{
    if (!to || !from) {
        Com_Errorf(ERR_DROP, "%s: NULL", __func__);
    }

    if (to->number < OFFSET_AMBIENT_ENTITIES || to->number >= OFFSET_PRIVATE_ENTITIES)
        Com_Errorf(ERR_DROP, "%s: bad number: %d", __func__, to->number);

    if (!from)
        from = &nullEntityState;

    MSG_WriteDeltaEntity(from, to, flags | MSG_ES_AMBIENT);
}

static void write_baseline(entity_state_t *base, msgEsFlags_t esFlags)
{
    MSG_WriteDeltaPacketEntity(NULL, base, esFlags | MSG_ES_FORCE);
}

static void write_ambient(entity_state_t *base, msgEsFlags_t esFlags)
{
    MSG_WriteDeltaAmbientEntity(&nullEntityState, base, esFlags | MSG_ES_FORCE);
}

void MSG_WriteGamestate(char *configstrings, entity_state_t *baselines, size_t num_baselines, entity_state_t *ambients, uint16_t num_ambients, uint8_t ambient_state_id, msgEsFlags_t esFlags)
{
    entity_state_t  *base;
    int         i;
    size_t      length;
    char        *string;

    MSG_WriteByte(svc_gamestate);

    // write configstrings
    string = (char *) configstrings;
    for (i = 0; i < MAX_CONFIGSTRINGS; i++, string += MAX_QPATH) {
        if (!string[0]) {
            continue;
        }
        length = strlen(string);
        if (length > MAX_QPATH) {
            length = MAX_QPATH;
        }

        MSG_WriteShort(i);
        MSG_WriteData(string, length);
        MSG_WriteByte(0);
    }
    MSG_WriteShort(MAX_CONFIGSTRINGS);   // end of configstrings

    // write baselines
    for (i = 0, base = baselines; i < num_baselines; i++, base++) {
        if (base->number) {
            write_baseline(base, esFlags);
        }
    }
    MSG_WriteShort(0);   // end of baselines

    // write ambients
    for (i = 0, base = ambients; i < num_ambients; i++, base++) {
        if (base->number) {
            write_ambient(base, esFlags);
        }
    }
    MSG_WriteByte(0);
    MSG_WriteShort(OFFSET_PRIVATE_ENTITIES); // end of ambients
    MSG_WriteByte(ambient_state_id); // sync ambient ID
    MSG_WriteShort(num_ambients);
}

static inline int OFFSET2SHORT(float x)
{
    return FLOAT2COMPRESS(x, 1024.f);
}

static inline void MSG_WriteOffset(vec3_t v)
{
    MSG_WriteShort(OFFSET2SHORT(v[0]));
    MSG_WriteShort(OFFSET2SHORT(v[1]));
    MSG_WriteShort(OFFSET2SHORT(v[2]));
}

#define OffsetCompare(x, y) \
    (OFFSET2SHORT((x)[0]) == OFFSET2SHORT((y)[0]) && \
    OFFSET2SHORT((x)[1]) == OFFSET2SHORT((y)[1]) && \
    OFFSET2SHORT((x)[2]) == OFFSET2SHORT((y)[2]))

static inline float SHORT2OFFSET(int x)
{
    return COMPRESS2FLOAT(x, 1024.f);
}

static inline void MSG_ReadOffset(vec3_t v)
{
    v[0] = SHORT2OFFSET(MSG_ReadShort());
    v[1] = SHORT2OFFSET(MSG_ReadShort());
    v[2] = SHORT2OFFSET(MSG_ReadShort());
}

static inline int BLEND2BYTE(float x)
{
    return clamp(x, 0, 1) * 255;
}

#define BlendCompare(x, y) \
    (BLEND2BYTE((x)[0]) == BLEND2BYTE((y)[0]) && \
    BLEND2BYTE((x)[1]) == BLEND2BYTE((y)[1]) && \
    BLEND2BYTE((x)[2]) == BLEND2BYTE((y)[2]) && \
    BLEND2BYTE((x)[3]) == BLEND2BYTE((y)[3]))

static inline float BYTE2BLEND(int x)
{
    return x / 255.0f;
}

int MSG_WriteDeltaPlayerstate(const player_state_t    *from,
                              player_state_t          *to,
                              msgPsFlags_t            flags)
{
    int     i;
    int     eflags;
    int     statbits;

    if (!to)
        Com_Errorf(ERR_DROP, "%s: NULL", __func__);

    if (!from)
        from = &nullPlayerState;

    //
    // write it
    //
    uint16_t *pflags = SZ_GetSpace(&msg_write, 2);
    *pflags = 0;

    eflags = 0;

    if (to->pmove.pm_type != from->pmove.pm_type) {
        *pflags |= PS_M_TYPE;
        MSG_WriteByte(to->pmove.pm_type);
    }

    if (COORD2SHORT(to->pmove.origin[0]) != COORD2SHORT(from->pmove.origin[0]) ||
        COORD2SHORT(to->pmove.origin[1]) != COORD2SHORT(from->pmove.origin[1]) ||
        to->pmove.pm_type != from->pmove.pm_type) {
        *pflags |= PS_M_ORIGIN;
        if (to->pmove.pm_type == PM_SPECTATOR)
        {
            MSG_WriteFloat(to->pmove.origin[0]);
            MSG_WriteFloat(to->pmove.origin[1]);
        }
        else
        {
            MSG_WriteCoord(to->pmove.origin[0]);
            MSG_WriteCoord(to->pmove.origin[1]);
        }
    }

    if (COORD2SHORT(to->pmove.origin[2]) != COORD2SHORT(from->pmove.origin[2]) ||
        to->pmove.pm_type != from->pmove.pm_type) {
        eflags |= EPS_M_ORIGIN2;
        if (to->pmove.pm_type == PM_SPECTATOR)
            MSG_WriteFloat(to->pmove.origin[2]);
        else
            MSG_WriteCoord(to->pmove.origin[2]);
    }

    if (!(flags & MSG_PS_IGNORE_PREDICTION)) {
        if (COORD2SHORT(to->pmove.velocity[0]) != COORD2SHORT(from->pmove.velocity[0]) ||
            COORD2SHORT(to->pmove.velocity[1]) != COORD2SHORT(from->pmove.velocity[1]) ||
            to->pmove.pm_type != from->pmove.pm_type) {
            *pflags |= PS_M_VELOCITY;
            if (to->pmove.pm_type == PM_SPECTATOR)
            {
                MSG_WriteFloat(to->pmove.velocity[0]);
                MSG_WriteFloat(to->pmove.velocity[1]);
            }
            else
            {
                MSG_WriteCoord(to->pmove.velocity[0]);
                MSG_WriteCoord(to->pmove.velocity[1]);
            }
        }

        if (COORD2SHORT(to->pmove.velocity[2]) != COORD2SHORT(from->pmove.velocity[2]) ||
            to->pmove.pm_type != from->pmove.pm_type) {
            eflags |= EPS_M_VELOCITY2;
            if (to->pmove.pm_type == PM_SPECTATOR)
                MSG_WriteFloat(to->pmove.velocity[2]);
            else
                MSG_WriteCoord(to->pmove.velocity[2]);
        }

        if (to->pmove.pm_time != from->pmove.pm_time) {
            *pflags |= PS_M_TIME;
            MSG_WriteByte(to->pmove.pm_time);
        }

        if (to->pmove.pm_flags != from->pmove.pm_flags) {
            *pflags |= PS_M_FLAGS;
            MSG_WriteByte(to->pmove.pm_flags);
        }

        if ((int16_t) to->pmove.gravity != (int16_t) from->pmove.gravity) {
            *pflags |= PS_M_GRAVITY;
            MSG_WriteShort(to->pmove.gravity);
        }
    } else {
        // save previous state
        VectorCopy(from->pmove.velocity, to->pmove.velocity);
        to->pmove.pm_time = from->pmove.pm_time;
        to->pmove.pm_flags = from->pmove.pm_flags;
        to->pmove.gravity = from->pmove.gravity;
    }

    if (!(flags & MSG_PS_IGNORE_DELTAANGLES)) {
        if (ANGLE2SHORT(from->pmove.delta_angles[0]) != ANGLE2SHORT(to->pmove.delta_angles[0]) ||
            ANGLE2SHORT(from->pmove.delta_angles[1]) != ANGLE2SHORT(to->pmove.delta_angles[1]) ||
            ANGLE2SHORT(from->pmove.delta_angles[2]) != ANGLE2SHORT(to->pmove.delta_angles[2])) {
            *pflags |= PS_M_DELTA_ANGLES;
            MSG_WriteAngle16(to->pmove.delta_angles[0]);
            MSG_WriteAngle16(to->pmove.delta_angles[1]);
            MSG_WriteAngle16(to->pmove.delta_angles[2]);
        }
    } else {
        // save previous state
        VectorCopy(from->pmove.delta_angles, to->pmove.delta_angles);
    }

    if (!OffsetCompare(from->viewoffset, to->viewoffset)) {
        *pflags |= PS_VIEWOFFSET;
        MSG_WriteOffset(to->viewoffset);
    }

    if (!(flags & MSG_PS_IGNORE_VIEWANGLES)) {
        if (ANGLE2SHORT(from->viewangles[0]) != ANGLE2SHORT(to->viewangles[0]) ||
            ANGLE2SHORT(from->viewangles[1]) != ANGLE2SHORT(to->viewangles[1])) {
            *pflags |= PS_VIEWANGLES;
            MSG_WriteAngle16(to->viewangles[0]);
            MSG_WriteAngle16(to->viewangles[1]);
        }

        if (ANGLE2SHORT(from->viewangles[2]) != ANGLE2SHORT(to->viewangles[2])) {
            eflags |= EPS_VIEWANGLE2;
            MSG_WriteAngle16(to->viewangles[2]);
        }
    } else {
        // save previous state
        VectorCopy(from->viewangles, to->viewangles);
    }

    if (!OffsetCompare(from->kick_angles, to->kick_angles)) {
        *pflags |= PS_KICKANGLES;
        MSG_WriteOffset(to->kick_angles);
    }

    if (!(flags & MSG_PS_IGNORE_BLEND)) {
        if (!BlendCompare(from->blend, to->blend)) {
            *pflags |= PS_BLEND;
            MSG_WriteByte(BLEND2BYTE(to->blend[0]));
            MSG_WriteByte(BLEND2BYTE(to->blend[1]));
            MSG_WriteByte(BLEND2BYTE(to->blend[2]));
            MSG_WriteByte(BLEND2BYTE(to->blend[3]));
        }
    } else {
        // save previous state
        Vector4Copy(from->blend, to->blend);
    }

    if ((uint8_t) from->fov != (uint8_t) to->fov) {
        *pflags |= PS_FOV;
        MSG_WriteByte(to->fov);
    }

    if (to->rdflags != from->rdflags) {
        *pflags |= PS_RDFLAGS;
        MSG_WriteByte(to->rdflags);
    }

    int gunflags = 0;

    if (!(flags & MSG_PS_IGNORE_GUNINDEX)) {
        if (to->gun[0].index != from->gun[0].index) {
            gunflags |= GB_INDEX0;
        }
        if (to->gun[1].index != from->gun[1].index) {
            gunflags |= GB_INDEX1;
        }
    } else {
        // save previous state
        to->gun[0].index = from->gun[0].index;
        to->gun[1].index = from->gun[1].index;
    }

    if (!(flags & MSG_PS_IGNORE_GUNFRAMES)) {
        if (to->gun[0].frame != from->gun[0].frame) {
            gunflags |= GB_FRAME0;
        }
        if (to->gun[1].frame != from->gun[1].frame) {
            gunflags |= GB_FRAME1;
        }
        if (FLOAT2COMPRESS(to->gun[0].spin, 2048) != FLOAT2COMPRESS(from->gun[0].spin, 2048)) {
            gunflags |= GB_SPIN0;
        }
        if (FLOAT2COMPRESS(to->gun[1].spin, 2048) != FLOAT2COMPRESS(from->gun[1].spin, 2048)) {
            gunflags |= GB_SPIN1;
        }
    } else {
        // save previous state
        to->gun[0].frame = from->gun[0].frame;
        to->gun[0].spin = from->gun[0].spin;
        to->gun[1].frame = from->gun[1].frame;
        to->gun[1].spin = from->gun[1].spin;
    }

    if (gunflags) {
        *pflags |= PS_GUNS;

        MSG_WriteByte(gunflags);
        
        if (gunflags & GB_INDEX0) {
            MSG_WriteByte(to->gun[0].index);
        }
        if (gunflags & GB_INDEX1) {
            MSG_WriteByte(to->gun[1].index);
        }
        if (gunflags & GB_FRAME0) {
            MSG_WriteShort(to->gun[0].frame);
        }
        if (gunflags & GB_FRAME1) {
            MSG_WriteShort(to->gun[1].frame);
        }
        if (gunflags & GB_SPIN0) {
            MSG_WriteLong(FLOAT2COMPRESS(to->gun[0].spin, 2048));
        }
        if (gunflags & GB_SPIN1) {
            MSG_WriteLong(FLOAT2COMPRESS(to->gun[1].spin, 2048));
        }
    }

    statbits = 0;
    for (i = 0; i < MAX_STATS; i++)
        if (to->stats[i] != from->stats[i])
            statbits |= 1U << i;

    if (statbits) {
        eflags |= EPS_STATS;

        // send stats
        if (eflags & EPS_STATS) {
            MSG_WriteLong(statbits);
            for (i = 0; i < MAX_STATS; i++)
                if (statbits & (1U << i))
                    MSG_WriteShort(to->stats[i]);
        }
    }

    if (from->reverb != to->reverb) {
        *pflags |= PS_REVERB;
        MSG_WriteByte(to->reverb);
    }

    return eflags;
}

/*
==============================================================================

            READING

==============================================================================
*/

void MSG_BeginReading(void)
{
    SZ_BeginReading(&msg_read);
}

byte *MSG_ReadData(size_t len)
{
    return SZ_ReadData(&msg_read, len);
}

// returns -1 if no more characters are available
int MSG_ReadChar(void)
{
    byte *buf = MSG_ReadData(1);
    int c;

    if (!buf) {
        c = -1;
    } else {
        c = (signed char)buf[0];
    }

    return c;
}

int MSG_ReadByte(void)
{
    byte *buf = MSG_ReadData(1);
    int c;

    if (!buf) {
        c = -1;
    } else {
        c = (unsigned char)buf[0];
    }

    return c;
}

int MSG_ReadShort(void)
{
    byte *buf = MSG_ReadData(2);
    int c;

    if (!buf) {
        c = -1;
    } else {
        c = (signed short)RL16(buf);
    }

    return c;
}

int MSG_ReadWord(void)
{
    byte *buf = MSG_ReadData(2);
    int c;

    if (!buf) {
        c = -1;
    } else {
        c = (unsigned short)RL16(buf);
    }

    return c;
}

int MSG_ReadLong(void)
{
    byte *buf = MSG_ReadData(4);
    int c;

    if (!buf) {
        c = -1;
    } else {
        c = RL32(buf);
    }

    return c;
}

float MSG_ReadFloat(void)
{
    byte *buf = MSG_ReadData(4);
    union
    {
        int c;
        float v;
    } i2f;

    if (!buf) {
        i2f.v = -1;
    } else {
        i2f.c = RL32(buf);
    }

    return i2f.v;
}

size_t MSG_ReadString(char *dest, size_t size)
{
    int     c;
    size_t  len = 0;

    while (1) {
        c = MSG_ReadByte();
        if (c == -1 || c == 0) {
            break;
        }
        if (len + 1 < size) {
            *dest++ = c;
        }
        len++;
    }
    if (size) {
        *dest = 0;
    }

    return len;
}

size_t MSG_ReadStringLine(char *dest, size_t size)
{
    int     c;
    size_t  len = 0;

    while (1) {
        c = MSG_ReadByte();
        if (c == -1 || c == 0 || c == '\n') {
            break;
        }
        if (len + 1 < size) {
            *dest++ = c;
        }
        len++;
    }
    if (size) {
        *dest = 0;
    }

    return len;
}

static inline float MSG_ReadCoord(void)
{
    return SHORT2COORD(MSG_ReadLong());
}

void MSG_ReadPos(vec3_t pos)
{
    pos[0] = MSG_ReadCoord();
    pos[1] = MSG_ReadCoord();
    pos[2] = MSG_ReadCoord();
}

static inline float MSG_ReadAngle(void)
{
    return BYTE2ANGLE(MSG_ReadChar());
}

static inline float MSG_ReadAngle16(void)
{
    return SHORT2ANGLE(MSG_ReadShort());
}

void MSG_ReadDir(vec3_t dir)
{
    int     b;

    b = MSG_ReadByte();
    if (b < 0 || b >= NUMVERTEXNORMALS)
        Com_Error(ERR_DROP, "MSG_ReadDir: out of range");
    VectorCopy(bytedirs[b], dir);
}

int MSG_ReadBits(int bits)
{
    int i, value;
    size_t bitpos;
    bool sgn;

    if (bits == 0 || bits < -31 || bits > 32) {
        Com_Errorf(ERR_FATAL, "MSG_ReadBits: bad bits: %d", bits);
    }

    bitpos = msg_read.bitpos;
    if ((bitpos & 7) == 0) {
        // optimized case
        switch (bits) {
        case -8:
            value = MSG_ReadChar();
            return value;
        case 8:
            value = MSG_ReadByte();
            return value;
        case -16:
            value = MSG_ReadShort();
            return value;
        case 32:
            value = MSG_ReadLong();
            return value;
        default:
            break;
        }
    }

    sgn = false;
    if (bits < 0) {
        bits = -bits;
        sgn = true;
    }

    value = 0;
    for (i = 0; i < bits; i++, bitpos++) {
        unsigned get = (msg_read.data[bitpos >> 3] >> (bitpos & 7)) & 1;
        value |= get << i;
    }
    msg_read.bitpos = bitpos;
    msg_read.readcount = (bitpos + 7) >> 3;

    if (sgn) {
        if (value & (1 << (bits - 1))) {
            value |= -1 ^ ((1U << bits) - 1);
        }
    }

    return value;
}

void MSG_ReadDeltaUsercmd(const usercmd_t *from,
                          usercmd_t *to)
{
    int bits, count;

    if (from) {
        memcpy(to, from, sizeof(*to));
    } else {
        memset(to, 0, sizeof(*to));
    }

    if (!MSG_ReadBits(1)) {
        return;
    }

    bits = MSG_ReadBits(8);

// read current angles
    if (bits & CM_ANGLE1) {
        if (MSG_ReadBits(1)) {
            to->angles[0] += SHORT2ANGLE(MSG_ReadBits(-8));
        } else {
            to->angles[0] = SHORT2ANGLE(MSG_ReadBits(-16));
        }
    }
    if (bits & CM_ANGLE2) {
        if (MSG_ReadBits(1)) {
            to->angles[1] += SHORT2ANGLE(MSG_ReadBits(-8));
        } else {
            to->angles[1] = SHORT2ANGLE(MSG_ReadBits(-16));
        }
    }
    if (bits & CM_ANGLE3) {
        to->angles[2] = SHORT2ANGLE(MSG_ReadBits(-16));
    }

// read movement
    count = -10;

    if (bits & CM_FORWARD) {
        to->forwardmove = MSG_ReadBits(count);
    }
    if (bits & CM_SIDE) {
        to->sidemove = MSG_ReadBits(count);
    }
    if (bits & CM_UP) {
        to->upmove = MSG_ReadBits(count);
    }

// read buttons
    if (bits & CM_BUTTONS) {
        int buttons = MSG_ReadBits(3);
        to->buttons = (buttons & 3) | ((buttons & 4) << 5);
    }

// read time to run command
    if (bits & CM_IMPULSE) {
        to->msec = MSG_ReadBits(8);
    }
}

#if USE_CLIENT

/*
=================
MSG_ParseEntityBits

Returns the entity number and the header bits
=================
*/
int MSG_ParseEntityBits(int *bits, msgEsFlags_t flags)
{
    unsigned    b, total;
    int         number;

    total = MSG_ReadByte();
    if (total & U_MOREBITS1) {
        b = MSG_ReadByte();
        total |= b << 8;
    }
    if (total & U_MOREBITS2) {
        b = MSG_ReadByte();
        total |= b << 16;
    }
    if (total & U_MOREBITS3) {
        b = MSG_ReadByte();
        total |= b << 24;
    }

    if ((total & U_NUMBER16) || (flags & MSG_ES_AMBIENT))
        number = MSG_ReadShort();
    else
        number = MSG_ReadByte();

    *bits = total;

    return number;
}

/*
==================
MSG_ParseDeltaEntity

Can go from either a baseline or a previous packet_entity
==================
*/
void MSG_ParseDeltaEntity(const entity_state_t *from,
    entity_state_t *to,
    int            number,
    int            bits,
    msgEsFlags_t   flags)
{
    if (!to) {
        Com_Errorf(ERR_DROP, "%s: NULL", __func__);
    }

    // set everything to the state we are delta'ing from
    if (!from) {
        memset(to, 0, sizeof(*to));
    } else if (to != from) {
        memcpy(to, from, sizeof(*to));
    }

    to->number = number;
    to->event = 0;

    if (!bits) {
        return;
    }

    if (bits & U_MODEL) {
        to->modelindex = MSG_ReadByte();
    }
    if (bits & U_MODEL2) {
        to->modelindex2 = MSG_ReadByte();
    }
    if (bits & U_MODEL3) {
        to->modelindex3 = MSG_ReadByte();
    }
    if (bits & U_MODEL4) {
        to->modelindex4 = MSG_ReadByte();
    }

    if (bits & U_FRAME8)
        to->frame = MSG_ReadByte();
    if (bits & U_FRAME16)
        to->frame = MSG_ReadShort();

    if ((bits & (U_SKIN8 | U_SKIN16)) == (U_SKIN8 | U_SKIN16))  //used for laser colors
        to->skinnum = MSG_ReadLong();
    else if (bits & U_SKIN8)
        to->skinnum = MSG_ReadByte();
    else if (bits & U_SKIN16)
        to->skinnum = MSG_ReadWord();

    if ((bits & (U_EFFECTS8 | U_EFFECTS16)) == (U_EFFECTS8 | U_EFFECTS16))
        to->effects = MSG_ReadLong();
    else if (bits & U_EFFECTS8)
        to->effects = MSG_ReadByte();
    else if (bits & U_EFFECTS16)
        to->effects = MSG_ReadWord();

    if ((bits & (U_RENDERFX8 | U_RENDERFX16)) == (U_RENDERFX8 | U_RENDERFX16))
        to->renderfx = MSG_ReadLong();
    else if (bits & U_RENDERFX8)
        to->renderfx = MSG_ReadByte();
    else if (bits & U_RENDERFX16)
        to->renderfx = MSG_ReadWord();

    if (bits & U_ORIGIN1) {
        to->origin[0] = MSG_ReadCoord();
    }
    if (bits & U_ORIGIN2) {
        to->origin[1] = MSG_ReadCoord();
    }
    if (bits & U_ORIGIN3) {
        to->origin[2] = MSG_ReadCoord();
    }

    if (bits & U_ANGLE1)
        to->angles[0] = MSG_ReadAngle16();
    if (bits & U_ANGLE2)
        to->angles[1] = MSG_ReadAngle16();
    if (bits & U_ANGLE3)
        to->angles[2] = MSG_ReadAngle16();

    if (bits & U_OLDORIGIN) {
        MSG_ReadPos(to->old_origin);
    }

    if (bits & U_SOUND) {
        to->sound = MSG_ReadByte();
    }

    if (bits & U_EVENT) {
        to->event = MSG_ReadByte();
    }

    if (bits & U_SOLID) {
        to->bbox = MSG_ReadLong();
    }

    if (bits & U_SOUNDPITCH) {
        to->sound_pitch = MSG_ReadChar();
    }
}

/*
==================
MSG_ParseDeltaPacketEntity

Can go from either a baseline or a previous packet_entity
==================
*/
void MSG_ParseDeltaPacketEntity(const entity_state_t *from,
    entity_state_t *to,
    int            number,
    int            bits,
    msgEsFlags_t   flags)
{
    if (number < 1 || number >= MAX_PACKET_ENTITIES) {
        Com_Errorf(ERR_DROP, "%s: bad entity number: %d", __func__, number);
    }

    MSG_ParseDeltaEntity(from, to, number, bits, flags);
}

/*
==================
MSG_ParseDeltaAmbientEntity

Can go from either a baseline or a previous packet_entity
==================
*/
void MSG_ParseDeltaAmbientEntity(const entity_state_t *from,
    entity_state_t *to,
    int            number,
    int            bits,
    msgEsFlags_t   flags)
{
    if (number < OFFSET_AMBIENT_ENTITIES || number >= OFFSET_PRIVATE_ENTITIES) {
        Com_Errorf(ERR_DROP, "%s: bad entity number: %d", __func__, number);
    }

    MSG_ParseDeltaEntity(from, to, number, bits, flags);
}

#endif // USE_CLIENT

#if USE_CLIENT

/*
===================
MSG_ParseDeltaPlayerstate_Default
===================
*/
void MSG_ParseDeltaPlayerstate(const player_state_t    *from,
                               player_state_t    *to,
                               int               flags,
                               int               extraflags)
{
    int         i;
    int         statbits;

    if (!to) {
        Com_Errorf(ERR_DROP, "%s: NULL", __func__);
    }

    // clear to old value before delta parsing
    if (!from) {
        memset(to, 0, sizeof(*to));
    } else if (to != from) {
        memcpy(to, from, sizeof(*to));
    }

    //
    // parse the pmove_state_t
    //
    if (flags & PS_M_TYPE)
        to->pmove.pm_type = MSG_ReadByte();

    if (flags & PS_M_ORIGIN) {
        if (to->pmove.pm_type == PM_SPECTATOR)
        {
            to->pmove.origin[0] = MSG_ReadFloat();
            to->pmove.origin[1] = MSG_ReadFloat();
        }
        else
        {
            to->pmove.origin[0] = MSG_ReadCoord();
            to->pmove.origin[1] = MSG_ReadCoord();
        }
    }

    if (extraflags & EPS_M_ORIGIN2) {
        if (to->pmove.pm_type == PM_SPECTATOR)
            to->pmove.origin[2] = MSG_ReadFloat();
        else
            to->pmove.origin[2] = MSG_ReadCoord();
    }

    if (flags & PS_M_VELOCITY) {
        if (to->pmove.pm_type == PM_SPECTATOR)
        {
            to->pmove.velocity[0] = MSG_ReadFloat();
            to->pmove.velocity[1] = MSG_ReadFloat();
        }
        else
        {
            to->pmove.velocity[0] = MSG_ReadCoord();
            to->pmove.velocity[1] = MSG_ReadCoord();
        }
    }

    if (extraflags & EPS_M_VELOCITY2) {
        if (to->pmove.pm_type == PM_SPECTATOR)
            to->pmove.velocity[2] = MSG_ReadFloat();
        else
            to->pmove.velocity[2] = MSG_ReadCoord();
    }

    if (flags & PS_M_TIME)
        to->pmove.pm_time = MSG_ReadByte();

    if (flags & PS_M_FLAGS)
        to->pmove.pm_flags = MSG_ReadByte();

    if (flags & PS_M_GRAVITY)
        to->pmove.gravity = MSG_ReadShort();

    if (flags & PS_M_DELTA_ANGLES) {
        to->pmove.delta_angles[0] = MSG_ReadAngle16();
        to->pmove.delta_angles[1] = MSG_ReadAngle16();
        to->pmove.delta_angles[2] = MSG_ReadAngle16();
    }

    //
    // parse the rest of the player_state_t
    //
    if (flags & PS_VIEWOFFSET) {
        MSG_ReadOffset(to->viewoffset);
    }

    if (flags & PS_VIEWANGLES) {
        to->viewangles[0] = MSG_ReadAngle16();
        to->viewangles[1] = MSG_ReadAngle16();
    }

    if (extraflags & EPS_VIEWANGLE2) {
        to->viewangles[2] = MSG_ReadAngle16();
    }

    if (flags & PS_KICKANGLES) {
        MSG_ReadOffset(to->kick_angles);
    }
    
    if (flags & PS_BLEND) {
        to->blend[0] = BYTE2BLEND(MSG_ReadByte());
        to->blend[1] = BYTE2BLEND(MSG_ReadByte());
        to->blend[2] = BYTE2BLEND(MSG_ReadByte());
        to->blend[3] = BYTE2BLEND(MSG_ReadByte());
    }
    
    if (flags & PS_FOV)
        to->fov = MSG_ReadByte();
    
    if (flags & PS_RDFLAGS)
        to->rdflags = MSG_ReadByte();

    if (flags & PS_GUNS) {
        int gunflags = MSG_ReadByte();
        
        if (gunflags & GB_INDEX0) {
            to->gun[0].index = MSG_ReadByte();
        }
        if (gunflags & GB_INDEX1) {
            to->gun[1].index = MSG_ReadByte();
        }
        if (gunflags & GB_FRAME0) {
            to->gun[0].frame = MSG_ReadWord();
        }
        if (gunflags & GB_FRAME1) {
            to->gun[1].frame = MSG_ReadWord();
        }
        if (gunflags & GB_SPIN0) {
            to->gun[0].spin = COMPRESS2FLOAT(MSG_ReadLong(), 2048);
        }
        if (gunflags & GB_SPIN1) {
            to->gun[1].spin = COMPRESS2FLOAT(MSG_ReadLong(), 2048);
        }
    }

    // parse stats
    if (extraflags & EPS_STATS) {
        statbits = MSG_ReadLong();
        for (i = 0; i < MAX_STATS; i++) {
            if (statbits & (1U << i)) {
                to->stats[i] = MSG_ReadShort();
            }
        }
    }

    if (flags & PS_REVERB) {
        to->reverb = MSG_ReadByte();
    }
}

#endif // USE_CLIENT

/*
==============================================================================

            DEBUGGING STUFF

==============================================================================
*/

#if USE_DEBUG

#define SHOWBITS(x) Com_LPrintf(PRINT_DEVELOPER, x " ")

#if USE_CLIENT

void MSG_ShowDeltaPlayerstateBits(int flags, int extraflags)
{
#define SP(b,s) if(flags&PS_##b) SHOWBITS(s)
#define SE(b,s) if(extraflags&EPS_##b) SHOWBITS(s)
    SP(M_TYPE,          "pmove.pm_type");
    SP(M_ORIGIN,        "pmove.origin[0,1]");
    SE(M_ORIGIN2,       "pmove.origin[2]");
    SP(M_VELOCITY,      "pmove.velocity[0,1]");
    SE(M_VELOCITY2,     "pmove.velocity[2]");
    SP(M_TIME,          "pmove.pm_time");
    SP(M_FLAGS,         "pmove.pm_flags");
    SP(M_GRAVITY,       "pmove.gravity");
    SP(M_DELTA_ANGLES,  "pmove.delta_angles");
    SP(VIEWOFFSET,      "viewoffset");
    SP(VIEWANGLES,      "viewangles[0,1]");
    SE(VIEWANGLE2,      "viewangles[2]");
    SP(KICKANGLES,      "kick_angles");
    SP(GUNS,            "guns");
    SP(BLEND,           "blend");
    SP(FOV,             "fov");
    SP(RDFLAGS,         "rdflags");
    SE(STATS,           "stats");
#undef SP
#undef SE
}

void MSG_ShowDeltaUsercmdBits(int bits)
{
    if (!bits) {
        SHOWBITS("<none>");
        return;
    }

#define S(b,s) if(bits&CM_##b) SHOWBITS(s)
    S(ANGLE1,   "angle1");
    S(ANGLE2,   "angle2");
    S(ANGLE3,   "angle3");
    S(FORWARD,  "forward");
    S(SIDE,     "side");
    S(UP,       "up");
    S(BUTTONS,  "buttons");
    S(IMPULSE,  "msec");
#undef S
}

#endif // USE_CLIENT

#if USE_CLIENT

void MSG_ShowDeltaEntityBits(int bits)
{
#define S(b,s) if(bits&U_##b) SHOWBITS(s)
    S(MODEL, "modelindex");
    S(MODEL2, "modelindex2");
    S(MODEL3, "modelindex3");
    S(MODEL4, "modelindex4");

    if (bits & U_FRAME8)
        SHOWBITS("frame8");
    if (bits & U_FRAME16)
        SHOWBITS("frame16");

    if ((bits & (U_SKIN8 | U_SKIN16)) == (U_SKIN8 | U_SKIN16))
        SHOWBITS("skinnum32");
    else if (bits & U_SKIN8)
        SHOWBITS("skinnum8");
    else if (bits & U_SKIN16)
        SHOWBITS("skinnum16");

    if ((bits & (U_EFFECTS8 | U_EFFECTS16)) == (U_EFFECTS8 | U_EFFECTS16))
        SHOWBITS("effects32");
    else if (bits & U_EFFECTS8)
        SHOWBITS("effects8");
    else if (bits & U_EFFECTS16)
        SHOWBITS("effects16");

    if ((bits & (U_RENDERFX8 | U_RENDERFX16)) == (U_RENDERFX8 | U_RENDERFX16))
        SHOWBITS("renderfx32");
    else if (bits & U_RENDERFX8)
        SHOWBITS("renderfx8");
    else if (bits & U_RENDERFX16)
        SHOWBITS("renderfx16");

    S(ORIGIN1, "origin[0]");
    S(ORIGIN2, "origin[1]");
    S(ORIGIN3, "origin[2]");
    S(ANGLE1, "angles[0]");
    S(ANGLE2, "angles[1]");
    S(ANGLE3, "angles[2]");
    S(OLDORIGIN, "old_origin");
    S(SOUND, "sound");
    S(EVENT, "event");
    S(SOLID, "solid");
    S(SOUNDPITCH, "sound_pitch");
#undef S
}

const char *MSG_ServerCommandString(int cmd)
{
    switch (cmd) {
    case -1: return "END OF MESSAGE";
    default: return "UNKNOWN COMMAND";
#define S(x) case svc_##x: return "svc_" #x;
        S(bad)
        S(muzzleflash)
        S(muzzleflash2)
        S(temp_entity)
        S(layout)
        S(inventory)
        S(nop)
        S(disconnect)
        S(reconnect)
        S(sound)
        S(print)
        S(stufftext)
        S(serverdata)
        S(configstring)
        S(centerprint)
        S(download)
        S(playerinfo)
        S(frame)
        S(ambient)
        S(zpacket)
        S(zdownload)
        S(gamestate)
#undef S
    }
}

#endif // USE_CLIENT

#endif // USE_DEBUG

