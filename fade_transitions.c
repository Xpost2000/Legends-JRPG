/*
  For transitioning effects

  These transitions can also have additional stuff attached to it,
  (not callbacks per say, though basically like that...)
*/

enum transition_fader_type {
    /*
      NOTE:
      While crossfade scene is also a type of transition fader technically, it
      doesn't really obey the rules of any other type of transition (as there's no "fade out" by definition),
      so it's staying as a special effect
    */
    TRANSITION_FADER_TYPE_NONE,
    TRANSITION_FADER_TYPE_COLOR,

    TRANSITION_FADER_TYPE_VERTICAL_SLIDE,
    TRANSITION_FADER_TYPE_HORIZONTAL_SLIDE,

    TRANSITION_FADER_TYPE_SHUTEYE_SLIDE,
    TRANSITION_FADER_TYPE_CURTAINCLOSE_SLIDE,

    TRANSITION_FADER_TYPE_COUNT,
};

typedef void (*transition_on_delay_finish_callback)(void* data);
typedef void (*transition_on_finish_callback)(void* data);
typedef void (*transition_on_start_callback)(void* data);

void ___transition_stubs(void* data){}

#define TRANSITION_MAX_CALLBACK_DATA (Bytes(256))
struct transition_fader_state {
    u32                                 type;
    union color32f32                    color;
    f32                                 max_time;
    f32                                 delay_time;
    f32                                 time;
    bool                                forwards;
    transition_on_delay_finish_callback on_delay_finish;
    transition_on_finish_callback       on_finish;
    transition_on_start_callback        on_start;
    u8                                  callback_data_on_start[TRANSITION_MAX_CALLBACK_DATA];
    u8                                  callback_data_on_finish[TRANSITION_MAX_CALLBACK_DATA];
    u8                                  callback_data_on_delay_finish[TRANSITION_MAX_CALLBACK_DATA];
};

local struct transition_fader_state global_transition_fader_state      = {};
local struct transition_fader_state last_global_transition_fader_state = {};

local void stop_transitions(void) {
    zero_memory(&global_transition_fader_state, sizeof(global_transition_fader_state));
}

local bool transition_fading(void) {
    if (global_transition_fader_state.type != TRANSITION_FADER_TYPE_NONE) {
        if (global_transition_fader_state.delay_time <= 0) {
            if (global_transition_fader_state.time >= global_transition_fader_state.max_time) {
                return false;
            }

            return true;
        } else {
            return true;
        }
    }

    return false;
}

local bool transition_faded_in(void) {
    if (last_global_transition_fader_state.forwards) {
        return true;
    }

    return false;
}

local void transition_register_on_delay_finish(transition_on_delay_finish_callback callback, u8* data, size_t data_length) {
    struct transition_fader_state* state = &global_transition_fader_state;
    state->on_delay_finish               = callback;
    if (data_length > TRANSITION_MAX_CALLBACK_DATA) {
        data_length = TRANSITION_MAX_CALLBACK_DATA;   
    }
    memory_copy(data, state->callback_data_on_delay_finish, data_length);
}
local void transition_register_on_finish(transition_on_finish_callback callback, u8* data, size_t data_length) {
    struct transition_fader_state* state = &global_transition_fader_state;
    state->on_finish                     = callback;
    if (data_length > TRANSITION_MAX_CALLBACK_DATA) {
        data_length = TRANSITION_MAX_CALLBACK_DATA;   
    }
    memory_copy(data, state->callback_data_on_finish, data_length);
}
local void transition_register_on_start(transition_on_start_callback callback, u8* data, size_t data_length) {
    struct transition_fader_state* state = &global_transition_fader_state;
    state->on_start                     = callback;
    if (data_length > TRANSITION_MAX_CALLBACK_DATA) {
        data_length = TRANSITION_MAX_CALLBACK_DATA;   
    }
    memory_copy(data, state->callback_data_on_start, data_length);
}

local void fader_generic_setup(s32 type, union color32f32 color, f32 delay_timer, f32 time, bool forwards) {
    struct transition_fader_state* state = &global_transition_fader_state;
    state->type                          = type;
    state->color                         = color;
    state->max_time                      = time;
    state->time                          = 0;
    state->delay_time                    = delay_timer;
    state->forwards                      = forwards;

    transition_register_on_delay_finish(___transition_stubs, NULL, 0);
    transition_register_on_finish(___transition_stubs, NULL, 0);
    transition_register_on_start(___transition_stubs, NULL, 0);
}

local void do_horizontal_slide_in(union color32f32 target_color, f32 delay_timer, f32 time) {
    struct transition_fader_state* state = &global_transition_fader_state;
    fader_generic_setup(
        TRANSITION_FADER_TYPE_HORIZONTAL_SLIDE,
        target_color,
        delay_timer,
        time,
        true
    );
    _debugprintf("Starting a horizontal slide fade in!");
}

local void do_horizontal_slide_out(union color32f32 target_color, f32 delay_timer, f32 time) {
    struct transition_fader_state* state = &global_transition_fader_state;
    fader_generic_setup(
        TRANSITION_FADER_TYPE_HORIZONTAL_SLIDE,
        target_color,
        delay_timer,
        time,
        false
    );
    _debugprintf("Starting a horizontal slide fade out!");
}

local void do_vertical_slide_in(union color32f32 target_color, f32 delay_timer, f32 time) {
    struct transition_fader_state* state = &global_transition_fader_state;
    fader_generic_setup(
        TRANSITION_FADER_TYPE_VERTICAL_SLIDE,
        target_color,
        delay_timer,
        time,
        true
    );
    _debugprintf("Starting a vertical slide fade in!");
}

local void do_vertical_slide_out(union color32f32 target_color, f32 delay_timer, f32 time) {
    struct transition_fader_state* state = &global_transition_fader_state;
    fader_generic_setup(
        TRANSITION_FADER_TYPE_VERTICAL_SLIDE,
        target_color,
        delay_timer,
        time,
        false
    );
    _debugprintf("Starting a vertical slide fade out!");
}

local void do_curtainclose_in(union color32f32 target_color, f32 delay_timer, f32 time) {
    struct transition_fader_state* state = &global_transition_fader_state;
    fader_generic_setup(
        TRANSITION_FADER_TYPE_CURTAINCLOSE_SLIDE,
        target_color,
        delay_timer,
        time,
        true
    );
    _debugprintf("Starting a curtain close slide fade in!");
}

local void do_curtainclose_out(union color32f32 target_color, f32 delay_timer, f32 time) {
    struct transition_fader_state* state = &global_transition_fader_state;
    fader_generic_setup(
        TRANSITION_FADER_TYPE_CURTAINCLOSE_SLIDE,
        target_color,
        delay_timer,
        time,
        false
    );
    _debugprintf("Starting a curtain close slide fade out!");
}

local void do_shuteye_in(union color32f32 target_color, f32 delay_timer, f32 time) {
    struct transition_fader_state* state = &global_transition_fader_state;
    fader_generic_setup(
        TRANSITION_FADER_TYPE_SHUTEYE_SLIDE,
        target_color,
        delay_timer,
        time,
        true
    );
    _debugprintf("Starting a shuteye slide fade in!");
}

local void do_shuteye_out(union color32f32 target_color, f32 delay_timer, f32 time) {
    struct transition_fader_state* state = &global_transition_fader_state;
    fader_generic_setup(
        TRANSITION_FADER_TYPE_SHUTEYE_SLIDE,
        target_color,
        delay_timer,
        time,
        false
    );
    _debugprintf("Starting a shuteye slide fade out!");
}

local void do_color_transition_in(union color32f32 target_color, f32 delay_time, f32 time) {
    struct transition_fader_state* state = &global_transition_fader_state;
    fader_generic_setup(
        TRANSITION_FADER_TYPE_COLOR,
        target_color,
        delay_time,
        time,
        true
    );

    _debugprintf("Starting a color fade in!");
}

local void do_color_transition_out(union color32f32 target_color, f32 delay_time, f32 time) {
    struct transition_fader_state* state = &global_transition_fader_state;
    fader_generic_setup(
        TRANSITION_FADER_TYPE_COLOR,
        target_color,
        delay_time,
        time,
        false
    );

    _debugprintf("Starting a color fade out!");
}

local void update_and_render_color_fades(struct game_state* state, struct transition_fader_state* fader_state, struct software_framebuffer* framebuffer, f32 effective_t);
local void update_and_render_horizontal_slide_fades(struct game_state* state, struct transition_fader_state* fader_state, struct software_framebuffer* framebuffer, f32 effective_t);
local void update_and_render_vertical_slide_fades(struct game_state* state, struct transition_fader_state* fader_state, struct software_framebuffer* framebuffer, f32 effective_t);
local void update_and_render_curtainclose_fades(struct game_state* state, struct transition_fader_state* fader_state, struct software_framebuffer* framebuffer, f32 effective_t);
local void update_and_render_shuteye_fades(struct game_state* state, struct transition_fader_state* fader_state, struct software_framebuffer* framebuffer, f32 effective_t);

local void transitions_update_and_render(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    struct transition_fader_state* transition_state = &global_transition_fader_state;

    if (transition_state->type == TRANSITION_FADER_TYPE_NONE) {
        return;
    }

    if (transition_state->time == 0.0f) {
        transition_state->on_start(transition_state->callback_data_on_start);
    }

    f32 effective_t = clamp_f32((transition_state->time / transition_state->max_time), 0, 1);

    if (transition_state->delay_time > 0) {
        transition_state->delay_time -= dt;

        if (transition_state->delay_time <= 0) {
            transition_state->on_delay_finish(transition_state->callback_data_on_delay_finish);
        }
    } else {
        if (transition_state->time < transition_state->max_time) {
            transition_state->time += dt;

            if (transition_state->time >= transition_state->max_time) {
                last_global_transition_fader_state = global_transition_fader_state;
                transition_state->on_finish(transition_state->callback_data_on_finish);
            }
        } else {
            /* ? */
        }
    }

    switch (transition_state->type) {
        case TRANSITION_FADER_TYPE_COLOR: {
            update_and_render_color_fades(state, transition_state, framebuffer, effective_t);
        } break;
        case TRANSITION_FADER_TYPE_HORIZONTAL_SLIDE: {
            update_and_render_horizontal_slide_fades(state, transition_state, framebuffer, effective_t);
        } break;
        case TRANSITION_FADER_TYPE_VERTICAL_SLIDE: {
            update_and_render_vertical_slide_fades(state, transition_state, framebuffer, effective_t);
        } break;
        case TRANSITION_FADER_TYPE_SHUTEYE_SLIDE: {
            update_and_render_shuteye_fades(state, transition_state, framebuffer, effective_t);
        } break;
        case TRANSITION_FADER_TYPE_CURTAINCLOSE_SLIDE: {
            update_and_render_curtainclose_fades(state, transition_state, framebuffer, effective_t);
        } break;

            bad_case;
    }

}

local void update_and_render_color_fades(struct game_state* state, struct transition_fader_state* fader_state, struct software_framebuffer* framebuffer, f32 effective_t) {
    if (fader_state->forwards) {
        effective_t = 1 - effective_t;
    }

    union color32f32 render_color = fader_state->color;
    render_color.a                = lerp_f32(fader_state->color.a, 0, effective_t);

    software_framebuffer_draw_quad(framebuffer, rectangle_f32(0, 0, framebuffer->width, framebuffer->height), color32f32_to_color32u8(render_color), BLEND_MODE_ALPHA);
}

local void update_and_render_horizontal_slide_fades(struct game_state* state, struct transition_fader_state* fader_state, struct software_framebuffer* framebuffer, f32 effective_t) {
    if (!fader_state->forwards) {
        effective_t = 1 - effective_t;
    }

    union color32f32 render_color = fader_state->color;
    f32 position_to_draw          = lerp_f32(-(s32)framebuffer->width, 0, effective_t);

    software_framebuffer_draw_quad(framebuffer,
                                   rectangle_f32(position_to_draw, 0,
                                                 framebuffer->width, framebuffer->height),
                                   color32f32_to_color32u8(render_color), BLEND_MODE_ALPHA);
}

local void update_and_render_vertical_slide_fades(struct game_state* state, struct transition_fader_state* fader_state, struct software_framebuffer* framebuffer, f32 effective_t) {
    if (!fader_state->forwards) {
        effective_t = 1 - effective_t;
    }

    union color32f32 render_color = fader_state->color;
    f32 position_to_draw          = lerp_f32(-(s32)framebuffer->height, 0, effective_t);

    software_framebuffer_draw_quad(framebuffer, rectangle_f32(0, position_to_draw, framebuffer->width, framebuffer->height), color32f32_to_color32u8(render_color), BLEND_MODE_ALPHA);
}

local void update_and_render_curtainclose_fades(struct game_state* state, struct transition_fader_state* fader_state, struct software_framebuffer* framebuffer, f32 effective_t) {
    if (!fader_state->forwards) {
        effective_t = 1 - effective_t;
    }

    union color32f32 render_color = fader_state->color;
    s32              half_width  = framebuffer->width/2;

    f32 lid_positions[] = {
        lerp_f32(-(s32)framebuffer->width,                     0, effective_t),
        lerp_f32((s32)framebuffer->width+half_width,  half_width, effective_t),
    };

    software_framebuffer_draw_quad(framebuffer, rectangle_f32(lid_positions[0], 0, half_width, framebuffer->height), color32f32_to_color32u8(render_color), BLEND_MODE_ALPHA);
    software_framebuffer_draw_quad(framebuffer, rectangle_f32(lid_positions[1], 0, half_width, framebuffer->height), color32f32_to_color32u8(render_color), BLEND_MODE_ALPHA);
}

local void update_and_render_shuteye_fades(struct game_state* state, struct transition_fader_state* fader_state, struct software_framebuffer* framebuffer, f32 effective_t) {
    if (!fader_state->forwards) {
        effective_t = 1 - effective_t;
    }

    union color32f32 render_color = fader_state->color;
    s32              half_height  = framebuffer->height/2;

    f32 lid_positions[] = {
        lerp_f32(-(s32)framebuffer->height,                        0, effective_t),
        lerp_f32((s32)framebuffer->height+half_height,   half_height, effective_t),
    };

    software_framebuffer_draw_quad(framebuffer, rectangle_f32(0, lid_positions[0], framebuffer->width, half_height), color32f32_to_color32u8(render_color), BLEND_MODE_ALPHA);
    software_framebuffer_draw_quad(framebuffer, rectangle_f32(0, lid_positions[1], framebuffer->width, half_height), color32f32_to_color32u8(render_color), BLEND_MODE_ALPHA);
}
