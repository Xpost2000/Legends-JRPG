/* global main menu code */

enum main_menu_animation_phase {
    MAIN_MENU_LIGHTNING_FLASHES,
    MAIN_MENU_TITLE_APPEAR,
    MAIN_MENU_OPTIONS_APPEAR,
    MAIN_MENU_IDLE,
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
    union {
        struct {
            f32 slider_min;
            f32 slider_max;
        };
        bool   value;
        char*  selections[32];
    };
};

local struct main_menu_option main_menu_first_page_options[] = {
    (struct main_menu_option) { .choice = ("continue") },
    (struct main_menu_option) { .choice = ("new game") },
    (struct main_menu_option) { .choice = ("load game") },
    (struct main_menu_option) { .choice = ("options") },
    (struct main_menu_option) { .choice = ("exit") },
};

local void main_menu_next_lightning_time(void) {
    if (main_menu.lightning_flashing == true) {
        main_menu.lightning_delay_timer = random_float(&main_menu.rnd) * 0.05 + 0.07;
    } else {
        main_menu.lightning_delay_timer = random_float(&main_menu.rnd) * 0.65 + 0.1;
    }
}
local void initialize_main_menu(void) {
    main_menu.rnd                   = random_state();
    main_menu_next_lightning_time();
    return;
}

local s32 main_menu_do_menu_ui(v2f32 where, struct software_framebuffer* framebuffer, struct main_menu_option* options, s32 count, s32* option_ptr) {
    const f32 TITLE_FONT_SCALE  = 6.0;
    const f32 NORMAL_FONT_SCALE = 4.0;
    /* TODO gamepad */
    bool up    = is_key_down_with_repeat(KEY_UP)  ;
    bool down  = is_key_down_with_repeat(KEY_DOWN);
    bool left  = is_key_down_with_repeat(KEY_LEFT);
    bool right = is_key_down_with_repeat(KEY_RIGHT);

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

        draw_ui_breathing_text_centered(framebuffer, rectangle_f32(where.x, y_cursor, framebuffer->width, framebuffer->height*0.1), painting_font, NORMAL_FONT_SCALE, string_from_cstring(it->choice), index, color32f32_WHITE);
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

    if (is_key_pressed(KEY_RETURN)) {
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

            /* TODO, better for different resolutions */
            f32 t = main_menu.timer / 3.5f;
            if (t >= 1.0) t = 1.0;

            f32 y_cursor = 200;
            f32 x_cursor = cubic_ease_in_f32(-999, 0, t);

            software_framebuffer_draw_text_bounds_centered(framebuffer, font, TITLE_FONT_SCALE, rectangle_f32(0, 100, framebuffer->width, framebuffer->height*0.1),
                                                           string_literal("Tendered Shadows"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);

            s32 index = 0;
            Array_For_Each(it, struct main_menu_option, main_menu_first_page_options, array_count(main_menu_first_page_options)) {
                draw_ui_breathing_text_centered(framebuffer,  rectangle_f32(x_cursor, y_cursor, framebuffer->width, framebuffer->height*0.1), font1, NORMAL_FONT_SCALE, string_from_cstring(it->choice), index, color32f32_WHITE);
                y_cursor += font_height * (NORMAL_FONT_SCALE + 0.2);
                index++;
            }

            if (main_menu.timer >= 3.8f) {
                main_menu.phase = MAIN_MENU_IDLE;
            }
        } break;
        case MAIN_MENU_IDLE: {
            software_framebuffer_draw_text_bounds_centered(framebuffer, font, TITLE_FONT_SCALE, rectangle_f32(0, 100, framebuffer->width, framebuffer->height*0.1),
                                                           string_literal("Tendered Shadows"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            
            f32 y_cursor = 200;

            s32 index = 0;
            s32 choice = main_menu_do_menu_ui(v2f32(0, y_cursor), framebuffer, main_menu_first_page_options, array_count(main_menu_first_page_options), &main_menu.currently_selected_option_choice);
            if (choice != -1) {
                /* play a transition if needed */
                main_menu.currently_selected_option_choice = 0;
                switch (choice) {
                    case 1: {
                        screen_mode = GAME_SCREEN_INGAME;
                        void open_TEST_storyboard();
                        /*TODO*/open_TEST_storyboard();
                    } break;
                    case 4: {
                        global_game_running = false;
                    } break;
                }
            }
        } break;
    }

    main_menu.timer += dt; 
}
