#ifndef SPECIAL_EFFECTS_DEF_C
#define SPECIAL_EFFECTS_DEF_C

/* These are some more specialized types of effects that require lots of perverse insertions */
#define INVERSION_TIME_BETWEEN_FLASHES (0.07)
#define INVERSION_FLASH_MAX            (12)

enum special_effect_id {
    SPECIAL_EFFECT_NONE,
    SPECIAL_EFFECT_INVERSION_1     = 1,
    /* crossfade on the framebuffer */
    SPECIAL_EFFECT_CROSSFADE_SCENE = 2,
};

struct special_effect_information {
    s32 type;

    s32 flashes;
    f32 time;

    f32 crossfade_timer;
    f32 crossfade_max_timer;
    f32 crossfade_delay_timer;
    /* use the global copy framebuffer */
} special_full_effects;

bool special_effects_active(void);
void special_effect_start_inversion(void);
void special_effect_stop_effects(void);
void game_do_special_effects(struct software_framebuffer* framebuffer, f32 dt);
void special_effect_start_crossfade_scene(f32 delay_before_fade, f32 fade_time);
union color32f32 game_background_things_shader(struct software_framebuffer* framebuffer, union color32f32 source_pixel, v2f32 pixel_position, void* context);
union color32f32 game_foreground_things_shader(struct software_framebuffer* framebuffer, union color32f32 source_pixel, v2f32 pixel_position, void* context);

#endif
