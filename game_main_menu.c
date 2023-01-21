/* global main menu code */
/*
  NOTE: 1/20/23,
  the main menu is being redesigned to be more in-line with a JRPG standardized UI format.

  As of now the pause menu is the only menu where this is not true (although I don't dislike the pause menu's design, so it's not changing.)

  I would like to have background details but I probably don't have the chance for it.

  I am still a little hurt at my refusal to write a proper UI library but thankfully these UIs aren't super complicated to write.
*/

enum main_menu_animation_phase {
    MAIN_MENU_FADE_IN_TITLE,
    MAIN_MENU_SLIDE_TITLE_UP,
    MAIN_MENU_OPEN_OPTIONS_BOX,
    MAIN_MENU_IDLE,

    MAIN_MENU_OPTIONS_MENU,
    MAIN_MENU_SAVE_MENU,
};

/* All option menu code is also placed here. Along with the special *IMGUI* code lol */
struct {
    s32 phase;
    f32 timer;
    s32 currently_selected_option_choice;
} main_menu;

enum main_menu_option_choices {
    MAIN_MENU_OPTION_CONTINUE,
    MAIN_MENU_OPTION_NEW_GAME,
    MAIN_MENU_OPTION_LOAD_GAME,
    MAIN_MENU_OPTION_OPTIONS,
    MAIN_MENU_OPTION_CREDITS,
    MAIN_MENU_OPTION_QUIT,
    MAIN_MENU_OPTION_COUNT,
};

local bool main_menu_option_choice_usability[MAIN_MENU_OPTION_COUNT] = {};

local string main_menu_options[] = {
    [MAIN_MENU_OPTION_CONTINUE]  = string_literal("Continue"),
    [MAIN_MENU_OPTION_NEW_GAME]  = string_literal("New Game"),
    [MAIN_MENU_OPTION_LOAD_GAME] = string_literal("Load Game"),
    [MAIN_MENU_OPTION_OPTIONS]   = string_literal("Options"),
    [MAIN_MENU_OPTION_CREDITS]   = string_literal("Credits"),
    [MAIN_MENU_OPTION_QUIT]      = string_literal("Quit"),
};

local void initialize_main_menu(void) {
    main_menu.timer                            = 0;
    main_menu.phase                            = MAIN_MENU_FADE_IN_TITLE;
    main_menu.currently_selected_option_choice = 0;
    return;
}

local void _main_menu_draw_title(struct software_framebuffer* framebuffer, f32 y) {
    struct font_cache* font  = game_get_font(MENU_FONT_COLOR_GOLD);
}

local void update_and_render_main_menu(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    const f32 TITLE_FONT_SCALE  = 6.0;
    const f32 NORMAL_FONT_SCALE = 4.0;

    struct font_cache* font1 = game_get_font(MENU_FONT_COLOR_STEEL);

    software_framebuffer_clear_buffer(framebuffer, color32u8(0,0,0,255));

    f32 font_height = font_cache_text_height(font1);
    switch (main_menu.phase) {
        case MAIN_MENU_FADE_IN_TITLE: {
            
        } break;
        case MAIN_MENU_SLIDE_TITLE_UP: {
            
        } break;

        case MAIN_MENU_IDLE: {
        } break;

        case MAIN_MENU_OPTIONS_MENU: {
            /* TODO */
            unimplemented("New options menu not done");
        } break;

        case MAIN_MENU_SAVE_MENU: {
            s32 save_menu_result = do_save_menu(framebuffer, dt);
            switch (save_menu_result) {
                case SAVE_MENU_PROCESS_ID_EXIT: {
                    main_menu.phase                            = MAIN_MENU_TITLE_APPEAR;
                    main_menu.timer                            = 0;
                    main_menu.currently_selected_option_choice = 0;
                } break;
                case SAVE_MENU_PROCESS_ID_LOADED_EXIT: {
                    
                } break;
                case SAVE_MENU_PROCESS_ID_SAVED_EXIT: {
                } break;
            }
        } break;
    } 

    if (any_key_down() || controller_any_button_down(get_gamepad(0))) {
        if (main_menu.phase < MAIN_MENU_IDLE) {
            main_menu.phase = MAIN_MENU_IDLE;
            return;
        }
    }
}
