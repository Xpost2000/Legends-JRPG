void common_ui_init(void) {
    common_ui_text_normal      = game_get_font(MENU_FONT_COLOR_WHITE);
    common_ui_text_highlighted = game_get_font(MENU_FONT_COLOR_BLOODRED);
    common_ui_text_focused     = game_get_font(MENU_FONT_COLOR_GOLD);
}

void common_ui_layout_advance(struct common_ui_layout* layout, f32 width, f32 height) {
    /* would advance differently depending on layout settings, but for now since it's only vertical */
    layout->y += height;
}

/* NOTE: all UI is assumed to happen on an identity transformation */
/* Ideally I'd also push render commands for this but whatever */
bool common_ui_button(struct common_ui_layout* layout, struct software_framebuffer* framebuffer, string text, f32 scale, s32 button_id, s32* selected_id, u32 flags) {
    /* generally all fonts have the same size... They're just different color variations... */
    struct font_cache* font_to_use = common_ui_text_normal;

    f32 text_width  = font_cache_text_width(common_ui_text_normal, text, scale);
    f32 text_height = font_cache_text_height(common_ui_text_normal) * scale;

    s32 mouse_location[2];
    get_mouse_location(mouse_location, mouse_location+1);

    v2f32 position = v2f32(layout->x, layout->y);
    common_ui_layout_advance(layout, text_width, text_height*1.2);

    f32 alpha = 1;

    bool result = false;
    if (!(flags & COMMON_UI_BUTTON_FLAGS_DISABLED)) {
        if (rectangle_f32_intersect(rectangle_f32(position.x, position.y, text_width, text_height),
                                    rectangle_f32(mouse_location[0], mouse_location[1], 5, 5))) {
            font_to_use = common_ui_text_highlighted;
            bool left, middle, right;
            get_mouse_buttons(&left, &middle, &right);

            if (left) {
                result = true;
            }
        }

        if (selected_id && *selected_id == button_id) {
            font_to_use = common_ui_text_focused;
            if (is_action_pressed(INPUT_ACTION_CONFIRMATION)) {
                result = true;
            }
        }
    } else {
        alpha = 0.5;
    }


    software_framebuffer_draw_text(framebuffer, font_to_use, scale, v2f32(position.x, position.y), text, color32f32(1,1,1,alpha), BLEND_MODE_ALPHA);
    return result;
}

void common_ui_visual_slider(struct common_ui_layout* layout, struct software_framebuffer* framebuffer, f32 scale, string* strings, s32 count, s32* option_ptr, s32 slider_id, s32* selected_id, u32 flags) {
    /* generally all fonts have the same size... They're just different color variations... */
    struct font_cache* font_to_use = common_ui_text_normal;

    f32 longest_string_width = font_cache_text_width(common_ui_text_normal, longest_string_in_list(strings, count), scale);
    f32 text_width  = longest_string_width * count;
    f32 text_height = font_cache_text_height(common_ui_text_normal) * scale;

    v2f32 position = v2f32(layout->x, layout->y);
    common_ui_layout_advance(layout, text_width, text_height*1.2);

    /* This will also handle arrow key selection */
    if (selected_id && *selected_id == slider_id) {
        assertion(option_ptr && "Hmm? Why are you using a slider if there's nothing to select?");

        if (is_action_down_with_repeat(INPUT_ACTION_MOVE_LEFT)) {
            (*option_ptr)--;
            if ((*option_ptr) < 0) {
                *option_ptr = count-1;
            }
        } else if (is_action_down_with_repeat(INPUT_ACTION_MOVE_RIGHT)) {
            (*option_ptr)++;
            if ((*option_ptr) >= count) {
                *option_ptr = 0;
            }
        }
    }

    
    f32 old_layout_x = layout->x;
    if (flags & COMMON_UI_VISUAL_SLIDER_FLAGS_LOTSOFOPTIONS) {
        s32 string_index = *option_ptr;
        layout->y = position.y;
        /* TODO: need some more state to fix button stuff  */
        if (common_ui_button(layout, framebuffer, string_literal("<<"), scale, 0, 0, COMMON_UI_BUTTON_FLAGS_NONE)) {
            (*option_ptr)--;
            if ((*option_ptr) < 0) {
                *option_ptr = count-1;
            }
        }
        layout->y = position.y;
        layout->x += font_cache_text_width(common_ui_text_normal, string_literal("<<"), scale) * 3;
        if (common_ui_button(layout, framebuffer, strings[string_index], scale, string_index, 0, COMMON_UI_BUTTON_FLAGS_NONE)) {
            /*?*/ 
        }
        layout->y = position.y;
        layout->x += longest_string_width * 1.2;
        if (common_ui_button(layout, framebuffer, string_literal(">>"), scale, 0, 0, COMMON_UI_BUTTON_FLAGS_NONE)) {
            (*option_ptr)++;
            if ((*option_ptr) >= count) {
                *option_ptr = 0;
            }
        }
    } else {
        for (s32 string_index = 0; string_index < count; ++string_index) {
            layout->y = position.y;
            if (common_ui_button(layout, framebuffer, strings[string_index], scale, string_index, option_ptr, COMMON_UI_BUTTON_FLAGS_NONE)) {
                *option_ptr = string_index;
            }
            layout->x += longest_string_width*1.2;
        }

    }
    layout->x = old_layout_x;
    layout->y = position.y;

    common_ui_layout_advance(layout, text_width, text_height);
}
