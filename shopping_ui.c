enum shopping_ui_animation_phase {
    SHOPPING_UI_ANIMATION_PHASE_FADE_IN,
    SHOPPING_UI_ANIMATION_PHASE_SLIDE_IN_SHOPPING,
    SHOPPING_UI_ANIMATION_PHASE_IDLE,
    SHOPPING_UI_ANIMATION_PHASE_SLIDE_OUT_SHOPPING,
    SHOPPING_UI_ANIMATION_PHASE_PHASE_FADE_OUT,
};

#define MAX_CART_ENTRIES (1024)

enum shopping_page_filter {
    SHOPPING_PAGE_ALL,
    SHOPPING_PAGE_CONSUMABLES,
    SHOPPING_PAGE_WEAPONS,
    SHOPPING_PAGE_EQUIPMENT,
    SHOPPING_PAGE_MISC,
    SHOPPING_PAGE_BUYBACK,
    SHOPPING_PAGE_COUNT,
};

local string shopping_page_filter_strings[] = {
    string_literal("all"),
    string_literal("cons."),
    string_literal("wpns."),
    string_literal("equip."),
    string_literal("misc."),
    string_literal("buy back"),
    string_literal("(count)"),
};

struct {
    s32 phase;

    s32 current_shopping_page_filter;
    s32 shopping_item_index;

    /* remap the indices to match the current page */
    s16 cart_entry_count[MAX_CART_ENTRIES];

    /* also want to visibly animate coinage stuff */
    struct {
        s32 current;
        s32 target;

        f32 timer;
    } coin_visual;

    f32 timer;
} shopping_ui = {};

local void shop_ui_set_phase(s32 phase) {
    shopping_ui.phase = phase;
    shopping_ui.timer = 0;
}

local void shopping_ui_begin(void) {
    _debugprintf("hi shopper");
    zero_memory(&shopping_ui, sizeof(shopping_ui));
    shopping_ui.coin_visual.current = shopping_ui.coin_visual.target = 150;
}

local void do_shopping_menu(struct software_framebuffer* framebuffer, f32 x, bool allow_input) {
    struct font_cache* normal_font      = game_get_font(MENU_FONT_COLOR_WHITE);
    struct font_cache* highlighted_font = game_get_font(MENU_FONT_COLOR_GOLD);

    union color32f32 modulation_color = color32f32_WHITE;
    union color32f32 ui_color         = UI_DEFAULT_COLOR;

    bool selection_down         = is_key_down_with_repeat(KEY_DOWN);
    bool selection_up           = is_key_down_with_repeat(KEY_UP);
    bool selection_increment    = is_key_down_with_repeat(KEY_RIGHT);
    bool selection_decrement    = is_key_down_with_repeat(KEY_LEFT);
    bool selection_confirmation = is_key_pressed(KEY_RETURN);

    f32 text_scale = 2;

    if (!allow_input) {
        selection_down = selection_up =
        selection_confirmation = selection_increment =
        selection_decrement = false;

        modulation_color.a = ui_color.a = 0.5;
    }

    f32 y_cursor = 100;

    {
        f32 x_cursor = x;
        f32 largest_text_width = 0;

        for (s32 filter_index = 0; filter_index < SHOPPING_PAGE_COUNT; ++filter_index) {
            f32 measured_text_width = font_cache_text_width(normal_font, shopping_page_filter_strings[filter_index], text_scale);
            if (measured_text_width > largest_text_width) {
                largest_text_width = measured_text_width;
            }
        }

        /* Okay this UI could take advantage of having icons but that's a bit later. This'll pass in the act 1 demo I suppose. */
        for (s32 filter_index = 0; filter_index < SHOPPING_PAGE_COUNT; ++filter_index) {
            f32 page_tab_selector_offset_y = 16*2;

            struct font_cache* painting_text = normal_font;
            if (filter_index == shopping_ui.current_shopping_page_filter) {
                page_tab_selector_offset_y += 10;
                painting_text               = highlighted_font;
            }

            draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, v2f32(x_cursor, y_cursor-page_tab_selector_offset_y), largest_text_width/64+3, 3, ui_color);
            software_framebuffer_draw_text(framebuffer, painting_text, text_scale, v2f32(x_cursor, y_cursor-page_tab_selector_offset_y + 8), shopping_page_filter_strings[filter_index], modulation_color, BLEND_MODE_ALPHA);
            x_cursor += largest_text_width;
        }
    }

    draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, v2f32(x, y_cursor), 32, 18, ui_color);
}

local void game_display_and_update_shop_ui(struct software_framebuffer* framebuffer, f32 dt)  {
    /* copied from the pause menu. It's supposed to use the same type of blur anyways */
    u32 blur_samples = 2;
    f32 max_blur = 1.0;
    f32 max_grayscale = 0.8;

    const f32 FINAL_SHOPPING_MENU_X = 100;

    switch (shopping_ui.phase) {
        case SHOPPING_UI_ANIMATION_PHASE_FADE_IN: {
            const f32 MAX_TIME = 1.2f;

            f32 t = shopping_ui.timer / MAX_TIME;
            if (t >= 1.0) t = 1.0;

            game_postprocess_blur(framebuffer, blur_samples, max_blur * t, BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale * t);

            if (shopping_ui.timer >= MAX_TIME) {
                shop_ui_set_phase(SHOPPING_UI_ANIMATION_PHASE_SLIDE_IN_SHOPPING);
            }
        } break;
        case SHOPPING_UI_ANIMATION_PHASE_SLIDE_IN_SHOPPING: {
            const f32 MAX_TIME = 1.f;

            game_postprocess_blur(framebuffer, blur_samples, max_blur, BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale);

            f32 t = shopping_ui.timer / MAX_TIME;
            if (t >= 1.0) t = 1.0;

            if (shopping_ui.timer >= MAX_TIME) {
                shop_ui_set_phase(SHOPPING_UI_ANIMATION_PHASE_IDLE);
            }

            do_shopping_menu(framebuffer, lerp_f32(-999, FINAL_SHOPPING_MENU_X, t), false);
        } break;
        case SHOPPING_UI_ANIMATION_PHASE_IDLE: {
            game_postprocess_blur(framebuffer, blur_samples, max_blur, BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale);
            
            do_shopping_menu(framebuffer, FINAL_SHOPPING_MENU_X, true);
        } break;
        case SHOPPING_UI_ANIMATION_PHASE_SLIDE_OUT_SHOPPING: {
            
        } break;
        case SHOPPING_UI_ANIMATION_PHASE_PHASE_FADE_OUT: {
            
        } break;
    }

    shopping_ui.timer += dt;
}
