#include "sound.h"

#include <SDL3/SDL.h>

/* Matching source: src/sound.c (segment 23). Cross-refs:
   InitSound (0017:0000)       sound_open
   FinishWave (0017:003E)      finish
   TickSound (0017:008B)       sound_tick
   StopSfx (0017:00A2)         force path in sound_play
   CloseSfx (0017:00E1)        sound_close
   LoadSound (0017:013C)       load_clip
   PreloadSounds (0017:0219)   sound_preload
   PlaySfx / PlaySoundEx       sound_play
     (0017:025F / 0017:0275)
   SelectVolume (0017:02D5)    sound_volume
   g_anVolume (0024:0948)      unused for PCM in orig; see k_amp
   Cache: orig g_ahSnd[16] / g_nSndNext; port uses 48 slots
   Speaker path (src/speaker.c) is unused; the port always uses WAVE files. */

/* port: larger than orig 16 — more preloads without evicting. */
#define CACHE 48

typedef struct Clip {
    int id;
    Uint8 *data;
    int len;
} Clip;

static char *res_root;
static SDL_AudioStream *stream;
static SDL_AudioSpec spec;
static Clip cache[CACHE];
static int cache_n;
static int cache_next;
static int busy;
static Uint64 done_at;
/* SelectVolume step. 0 = mute (g_fSoundOn = 0); 1..9 scale PCM via k_amp. */
static int volume_step = 9;
/* Orig g_anVolume was 0,0x40,0x80,0x400,…,0xFFFF and only toggled sound on.
   Raw PCM multiply of those values mutes Ctrl+1..3. Port ladder ≈12%→100%. */
static const int k_amp[10] = {
    0, 0x2000, 0x3000, 0x4000, 0x5000, 0x6000, 0x8000, 0xA000, 0xC000, 0xFFFF
};

/* orig: FinishWave (0017:003E) — drain when the clip ends */
static void finish(void)
{
    int queued;
    int available;

    if (!busy || !stream)
        return;
    queued = SDL_GetAudioStreamQueued(stream);
    available = SDL_GetAudioStreamAvailable(stream);
    if ((queued == 0 && available == 0) || SDL_GetTicks() >= done_at) {
        if (queued != 0 || available != 0)
            SDL_ClearAudioStream(stream);
        busy = 0;
    }
}

/* orig: LoadSound (0017:013C) */
static Clip *load_clip(int id)
{
    char path[1024];
    SDL_AudioSpec got;
    Uint8 *raw = NULL;
    Uint32 raw_len = 0;
    Clip *slot;
    int i;

    for (i = 0; i < cache_n; i++) {
        if (cache[i].id == id && cache[i].data)
            return &cache[i];
    }
    if (!res_root)
        return NULL;
    SDL_snprintf(path, sizeof(path), "%s/AARCADE/WAVE/%d.wav", res_root, id);
    if (!SDL_LoadWAV(path, &got, &raw, &raw_len) || !raw || raw_len == 0)
        return NULL;
    if (cache_n < CACHE)
        slot = &cache[cache_n++];
    else {
        slot = &cache[cache_next];
        cache_next = (cache_next + 1) % CACHE;
        SDL_free(slot->data);
        slot->data = NULL;
    }
    slot->id = id;
    slot->data = NULL;
    slot->len = 0;
    if (got.format == spec.format && got.channels == spec.channels && got.freq == spec.freq) {
        slot->data = raw;
        slot->len = (int)raw_len;
        raw = NULL;
    } else {
        Uint8 *conv = NULL;
        int conv_len = 0;

        if (SDL_ConvertAudioSamples(&got, raw, (int)raw_len, &spec, &conv, &conv_len) && conv) {
            slot->data = conv;
            slot->len = conv_len;
        }
    }
    SDL_free(raw);
    if (!slot->data) {
        slot->id = 0;
        if (cache_n > 0 && slot == &cache[cache_n - 1])
            cache_n--;
        return NULL;
    }
    return slot;
}

/* orig: InitSound (0017:0000) */
void sound_open(const char *resources)
{
    if (!resources)
        return;
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_Log("sound: %s", SDL_GetError());
        return;
    }
    spec.format = SDL_AUDIO_U8;
    spec.channels = 1;
    spec.freq = 11025;
    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!stream) {
        SDL_Log("sound: %s", SDL_GetError());
        return;
    }
    if (!SDL_ResumeAudioStreamDevice(stream)) {
        SDL_Log("sound: %s", SDL_GetError());
        SDL_DestroyAudioStream(stream);
        stream = NULL;
        return;
    }
    res_root = SDL_strdup(resources);
}

/* orig: CloseSfx (0017:00E1) */
void sound_close(void)
{
    int i;

    if (stream) {
        SDL_DestroyAudioStream(stream);
        stream = NULL;
    }
    for (i = 0; i < cache_n; i++)
        SDL_free(cache[i].data);
    cache_n = 0;
    cache_next = 0;
    busy = 0;
    done_at = 0;
    SDL_free(res_root);
    res_root = NULL;
}

/* orig: TickSound (0017:008B) */
void sound_tick(void)
{
    finish();
}

/* orig: SelectVolume (0017:02D5) — step 0 mutes; 1..9 set amplitude */
void sound_volume(int step)
{
    if (step < 0)
        step = 0;
    if (step > 9)
        step = 9;
    volume_step = step;
}

/* orig: PreloadSounds (0017:0219) — skipped when sound is off */
void sound_preload(int id, int n)
{
    int i;

    if (volume_step == 0 || n < 0)
        return;
    for (i = 0; i < n; i++)
        load_clip(id + i);
}

/* orig: PlaySfx (0017:025F) / PlaySoundEx (0017:0275);
   force path matches StopSfx (0017:00A2) then StartWave */
void sound_play(int id, int force)
{
    Clip *clip;
    int amp = k_amp[volume_step];
    Uint8 *scaled = NULL;
    const void *pcm;
    int i;

    if (!stream || amp == 0)
        return;
    finish();
    if (busy && !force)
        return;
    if (busy) {
        SDL_ClearAudioStream(stream);
        busy = 0;
    }
    clip = load_clip(id);
    if (!clip)
        return;
    pcm = clip->data;
    if (amp != 0xFFFF) {
        scaled = SDL_malloc((size_t)clip->len);
        if (!scaled)
            return;
        for (i = 0; i < clip->len; i++) {
            int s = (int)clip->data[i] - 128;

            s = s * amp / 65535;
            scaled[i] = (Uint8)(s + 128);
        }
        pcm = scaled;
    }
    if (!SDL_PutAudioStreamData(stream, pcm, clip->len)) {
        SDL_free(scaled);
        return;
    }
    SDL_free(scaled);
    SDL_FlushAudioStream(stream);
    /* U8 mono: one byte per sample. A little past the clip, in case the
       device is still holding the tail when the queues already read empty. */
    done_at = SDL_GetTicks() + (Uint64)((clip->len * 1000) / spec.freq) + 80;
    busy = 1;
}
