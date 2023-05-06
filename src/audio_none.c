#include "audio_def.c"

void audio_initialize(void) {
    _debugprintf("hi I'm deaf");
}

void audio_deinitialize(void) {
    _debugprintf("bye I'm dead");
}

sound_id load_sound(string filepath, bool streamed) {
    sound_id result = {};
    return result;
}

void play_sound(sound_id sound) {
}

void play_sound_fadein(sound_id sound, s32 fadein_ms) {
    
}
void stop_music_fadeout(s32 fadeout_ms) {
    
}
bool music_playing(void) {
    return false;
}
