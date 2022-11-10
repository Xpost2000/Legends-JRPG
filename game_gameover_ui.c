enum game_over_ui_phase {
    GAME_OVER_UI_PHASE_POP_IN_TEXT,
    GAME_OVER_UI_PHASE_SHOW_OPTIONS,
    GAME_OVER_UI_PHASE_IDLE,
    GAME_OVER_UI_PHASE_BYE,
};

#define GAME_OVER_TEXT_DELAY (0.100)
local string game_over_text = string_literal("And so the tale ends... Never to be concluded...");
local string game_over_options[] = {
    /*
      Only per game session! Will make relevant game state data copies to work with,
      but within fixed footprint of maybe 4 MB.
    */
    string_literal("CHECKPOINT"),          
    string_literal("LOAD SAVE"),            /**/
    string_literal("RETURN TO MAIN MENU"),
};

struct game_over_ui_state {
    s32 phase;

    s32 characters_shown_in_text;
    s32 currently_selected_option;
    f32 character_show_timer;

    bool selectable_options[array_count(game_over_options)];
} global_game_over_ui_state;

local void game_over_ui_setup(void) {
    global_game_over_ui_state.phase                     = GAME_OVER_UI_PHASE_POP_IN_TEXT;
    global_game_over_ui_state.character_show_timer      = 0;
    global_game_over_ui_state.currently_selected_option = 0;
    global_game_over_ui_state.characters_shown_in_text  = 0;
}

local void game_over_ui_finish(void) {
    global_game_initiated_death_ui = false;
}

local bool game_over_ui_state_advance_text_length(f32 dt) {
    global_game_over_ui_state.character_show_timer += dt;

    if (global_game_over_ui_state.character_show_timer >= GAME_OVER_TEXT_DELAY) {
        global_game_over_ui_state.characters_shown_in_text += 1;
        global_game_over_ui_state.character_show_timer      = 0;

        if (global_game_over_ui_state.characters_shown_in_text >= game_over_text.length) {
            global_game_over_ui_state.characters_shown_in_text = game_over_text.length;
            return false;
        }
    }

    return true;
}

local void update_and_render_gameover_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    struct font_cache* font              = game_get_font(MENU_FONT_COLOR_WHITE);
    const f32          FONT_SCALE        = 4;
    s32                SEED_DISPLACEMENT = 54;
    f32                WRAP_BOUNDS_W     = SCREEN_WIDTH * 0.8;
    f32                BOUNDS_H          = SCREEN_HEIGHT*0.35;

    switch (global_game_over_ui_state.phase) {
        case GAME_OVER_UI_PHASE_POP_IN_TEXT: {
            game_over_ui_state_advance_text_length(dt);
            draw_ui_breathing_text_word_wrapped_centered(
                framebuffer,
                rectangle_f32(0, 100, SCREEN_WIDTH, BOUNDS_H),
                WRAP_BOUNDS_W,
                font, FONT_SCALE,
                string_slice(game_over_text, 0, global_game_over_ui_state.characters_shown_in_text),
                SEED_DISPLACEMENT, color32f32_WHITE);
        } break;

        case GAME_OVER_UI_PHASE_SHOW_OPTIONS: {
            game_over_ui_state_advance_text_length(dt);
            draw_ui_breathing_text_word_wrapped_centered(
                framebuffer,
                rectangle_f32(0, 100, SCREEN_WIDTH, BOUNDS_H),
                WRAP_BOUNDS_W,
                font, FONT_SCALE,
                string_slice(game_over_text, 0, global_game_over_ui_state.characters_shown_in_text),
                SEED_DISPLACEMENT, color32f32_WHITE);
            /* do_game_over_options(false); */
        } break;

        case GAME_OVER_UI_PHASE_IDLE: {
            /* do_game_over_options(true); */
        } break;
        case GAME_OVER_UI_PHASE_BYE: {
            /* do_game_over_options(false); */
        } break;
    }
}
