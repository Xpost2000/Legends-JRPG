#include "weather_def.c"

bool weather_any_active(struct game_state* state) {
    return (Get_Bit(state->weather.features, WEATHER_RAIN)  |
            Get_Bit(state->weather.features, WEATHER_SNOW)  |
            Get_Bit(state->weather.features, WEATHER_FOGGY) |
            Get_Bit(state->weather.features, WEATHER_STORMY));
}

void weather_clear(struct game_state* state) {
    zero_memory(&state->weather, sizeof(state->weather));
}

void weather_start_rain(struct game_state* state) {
    struct weather* weather = &state->weather;

    /* NOTE needs framebuffer to determine width/height */
    for (s32 index = 0; index < MAX_WEATHER_PARTICLES; ++index) {
        v2f32* rain_droplet_position = weather->rain_positions + index;
        rain_droplet_position->x = random_ranged_integer(&state->rng, 0, 640);
        rain_droplet_position->y = random_ranged_integer(&state->rng, 0, 480) - 480;
    }

    Set_Bit(weather->features, WEATHER_RAIN);
}

void weather_start_snow(struct game_state* state) {
    struct weather* weather = &state->weather;

    /* NOTE needs framebuffer to determine width/height */
    for (s32 index = 0; index < MAX_WEATHER_PARTICLES; ++index) {
        v2f32* snow_droplet_position = weather->snow_positions + index;
        snow_droplet_position->x = random_ranged_integer(&state->rng, 0, 640);
        snow_droplet_position->y = random_ranged_integer(&state->rng, 0, 480) - 480;
    }

    Set_Bit(weather->features, WEATHER_SNOW);
}


void weather_render_rain(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    /* when setting up rain, simulate each particle individually... */
    while (state->weather.timer > 1.0/120.0f) {
        for (s32 index = 0; index < MAX_WEATHER_PARTICLES; ++index) {
            v2f32* current_position = &state->weather.rain_positions[index];

            /* this needs to have more variance */
            current_position->x -= 350 * 1.0/60.0f;
            current_position->y += 800 * 1.0/60.0f;

            if (current_position->x < 0)                   current_position->x = framebuffer->width;
            if (current_position->y > framebuffer->height) current_position->y = 0;
        }

        state->weather.timer -= 1.0/60.0f;
    }

    for (s32 index = 0; index < MAX_WEATHER_PARTICLES; ++index) {
        v2f32* current_position = &state->weather.rain_positions[index];
        software_framebuffer_draw_line(framebuffer, *current_position, v2f32_add(*current_position, v2f32(-3, 8)), color32u8(128, 128, 255, 255), BLEND_MODE_ALPHA);
    }
}

void weather_render_snow(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    while (state->weather.timer > 1.0/120.0f) {
        for (s32 index = 0; index < MAX_WEATHER_PARTICLES; ++index) {
            v2f32* current_position = &state->weather.snow_positions[index];

            /* this needs to have more variance */
            /* current_position->x -= 350 * 1.0/60.0f; */
            current_position->y += 90 * 1.0/60.0f;

            if (current_position->x < 0)                   current_position->x = framebuffer->width;
            if (current_position->y > framebuffer->height) current_position->y = 0;
        }

        state->weather.timer -= 1.0/60.0f;
    }

    for (s32 index = 0; index < MAX_WEATHER_PARTICLES; ++index) {
        /* v2f32 current_position = v2f32_add(state->weather.snow_positions[index], v2f32(random_ranged_integer(&state->rng, -3, 3), 0)); */
        v2f32 current_position = v2f32_add(state->weather.snow_positions[index], v2f32(sinf(global_elapsed_time*5 + index*3.141592654) * 8, 0));
        software_framebuffer_draw_line(framebuffer, current_position, v2f32_add(current_position, v2f32(0, 2)), color32u8(255, 255, 255, 255), BLEND_MODE_ALPHA);
    }
}
