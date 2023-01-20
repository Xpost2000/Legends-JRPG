/*
  NOTE:

  The credits should include a mode7 preview of the world (and various other screen caps of the world),
  but it's okay...
*/
enum credits_state_phases {
    CREDITS_PHASE_FADE_IN_TITLE,
    CREDITS_PHASE_SLIDE_TITLE_UP,

    CREDITS_PHASE_MAIN_ANIMATION,

    CREDITS_PHASE_SLIDE_TITLE_DOWN,
    CREDITS_PHASE_FADE_OUT_TITLE,
    CREDITS_PHASE_SEE_YOU_NEXT_TIME,
};
struct credits_state {
    s32 phase;
    f32 timer;

    /*
      NOTE:
      This is for the debug mode so I can time how long a music track should be for the
      credits
    */
#ifndef RELEASE
    f32 total_elapsed_time;
#endif
};

local struct credits_state credits_state = {};

void setup_credits_mode(void) {
    zero_memory();
}

void update_and_render_credits_screen(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
}
