/*
  This is mostly done using external libraries, as parsing audio formats and decoding
  is kind of scary by hand.

  The included backend uses SoLoud, since it's better than SDL2 mixer and seems nice.
  
  TODO: volume settings.
  TODO: stopping actively playing audio sources.
*/
#ifndef AUDIO_DEF_C
#define AUDIO_DEF_C

#define MAX_LOADED_SAMPLES (2048)
#define MAX_LOADED_STREAMS (512)

typedef struct sound_id {
    u8  streaming;
    s32 index;
} sound_id;

void audio_initialize(void);
void audio_deinitialize(void);

sound_id load_sound(string filepath, bool streamed);
void     play_sound(sound_id sound);
void     stop_music(void);
void     play_sound_fadein(sound_id sound, s32 fadein_ms);
void     stop_music_fadeout(s32 fadeout_ms);
bool     music_playing(void);

#endif
