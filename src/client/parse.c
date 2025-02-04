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
// cl_parse.c  -- parse a message received from the server

#include "client.h"

/*
=====================================================================

  DELTA FRAME PARSING

=====================================================================
*/

static inline void CL_ParseDeltaEntity(server_frame_t  *frame,
                                       int             newnum,
                                       entity_state_t  *old,
                                       int             bits)
{
    entity_state_t    *state = &cl.entityStates[cl.numEntityStates & PARSE_ENTITIES_MASK];
    cl.numEntityStates++;
    frame->numEntities++;

#if USE_DEBUG
    if (cl_shownet->integer > 2 && bits) {
        MSG_ShowDeltaEntityBits(bits);
        Com_LPrintf(PRINT_DEVELOPER, "\n");
    }
#endif

    MSG_ParseDeltaPacketEntity(old, state, newnum, bits, cl.esFlags);

    // shuffle previous origin to old
    if (!(bits & U_OLDORIGIN) && !(state->renderfx & RF_MASK_BEAMLIKE))
        VectorCopy(old->origin, state->old_origin);
}

static void CL_ParsePacketEntities(server_frame_t *oldframe,
                                   server_frame_t *frame)
{
    int            newnum;
    int            bits;
    entity_state_t    *oldstate;
    int            oldindex, oldnum;
    int i;

    frame->firstEntity = cl.numEntityStates;
    frame->numEntities = 0;

    // delta from the entities present in oldframe
    oldindex = 0;
    oldstate = NULL;
    if (!oldframe) {
        oldnum = 99999;
    } else {
        if (oldindex >= oldframe->numEntities) {
            oldnum = 99999;
        } else {
            i = oldframe->firstEntity + oldindex;
            oldstate = &cl.entityStates[i & PARSE_ENTITIES_MASK];
            oldnum = oldstate->number;
        }
    }

    while (1) {
        newnum = MSG_ParseEntityBits(&bits, 0);
        if (newnum < 0 || newnum >= MAX_PACKET_ENTITIES) {
            Com_Errorf(ERR_DROP, "%s: bad number: %d", __func__, newnum);
        }

        if (msg_read.readcount > msg_read.cursize) {
            Com_Errorf(ERR_DROP, "%s: read past end of message", __func__);
        }

        if (!newnum) {
            break;
        }

        while (oldnum < newnum) {
            // one or more entities from the old packet are unchanged
            SHOWNET(3, "   unchanged: %i\n", oldnum);
            CL_ParseDeltaEntity(frame, oldnum, oldstate, 0);

            oldindex++;

            if (oldindex >= oldframe->numEntities) {
                oldnum = 99999;
            } else {
                i = oldframe->firstEntity + oldindex;
                oldstate = &cl.entityStates[i & PARSE_ENTITIES_MASK];
                oldnum = oldstate->number;
            }
        }

        if (bits & U_REMOVE) {
            // the entity present in oldframe is not in the current frame
            SHOWNET(2, "   remove: %i\n", newnum);
            if (oldnum != newnum) {
                Com_DPrintf("U_REMOVE: oldnum != newnum\n");
            }
            if (!oldframe) {
                Com_Errorf(ERR_DROP, "%s: U_REMOVE with NULL oldframe", __func__);
            }

            oldindex++;

            if (oldindex >= oldframe->numEntities) {
                oldnum = 99999;
            } else {
                i = oldframe->firstEntity + oldindex;
                oldstate = &cl.entityStates[i & PARSE_ENTITIES_MASK];
                oldnum = oldstate->number;
            }
            continue;
        }

        if (oldnum == newnum) {
            // delta from previous state
            SHOWNET(2, "   delta: %i ", newnum);
            CL_ParseDeltaEntity(frame, newnum, oldstate, bits);
            if (!bits) {
                SHOWNET(2, "\n");
            }

            oldindex++;

            if (oldindex >= oldframe->numEntities) {
                oldnum = 99999;
            } else {
                i = oldframe->firstEntity + oldindex;
                oldstate = &cl.entityStates[i & PARSE_ENTITIES_MASK];
                oldnum = oldstate->number;
            }
            continue;
        }

        if (oldnum > newnum) {
            // delta from baseline
            SHOWNET(2, "   baseline: %i ", newnum);
            CL_ParseDeltaEntity(frame, newnum, &cl.baselines[newnum], bits);
            if (!bits) {
                SHOWNET(2, "\n");
            }
            continue;
        }
    }

    // any remaining entities in the old frame are copied over
    while (oldnum != 99999) {
        // one or more entities from the old packet are unchanged
        SHOWNET(3, "   unchanged: %i\n", oldnum);
        CL_ParseDeltaEntity(frame, oldnum, oldstate, 0);

        oldindex++;

        if (oldindex >= oldframe->numEntities) {
            oldnum = 99999;
        } else {
            i = oldframe->firstEntity + oldindex;
            oldstate = &cl.entityStates[i & PARSE_ENTITIES_MASK];
            oldnum = oldstate->number;
        }
    }
}

static void CL_ParseFrame(int extrabits)
{
    uint32_t bits, extraflags;
    int     currentframe, deltaframe,
            delta, suppressed;
    server_frame_t  frame, *oldframe;
    player_state_t  *from;
    int     length;

    memset(&frame, 0, sizeof(frame));

    cl.frameflags = 0;

    extraflags = 0;
    bits = MSG_ReadLong();

    currentframe = bits & FRAMENUM_MASK;
    delta = bits >> FRAMENUM_BITS;

    if (delta == 31) {
        deltaframe = -1;
    } else {
        deltaframe = currentframe - delta;
    }

    bits = MSG_ReadByte();

    suppressed = bits & SUPPRESSCOUNT_MASK;
    if (suppressed & FF_CLIENTPRED) {
        // CLIENTDROP is implied, don't draw both
        suppressed &= ~FF_CLIENTDROP;
    }
    cl.frameflags |= suppressed;
    extraflags = (extrabits << 4) | (bits >> SUPPRESSCOUNT_BITS);

    frame.number = currentframe;
    frame.delta = deltaframe;

    if (cls.netchan && cls.netchan->dropped) {
        cl.frameflags |= FF_SERVERDROP;
    }

    // if the frame is delta compressed from data that we no longer have
    // available, we must suck up the rest of the frame, but not use it, then
    // ask for a non-compressed message
    if (deltaframe > 0) {
        oldframe = &cl.frames[deltaframe & UPDATE_MASK];
        from = &oldframe->ps;
        if (deltaframe == currentframe) {
            // old servers may cause this on map change
            Com_DPrintf("%s: delta from current frame\n", __func__);
            cl.frameflags |= FF_BADFRAME;
        } else if (oldframe->number != deltaframe) {
            // the frame that the server did the delta from
            // is too old, so we can't reconstruct it properly.
            Com_DPrintf("%s: delta frame was never received or too old\n", __func__);
            cl.frameflags |= FF_OLDFRAME;
        } else if (!oldframe->valid) {
            // should never happen
            Com_DPrintf("%s: delta from invalid frame\n", __func__);
            cl.frameflags |= FF_BADFRAME;
        } else if (cl.numEntityStates - oldframe->firstEntity >
                   MAX_PARSE_ENTITIES - MAX_PACKET_ENTITIES) {
            Com_DPrintf("%s: delta entities too old\n", __func__);
            cl.frameflags |= FF_OLDENT;
        } else {
            frame.valid = true; // valid delta parse
        }
        if (!frame.valid && cl.frame.valid && cls.demo.playback) {
            Com_DPrintf("%s: recovering broken demo\n", __func__);
            oldframe = &cl.frame;
            from = &oldframe->ps;
            frame.valid = true;
        }
    } else {
        oldframe = NULL;
        from = NULL;
        frame.valid = true; // uncompressed frame
        cl.frameflags |= FF_NODELTA;
    }

    // read areabits
    length = MSG_ReadByte();
    if (length) {
        if (length < 0 || msg_read.readcount + length > msg_read.cursize) {
            Com_Errorf(ERR_DROP, "%s: read past end of message", __func__);
        }
        if (length > sizeof(frame.areabits)) {
            Com_Errorf(ERR_DROP, "%s: invalid areabits length", __func__);
        }
        memcpy(frame.areabits, msg_read.data + msg_read.readcount, length);
        msg_read.readcount += length;
        frame.areabytes = length;
    } else {
        frame.areabytes = 0;
    }

    SHOWNET(2, "%3zu:playerinfo\n", msg_read.readcount - 1);

    // parse playerstate
    bits = MSG_ReadWord();

    MSG_ParseDeltaPlayerstate(from, &frame.ps, bits, extraflags);

#if USE_DEBUG
    if (cl_shownet->integer > 2 && (bits || extraflags)) {
        MSG_ShowDeltaPlayerstateBits(bits, extraflags);
        Com_LPrintf(PRINT_DEVELOPER, "\n");
    }
#endif

    // parse clientNum
    if (extraflags & EPS_CLIENTNUM) {
        frame.clientNum = MSG_ReadByte();
    } else if (oldframe) {
        frame.clientNum = oldframe->clientNum;
    }

    // parse packetentities
    SHOWNET(2, "%3zu:packetentities\n", msg_read.readcount - 1);

    CL_ParsePacketEntities(oldframe, &frame);

    // save the frame off in the backup array for later delta comparisons
    cl.frames[currentframe & UPDATE_MASK] = frame;

#if USE_DEBUG
    if (cl_shownet->integer > 2) {
        int rtt = 0;
        if (cls.netchan) {
            int seq = cls.netchan->incoming_acknowledged & CMD_MASK;
            rtt = cls.realtime - cl.history[seq].sent;
        }
        Com_LPrintf(PRINT_DEVELOPER, "%3zu:frame:%d  delta:%d  rtt:%d\n",
                    msg_read.readcount - 1, frame.number, frame.delta, rtt);
    }
#endif

    if (!frame.valid) {
        cl.frame.valid = false;
        return; // do not change anything
    }

    if (!frame.ps.fov) {
        // fail out early to prevent spurious errors later
        Com_Errorf(ERR_DROP, "%s: bad fov", __func__);
    }

    if (cls.state < ca_precached)
        return;

    cl.oldframe = cl.frame;
    cl.frame = frame;

    cls.demo.frames_read++;

    if (!cls.demo.seeking)
        CL_DeltaFrame();
}

static void CL_ParseAmbients(void)
{
    cl.ambient_state_id = MSG_ReadByte();
    cl.num_ambient_entities = MSG_ReadWord();

    // delta from the entities present in oldframe
    while (1) {
        uint32_t bits;
        int32_t num = MSG_ParseEntityBits(&bits, MSG_ES_AMBIENT);
        if (num < OFFSET_AMBIENT_ENTITIES || num > OFFSET_PRIVATE_ENTITIES) {
            Com_Errorf(ERR_DROP, "%s: bad number: %d", __func__, num);
        }

        if (msg_read.readcount > msg_read.cursize) {
            Com_Errorf(ERR_DROP, "%s: read past end of message", __func__);
        }

        if (num == OFFSET_PRIVATE_ENTITIES) {
            break;
        }

        // delta from ambient info
        SHOWNET(2, "   ambient: %i ", num);
        MSG_ParseDeltaAmbientEntity(&cl.ambients[num - MAX_PACKET_ENTITIES], &cl.ambients[num - MAX_PACKET_ENTITIES], num, bits, MSG_ES_AMBIENT);
        if (!bits) {
            SHOWNET(2, "\n");
        }
    }

    if (!cls.demo.playback) {
        MSG_WriteByte(clc_ambient);
        MSG_WriteByte(cl.ambient_state_id);
        MSG_FlushTo(&cls.netchan->message);
    }
}

/*
=====================================================================

  SERVER CONNECTING MESSAGES

=====================================================================
*/

static void CL_ParseConfigstring(int index)
{
    size_t  len, maxlen;
    char    *s;

    if (index < 0 || index >= MAX_CONFIGSTRINGS) {
        Com_Errorf(ERR_DROP, "%s: bad index: %d", __func__, index);
    }

    s = cl.configstrings[index];
    maxlen = CS_SIZE(index);
    len = MSG_ReadString(s, maxlen);

    SHOWNET(2, "    %d \"%s\"\n", index, s);

    if (len >= maxlen) {
        Com_WPrintf(
            "%s: index %d overflowed: %zu > %zu\n",
            __func__, index, len, maxlen - 1);
    }

    if (cls.demo.seeking) {
        Q_SetBit(cl.dcs, index);
        return;
    }

    if (cls.demo.recording && cls.demo.paused) {
        Q_SetBit(cl.dcs, index);
    }

    // do something apropriate
    CL_UpdateConfigstring(index);
}

static void CL_ParseBaseline(int index, int bits)
{
    if (index < 1 || index >= MAX_PACKET_ENTITIES) {
        Com_Errorf(ERR_DROP, "%s: bad index: %d", __func__, index);
    }
#if USE_DEBUG
    if (cl_shownet->integer > 2) {
        MSG_ShowDeltaEntityBits(bits);
        Com_LPrintf(PRINT_DEVELOPER, "\n");
    }
#endif
    MSG_ParseDeltaPacketEntity(NULL, &cl.baselines[index], index, bits, cl.esFlags);
}

static void CL_ParseAmbient(int index, int bits)
{
    if (index < OFFSET_AMBIENT_ENTITIES || index >= OFFSET_PRIVATE_ENTITIES) {
        Com_Errorf(ERR_DROP, "%s: bad index: %d", __func__, index);
    }
#ifdef _DEBUG
    if (cl_shownet->integer > 2) {
        MSG_ShowDeltaEntityBits(bits);
        Com_LPrintf(PRINT_DEVELOPER, "\n");
    }
#endif
    MSG_ParseDeltaAmbientEntity(NULL, &cl.ambients[index - MAX_PACKET_ENTITIES], index, bits, cl.esFlags | MSG_ES_AMBIENT);
}

// instead of wasting space for svc_configstring and svc_spawnbaseline
// bytes, entire game state is compressed into a single stream.
static void CL_ParseGamestate(void)
{
    int        index, bits;

    while (msg_read.readcount < msg_read.cursize) {
        index = MSG_ReadShort();
        if (index == MAX_CONFIGSTRINGS) {
            break;
        }
        CL_ParseConfigstring(index);
    }

    while (msg_read.readcount < msg_read.cursize) {
        index = MSG_ParseEntityBits(&bits, 0);
        if (!index) {
            break;
        }
        CL_ParseBaseline(index, bits);
    }

    while (msg_read.readcount < msg_read.cursize) {
        index = MSG_ParseEntityBits(&bits, MSG_ES_AMBIENT);
        if (index == OFFSET_PRIVATE_ENTITIES) {
            break;
        }
        CL_ParseAmbient(index, bits);
    }

    cl.ambient_state_id = MSG_ReadByte();
    cl.num_ambient_entities = MSG_ReadWord();
}

static void CL_ParseServerData(void)
{
    char    levelname[MAX_QPATH];
    int     protocol, attractloop q_unused;

    Cbuf_Execute(&cl_cmdbuf);          // make sure any stuffed commands are done

    // wipe the client_state_t struct
    CL_ClearState();

    // parse protocol version number
    protocol = MSG_ReadLong();
    cl.servercount = MSG_ReadLong();
    attractloop = MSG_ReadByte();

    Com_DPrintf("Serverdata packet received "
                "(protocol=%d, servercount=%d, attractloop=%d)\n",
                protocol, cl.servercount, attractloop);

    // check protocol
    if (cls.serverProtocol != protocol && !cls.demo.playback) {
        Com_Errorf(ERR_DROP, "Requested protocol version %d, but server returned %d.",
                    cls.serverProtocol, protocol);
    }

    // game directory
    if (MSG_ReadString(cl.gamedir, sizeof(cl.gamedir)) >= sizeof(cl.gamedir)) {
        Com_Error(ERR_DROP, "Oversize gamedir string");
    }

    // never allow demos to change gamedir
    // do not change gamedir if connected to local sever either,
    // as it was already done by SV_InitGame, and changing it
    // here will not work since server is now running
    if (!cls.demo.playback && !sv_running->integer) {
        // pretend it has been set by user, so that 'changed' hook
        // gets called and filesystem is restarted
        Cvar_UserSet("game", cl.gamedir);

        // protect it from modifications while we are connected
        fs_game->flags |= CVAR_ROM;
    }

    // parse player entity number
    cl.clientNum = MSG_ReadShort();

    // get the full level name
    MSG_ReadString(levelname, sizeof(levelname));

    // setup default pmove parameters
    PmoveInit(&cl.pmp);

    if (cl.clientNum == -1) {
        SCR_PlayCinematic(levelname);
    } else {
        // seperate the printfs so the server message can have a color
        Con_Printf(
            "\n\n"
            "\35\36\36\36\36\36\36\36\36\36\36\36"
            "\36\36\36\36\36\36\36\36\36\36\36\36"
            "\36\36\36\36\36\36\36\36\36\36\36\37"
            "\n\n");

        Com_SetColor(COLOR_ALT);
        Com_Printf("%s\n", levelname);
        Com_SetColor(COLOR_NONE);

        // make sure clientNum is in range
        if (cl.clientNum < 0 || cl.clientNum >= MAX_CLIENTS) {
            cl.clientNum = CLIENTNUM_NONE;
        }
    }
}

/*
=====================================================================

ACTION MESSAGES

=====================================================================
*/

tent_params_t   te;
mz_params_t     mz;
snd_params_t    snd;

static void CL_ParseTEntPacket(void)
{
    te.type = MSG_ReadByte();

    switch (te.type) {
    case TE_BLOOD:
    case TE_GUNSHOT:
    case TE_SPARKS:
    case TE_BULLET_SPARKS:
    case TE_SCREEN_SPARKS:
    case TE_SHIELD_SPARKS:
    case TE_SHOTGUN:
    case TE_BLASTER:
    case TE_GREENBLOOD:
    case TE_BLASTER2:
	case TE_FLECHETTE:
    case TE_HEATBEAM_SPARKS:
    case TE_HEATBEAM_STEAM:
    case TE_MOREBLOOD:
    case TE_ELECTRIC_SPARKS:
        MSG_ReadPos(te.pos1);
        MSG_ReadDir(te.dir);
        break;

    case TE_SPLASH:
    case TE_LASER_SPARKS:
    case TE_WELDING_SPARKS:
    case TE_TUNNEL_SPARKS:
        te.count = MSG_ReadByte();
        MSG_ReadPos(te.pos1);
        MSG_ReadDir(te.dir);
        te.color = MSG_ReadByte();
        break;

    case TE_BLUEHYPERBLASTER:
    case TE_RAILTRAIL:
    case TE_BUBBLETRAIL:
    case TE_DEBUGTRAIL:
    case TE_BUBBLETRAIL2:
    case TE_BFG_LASER:
        MSG_ReadPos(te.pos1);
        MSG_ReadPos(te.pos2);
        break;

    case TE_GRENADE_EXPLOSION:
    case TE_GRENADE_EXPLOSION_WATER:
    case TE_EXPLOSION2:
    case TE_PLASMA_EXPLOSION:
    case TE_ROCKET_EXPLOSION:
    case TE_ROCKET_EXPLOSION_WATER:
    case TE_EXPLOSION1:
    case TE_EXPLOSION1_NP:
    case TE_EXPLOSION1_BIG:
    case TE_BFG_EXPLOSION:
    case TE_BFG_BIGEXPLOSION:
    case TE_BOSSTPORT:
    case TE_PLAIN_EXPLOSION:
    case TE_CHAINFIST_SMOKE:
    case TE_TRACKER_EXPLOSION:
    case TE_TELEPORT_EFFECT:
    case TE_DBALL_GOAL:
    case TE_WIDOWSPLASH:
    case TE_NUKEBLAST:
        MSG_ReadPos(te.pos1);
        break;

    case TE_PARASITE_ATTACK:
    case TE_MEDIC_CABLE_ATTACK:
    case TE_HEATBEAM:
    case TE_MONSTER_HEATBEAM:
        te.entity1 = MSG_ReadShort();
        MSG_ReadPos(te.pos1);
        MSG_ReadPos(te.pos2);
        break;

    case TE_GRAPPLE_CABLE:
        te.entity1 = MSG_ReadShort();
        MSG_ReadPos(te.pos1);
        MSG_ReadPos(te.pos2);
        MSG_ReadPos(te.offset);
        break;

    case TE_LIGHTNING:
        te.entity1 = MSG_ReadShort();
        te.entity2 = MSG_ReadShort();
        MSG_ReadPos(te.pos1);
        MSG_ReadPos(te.pos2);
        break;

    case TE_FLASHLIGHT:
        MSG_ReadPos(te.pos1);
        te.entity1 = MSG_ReadShort();
        break;

    case TE_FORCEWALL:
        MSG_ReadPos(te.pos1);
        MSG_ReadPos(te.pos2);
        te.color = MSG_ReadByte();
        break;

    case TE_STEAM:
        te.entity1 = MSG_ReadShort();
        te.count = MSG_ReadByte();
        MSG_ReadPos(te.pos1);
        MSG_ReadDir(te.dir);
        te.color = MSG_ReadByte();
        te.entity2 = MSG_ReadShort();
        if (te.entity1 != -1) {
            te.time = MSG_ReadLong();
        }
        break;

    case TE_WIDOWBEAMOUT:
        te.entity1 = MSG_ReadShort();
        MSG_ReadPos(te.pos1);
        break;

    case TE_FLARE:
        te.entity1 = MSG_ReadShort();
        te.count = MSG_ReadByte();
        MSG_ReadPos(te.pos1);
        MSG_ReadDir(te.dir);
        break;

    default:
        Com_Errorf(ERR_DROP, "%s: bad type", __func__);
    }
}

static void CL_ParseMuzzleFlashPacket(int mask)
{
    int entity, weapon;

    entity = MSG_ReadShort();
    if (entity < 1 || entity >= OFFSET_PRIVATE_ENTITIES)
        Com_Errorf(ERR_DROP, "%s: bad entity", __func__);

    weapon = MSG_ReadByte();
    mz.silenced = weapon & mask;
    mz.weapon = weapon & ~mask;
    mz.entity = entity;
}

static void CL_ParseStartSoundPacket(void)
{
    int flags, channel, entity;

    flags = MSG_ReadByte();
    if ((flags & (SND_ENT | SND_POS)) == 0)
        Com_Errorf(ERR_DROP, "%s: neither SND_ENT nor SND_POS set", __func__);

    snd.index = MSG_ReadByte();
    if (snd.index == -1)
        Com_Errorf(ERR_DROP, "%s: read past end of message", __func__);

    if (flags & SND_VOLUME)
        snd.volume = MSG_ReadByte() / 255.0f;
    else
        snd.volume = DEFAULT_SOUND_PACKET_VOLUME;

    if (flags & SND_ATTENUATION)
        snd.attenuation = MSG_ReadByte() / 64.0f;
    else
        snd.attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;

    if (flags & SND_ENT) {
        // entity relative
        channel = MSG_ReadShort();
        entity = channel >> 3;
        if (entity < 0 || entity >= OFFSET_PRIVATE_ENTITIES)
            Com_Errorf(ERR_DROP, "%s: bad entity: %d", __func__, entity);
        snd.entity = entity;
        snd.channel = channel & 7;
    } else {
        snd.entity = 0;
        snd.channel = 0;
    }

    // positioned in space
    if (flags & SND_POS)
        MSG_ReadPos(snd.pos);

    if (flags & SND_PITCH)
        snd.pitch = MSG_ReadChar();
    else
        snd.pitch = 0;

    snd.flags = flags;

    SHOWNET(2, "    %s\n", cl.configstrings[CS_SOUNDS + snd.index]);
}

static void CL_ParseReconnect(void)
{
    if (cls.demo.playback) {
        Com_Error(ERR_DISCONNECT, "Server disconnected");
    }

    Com_Printf("Server disconnected, reconnecting\n");

    // free netchan now to prevent `disconnect'
    // message from being sent to server
    if (cls.netchan) {
        Netchan_Close(cls.netchan);
        cls.netchan = NULL;
    }

    CL_Disconnect(ERR_RECONNECT);

    cls.state = ca_challenging;
    cls.connect_time -= CONNECT_FAST;
    cls.connect_count = 0;

    CL_CheckForResend();
}

#if USE_AUTOREPLY
static void CL_CheckForVersion(const char *s)
{
    char *p;

    p = strstr(s, ": ");
    if (!p) {
        return;
    }

    if (strncmp(p + 2, "!version", 8)) {
        return;
    }

    if (cl.reply_time && cls.realtime - cl.reply_time < 120000) {
        return;
    }

    cl.reply_time = cls.realtime;
    cl.reply_delta = 1024 + (Q_rand() & 1023);
}
#endif

// attempt to scan out an IP address in dotted-quad notation and
// add it into circular array of recent addresses
static void CL_CheckForIP(const char *s)
{
    unsigned b1, b2, b3, b4, port;
    netadr_t *a;
    char *p;

    while (*s) {
        if (sscanf(s, "%3u.%3u.%3u.%3u", &b1, &b2, &b3, &b4) == 4 &&
            b1 < 256 && b2 < 256 && b3 < 256 && b4 < 256) {
            p = strchr(s, ':');
            if (p) {
                port = strtoul(p + 1, NULL, 10);
                if (port < 1024 || port > 65535) {
                    break; // privileged or invalid port
                }
            } else {
                port = PORT_SERVER;
            }

            a = &cls.recent_addr[cls.recent_head++ & RECENT_MASK];
            a->type = NA_IP;
            a->ip.u8[0] = b1;
            a->ip.u8[1] = b2;
            a->ip.u8[2] = b3;
            a->ip.u8[3] = b4;
            a->port = BigShort(port);
            break;
        }

        s++;
    }
}

static void CL_ParsePrint(void)
{
    int level;
    char s[MAX_STRING_CHARS];
    const char *fmt;

    level = MSG_ReadByte();
    MSG_ReadString(s, sizeof(s));

    SHOWNET(2, "    %i \"%s\"\n", level, s);

    if (level != PRINT_CHAT) {
        Com_Printf("%s", s);
        if (!cls.demo.playback) {
            COM_strclr(s);
            Cmd_ExecTrigger(s);
        }
        return;
    }

    if (CL_CheckForIgnore(s)) {
        return;
    }

#if USE_AUTOREPLY
    if (!cls.demo.playback) {
        CL_CheckForVersion(s);
    }
#endif

    CL_CheckForIP(s);

    // disable notify
    if (!cl_chat_notify->integer) {
        Con_SkipNotify(true);
    }

    // filter text
    if (cl_chat_filter->integer) {
        COM_strclr(s);
        fmt = "%s\n";
    } else {
        fmt = "%s";
    }

    Com_LPrintf(PRINT_TALK, fmt, s);

    Con_SkipNotify(false);

    SCR_AddToChatHUD(s);

    // play sound
    if (cl_chat_sound->integer > 1)
        S_StartLocalSound_(ASSET_SOUND_GAME_MESSAGE);
    else if (cl_chat_sound->integer > 0)
        S_StartLocalSound_(ASSET_SOUND_CHAT);
}

static void CL_ParseCenterPrint(void)
{
    char s[MAX_STRING_CHARS];

    MSG_ReadString(s, sizeof(s));
    SHOWNET(2, "    \"%s\"\n", s);
    SCR_CenterPrint(s);

    if (!cls.demo.playback) {
        COM_strclr(s);
        Cmd_ExecTrigger(s);
    }
}

static void CL_ParseStuffText(void)
{
    char s[MAX_STRING_CHARS];

    MSG_ReadString(s, sizeof(s));
    SHOWNET(2, "    \"%s\"\n", s);
    Cbuf_AddText(&cl_cmdbuf, s);
}

static void CL_ParseLayout(void)
{
    MSG_ReadString(cl.layout, sizeof(cl.layout));
    SHOWNET(2, "    \"%s\"\n", cl.layout);
}

static void CL_ParseInventory(void)
{
    for (int i = 0; i < cl.num_items; i++) {
        cl.inventory[i] = MSG_ReadShort();
    }
}

static void CL_ParseDownload(int cmd)
{
    int size, percent, decompressed_size;
    byte *data;

    if (!cls.download.temp[0]) {
        Com_Errorf(ERR_DROP, "%s: no download requested", __func__);
    }

    // read the data
    size = MSG_ReadShort();
    percent = MSG_ReadByte();
    if (size == -1) {
        CL_HandleDownload(NULL, size, percent, 0);
        return;
    }

    // read optional decompressed packet size
    if (cmd == svc_zdownload) {
        decompressed_size = -1;
    } else {
        decompressed_size = 0;
    }

    if (size < 0) {
        Com_Errorf(ERR_DROP, "%s: bad size: %d", __func__, size);
    }

    if (msg_read.readcount + size > msg_read.cursize) {
        Com_Errorf(ERR_DROP, "%s: read past end of message", __func__);
    }

    data = msg_read.data + msg_read.readcount;
    msg_read.readcount += size;

    CL_HandleDownload(data, size, percent, decompressed_size);
}

static void CL_ParseZPacket(void)
{
    sizebuf_t   temp;
    byte        buffer[MAX_MSGLEN];
    int         ret, inlen, outlen;

    if (msg_read.data != msg_read_buffer) {
        Com_Errorf(ERR_DROP, "%s: recursively entered", __func__);
    }

    inlen = MSG_ReadWord();
    outlen = MSG_ReadWord();

    if (inlen == -1 || outlen == -1 || msg_read.readcount + inlen > msg_read.cursize) {
        Com_Errorf(ERR_DROP, "%s: read past end of message", __func__);
    }

    if (outlen > MAX_MSGLEN) {
        Com_Errorf(ERR_DROP, "%s: invalid output length", __func__);
    }

    inflateReset(&cls.z);

    cls.z.next_in = msg_read.data + msg_read.readcount;
    cls.z.avail_in = (uInt)inlen;
    cls.z.next_out = buffer;
    cls.z.avail_out = (uInt)outlen;
    ret = inflate(&cls.z, Z_FINISH);
    if (ret != Z_STREAM_END) {
        Com_Errorf(ERR_DROP, "%s: inflate() failed with error %d", __func__, ret);
    }

    msg_read.readcount += inlen;

    temp = msg_read;
    SZ_Init(&msg_read, buffer, outlen);
    msg_read.cursize = outlen;

    CL_ParseServerMessage();

    msg_read = temp;
}

static void CL_ParseSetting(void)
{
    int index q_unused;
    int value q_unused;

    index = MSG_ReadLong();
    value = MSG_ReadLong();

    switch (index) {
    default:
        break;
    }
}

/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage(void)
{
    int         cmd, extrabits;
    size_t      readcount;
    int         index;

#if USE_DEBUG
    if (cl_shownet->integer == 1) {
        Com_LPrintf(PRINT_DEVELOPER, "%zu ", msg_read.cursize);
    } else if (cl_shownet->integer > 1) {
        Com_LPrintf(PRINT_DEVELOPER, "------------------\n");
    }
#endif

//
// parse the message
//
    while (1) {
        if (msg_read.readcount > msg_read.cursize) {
            Com_Errorf(ERR_DROP, "%s: read past end of server message", __func__);
        }

        readcount = msg_read.readcount;

        if ((cmd = MSG_ReadByte()) == -1) {
            SHOWNET(1, "%3zu:END OF MESSAGE\n", msg_read.readcount - 1);
            break;
        }

        extrabits = cmd >> SVCMD_BITS;
        cmd &= SVCMD_MASK;

#if USE_DEBUG
        if (cl_shownet->integer > 1) {
            MSG_ShowSVC(cmd);
        }
#endif

        // other commands
        switch (cmd) {
        default:
            Com_Errorf(ERR_DROP, "%s: illegible server message: %d", __func__, cmd);
            break;

        case svc_nop:
            break;

        case svc_disconnect:
            Com_Error(ERR_DISCONNECT, "Server disconnected");
            break;

        case svc_reconnect:
            CL_ParseReconnect();
            return;

        case svc_print:
            CL_ParsePrint();
            break;

        case svc_centerprint:
            CL_ParseCenterPrint();
            break;

        case svc_stufftext:
            CL_ParseStuffText();
            break;

        case svc_serverdata:
            CL_ParseServerData();
            continue;

        case svc_configstring:
            index = MSG_ReadShort();
            CL_ParseConfigstring(index);
            break;

        case svc_sound:
            CL_ParseStartSoundPacket();
            S_ParseStartSound();
            break;

        case svc_temp_entity:
            CL_ParseTEntPacket();
            CL_ParseTEnt();
            break;

        case svc_muzzleflash:
            CL_ParseMuzzleFlashPacket(MZ_SILENCED);
            CL_MuzzleFlash();
            break;

        case svc_muzzleflash2:
            CL_ParseMuzzleFlashPacket(0);
            CL_MuzzleFlash2();
            break;

        case svc_download:
            CL_ParseDownload(cmd);
            continue;

        case svc_frame:
            CL_ParseFrame(extrabits);
            continue;

        case svc_ambient:
            CL_ParseAmbients();
            break;

        case svc_inventory:
            CL_ParseInventory();
            break;

        case svc_layout:
            CL_ParseLayout();
            break;

        case svc_zpacket:
            CL_ParseZPacket();
            continue;

        case svc_zdownload:
            CL_ParseDownload(cmd);
            continue;

        case svc_gamestate:
            CL_ParseGamestate();
            continue;

        case svc_setting:
            CL_ParseSetting();
            continue;
        }

        // if recording demos, copy off protocol invariant stuff
        if (cls.demo.recording && !cls.demo.paused) {
            size_t len = msg_read.readcount - readcount;

            // it is very easy to overflow standard 1390 bytes
            // demo frame with modern servers... attempt to preserve
            // reliable messages at least, assuming they come first
            if (cls.demo.buffer.cursize + len < cls.demo.buffer.maxsize) {
                SZ_Write(&cls.demo.buffer, msg_read.data + readcount, len);
            } else {
                cls.demo.others_dropped++;
            }
        }
    }
}

/*
=====================
CL_SeekDemoMessage

A variant of ParseServerMessage that skips over non-important action messages,
used for seeking in demos.
=====================
*/
void CL_SeekDemoMessage(void)
{
    int         cmd, extrabits;
    int         index;

#if USE_DEBUG
    if (cl_shownet->integer == 1) {
        Com_LPrintf(PRINT_DEVELOPER, "%zu ", msg_read.cursize);
    } else if (cl_shownet->integer > 1) {
        Com_LPrintf(PRINT_DEVELOPER, "------------------\n");
    }
#endif

//
// parse the message
//
    while (1) {
        if (msg_read.readcount > msg_read.cursize) {
            Com_Errorf(ERR_DROP, "%s: read past end of server message", __func__);
        }

        if ((cmd = MSG_ReadByte()) == -1) {
            SHOWNET(1, "%3zu:END OF MESSAGE\n", msg_read.readcount - 1);
            break;
        }

        extrabits = cmd >> SVCMD_BITS;
        cmd &= SVCMD_MASK;

#if USE_DEBUG
        if (cl_shownet->integer > 1) {
            MSG_ShowSVC(cmd);
        }
#endif

        // other commands
        switch (cmd) {
        default:
            Com_Errorf(ERR_DROP, "%s: illegible server message: %d", __func__, cmd);
            break;

        case svc_nop:
            break;

        case svc_disconnect:
        case svc_reconnect:
            Com_Error(ERR_DISCONNECT, "Server disconnected");
            break;

        case svc_print:
            MSG_ReadByte();
            // fall through

        case svc_centerprint:
        case svc_stufftext:
            MSG_ReadString(NULL, 0);
            break;

        case svc_configstring:
            index = MSG_ReadShort();
            CL_ParseConfigstring(index);
            break;

        case svc_sound:
            CL_ParseStartSoundPacket();
            break;

        case svc_temp_entity:
            CL_ParseTEntPacket();
            break;

        case svc_muzzleflash:
        case svc_muzzleflash2:
            CL_ParseMuzzleFlashPacket(0);
            break;

        case svc_frame:
            CL_ParseFrame(extrabits);
            continue;

        case svc_ambient:
            CL_ParseAmbients();
            continue;

        case svc_inventory:
            CL_ParseInventory();
            break;

        case svc_layout:
            CL_ParseLayout();
            break;

        }
    }
}
