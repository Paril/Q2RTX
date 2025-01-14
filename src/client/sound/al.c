/*
Copyright (C) 2010 Andrey Nazarov

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

#include "sound.h"

#include "qal/fixed.h"

// translates from AL coordinate system to quake
#define AL_UnpackVector(v)  -v[1],v[2],-v[0]
#define AL_CopyVector(a,b)  ((b)[0]=-(a)[1],(b)[1]=(a)[2],(b)[2]=-(a)[0])

// OpenAL implementation should support at least this number of sources
#define MIN_CHANNELS 16

int active_buffers = 0;
bool streamPlaying = false;
static ALuint s_srcnums[MAX_CHANNELS];
static ALuint streamSource = 0;
static int s_framecount;
static ALuint s_effect, s_auxEffectSlot;
static cvar_t *s_testReverb;
static int s_activePreset = 0;

static const EFXEAXREVERBPROPERTIES reverb_presets[] = {
    EFX_REVERB_PRESET_GENERIC,
    EFX_REVERB_PRESET_PADDEDCELL,
    EFX_REVERB_PRESET_ROOM,
    EFX_REVERB_PRESET_BATHROOM,
    EFX_REVERB_PRESET_LIVINGROOM,
    EFX_REVERB_PRESET_STONEROOM,
    EFX_REVERB_PRESET_AUDITORIUM,
    EFX_REVERB_PRESET_CONCERTHALL,
    EFX_REVERB_PRESET_CAVE,
    EFX_REVERB_PRESET_ARENA,
    EFX_REVERB_PRESET_HANGAR,
    EFX_REVERB_PRESET_CARPETEDHALLWAY,
    EFX_REVERB_PRESET_HALLWAY,
    EFX_REVERB_PRESET_STONECORRIDOR,
    EFX_REVERB_PRESET_ALLEY,
    EFX_REVERB_PRESET_FOREST,
    EFX_REVERB_PRESET_CITY,
    EFX_REVERB_PRESET_MOUNTAINS,
    EFX_REVERB_PRESET_QUARRY,
    EFX_REVERB_PRESET_PLAIN,
    EFX_REVERB_PRESET_PARKINGLOT,
    EFX_REVERB_PRESET_SEWERPIPE,
    EFX_REVERB_PRESET_UNDERWATER,
    EFX_REVERB_PRESET_DRUGGED,
    EFX_REVERB_PRESET_DIZZY,
    EFX_REVERB_PRESET_PSYCHOTIC,

    EFX_REVERB_PRESET_CASTLE_SMALLROOM,
    EFX_REVERB_PRESET_CASTLE_SHORTPASSAGE,
    EFX_REVERB_PRESET_CASTLE_MEDIUMROOM,
    EFX_REVERB_PRESET_CASTLE_LARGEROOM,
    EFX_REVERB_PRESET_CASTLE_LONGPASSAGE,
    EFX_REVERB_PRESET_CASTLE_HALL,
    EFX_REVERB_PRESET_CASTLE_CUPBOARD,
    EFX_REVERB_PRESET_CASTLE_COURTYARD,
    EFX_REVERB_PRESET_CASTLE_ALCOVE,

    EFX_REVERB_PRESET_FACTORY_SMALLROOM,
    EFX_REVERB_PRESET_FACTORY_SHORTPASSAGE,
    EFX_REVERB_PRESET_FACTORY_MEDIUMROOM,
    EFX_REVERB_PRESET_FACTORY_LARGEROOM,
    EFX_REVERB_PRESET_FACTORY_LONGPASSAGE,
    EFX_REVERB_PRESET_FACTORY_HALL,
    EFX_REVERB_PRESET_FACTORY_CUPBOARD,
    EFX_REVERB_PRESET_FACTORY_COURTYARD,
    EFX_REVERB_PRESET_FACTORY_ALCOVE,

    EFX_REVERB_PRESET_ICEPALACE_SMALLROOM,
    EFX_REVERB_PRESET_ICEPALACE_SHORTPASSAGE,
    EFX_REVERB_PRESET_ICEPALACE_MEDIUMROOM,
    EFX_REVERB_PRESET_ICEPALACE_LARGEROOM,
    EFX_REVERB_PRESET_ICEPALACE_LONGPASSAGE,
    EFX_REVERB_PRESET_ICEPALACE_HALL,
    EFX_REVERB_PRESET_ICEPALACE_CUPBOARD,
    EFX_REVERB_PRESET_ICEPALACE_COURTYARD,
    EFX_REVERB_PRESET_ICEPALACE_ALCOVE,

    EFX_REVERB_PRESET_SPACESTATION_SMALLROOM,
    EFX_REVERB_PRESET_SPACESTATION_SHORTPASSAGE,
    EFX_REVERB_PRESET_SPACESTATION_MEDIUMROOM,
    EFX_REVERB_PRESET_SPACESTATION_LARGEROOM,
    EFX_REVERB_PRESET_SPACESTATION_LONGPASSAGE,
    EFX_REVERB_PRESET_SPACESTATION_HALL,
    EFX_REVERB_PRESET_SPACESTATION_CUPBOARD,
    EFX_REVERB_PRESET_SPACESTATION_ALCOVE,

    EFX_REVERB_PRESET_WOODEN_SMALLROOM,
    EFX_REVERB_PRESET_WOODEN_SHORTPASSAGE,
    EFX_REVERB_PRESET_WOODEN_MEDIUMROOM,
    EFX_REVERB_PRESET_WOODEN_LARGEROOM,
    EFX_REVERB_PRESET_WOODEN_LONGPASSAGE,
    EFX_REVERB_PRESET_WOODEN_HALL,
    EFX_REVERB_PRESET_WOODEN_CUPBOARD,
    EFX_REVERB_PRESET_WOODEN_COURTYARD,
    EFX_REVERB_PRESET_WOODEN_ALCOVE,

    EFX_REVERB_PRESET_SPORT_EMPTYSTADIUM,
    EFX_REVERB_PRESET_SPORT_SQUASHCOURT,
    EFX_REVERB_PRESET_SPORT_SMALLSWIMMINGPOOL,
    EFX_REVERB_PRESET_SPORT_LARGESWIMMINGPOOL,
    EFX_REVERB_PRESET_SPORT_GYMNASIUM,
    EFX_REVERB_PRESET_SPORT_FULLSTADIUM,
    EFX_REVERB_PRESET_SPORT_STADIUMTANNOY,

    EFX_REVERB_PRESET_PREFAB_WORKSHOP,
    EFX_REVERB_PRESET_PREFAB_SCHOOLROOM,
    EFX_REVERB_PRESET_PREFAB_PRACTISEROOM,
    EFX_REVERB_PRESET_PREFAB_OUTHOUSE,
    EFX_REVERB_PRESET_PREFAB_CARAVAN,

    EFX_REVERB_PRESET_DOME_TOMB,
    EFX_REVERB_PRESET_PIPE_SMALL,
    EFX_REVERB_PRESET_DOME_SAINTPAULS,
    EFX_REVERB_PRESET_PIPE_LONGTHIN,
    EFX_REVERB_PRESET_PIPE_LARGE,
    EFX_REVERB_PRESET_PIPE_RESONANT,

    EFX_REVERB_PRESET_OUTDOORS_BACKYARD,
    EFX_REVERB_PRESET_OUTDOORS_ROLLINGPLAINS,
    EFX_REVERB_PRESET_OUTDOORS_DEEPCANYON,
    EFX_REVERB_PRESET_OUTDOORS_CREEK,
    EFX_REVERB_PRESET_OUTDOORS_VALLEY,

    EFX_REVERB_PRESET_MOOD_HEAVEN,
    EFX_REVERB_PRESET_MOOD_HELL,
    EFX_REVERB_PRESET_MOOD_MEMORY,

    EFX_REVERB_PRESET_DRIVING_COMMENTATOR,
    EFX_REVERB_PRESET_DRIVING_PITGARAGE,
    EFX_REVERB_PRESET_DRIVING_INCAR_RACER,
    EFX_REVERB_PRESET_DRIVING_INCAR_SPORTS,
    EFX_REVERB_PRESET_DRIVING_INCAR_LUXURY,
    EFX_REVERB_PRESET_DRIVING_FULLGRANDSTAND,
    EFX_REVERB_PRESET_DRIVING_EMPTYGRANDSTAND,
    EFX_REVERB_PRESET_DRIVING_TUNNEL,

    EFX_REVERB_PRESET_CITY_STREETS,
    EFX_REVERB_PRESET_CITY_SUBWAY,
    EFX_REVERB_PRESET_CITY_MUSEUM,
    EFX_REVERB_PRESET_CITY_LIBRARY,
    EFX_REVERB_PRESET_CITY_UNDERPASS,
    EFX_REVERB_PRESET_CITY_ABANDONED,

    EFX_REVERB_PRESET_DUSTYROOM,
    EFX_REVERB_PRESET_CHAPEL,
    EFX_REVERB_PRESET_SMALLWATERROOM
};

static const EFXEAXREVERBPROPERTIES reverb_identity = { 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.49f, 1.0f, 1.0f, 0.0f, 0.0f, { 0.f, 0.f, 0.f }, 0.f, 0.f, { 0.f, 0.f, 0.f }, 0.075f, 0.0f, 0.04f, 0.0f, 1.0f, 5000.f, 250.f, 0.f, AL_TRUE };

// reverb lerb speed
#define REVERB_LERP_SPEED    2.0f

static float s_reverbLerp = 0.f;
static int s_lerpFromPreset = 0;
static bool s_reverbDirty = true;

void AL_SoundInfo(void)
{
    Com_Printf("AL_VENDOR: %s\n", alGetString(AL_VENDOR));
    Com_Printf("AL_RENDERER: %s\n", alGetString(AL_RENDERER));
    Com_Printf("AL_VERSION: %s\n", alGetString(AL_VERSION));
    Com_Printf("AL_EXTENSIONS: %s\n", alGetString(AL_EXTENSIONS));
    Com_Printf("Number of sources: %d\n", s_numchannels);
}

/*
* Set up the stream sources
*/
static void
AL_InitStreamSource(void)
{
	alSource3f(streamSource, AL_POSITION, 0.0, 0.0, 0.0);
	alSource3f(streamSource, AL_VELOCITY, 0.0, 0.0, 0.0);
	alSource3f(streamSource, AL_DIRECTION, 0.0, 0.0, 0.0);
	alSourcef(streamSource, AL_ROLLOFF_FACTOR, 0.0);
	alSourcei(streamSource, AL_BUFFER, 0);
	alSourcei(streamSource, AL_LOOPING, AL_FALSE);
	alSourcei(streamSource, AL_SOURCE_RELATIVE, AL_TRUE);
}

/*
* Silence / stop all OpenAL streams
*/
static void
AL_StreamDie(void)
{
	int numBuffers;

	streamPlaying = false;
	alSourceStop(streamSource);

	/* Un-queue any buffers, and delete them */
	alGetSourcei(streamSource, AL_BUFFERS_QUEUED, &numBuffers);

	while (numBuffers--)
	{
		ALuint buffer;
		alSourceUnqueueBuffers(streamSource, 1, &buffer);
		alDeleteBuffers(1, &buffer);
		active_buffers--;
	}
}

/*
* Updates stream sources by removing all played
* buffers and restarting playback if necessary.
*/
static void
AL_StreamUpdate(void)
{
	int numBuffers;
	ALint state;

	alGetSourcei(streamSource, AL_SOURCE_STATE, &state);

	if (state == AL_STOPPED)
	{
		streamPlaying = false;
	}
	else
	{
		/* Un-queue any already played buffers and delete them */
		alGetSourcei(streamSource, AL_BUFFERS_PROCESSED, &numBuffers);

		while (numBuffers--)
		{
			ALuint buffer;
			alSourceUnqueueBuffers(streamSource, 1, &buffer);
			alDeleteBuffers(1, &buffer);
			active_buffers--;
		}
	}

	/* Start the streamSource playing if necessary */
	alGetSourcei(streamSource, AL_BUFFERS_QUEUED, &numBuffers);

	if (!streamPlaying && numBuffers)
	{
		alSourcePlay(streamSource);
		streamPlaying = true;
	}
}

bool AL_Init(void)
{
    int i;

    Com_DPrintf("Initializing OpenAL\n");

    if (!QAL_Init()) {
        goto fail0;
    }

    // check for linear distance extension
    if (!alIsExtensionPresent("AL_EXT_LINEAR_DISTANCE")) {
        Com_SetLastError("AL_EXT_LINEAR_DISTANCE extension is missing");
        goto fail1;
    }

    s_testReverb = Cvar_Get("s_testReverb", "0", 0);

	/* generate source names */
	alGetError();
	alGenSources(1, &streamSource);

	if (alGetError() != AL_NO_ERROR)
	{
		Com_Printf("ERROR: Couldn't get a single Source.\n");
		QAL_Shutdown();
		return false;
	}
	else
	{
		for (i = 0; i < MAX_CHANNELS; i++) {
			alGenSources(1, &s_srcnums[i]);
			if (alGetError() != AL_NO_ERROR) {
				break;
			}
		}
	}

    Com_DPrintf("Got %d AL sources\n", i);

    if (i < MIN_CHANNELS) {
        Com_SetLastError("Insufficient number of AL sources");
        goto fail1;
    }

    s_numchannels = i;
	AL_InitStreamSource();

    alGenEffects(1, &s_effect);

    alEffecti(s_effect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);

    alGenAuxiliaryEffectSlots(1, &s_auxEffectSlot);

    alAuxiliaryEffectSloti(s_auxEffectSlot, AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL);

    alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);

    // Approximate speed of sound (assumes 1 meter = 16 units)
    alSpeedOfSound(343.3 * 16.f);

    Com_Printf("OpenAL initialized.\n");

    AL_SoundInfo();

    return true;

fail1:
    QAL_Shutdown();
fail0:
    Com_EPrintf("Failed to initialize OpenAL: %s\n", Com_GetLastError());
    return false;
}

void AL_Shutdown(void)
{
    Com_Printf("Shutting down OpenAL.\n");

	AL_StopAllChannels();

	alDeleteSources(1, &streamSource);
    alDeleteEffects(1, &s_effect);
    alDeleteAuxiliaryEffectSlots(1, &s_auxEffectSlot);

    if (s_numchannels) {
        // delete source names
        alDeleteSources(s_numchannels, s_srcnums);
        memset(s_srcnums, 0, sizeof(s_srcnums));
        s_numchannels = 0;
    }

    QAL_Shutdown();
}

sfxcache_t *AL_UploadSfx(sfx_t *s)
{
    sfxcache_t *sc;
    ALsizei size = s_info.samples * s_info.width;
    ALenum format = s_info.width == 2 ? AL_FORMAT_MONO16 : AL_FORMAT_MONO8;
    ALuint name;

    if (!size) {
        s->error = Q_ERR_TOO_FEW;
        return NULL;
    }

    alGetError();
    alGenBuffers(1, &name);
    alBufferData(name, format, s_info.data, size, s_info.rate);
    if (alGetError() != AL_NO_ERROR) {
        s->error = Q_ERR_LIBRARY_ERROR;
        return NULL;
    }

#if 0
    // specify OpenAL-Soft style loop points
    if (s_info.loopstart > 0 && alIsExtensionPresent("AL_SOFT_loop_points")) {
        ALint points[2] = { s_info.loopstart, s_info.samples };
        alBufferiv(name, AL_LOOP_POINTS_SOFT, points);
    }
#endif

    // allocate placeholder sfxcache
    sc = s->cache = S_Malloc(sizeof(*sc));
    sc->length = s_info.samples * 1000 / s_info.rate; // in msec
    sc->loopstart = s_info.loopstart;
    sc->width = s_info.width;
    sc->size = size;
    sc->bufnum = name;

    return sc;
}

void AL_DeleteSfx(sfx_t *s)
{
    sfxcache_t *sc;
    ALuint name;

    sc = s->cache;
    if (!sc) {
        return;
    }

    name = sc->bufnum;
    alDeleteBuffers(1, &name);
}

#define TONES_PER_OCTAVE	48

static void AL_Spatialize(channel_t *ch)
{
    vec3_t      origin;

    // anything coming from the view entity will always be full volume
    // no attenuation = no spatialization
    if (ch->entnum == -1 || ch->entnum == listener_entnum || !ch->dist_mult) {
        VectorCopy(listener_origin, origin);
    } else if (ch->fixed_origin) {
        VectorCopy(ch->origin, origin);
    } else {
        CL_GetEntitySoundOrigin(ch->entnum, origin);
    }

    alSource3f(ch->srcnum, AL_POSITION, AL_UnpackVector(origin));

	// offset pitch by sound-requested offset
    float pitch = 1.f;

	if (ch->pitch) {
		const float octaves = (float) pow(2.0, 0.69314718 * ((float) ch->pitch / TONES_PER_OCTAVE));
		pitch *= octaves;
	}

    alSourcef(ch->srcnum, AL_PITCH, pitch);

    if (!ch->dist_mult) {
        alSource3i(ch->srcnum, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);
    } else {
        alSource3i(ch->srcnum, AL_AUXILIARY_SEND_FILTER, (ALint) s_auxEffectSlot, 0, AL_FILTER_NULL);
    }
}

void AL_StopChannel(channel_t *ch)
{
#if USE_DEBUG
    if (s_show->integer > 1)
        Com_Printf("%s: %s\n", __func__, ch->sfx->name);
#endif

    // stop it
    alSourceStop(ch->srcnum);
    alSourcei(ch->srcnum, AL_BUFFER, AL_NONE);
    memset(ch, 0, sizeof(*ch));
}

void AL_PlayChannel(channel_t *ch)
{
    sfxcache_t *sc = ch->sfx->cache;

#if USE_DEBUG
    if (s_show->integer > 1)
        Com_Printf("%s: %s\n", __func__, ch->sfx->name);
#endif

    ch->srcnum = s_srcnums[ch - channels];
    alGetError();
    alSourcei(ch->srcnum, AL_BUFFER, sc->bufnum);
    if (ch->autosound /*|| sc->loopstart >= 0*/) {
        alSourcei(ch->srcnum, AL_LOOPING, AL_TRUE);
    } else {
        alSourcei(ch->srcnum, AL_LOOPING, AL_FALSE);
    }
    alSourcef(ch->srcnum, AL_GAIN, ch->master_vol);
    alSourcef(ch->srcnum, AL_REFERENCE_DISTANCE, SOUND_FULLVOLUME);
    alSourcef(ch->srcnum, AL_MAX_DISTANCE, 8192);
    alSourcef(ch->srcnum, AL_ROLLOFF_FACTOR, ch->dist_mult * (8192 - SOUND_FULLVOLUME));

    AL_Spatialize(ch);

    // play it
    alSourcePlay(ch->srcnum);
    if (alGetError() != AL_NO_ERROR) {
        AL_StopChannel(ch);
    }
}

static void AL_IssuePlaysounds(void)
{
    playsound_t *ps;

    // start any playsounds
    while (1) {
        ps = s_pendingplays.next;
        if (ps == &s_pendingplays)
            break;  // no more pending sounds
        if (ps->begin > paintedtime)
            break;
        S_IssuePlaysound(ps);
    }
}

void AL_StopAllChannels(void)
{
    int         i;
    channel_t   *ch;

    ch = channels;
    for (i = 0; i < s_numchannels; i++, ch++) {
        if (!ch->sfx)
            continue;
        AL_StopChannel(ch);
    }
}

static channel_t *AL_FindLoopingSound(int entnum, sfx_t *sfx)
{
    int         i;
    channel_t   *ch;

    ch = channels;
    for (i = 0; i < s_numchannels; i++, ch++) {
        if (!ch->sfx)
            continue;
        if (!ch->autosound)
            continue;
        if (entnum && ch->entnum != entnum)
            continue;
        if (ch->sfx != sfx)
            continue;
        return ch;
    }

    return NULL;
}

static entity_sound_t sounds[MAX_PACKET_ENTITIES + MAX_AMBIENT_ENTITIES];

static void AL_AddLoopSound(int32_t i)
{
    channel_t   *ch, *ch2;
    sfx_t       *sfx;
    sfxcache_t  *sc;
    int         num;
    entity_state_t  *ent;
    vec3_t      origin;
    float       dist;

    if (!sounds[i].sound)
        return;

    sfx = S_SfxForHandle(cl.sound_precache[sounds[i].sound]);
    if (!sfx)
        return;       // bad sound effect
    sc = sfx->cache;
    if (!sc)
        return;

    if (Ent_IsPacket(i)) {
        num = (cl.frame.firstEntity + i) & PARSE_ENTITIES_MASK;
        ent = &cl.entityStates[num];
    } else {
        ent = &cl.ambients[i - MAX_PACKET_ENTITIES];
    }

    ch = AL_FindLoopingSound(ent->number, sfx);
    if (ch) {
        ch->autoframe = s_framecount;
        ch->end = paintedtime + sc->length;
        ch->pitch = sounds[i].pitch;
        return;
    }

    // check attenuation before playing the sound
    CL_GetEntitySoundOrigin(ent->number, origin);
    VectorSubtract(origin, listener_origin, origin);
    dist = VectorNormalize(origin);
    dist = (dist - SOUND_FULLVOLUME) * SOUND_LOOPATTENUATE;
    if(dist >= 1.f)
        return; // completely attenuated

                  // allocate a channel
    ch = S_PickChannel(0, 0);
    if (!ch)
        return;

    ch2 = AL_FindLoopingSound(0, sfx);

    ch->autosound = true;   // remove next frame
    ch->autoframe = s_framecount;
    ch->sfx = sfx;
    ch->entnum = ent->number;
    ch->master_vol = 1;
    ch->dist_mult = SOUND_LOOPATTENUATE;
    ch->end = paintedtime + sc->length;
    ch->pitch = sounds[i].pitch;

    AL_PlayChannel(ch);

    // attempt to synchronize with existing sounds of the same type
    if (ch2) {
        ALint offset;

        alGetSourcei(ch2->srcnum, AL_SAMPLE_OFFSET, &offset);
        alSourcei(ch->srcnum, AL_SAMPLE_OFFSET, offset);
    }
}

static void AL_AddLoopSounds(void)
{
    if (cls.state != ca_active || sv_paused->integer || !s_ambient->integer) {
        return;
    }

    S_BuildSoundList(sounds);

    for (int i = 0; i < cl.frame.numEntities; i++) {
        AL_AddLoopSound(i);
    }

    for (int i = OFFSET_AMBIENT_ENTITIES; i < OFFSET_AMBIENT_ENTITIES + cl.num_ambient_entities; i++) {
        AL_AddLoopSound(i);
    }
}

static void AL_SetReverb(void)
{
    int32_t preset = 0;

    if (cls.state != ca_active) {
        preset = 0;
    } else if (s_testReverb->integer) {
        preset = s_testReverb->integer;
        clamp(preset, 0, q_countof(reverb_presets) + 1);
    } else {
        preset = cl.frame.ps.reverb;
    }

    if (s_activePreset != preset) {

        if (cls.state == ca_active) {
            s_lerpFromPreset = s_activePreset;
            s_reverbLerp = 1.0f;
        } else {
            s_lerpFromPreset = q_countof(reverb_presets) - 1;
        }

        s_activePreset = preset;
        s_reverbDirty = true;
    }

    // reverb parameters not changed
    if (!s_reverbDirty) {
        return;
    }

    // simple path: no active reverb and not lerping
    if (s_activePreset == 0 && s_lerpFromPreset == 0) {

        alAuxiliaryEffectSloti(s_auxEffectSlot, AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL);
        s_reverbDirty = false;
        return;
    }

    EFXEAXREVERBPROPERTIES activeReverb = s_activePreset ? reverb_presets[s_activePreset - 1] : reverb_identity;

    if (s_reverbLerp > 0) {
        s_reverbLerp = max(0.f, s_reverbLerp - cls.frametime * REVERB_LERP_SPEED);

        const EFXEAXREVERBPROPERTIES *lerpReverb = s_lerpFromPreset ? &reverb_presets[s_lerpFromPreset - 1] : &reverb_identity;
#define REVERB_MIX(p) activeReverb.p = Mix(lerpReverb->p, activeReverb.p, s_reverbLerp)
        REVERB_MIX(flDensity);
        REVERB_MIX(flDiffusion);
        REVERB_MIX(flGain);
        REVERB_MIX(flGainHF);
        REVERB_MIX(flGainLF);
        REVERB_MIX(flDecayTime);
        REVERB_MIX(flDecayHFRatio);
        REVERB_MIX(flDecayLFRatio);
        REVERB_MIX(flReflectionsGain);
        REVERB_MIX(flReflectionsDelay);
        REVERB_MIX(flReflectionsPan[0]);
        REVERB_MIX(flReflectionsPan[1]);
        REVERB_MIX(flReflectionsPan[2]);
        REVERB_MIX(flLateReverbGain);
        REVERB_MIX(flLateReverbDelay);
        REVERB_MIX(flLateReverbPan[0]);
        REVERB_MIX(flLateReverbPan[1]);
        REVERB_MIX(flLateReverbPan[2]);
        REVERB_MIX(flEchoTime);
        REVERB_MIX(flEchoDepth);
        REVERB_MIX(flModulationTime);
        REVERB_MIX(flModulationDepth);
        REVERB_MIX(flAirAbsorptionGainHF);
        REVERB_MIX(flHFReference);
        REVERB_MIX(flLFReference);
        REVERB_MIX(flRoomRolloffFactor);
        REVERB_MIX(iDecayHFLimit);

        if (s_reverbLerp == 0.f) {
            s_reverbDirty = false;
        }
    } else {
        s_reverbDirty = false;
    }

    alEffectf(s_effect, AL_EAXREVERB_DENSITY, activeReverb.flDensity);
    alEffectf(s_effect, AL_EAXREVERB_DIFFUSION, activeReverb.flDiffusion);
    alEffectf(s_effect, AL_EAXREVERB_GAIN, activeReverb.flGain);
    alEffectf(s_effect, AL_EAXREVERB_GAINHF, activeReverb.flGainHF);
    alEffectf(s_effect, AL_EAXREVERB_GAINLF, activeReverb.flGainLF);
    alEffectf(s_effect, AL_EAXREVERB_DECAY_TIME, activeReverb.flDecayTime);
    alEffectf(s_effect, AL_EAXREVERB_DECAY_HFRATIO, activeReverb.flDecayHFRatio);
    alEffectf(s_effect, AL_EAXREVERB_DECAY_LFRATIO, activeReverb.flDecayLFRatio);
    alEffectf(s_effect, AL_EAXREVERB_REFLECTIONS_GAIN, activeReverb.flReflectionsGain);
    alEffectf(s_effect, AL_EAXREVERB_REFLECTIONS_DELAY, activeReverb.flReflectionsDelay);
    alEffectfv(s_effect, AL_EAXREVERB_REFLECTIONS_PAN, activeReverb.flReflectionsPan);
    alEffectf(s_effect, AL_EAXREVERB_LATE_REVERB_GAIN, activeReverb.flLateReverbGain);
    alEffectf(s_effect, AL_EAXREVERB_LATE_REVERB_DELAY, activeReverb.flLateReverbDelay);
    alEffectfv(s_effect, AL_EAXREVERB_LATE_REVERB_PAN, activeReverb.flLateReverbPan);
    alEffectf(s_effect, AL_EAXREVERB_ECHO_TIME, activeReverb.flEchoTime);
    alEffectf(s_effect, AL_EAXREVERB_ECHO_DEPTH, activeReverb.flEchoDepth);
    alEffectf(s_effect, AL_EAXREVERB_MODULATION_TIME, activeReverb.flModulationTime);
    alEffectf(s_effect, AL_EAXREVERB_MODULATION_DEPTH, activeReverb.flModulationDepth);
    alEffectf(s_effect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, activeReverb.flAirAbsorptionGainHF);
    alEffectf(s_effect, AL_EAXREVERB_HFREFERENCE, activeReverb.flHFReference);
    alEffectf(s_effect, AL_EAXREVERB_LFREFERENCE, activeReverb.flLFReference);
    alEffectf(s_effect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, activeReverb.flRoomRolloffFactor);
    alEffecti(s_effect, AL_EAXREVERB_DECAY_HFLIMIT, activeReverb.iDecayHFLimit);

    alAuxiliaryEffectSloti(s_auxEffectSlot, AL_EFFECTSLOT_EFFECT, (ALint)s_effect);
}

void AL_Update(void)
{
    int         i;
    channel_t   *ch;
    vec_t       orientation[6];

    if (!s_active) {
        return;
    }

    paintedtime = cl.time;

    // set listener parameters
    alListener3f(AL_POSITION, AL_UnpackVector(listener_origin));
    AL_CopyVector(listener_forward, orientation);
    AL_CopyVector(listener_up, orientation + 3);
    alListenerfv(AL_ORIENTATION, orientation);
    alListenerf(AL_GAIN, S_GetLinearVolume(s_volume->value));

    // update spatialization for dynamic sounds
    ch = channels;
    for (i = 0; i < s_numchannels; i++, ch++) {
        if (!ch->sfx)
            continue;

        if (ch->autosound) {
            // autosounds are regenerated fresh each frame
            if (ch->autoframe != s_framecount) {
                AL_StopChannel(ch);
                continue;
            }
        } else {
            ALenum state;

            alGetError();
            alGetSourcei(ch->srcnum, AL_SOURCE_STATE, &state);
            if (alGetError() != AL_NO_ERROR || state == AL_STOPPED) {
                AL_StopChannel(ch);
                continue;
            }
        }

#if USE_DEBUG
        if (s_show->integer) {
            Com_Printf("%.1f %s\n", ch->master_vol, ch->sfx->name);
            //    total++;
        }
#endif

        AL_Spatialize(ch);         // respatialize channel
    }

    s_framecount++;

    // add loopsounds
    AL_AddLoopSounds();

	AL_StreamUpdate();
    AL_IssuePlaysounds();

    AL_SetReverb();
}

/*
* Queues raw samples for playback. Used
* by the background music an cinematics.
*/
void
AL_RawSamples(int samples, int rate, int width, int channels,
	byte *data, float volume)
{
	ALuint buffer;
	ALuint format = 0;

	/* Work out format */
	if (width == 1)
	{
		if (channels == 1)
		{
			format = AL_FORMAT_MONO8;
		}
		else if (channels == 2)
		{
			format = AL_FORMAT_STEREO8;
		}
	}
	else if (width == 2)
	{
		if (channels == 1)
		{
			format = AL_FORMAT_MONO16;
		}
		else if (channels == 2)
		{
			format = AL_FORMAT_STEREO16;
		}
	}

	/* Create a buffer, and stuff the data into it */
	alGenBuffers(1, &buffer);
	alBufferData(buffer, format, (ALvoid *)data,
		(samples * width * channels), rate);
	active_buffers++;

	/* set volume */
	if (volume > 1.0f)
	{
		volume = 1.0f;
	}

	alSourcef(streamSource, AL_GAIN, volume);

	/* Shove the data onto the streamSource */
	alSourceQueueBuffers(streamSource, 1, &buffer);
}

/*
* Kills all raw samples still in flight.
* This is used to stop music playback
* when silence is triggered.
*/
void
AL_UnqueueRawSamples()
{
	AL_StreamDie();
}