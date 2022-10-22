#include "special_effects_def.c"

bool special_effects_active(void) {
    if (special_full_effects.type != SPECIAL_EFFECT_NONE) {
        return true;
    }
    return false;
}
void special_effect_start_inversion(void) {
    special_full_effects.type    = SPECIAL_EFFECT_INVERSION_1;
    special_full_effects.flashes = 0;
    special_full_effects.time    = 0;
}

void special_effect_stop_effects(void) {
    special_full_effects.type = 0;
}

void special_effect_start_crossfade_scene(f32 delay_before_fade, f32 fade_time) {
    /* not sure if "blit" would be faster, but let's just do it this way first... */
    memory_copy(global_default_framebuffer.pixels,
                global_copy_framebuffer.pixels,
                global_default_framebuffer.width * global_default_framebuffer.height * sizeof(u32));
    special_full_effects.type                  = SPECIAL_EFFECT_CROSSFADE_SCENE;
    special_full_effects.crossfade_timer       = fade_time;
    special_full_effects.crossfade_delay_timer = delay_before_fade;
    special_full_effects.crossfade_max_timer   = fade_time;
}

void game_do_special_effects(struct software_framebuffer* framebuffer, f32 dt) {
    { /* run special effects code */
        switch (special_full_effects.type) {
            case SPECIAL_EFFECT_NONE: {
                
            } break;
            case SPECIAL_EFFECT_INVERSION_1: {
                if (special_full_effects.time >= INVERSION_TIME_BETWEEN_FLASHES) {
                    special_full_effects.time = 0;
                    special_full_effects.flashes += 1;
                }

                if (special_full_effects.flashes >= INVERSION_FLASH_MAX) {
                    special_effect_stop_effects();
                }

                special_full_effects.time += dt;
            } break;
            case SPECIAL_EFFECT_CROSSFADE_SCENE: {
                f32 alpha = (special_full_effects.crossfade_delay_timer + special_full_effects.crossfade_timer) / special_full_effects.crossfade_max_timer;
                if (alpha < 0) alpha = 0;
                if (alpha > 1) alpha = 1;

                if (special_full_effects.crossfade_delay_timer > 0) {
                    special_full_effects.crossfade_delay_timer -= dt;
                } else {
                    if (special_full_effects.crossfade_timer > 0) {
                        special_full_effects.crossfade_timer -= dt;
                    } else {
                        special_effect_stop_effects();
                    }
                }

                software_framebuffer_draw_image_ex(framebuffer, (struct image_buffer*)&global_copy_framebuffer,
                                                   rectangle_f32(0, 0, framebuffer->width, framebuffer->height),
                                                   RECTANGLE_F32_NULL,
                                                   color32f32(1, 1, 1, alpha),
                                                   NO_FLAGS, BLEND_MODE_ALPHA);
            } break;
            default: {
                unimplemented("This effect was not done?");
            } break;
        }
    }
}

union color32f32 game_background_things_shader(struct software_framebuffer* framebuffer, union color32f32 source_pixel, v2f32 pixel_position, void* context) {
    switch (special_full_effects.type) {
        case SPECIAL_EFFECT_INVERSION_1: {
            if ((special_full_effects.flashes % 2) == 0) { 
                if (source_pixel.a > 0.5) {
                    return color32f32(1, 1, 1, 1);
                } else {
                    return color32f32(0, 0, 0, 1);
                }
            } else {
                if (source_pixel.a > 0.5) {
                    return color32f32(0, 0, 0, 1);
                } else {
                    return color32f32(1, 1, 1, 1);
                }
            }
        } break;
    }
    return source_pixel;
}

union color32f32 game_foreground_things_shader(struct software_framebuffer* framebuffer, union color32f32 source_pixel, v2f32 pixel_position, void* context) {
    switch (special_full_effects.type) {
        case SPECIAL_EFFECT_INVERSION_1: {
            if ((special_full_effects.flashes % 2) == 0) { 
                if (source_pixel.a > 0.5) {
                    return color32f32(0, 0, 0, 1);
                } else {
                    return color32f32(1, 1, 1, 1);
                }
            } else {
                if (source_pixel.a > 0.5) {
                    return color32f32(1, 1, 1, 1);
                } else {
                    return color32f32(0, 0, 0, 1);
                }
            }
        } break;
    }
    return source_pixel;
}
