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
#include "common/protocol.h"
#include "common/sizebuf.h"
#include "common/intreadwrite.h"

void SZ_TagInit(sizebuf_t *buf, void *data, size_t size, uint32_t tag)
{
    memset(buf, 0, sizeof(*buf));
    buf->data = data;
    buf->maxsize = size;
    buf->tag = tag;
}

void SZ_Init(sizebuf_t *buf, void *data, size_t size)
{
    memset(buf, 0, sizeof(*buf));
    buf->data = data;
    buf->maxsize = size;
    buf->allowoverflow = true;
    buf->allowunderflow = true;
}

void SZ_InitRead(sizebuf_t *buf, void *data, size_t size)
{
    memset(buf, 0, sizeof(*buf));
    buf->data = data;
    buf->maxsize = buf->cursize = size;
}

void SZ_Clear(sizebuf_t *buf)
{
    buf->cursize = 0;
    buf->readcount = 0;
    buf->bitpos = 0;
    buf->overflowed = false;
}

void *SZ_GetSpace(sizebuf_t *buf, size_t len)
{
    void    *data;

    if (buf->cursize > buf->maxsize) {
        Com_Errorf(ERR_FATAL,
                  "%s: %#x: already overflowed",
                  __func__, buf->tag);
    }

    if (len > buf->maxsize - buf->cursize) {
        if (len > buf->maxsize) {
            Com_Errorf(ERR_FATAL,
                      "%s: %#x: %zu is > full buffer size %zu",
                      __func__, buf->tag, len, buf->maxsize);
        }

        if (!buf->allowoverflow) {
            Com_Errorf(ERR_FATAL,
                      "%s: %#x: overflow without allowoverflow set",
                      __func__, buf->tag);
        }

        //Com_DPrintf("%s: %#x: overflow\n", __func__, buf->tag);
        SZ_Clear(buf);
        buf->overflowed = true;
    }

    data = buf->data + buf->cursize;
    buf->cursize += len;
    buf->bitpos = buf->cursize << 3;
    return data;
}

void SZ_WriteByte(sizebuf_t *sb, int c)
{
    byte    *buf;

    buf = SZ_GetSpace(sb, 1);
    buf[0] = c;
}

void SZ_WriteShort(sizebuf_t *sb, int c)
{
    byte    *buf;

    buf = SZ_GetSpace(sb, 2);
    buf[0] = c & 0xff;
    buf[1] = c >> 8;
}

void SZ_WriteLong(sizebuf_t *sb, int c)
{
    byte    *buf;

    buf = SZ_GetSpace(sb, 4);
    buf[0] = c & 0xff;
    buf[1] = (c >> 8) & 0xff;
    buf[2] = (c >> 16) & 0xff;
    buf[3] = c >> 24;
}

void SZ_BeginReading(sizebuf_t *sb)
{
    sb->readcount = 0;
    sb->bitpos = 0;
}

byte *SZ_ReadData(sizebuf_t *sb, size_t len)
{
    byte *buf = sb->data + sb->readcount;

    sb->readcount += len;
    sb->bitpos = sb->readcount << 3;

    if (sb->readcount > sb->cursize) {
        if (!sb->allowunderflow) {
            Com_Errorf(ERR_DROP, "%s: read past end of message", __func__);
        }
        return NULL;
    }

    return buf;
}

int SZ_ReadByte(sizebuf_t *sb)
{
    byte *buf = SZ_ReadData(sb, 1);
    int c;

    if (!buf) {
        c = -1;
    } else {
        c = (unsigned char)buf[0];
    }

    return c;
}

int SZ_ReadShort(sizebuf_t *sb)
{
    byte *buf = SZ_ReadData(sb, 2);
    int c;

    if (!buf) {
        c = -1;
    } else {
        c = (signed short)RL16(buf);
    }

    return c;
}

int SZ_ReadLong(sizebuf_t *sb)
{
    byte *buf = SZ_ReadData(sb, 4);
    int c;

    if (!buf) {
        c = -1;
    } else {
        c = RL32(buf);
    }

    return c;
}