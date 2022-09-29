/*
  TODO: might be using too many globals.
  TODO: Move the common inventory viewing code elsewhere.
 */
enum shopping_ui_animation_phase {
    SHOPPING_UI_ANIMATION_PHASE_FADE_IN,
    SHOPPING_UI_ANIMATION_PHASE_SELECT_SHOPPING_MODE,
    SHOPPING_UI_ANIMATION_PHASE_SLIDE_IN_SHOPPING,
    SHOPPING_UI_ANIMATION_PHASE_IDLE,
    SHOPPING_UI_ANIMATION_PHASE_SLIDE_OUT_SHOPPING,
    SHOPPING_UI_ANIMATION_PHASE_PHASE_FADE_OUT,
};


/* Fixed arrays are kind of dumb but it's okay, this is alot */
#define MAX_CART_ENTRIES (1024)
enum shopping_page_filter {
    PAGE_ALL,
    PAGE_CONSUMABLES,
    PAGE_WEAPONS,
    PAGE_EQUIPMENT,
    PAGE_MISC,
    PAGE_COUNT,
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
    /* used for inventory mode */
    /* inventory mode is reusing most of this code and I should move this somewhere */
    /* just not sure right now so we're going to hack this in, and reuse this with a different behavior setting */
    SHOPPING_MODE_VIEWING,
};

local string shopping_mode_type_strings[] = {
    string_literal("Buying"),
    string_literal("Selling"),
    string_literal("Cancel"),
};

struct cart_entry {
    s32 item_index;
    s32 count;
};

struct {
    s32 phase;

    u8 shopping_mode;

    s32 current_shopping_page_filter;
    s32 shopping_item_index;

    /* densely populated set */
    struct cart_entry cart_entries[MAX_CART_ENTRIES];
    s32 cart_entry_count;

    /* I can solve everything with an additional layer of indirection... */
    s32 shop_filtered_array[MAX_CART_ENTRIES];
    s32 shop_filtered_array_count;

    /* also want to visibly animate coinage stuff */
    struct {
        s32 current;
        s32 target;

        f32 timer;
    } coin_visual;

    f32 timer;

    /* bool open_quit_confirmation; */
} shopping_ui = {};

local void shopping_ui_clear_cart(void) {
    shopping_ui.cart_entry_count = 0;
    zero_array(shopping_ui.cart_entries);
}

local s32 shopping_ui_find_cart_entry_for(s32 item_index) {
    for (s32 cart_entry_index = 0; cart_entry_index < shopping_ui.cart_entry_count; ++cart_entry_index) {
        struct cart_entry* current_cart_entry = shopping_ui.cart_entries + cart_entry_index;

        if (current_cart_entry->item_index == item_index) {
            return cart_entry_index;
        }
    }

    return -1;
}

local s32 shopping_ui_find_or_add_cart_entry_for(s32 item_index) {
    s32 result = shopping_ui_find_cart_entry_for(item_index);

    if (result == -1) {
        struct cart_entry* current_cart_entry = &shopping_ui.cart_entries[shopping_ui.cart_entry_count++];
        current_cart_entry->item_index = item_index;
        current_cart_entry->count      = 0;

        result = shopping_ui.cart_entry_count - 1;
    }

    return result;
}

void shopping_ui_increment_cart(s32 item_index) {
    struct shop_instance*    shop      = &game_state->active_shop;
    struct entity_inventory* inventory = (struct entity_inventory*)(&game_state->inventory);

    s32 cart_item_index = shopping_ui_find_or_add_cart_entry_for(item_index);
    shopping_ui.cart_entries[cart_item_index].count += 1;

    s32 item_stock_size = 0;
    bool is_infinite_item = false;

    if (shopping_ui.shopping_mode == SHOPPING_MODE_BUYING) {
        struct shop_item* current_shop_item = shop->items + item_index;
        item_stock_size                     = current_shop_item->count;

        if (item_stock_size == -1) {
            is_infinite_item = true;
        }
    } else {
        struct item_instance* current_inventory_item = inventory->items + item_index;
        item_stock_size                              = current_inventory_item->count;
    }

    if (!is_infinite_item) {
        if (shopping_ui.cart_entries[cart_item_index].count > item_stock_size) {
            shopping_ui.cart_entries[cart_item_index].count = item_stock_size;
        }
    }
}

void shopping_ui_decrement_cart(s32 item_index) {
    s32 cart_item_index = shopping_ui_find_or_add_cart_entry_for(item_index);
    shopping_ui.cart_entries[cart_item_index].count -= 1;

    if (shopping_ui.cart_entries[cart_item_index].count <= 0) {
        shopping_ui.cart_entries[cart_item_index] = shopping_ui.cart_entries[--shopping_ui.cart_entry_count];
    }
}

local s32 find_page_filter(void) {
    switch (shopping_ui.current_shopping_page_filter) {
        case PAGE_CONSUMABLES: return ITEM_TYPE_CONSUMABLE_ITEM;break;
        case PAGE_WEAPONS:     return ITEM_TYPE_WEAPON;         break;
        case PAGE_EQUIPMENT:   return ITEM_TYPE_EQUIPMENT;      break;
        case PAGE_MISC:        return ITEM_TYPE_MISC;           break;
        default: break;
    }

    return -1;
}

/* I should probably consider dictionary like constructs in the future? */
local void shopping_ui_populate_filtered_page(s32 shop_mode) {
    s32 item_filter = find_page_filter();

    shopping_ui.shop_filtered_array_count = 0;

    if (shop_mode == SHOPPING_MODE_BUYING) {
        _debugprintf("Buying!");
        struct shop_instance* shop = &game_state->active_shop;

        for (s32 shop_item_index = 0; shop_item_index < shop->item_count; ++shop_item_index) {
            struct shop_item* current_shop_item = shop->items + shop_item_index;
            struct item_def*  item_definition   = item_database_find_by_id(current_shop_item->item);

            if (item_definition->type == item_filter || shopping_ui.current_shopping_page_filter == PAGE_ALL) {
                shopping_ui.shop_filtered_array[shopping_ui.shop_filtered_array_count++] = shop_item_index;
            }
        }
    } else {
        _debugprintf("Selling!");
        struct entity_inventory* inventory = (struct entity_inventory*)(&game_state->inventory);

        for (s32 inventory_item_index = 0; inventory_item_index < inventory->count; ++inventory_item_index) {
            struct item_instance* current_inventory_item = inventory->items + inventory_item_index;
            struct item_def*      item_definition        = item_database_find_by_id(current_inventory_item->item);

            bool allow_item = true;

            if (item_id_equal(item_get_id(item_definition), item_id_make(string_literal("item_gold")))) {
                _debugprintf("I found gold!");
                allow_item = false;
            }

            if (shopping_ui.current_shopping_page_filter != PAGE_ALL) {
                if (item_definition->type != item_filter) {
                    allow_item = false;
                }
            }

            _debugprintf("%d", allow_item);
            if (allow_item) {
                _debugprintf("I'm going to add this item (\"%.*s\")", item_definition->name.length, item_definition->name.data);
                shopping_ui.shop_filtered_array[shopping_ui.shop_filtered_array_count++] = inventory_item_index;
            }
            _debugprintf("next item");
        }
    }
}

local void shop_ui_set_phase(s32 phase) {
    shopping_ui.phase = phase;
    shopping_ui.timer = 0;
}

local void shopping_ui_begin(void) {
    _debugprintf("hi shopper");
    zero_memory(&shopping_ui, sizeof(shopping_ui));
}

local void shopping_ui_finish(void) {
    game_state->shopping = false;
}

local void do_gold_counter(struct software_framebuffer* framebuffer, f32 dt) {
    /* TODO: Consider animating */
    f32 TEXT_SCALE = 4;

    struct shop_instance*    shop         = &game_state->active_shop;
    struct entity_inventory* inventory    = (struct entity_inventory*)(&game_state->inventory);
    s32                      gold_count   = entity_inventory_count_instances_of(inventory, string_literal("item_gold"));
    string                   value_string = string_from_cstring(format_temp("%d Gold", gold_count));

    struct font_cache* painting_font = game_get_font(MENU_FONT_COLOR_STEEL);
    f32                text_width    = font_cache_text_width(painting_font, value_string, TEXT_SCALE); 
    f32                text_height   = font_cache_text_height(painting_font) * TEXT_SCALE;

    s32 current_cart_value = 0;
    {
        s32 current_shop_filter = find_page_filter();

        for (s32 cart_item_index = 0; cart_item_index < shopping_ui.cart_entry_count; ++cart_item_index) {
            struct cart_entry* current_cart_entry = &shopping_ui.cart_entries[cart_item_index];

            struct item_def* item_base       = 0;
            s32              price           = 0;

            if (shopping_ui.shopping_mode == SHOPPING_MODE_BUYING) {
                struct shop_item* current_shop_item = shop->items + current_cart_entry->item_index;
                item_base                           = item_database_find_by_id(current_shop_item->item);
                price                               = shop_item_get_effective_price(shop, shop_item_id_standard(current_cart_entry->item_index));
            } else {
                struct item_instance* current_inventory_item = inventory->items + current_cart_entry->item_index;
                item_base                                    = item_database_find_by_id(current_inventory_item->item);
                price                                        = item_base->gold_value;
            }

            current_cart_value += price * current_cart_entry->count;
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
local void do_shopping_menu(struct software_framebuffer* framebuffer, f32 x, bool allow_input, s32 shop_mode) {
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

    { /* TAB SELECTOR */
        f32 x_cursor = x;
        f32 largest_text_width = 0;

        for (s32 filter_index = 0; filter_index < PAGE_COUNT; ++filter_index) {
            f32 measured_text_width = font_cache_text_width(normal_font, shopping_page_filter_strings[filter_index], text_scale);
            if (measured_text_width > largest_text_width) {
                largest_text_width = measured_text_width;
            }
        }

        /* Okay this UI could take advantage of having icons but that's a bit later. This'll pass in the act 1 demo I suppose. */
        for (s32 filter_index = 0; filter_index < PAGE_COUNT; ++filter_index) {
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
        struct shop_instance*    shop      = &game_state->active_shop;
        struct entity_inventory* inventory = (struct entity_inventory*)(&game_state->inventory);

        v2f32 ui_box_extents = nine_patch_estimate_extents(ui_chunky, 1, BOX_WIDTH, BOX_HEIGHT);

        if (shop_mode != SHOPPING_MODE_VIEWING) {
            if (selection_increment) {
                shopping_ui_increment_cart(shopping_ui.shop_filtered_array[shopping_ui.shopping_item_index]);
            } else if (selection_decrement) {
                shopping_ui_decrement_cart(shopping_ui.shop_filtered_array[shopping_ui.shopping_item_index]);
            }
        }

        if (selection_down) {
            shopping_ui.shopping_item_index += 1;

            if (shopping_ui.shopping_item_index >= shopping_ui.shop_filtered_array_count) {
                shopping_ui.shopping_item_index = 0;
            }
        } else if (selection_up) {
            shopping_ui.shopping_item_index -= 1;

            if (shopping_ui.shopping_item_index <= 0) {
                shopping_ui.shopping_item_index = shopping_ui.shop_filtered_array_count - 1;
            }
        }

        if (selection_switch_tab) {
            if (selection_switch_tab_modifier) {
                shopping_ui.current_shopping_page_filter -= 1;

                if (shopping_ui.current_shopping_page_filter < 0) {
                    shopping_ui.current_shopping_page_filter = PAGE_COUNT - 1;
                }
            } else {
                shopping_ui.current_shopping_page_filter += 1;

                if (shopping_ui.current_shopping_page_filter >= PAGE_COUNT) {
                    shopping_ui.current_shopping_page_filter = 0;
                }
            }

            shopping_ui_populate_filtered_page(shop_mode);
        }

        y_cursor += 10;

        /* It would be nice to have essentially matching pointers to make dereferencing easier. */
        for (s32 item_index = 0; item_index < shopping_ui.shop_filtered_array_count; ++item_index) {
            s32                lookup_index  = shopping_ui.shop_filtered_array[item_index];
            struct font_cache* painting_text = normal_font;

            if (item_index == shopping_ui.shopping_item_index) {
                painting_text = highlighted_font;
            }

            s32              price           = 0;
            struct item_def* item_base       = 0;
            s32              item_stack_size = 0;
            s32              item_max_size   = 0;
            
            if (shop_mode == SHOPPING_MODE_BUYING) {
                struct shop_item* current_shop_item = shop->items + lookup_index;
                item_base                           = item_database_find_by_id(current_shop_item->item);
                price                               = shop_item_get_effective_price(shop, shop_item_id_standard(item_index));
                item_stack_size                     = current_shop_item->count;
                item_max_size                       = item_stack_size;
            } else {
                struct item_instance* current_inventory_item = inventory->items + lookup_index;
                item_base                                    = item_database_find_by_id(current_inventory_item->item);
                price                                        = item_base->gold_value;
                item_stack_size                              = current_inventory_item->count;
                item_max_size                                = current_inventory_item->count;
            }

            string item_name = item_base->name;

            software_framebuffer_draw_text(framebuffer, painting_text, text_scale, v2f32(x+15, y_cursor), item_name, modulation_color, BLEND_MODE_ALPHA);
            string cart_selection_text = {};

            {
                s32 cart_amount     = 0;
                s32 cart_item_index = shopping_ui_find_cart_entry_for(lookup_index);

                if (cart_item_index != -1) {
                    cart_amount = shopping_ui.cart_entries[cart_item_index].count;
                }

                if (shop_mode == SHOPPING_MODE_VIEWING) {
                    cart_selection_text = string_from_cstring(format_temp("%d", item_stack_size));
                } else {
                    if (item_max_size == SHOP_ITEM_INFINITE) {
                        cart_selection_text = string_from_cstring(format_temp("%d (%d)", cart_amount, price * cart_amount));
                    } else {
                        cart_selection_text = string_from_cstring(format_temp("%d / %d (%d)", cart_amount, item_stack_size, price * cart_amount));
                    }
                }
            }

            f32 measurement_width = font_cache_text_width(painting_text, cart_selection_text, text_scale);

            software_framebuffer_draw_text(framebuffer, painting_text, text_scale, v2f32(x + ui_box_extents.x - (measurement_width), y_cursor), cart_selection_text, modulation_color, BLEND_MODE_ALPHA);
            y_cursor += 16*2*1.2;
        }

        if (selection_confirmation) {
            struct entity_inventory*  inventory  = (struct entity_inventory*)(&game_state->inventory);
                struct shop_instance* shop       = &game_state->active_shop;
            s32                       gold_count = entity_inventory_count_instances_of(inventory, string_literal("item_gold"));

            for (s32 item_index = 0; item_index < shopping_ui.cart_entry_count; ++item_index) {
                struct cart_entry* current_cart_entry = shopping_ui.cart_entries + item_index;

                assertion(current_cart_entry->count > 0 && "This should never happen. Just sanity checking...");

                if (shop_mode == SHOPPING_MODE_BUYING) {
                    /* shouldn't be exposed out here but okay... */
                    /* TODO: This is slightly bugged but I'm trying to replicate behavior so bug fixing comes after. */
                    for (s32 purchase_count = 0; purchase_count < current_cart_entry->count; ++purchase_count) {
                        if (purchase_item_from_shop_and_add_to_inventory(shop, inventory, MAX_PARTY_ITEMS, &gold_count, shop_item_id_standard(current_cart_entry->item_index))) {
                            _debugprintf("Thanks for shopping");
                        }
                    }
                } else if (shop_mode == SHOPPING_MODE_SELLING) {
                    for (s32 sell_count = 0; sell_count < current_cart_entry->count; ++sell_count) {
                        if (sell_item_to_shop(shop, inventory, MAX_PARTY_ITEMS, &gold_count, current_cart_entry->item_index)) {
                            _debugprintf("Thanks for selling");
                        }
                    }
                } else {
                    /* use item here */
                }
            }

            entity_inventory_set_gold_count(inventory, gold_count);
            shopping_ui_clear_cart();
            shopping_ui_populate_filtered_page(shop_mode);
            /* In reality I should keep my cursor on the nearest item to be less confusing. */
            shopping_ui.shopping_item_index = 0;
        }

        if (selection_quit) {
            shopping_ui_clear_cart();
            shopping_ui.shopping_item_index = 0;

            /* This is a bit nasty, but I'd rather handle it here. */
            if (shop_mode != SHOPPING_MODE_VIEWING) {
                shop_ui_set_phase(SHOPPING_UI_ANIMATION_PHASE_SLIDE_OUT_SHOPPING);
            } else {
                void close_party_inventory_screen(void);
                close_party_inventory_screen();
            }
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

                            shopping_ui_populate_filtered_page(shopping_ui.shopping_mode);
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

            do_shopping_menu(framebuffer, lerp_f32(-999, FINAL_SHOPPING_MENU_X, t), false, shopping_ui.shopping_mode);
            software_framebuffer_draw_text(framebuffer, normal_font, 4, v2f32(10, 10), shopping_mode_type_strings[shopping_ui.shopping_mode], color32f32(1,1,1,t2), BLEND_MODE_ALPHA);
        } break;

        case SHOPPING_UI_ANIMATION_PHASE_IDLE: {
            game_postprocess_blur(framebuffer, blur_samples, max_blur, BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale);
            
            do_gold_counter(framebuffer, dt);
            do_shopping_menu(framebuffer, FINAL_SHOPPING_MENU_X, true, shopping_ui.shopping_mode);
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

            do_shopping_menu(framebuffer, lerp_f32(-999, FINAL_SHOPPING_MENU_X, (1.0 - t)), false, shopping_ui.shopping_mode);
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
