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
// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

#include "client.h"
#include "refresh/images.h"

#define STAT_PICS       11
#define STAT_MINUS      (STAT_PICS - 1)  // num frame for '-' stats digit

typedef enum {
    // just so zero is never a valid opcode
    HUD_OP_NOP,

    // positioning
    HUD_OP_XL, // <x: int>
    HUD_OP_XR, // <x: int>
    HUD_OP_YT, // <y: int>
    HUD_OP_YB, // <y: int>
    HUD_OP_XV, // <x: int>,
    HUD_OP_YV, // <y: int>,

    HUD_OP_PICN, // <s: string_int>
    HUD_OP_HNUM,
    HUD_OP_ANUM,
    HUD_OP_RNUM,
    HUD_OP_CSTRING, // <s: string_int>
    HUD_OP_CSTRING2, // <s: string_int>

    HUD_OP_STRING, // <s: string_int>
    HUD_OP_STRING2, // <s: string_int>

    HUD_OP_COLOR, // <r: byte> <g: byte> <b: byte> <a: byte>

    HUD_OP_PIC, // <stat: byte>
    HUD_OP_NUM, // <width: byte> <stat: byte>
    HUD_OP_STAT_STRING, // <stat: byte>
    HUD_OP_STAT_STRING2, // <stat: byte>

    HUD_OP_IF, // <stat: byte> <loc_false: int>
} hud_opcode_t;

typedef struct {
    char key[MAX_OSPATH];
    char value[MAX_OSPATH];
} hud_macro_t;

typedef struct hud_control_s {
    size_t p;
    struct hud_control_s *next;
} hud_control_t;

typedef struct {
    byte *bytecode; // bytecode
    size_t bytecode_length;
    size_t bytecode_pos;
    char *string_data; // constant string data
    size_t string_data_length, string_count, string_len;

    // during compilation only
    hud_macro_t *constants;
    size_t allocated_constants;
    size_t num_constants;
    hud_control_t *control_head;
} hud_script_t;

static struct {
    bool        initialized;        // ready to draw

    qhandle_t   crosshair_pic;
    int         crosshair_width, crosshair_height;
    color_t     crosshair_color;

    qhandle_t   pause_pic;
    int         pause_width, pause_height;

    qhandle_t   loading_pic;
    int         loading_width, loading_height;
    bool        draw_loading;

    qhandle_t   sb_pics[2][STAT_PICS];
    qhandle_t   inven_pic;
    qhandle_t   field_pic;

    qhandle_t   backtile_pic;

    qhandle_t   net_pic;
    qhandle_t   font_pic;

    int         hud_width, hud_height;
    float       hud_scale;
    float       hud_alpha;

    hud_script_t *hud_script;
} scr;

cvar_t   *scr_viewsize;
static cvar_t   *scr_centertime;
static cvar_t   *scr_showpause;
#if USE_DEBUG
static cvar_t   *scr_showstats;
static cvar_t   *scr_showpmove;
#endif
static cvar_t   *scr_showturtle;
static cvar_t   *scr_showitemname;

static cvar_t   *scr_draw2d;
static cvar_t   *scr_lag_x;
static cvar_t   *scr_lag_y;
static cvar_t   *scr_lag_draw;
static cvar_t   *scr_lag_min;
static cvar_t   *scr_lag_max;
static cvar_t   *scr_alpha;
static cvar_t   *scr_fps;

static cvar_t   *scr_demobar;
static cvar_t   *scr_font;
static cvar_t   *scr_hud;
static cvar_t   *scr_scale;

static cvar_t   *scr_crosshair;

static cvar_t   *scr_chathud;
static cvar_t   *scr_chathud_lines;
static cvar_t   *scr_chathud_time;
static cvar_t   *scr_chathud_x;
static cvar_t   *scr_chathud_y;

static cvar_t   *ch_health;
static cvar_t   *ch_red;
static cvar_t   *ch_green;
static cvar_t   *ch_blue;
static cvar_t   *ch_alpha;

static cvar_t   *ch_scale;
static cvar_t   *ch_x;
static cvar_t   *ch_y;

vrect_t     scr_vrect;      // position of render window on screen

static const char *const sb_nums[2][STAT_PICS] = {
    {
        "num_0", "num_1", "num_2", "num_3", "num_4", "num_5",
        "num_6", "num_7", "num_8", "num_9", "num_minus"
    },
    {
        "anum_0", "anum_1", "anum_2", "anum_3", "anum_4", "anum_5",
        "anum_6", "anum_7", "anum_8", "anum_9", "anum_minus"
    }
};

const uint32_t colorTable[8] = {
    U32_BLACK, U32_RED, U32_GREEN, U32_YELLOW,
    U32_BLUE, U32_CYAN, U32_MAGENTA, U32_WHITE
};

/*
===============================================================================

UTILS

===============================================================================
*/

#define SCR_DrawString(x, y, flags, string) \
    SCR_DrawStringEx(x, y, flags, MAX_STRING_CHARS, string, scr.font_pic)

/*
==============
SCR_DrawStringEx
==============
*/
int SCR_DrawStringEx(int x, int y, int flags, size_t maxlen,
                     const char *s, qhandle_t font)
{
    size_t len = strlen(s);

    if (len > maxlen) {
        len = maxlen;
    }

    if ((flags & UI_CENTER) == UI_CENTER) {
        x -= len * CHAR_WIDTH / 2;
    } else if (flags & UI_RIGHT) {
        x -= len * CHAR_WIDTH;
    }

    return R_DrawString(x, y, flags, maxlen, s, font);
}


/*
==============
SCR_DrawStringMulti
==============
*/
void SCR_DrawStringMulti(int x, int y, int flags, size_t maxlen,
                         const char *s, qhandle_t font)
{
    char    *p;
    size_t  len;

    while (*s) {
        p = strchr(s, '\n');
        if (!p) {
            SCR_DrawStringEx(x, y, flags, maxlen, s, font);
            break;
        }

        len = p - s;
        if (len > maxlen) {
            len = maxlen;
        }
        SCR_DrawStringEx(x, y, flags, len, s, font);

        y += CHAR_HEIGHT;
        s = p + 1;
    }
}


/*
=================
SCR_FadeAlpha
=================
*/
float SCR_FadeAlpha(unsigned startTime, unsigned visTime, unsigned fadeTime)
{
    float alpha;
    unsigned timeLeft, delta = cls.realtime - startTime;

    if (delta >= visTime) {
        return 0;
    }

    if (fadeTime > visTime) {
        fadeTime = visTime;
    }

    alpha = 1;
    timeLeft = visTime - delta;
    if (timeLeft < fadeTime) {
        alpha = (float)timeLeft / fadeTime;
    }

    return alpha;
}

bool SCR_ParseColor(const char *s, color_t *color)
{
    int i;
    int c[8];

    // parse generic color
    if (*s == '#') {
        s++;
        for (i = 0; s[i]; i++) {
            if (i == 8) {
                return false;
            }
            c[i] = Q_charhex(s[i]);
            if (c[i] == -1) {
                return false;
            }
        }

        switch (i) {
        case 3:
            color->u8[0] = c[0] | (c[0] << 4);
            color->u8[1] = c[1] | (c[1] << 4);
            color->u8[2] = c[2] | (c[2] << 4);
            color->u8[3] = 255;
            break;
        case 4:
            color->u8[0] = c[0] | (c[0] << 4);
            color->u8[1] = c[1] | (c[1] << 4);
            color->u8[2] = c[2] | (c[2] << 4);
            color->u8[3] = c[3] | (c[3] << 4);
            break;
        case 6:
            color->u8[0] = c[1] | (c[0] << 4);
            color->u8[1] = c[3] | (c[2] << 4);
            color->u8[2] = c[5] | (c[4] << 4);
            color->u8[3] = 255;
            break;
        case 8:
            color->u8[0] = c[1] | (c[0] << 4);
            color->u8[1] = c[3] | (c[2] << 4);
            color->u8[2] = c[5] | (c[4] << 4);
            color->u8[3] = c[7] | (c[6] << 4);
            break;
        default:
            return false;
        }

        return true;
    }

    // parse name or index
    i = Com_ParseColor(s, COLOR_WHITE);
    if (i == COLOR_NONE) {
        return false;
    }

    color->u32 = colorTable[i];
    return true;
}

/*
===============================================================================

BAR GRAPHS

===============================================================================
*/

static void draw_percent_bar(int percent, bool paused, int framenum)
{
    char buffer[16];
    int x, w;
    size_t len;

    scr.hud_height -= CHAR_HEIGHT;

    w = scr.hud_width * percent / 100;

    R_DrawFill8(0, scr.hud_height, w, CHAR_HEIGHT, 4);
    R_DrawFill8(w, scr.hud_height, scr.hud_width - w, CHAR_HEIGHT, 0);

    len = Q_snprintf(buffer, sizeof(buffer), "%d%%", percent);
    x = (scr.hud_width - len * CHAR_WIDTH) / 2;
    R_DrawString(x, scr.hud_height, 0, MAX_STRING_CHARS, buffer, scr.font_pic);

    if (scr_demobar->integer > 1) {
        int sec = framenum / 10;
        int min = sec / 60; sec %= 60;

        Q_snprintf(buffer, sizeof(buffer), "%d:%02d.%d", min, sec, framenum % 10);
        R_DrawString(0, scr.hud_height, 0, MAX_STRING_CHARS, buffer, scr.font_pic);
    }

    if (paused) {
        SCR_DrawString(scr.hud_width, scr.hud_height, UI_RIGHT, "[PAUSED]");
    }
}

static void SCR_DrawDemo(void)
{
    if (!scr_demobar->integer) {
        return;
    }

    if (cls.demo.playback) {
        if (cls.demo.file_size) {
            draw_percent_bar(
                cls.demo.file_percent,
                sv_paused->integer &&
                cl_paused->integer &&
                scr_showpause->integer == 2,
                cls.demo.frames_read);
        }
        return;
    }
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

static char     scr_centerstring[MAX_STRING_CHARS];
static unsigned scr_centertime_start;   // for slow victory printing
static int      scr_center_lines;

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint(const char *str)
{
    const char  *s;

    scr_centertime_start = cls.realtime;
    if (!strcmp(scr_centerstring, str)) {
        return;
    }

    Q_strlcpy(scr_centerstring, str, sizeof(scr_centerstring));

    // count the number of lines for centering
    scr_center_lines = 1;
    s = str;
    while (*s) {
        if (*s == '\n')
            scr_center_lines++;
        s++;
    }

    // echo it to the console
    Com_Printf("%s\n", scr_centerstring);
    Con_ClearNotify_f();
}

static void SCR_DrawCenterString(void)
{
    int y;
    float alpha;

    Cvar_ClampValue(scr_centertime, 0.3f, 10.0f);

    alpha = SCR_FadeAlpha(scr_centertime_start, scr_centertime->value * 1000, 300);
    if (!alpha) {
        return;
    }

    R_SetAlpha(alpha * scr_alpha->value);

    y = scr.hud_height / 4 - scr_center_lines * 8 / 2;

    SCR_DrawStringMulti(scr.hud_width / 2, y, UI_CENTER,
                        MAX_STRING_CHARS, scr_centerstring, scr.font_pic);

    R_SetAlpha(scr_alpha->value);
}

/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define LAG_WIDTH   48
#define LAG_HEIGHT  48

#define LAG_CRIT_BIT    (1U << 31)
#define LAG_WARN_BIT    (1U << 30)

#define LAG_BASE    0xD5
#define LAG_WARN    0xDC
#define LAG_CRIT    0xF2

static struct {
    unsigned samples[LAG_WIDTH];
    unsigned head;
} lag;

void SCR_LagClear(void)
{
    lag.head = 0;
}

void SCR_LagSample(void)
{
    int i = cls.netchan->incoming_acknowledged & CMD_MASK;
    client_history_t *h = &cl.history[i];
    unsigned ping;

    h->rcvd = cls.realtime;
    if (!h->cmdNumber || h->rcvd < h->sent) {
        return;
    }

    ping = h->rcvd - h->sent;
    for (i = 0; i < cls.netchan->dropped; i++) {
        lag.samples[lag.head % LAG_WIDTH] = ping | LAG_CRIT_BIT;
        lag.head++;
    }

    if (cl.frameflags & FF_SUPPRESSED) {
        ping |= LAG_WARN_BIT;
    }
    lag.samples[lag.head % LAG_WIDTH] = ping;
    lag.head++;
}

static void SCR_LagDraw(int x, int y)
{
    int i, j, v, c, v_min, v_max, v_range;

    v_min = Cvar_ClampInteger(scr_lag_min, 0, LAG_HEIGHT * 10);
    v_max = Cvar_ClampInteger(scr_lag_max, 0, LAG_HEIGHT * 10);

    v_range = v_max - v_min;
    if (v_range < 1)
        return;

    for (i = 0; i < LAG_WIDTH; i++) {
        j = lag.head - i - 1;
        if (j < 0) {
            break;
        }

        v = lag.samples[j % LAG_WIDTH];

        if (v & LAG_CRIT_BIT) {
            c = LAG_CRIT;
        } else if (v & LAG_WARN_BIT) {
            c = LAG_WARN;
        } else {
            c = LAG_BASE;
        }

        v &= ~(LAG_WARN_BIT | LAG_CRIT_BIT);
        v = (v - v_min) * LAG_HEIGHT / v_range;
        clamp(v, 0, LAG_HEIGHT);

        R_DrawFill8(x + LAG_WIDTH - i - 1, y + LAG_HEIGHT - v, 1, v, c);
    }
}

static void SCR_DrawNet(void)
{
    int x = scr_lag_x->integer;
    int y = scr_lag_y->integer;

    if (x < 0) {
        x += scr.hud_width - LAG_WIDTH + 1;
    }
    if (y < 0) {
        y += scr.hud_height - LAG_HEIGHT + 1;
    }

    // draw ping graph
    if (scr_lag_draw->integer) {
        if (scr_lag_draw->integer > 1) {
            R_DrawFill8(x, y, LAG_WIDTH, LAG_HEIGHT, 4);
        }
        SCR_LagDraw(x, y);
    }

    // draw phone jack
    if (cls.netchan && cls.netchan->outgoing_sequence - cls.netchan->incoming_acknowledged >= CMD_BACKUP) {
        if ((cls.realtime >> 8) & 3) {
            R_DrawStretchPic(x, y, LAG_WIDTH, LAG_HEIGHT, scr.net_pic);
        }
    }
}


/*
===============================================================================

DRAW OBJECTS

===============================================================================
*/

typedef struct {
    list_t          entry;
    int             x, y;
    cvar_t          *cvar;
    cmd_macro_t     *macro;
    int             flags;
    color_t         color;
} drawobj_t;

#define FOR_EACH_DRAWOBJ(obj) \
    LIST_FOR_EACH(drawobj_t, obj, &scr_objects, entry)
#define FOR_EACH_DRAWOBJ_SAFE(obj, next) \
    LIST_FOR_EACH_SAFE(drawobj_t, obj, next, &scr_objects, entry)

static LIST_DECL(scr_objects);

static void SCR_Color_g(genctx_t *ctx)
{
    int color;

    for (color = 0; color < 10; color++)
        Prompt_AddMatch(ctx, colorNames[color]);
}

static void SCR_Draw_c(genctx_t *ctx, int argnum)
{
    if (argnum == 1) {
        Cvar_Variable_g(ctx);
        Cmd_Macro_g(ctx);
    } else if (argnum == 4) {
        SCR_Color_g(ctx);
    }
}

// draw cl_fps -1 80
static void SCR_Draw_f(void)
{
    int x, y;
    const char *s, *c;
    drawobj_t *obj;
    cmd_macro_t *macro;
    cvar_t *cvar;
    color_t color;
    int flags;
    int argc = Cmd_Argc();

    if (argc == 1) {
        if (LIST_EMPTY(&scr_objects)) {
            Com_Printf("No draw strings registered.\n");
            return;
        }
        Com_Printf("Name               X    Y\n"
                   "--------------- ---- ----\n");
        FOR_EACH_DRAWOBJ(obj) {
            s = obj->macro ? obj->macro->name : obj->cvar->name;
            Com_Printf("%-15s %4d %4d\n", s, obj->x, obj->y);
        }
        return;
    }

    if (argc < 4) {
        Com_Printf("Usage: %s <name> <x> <y> [color]\n", Cmd_Argv(0));
        return;
    }

    color.u32 = U32_BLACK;
    flags = UI_IGNORECOLOR;

    s = Cmd_Argv(1);
    x = atoi(Cmd_Argv(2));
    y = atoi(Cmd_Argv(3));

    if (x < 0) {
        flags |= UI_RIGHT;
    }

    if (argc > 4) {
        c = Cmd_Argv(4);
        if (!strcmp(c, "alt")) {
            flags |= UI_ALTCOLOR;
        } else if (strcmp(c, "none")) {
            if (!SCR_ParseColor(c, &color)) {
                Com_Printf("Unknown color '%s'\n", c);
                return;
            }
            flags &= ~UI_IGNORECOLOR;
        }
    }

    cvar = NULL;
    macro = Cmd_FindMacro(s);
    if (!macro) {
        cvar = Cvar_WeakGet(s);
    }

    FOR_EACH_DRAWOBJ(obj) {
        if (obj->macro == macro && obj->cvar == cvar) {
            obj->x = x;
            obj->y = y;
            obj->flags = flags;
            obj->color.u32 = color.u32;
            return;
        }
    }

    obj = Z_Malloc(sizeof(*obj));
    obj->x = x;
    obj->y = y;
    obj->cvar = cvar;
    obj->macro = macro;
    obj->flags = flags;
    obj->color.u32 = color.u32;

    List_Append(&scr_objects, &obj->entry);
}

static void SCR_Draw_g(genctx_t *ctx)
{
    drawobj_t *obj;
    const char *s;

    if (LIST_EMPTY(&scr_objects)) {
        return;
    }

    Prompt_AddMatch(ctx, "all");

    FOR_EACH_DRAWOBJ(obj) {
        s = obj->macro ? obj->macro->name : obj->cvar->name;
        Prompt_AddMatch(ctx, s);
    }
}

static void SCR_UnDraw_c(genctx_t *ctx, int argnum)
{
    if (argnum == 1) {
        SCR_Draw_g(ctx);
    }
}

static void SCR_UnDraw_f(void)
{
    char *s;
    drawobj_t *obj, *next;
    cmd_macro_t *macro;
    cvar_t *cvar;

    if (Cmd_Argc() != 2) {
        Com_Printf("Usage: %s <name>\n", Cmd_Argv(0));
        return;
    }

    if (LIST_EMPTY(&scr_objects)) {
        Com_Printf("No draw strings registered.\n");
        return;
    }

    s = Cmd_Argv(1);
    if (!strcmp(s, "all")) {
        FOR_EACH_DRAWOBJ_SAFE(obj, next) {
            Z_Free(obj);
        }
        List_Init(&scr_objects);
        Com_Printf("Deleted all draw strings.\n");
        return;
    }

    cvar = NULL;
    macro = Cmd_FindMacro(s);
    if (!macro) {
        cvar = Cvar_WeakGet(s);
    }

    FOR_EACH_DRAWOBJ_SAFE(obj, next) {
        if (obj->macro == macro && obj->cvar == cvar) {
            List_Remove(&obj->entry);
            Z_Free(obj);
            return;
        }
    }

    Com_Printf("Draw string '%s' not found.\n", s);
}

static void SCR_DrawObjects(void)
{
    char buffer[MAX_QPATH];
    int x, y;
    drawobj_t *obj;

    FOR_EACH_DRAWOBJ(obj) {
        x = obj->x;
        y = obj->y;
        if (x < 0) {
            x += scr.hud_width + 1;
        }
        if (y < 0) {
            y += scr.hud_height - CHAR_HEIGHT + 1;
        }
        if (!(obj->flags & UI_IGNORECOLOR)) {
            R_SetColor(obj->color.u32);
        }
        if (obj->macro) {
            obj->macro->function(buffer, sizeof(buffer));
            SCR_DrawString(x, y, obj->flags, buffer);
        } else {
            SCR_DrawString(x, y, obj->flags, obj->cvar->string);
        }
        if (!(obj->flags & UI_IGNORECOLOR)) {
            R_ClearColor();
            R_SetAlpha(scr_alpha->value);
        }
    }
}

extern int CL_GetFps(void);
extern int CL_GetResolutionScale(void);

static void SCR_DrawFPS(void)
{
	if (scr_fps->integer == 0)
		return;

	int fps = R_FPS;
	int scale = CL_GetResolutionScale();

	char buffer[MAX_QPATH];
	if (scr_fps->integer == 2)
		Q_snprintf(buffer, MAX_QPATH, "%d FPS at %3d%%", fps, scale);
	else
		Q_snprintf(buffer, MAX_QPATH, "%d FPS", fps);

	int x = scr.hud_width - 2;
	int y = 1;

	R_SetColor(~0u);
	SCR_DrawString(x, y, UI_RIGHT, buffer);
}

/*
===============================================================================

CHAT HUD

===============================================================================
*/

#define MAX_CHAT_TEXT       150
#define MAX_CHAT_LINES      32
#define CHAT_LINE_MASK      (MAX_CHAT_LINES - 1)

typedef struct {
    char        text[MAX_CHAT_TEXT];
    unsigned    time;
} chatline_t;

static chatline_t   scr_chatlines[MAX_CHAT_LINES];
static unsigned     scr_chathead;

void SCR_ClearChatHUD_f(void)
{
    memset(scr_chatlines, 0, sizeof(scr_chatlines));
    scr_chathead = 0;
}

void SCR_AddToChatHUD(const char *text)
{
    chatline_t *line;
    char *p;

    line = &scr_chatlines[scr_chathead++ & CHAT_LINE_MASK];
    Q_strlcpy(line->text, text, sizeof(line->text));
    line->time = cls.realtime;

    p = strrchr(line->text, '\n');
    if (p)
        *p = 0;
}

static void SCR_DrawChatHUD(void)
{
    int x, y, i, lines, flags, step;
    float alpha;
    chatline_t *line;

    if (scr_chathud->integer == 0)
        return;

    x = scr_chathud_x->integer;
    y = scr_chathud_y->integer;

    if (scr_chathud->integer == 2)
        flags = UI_ALTCOLOR;
    else
        flags = 0;

    if (x < 0) {
        x += scr.hud_width + 1;
        flags |= UI_RIGHT;
    } else {
        flags |= UI_LEFT;
    }

    if (y < 0) {
        y += scr.hud_height - CHAR_HEIGHT + 1;
        step = -CHAR_HEIGHT;
    } else {
        step = CHAR_HEIGHT;
    }

    lines = scr_chathud_lines->integer;
    if (lines > scr_chathead)
        lines = scr_chathead;

    for (i = 0; i < lines; i++) {
        line = &scr_chatlines[(scr_chathead - i - 1) & CHAT_LINE_MASK];

        if (scr_chathud_time->integer) {
            alpha = SCR_FadeAlpha(line->time, scr_chathud_time->integer, 1000);
            if (!alpha)
                break;

            R_SetAlpha(alpha * scr_alpha->value);
            SCR_DrawString(x, y, flags, line->text);
            R_SetAlpha(scr_alpha->value);
        } else {
            SCR_DrawString(x, y, flags, line->text);
        }

        y += step;
    }
}

/*
===============================================================================

DEBUG STUFF

===============================================================================
*/

static void SCR_DrawTurtle(void)
{
    int x, y;

    if (scr_showturtle->integer <= 0)
        return;

    if (!cl.frameflags)
        return;

    x = CHAR_WIDTH;
    y = scr.hud_height - 11 * CHAR_HEIGHT;

#define DF(f) \
    if (cl.frameflags & FF_##f) { \
        SCR_DrawString(x, y, UI_ALTCOLOR, #f); \
        y += CHAR_HEIGHT; \
    }

    if (scr_showturtle->integer > 1) {
        DF(SUPPRESSED)
    }
    DF(CLIENTPRED)
    if (scr_showturtle->integer > 1) {
        DF(CLIENTDROP)
        DF(SERVERDROP)
    }
    DF(BADFRAME)
    DF(OLDFRAME)
    DF(OLDENT)
    DF(NODELTA)

#undef DF
}

#if USE_DEBUG

static void SCR_DrawDebugStats(void)
{
    char buffer[MAX_QPATH];
    int i, j;
    int x, y;

    j = scr_showstats->integer;
    if (j <= 0)
        return;

    if (j > MAX_STATS)
        j = MAX_STATS;

    x = CHAR_WIDTH;
    y = (scr.hud_height - j * CHAR_HEIGHT) / 2;
    for (i = 0; i < j; i++) {
        Q_snprintf(buffer, sizeof(buffer), "%2d: %d", i, cl.frame.ps.stats[i]);
        if (cl.oldframe.ps.stats[i] != cl.frame.ps.stats[i]) {
            R_SetColor(U32_RED);
        }
        R_DrawString(x, y, 0, MAX_STRING_CHARS, buffer, scr.font_pic);
        R_ClearColor();
        y += CHAR_HEIGHT;
    }
}

static void SCR_DrawDebugPmove(void)
{
    static const char * const types[] = {
        "NORMAL", "SPECTATOR", "DEAD", "GIB", "FREEZE"
    };
    static const char * const flags[] = {
        "DUCKED", "JUMP_HELD", "ON_GROUND",
        "TIME_WATERJUMP", "TIME_LAND", "TIME_TELEPORT",
        "NO_PREDICTION", "TELEPORT_BIT"
    };
    unsigned i, j;
    int x, y;

    if (!scr_showpmove->integer)
        return;

    x = CHAR_WIDTH;
    y = (scr.hud_height - 2 * CHAR_HEIGHT) / 2;

    i = cl.frame.ps.pmove.pm_type;
    if (i > PM_FREEZE)
        i = PM_FREEZE;

    R_DrawString(x, y, 0, MAX_STRING_CHARS, types[i], scr.font_pic);
    y += CHAR_HEIGHT;

    j = cl.frame.ps.pmove.pm_flags;
    for (i = 0; i < 8; i++) {
        if (j & (1 << i)) {
            x = R_DrawString(x, y, 0, MAX_STRING_CHARS, flags[i], scr.font_pic);
            x += CHAR_WIDTH;
        }
    }
}

#endif

//============================================================================

// Sets scr_vrect, the coordinates of the rendered window
void SCR_CalcVrect(void)
{
    scr_vrect.width = scr.hud_width;
    scr_vrect.height = scr.hud_height;
    scr_vrect.x = 0;
    scr_vrect.y = 0;
}

/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
static void SCR_SizeUp_f(void)
{
	int delta = (scr_viewsize->integer < 100) ? 5 : 10;
    Cvar_SetInteger(scr_viewsize, scr_viewsize->integer + delta, FROM_CONSOLE);
}

/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
static void SCR_SizeDown_f(void)
{
	int delta = (scr_viewsize->integer <= 100) ? 5 : 10;
	Cvar_SetInteger(scr_viewsize, scr_viewsize->integer - delta, FROM_CONSOLE);
}

/*
=================
SCR_Sky_f

Set a specific sky and rotation speed. If empty sky name is provided, falls
back to server defaults.
=================
*/
static void SCR_Sky_f(void)
{
    char    *name;
    float   rotate;
    vec3_t  axis;
    int     argc = Cmd_Argc();

    if (argc < 2) {
        Com_Printf("Usage: sky <basename> [rotate] [axis x y z]\n");
        return;
    }

    if (cls.state != ca_active) {
        Com_Printf("No map loaded.\n");
        return;
    }

    name = Cmd_Argv(1);
    if (!*name) {
        CL_SetSky();
        return;
    }

    if (argc > 2)
        rotate = atof(Cmd_Argv(2));
    else
        rotate = 0;

    if (argc == 6) {
        axis[0] = atof(Cmd_Argv(3));
        axis[1] = atof(Cmd_Argv(4));
        axis[2] = atof(Cmd_Argv(5));
    } else
        VectorSet(axis, 0, 0, 1);

    R_SetSky(name, rotate, axis);
}

/*
================
SCR_TimeRefresh_f
================
*/
static void SCR_TimeRefresh_f(void)
{
    int     i;
    unsigned    start, stop;
    float       time;

    if (cls.state != ca_active) {
        Com_Printf("No map loaded.\n");
        return;
    }

    start = Sys_Milliseconds();

    if (Cmd_Argc() == 2) {
        // run without page flipping
        R_BeginFrame();
        for (i = 0; i < 128; i++) {
            cl.refdef.viewangles[1] = i / 128.0f * 360.0f;
            R_RenderFrame(&cl.refdef);
        }
        R_EndFrame();
    } else {
        for (i = 0; i < 128; i++) {
            cl.refdef.viewangles[1] = i / 128.0f * 360.0f;

            R_BeginFrame();
            R_RenderFrame(&cl.refdef);
            R_EndFrame();
        }
    }

    stop = Sys_Milliseconds();
    time = (stop - start) * 0.001f;
    Com_Printf("%f seconds (%f fps)\n", time, 128.0f / time);
}


//============================================================================

static void scr_crosshair_changed(cvar_t *self)
{
    char buffer[16];
    int w, h;
    float scale;

    if (scr_crosshair->integer > 0) {
        Q_snprintf(buffer, sizeof(buffer), "ch%i", scr_crosshair->integer);
        scr.crosshair_pic = R_RegisterPic(buffer);
        R_GetPicSize(&w, &h, scr.crosshair_pic);

        // prescale
        scale = Cvar_ClampValue(ch_scale, 0.1f, 9.0f);
        scr.crosshair_width = w * scale;
        scr.crosshair_height = h * scale;
        if (scr.crosshair_width < 1)
            scr.crosshair_width = 1;
        if (scr.crosshair_height < 1)
            scr.crosshair_height = 1;

        if (ch_health->integer) {
            SCR_SetCrosshairColor();
        } else {
            scr.crosshair_color.u8[0] = Cvar_ClampValue(ch_red, 0, 1) * 255;
            scr.crosshair_color.u8[1] = Cvar_ClampValue(ch_green, 0, 1) * 255;
            scr.crosshair_color.u8[2] = Cvar_ClampValue(ch_blue, 0, 1) * 255;
        }
        scr.crosshair_color.u8[3] = Cvar_ClampValue(ch_alpha, 0, 1) * 255;
    } else {
        scr.crosshair_pic = 0;
    }
}

void SCR_SetCrosshairColor(void)
{
    int health;

    if (!ch_health->integer) {
        return;
    }

    health = cl.frame.ps.stats[STAT_HEALTH];
    if (health <= 0) {
        VectorSet(scr.crosshair_color.u8, 0, 0, 0);
        return;
    }

    // red
    scr.crosshair_color.u8[0] = 255;

    // green
    if (health >= 66) {
        scr.crosshair_color.u8[1] = 255;
    } else if (health < 33) {
        scr.crosshair_color.u8[1] = 0;
    } else {
        scr.crosshair_color.u8[1] = (255 * (health - 33)) / 33;
    }

    // blue
    if (health >= 99) {
        scr.crosshair_color.u8[2] = 255;
    } else if (health < 66) {
        scr.crosshair_color.u8[2] = 0;
    } else {
        scr.crosshair_color.u8[2] = (255 * (health - 66)) / 33;
    }
}

void SCR_ModeChanged(void)
{
    IN_Activate();
    Con_CheckResize();
    UI_ModeChanged();
    // video sync flag may have changed
    CL_UpdateFrameTimes();
    cls.disable_screen = 0;
    if (scr.initialized)
        scr.hud_scale = R_ClampScale(scr_scale);

    scr.hud_alpha = 1.f;
}

/*
==================
SCR_RegisterMedia
==================
*/
void SCR_RegisterMedia(void)
{
    int     i, j;

    for (i = 0; i < 2; i++)
        for (j = 0; j < STAT_PICS; j++)
            scr.sb_pics[i][j] = R_RegisterPic(sb_nums[i][j]);

    scr.inven_pic = R_RegisterPic("inventory");
    scr.field_pic = R_RegisterPic("field_3");

    scr.backtile_pic = R_RegisterImage("backtile", IT_PIC, IF_PERMANENT | IF_REPEAT, NULL);

    scr.pause_pic = R_RegisterPic("pause");
    R_GetPicSize(&scr.pause_width, &scr.pause_height, scr.pause_pic);

    scr.loading_pic = R_RegisterPic("loading");
    R_GetPicSize(&scr.loading_width, &scr.loading_height, scr.loading_pic);

    scr.net_pic = R_RegisterPic("net");
    scr.font_pic = R_RegisterFont(scr_font->string);

    scr_crosshair_changed(scr_crosshair);
}

static void scr_font_changed(cvar_t *self)
{
    scr.font_pic = R_RegisterFont(self->string);
}

static void scr_scale_changed(cvar_t *self)
{
    scr.hud_scale = R_ClampScale(self);
}

static void SCR_LoadHud(void);
static void SCR_DestroyHud(void);

static void scr_hud_changed(cvar_t *self)
{
    SCR_DestroyHud();
    SCR_LoadHud();
}

static void SCR_HudReload_f(void)
{
    scr_hud_changed(NULL);
}

static const cmdreg_t scr_cmds[] = {
    { "timerefresh", SCR_TimeRefresh_f },
    { "sizeup", SCR_SizeUp_f },
    { "sizedown", SCR_SizeDown_f },
    { "sky", SCR_Sky_f },
    { "draw", SCR_Draw_f, SCR_Draw_c },
    { "undraw", SCR_UnDraw_f, SCR_UnDraw_c },
    { "clearchathud", SCR_ClearChatHUD_f },
    { "hud_reload", SCR_HudReload_f },
    { NULL }
};

/*
==================
SCR_Init
==================
*/
void SCR_Init(void)
{
    scr_viewsize = Cvar_Get("viewsize", "100", CVAR_ARCHIVE);
    scr_showpause = Cvar_Get("scr_showpause", "1", 0);
    scr_centertime = Cvar_Get("scr_centertime", "2.5", 0);
    scr_demobar = Cvar_Get("scr_demobar", "1", 0);
    scr_font = Cvar_Get("scr_font", "conchars", 0);
    scr_font->changed = scr_font_changed;
    scr_hud = Cvar_Get("scr_hud", "default", 0);
    scr_hud->changed = scr_hud_changed;
    scr_scale = Cvar_Get("scr_scale", "0", 0);
    scr_scale->changed = scr_scale_changed;
    scr_crosshair = Cvar_Get("crosshair", "0", CVAR_ARCHIVE);
    scr_crosshair->changed = scr_crosshair_changed;

    scr_chathud = Cvar_Get("scr_chathud", "0", 0);
    scr_chathud_lines = Cvar_Get("scr_chathud_lines", "4", 0);
    scr_chathud_time = Cvar_Get("scr_chathud_time", "0", 0);
    scr_chathud_time->changed = cl_timeout_changed;
    scr_chathud_time->changed(scr_chathud_time);
    scr_chathud_x = Cvar_Get("scr_chathud_x", "8", 0);
    scr_chathud_y = Cvar_Get("scr_chathud_y", "-64", 0);

    ch_health = Cvar_Get("ch_health", "0", 0);
    ch_health->changed = scr_crosshair_changed;
    ch_red = Cvar_Get("ch_red", "1", 0);
    ch_red->changed = scr_crosshair_changed;
    ch_green = Cvar_Get("ch_green", "1", 0);
    ch_green->changed = scr_crosshair_changed;
    ch_blue = Cvar_Get("ch_blue", "1", 0);
    ch_blue->changed = scr_crosshair_changed;
    ch_alpha = Cvar_Get("ch_alpha", "1", 0);
    ch_alpha->changed = scr_crosshair_changed;

    ch_scale = Cvar_Get("ch_scale", "1", 0);
    ch_scale->changed = scr_crosshair_changed;
    ch_x = Cvar_Get("ch_x", "0", 0);
    ch_y = Cvar_Get("ch_y", "0", 0);

    scr_draw2d = Cvar_Get("scr_draw2d", "2", 0);
    scr_showturtle = Cvar_Get("scr_showturtle", "1", 0);
    scr_showitemname = Cvar_Get("scr_showitemname", "1", CVAR_ARCHIVE);
    scr_lag_x = Cvar_Get("scr_lag_x", "-1", 0);
    scr_lag_y = Cvar_Get("scr_lag_y", "-1", 0);
    scr_lag_draw = Cvar_Get("scr_lag_draw", "0", 0);
    scr_lag_min = Cvar_Get("scr_lag_min", "0", 0);
    scr_lag_max = Cvar_Get("scr_lag_max", "200", 0);
	scr_alpha = Cvar_Get("scr_alpha", "1", 0);
	scr_fps = Cvar_Get("scr_fps", "0", CVAR_ARCHIVE);
#ifdef USE_DEBUG
    scr_showstats = Cvar_Get("scr_showstats", "0", 0);
    scr_showpmove = Cvar_Get("scr_showpmove", "0", 0);
#endif

    Cmd_Register(scr_cmds);

    scr_scale_changed(scr_scale);

    SCR_LoadHud();

    scr.initialized = true;
}

void SCR_Shutdown(void)
{
    Cmd_Deregister(scr_cmds);
    SCR_DestroyHud();
    scr.initialized = false;
}

/*
================
SCR_BeginLoadingPlaque
================
*/
void SCR_BeginLoadingPlaque(void)
{
    if (!cls.state) {
        return;
    }

    if (cls.disable_screen) {
        return;
    }

#if USE_DEBUG
    if (developer->integer) {
        return;
    }
#endif

    // if at console or menu, don't bring up the plaque
    if (cls.key_dest & (KEY_CONSOLE | KEY_MENU)) {
        return;
    }

    scr.draw_loading = true;
    SCR_UpdateScreen();

    cls.disable_screen = Sys_Milliseconds();
}

/*
================
SCR_EndLoadingPlaque
================
*/
void SCR_EndLoadingPlaque(void)
{
    if (!cls.state) {
        return;
    }
    cls.disable_screen = 0;
    Con_ClearNotify_f();
}

// Clear any parts of the tiled background that were drawn on last frame
static void SCR_TileClear(void)
{
}

/*
===============================================================================

STAT PROGRAMS

===============================================================================
*/

#define ICON_WIDTH  24
#define ICON_HEIGHT 24
#define DIGIT_WIDTH 16
#define ICON_SPACE  8

#define HUD_DrawString(x, y, string) \
    R_DrawString(x, y, 0, MAX_STRING_CHARS, string, scr.font_pic)

#define HUD_DrawAltString(x, y, string) \
    R_DrawString(x, y, UI_XORCOLOR, MAX_STRING_CHARS, string, scr.font_pic)

#define HUD_DrawCenterString(x, y, string) \
    SCR_DrawStringMulti(x, y, UI_CENTER, MAX_STRING_CHARS, string, scr.font_pic)

#define HUD_DrawAltCenterString(x, y, string) \
    SCR_DrawStringMulti(x, y, UI_CENTER | UI_XORCOLOR, MAX_STRING_CHARS, string, scr.font_pic)

static void HUD_DrawNumber(int x, int y, int color, int width, int value)
{
    char    num[16], *ptr;
    int     l;
    int     frame;

    if (width < 1)
        return;

    // draw number string
    if (width > 5)
        width = 5;

    color &= 1;

    l = Q_snprintf(num, sizeof(num), "%i", value);
    if (l > width)
        l = width;
    x += 2 + DIGIT_WIDTH * (width - l);

    ptr = num;
    while (*ptr && l) {
        if (*ptr == '-')
            frame = STAT_MINUS;
        else
            frame = *ptr - '0';

        R_DrawPic(x, y, scr.sb_pics[color][frame]);
        x += DIGIT_WIDTH;
        ptr++;
        l--;
    }
}

#define DISPLAY_ITEMS   17

static void SCR_DrawInventory(void)
{
    int     i;
    int     num, selected_num, item;
    int     index[MAX_ITEMS];
    char    string[MAX_STRING_CHARS];
    int     x, y;
    char    *bind;
    int     selected;
    int     top;

    if (!(cl.frame.ps.stats[STAT_LAYOUTS] & 2))
        return;

    selected = cl.frame.ps.stats[STAT_SELECTED_ITEM];

    num = 0;
    selected_num = 0;
    for (i = 0; i < cl.num_items; i++) {
        if (i == selected) {
            selected_num = num;
        }
        if (cl.inventory[i]) {
            index[num++] = i;
        }
    }

    // determine scroll point
    top = selected_num - DISPLAY_ITEMS / 2;
    if (top > num - DISPLAY_ITEMS) {
        top = num - DISPLAY_ITEMS;
    }
    if (top < 0) {
        top = 0;
    }

    x = (scr.hud_width - 256) / 2;
    y = (scr.hud_height - 240) / 2;

    R_DrawPic(x, y + 8, scr.inven_pic);
    y += 24;
    x += 24;

    HUD_DrawString(x, y, "hotkey ### item");
    y += CHAR_HEIGHT;

    HUD_DrawString(x, y, "------ --- ----");
    y += CHAR_HEIGHT;

    for (i = top; i < num && i < top + DISPLAY_ITEMS; i++) {
        item = index[i];
        // search for a binding
        Q_concat(string, sizeof(string), "use ", cl.configstrings[CS_ITEMS + item]);
        bind = Key_GetBinding(string);

        Q_snprintf(string, sizeof(string), "%6s %3i %s",
                   bind, cl.inventory[item], cl.configstrings[CS_ITEMS + item]);

        if (item != selected) {
            HUD_DrawAltString(x, y, string);
        } else {    // draw a blinky cursor by the selected item
            HUD_DrawString(x, y, string);
            if ((cls.realtime >> 8) & 1) {
                R_DrawChar(x - CHAR_WIDTH, y, 0, 15, scr.font_pic);
            }
        }

        y += CHAR_HEIGHT;
    }
}

static void SCR_DrawSelectedItemName(int x, int y, int item)
{
    static int display_item = -1;
    static int display_start_time = 0;

    float duration = 0.f;
    if (display_item != item)
    {
        display_start_time = Sys_Milliseconds();
        display_item = item;
    }
    else
    {
        duration = (float)(Sys_Milliseconds() - display_start_time) * 0.001f;
    }

    float alpha;
    if (scr_showitemname->integer < 2)
        alpha = max(0.f, min(1.f, 5.f - 4.f * duration)); // show and hide
    else
        alpha = 1; // always show

    if (alpha > 0.f)
    {
        R_SetAlpha(alpha * scr_alpha->value);

        int index = CS_ITEMS + item;
        HUD_DrawString(x, y, cl.configstrings[index]);

        R_SetAlpha(scr_alpha->value);
    }
}

static void SCR_ExecuteLayoutBytecode(const hud_script_t *script)
{
    sizebuf_t buf;
    SZ_InitRead(&buf, script->bytecode, script->bytecode_pos);

    int x = 0, y = 0;

    while (buf.readcount < buf.cursize)
    {
        hud_opcode_t opcode = SZ_ReadByte(&buf);

        switch (opcode)
        {
            case HUD_OP_NOP:
                break;
            case HUD_OP_XL:
                x = SZ_ReadLong(&buf);
                break;
            case HUD_OP_XR:
                x = scr.hud_width + SZ_ReadLong(&buf);
                break;
            case HUD_OP_YT:
                y = SZ_ReadLong(&buf);
                break;
            case HUD_OP_YB:
                y = scr.hud_height + SZ_ReadLong(&buf);
                break;
            case HUD_OP_XV:
                x = scr.hud_width / 2 - 160 + SZ_ReadLong(&buf);
                break;
            case HUD_OP_YV:
                y = scr.hud_height / 2 - 120 + SZ_ReadLong(&buf);
                break;
            case HUD_OP_PICN:
                R_DrawPic(x, y, R_RegisterPic2(script->string_data + SZ_ReadLong(&buf)));
                break;
            case HUD_OP_HNUM: {
                // health number
                int width = 3;
                int value = cl.frame.ps.stats[STAT_HEALTH];
                int color;
                if (value > 25)
                    color = 0;  // green
                else if (value > 0)
                    color = ((cl.frame.number / BASE_FRAMEDIV) >> 2) & 1;     // flash
                else
                    color = 1;

                if (cl.frame.ps.stats[STAT_FLASHES] & 1)
                    R_DrawPic(x, y, scr.field_pic);

                HUD_DrawNumber(x, y, color, width, value);
                break;
            }
            case HUD_OP_ANUM: {
                // ammo number
                int width = 3;
                int value = cl.frame.ps.stats[STAT_AMMO];
                int color;
                if (value > 5)
                    color = 0;  // green
                else if (value >= 0)
                    color = ((cl.frame.number / BASE_FRAMEDIV) >> 2) & 1;     // flash
                else
                    break;   // negative number = don't show

                if (cl.frame.ps.stats[STAT_FLASHES] & 4)
                    R_DrawPic(x, y, scr.field_pic);

                HUD_DrawNumber(x, y, color, width, value);
                break;
            }
            case HUD_OP_RNUM: {
                // armor number
                int width = 3;
                int value = cl.frame.ps.stats[STAT_ARMOR];
                if (value < 1)
                    break;

                int color = 0;  // green

                if (cl.frame.ps.stats[STAT_FLASHES] & 2)
                    R_DrawPic(x, y, scr.field_pic);

                HUD_DrawNumber(x, y, color, width, value);
                break;
            }
            case HUD_OP_CSTRING:
                HUD_DrawCenterString(x + 320 / 2, y, script->string_data + SZ_ReadLong(&buf));
                break;
            case HUD_OP_CSTRING2:
                HUD_DrawAltCenterString(x + 320 / 2, y, script->string_data + SZ_ReadLong(&buf));
                break;
            case HUD_OP_STRING:
                HUD_DrawString(x, y, script->string_data + SZ_ReadLong(&buf));
                break;
            case HUD_OP_STRING2:
                HUD_DrawAltString(x, y, script->string_data + SZ_ReadLong(&buf));
                break;
            case HUD_OP_COLOR: {
                color_t c;
                c.u32 = SZ_ReadLong(&buf);
                c.u8[3] *= scr_alpha->value;
                R_SetColor(c.u32);
                break;
            }
            case HUD_OP_PIC: {
                // draw a pic from a stat number
                int value = SZ_ReadByte(&buf);
                if (value < 0 || value >= MAX_STATS) {
                    Com_Errorf(ERR_DROP, "%s: invalid stat index", __func__);
                }
                int index = cl.frame.ps.stats[value];
                if (index < 0 || index >= MAX_IMAGES) {
                    Com_Errorf(ERR_DROP, "%s: invalid pic index", __func__);
                }
                const char *token = cl.configstrings[CS_IMAGES + index];
                if (token[0] && cl.image_precache[index]) {
                    R_DrawPic(x, y, cl.image_precache[index]);
                }

                if (value == STAT_SELECTED_ICON && scr_showitemname->integer) {
                    SCR_DrawSelectedItemName(x + 32, y + 8, cl.frame.ps.stats[STAT_SELECTED_ITEM]);
                }
                break;
            }
            case HUD_OP_NUM: {
                // draw a number
                int width = SZ_ReadByte(&buf);
                int value = SZ_ReadByte(&buf);
                if (value < 0 || value >= MAX_STATS) {
                    Com_Errorf(ERR_DROP, "%s: invalid stat index", __func__);
                }
                value = cl.frame.ps.stats[value];
                HUD_DrawNumber(x, y, 0, width, value);
                break;
            }
            case HUD_OP_STAT_STRING: {
                int index = SZ_ReadByte(&buf);
                if (index < 0 || index >= MAX_STATS) {
                    Com_Errorf(ERR_DROP, "%s: invalid stat index", __func__);
                }
                index = cl.frame.ps.stats[index];
                if (index < 0 || index >= MAX_CONFIGSTRINGS) {
                    Com_Errorf(ERR_DROP, "%s: invalid string index", __func__);
                }
                HUD_DrawString(x, y, cl.configstrings[index]);
                break;
            }
            case HUD_OP_STAT_STRING2: {
                int index = SZ_ReadByte(&buf);
                if (index < 0 || index >= MAX_STATS) {
                    Com_Errorf(ERR_DROP, "%s: invalid stat index", __func__);
                }
                index = cl.frame.ps.stats[index];
                if (index < 0 || index >= MAX_CONFIGSTRINGS) {
                    Com_Errorf(ERR_DROP, "%s: invalid string index", __func__);
                }
                HUD_DrawAltString(x, y, cl.configstrings[index]);
                break;
            }
            case HUD_OP_IF: {
                int value = SZ_ReadByte(&buf);
                if (value < 0 || value >= MAX_STATS) {
                    Com_Errorf(ERR_DROP, "%s: invalid stat index", __func__);
                }
                int skip = SZ_ReadLong(&buf);
                value = cl.frame.ps.stats[value];
                if (!value) {   // skip to endif
                    buf.readcount = skip;
                }
                break;
            }
        }
    }
}

static void SCR_ExecuteLayoutString(const char *s)
{
    char    buffer[MAX_QPATH];
    int     x, y;
    int     value;
    char    *token;
    int     width;
    int     index;
    clientinfo_t    *ci;

    if (!s[0])
        return;

    x = 0;
    y = 0;

    while (s) {
        token = COM_Parse(&s);
        if (token[2] == 0) {
            if (token[0] == 'x') {
                if (token[1] == 'l') {
                    token = COM_Parse(&s);
                    x = atoi(token);
                    continue;
                }

                if (token[1] == 'r') {
                    token = COM_Parse(&s);
                    x = scr.hud_width + atoi(token);
                    continue;
                }

                if (token[1] == 'v') {
                    token = COM_Parse(&s);
                    x = scr.hud_width / 2 - 160 + atoi(token);
                    continue;
                }
            }

            if (token[0] == 'y') {
                if (token[1] == 't') {
                    token = COM_Parse(&s);
                    y = atoi(token);
                    continue;
                }

                if (token[1] == 'b') {
                    token = COM_Parse(&s);
                    y = scr.hud_height + atoi(token);
                    continue;
                }

                if (token[1] == 'v') {
                    token = COM_Parse(&s);
                    y = scr.hud_height / 2 - 120 + atoi(token);
                    continue;
                }
            }
        }

        if (!strcmp(token, "pic")) {
            // draw a pic from a stat number
            token = COM_Parse(&s);
            value = atoi(token);
            if (value < 0 || value >= MAX_STATS) {
                Com_Errorf(ERR_DROP, "%s: invalid stat index", __func__);
            }
            index = cl.frame.ps.stats[value];
            if (index < 0 || index >= MAX_IMAGES) {
                Com_Errorf(ERR_DROP, "%s: invalid pic index", __func__);
            }
            token = cl.configstrings[CS_IMAGES + index];
            if (token[0] && cl.image_precache[index]) {
                R_DrawPic(x, y, cl.image_precache[index]);
            }

            if (value == STAT_SELECTED_ICON && scr_showitemname->integer)
            {
                SCR_DrawSelectedItemName(x + 32, y + 8, cl.frame.ps.stats[STAT_SELECTED_ITEM]);
            }
            continue;
        }

        if (!strcmp(token, "client")) {
            // draw a deathmatch client block
            int     score, ping, time;

            token = COM_Parse(&s);
            x = scr.hud_width / 2 - 160 + atoi(token);
            token = COM_Parse(&s);
            y = scr.hud_height / 2 - 120 + atoi(token);

            token = COM_Parse(&s);
            value = atoi(token);
            if (value < 0 || value >= MAX_CLIENTS) {
                Com_Errorf(ERR_DROP, "%s: invalid client index", __func__);
            }
            ci = &cl.clientinfo[value];

            token = COM_Parse(&s);
            score = atoi(token);

            token = COM_Parse(&s);
            ping = atoi(token);

            token = COM_Parse(&s);
            time = atoi(token);

            HUD_DrawAltString(x + 32, y, ci->name);
            HUD_DrawString(x + 32, y + CHAR_HEIGHT, "Score: ");
            Q_snprintf(buffer, sizeof(buffer), "%i", score);
            HUD_DrawAltString(x + 32 + 7 * CHAR_WIDTH, y + CHAR_HEIGHT, buffer);
            Q_snprintf(buffer, sizeof(buffer), "Ping:  %i", ping);
            HUD_DrawString(x + 32, y + 2 * CHAR_HEIGHT, buffer);
            Q_snprintf(buffer, sizeof(buffer), "Time:  %i", time);
            HUD_DrawString(x + 32, y + 3 * CHAR_HEIGHT, buffer);

            if (!ci->icon) {
                ci = &cl.baseclientinfo;
            }
            R_DrawPic(x, y, ci->icon);
            continue;
        }

        if (!strcmp(token, "ctf")) {
            // draw a ctf client block
            int     score, ping;

            token = COM_Parse(&s);
            x = scr.hud_width / 2 - 160 + atoi(token);
            token = COM_Parse(&s);
            y = scr.hud_height / 2 - 120 + atoi(token);

            token = COM_Parse(&s);
            value = atoi(token);
            if (value < 0 || value >= MAX_CLIENTS) {
                Com_Errorf(ERR_DROP, "%s: invalid client index", __func__);
            }
            ci = &cl.clientinfo[value];

            token = COM_Parse(&s);
            score = atoi(token);

            token = COM_Parse(&s);
            ping = atoi(token);
            if (ping > 999)
                ping = 999;

            Q_snprintf(buffer, sizeof(buffer), "%3d %3d %-12.12s",
                       score, ping, ci->name);
            if (value == cl.frame.clientNum) {
                HUD_DrawAltString(x, y, buffer);
            } else {
                HUD_DrawString(x, y, buffer);
            }
            continue;
        }

        if (!strcmp(token, "picn")) {
            // draw a pic from a name
            token = COM_Parse(&s);
            R_DrawPic(x, y, R_RegisterPic2(token));
            continue;
        }

        if (!strcmp(token, "num")) {
            // draw a number
            token = COM_Parse(&s);
            width = atoi(token);
            token = COM_Parse(&s);
            value = atoi(token);
            if (value < 0 || value >= MAX_STATS) {
                Com_Errorf(ERR_DROP, "%s: invalid stat index", __func__);
            }
            value = cl.frame.ps.stats[value];
            HUD_DrawNumber(x, y, 0, width, value);
            continue;
        }

        if (!strcmp(token, "hnum")) {
            // health number
            int     color;

            width = 3;
            value = cl.frame.ps.stats[STAT_HEALTH];
            if (value > 25)
                color = 0;  // green
            else if (value > 0)
                color = ((cl.frame.number / BASE_FRAMEDIV) >> 2) & 1;     // flash
            else
                color = 1;

            if (cl.frame.ps.stats[STAT_FLASHES] & 1)
                R_DrawPic(x, y, scr.field_pic);

            HUD_DrawNumber(x, y, color, width, value);
            continue;
        }

        if (!strcmp(token, "anum")) {
            // ammo number
            int     color;

            width = 3;
            value = cl.frame.ps.stats[STAT_AMMO];
            if (value > 5)
                color = 0;  // green
            else if (value >= 0)
                color = ((cl.frame.number / BASE_FRAMEDIV) >> 2) & 1;     // flash
            else
                continue;   // negative number = don't show

            if (cl.frame.ps.stats[STAT_FLASHES] & 4)
                R_DrawPic(x, y, scr.field_pic);

            HUD_DrawNumber(x, y, color, width, value);
            continue;
        }

        if (!strcmp(token, "rnum")) {
            // armor number
            int     color;

            width = 3;
            value = cl.frame.ps.stats[STAT_ARMOR];
            if (value < 1)
                continue;

            color = 0;  // green

            if (cl.frame.ps.stats[STAT_FLASHES] & 2)
                R_DrawPic(x, y, scr.field_pic);

            HUD_DrawNumber(x, y, color, width, value);
            continue;
        }

        if (!strcmp(token, "stat_string")) {
            token = COM_Parse(&s);
            index = atoi(token);
            if (index < 0 || index >= MAX_STATS) {
                Com_Errorf(ERR_DROP, "%s: invalid stat index", __func__);
            }
            index = cl.frame.ps.stats[index];
            if (index < 0 || index >= MAX_CONFIGSTRINGS) {
                Com_Errorf(ERR_DROP, "%s: invalid string index", __func__);
            }
            HUD_DrawString(x, y, cl.configstrings[index]);
            continue;
        }

        if (!strcmp(token, "cstring")) {
            token = COM_Parse(&s);
            HUD_DrawCenterString(x + 320 / 2, y, token);
            continue;
        }

        if (!strcmp(token, "cstring2")) {
            token = COM_Parse(&s);
            HUD_DrawAltCenterString(x + 320 / 2, y, token);
            continue;
        }

        if (!strcmp(token, "string")) {
            token = COM_Parse(&s);
            HUD_DrawString(x, y, token);
            continue;
        }

        if (!strcmp(token, "string2")) {
            token = COM_Parse(&s);
            HUD_DrawAltString(x, y, token);
            continue;
        }

        if (!strcmp(token, "if")) {
            token = COM_Parse(&s);
            value = atoi(token);
            if (value < 0 || value >= MAX_STATS) {
                Com_Errorf(ERR_DROP, "%s: invalid stat index", __func__);
            }
            value = cl.frame.ps.stats[value];
            if (!value) {   // skip to endif
                while (strcmp(token, "endif")) {
                    token = COM_Parse(&s);
                    if (!s) {
                        break;
                    }
                }
            }
            continue;
        }

        if (!strcmp(token, "color")) {
            color_t     color;

            token = COM_Parse(&s);
            if (SCR_ParseColor(token, &color)) {
                color.u8[3] *= scr_alpha->value;
                R_SetColor(color.u32);
            }
            continue;
        }
    }

    R_ClearColor();
    R_SetAlpha(scr_alpha->value);
}

static hud_script_t *SCR_AllocateHudScript(void)
{
    hud_script_t *script = Z_TagMallocz(sizeof(hud_script_t), TAG_HUD);
    script->bytecode = Z_TagMalloc(script->bytecode_length = 2048, TAG_HUD);
    script->string_data = Z_TagMallocz(script->string_data_length = 2048, TAG_HUD);
    script->constants = Z_TagMalloc(sizeof(hud_macro_t) * (script->allocated_constants = 64), TAG_HUD);
    return script;
}

static void SCR_ShrinkHudScript(void)
{
    scr.hud_script->bytecode = Z_Realloc(scr.hud_script->bytecode, scr.hud_script->bytecode_pos);
    scr.hud_script->bytecode_length = scr.hud_script->bytecode_pos;

    scr.hud_script->string_data = Z_Realloc(scr.hud_script->string_data, scr.hud_script->string_len);
    scr.hud_script->string_data_length = scr.hud_script->string_len;
}

static void SCR_DestroyHudScriptTemp(void)
{
    if (scr.hud_script->constants) {
        Z_Free(scr.hud_script->constants);
        scr.hud_script->constants = NULL;
    }

    while (scr.hud_script->control_head) {
        hud_control_t *n = scr.hud_script->control_head->next;
        Z_Free(scr.hud_script->control_head);
        scr.hud_script->control_head = n;
    }

    scr.hud_script->control_head = NULL;
}

static void SCR_DestroyHudScript(void)
{
    if (scr.hud_script->bytecode) {
        Z_Free(scr.hud_script->bytecode);
        scr.hud_script->bytecode = NULL;
    }

    if (scr.hud_script->string_data) {
        Z_Free(scr.hud_script->string_data);
        scr.hud_script->string_data = NULL;
    }

    SCR_DestroyHudScriptTemp();

    Z_Free(scr.hud_script);
    scr.hud_script = NULL;
}

static void SCR_PushHudBytecode(hud_script_t *script, sizebuf_t *buf)
{
    if (script->bytecode_pos + buf->cursize > script->bytecode_length) {
        script->bytecode_length += 2048;
        script->bytecode = Z_Realloc(script->bytecode, script->bytecode_length);
    }

    memcpy(script->bytecode + script->bytecode_pos, buf->data, buf->cursize);
    script->bytecode_pos += buf->cursize;
    SZ_Clear(buf);
}

static void SCR_PushHudConstant(hud_script_t *script, const char *key, const char *value)
{
    for (size_t i = 0; i < script->num_constants; i++) {
        if (Q_strcasecmp(script->constants[i].key, key) == 0) {
            Q_strlcpy(script->constants[i].value, value, sizeof(script->constants[i].value));
            return;
        }
    }

    if (script->num_constants == script->allocated_constants) {
        script->allocated_constants += 64;
        script->constants = Z_Realloc(script->constants, sizeof(hud_macro_t) * (script->allocated_constants));
    }

    Q_strlcpy(script->constants[script->num_constants].key, key, sizeof(script->constants[script->num_constants].key));
    Q_strlcpy(script->constants[script->num_constants].value, value, sizeof(script->constants[script->num_constants].value));
    script->num_constants++;
}

static uint32_t SCR_FindHudString(hud_script_t *script, const char *str)
{
    char *s = script->string_data;

    for (size_t i = 0; i < script->string_count; i++) {
        if (strcmp(s, str) == 0) {
            return s - script->string_data;
        }

        s += strlen(s) + 1;
    }

    // not found, we can write one here
    size_t pos = s - script->string_data;
    size_t reserve_len = strlen(str) + 1;

    if (pos + reserve_len > script->string_data_length) {
        script->string_data_length += 2048;
        script->string_data = Z_Realloc(script->string_data, script->string_data_length);
        s = script->string_data + pos;
        memset(s, 0, script->string_data_length - pos);
    }

    Q_strlcpy(s, str, script->string_data_length - pos);
    script->string_count++;
    script->string_len += reserve_len;

    return pos;
}

static void SCR_PrintError(const char *script_path, const char *script_data, const char *token_location, const char *token, const char *message)
{
    char loc_buffer[128];
    Q_snprintf(loc_buffer, sizeof(loc_buffer), "<unknown>");

    if (script_data) {
        if (!token_location) {
            Q_snprintf(loc_buffer, sizeof(loc_buffer), "<eof>");
        } else if (token_location >= script_data && token_location <= script_data + strlen(script_data)) {
            size_t last_newline = 0, newline_count = 0;
            const char *p = script_data;

            while (p != token_location && *p++) {
                if (*p == '\n') {
                    last_newline = p - script_data;
                    newline_count++;
                }
            }

            Q_snprintf(loc_buffer, sizeof(loc_buffer), "%i:%i", newline_count + 1, (token_location - script_data) - last_newline - strlen(token));
        }
    }

    Com_EPrintf("Script error in %s[%s]: %s\n", script_path, loc_buffer, message);
}

#define SCR_HudError(msg, token) \
    SCR_PrintError(path, script_data, buffer, token, msg); FS_FreeFile(script_data); return false

#define SCR_ExpectInt(name) \
    int name; { char *end = NULL; const char *s = SCR_ResolveConstant(script, token); name = strtol(s, &end, 10); if (end == s) { SCR_HudError("expected integer", token); } }

static const char *SCR_ResolveConstant(hud_script_t *script, const char *key)
{
    if (key[0] != '$') {
        return key;
    }

    for (size_t i = 0; i < script->num_constants; i++) {
        if (Q_strcasecmp(script->constants[i].key, key + 1) == 0) {
            return script->constants[i].value;
        }
    }

    return key;
}

static bool SCR_CompileHudFile(hud_script_t *script, const char *path)
{
    char *buffer;

    int ret = FS_LoadFile(path, &buffer);

    if (!buffer) {
        Com_WPrintf("Couldn't find HUD file \"%s\"\n", path);
        return false;
    }

    char *script_data = buffer;

    // parse all the opcodes of this file
    char *token;

    byte opcode_buffer[32];
    sizebuf_t opcodes = { 0 };
    SZ_TagInit(&opcodes, &opcode_buffer, sizeof(opcode_buffer), TAG_HUD);

    while (*(token = COM_Parse(&buffer)))
    {
        // import another file
        if (Q_strcasecmp(token, "include") == 0) {
            char include_path[MAX_OSPATH];

            token = COM_Parse(&buffer);
            Q_snprintf(include_path, sizeof(include_path), "huds/%s", SCR_ResolveConstant(script, token));

            if (!SCR_CompileHudFile(script, include_path)) {
                SCR_HudError("couldn't find HUD import", token);
                return false;
            }
        // define constant
        } else if (Q_strcasecmp(token, "define") == 0) {
            const char *key = COM_Parse(&buffer);
            
            if (!*key) {
                SCR_HudError("invalid macro name", key);
            }

            token = COM_Parse(&buffer);

            if (!*token) {
                SCR_HudError("invalid macro value", token);
            }

            SCR_PushHudConstant(script, key, token);
        } else if (Q_strcasecmp(token, "xl") == 0) {
            token = COM_Parse(&buffer);
            SCR_ExpectInt(v);

            SZ_WriteByte(&opcodes, HUD_OP_XL);
            SZ_WriteLong(&opcodes, v);
            SCR_PushHudBytecode(script, &opcodes);
        } else if (Q_strcasecmp(token, "xr") == 0) {
            token = COM_Parse(&buffer);
            SCR_ExpectInt(v);

            SZ_WriteByte(&opcodes, HUD_OP_XR);
            SZ_WriteLong(&opcodes, v);
            SCR_PushHudBytecode(script, &opcodes);
        } else if (Q_strcasecmp(token, "yt") == 0) {
            token = COM_Parse(&buffer);
            SCR_ExpectInt(v);

            SZ_WriteByte(&opcodes, HUD_OP_YT);
            SZ_WriteLong(&opcodes, v);
            SCR_PushHudBytecode(script, &opcodes);
        } else if (Q_strcasecmp(token, "yb") == 0) {
            token = COM_Parse(&buffer);
            SCR_ExpectInt(v);

            SZ_WriteByte(&opcodes, HUD_OP_YB);
            SZ_WriteLong(&opcodes, v);
            SCR_PushHudBytecode(script, &opcodes);
        } else if (Q_strcasecmp(token, "xv") == 0) {
            token = COM_Parse(&buffer);
            SCR_ExpectInt(v);

            SZ_WriteByte(&opcodes, HUD_OP_XV);
            SZ_WriteLong(&opcodes, v);
            SCR_PushHudBytecode(script, &opcodes);
        } else if (Q_strcasecmp(token, "yv") == 0) {
            token = COM_Parse(&buffer);
            SCR_ExpectInt(v);

            SZ_WriteByte(&opcodes, HUD_OP_YV);
            SZ_WriteLong(&opcodes, v);
            SCR_PushHudBytecode(script, &opcodes);
        } else if (Q_strcasecmp(token, "picn") == 0) {
            token = COM_Parse(&buffer);

            if (!*token) {
                SCR_HudError("invalid image", token);
            }

            uint32_t id = SCR_FindHudString(script, SCR_ResolveConstant(script, token));

            SZ_WriteByte(&opcodes, HUD_OP_PICN);
            SZ_WriteLong(&opcodes, id);
            SCR_PushHudBytecode(script, &opcodes);
        } else if (Q_strcasecmp(token, "hnum") == 0) {
            SZ_WriteByte(&opcodes, HUD_OP_HNUM);
            SCR_PushHudBytecode(script, &opcodes);
        } else if (Q_strcasecmp(token, "anum") == 0) {
            SZ_WriteByte(&opcodes, HUD_OP_ANUM);
            SCR_PushHudBytecode(script, &opcodes);
        } else if (Q_strcasecmp(token, "rnum") == 0) {
            SZ_WriteByte(&opcodes, HUD_OP_RNUM);
            SCR_PushHudBytecode(script, &opcodes);
        } else if (Q_strcasecmp(token, "cstring") == 0) {
            token = COM_Parse(&buffer);
            uint32_t id = SCR_FindHudString(script, SCR_ResolveConstant(script, token));

            SZ_WriteByte(&opcodes, HUD_OP_CSTRING);
            SZ_WriteLong(&opcodes, id);
            SCR_PushHudBytecode(script, &opcodes);
        } else if (Q_strcasecmp(token, "cstring2") == 0) {
            token = COM_Parse(&buffer);
            uint32_t id = SCR_FindHudString(script, SCR_ResolveConstant(script, token));

            SZ_WriteByte(&opcodes, HUD_OP_CSTRING2);
            SZ_WriteLong(&opcodes, id);
            SCR_PushHudBytecode(script, &opcodes);
        } else if (Q_strcasecmp(token, "string") == 0) {
            token = COM_Parse(&buffer);
            uint32_t id = SCR_FindHudString(script, SCR_ResolveConstant(script, token));

            SZ_WriteByte(&opcodes, HUD_OP_STRING);
            SZ_WriteLong(&opcodes, id);
            SCR_PushHudBytecode(script, &opcodes);
        } else if (Q_strcasecmp(token, "string2") == 0) {
            token = COM_Parse(&buffer);
            uint32_t id = SCR_FindHudString(script, SCR_ResolveConstant(script, token));

            SZ_WriteByte(&opcodes, HUD_OP_STRING2);
            SZ_WriteLong(&opcodes, id);
            SCR_PushHudBytecode(script, &opcodes);
        } else if (Q_strcasecmp(token, "color") == 0) {
            token = COM_Parse(&buffer);
            color_t c;
            if (!SCR_ParseColor(SCR_ResolveConstant(script, token), &c)) {
                SCR_HudError("invalid color", token);
            }

            SZ_WriteByte(&opcodes, HUD_OP_COLOR);
            SZ_WriteLong(&opcodes, c.u32);
            SCR_PushHudBytecode(script, &opcodes);
        } else if (Q_strcasecmp(token, "pic") == 0) {
            token = COM_Parse(&buffer);
            SCR_ExpectInt(v);

            SZ_WriteByte(&opcodes, HUD_OP_PIC);
            SZ_WriteByte(&opcodes, v);
            SCR_PushHudBytecode(script, &opcodes);
        } else if (Q_strcasecmp(token, "num") == 0) {
            token = COM_Parse(&buffer);
            SCR_ExpectInt(width);

            token = COM_Parse(&buffer);
            SCR_ExpectInt(v);

            SZ_WriteByte(&opcodes, HUD_OP_NUM);
            SZ_WriteByte(&opcodes, width);
            SZ_WriteByte(&opcodes, v);
            SCR_PushHudBytecode(script, &opcodes);
        } else if (Q_strcasecmp(token, "stat_string") == 0) {
            token = COM_Parse(&buffer);
            SCR_ExpectInt(v);

            SZ_WriteByte(&opcodes, HUD_OP_STAT_STRING);
            SZ_WriteByte(&opcodes, v);
            SCR_PushHudBytecode(script, &opcodes);
        } else if (Q_strcasecmp(token, "stat_string2") == 0) {
            token = COM_Parse(&buffer);
            SCR_ExpectInt(v);

            SZ_WriteByte(&opcodes, HUD_OP_STAT_STRING2);
            SZ_WriteByte(&opcodes, v);
            SCR_PushHudBytecode(script, &opcodes);
        } else if (Q_strcasecmp(token, "if") == 0) {
            token = COM_Parse(&buffer);
            SCR_ExpectInt(v);

            SZ_WriteByte(&opcodes, HUD_OP_IF);
            SZ_WriteByte(&opcodes, v);

            hud_control_t *ctrl = Z_TagMallocz(sizeof(hud_control_t), TAG_HUD);
            ctrl->p = script->bytecode_pos + opcodes.cursize;
            ctrl->next = script->control_head;
            script->control_head = ctrl;

            SZ_WriteLong(&opcodes, 12345678);
            SCR_PushHudBytecode(script, &opcodes);
        } else if (Q_strcasecmp(token, "endif") == 0) {
            if (!script->control_head) {
                SCR_HudError("endif without matching if", token);
            }

            *((int32_t *) (script->bytecode + script->control_head->p)) = script->bytecode_pos;

            hud_control_t *n = script->control_head->next;
            Z_Free(script->control_head);
            script->control_head = n;
        } else {
            SCR_HudError("invalid opcode", token);
        }
    }

    FS_FreeFile(script_data);
    return true;
}

static void SCR_LoadHud(void)
{
    char path[MAX_OSPATH];

    Q_snprintf(path, sizeof(path), "huds/%s.hud", scr_hud->string);

    scr.hud_script = SCR_AllocateHudScript();

    if (!SCR_CompileHudFile(scr.hud_script, path)) {
        SCR_DestroyHudScript();
        return;
    }

    if (scr.hud_script->control_head) {
        SCR_PrintError(path, NULL, NULL, NULL, "unexpected end of script; no endif found for if");
        SCR_DestroyHudScript();
        return;
    }

    SCR_DestroyHudScriptTemp();
    SCR_ShrinkHudScript();
}

static void SCR_DestroyHud(void)
{
    SCR_DestroyHudScript();
}

//=============================================================================

static void SCR_DrawPause(void)
{
    int x, y;

    if (!sv_paused->integer)
        return;
    if (!cl_paused->integer)
        return;
    if (scr_showpause->integer != 1)
        return;

    x = (scr.hud_width - scr.pause_width) / 2;
    y = (scr.hud_height - scr.pause_height) / 2;

    R_DrawPic(x, y, scr.pause_pic);
}

static void SCR_DrawLoading(void)
{
    int x, y;

    if (!scr.draw_loading)
        return;

    scr.draw_loading = false;

    R_SetScale(scr.hud_scale);

    x = (r_config.width * scr.hud_scale - scr.loading_width) / 2;
    y = (r_config.height * scr.hud_scale - scr.loading_height) / 2;

    R_DrawPic(x, y, scr.loading_pic);

    R_SetScale(1.0f);
}

static void SCR_DrawCrosshair(void)
{
    int x, y;

    if (!scr_crosshair->integer)
        return;

    x = (scr.hud_width - scr.crosshair_width) / 2;
    y = (scr.hud_height - scr.crosshair_height) / 2;

    R_SetColor(scr.crosshair_color.u32);

    R_DrawStretchPic(x + ch_x->integer,
                     y + ch_y->integer,
                     scr.crosshair_width,
                     scr.crosshair_height,
                     scr.crosshair_pic);
}

// The status bar is a small layout program that is based on the stats array
static void SCR_DrawStats(void)
{
    if (scr_draw2d->integer <= 1)
        return;

    SCR_ExecuteLayoutBytecode(scr.hud_script);
}

static void SCR_DrawLayout(void)
{
    if (scr_draw2d->integer == 3 && !Key_IsDown(K_F1))
        return;     // turn off for GTV

    if (cls.demo.playback && Key_IsDown(K_F1))
        goto draw;

    if (!(cl.frame.ps.stats[STAT_LAYOUTS] & 1))
        return;

draw:
    SCR_ExecuteLayoutString(cl.layout);
}

static void SCR_Draw2D(void)
{
	if (scr_draw2d->integer <= 0)
		return;     // turn off for screenshots

	if (cls.key_dest & KEY_MENU)
		return;

	R_SetAlphaScale(scr.hud_alpha);

    R_SetScale(scr.hud_scale);

    scr.hud_height *= scr.hud_scale;
    scr.hud_width *= scr.hud_scale;

    // crosshair has its own color and alpha
    SCR_DrawCrosshair();

    // the rest of 2D elements share common alpha
    R_ClearColor();
    R_SetAlpha(Cvar_ClampValue(scr_alpha, 0, 1));

    SCR_DrawStats();

    SCR_DrawLayout();

    SCR_DrawInventory();

    SCR_DrawCenterString();

    SCR_DrawNet();

    SCR_DrawObjects();

	SCR_DrawFPS();

    SCR_DrawChatHUD();

    SCR_DrawTurtle();

    SCR_DrawPause();

    // debug stats have no alpha
    R_ClearColor();

#if USE_DEBUG
    SCR_DrawDebugStats();
    SCR_DrawDebugPmove();
#endif

    R_SetScale(1.0f);
	R_SetAlphaScale(1.0f);
}

static void SCR_DrawActive(void)
{
    // if full screen menu is up, do nothing at all
    if (!UI_IsTransparent())
        return;

    // draw black background if not active
    if (cls.state < ca_active) {
        R_DrawFill8(0, 0, r_config.width, r_config.height, 0);
        return;
    }

    if (cls.state == ca_cinematic) {
        if (cl.image_precache[0]) 
        {
            // scale the image to touch the screen from inside, keeping the aspect ratio

            image_t* image = IMG_ForHandle(cl.image_precache[0]);

            float zoomx = (float)r_config.width / (float)image->width;
            float zoomy = (float)r_config.height / (float)image->height;
            float zoom = min(zoomx, zoomy);

            int w = (int)(image->width * zoom);
            int h = (int)(image->height * zoom);
            int x = (r_config.width - w) / 2;
            int y = (r_config.height - h) / 2;

            R_DrawFill8(0, 0, r_config.width, r_config.height, 0);
            R_DrawStretchPic(x, y, w, h, cl.image_precache[0]);
        }
        return;
    }

    // start with full screen HUD
    scr.hud_height = r_config.height;
    scr.hud_width = r_config.width;

    SCR_DrawDemo();

    SCR_CalcVrect();

    // clear any dirty part of the background
    SCR_TileClear();

    // draw 3D game view
    V_RenderView();

    // draw all 2D elements
    SCR_Draw2D();
}

//=======================================================

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen(void)
{
    static int recursive;

    if (!scr.initialized) {
        return;             // not initialized yet
    }

    // if the screen is disabled (loading plaque is up), do nothing at all
    if (cls.disable_screen) {
        unsigned delta = Sys_Milliseconds() - cls.disable_screen;

        if (delta < 120 * 1000) {
            return;
        }

        cls.disable_screen = 0;
        Com_Printf("Loading plaque timed out.\n");
    }

    if (recursive > 1) {
        Com_Errorf(ERR_FATAL, "%s: recursively called", __func__);
    }

    recursive++;

    R_BeginFrame();

    // do 3D refresh drawing
    SCR_DrawActive();

    // draw main menu
    UI_Draw(cls.realtime);

    // draw console
    Con_DrawConsole();

    // draw loading plaque
    SCR_DrawLoading();

    R_EndFrame();

    recursive--;
}

qhandle_t SCR_GetFont(void)
{
	return scr.font_pic;
}

void SCR_SetHudAlpha(float alpha)
{
	scr.hud_alpha = alpha;
}
