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

    f32 alpha = 1;

    if (!(flags & COMMON_UI_BUTTON_FLAGS_DISABLED)) {
        if (rectangle_f32_intersect(rectangle_f32(position.x, position.y, text_width, text_height),
                                    rectangle_f32(mouse_location[0], mouse_location[1], 5, 5))) {
            font_to_use = common_ui_text_highlighted;
            bool left, middle, right;
            get_mouse_buttons(&left, &middle, &right);

            if (left) {
                return true;
            }
        }

        if (selected_id && *selected_id == button_id) {
            font_to_use = common_ui_text_focused;
            if (is_action_pressed(INPUT_ACTION_CONFIRMATION)) {
                return true;
            }
        }
    } else {
        alpha = 0.5;
    }


    software_framebuffer_draw_text(framebuffer, font_to_use, scale, v2f32(position.x, position.y), text, color32f32(1,1,1,alpha), BLEND_MODE_ALPHA);
    common_ui_layout_advance(layout, text_width, text_height*1.1);
    return false;
}
