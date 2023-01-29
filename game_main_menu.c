/* global main menu code */
/*
  NOTE: I would like to animate some stuff again in this menu but that's enough work for now lol.
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
    MAIN_MENU_OPTION_NONE,
};

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

/* NOTE: centered */
local void _main_menu_draw_title(struct software_framebuffer* framebuffer, f32 y, f32 alpha) {
#ifdef EXPERIMENTAL_320
    const f32          TITLE_FONT_SCALE = 2.0;
#else
    const f32          TITLE_FONT_SCALE = 4.0;
#endif
    struct font_cache* font             = game_get_font(MENU_FONT_COLOR_GOLD);
    string             text             = game_title;

    f32                text_width       = font_cache_text_width(font, text, TITLE_FONT_SCALE);
    f32                text_height      = font_cache_text_height(font) * TITLE_FONT_SCALE;

    software_framebuffer_draw_text(framebuffer, font, TITLE_FONT_SCALE, v2f32(SCREEN_WIDTH/2-text_width/2, y-text_height/2), text, color32f32(1,1,1,alpha), BLEND_MODE_ALPHA);
}

local s32 _do_main_menu_options_menu(struct software_framebuffer* framebuffer, f32 open_t, bool use_options) {
    struct font_cache* font1              = game_get_font(MENU_FONT_COLOR_STEEL);
    struct font_cache* font2              = game_get_font(MENU_FONT_COLOR_GOLD);
#ifdef EXPERIMENTAL_320
    const f32          NORMAL_FONT_SCALE  = 2.0;
    const s32          OPTIONS_BOX_WIDTH  = 6;
    const s32          OPTIONS_BOX_HEIGHT = 6;
#else
    const f32          NORMAL_FONT_SCALE  = 4.0;
    const s32          OPTIONS_BOX_WIDTH  = 12;
    const s32          OPTIONS_BOX_HEIGHT = 12;
#endif

    f32   current_box_width          = quadratic_ease_in_f32(0, OPTIONS_BOX_WIDTH,  open_t);
    f32   current_box_height         = quadratic_ease_in_f32(0, OPTIONS_BOX_HEIGHT, open_t);
    v2f32 options_box_extents        = nine_patch_estimate_extents(ui_chunky, 1, current_box_width, current_box_height);
    v2f32 options_box_start_position = v2f32((SCREEN_WIDTH/2 - options_box_extents.x/2), (SCREEN_HEIGHT * 0.5) - options_box_extents.y/2);
    draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, options_box_start_position, current_box_width, current_box_height, UI_DEFAULT_COLOR);

    if (!use_options) {
        return MAIN_MENU_OPTION_NONE;
    }

    struct common_ui_layout layout = common_ui_vertical_layout(options_box_start_position.x + 15, options_box_start_position.y + 15);
    bool main_menu_option_choice_disabled[MAIN_MENU_OPTION_COUNT] = {};
    {
        main_menu_option_choice_disabled[MAIN_MENU_OPTION_CONTINUE] = true;

        /* reset cursor if it's starting in a bad place */
        {
            while (main_menu_option_choice_disabled[main_menu.currently_selected_option_choice]) {
                main_menu.currently_selected_option_choice++;
            }
        }
    }

    { 
        bool selection_down = is_action_down_with_repeat(INPUT_ACTION_MOVE_DOWN);
        bool selection_up   = is_action_down_with_repeat(INPUT_ACTION_MOVE_UP);

        if (selection_down) {
            do {
                main_menu.currently_selected_option_choice++;
                if (main_menu.currently_selected_option_choice >= array_count(main_menu_options)-1) {
                    main_menu.currently_selected_option_choice = 0;
                }
            } while(main_menu_option_choice_disabled[main_menu.currently_selected_option_choice]);
        } else if (selection_up) {
            do {
                main_menu.currently_selected_option_choice--;
                if (main_menu.currently_selected_option_choice < 0) {
                    main_menu.currently_selected_option_choice = array_count(main_menu_options)-1;
                }
            } while(main_menu_option_choice_disabled[main_menu.currently_selected_option_choice]);
        }
    }

    for (s32 option_index = 0; option_index < array_count(main_menu_options); ++option_index) {
        u32 flags = COMMON_UI_BUTTON_FLAGS_NONE;
        if (main_menu_option_choice_disabled[option_index]) {
            flags |= COMMON_UI_BUTTON_FLAGS_DISABLED;
        }
        
#ifdef EXPERIMENTAL_320
        bool button_result = common_ui_button(&layout, framebuffer, main_menu_options[option_index], 1, option_index, &main_menu.currently_selected_option_choice, flags);
#else
        bool button_result = common_ui_button(&layout, framebuffer, main_menu_options[option_index], 2, option_index, &main_menu.currently_selected_option_choice, flags);
#endif

        if (button_result) {
            return option_index;
        }
    }

    return MAIN_MENU_OPTION_NONE;
}

local void update_and_render_main_menu(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    struct font_cache* font1 = game_get_font(MENU_FONT_COLOR_STEEL);

    software_framebuffer_clear_buffer(framebuffer, color32u8(0,0,0,255));
    const f32 FINAL_MAIN_MENU_TITLE_Y = 50;

    switch (main_menu.phase) {
        case MAIN_MENU_FADE_IN_TITLE: {
            f32 MAX_PHASE_T = 1.5f;
            f32 alpha       = clamp_f32(main_menu.timer/MAX_PHASE_T, 0, 1);

            _main_menu_draw_title(framebuffer, SCREEN_HEIGHT/2, alpha);

            if (main_menu.timer >= MAX_PHASE_T) {
                main_menu.phase = MAIN_MENU_SLIDE_TITLE_UP;
                main_menu.timer = 0;
                return;
            }
        } break;

        case MAIN_MENU_SLIDE_TITLE_UP: {
            f32 MAX_PHASE_T = 0.5f;
            f32 alpha       = clamp_f32(main_menu.timer/MAX_PHASE_T, 0, 1);
            f32 y           = lerp_f32(SCREEN_HEIGHT/2, FINAL_MAIN_MENU_TITLE_Y, alpha);

            _main_menu_draw_title(framebuffer, y, 1.0);

            if (main_menu.timer >= MAX_PHASE_T) {
                main_menu.phase = MAIN_MENU_OPEN_OPTIONS_BOX;
                main_menu.timer = 0;
                return;
            }
        } break;

        case MAIN_MENU_OPEN_OPTIONS_BOX: {
            _main_menu_draw_title(framebuffer, FINAL_MAIN_MENU_TITLE_Y, 1.0);

            f32 MAX_PHASE_T = 0.45f;
            f32 effective_t = clamp_f32(main_menu.timer/MAX_PHASE_T, 0, 1);

            _do_main_menu_options_menu(framebuffer, effective_t, false);

            if (main_menu.timer >= MAX_PHASE_T) {
                main_menu.phase = MAIN_MENU_IDLE;
                return;
            }
        } break;

        case MAIN_MENU_IDLE: {
            _main_menu_draw_title(framebuffer, FINAL_MAIN_MENU_TITLE_Y, 1.0);
           s32 option_result =  _do_main_menu_options_menu(framebuffer, 1, true);

           switch (option_result) {
               case MAIN_MENU_OPTION_CONTINUE: {
               } break;
               case MAIN_MENU_OPTION_NEW_GAME: {
                   set_game_screen_mode(GAME_SCREEN_INGAME);
                   fade_into_game();
               } break;
               case MAIN_MENU_OPTION_LOAD_GAME: {
                   save_menu_open_for_loading();
                   main_menu.phase = MAIN_MENU_SAVE_MENU;
               } break;
               case MAIN_MENU_OPTION_OPTIONS: {
                   options_menu_open();
                   main_menu.phase = MAIN_MENU_OPTIONS_MENU;
               } break;
               case MAIN_MENU_OPTION_CREDITS: {
                   enter_credits();
               } break;
               case MAIN_MENU_OPTION_QUIT: {
                   global_game_running = false;
               } break;
           }
        } break;

        case MAIN_MENU_OPTIONS_MENU: {
            /* TODO */
            s32 options_menu_result = do_options_menu(framebuffer, dt);

            switch (options_menu_result) {
                case OPTIONS_MENU_PROCESS_ID_EXIT: {
                    main_menu.phase = MAIN_MENU_IDLE;
                    main_menu.timer = 0;
                    main_menu.currently_selected_option_choice = 0;
                } break;

                default: {
                    
                } break;
            }
        } break;

        case MAIN_MENU_SAVE_MENU: {
            s32 save_menu_result = do_save_menu(framebuffer, dt);

            switch (save_menu_result) {
                case SAVE_MENU_PROCESS_ID_EXIT: {
                    main_menu.phase                            = MAIN_MENU_IDLE;
                    main_menu.timer                            = 0;
                    main_menu.currently_selected_option_choice = 0;
                } break;

                case SAVE_MENU_PROCESS_ID_LOADED_EXIT: {
                    
                } break;

                case SAVE_MENU_PROCESS_ID_SAVED_EXIT: {

                } break;

                default: {
                    
                } break;
            }
        } break;
    } 

    main_menu.timer += dt;

    if (any_key_down() || controller_any_button_down(get_gamepad(0))) {
        if (main_menu.phase < MAIN_MENU_IDLE) {
            main_menu.phase = MAIN_MENU_IDLE;
            return;
        }
    }
}
