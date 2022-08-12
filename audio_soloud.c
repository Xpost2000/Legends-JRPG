#include "audio_def.c"

#include <soloud/soloud_c.h>

local Soloud global_soloud = NULL;

struct {
    u32 count;
    Wav wav_objects[MAX_LOADED_SAMPLES];
} global_samples = {};

struct {
    u32 count;
    WavStream wav_objects[MAX_LOADED_STREAMS];
} global_streams = {};

void audio_initialize(void) {
    global_soloud = Soloud_create();
    Soloud_initEx(&global_soloud, SOLOUD_CLIP_ROUNDOFF, SOLOUD_SDL2, SOLOUD_AUTO, SOLOUD_AUTO, 2);
    _debugprintf("hi soloud");
}

void audio_deinitialize(void) {
    _debugprintf("bye soloud");
    Soloud_deinit(&global_soloud);
    Soloud_destroy(&global_soloud);
}

sound_id load_sound(string filepath, bool streamed) {
    sound_id result = {};

    if (streamed) {
        WavStream new_wavstream;
        WavStream_load(&new_wavstream, (const char*)filepath.data);

        result.streaming = true;
        result.index     = global_streams.count + 1;

        global_streams.wav_objects[global_streams.count++] = new_wavstream;
    } else {
        Wav new_wav;
        Wav_load(&new_wav, (const char*)filepath.data);

        result.index     = global_samples.count + 1;

        global_samples.wav_objects[global_samples.count++] = new_wav;
    }

    return result;
}

void play_sound(sound_id sound) {
    AudioSource source;

    if (sound.streaming) {
        source = global_streams.wav_objects[sound.index-1];
    } else {
        source = global_samples.wav_objects[sound.index-1];
    }

    Soloud_play(&global_soloud, &source);
}
