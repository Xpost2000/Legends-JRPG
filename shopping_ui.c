/*
  TODO: This should become the new basis for the inventory system UI.
  TODO: Needs a confirm menu.
*/
enum shopping_ui_animation_phase {
    SHOPPING_UI_ANIMATION_PHASE_FADE_IN,
    SHOPPING_UI_ANIMATION_PHASE_SELECT_SHOPPING_MODE,
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
    SHOPPING_PAGE_COUNT,
    SHOPPING_PAGE_BUYBACK,
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

enum shopping_mode_type {
    SHOPPING_MODE_BUYING,
    SHOPPING_MODE_SELLING,
};

local string shopping_mode_type_strings[] = {
    string_literal("Buying"),
    string_literal("Selling"),
    string_literal("Cancel"),
};

struct {
    s32 phase;

    u8 shopping_mode;

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

    /* bool open_quit_confirmation; */
} shopping_ui = {};

local s32 find_shop_filter(void) {
    switch (shopping_ui.current_shopping_page_filter) {
        case SHOPPING_PAGE_CONSUMABLES: return ITEM_TYPE_CONSUMABLE_ITEM;break;
        case SHOPPING_PAGE_WEAPONS:     return ITEM_TYPE_WEAPON;         break;
        case SHOPPING_PAGE_EQUIPMENT:   return ITEM_TYPE_EQUIPMENT;      break;
        case SHOPPING_PAGE_MISC:        return ITEM_TYPE_MISC;           break;
        default: break;
    }

    return -1;
}

local void shop_ui_set_phase(s32 phase) {
    shopping_ui.phase = phase;
    shopping_ui.timer = 0;
}

local void shopping_ui_begin(void) {
    _debugprintf("hi shopper");
    zero_memory(&shopping_ui, sizeof(shopping_ui));
    shopping_ui.coin_visual.current = shopping_ui.coin_visual.target = 150;
}

local void shopping_ui_finish(void) {
    game_state->shopping = false;
}

local void do_gold_counter(struct software_framebuffer* framebuffer, f32 dt) {
    /* TODO: Consider animating */
    f32 TEXT_SCALE = 4;

    struct entity_inventory* inventory    = (struct entity_inventory*)(&game_state->inventory);
    s32                      gold_count   = entity_inventory_count_instances_of(inventory, string_literal("item_gold"));
    string                   value_string = string_from_cstring(format_temp("%d Gold", gold_count));

    struct font_cache* painting_font = game_get_font(MENU_FONT_COLOR_STEEL);
    f32                text_width    = font_cache_text_width(painting_font, value_string, TEXT_SCALE); 
    f32                text_height   = font_cache_text_height(painting_font) * TEXT_SCALE;

    s32 current_cart_value = 0;
    {
        s32 current_shop_filter = find_shop_filter();

        if (shopping_ui.shopping_mode == SHOPPING_MODE_BUYING) {
            struct shop_instance* shop = &game_state->active_shop;

            s32 remapped_index = 0;
            for (s32 index = 0; index < shop->item_count; ++index) {
                struct shop_item* cart_item = &shop->items[index];
                struct item_def* item_base = item_database_find_by_id(cart_item->item);

                if (current_shop_filter != -1 && item_base->type != current_shop_filter) {
                    continue;
                }

                current_cart_value += cart_item->price * shopping_ui.cart_entry_count[remapped_index];
                remapped_index += 1;
            }
        } else {
        }
    } 

    draw_ui_breathing_text(framebuffer, v2f32(SCREEN_WIDTH - text_width, 15), painting_font, TEXT_SCALE, value_string, 2142, color32f32_WHITE);

    if (current_cart_value != 0) {
        if (shopping_ui.shopping_mode == SHOPPING_MODE_BUYING) {
            painting_font = game_get_font(MENU_FONT_COLOR_BLOODRED);
            value_string = string_from_cstring(format_temp("- %d", current_cart_value));
        } else {
            painting_font = game_get_font(MENU_FONT_COLOR_LIME);
            value_string = string_from_cstring(format_temp("+ %d", current_cart_value));
        }
        
        text_width   = font_cache_text_width(painting_font, value_string, TEXT_SCALE); 
        draw_ui_breathing_text(framebuffer, v2f32(SCREEN_WIDTH - text_width, 15 + text_height*1.1), painting_font, TEXT_SCALE, value_string, 2142, color32f32_WHITE);
    }
}

/* There is duplicated code. Beware, and maybe try and shrink this. */
local void do_shopping_menu(struct software_framebuffer* framebuffer, f32 x, bool allow_input) {
    struct font_cache* normal_font      = game_get_font(MENU_FONT_COLOR_WHITE);
    struct font_cache* highlighted_font = game_get_font(MENU_FONT_COLOR_GOLD);

    union color32f32 modulation_color = color32f32_WHITE;
    union color32f32 ui_color         = UI_DEFAULT_COLOR;

    bool selection_down                = is_key_down_with_repeat(KEY_DOWN);
    bool selection_up                  = is_key_down_with_repeat(KEY_UP);
    bool selection_increment           = is_key_down_with_repeat(KEY_RIGHT);
    bool selection_decrement           = is_key_down_with_repeat(KEY_LEFT);
    bool selection_confirmation        = is_key_pressed(KEY_RETURN);
    bool selection_switch_tab_modifier = is_key_down(KEY_SHIFT);
    bool selection_switch_tab          = is_key_down_with_repeat(KEY_TAB);
    bool selection_quit                = is_key_pressed(KEY_ESCAPE);

    f32 text_scale = 2;

    if (!allow_input) {
        selection_down = selection_up = selection_confirmation = selection_increment = selection_decrement = selection_switch_tab_modifier = selection_switch_tab = selection_quit = false;
        modulation_color.a = ui_color.a = 0.5;
    }

    f32 y_cursor = 100;

    /* TODO:

       Does not handle larger lists of items simply cause there are no shops that exist like that right now.
     */
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
            software_framebuffer_draw_text(framebuffer, painting_text, text_scale, v2f32(x_cursor+8, y_cursor-page_tab_selector_offset_y + 9), shopping_page_filter_strings[filter_index], modulation_color, BLEND_MODE_ALPHA);
            x_cursor += largest_text_width;
        }
    }

    s32 BOX_WIDTH  = 35;
    s32 BOX_HEIGHT = 20;

    draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, v2f32(x, y_cursor), BOX_WIDTH, BOX_HEIGHT, ui_color);

    {
        struct shop_instance* shop = &game_state->active_shop;

        v2f32 ui_box_extents = nine_patch_estimate_extents(ui_chunky, 1, BOX_WIDTH, BOX_HEIGHT);

        if (selection_increment) {
            shopping_ui.cart_entry_count[shopping_ui.shopping_item_index] += 1;
            /* can't check the clamp here since I don't have the item information yet, and it's a waste to
               query it here.*/
        } else if (selection_decrement) {
            shopping_ui.cart_entry_count[shopping_ui.shopping_item_index] -= 1;
            if (shopping_ui.cart_entry_count[shopping_ui.shopping_item_index] < 0) {
                shopping_ui.cart_entry_count[shopping_ui.shopping_item_index] = 0;
            }
        }

        if (selection_down) {
            shopping_ui.shopping_item_index += 1;

            if (shopping_ui.shopping_item_index >= shop->item_count) {
                shopping_ui.shopping_item_index = 0;
            }
        } else if (selection_up) {
            shopping_ui.shopping_item_index -= 1;

            if (shopping_ui.shopping_item_index < 0) {
                shopping_ui.shopping_item_index = shop->item_count - 1;
            }
        }

        if (selection_switch_tab) {
            if (selection_switch_tab_modifier) {
                shopping_ui.current_shopping_page_filter -= 1;

                if (shopping_ui.current_shopping_page_filter < 0) {
                    shopping_ui.current_shopping_page_filter = SHOPPING_PAGE_COUNT - 1;
                }
            } else {
                shopping_ui.current_shopping_page_filter += 1;

                if (shopping_ui.current_shopping_page_filter >= SHOPPING_PAGE_COUNT) {
                    shopping_ui.current_shopping_page_filter = 0;
                }
            }

            zero_array(shopping_ui.cart_entry_count);
        }

        switch (shopping_ui.current_shopping_page_filter) {
            case SHOPPING_PAGE_ALL: {
                f32 y_cursor = 120;

                switch (shopping_ui.shopping_mode) {
                    case SHOPPING_MODE_BUYING: {
                        for (s32 item_index = 0; item_index < shop->item_count; ++item_index) {
                            struct shop_item* current_shop_item = shop->items + item_index;
                            struct font_cache* painting_text = normal_font;

                            if (item_index == shopping_ui.shopping_item_index) {
                                painting_text = highlighted_font;
                            }

                            struct item_def* item_base = item_database_find_by_id(current_shop_item->item);
                            string item_name = item_base->name;

                            software_framebuffer_draw_text(framebuffer, painting_text, text_scale, v2f32(x+15, y_cursor), item_name, modulation_color, BLEND_MODE_ALPHA);
                            string cart_selection_text = {};

                            /* NOTE: This should be tabular but whatever. */
                            if (current_shop_item->count == SHOP_ITEM_INFINITE) {
                                cart_selection_text = string_from_cstring(format_temp("%d", shopping_ui.cart_entry_count[item_index]));
                            } else {
                                if (shopping_ui.cart_entry_count[shopping_ui.shopping_item_index] > current_shop_item->count) {
                                    shopping_ui.cart_entry_count[shopping_ui.shopping_item_index] = current_shop_item->count;
                                }

                                cart_selection_text = string_from_cstring(format_temp("%d / %d", shopping_ui.cart_entry_count[item_index], current_shop_item->count));
                            }

                            f32 measurement_width = font_cache_text_width(painting_text, cart_selection_text, text_scale);

                            software_framebuffer_draw_text(framebuffer, painting_text, text_scale, v2f32(x + ui_box_extents.x - (measurement_width), y_cursor), cart_selection_text, modulation_color, BLEND_MODE_ALPHA);
                            y_cursor += 16*2*1.2;
                        }
                    } break;
                    case SHOPPING_MODE_SELLING: {
                        struct entity_inventory* inventory = (struct entity_inventory*)(&game_state->inventory);

                        for (s32 item_index = 0; item_index < inventory->count; ++item_index) {
                            struct item_instance* current_inventory_item = inventory->items + item_index;
                            struct font_cache* painting_text = normal_font;

                            if (item_index == shopping_ui.shopping_item_index) {
                                painting_text = highlighted_font;
                            }

                            struct item_def* item_base = item_database_find_by_id(current_inventory_item->item);
                            string item_name = item_base->name;

                            software_framebuffer_draw_text(framebuffer, painting_text, text_scale, v2f32(x+15, y_cursor), item_name, modulation_color, BLEND_MODE_ALPHA);
                            string cart_selection_text = {};

                            /* NOTE: This should be tabular but whatever. */
                            if (current_inventory_item->count == SHOP_ITEM_INFINITE) {
                                cart_selection_text = string_from_cstring(format_temp("%d", shopping_ui.cart_entry_count[item_index]));
                            } else {
                                if (shopping_ui.cart_entry_count[shopping_ui.shopping_item_index] > current_inventory_item->count) {
                                    shopping_ui.cart_entry_count[shopping_ui.shopping_item_index] = current_inventory_item->count;
                                }

                                cart_selection_text = string_from_cstring(format_temp("%d / %d", shopping_ui.cart_entry_count[item_index], current_inventory_item->count));
                            }

                            f32 measurement_width = font_cache_text_width(painting_text, cart_selection_text, text_scale);

                            software_framebuffer_draw_text(framebuffer, painting_text, text_scale, v2f32(x + ui_box_extents.x - (measurement_width), y_cursor), cart_selection_text, modulation_color, BLEND_MODE_ALPHA);
                            y_cursor += 16*2*1.2;
                        }
                    } break;
                }
            } break;
                
            case SHOPPING_PAGE_BUYBACK: {/* Not planning to do buyback anymore. */} break;

            default: {
                f32 y_cursor = 120;

                s32 filter_for = find_shop_filter();

                /* NOTE: should be refactored */
                /* NOTE: Turn prices into a table, which will allow me to reuse the same code for both modes. Outside of having to display prices. */
                /* TODO: We don't show prices yet. */
                s32 remapped_index = 0;
                switch (shopping_ui.shopping_mode) {
                    case SHOPPING_MODE_BUYING: {
                        for (s32 item_index = 0; item_index < shop->item_count; ++item_index) {
                            struct shop_item* current_shop_item = shop->items + item_index;
                            struct font_cache* painting_text = normal_font;

                            if (remapped_index == shopping_ui.shopping_item_index) {
                                painting_text = highlighted_font;
                            }

                            struct item_def* item_base = item_database_find_by_id(current_shop_item->item);

                            if (item_base->type != filter_for) {
                                continue;
                            }

                            string item_name = item_base->name;

                            software_framebuffer_draw_text(framebuffer, painting_text, text_scale, v2f32(x+15, y_cursor), item_name, modulation_color, BLEND_MODE_ALPHA);
                            string cart_selection_text = {};

                            /* NOTE: This should be tabular but whatever. */
                            if (current_shop_item->count == SHOP_ITEM_INFINITE) {
                                cart_selection_text = string_from_cstring(format_temp("%d", shopping_ui.cart_entry_count[remapped_index]));
                            } else {
                                if (shopping_ui.cart_entry_count[shopping_ui.shopping_item_index] > current_shop_item->count) {
                                    shopping_ui.cart_entry_count[shopping_ui.shopping_item_index] = current_shop_item->count;
                                }

                                cart_selection_text = string_from_cstring(format_temp("%d / %d", shopping_ui.cart_entry_count[remapped_index], current_shop_item->count));
                            }

                            f32 measurement_width = font_cache_text_width(painting_text, cart_selection_text, text_scale);

                            software_framebuffer_draw_text(framebuffer, painting_text, text_scale, v2f32(x + ui_box_extents.x - (measurement_width), y_cursor), cart_selection_text, modulation_color, BLEND_MODE_ALPHA);
                            y_cursor += 16*2*1.2;

                            remapped_index += 1;
                        }
                    } break;
                    case SHOPPING_MODE_SELLING: {
                        struct entity_inventory* inventory = (struct entity_inventory*)(&game_state->inventory);
                        for (s32 item_index = 0; item_index < inventory->count; ++item_index) {
                            struct item_instance* current_inventory_item = inventory->items + item_index;
                            struct font_cache* painting_text = normal_font;

                            if (remapped_index == shopping_ui.shopping_item_index) {
                                painting_text = highlighted_font;
                            }

                            struct item_def* item_base = item_database_find_by_id(current_inventory_item->item);

                            if (item_base->type != filter_for) {
                                continue;
                            }

                            string item_name = item_base->name;

                            software_framebuffer_draw_text(framebuffer, painting_text, text_scale, v2f32(x+15, y_cursor), item_name, modulation_color, BLEND_MODE_ALPHA);
                            string cart_selection_text = {};

                            /* NOTE: This should be tabular but whatever. */
                            if (current_inventory_item->count == SHOP_ITEM_INFINITE) {
                                cart_selection_text = string_from_cstring(format_temp("%d", shopping_ui.cart_entry_count[remapped_index]));
                            } else {
                                if (shopping_ui.cart_entry_count[shopping_ui.shopping_item_index] > current_inventory_item->count) {
                                    shopping_ui.cart_entry_count[shopping_ui.shopping_item_index] = current_inventory_item->count;
                                }

                                cart_selection_text = string_from_cstring(format_temp("%d / %d", shopping_ui.cart_entry_count[remapped_index], current_inventory_item->count));
                            }

                            f32 measurement_width = font_cache_text_width(painting_text, cart_selection_text, text_scale);

                            software_framebuffer_draw_text(framebuffer, painting_text, text_scale, v2f32(x + ui_box_extents.x - (measurement_width), y_cursor), cart_selection_text, modulation_color, BLEND_MODE_ALPHA);
                            y_cursor += 16*2*1.2;

                            remapped_index += 1;
                        }
                    } break;
                }
                _debugprintf("TODO not done");
            } break;
        }

        if (selection_confirmation) {
            _debugprintf("TODO: shop!"); 
            /* We would sell the item or buy the item now. Not implemented until needed. */
        }

        if (selection_quit) {
            shop_ui_set_phase(SHOPPING_UI_ANIMATION_PHASE_SLIDE_OUT_SHOPPING);
        }
    }
}

local void game_display_and_update_shop_ui(struct software_framebuffer* framebuffer, f32 dt)  {
    /* copied from the pause menu. It's supposed to use the same type of blur anyways */
    u32 blur_samples = 2;
    f32 max_blur = 1.0;
    f32 max_grayscale = 0.8;

    const f32 FINAL_SHOPPING_MENU_X = 20;

    struct font_cache* normal_font      = game_get_font(MENU_FONT_COLOR_WHITE);
    struct font_cache* highlighted_font = game_get_font(MENU_FONT_COLOR_GOLD);


    switch (shopping_ui.phase) {
        case SHOPPING_UI_ANIMATION_PHASE_FADE_IN: {
            const f32 MAX_TIME = 1.2f;

            f32 t = shopping_ui.timer / MAX_TIME;
            if (t >= 1.0) t = 1.0;

            game_postprocess_blur(framebuffer, blur_samples, max_blur * t, BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale * t);

            if (shopping_ui.timer >= MAX_TIME) {
                shop_ui_set_phase(SHOPPING_UI_ANIMATION_PHASE_SELECT_SHOPPING_MODE);
            }
        } break;

        case SHOPPING_UI_ANIMATION_PHASE_SELECT_SHOPPING_MODE: {
            game_postprocess_blur(framebuffer, blur_samples, max_blur, BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale);

            s32       BOX_WIDTH  = 10;
            s32       BOX_HEIGHT = 9;
            const f32 text_scale = 2;
            v2f32 ui_box_extents = nine_patch_estimate_extents(ui_chunky, 1, BOX_WIDTH, BOX_HEIGHT);
            v2f32 cursor_draw    = v2f32(framebuffer->width/2 - ui_box_extents.x/2, framebuffer->height/2 - ui_box_extents.y/2);

            draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, cursor_draw, BOX_WIDTH, BOX_HEIGHT, UI_DEFAULT_COLOR);
            cursor_draw.x += 10;
            cursor_draw.y += 10;
            software_framebuffer_draw_text(framebuffer, normal_font, text_scale, cursor_draw, string_literal("SHOPPING"), color32f32_WHITE, BLEND_MODE_ALPHA);

            {
                struct font_cache* painting_text = normal_font;

                for (s32 mode_selection_index = 0; mode_selection_index < 2; ++mode_selection_index) {
                    painting_text = normal_font;

                    if (mode_selection_index == shopping_ui.shopping_item_index) {
                        painting_text = highlighted_font;
                    }

                    cursor_draw.y += 2 * 16 * 1.1; 
                    software_framebuffer_draw_text(framebuffer, painting_text, text_scale, cursor_draw, shopping_mode_type_strings[mode_selection_index], color32f32_WHITE, BLEND_MODE_ALPHA);
                }

                cursor_draw.y += 2 * 16 * 1.5;
                if (shopping_ui.shopping_item_index== 2) {
                    painting_text = highlighted_font;
                } else {
                    painting_text = normal_font;
                }

                software_framebuffer_draw_text(framebuffer, painting_text, text_scale, cursor_draw, shopping_mode_type_strings[2], color32f32_WHITE, BLEND_MODE_ALPHA);

                /* Am I getting punished for not writing a proper UI library? Maybe. */
                {
                    bool selection_confirmation = is_key_pressed(KEY_RETURN);
                    bool selection_down         = is_key_down_with_repeat(KEY_DOWN);
                    bool selection_up           = is_key_down_with_repeat(KEY_UP);

                    if (selection_up) {
                        shopping_ui.shopping_item_index -= 1;

                        if (shopping_ui.shopping_item_index < 0) {
                            shopping_ui.shopping_item_index = 2;
                        }
                    } else if (selection_down) {
                        shopping_ui.shopping_item_index += 1;

                        if (shopping_ui.shopping_item_index > 2) {
                            shopping_ui.shopping_item_index = 0;
                        }
                    }

                    if (selection_confirmation) {
                        if (shopping_ui.shopping_item_index == 2) {
                            shop_ui_set_phase(SHOPPING_UI_ANIMATION_PHASE_PHASE_FADE_OUT);
                        } else {
                            if (shopping_ui.shopping_item_index == 0) {
                                shopping_ui.shopping_mode = SHOPPING_MODE_BUYING;
                            } else {
                                shopping_ui.shopping_mode = SHOPPING_MODE_SELLING;
                            }

                            shopping_ui.shopping_item_index = 0;
                            shop_ui_set_phase(SHOPPING_UI_ANIMATION_PHASE_SLIDE_IN_SHOPPING);
                        }
                    }
                }
            }
        } break;

        case SHOPPING_UI_ANIMATION_PHASE_SLIDE_IN_SHOPPING: {
            const f32 MAX_TIME = 1.6f;

            game_postprocess_blur(framebuffer, blur_samples, max_blur, BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale);

            f32 t  = shopping_ui.timer / (MAX_TIME-0.6);
            f32 t2 = (shopping_ui.timer-1.0) / 0.6;
            if (t2 < 0)    t2 = 0.0;
            if (t2 >= 1.0) t2 = 1.0;
            if (t >= 1.0)  t  = 1.0;

            if (shopping_ui.timer >= MAX_TIME) {
                shop_ui_set_phase(SHOPPING_UI_ANIMATION_PHASE_IDLE);
            }

            do_shopping_menu(framebuffer, lerp_f32(-999, FINAL_SHOPPING_MENU_X, t), false);
            software_framebuffer_draw_text(framebuffer, normal_font, 4, v2f32(10, 10), shopping_mode_type_strings[shopping_ui.shopping_mode], color32f32(1,1,1,t2), BLEND_MODE_ALPHA);
        } break;

        case SHOPPING_UI_ANIMATION_PHASE_IDLE: {
            game_postprocess_blur(framebuffer, blur_samples, max_blur, BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale);
            
            do_gold_counter(framebuffer, dt);
            do_shopping_menu(framebuffer, FINAL_SHOPPING_MENU_X, true);
            software_framebuffer_draw_text(framebuffer, normal_font, 4, v2f32(10, 10), shopping_mode_type_strings[shopping_ui.shopping_mode], color32f32_WHITE, BLEND_MODE_ALPHA);
        } break;

        case SHOPPING_UI_ANIMATION_PHASE_SLIDE_OUT_SHOPPING: {
            const f32 MAX_TIME = 1.6f;

            game_postprocess_blur(framebuffer, blur_samples, max_blur, BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale);

            f32 t  = shopping_ui.timer / (MAX_TIME-0.6);
            f32 t2 = (shopping_ui.timer-1.0) / 0.6;
            if (t2 < 0)    t2 = 0.0;
            if (t2 >= 1.0) t2 = 1.0;
            if (t >= 1.0)  t  = 1.0;

            if (shopping_ui.timer >= MAX_TIME) {
                shop_ui_set_phase(SHOPPING_UI_ANIMATION_PHASE_SELECT_SHOPPING_MODE);
            }

            do_shopping_menu(framebuffer, lerp_f32(-999, FINAL_SHOPPING_MENU_X, (1.0 - t)), false);
            software_framebuffer_draw_text(framebuffer, normal_font, 4, v2f32(10, 10), shopping_mode_type_strings[shopping_ui.shopping_mode], color32f32(1,1,1,(1.0 - t2)), BLEND_MODE_ALPHA);
        } break;

        case SHOPPING_UI_ANIMATION_PHASE_PHASE_FADE_OUT: {
            const f32 MAX_TIME = 1.2f;

            f32 t = shopping_ui.timer / MAX_TIME;
            if (t >= 1.0) t = 1.0;

            game_postprocess_blur(framebuffer, blur_samples, max_blur * (1.0 - t), BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale * (1.0 - t));

            if (shopping_ui.timer >= MAX_TIME) {
                shopping_ui_finish();
            }
        } break;
    }

    shopping_ui.timer += dt;
}
