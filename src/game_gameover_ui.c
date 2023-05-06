enum game_over_ui_phase {
    GAME_OVER_UI_PHASE_POP_IN_TEXT,
    GAME_OVER_UI_PHASE_SHOW_OPTIONS,
    GAME_OVER_UI_PHASE_IDLE,
    GAME_OVER_UI_PHASE_BYE,
};

#define GAME_OVER_TEXT_DELAY (0.1)
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

    f32 timer;

    bool selectable_options[array_count(game_over_options)];
} global_game_over_ui_state;

local void game_over_ui_setup(void) {
    global_game_over_ui_state.phase                     = GAME_OVER_UI_PHASE_POP_IN_TEXT;
    global_game_over_ui_state.character_show_timer      = 0;
    global_game_over_ui_state.currently_selected_option = 0;
    global_game_over_ui_state.characters_shown_in_text  = 0;
    global_game_over_ui_state.timer                     = 0;

    global_game_over_ui_state.selectable_options[0] = true;
    global_game_over_ui_state.selectable_options[1] = false;
    global_game_over_ui_state.selectable_options[2] = true;
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

local void do_game_over_options(struct software_framebuffer* framebuffer, f32 dt, f32 t, bool allow_input) {
    struct font_cache* font              = game_get_font(MENU_FONT_COLOR_STEEL);
    struct font_cache* font_selected     = game_get_font(MENU_FONT_COLOR_GOLD);
    const f32          FONT_SCALE        = 3;
    f32                font_height       = font_cache_text_height(font) * (FONT_SCALE+0.1);
    const s32          SEED_DISPLACEMENT = 123;

    s32 selected_option = -1;

    f32 y_cursor = SCREEN_HEIGHT * 0.65;

    if (allow_input) {
        if (is_action_down_with_repeat(INPUT_ACTION_MOVE_UP)) {
            global_game_over_ui_state.currently_selected_option--;

            if (global_game_over_ui_state.currently_selected_option < 0) {
                global_game_over_ui_state.currently_selected_option = array_count(game_over_options)-1;
            }
        } else if (is_action_down_with_repeat(INPUT_ACTION_MOVE_DOWN)) {
            global_game_over_ui_state.currently_selected_option++;

            if (global_game_over_ui_state.currently_selected_option >= array_count(game_over_options)) {
                global_game_over_ui_state.currently_selected_option = 0;
            }
        }
    }

    for (unsigned option_index = 0; option_index < array_count(game_over_options); ++option_index) {
        struct font_cache* draw_font = font;

        if (option_index == global_game_over_ui_state.currently_selected_option) {
            draw_font = font_selected;
        }

        f32 alpha = 1;
        if (!global_game_over_ui_state.selectable_options[option_index]) {
            alpha = 0.5;
        }

        draw_ui_breathing_text_centered(framebuffer,
                                        rectangle_f32(0, y_cursor, SCREEN_WIDTH, SCREEN_HEIGHT*0.1), draw_font,
                                        FONT_SCALE, game_over_options[option_index], SEED_DISPLACEMENT, color32f32(1, 1, 1, t*alpha));

        if (global_game_over_ui_state.selectable_options[option_index]) {
            if (allow_input) {
                if (is_action_pressed(INPUT_ACTION_CONFIRMATION)) {
                    selected_option = option_index;
                }
            }
        }

        y_cursor += font_height;
    }

    /* TODO: LOAD SAVE GAME, Not done yet! Need to move out save menu! */
    switch (selected_option) {
        case 0: {/* TODO: Checkpoint */
            /* Who knows? */
            set_game_screen_mode(GAME_SCREEN_INGAME);
            fade_into_game();
            return ;
        } break;
        case 1: { /* TODO: Load from old save */
            return ;
        } break;
        case 2: { /* Return to main menu */
            global_game_over_ui_state.phase = GAME_OVER_UI_PHASE_BYE;
            global_game_over_ui_state.timer = 0;
            return ;
        } break;
        default: {
            return;
        } break;
    }

    global_game_over_ui_state.timer += dt;
}

local string gameover_string_sliced_properly(void) {
    s32 word_remainder = 0;
    while (global_game_over_ui_state.characters_shown_in_text+word_remainder < game_over_text.length) {
        if (is_whitespace(game_over_text.data[global_game_over_ui_state.characters_shown_in_text+word_remainder])) {
            break;
        }
        word_remainder++;
    }

    string result = string_slice(game_over_text, 0, global_game_over_ui_state.characters_shown_in_text+word_remainder);
    return result;
}

local void update_and_render_gameover_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    struct font_cache* font                     = game_get_font(MENU_FONT_COLOR_WHITE);
    const f32          FONT_SCALE               = 4;
    const s32          SEED_DISPLACEMENT        = 54;

    /* WARP_BOUNDS_W need to be adjusted based off of aspect ratio */
    f32          WRAP_BOUNDS_W            = SCREEN_WIDTH * 0.8;

    if (!is_4by3()) {
        WRAP_BOUNDS_W = SCREEN_WIDTH * 0.6;
    }

    const f32          BOUNDS_H                 = SCREEN_HEIGHT*0.35;
    const f32          MAX_FADE_IN_OPTIONS_TIME = 1.3f;

    switch (global_game_over_ui_state.phase) {
        case GAME_OVER_UI_PHASE_POP_IN_TEXT: {
            bool text_status = game_over_ui_state_advance_text_length(dt);

            draw_ui_breathing_text_word_wrapped_centered1(
                framebuffer,
                rectangle_f32(0, 100, SCREEN_WIDTH, BOUNDS_H),
                WRAP_BOUNDS_W,
                font, FONT_SCALE,
                gameover_string_sliced_properly(),
                string_slice(game_over_text, 0, global_game_over_ui_state.characters_shown_in_text),
                SEED_DISPLACEMENT, color32f32_WHITE);

            if (!text_status) {
                global_game_over_ui_state.phase = GAME_OVER_UI_PHASE_SHOW_OPTIONS;
                global_game_over_ui_state.timer = 0;
            }
        } break;

        case GAME_OVER_UI_PHASE_SHOW_OPTIONS: {
            f32 effective_t = global_game_over_ui_state.timer / MAX_FADE_IN_OPTIONS_TIME;
            game_over_ui_state_advance_text_length(dt);
            draw_ui_breathing_text_word_wrapped_centered1(
                framebuffer,
                rectangle_f32(0, 100, SCREEN_WIDTH, BOUNDS_H),
                WRAP_BOUNDS_W,
                font, FONT_SCALE,
                gameover_string_sliced_properly(),
                string_slice(game_over_text, 0, global_game_over_ui_state.characters_shown_in_text),
                SEED_DISPLACEMENT, color32f32_WHITE);
            if (effective_t > 1) {
                effective_t = 1;
            } else if (effective_t < 0) {
                effective_t = 0;
            }


            do_game_over_options(framebuffer, dt, effective_t, false);
            global_game_over_ui_state.timer += dt;

            if (effective_t >= 1) {
                global_game_over_ui_state.phase = GAME_OVER_UI_PHASE_IDLE;
                global_game_over_ui_state.timer = 0;
            }
        } break;

        case GAME_OVER_UI_PHASE_IDLE: {
            game_over_ui_state_advance_text_length(dt);
            draw_ui_breathing_text_word_wrapped_centered1(
                framebuffer,
                rectangle_f32(0, 100, SCREEN_WIDTH, BOUNDS_H),
                WRAP_BOUNDS_W,
                font, FONT_SCALE,
                gameover_string_sliced_properly(),
                string_slice(game_over_text, 0, global_game_over_ui_state.characters_shown_in_text),
                SEED_DISPLACEMENT, color32f32_WHITE);
            do_game_over_options(framebuffer, dt, 1, true);

        } break;
        case GAME_OVER_UI_PHASE_BYE: {
            f32 effective_t = global_game_over_ui_state.timer / MAX_FADE_IN_OPTIONS_TIME;

            if (effective_t > 1) {
                effective_t = 1;
            } else if (effective_t < 0) {
                effective_t = 0;
            }

            draw_ui_breathing_text_word_wrapped_centered1(
                framebuffer,
                rectangle_f32(0, 100, SCREEN_WIDTH, BOUNDS_H),
                WRAP_BOUNDS_W,
                font, FONT_SCALE,
                gameover_string_sliced_properly(),
                string_slice(game_over_text, 0, global_game_over_ui_state.characters_shown_in_text),
                SEED_DISPLACEMENT, color32f32(1, 1, 1, 1 - effective_t));
            do_game_over_options(framebuffer, dt, 1 - effective_t, false);
            global_game_over_ui_state.timer += dt;

            if (effective_t >= 1.0) {
                initialize_main_menu();
                set_game_screen_mode(GAME_SCREEN_MAIN_MENU);
            }
        } break;
    }

    if (any_key_down() || controller_any_button_down(get_gamepad(0))) {
        if (global_game_over_ui_state.phase < GAME_OVER_UI_PHASE_SHOW_OPTIONS) {
            global_game_over_ui_state.phase = GAME_OVER_UI_PHASE_SHOW_OPTIONS;
            global_game_over_ui_state.timer = 0;
        }
    }
}
