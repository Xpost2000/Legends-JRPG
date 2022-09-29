/* need these to be coresponding to the shopping_ui file since it uses similar code. */
enum inventory_ui_animation_phase {
    UNUSED_INVENTORY_UI_STATE_0,
    UNUSED_INVENTORY_UI_STATE_1,
    INVENTORY_UI_ANIMATION_PHASE_SLIDE_IN,
    INVENTORY_UI_ANIMATION_PHASE_IDLE,
    INVENTORY_UI_ANIMATION_PHASE_SLIDE_OUT,
    UNUSED_INVENTORY_UI_STATE_5,
};

struct {
    s32  queued_item_use_index;

    s32 confirmation_popup_selection;
} specific_inventory_ui_state;

/*
  reusing the shopping_ui
  state, which is really just going to be inventory_view_state later.
*/

void open_party_inventory_screen(void) {
    shopping_ui.phase = INVENTORY_UI_ANIMATION_PHASE_IDLE;
    shopping_ui_populate_filtered_page(SHOPPING_MODE_VIEWING);
    specific_inventory_ui_state.queued_item_use_index = -1;
    specific_inventory_ui_state.confirmation_popup_selection = 0;
}

void inventory_remove_queued_item_usage(void) {
    specific_inventory_ui_state.queued_item_use_index = -1;
}

void user_use_inventory_item_at_index(s32 item_index) {
    if (specific_inventory_ui_state.queued_item_use_index == -1) {
        specific_inventory_ui_state.queued_item_use_index = item_index;
        _debugprintf("item index: %d", item_index);
    }
}

void close_party_inventory_screen(void) {
    struct ui_pause_menu* menu_state = &game_state->ui_pause;
    menu_state->animation_state     = UI_PAUSE_MENU_TRANSITION_IN;
    menu_state->last_sub_menu_state = menu_state->sub_menu_state;
    menu_state->sub_menu_state      = UI_PAUSE_MENU_SUB_MENU_STATE_NONE;
    menu_state->transition_t = 0;
    zero_memory(&shopping_ui, sizeof(shopping_ui));
}

local void do_inventory_use_item_popup(struct software_framebuffer* framebuffer, bool viewing_inventory) {
    if (viewing_inventory)
        return;

    bool selection_move_right   = is_key_down_with_repeat(KEY_DOWN) || is_key_down_with_repeat(KEY_RIGHT);
    bool selection_move_left    = is_key_down_with_repeat(KEY_UP) || is_key_down_with_repeat(KEY_LEFT);
    bool selection_cancel       = is_key_down_with_repeat(KEY_ESCAPE);
    bool selection_confirmation = is_key_pressed(KEY_RETURN);

    if (selection_cancel) {
        inventory_remove_queued_item_usage();
        return;
    }

    const f32 TEXT_SCALE = 2;

    struct font_cache* normal_font      = game_get_font(MENU_FONT_COLOR_WHITE);
    struct font_cache* highlighted_font = game_get_font(MENU_FONT_COLOR_GOLD);

    struct entity_inventory* inventory             = (struct entity_inventory*)(&game_state->inventory);
    struct item_instance*   current_inventory_item = inventory->items + specific_inventory_ui_state.queued_item_use_index;
    struct item_def*  item_base             = item_database_find_by_id(current_inventory_item->item);

    string confirmation_string = format_temp_s("Use \"%.*s\"?", item_base->name.length, item_base->name.data);
    f32 text_width  = font_cache_text_width(normal_font, confirmation_string, TEXT_SCALE); 
    f32 text_height = font_cache_text_height(normal_font) * TEXT_SCALE;

    /* This should be a fixed size box, we'll have to find out what the longest name in the game is and scale it based off of that... */
    s32 BOX_WIDTH = (text_width/16+3);
    if (BOX_WIDTH < 25) BOX_WIDTH = 25;
    s32 BOX_HEIGHT = (5);

    v2f32 ui_box_extents = nine_patch_estimate_extents(ui_chunky, 1, BOX_WIDTH, BOX_HEIGHT);

    v2f32 start_box_point = v2f32(framebuffer->width/2 - ui_box_extents.x/2, framebuffer->height/2 - ui_box_extents.y/2);

    draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, start_box_point, BOX_WIDTH, BOX_HEIGHT, UI_DEFAULT_COLOR);
    software_framebuffer_draw_text(framebuffer, normal_font, TEXT_SCALE, v2f32(start_box_point.x+15, start_box_point.y+15), confirmation_string, color32f32_WHITE, BLEND_MODE_ALPHA);

    string options[] = {
        string_literal("USE"),
        string_literal("CANCEL"),
    };

    f32 longest_width = 0;
    {
        for (s32 option_index = 0; option_index < array_count(options); ++option_index) {
            f32 current_width = font_cache_text_width(normal_font, options[option_index], TEXT_SCALE);
            if (current_width > longest_width) {
                longest_width = current_width;
            }
        }
    }

    if (selection_move_right) {
        specific_inventory_ui_state.confirmation_popup_selection += 1;
        if (specific_inventory_ui_state.confirmation_popup_selection >= array_count(options)) {
            specific_inventory_ui_state.confirmation_popup_selection = 0;
        }
    } else if (selection_move_left) {
        specific_inventory_ui_state.confirmation_popup_selection -= 1;
        if (specific_inventory_ui_state.confirmation_popup_selection < 0) {
            specific_inventory_ui_state.confirmation_popup_selection = array_count(options)-1;
        }
    }

    {
        f32 x_cursor = start_box_point.x + 15;
        f32 y_cursor = start_box_point.y + ui_box_extents.y*0.9;

        for (s32 option_index = 0; option_index < array_count(options); ++option_index) {
            struct font_cache* painting_font = normal_font;

            if (option_index == specific_inventory_ui_state.confirmation_popup_selection) {
                painting_font = highlighted_font;
            }

            software_framebuffer_draw_text(framebuffer, painting_font, TEXT_SCALE, v2f32(x_cursor, y_cursor), options[option_index], color32f32_WHITE, BLEND_MODE_ALPHA);
            x_cursor += longest_width;
        }
    }

    /*
      would probably need to modify the menu a bit to show more stuff, but for the demo it's
      okay if I omit that feature (IE: if I heal it should show that I'm healing, but that takes some more
      UI state work, and I would best be served refactoring some of that anyways before I get to work on that...)
    */
    if (selection_confirmation) {
        switch (specific_inventory_ui_state.confirmation_popup_selection) {
            case 0: {
                /* actually use the item here. */
                {
                    s32 index = specific_inventory_ui_state.queued_item_use_index;
                    struct item_def* item = item_database_find_by_id(inventory->items[index].item);

                    if (item->type != ITEM_TYPE_MISC) {
                        _debugprintf("use item \"%.*s\"", item->name.length, item->name.data);

                        struct entity* player = game_get_player(game_state);
                        entity_inventory_use_item(inventory, index, player);
                    }   
                }
                specific_inventory_ui_state.queued_item_use_index = -1;
            } break;
            case 1: {
                specific_inventory_ui_state.queued_item_use_index = -1;
            } break;
        }
    }
}

local void update_and_render_party_inventory_screen(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    const f32 FINAL_SHOPPING_MENU_X = 20;
    struct font_cache* normal_font      = game_get_font(MENU_FONT_COLOR_WHITE);
    struct font_cache* highlighted_font = game_get_font(MENU_FONT_COLOR_GOLD);

    switch (shopping_ui.phase) {
        case INVENTORY_UI_ANIMATION_PHASE_IDLE: {
            do_gold_counter(framebuffer, dt);

            bool should_use_input = true;

            if (specific_inventory_ui_state.queued_item_use_index != -1) {
                should_use_input = false;
            }

            do_shopping_menu(framebuffer, FINAL_SHOPPING_MENU_X, should_use_input, SHOPPING_MODE_VIEWING);
            software_framebuffer_draw_text(framebuffer, normal_font, 4, v2f32(10, 10),
                                           string_literal("INVENTORY"), color32f32_WHITE, BLEND_MODE_ALPHA);
            do_inventory_use_item_popup(framebuffer, should_use_input);
        } break;
        default: {
            _debugprintf("bad case");
        } break;;
    }
}
