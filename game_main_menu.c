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

local string main_menu_first_page_options[] = {
    string_literal("continue"),
    string_literal("new game"),
    string_literal("load game"),
    string_literal("options"),
    string_literal("exit"),
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

local void update_and_render_main_menu(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    struct font_cache* font  = game_get_font(MENU_FONT_COLOR_GOLD);
    struct font_cache* font1 = game_get_font(MENU_FONT_COLOR_STEEL);
    struct font_cache* font2 = game_get_font(MENU_FONT_COLOR_YELLOW);

    software_framebuffer_clear_buffer(framebuffer, color32u8(0,0,0,255));

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

            software_framebuffer_draw_text_bounds_centered(framebuffer, font, 8.0, rectangle_f32(0, y_cursor, framebuffer->width, framebuffer->height*0.1),
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

            software_framebuffer_draw_text_bounds_centered(framebuffer, font, 8.0, rectangle_f32(0, 100, framebuffer->width, framebuffer->height*0.1),
                                                           string_literal("Tendered Shadows"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);

            Array_For_Each(it, string, main_menu_first_page_options, array_count(main_menu_first_page_options)) {
                software_framebuffer_draw_text_bounds_centered(framebuffer, font1, 2.0, rectangle_f32(x_cursor, y_cursor, framebuffer->width, framebuffer->height*0.1),
                                                               *it, color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                y_cursor += font_height * 2.3;
            }

            if (main_menu.timer >= 3.8f) {
                main_menu.phase = MAIN_MENU_IDLE;
            }
        } break;
        case MAIN_MENU_IDLE: {
            software_framebuffer_draw_text_bounds_centered(framebuffer, font, 8.0, rectangle_f32(0, 100, framebuffer->width, framebuffer->height*0.1),
                                                           string_literal("Tendered Shadows"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            
            f32 y_cursor = 200;
            Array_For_Each(it, string, main_menu_first_page_options, array_count(main_menu_first_page_options)) {
                software_framebuffer_draw_text_bounds_centered(framebuffer, font1, 2.0, rectangle_f32(0, y_cursor, framebuffer->width, framebuffer->height*0.1),
                                                               *it, color32f32(1,1,1,1), BLEND_MODE_ALPHA);

                y_cursor += font_height * 2.3;
            }
        } break;
    }

    main_menu.timer += dt; 
}
