/* need these to be coresponding to the shopping_ui file since it uses similar code. */
/* small selection bug error */
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
    s32  queued_party_member_use_index;

    s32 confirmation_popup_selection;
} specific_inventory_ui_state;

/*
  reusing the shopping_ui
  state, which is really just going to be inventory_view_state later.
*/

local void repopulate_inventory_view(void) {
    shopping_ui_populate_filtered_page(SHOPPING_MODE_VIEWING);
}

void open_party_inventory_screen(void) {
    shopping_ui.phase = INVENTORY_UI_ANIMATION_PHASE_SLIDE_IN;
    /* shopping_ui.phase = INVENTORY_UI_ANIMATION_PHASE_IDLE; */
    repopulate_inventory_view();
    specific_inventory_ui_state.queued_item_use_index         = -1;
    specific_inventory_ui_state.queued_party_member_use_index = -1;
    specific_inventory_ui_state.confirmation_popup_selection  = 0;
}

void inventory_remove_queued_item_usage(void) {
    specific_inventory_ui_state.queued_item_use_index = -1;
}

void user_use_inventory_item_at_index(s32 item_index) {
    struct entity_inventory* inventory              = (struct entity_inventory*)(&game_state->inventory);
    struct item_instance*    current_inventory_item = inventory->items + item_index;
    struct item_def*         item_base              = item_database_find_by_id(current_inventory_item->item);

    /* do not allow random items to be used. */
    if (item_base->type != ITEM_TYPE_CONSUMABLE_ITEM) {
        return;
    }

    /* can only be used in through the combat menu. */
    if (item_base->flags & ITEM_COMBAT_ONLY) {
        return;
    }

    if (specific_inventory_ui_state.queued_item_use_index == -1) {
        specific_inventory_ui_state.queued_item_use_index = item_index;
        _debugprintf("item index: %d", item_index);
    }
}

void close_party_inventory_screen(void) {
    reexpose_pause_menu_options();
    zero_memory(&shopping_ui, sizeof(shopping_ui));
}

local void do_inventory_use_item_popup(struct software_framebuffer* framebuffer, bool viewing_inventory) {
    if (viewing_inventory)
        return;

    bool selection_move_right   = is_action_down_with_repeat(INPUT_ACTION_MOVE_DOWN) || is_action_down_with_repeat(INPUT_ACTION_MOVE_RIGHT);
    bool selection_move_left    = is_action_down_with_repeat(INPUT_ACTION_MOVE_UP)   || is_action_down_with_repeat(INPUT_ACTION_MOVE_LEFT);
    bool selection_cancel       = is_action_down_with_repeat(INPUT_ACTION_PAUSE);
    bool selection_confirmation = is_action_pressed(INPUT_ACTION_CONFIRMATION);

    const f32 TEXT_SCALE = 2;

    struct font_cache* normal_font      = game_get_font(MENU_FONT_COLOR_WHITE);
    struct font_cache* highlighted_font = game_get_font(MENU_FONT_COLOR_GOLD);

    struct entity_inventory* inventory              = (struct entity_inventory*)(&game_state->inventory);
    struct item_instance*    current_inventory_item = inventory->items + specific_inventory_ui_state.queued_item_use_index;
    struct item_def*         item_base              = item_database_find_by_id(current_inventory_item->item);

    /* darken fader */
    {
        software_framebuffer_draw_quad(framebuffer, rectangle_f32(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT), color32u8(0, 0, 0, 100), BLEND_MODE_ALPHA);
    }

    if (specific_inventory_ui_state.queued_party_member_use_index == -1) {
        if (selection_cancel) {
            inventory_remove_queued_item_usage();
            return;
        }

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
                    /* opening up the queue menu. */
                    specific_inventory_ui_state.queued_party_member_use_index = 0;
                } break;
                case 1: {
                    specific_inventory_ui_state.queued_item_use_index = -1;
                } break;
            }
        }
    } else {
        /* would like to fade in but I don't have time for that :/ */ 
        s32   party_member_count   = game_state->party_member_count;
        v2f32 estimated_dimensions = party_member_card_dimensions_pixels();
        f32   cards_x              = SCREEN_WIDTH/2  - estimated_dimensions.x/2;
        /* magical spacing number */
        f32   y_cursor             = SCREEN_HEIGHT/2 - (estimated_dimensions.y * 1.23 * party_member_count)/2;

        s32              index = specific_inventory_ui_state.queued_item_use_index;
        struct item_def* item  = item_database_find_by_id(inventory->items[index].item);

        /* NOTE This would have to be a bit more specific to account for "KOed" party members. Who might not be selectable for certain items. */
        if (selection_move_right) {
            specific_inventory_ui_state.queued_party_member_use_index += 1;
            if (specific_inventory_ui_state.queued_party_member_use_index >= party_member_count) {
                specific_inventory_ui_state.queued_party_member_use_index = 0;
            }
        } else if (selection_move_left) {
            specific_inventory_ui_state.queued_party_member_use_index -= 1;
            if (specific_inventory_ui_state.queued_party_member_use_index < 0) {
                specific_inventory_ui_state.queued_party_member_use_index = party_member_count-1;
            }
        }

        for (s32 party_member_index = 0; party_member_index < party_member_count; ++party_member_index) {
            f32 offset = 0;
            if (party_member_index == specific_inventory_ui_state.queued_party_member_use_index) {
                offset = TILE_UNIT_SIZE;
            }
            draw_party_member_card(framebuffer, cards_x + offset, y_cursor, party_member_index);
            y_cursor += estimated_dimensions.y * 1.23;
        }

        if (selection_confirmation) {
            /* TODO: I should show an animation to indicate item usage */
            if (item->type != ITEM_TYPE_MISC) {
                _debugprintf("use item \"%.*s\"", item->name.length, item->name.data);

                struct entity* player = game_get_party_member(specific_inventory_ui_state.queued_party_member_use_index);
                entity_inventory_use_item(inventory, index, player);
                repopulate_inventory_view();
                specific_inventory_ui_state.queued_item_use_index         = -1;
                specific_inventory_ui_state.queued_party_member_use_index = -1;
            }   
        } else if (selection_cancel) {
            specific_inventory_ui_state.queued_party_member_use_index = -1;
        }
    }
}

local void update_and_render_party_inventory_screen(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    const f32 FINAL_SHOPPING_MENU_X = 20;
    struct font_cache* normal_font      = game_get_font(MENU_FONT_COLOR_WHITE);
    struct font_cache* highlighted_font = game_get_font(MENU_FONT_COLOR_GOLD);

    v2f32 shopping_menu_dimensions = estimate_shopping_menu_dimensions();

    switch (shopping_ui.phase) {
        case INVENTORY_UI_ANIMATION_PHASE_SLIDE_IN: {
            const f32 MAX_TIME = 1.6f;

            f32 t  = shopping_ui.timer / (MAX_TIME-0.6);
            f32 t2 = (shopping_ui.timer-1.0) / 0.6;
            if (t2 < 0)    t2 = 0.0;
            if (t2 >= 1.0) t2 = 1.0;
            if (t >= 1.0)  t  = 1.0;

            if (shopping_ui.timer >= MAX_TIME) {
                shop_ui_set_phase(SHOPPING_UI_ANIMATION_PHASE_IDLE);
            }

            do_shopping_menu(framebuffer, lerp_f32(-FINAL_SHOPPING_MENU_X - shopping_menu_dimensions.x, FINAL_SHOPPING_MENU_X, t), false, SHOPPING_MODE_VIEWING);
#ifdef EXPERIMENTAL_320
            software_framebuffer_draw_text(framebuffer, normal_font, 1, v2f32(10, 10), string_literal("INVENTORY"), color32f32(1,1,1,t2), BLEND_MODE_ALPHA);
#else
            software_framebuffer_draw_text(framebuffer, normal_font, 4, v2f32(10, 10), string_literal("INVENTORY"), color32f32(1,1,1,t2), BLEND_MODE_ALPHA);
#endif
            shopping_ui.timer += dt;
        } break;
        case INVENTORY_UI_ANIMATION_PHASE_IDLE: {
            do_gold_counter(framebuffer, dt);

            bool should_use_input = true;

            if (specific_inventory_ui_state.queued_item_use_index != -1) {
                should_use_input = false;
            }

            do_shopping_menu(framebuffer, FINAL_SHOPPING_MENU_X, should_use_input, SHOPPING_MODE_VIEWING);
#ifdef EXPERIMENTAL_320
            software_framebuffer_draw_text(framebuffer, normal_font, 1, v2f32(10, 10), string_literal("INVENTORY"), color32f32_WHITE, BLEND_MODE_ALPHA);
#else
            software_framebuffer_draw_text(framebuffer, normal_font, 4, v2f32(10, 10), string_literal("INVENTORY"), color32f32_WHITE, BLEND_MODE_ALPHA);
#endif
            do_inventory_use_item_popup(framebuffer, should_use_input);
        } break;
        case INVENTORY_UI_ANIMATION_PHASE_SLIDE_OUT: {
            const f32 MAX_TIME = 1.6f;

            f32 t  = shopping_ui.timer / (MAX_TIME-0.6);
            f32 t2 = (shopping_ui.timer-1.0) / 0.6;
            if (t2 < 0)    t2 = 0.0;
            if (t2 >= 1.0) t2 = 1.0;
            if (t >= 1.0)  t  = 1.0;

            if (shopping_ui.timer >= MAX_TIME) {
                close_party_inventory_screen();
                shopping_ui.timer = 0;
            }

            do_shopping_menu(framebuffer, lerp_f32(-FINAL_SHOPPING_MENU_X - shopping_menu_dimensions.x, FINAL_SHOPPING_MENU_X, (1.0 - t)), false, SHOPPING_MODE_VIEWING);
#ifdef EXPERIMENTAL_320
            software_framebuffer_draw_text(framebuffer, normal_font, 1, v2f32(10, 10), string_literal("INVENTORY"), color32f32(1,1,1,(1.0 - t2)), BLEND_MODE_ALPHA);
#else
            software_framebuffer_draw_text(framebuffer, normal_font, 4, v2f32(10, 10), string_literal("INVENTORY"), color32f32(1,1,1,(1.0 - t2)), BLEND_MODE_ALPHA);
#endif
            shopping_ui.timer += dt;
        } break;
        default: {
            _debugprintf("bad case");
        } break;;
    }
}
