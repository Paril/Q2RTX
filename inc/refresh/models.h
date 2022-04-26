/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2008 Andrey Nazarov
Copyright (C) 2019, NVIDIA CORPORATION. All rights reserved.

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

#ifndef MODELS_H
#define MODELS_H

//
// models.h -- common models manager
//

#include "system/hunk.h"
#include "common/error.h"

#define MOD_Malloc(size)    Hunk_Alloc(&model->hunk, size)

#define CHECK(x)    if (!(x)) { ret = Q_ERR(ENOMEM); goto fail; }

#define MAX_ALIAS_SKINS     32
#define MAX_ALIAS_VERTS     4096

typedef struct mspriteframe_s {
    int             width, height;
    int             origin_x, origin_y;
    struct image_s  *image;
} mspriteframe_t;

typedef enum
{
	MCLASS_REGULAR,
	MCLASS_EXPLOSION,
	MCLASS_FLASH,
	MCLASS_SMOKE,
    MCLASS_STATIC_LIGHT,
    MCLASS_FLARE
} model_class_t;

// inter-quake model
#include "shared/iqm.h"

typedef struct light_poly_s {
	float positions[9]; // 3x vec3_t
	vec3_t off_center;
	vec3_t color;
	struct pbr_material_s* material;
	int cluster;
	int style;
	float emissive_factor;
} light_poly_t;

typedef struct model_s {
    enum {
        MOD_FREE,
        MOD_ALIAS,
        MOD_SPRITE,
        MOD_EMPTY
    } type;
    char name[MAX_QPATH];
    int registration_sequence;
    memhunk_t hunk;

    // alias models
    int numframes;
    struct maliasframe_s *frames;
#if USE_REF == REF_GL || USE_REF == REF_VKPT
    int nummeshes;
    struct maliasmesh_s *meshes;
	model_class_t model_class;
#else
    int numskins;
    struct image_s *skins[MAX_ALIAS_SKINS];
    int numtris;
    struct maliastri_s *tris;
    int numsts;
    struct maliasst_s *sts;
    int numverts;
    int skinwidth;
    int skinheight;
#endif

    // sprite models
    struct mspriteframe_s *spriteframes;
	bool sprite_vertical;

	iqm_model_t* iqmData;

	int num_light_polys;
	light_poly_t* light_polys;
} model_t;

extern model_t      r_models[];
extern int          r_numModels;

extern int registration_sequence;

typedef struct entity_s entity_t;

// these are implemented in r_models.c
void MOD_FreeUnused(void);
void MOD_FreeAll(void);
void MOD_Init(void);
void MOD_Shutdown(void);

model_t *MOD_ForHandle(qhandle_t h);
qhandle_t R_RegisterModel(const char *name);

struct dmd2header_s;
int MOD_ValidateMD2(struct dmd2header_s *header, size_t length);

bool R_ComputeIQMTransforms(const iqm_model_t* model, const entity_t* entity, float* pose_matrices, struct refdef_s *fd);

// these are implemented in [gl,sw]_models.c
typedef int (*mod_load_t)(model_t *, const void *, size_t, const char*);
extern int (*MOD_LoadMD2)(model_t *model, const void *rawdata, size_t length, const char* mod_name);
#if USE_MD3
extern int (*MOD_LoadMD3)(model_t *model, const void *rawdata, size_t length, const char* mod_name);
#endif
extern int(*MOD_LoadIQM)(model_t* model, const void* rawdata, size_t length, const char* mod_name);
extern void (*MOD_Reference)(model_t *model);

#endif // MODELS_H
