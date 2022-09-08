#include "audio_def.c"
#include <SDL2/SDL_mixer.h>

local Mix_Chunk* loaded_samples[MAX_LOADED_SAMPLES]={};
local u32        loaded_sample_hashes[MAX_LOADED_SAMPLES]={};
local Mix_Music* loaded_streams[MAX_LOADED_STREAMS]={};
local u32        loaded_stream_hashes[MAX_LOADED_SAMPLES]={};
local s32        loaded_sample_count               = 0;
local s32        loaded_stream_count               = 0;

void audio_initialize(void) {
    Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 2048);
}

void audio_deinitialize(void) {
    Mix_Quit();
}

sound_id load_sound(string filepath, bool streamed) {
    sound_id result;

    u32 path_hash = hash_bytes_fnv1a((u8*)filepath.data, filepath.length);

    if (streamed) {
        result.streaming = 1;

        bool load_required = true;
        for (s32 index = 0; index < loaded_stream_count; ++index) {
            if (path_hash == loaded_stream_hashes[index]) {
                result.index  = index+1;
                load_required = false;
                break;
            }
        }

        if (load_required) {
            Mix_Music* new_stream = Mix_LoadMUS(filepath.data);
            if (new_stream) {
                loaded_streams[loaded_stream_count]       = new_stream;
                loaded_stream_hashes[loaded_stream_count] = hash_bytes_fnv1a((u8*)filepath.data, filepath.length);

                loaded_stream_count++;

                result.index = loaded_stream_count;
            }
        }
    } else {
        bool load_required = true;
        for (s32 index = 0; index < loaded_sample_count; ++index) {
            if (path_hash == loaded_sample_hashes[index]) {
                result.index  = index+1;
                load_required = false;
                break;
            }
        }

        if (load_required) {
            Mix_Chunk* new_chunk = Mix_LoadWAV(filepath.data);
            if (new_chunk) {
                loaded_samples[loaded_sample_count]       = new_chunk;
                loaded_sample_hashes[loaded_sample_count] = hash_bytes_fnv1a((u8*)filepath.data, filepath.length);

                loaded_sample_count++;

                result.index = loaded_sample_count;
            }
        }
    }

    return result;
}

#define ANY_CHANNEL (-1)
void stop_music(void) {
    Mix_HaltMusic();
}

void play_sound(sound_id sound) {
    if (sound.index == 0)
        return;

    if (sound.streaming) {
        Mix_PlayChannel(ANY_CHANNEL, loaded_samples[sound.index-1], 0);
    } else {
        Mix_PlayMusic(loaded_streams[sound.index-1], -1);
    }
}
