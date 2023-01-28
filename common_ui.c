/*
  TODO:

  - need to fix button behavior (cause I don't record the mouse being pressed on a specific button.)
*/
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

    if ((flags & COMMON_UI_BUTTON_FLAGS_DISABLED)) {
        alpha = 0.5;
        result = false;
    } else {
        if (rectangle_f32_intersect(rectangle_f32(position.x, position.y, text_width, text_height), rectangle_f32(mouse_location[0], mouse_location[1], 3, 3))) {
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
    }


    if ((flags & COMMON_UI_BUTTON_FLAGS_BREATHING)) {
        draw_ui_breathing_text(framebuffer, v2f32(position.x, position.y), font_to_use, scale, text, button_id, color32f32(1,1,1,alpha));
    } else {
        software_framebuffer_draw_text(framebuffer, font_to_use, scale, v2f32(position.x, position.y), text, color32f32(1,1,1,alpha), BLEND_MODE_ALPHA);
    }

    return result;
}

void common_ui_visual_slider(struct common_ui_layout* layout, struct software_framebuffer* framebuffer, f32 scale, string* strings, s32 count, s32* option_ptr, s32 slider_id, s32* selected_id, u32 flags) {
    /* generally all fonts have the same size... They're just different color variations... */
    struct font_cache* font_to_use = common_ui_text_normal;

    f32 longest_string_width = font_cache_text_width(common_ui_text_normal, longest_string_in_list(strings, count), scale);
    f32 text_width  = longest_string_width * count;
    f32 text_height = font_cache_text_height(common_ui_text_normal) * scale;

    v2f32 position = v2f32(layout->x, layout->y);
    common_ui_layout_advance(layout, text_width, text_height*1.3);

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
    bool slider_interaction = false;

    if (flags & COMMON_UI_VISUAL_SLIDER_FLAGS_LOTSOFOPTIONS) {
        s32 string_index = *option_ptr;
        layout->y = position.y;
        /* TODO: need some more state to fix button stuff  */
        if (common_ui_button(layout, framebuffer, string_literal("<<"), scale, 0, 0, COMMON_UI_BUTTON_FLAGS_NONE)) {
            (*option_ptr)--;
            if ((*option_ptr) < 0) {
                *option_ptr = count-1;
            }
            slider_interaction = true;
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
            slider_interaction = true;
        }
    } else {
        for (s32 string_index = 0; string_index < count; ++string_index) {
            layout->y = position.y;
            if (common_ui_button(layout, framebuffer, strings[string_index], scale, string_index, option_ptr, COMMON_UI_BUTTON_FLAGS_NONE)) {
                *option_ptr = string_index;
                slider_interaction = true;
            }
            layout->x += longest_string_width*1.2;
        }

    }
    layout->x = old_layout_x;
    layout->y = position.y;

    /*
      NOTE: this doesn't work because of the way I assign widget ids and reuse them constantly.

      I'll circumvent it by making a helper button procedure to handle the "selected" state.
     */
#if 0
    if (slider_interaction && selected_id) {
        *selected_id = slider_id;
    }
#endif

    common_ui_layout_advance(layout, text_width, text_height);
}

/* kind of like a button but not quite. */
void common_ui_checkbox(struct common_ui_layout* layout, struct software_framebuffer* framebuffer, s32 checkbox_id, s32* selected_id, bool* option_ptr, u32 flags) {
    const f32 SQUARE_SIZE = TILE_UNIT_SIZE/2;
    s32 mouse_location[2];
    get_mouse_location(mouse_location, mouse_location+1);

    v2f32 position = v2f32(layout->x, layout->y);
    common_ui_layout_advance(layout, SQUARE_SIZE*1.2, SQUARE_SIZE*1.2);

    assertion(option_ptr && "checkbox needs boolean pointer to do stuff");
    software_framebuffer_draw_quad(framebuffer, rectangle_f32(position.x, position.y, SQUARE_SIZE, SQUARE_SIZE), color32u8_WHITE, BLEND_MODE_ALPHA);
    /* TODO fix visual */
    software_framebuffer_draw_quad(framebuffer, rectangle_f32(position.x+2.5, position.y+2.5, SQUARE_SIZE-5, SQUARE_SIZE-5), color32u8_BLACK, BLEND_MODE_ALPHA);

    if (selected_id && *selected_id == checkbox_id) {
        if (is_action_pressed(INPUT_ACTION_CONFIRMATION)) {
            (*option_ptr) ^= true;
        }
    }

    if (rectangle_f32_intersect(rectangle_f32(position.x, position.y, SQUARE_SIZE, SQUARE_SIZE), rectangle_f32(mouse_location[0], mouse_location[1], 5, 5))) {
        software_framebuffer_draw_quad(framebuffer, rectangle_f32(position.x+3, position.y+3, SQUARE_SIZE-6, SQUARE_SIZE-6), color32u8(255, 0, 0, 255), BLEND_MODE_ALPHA);

        bool left, middle, right;
        get_mouse_buttons(&left, &middle, &right);
        if (left) {
            (*option_ptr) ^= true;
        }
    }

    if (*option_ptr) {
        software_framebuffer_draw_quad(framebuffer, rectangle_f32(position.x+3, position.y+3, SQUARE_SIZE-6, SQUARE_SIZE-6), color32u8(0, 255, 0, 255), BLEND_MODE_ALPHA);
    }
}

void common_ui_f32_slider(struct common_ui_layout* layout, struct software_framebuffer* framebuffer, f32 width, s32* selected_id, s32 slider_id, f32* ptr, f32 min_bound, f32 max_bound, f32 step) {
    assertion(step == 0 && "Currently I'm not handling stepped float sliders...");

    const f32 HEIGHT = TILE_UNIT_SIZE/2;

    v2f32 position = v2f32(layout->x, layout->y);
    software_framebuffer_draw_quad(framebuffer, rectangle_f32(position.x, position.y, width, HEIGHT), color32u8(255, 255, 255, 255), BLEND_MODE_ALPHA);
    software_framebuffer_draw_quad(framebuffer, rectangle_f32(position.x+3, position.y+3, width-6, HEIGHT-6), color32u8(0, 0, 0, 255), BLEND_MODE_ALPHA);

    f32 range = (max_bound-min_bound);
    if (selected_id && *selected_id == slider_id) {
        bool is_shift_down = is_key_down(KEY_SHIFT);

        if (is_action_down_with_repeat(INPUT_ACTION_MOVE_RIGHT)) {
            if (is_shift_down) {
                (*ptr) += (range * 0.1);
            } else {
                (*ptr) += (range * 0.01);
            }
        } else if (is_action_down_with_repeat(INPUT_ACTION_MOVE_LEFT)) {
            if (is_shift_down) {
                (*ptr) -= (range * 0.1);
            } else {
                (*ptr) -= (range * 0.01);
            }
        }
    }

    /* this thing doesn't drag. it just snaps to where your mouse cursor is. */
    s32 mouse_location[2];
    get_mouse_location(mouse_location, mouse_location+1);

    f32 percent = *ptr/range;
    if (rectangle_f32_intersect(rectangle_f32(position.x, position.y, width, HEIGHT), rectangle_f32(mouse_location[0], mouse_location[1], 3, 3))) {
        bool left, middle, right;
        get_mouse_buttons(&left, &middle, &right);

        if (left) {
            f32 mouse_x = mouse_location[0];
            mouse_x -= position.x;
            mouse_x /= width;

            mouse_x = clamp_f32(mouse_x, 0, 1);
            (*ptr) = min_bound + (mouse_x * range);
        }
    }

    /* TODO fix visual */
    software_framebuffer_draw_quad(framebuffer, rectangle_f32(position.x+2, position.y+2, (width-4) * percent, HEIGHT-4), color32u8(0, 255, 0, 255), BLEND_MODE_ALPHA);

    (*ptr) = clamp_f32(*ptr, min_bound, max_bound);
    common_ui_layout_advance(layout, width, HEIGHT*1.2);
}
