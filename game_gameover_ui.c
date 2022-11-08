enum game_over_ui_phase {
    GAME_OVER_UI_PHASE_POP_IN_TEXT,
    GAME_OVER_UI_PHASE_SHOW_OPTIONS,
    GAME_OVER_UI_PHASE_IDLE,
    GAME_OVER_UI_PHASE_BYE,
};

#define GAME_OVER_TEXT_DELAY (0.212)
local string game_over_text = string_literal("And so the tale ends...");
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

local void game_over_ui_finish(void) {
    global_game_initiated_death_ui = false;
}

local void game_over_ui_state_advance_text_length(f32 dt) {
    global_game_over_ui_state.character_show_timer += dt;

    if (global_game_over_ui_state.character_show_timer >= GAME_OVER_TEXT_DELAY) {
        global_game_over_ui_state.characters_shown_in_text += 1;
        global_game_over_ui_state.character_show_timer      = 0;
    }
}

local void update_and_render_gameover_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    switch (global_game_over_ui_state.phase) {
        case GAME_OVER_UI_PHASE_POP_IN_TEXT: {
            game_over_ui_state_advance_text_length(dt);
        } break;

        case GAME_OVER_UI_PHASE_SHOW_OPTIONS: {
            game_over_ui_state_advance_text_length(dt);
        } break;

        case GAME_OVER_UI_PHASE_IDLE: {
            
        } break;
        case GAME_OVER_UI_PHASE_BYE: {
            
        } break;
    }
}
