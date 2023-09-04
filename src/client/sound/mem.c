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
// snd_mem.c: sound caching

#include "sound.h"
#include "common/intreadwrite.h"

wavinfo_t s_info;


#ifndef USE_LITTLE_ENDIAN
#define USE_LITTLE_ENDIAN 0
#endif

/*
===============================================================================

WAV loading

===============================================================================
*/

static byte     *data_p;
static byte     *iff_end;
static byte     *iff_data;
static uint32_t iff_chunk_len;

static int GetLittleShort(void)
{
    int val;

    if (data_p + 2 > iff_end) {
        return -1;
    }

    val = RL16(data_p);
    data_p += 2;
    return val;
}

static int GetLittleLong(void)
{
    int val;

    if (data_p + 4 > iff_end) {
        return -1;
    }

    val = RL32(data_p);
    data_p += 4;
    return val;
}

static void FindNextChunk(uint32_t search)
{
    uint32_t chunk, len;
    size_t remaining;

    while (data_p + 8 < iff_end) {
        chunk = RL32(data_p); data_p += 4;
        len = RL32(data_p); data_p += 4;
        remaining = (size_t)(iff_end - data_p);
        if (len > remaining) {
            len = remaining;
        }
        if (chunk == search) {
            iff_chunk_len = len;
            return;
        }
        data_p += ALIGN(len, 2);
    }

    // didn't find the chunk
    data_p = NULL;
}

static void FindChunk(uint32_t search)
{
    data_p = iff_data;
    FindNextChunk(search);
}

#define TAG_RIFF    MakeLittleLong('R', 'I', 'F', 'F')
#define TAG_WAVE    MakeLittleLong('W', 'A', 'V', 'E')
#define TAG_fmt     MakeLittleLong('f', 'm', 't', ' ')
#define TAG_cue     MakeLittleLong('c', 'u', 'e', ' ')
#define TAG_LIST    MakeLittleLong('L', 'I', 'S', 'T')
#define TAG_MARK    MakeLittleLong('M', 'A', 'R', 'K')
#define TAG_data    MakeLittleLong('d', 'a', 't', 'a')

static bool GetWavinfo(void)
{
    int format;
    int samples, width;
    uint32_t chunk;

// find "RIFF" chunk
    FindChunk(TAG_RIFF);
    if (!data_p) {
        Com_DPrintf("%s has missing/invalid RIFF chunk\n", s_info.name);
        return false;
    }
    chunk = GetLittleLong();
    if (chunk != TAG_WAVE) {
        Com_DPrintf("%s has missing/invalid WAVE chunk\n", s_info.name);
        return false;
    }

    iff_data = data_p;

// get "fmt " chunk
    FindChunk(TAG_fmt);
    if (!data_p) {
        Com_DPrintf("%s has missing/invalid fmt chunk\n", s_info.name);
        return false;
    }
    format = GetLittleShort();
    if (format != 1) {
        Com_DPrintf("%s has non-Microsoft PCM format\n", s_info.name);
        return false;
    }

    format = GetLittleShort();
    if (format != 1) {
        Com_DPrintf("%s has bad number of channels\n", s_info.name);
        return false;
    }

    s_info.rate = GetLittleLong();
    if (s_info.rate < 8000 || s_info.rate > 48000) {
        Com_DPrintf("%s has bad rate\n", s_info.name);
        return false;
    }

    data_p += 4 + 2;

    width = GetLittleShort();
    switch (width) {
    case 8:
        s_info.width = 1;
        break;
    case 16:
        s_info.width = 2;
        break;
    default:
        Com_DPrintf("%s has bad width\n", s_info.name);
        return false;
    }

// get cue chunk
    FindChunk(TAG_cue);
    if (data_p) {
        data_p += 24;
        s_info.loopstart = GetLittleLong();
        if (s_info.loopstart < 0 || s_info.loopstart > INT_MAX) {
            Com_DPrintf("%s has bad loop start\n", s_info.name);
            return false;
        }

        FindNextChunk(TAG_LIST);
        if (data_p) {
            data_p += 20;
            chunk = GetLittleLong();
            if (chunk == TAG_MARK) {
                // this is not a proper parse, but it works with cooledit...
                data_p += 16;
                samples = GetLittleLong();    // samples in loop
                if (samples < 0 || samples > INT_MAX - s_info.loopstart) {
                    Com_DPrintf("%s has bad loop length\n", s_info.name);
                    return false;
                }
                s_info.samples = s_info.loopstart + samples;
            }
        }
    } else {
        s_info.loopstart = -1;
    }

// find data chunk
    FindChunk(TAG_data);
    if (!data_p) {
        Com_DPrintf("%s has missing/invalid data chunk\n", s_info.name);
        return false;
    }

    samples = iff_chunk_len / s_info.width;
    if (!samples) {
        Com_DPrintf("%s has zero length\n", s_info.name);
        return false;
    }

    if (s_info.samples) {
        if (samples < s_info.samples) {
            Com_DPrintf("%s has bad loop length\n", s_info.name);
            return false;
        }
    } else {
        s_info.samples = samples;
    }

    s_info.data = data_p;

    return true;
}

/*
==============
S_LoadSound
==============
*/
sfxcache_t *S_LoadSound(sfx_t *s)
{
    byte        *data;
    sfxcache_t  *sc;
    int         len;
    char        *name;

    if (s->name[0] == '*')
        return NULL;

// see if still in memory
    sc = s->cache;
    if (sc)
        return sc;

// don't retry after error
    if (s->error)
        return NULL;

// load it in
    if (s->truename)
        name = s->truename;
    else
        name = s->name;

    len = FS_LoadFile(name, (void **)&data);
    if (!data) {
        s->error = len;
        return NULL;
    }

    memset(&s_info, 0, sizeof(s_info));
    s_info.name = name;

    iff_data = data;
    iff_end = data + len;
    if (!GetWavinfo()) {
        s->error = Q_ERR_INVALID_FORMAT;
        goto fail;
    }

    if (s_started == SS_OAL)
        sc = AL_UploadSfx(s);

fail:
    FS_FreeFile(data);
    return sc;
}

