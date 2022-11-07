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

    bool selectable_options[array_count(game_over_options)];
} global_game_over_ui_state;

local void update_and_render_gameover_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    switch (global_game_over_ui_state.phase) {
        case GAME_OVER_UI_PHASE_POP_IN_TEXT: {
            
        } break;
        case GAME_OVER_UI_PHASE_SHOW_OPTIONS: {
            
        } break;
        case GAME_OVER_UI_PHASE_IDLE: {
            
        } break;
        case GAME_OVER_UI_PHASE_BYE: {
            
        } break;
    }
}
