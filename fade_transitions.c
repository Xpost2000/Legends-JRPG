enum transition_fader_type {
    TRANSITION_FADER_TYPE_NONE,
    TRANSITION_FADER_TYPE_COLOR,
    TRANSITION_FADER_TYPE_COUNT,
};

struct transition_fader_state {
    u32              type;

    union color32f32 color;
    f32              max_time;
    f32              delay_time;
    f32              time;
    bool             forwards;
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

local void do_color_transition_in(union color32f32 target_color, f32 delay_time, f32 time) {
    struct transition_fader_state* state = &global_transition_fader_state;
    state->type                          = TRANSITION_FADER_TYPE_COLOR;
    state->color                         = target_color;
    state->max_time                      = time;
    state->time                          = 0;
    state->delay_time                    = delay_time;
    state->forwards                      = true;
}

local void do_color_transition_out(union color32f32 target_color, f32 delay_time, f32 time) {
    struct transition_fader_state* state = &global_transition_fader_state;
    state->type                          = TRANSITION_FADER_TYPE_COLOR;
    state->color                         = target_color;
    state->max_time                      = time;
    state->time                          = 0;
    state->delay_time                    = delay_time;
    state->forwards                      = false;
}

local void transitions_update_and_render(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    
}
