/* global main menu code */

/*
  NOTE: there is a major disadvantage and that is the fact most of the UI code is not reusable right now,
  (IE: the options menu isn't usable in the main game pause menu yet. That can change soon though.)
*/

enum main_menu_animation_phase {
    MAIN_MENU_LIGHTNING_FLASHES,
    MAIN_MENU_TITLE_APPEAR,
    MAIN_MENU_OPTIONS_APPEAR,
    MAIN_MENU_IDLE,

    MAIN_MENU_SHOW_OPTIONS_PAGE_MOVE_AWAY_NORMAL_OPTIONS,
    MAIN_MENU_SHOW_OPTIONS_PAGE_MOVE_IN_OPTIONS,
    MAIN_MENU_OPTIONS_PAGE_MOVE_OUT,
    MAIN_MENU_OPTIONS_PAGE_IDLE,
};

/* All option menu code is also placed here. Along with the special *IMGUI* code lol */
struct {
    s32 phase;

    s32 lightning_flash_count;
    f32 lightning_delay_timer;
    s32 lightning_flashing   ;

    f32 timer;
    struct random_state rnd;

    s32 currently_selected_option_choice;
} main_menu;

enum {
    MAIN_MENU_OPTION_CHOICE,
    MAIN_MENU_OPTION_SLIDER,
    MAIN_MENU_OPTION_CHECKBOX,
    MAIN_MENU_OPTION_SELECTOR,
};
struct main_menu_option {
    char* choice;
    s32    type;
    s32    selected_choice;
    union {
        struct {
            f32 slider_min;
            f32 slider_max;
        };
        bool   value;
        char*  selections[128];
    };
};

local struct main_menu_option main_menu_first_page_options[] = {
    (struct main_menu_option) { .choice = ("continue") },
    (struct main_menu_option) { .choice = ("new game") },
    (struct main_menu_option) { .choice = ("load game") },
    (struct main_menu_option) { .choice = ("options") },
    (struct main_menu_option) { .choice = ("exit") },
};

struct option_resolution_pair {
    s32 w;
    s32 h;
};

struct option_resolution_pair supported_resolutions[] = {
    (struct option_resolution_pair){ 640, 480 },
    (struct option_resolution_pair){ 1280, 720 },
    (struct option_resolution_pair){ 1920, 1080 },
};

local struct main_menu_option main_menu_options_page_options[] = {
    (struct main_menu_option) {
        .choice = ("resolution"),
        .type = MAIN_MENU_OPTION_SELECTOR,
        /* I should quey this later but for now this is the lazy option. */
        .selections[0] = "640x480",
        .selections[1] = "1280x720",
        .selections[2] = "1920x1080",
    },
    (struct main_menu_option) { .choice = ("fullscreen"), .type = MAIN_MENU_OPTION_CHECKBOX },
    (struct main_menu_option) { .choice = ("back"), .type = MAIN_MENU_OPTION_CHOICE },
};

local void main_menu_next_lightning_time(void) {
    if (main_menu.lightning_flashing == true) {
        main_menu.lightning_delay_timer = random_float(&main_menu.rnd) * 0.05 + 0.07;
    } else {
        main_menu.lightning_delay_timer = random_float(&main_menu.rnd) * 0.95 + 0.1;
    }
}
local void initialize_main_menu(void) {
    main_menu.rnd                   = random_state();
    main_menu_next_lightning_time();
    return;
}

local s32 main_menu_do_menu_ui(v2f32 where, struct software_framebuffer* framebuffer, struct main_menu_option* options, s32 count, s32* option_ptr, bool allow_input) {
    const f32 TITLE_FONT_SCALE  = 6.0;
    const f32 NORMAL_FONT_SCALE = 4.0;
    /* TODO gamepad */
    bool up                = is_key_down_with_repeat(KEY_UP)  ;
    bool down              = is_key_down_with_repeat(KEY_DOWN);
    bool left              = is_key_down_with_repeat(KEY_LEFT);
    bool right             = is_key_down_with_repeat(KEY_RIGHT);
    bool confirm_selection = is_key_pressed(KEY_RETURN);

    if (!allow_input) {
        up = down = left = right = confirm_selection = false;
    }

    struct font_cache* font1       = game_get_font(MENU_FONT_COLOR_STEEL);
    struct font_cache* font2       = game_get_font(MENU_FONT_COLOR_ORANGE);
    f32                font_height = font_cache_text_height(font1);

    s32 index    = 0;
    f32 y_cursor = where.y;
    Array_For_Each(it, struct main_menu_option, options, count) {
        struct font_cache* painting_font = font1;
        if (index == (*option_ptr)) {
            painting_font = font2;
        }

        if (it->type == MAIN_MENU_OPTION_SELECTOR) {
            string selected_choice = string_from_cstring(it->selections[it->selected_choice]);
            draw_ui_breathing_text_centered(framebuffer, rectangle_f32(where.x, y_cursor, framebuffer->width, framebuffer->height*0.1), painting_font, NORMAL_FONT_SCALE, string_from_cstring(format_temp("%s : <%s>", it->choice, selected_choice.data)), index, color32f32_WHITE);

            s32 highest_choice = 0;
            for (; highest_choice < array_count(it->selections); ++highest_choice) {
                if (it->selections[highest_choice] == NULL) {
                    break;
                }
            }

            if (left) {
                it->selected_choice -= 1;

                if (it->selected_choice < 0) {
                    it->selected_choice = highest_choice - 1;
                }
            } else if (right) {
                it->selected_choice += 1;

                if (it->selected_choice >= highest_choice) {
                    it->selected_choice = 0;
                }
            }

        } else {
            draw_ui_breathing_text_centered(framebuffer, rectangle_f32(where.x, y_cursor, framebuffer->width, framebuffer->height*0.1), painting_font, NORMAL_FONT_SCALE, string_from_cstring(it->choice), index, color32f32_WHITE);
        }
        y_cursor += font_height * (NORMAL_FONT_SCALE + 0.2);
        index++;
    }

    if (up) {
        (*option_ptr) -= 1;
    }

    if (down) {
        (*option_ptr) += 1;
    }

    if ((*option_ptr) < 0) {
        *option_ptr = count-1;
    } else if ((*option_ptr) >= count) {
        *option_ptr = 0;
    }

    if (confirm_selection) {
        return *option_ptr;
    }

    return -1;
}

local void update_and_render_main_menu(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    const f32 TITLE_FONT_SCALE  = 6.0;
    const f32 NORMAL_FONT_SCALE = 4.0;

    struct font_cache* font  = game_get_font(MENU_FONT_COLOR_GOLD);
    struct font_cache* font1 = game_get_font(MENU_FONT_COLOR_STEEL);

    software_framebuffer_clear_buffer(framebuffer, color32u8(0,0,0,255));

    if (any_key_down()) {
        if (main_menu.phase < MAIN_MENU_IDLE) {
            main_menu.phase = MAIN_MENU_IDLE;
        }
    }

    f32 font_height = font_cache_text_height(font1);
    switch (main_menu.phase) {
        case MAIN_MENU_LIGHTNING_FLASHES: {
            main_menu.lightning_delay_timer -= dt;

            if (main_menu.lightning_delay_timer <= 0.0) {
                main_menu.lightning_flashing ^= true;

                if (main_menu.lightning_flashing == true)
                    main_menu.lightning_flash_count++;

                main_menu_next_lightning_time();
            }

            if (main_menu.lightning_flashing) {
                software_framebuffer_draw_quad(framebuffer, rectangle_f32(0,0,framebuffer->width,framebuffer->height), color32u8(255,255,255,255), BLEND_MODE_NONE);
            }

            if (main_menu.lightning_flash_count > 4) {
                main_menu.phase = MAIN_MENU_TITLE_APPEAR;
                main_menu.timer = 0;
            }
        } break;

        case MAIN_MENU_TITLE_APPEAR: {
            f32 t        = main_menu.timer / 4.0f;

            if (t >= 1.0) t = 1.0;
            f32 y_cursor = lerp_f32(-999, 100, t);

            software_framebuffer_draw_text_bounds_centered(framebuffer, font, TITLE_FONT_SCALE, rectangle_f32(0, y_cursor, framebuffer->width, framebuffer->height*0.1),
                                                           string_literal("Tendered Shadows"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);

            if (main_menu.timer >= 4.3f) {
                main_menu.phase = MAIN_MENU_OPTIONS_APPEAR;
                main_menu.timer = 0;
            }
        } break;

        case MAIN_MENU_OPTIONS_APPEAR: {
            f32 t = main_menu.timer / 3.5f;
            if (t >= 1.0) t = 1.0;

            f32 y_cursor = 200;
            f32 x_cursor = cubic_ease_in_f32(-999, 0, t);

            software_framebuffer_draw_text_bounds_centered(framebuffer, font, TITLE_FONT_SCALE, rectangle_f32(0, 100, framebuffer->width, framebuffer->height*0.1),
                                                           string_literal("Tendered Shadows"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);

            s32 index = 0;
            main_menu_do_menu_ui(v2f32(x_cursor, y_cursor), framebuffer, main_menu_first_page_options, array_count(main_menu_first_page_options), &main_menu.currently_selected_option_choice, false);

            if (main_menu.timer >= 3.8f) {
                main_menu.phase = MAIN_MENU_IDLE;
            }
        } break;

        case MAIN_MENU_IDLE: {
            software_framebuffer_draw_text_bounds_centered(framebuffer, font, TITLE_FONT_SCALE, rectangle_f32(0, 100, framebuffer->width, framebuffer->height*0.1),
                                                           string_literal("Tendered Shadows"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            
            f32 y_cursor = 200;

            s32 choice = main_menu_do_menu_ui(v2f32(0, y_cursor), framebuffer, main_menu_first_page_options, array_count(main_menu_first_page_options), &main_menu.currently_selected_option_choice, true);
            if (choice != -1) {
                /* play a transition if needed */
                main_menu.currently_selected_option_choice = 0;
                switch (choice) {
                    case 1: {
                        screen_mode = GAME_SCREEN_INGAME;
                        void open_TEST_storyboard();
                        /*TODO*/open_TEST_storyboard();
                    } break;
                    case 3: {
                        main_menu.timer = 0;
                        main_menu.phase = MAIN_MENU_SHOW_OPTIONS_PAGE_MOVE_AWAY_NORMAL_OPTIONS;
                    } break;
                    case 4: {
                        global_game_running = false;
                    } break;
                }
            }
        } break;

        case MAIN_MENU_SHOW_OPTIONS_PAGE_MOVE_AWAY_NORMAL_OPTIONS: {
            software_framebuffer_draw_text_bounds_centered(framebuffer, font, TITLE_FONT_SCALE, rectangle_f32(0, 100, framebuffer->width, framebuffer->height*0.1),
                                                           string_literal("Tendered Shadows"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            f32 t = main_menu.timer / 3.5f;
            if (t >= 1.0) t = 1.0;

            f32 y_cursor = 200;
            f32 x_cursor = cubic_ease_in_f32(-999, 0, (1.0 - t));

            main_menu_do_menu_ui(v2f32(x_cursor, y_cursor), framebuffer, main_menu_first_page_options, array_count(main_menu_first_page_options), &main_menu.currently_selected_option_choice, false);

            if (main_menu.timer >= 3.8f) {
                main_menu.phase = MAIN_MENU_SHOW_OPTIONS_PAGE_MOVE_IN_OPTIONS;
                main_menu.timer = 0;
            }
        } break;

        case MAIN_MENU_SHOW_OPTIONS_PAGE_MOVE_IN_OPTIONS: {
            software_framebuffer_draw_text_bounds_centered(framebuffer, font, TITLE_FONT_SCALE, rectangle_f32(0, 100, framebuffer->width, framebuffer->height*0.1),
                                                           string_literal("Options"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            f32 t = main_menu.timer / 3.5f;
            if (t >= 1.0) t = 1.0;

            f32 y_cursor = 200;
            f32 x_cursor = cubic_ease_in_f32(-999, 0, (t));

            main_menu_do_menu_ui(v2f32(x_cursor, y_cursor), framebuffer, main_menu_options_page_options, array_count(main_menu_options_page_options), &main_menu.currently_selected_option_choice, false);

            if (main_menu.timer >= 3.8f) {
                main_menu.phase = MAIN_MENU_OPTIONS_PAGE_IDLE;
                main_menu.timer = 0;
            }
        } break;

        case MAIN_MENU_OPTIONS_PAGE_MOVE_OUT: {
            software_framebuffer_draw_text_bounds_centered(framebuffer, font, TITLE_FONT_SCALE, rectangle_f32(0, 100, framebuffer->width, framebuffer->height*0.1),
                                                           string_literal("Options"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            f32 t = main_menu.timer / 3.5f;
            if (t >= 1.0) t = 1.0;

            f32 y_cursor = 200;
            f32 x_cursor = cubic_ease_in_f32(-999, 0, (1.0 - t));

            main_menu_do_menu_ui(v2f32(x_cursor, y_cursor), framebuffer, main_menu_options_page_options, array_count(main_menu_options_page_options), &main_menu.currently_selected_option_choice, false);

            if (main_menu.timer >= 3.8f) {
                main_menu.phase = MAIN_MENU_OPTIONS_APPEAR;
                main_menu.timer = 0;
            }
        } break;

        case MAIN_MENU_OPTIONS_PAGE_IDLE: {
            software_framebuffer_draw_text_bounds_centered(framebuffer, font, TITLE_FONT_SCALE, rectangle_f32(0, 100, framebuffer->width, framebuffer->height*0.1),
                                                           string_literal("Options"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            f32 y_cursor = 200;

            s32 choice = main_menu_do_menu_ui(v2f32(0, y_cursor), framebuffer, main_menu_options_page_options, array_count(main_menu_options_page_options), &main_menu.currently_selected_option_choice, true);
            if (choice != -1) {
                /* play a transition if needed */
                main_menu.currently_selected_option_choice = 0;
                struct main_menu_option* selected_choice = &main_menu_options_page_options[choice];

                switch (choice) {
                    case 0: {
                        s32 resolution_id = selected_choice->selected_choice;
                        struct option_resolution_pair* new_resolution = &supported_resolutions[resolution_id];
                        change_resolution(new_resolution->w, new_resolution->h);
                    } break;
                    case 1: {
                        toggle_fullscreen();
                    } break;
                    case 2: {
                        main_menu.phase = MAIN_MENU_OPTIONS_PAGE_MOVE_OUT;
                        main_menu.timer = 0;
                    } break;
                }
            }
        } break;
    } 

    main_menu.timer += dt * 2.41; 
}
