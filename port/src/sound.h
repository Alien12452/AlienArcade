/* Sound effects. Matching source: src/sound.c (segment 23).
   One voice, like PlaySfx / PlaySoundEx: a new effect is dropped while
   one is still playing, unless force cuts the current one off.
   OpenWaveOut ignores its third argument, so the port does too. */

#ifndef SOUND_H
#define SOUND_H

void sound_open(const char *resources);
void sound_close(void);
void sound_tick(void);
void sound_play(int id, int force);
/* PreloadSounds: load id .. id+n-1 into the clip cache (no-op when muted). */
void sound_preload(int id, int n);
/* SelectVolume. Step 0 is mute. 1..9 scale PCM by the port amplitude ladder
   (not a literal multiply of unused g_anVolume). */
void sound_volume(int step);

#endif
