#include "pause_menu_ui_def.c"

local void open_pause_menu(void) {
    struct ui_pause_menu* ui_pause = &game_state->ui_pause;
    ui_pause->last_sub_menu_state = UI_PAUSE_MENU_SUB_MENU_STATE_NONE;
    ui_pause->animation_state = 0;
    ui_pause->transition_t    = 0;
    ui_pause->selection       = 0;
    zero_array(ui_pause->shift_t);
}

local void close_pause_menu(void) {
    struct ui_pause_menu* ui_pause = &game_state->ui_pause;
    ui_pause->animation_state      = UI_PAUSE_MENU_TRANSITION_CLOSING;
    ui_pause->transition_t         = 0;
}

local void reexpose_pause_menu_options(void) {
    struct ui_pause_menu* ui_pause = &game_state->ui_pause;
    ui_pause->animation_state      = UI_PAUSE_MENU_TRANSITION_IN;
    ui_pause->last_sub_menu_state  = ui_pause->sub_menu_state;
    ui_pause->sub_menu_state       = UI_PAUSE_MENU_SUB_MENU_STATE_NONE;
    ui_pause->transition_t         = 0;
}

local void pause_menu_transition_to(u32 submenu) {
    if (submenu == UI_PAUSE_MENU_SUB_MENU_STATE_NONE) {
        reexpose_pause_menu_options();
    } else {
        struct ui_pause_menu* ui_pause = &game_state->ui_pause;
        ui_pause->animation_state      = UI_PAUSE_MENU_TRANSITION_CLOSING;
        ui_pause->last_sub_menu_state  = UI_PAUSE_MENU_SUB_MENU_STATE_NONE;
        ui_pause->sub_menu_state       = submenu;
        ui_pause->transition_t         = 0;
    }
}

local void update_and_render_sub_menu_states(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    struct ui_pause_menu* menu_state = &state->ui_pause;

    assertion(menu_state->sub_menu_state != UI_PAUSE_MENU_SUB_MENU_STATE_NONE && "This should be impossible.");
    switch (menu_state->sub_menu_state) {
        case UI_PAUSE_MENU_SUB_MENU_STATE_OPTIONS: {
            reexpose_pause_menu_options();
        } break;
        case UI_PAUSE_MENU_SUB_MENU_STATE_EQUIPMENT: {
            /* We need to focus on the character id we are handling for future reference. */
            update_and_render_character_equipment_screen(state, framebuffer, dt);
        } break;
        case UI_PAUSE_MENU_SUB_MENU_STATE_INVENTORY: {
            update_and_render_party_inventory_screen(state, framebuffer, dt);
        } break;
    }
}

v2f32 party_member_card_dimensions_units(void) {
    return v2f32(16, 5);
}
v2f32 party_member_card_dimensions_pixels(void) {
    f32   ui_scale_factor      = 1;
    s32   CARD_WIDTH           = 8;
    s32   CARD_HEIGHT          = 3;
    v2f32 estimated_dimensions = nine_patch_estimate_extents(ui_chunky, ui_scale_factor, CARD_WIDTH, CARD_HEIGHT);

    return estimated_dimensions;
}

void draw_party_member_card(struct software_framebuffer* framebuffer, f32 x, f32 y, s32 member) {
    struct game_state* state           = game_state;
    s32                CARD_WIDTH      = 8;
    s32                CARD_HEIGHT     = 3;
    struct font_cache* font            = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_STEEL]);
    struct font_cache* font1           = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]);
    f32                font_scale      = 1;
    f32                ui_scale_factor = 1;

    draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, ui_scale_factor, v2f32(x, y), CARD_WIDTH, CARD_HEIGHT, UI_DEFAULT_COLOR);
    struct entity* entity = game_dereference_entity(state, state->party_members[member]);
    {
        struct entity_base_data* data                    = entity_database_find_by_index(&state->entity_database, entity->base_id_index);
        string                   facing_direction_string = facing_direction_strings_normal[0];
        struct entity_animation* anim                    = find_animation_by_name(data->model_index, format_temp_s("idle_%.*s", facing_direction_string.length, facing_direction_string.data));
        image_id sprite_to_use = anim->sprites[0];
        const s32 square_size = TILE_UNIT_SIZE;

        software_framebuffer_draw_quad(framebuffer, rectangle_f32(x + TILE_UNIT_SIZE, y + TILE_UNIT_SIZE*0.9, square_size, square_size+4), color32u8(0, 0, 0, 255), BLEND_MODE_ALPHA);
        software_framebuffer_draw_image_ex(framebuffer,
                                           graphics_assets_get_image_by_id(&graphics_assets, sprite_to_use),
                                           rectangle_f32(x + TILE_UNIT_SIZE, y + TILE_UNIT_SIZE*0.9, square_size, square_size+4),
                                           rectangle_f32(0, 0, 16, 20), /* This should be a little more generic but whatever for now */
                                           color32f32(1,1,1,1), NO_FLAGS, BLEND_MODE_ALPHA);
        draw_ui_breathing_text(framebuffer, v2f32(x + 2.2*TILE_UNIT_SIZE, y+TILE_UNIT_SIZE*0.33), font1, font_scale, entity->name, 341, color32f32_WHITE);
        draw_ui_breathing_text(framebuffer, v2f32(x + 2.2*TILE_UNIT_SIZE, y+TILE_UNIT_SIZE*0.65 + 22/2), font, 1, format_temp_s("LEVEL: %d", entity->stat_block.level), 341, color32f32_WHITE);
        draw_ui_breathing_text(framebuffer, v2f32(x + 2.2*TILE_UNIT_SIZE, y+TILE_UNIT_SIZE*0.65 + 22/2 + 16*1), font, 1, format_temp_s("HEALTH: %d/%d", entity->health.value, entity->health.max), 341, color32f32_WHITE);
        draw_ui_breathing_text(framebuffer, v2f32(x + 2.2*TILE_UNIT_SIZE, y+TILE_UNIT_SIZE*0.65 + 22/2 + 16*2), font, 1, format_temp_s("XP: %d", entity->stat_block.experience), 341, color32f32_WHITE);
    }
}

local void do_party_member_edits_or_selections(struct game_state* state, struct software_framebuffer* framebuffer, f32 x, f32 dt, bool allow_input) {
    struct ui_pause_menu* menu_state      = &state->ui_pause;
    struct font_cache*    font            = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_STEEL]);
    struct font_cache*    font1           = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]);
    f32                   font_scale      = 2;
    f32                   ui_scale_factor = 1;

    s32 CARD_WIDTH = 8;
    s32 CARD_HEIGHT = 3;
    v2f32 estimated_dimensions = nine_patch_estimate_extents(ui_chunky, ui_scale_factor, CARD_WIDTH, CARD_HEIGHT);
    /* NEED TO HANDLE SCROLLING LATER :) */
    f32 y_cursor = 50;

    f32 cards_x = SCREEN_WIDTH - estimated_dimensions.x * 1.1 + x;
    if (menu_state->allow_party_lineup_editing) {
        draw_ui_breathing_text(framebuffer, v2f32(0, 10), font1, font_scale, string_literal("EDITING LINEUP"), 341, color32f32_WHITE);
    } else if (menu_state->need_to_select_party_member) {
        draw_ui_breathing_text(framebuffer, v2f32(0, 10), font1, font_scale, string_literal("SELECT MEMBER"), 341, color32f32_WHITE);
    }

    for (s32 index = 0; index < state->party_member_count; ++index) {
        /* I want to lerp this too but don't have time today */
        f32 offset = 0;
        if (allow_input) {
            if (menu_state->party_member_index == index) {
                offset = TILE_UNIT_SIZE;
            }
        }
        draw_party_member_card(framebuffer, cards_x + offset, y_cursor, index);
        y_cursor += estimated_dimensions.y * 1.5;
    }

    if (!allow_input) {
        return;
    } else {
        bool selection_down    = is_action_down_with_repeat(INPUT_ACTION_MOVE_DOWN);
        bool selection_up      = is_action_down_with_repeat(INPUT_ACTION_MOVE_UP);
        bool selection_pause   = is_action_down_with_repeat(INPUT_ACTION_PAUSE);
        bool selection_cancel  = is_action_down_with_repeat(INPUT_ACTION_CANCEL);
        bool selection_confirm = is_action_down_with_repeat(INPUT_ACTION_CONFIRMATION);

        if (selection_down) {
            s32 next_index = menu_state->party_member_index+1;
            if (next_index >= state->party_member_count) {
                next_index = 0;
            }
            if (menu_state->allow_party_lineup_editing) {
                game_swap_party_member_index(menu_state->party_member_index, next_index);
            }
            menu_state->party_member_index = next_index;
            play_sound(ui_blip);
        } else if (selection_up) {
            s32 next_index = menu_state->party_member_index-1;
            if (next_index < 0) {
                next_index = state->party_member_count-1;
            }
            if (menu_state->allow_party_lineup_editing) {
                game_swap_party_member_index(menu_state->party_member_index, next_index);
            }
            menu_state->party_member_index = next_index;
            play_sound(ui_blip);
        }

        if (menu_state->need_to_select_party_member) {
            if (selection_confirm) {
                menu_state->need_to_select_party_member = false;
                switch (menu_state->party_queued_to) {
                    case PAUSE_MENU_RESUME: {
                        close_pause_menu();
                    } break;
                    case PAUSE_MENU_PARTY_EQUIPMENT: {
                        pause_menu_transition_to(UI_PAUSE_MENU_SUB_MENU_STATE_EQUIPMENT);
                        open_equipment_screen(game_state->party_members[menu_state->party_member_index]);
                    } break;
                    case PAUSE_MENU_PARTY_ITEMS: {
                        pause_menu_transition_to(UI_PAUSE_MENU_SUB_MENU_STATE_INVENTORY);
                        open_party_inventory_screen();
                    } break;
                    case PAUSE_MENU_OPTIONS: {
                        pause_menu_transition_to(UI_PAUSE_MENU_SUB_MENU_STATE_OPTIONS);
                    } break;
                    case PAUSE_MENU_QUIT: {
                        initialize_main_menu();
                        set_game_screen_mode(GAME_SCREEN_MAIN_MENU);
                    } break;
                    case PAUSE_MENU_RETURN_TO_DESKTOP: {
                        global_game_running = false;
                    } break;
                }
            }

            if (selection_pause || selection_cancel) {
                menu_state->need_to_select_party_member = false;
            }
        } else if (menu_state->editing_party_lineup) {
            if (selection_pause || selection_cancel) {
                if (menu_state->allow_party_lineup_editing) {
                    menu_state->allow_party_lineup_editing = false;
                } else {
                    menu_state->editing_party_lineup = false;
                }
            }

            if (selection_confirm) {
                menu_state->allow_party_lineup_editing ^= true;
            }
        }
    }
}

local void update_and_render_pause_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    /* needs a bit of cleanup */
    f32 font_scale = 1;
    struct ui_pause_menu* menu_state = &state->ui_pause;
    v2f32 item_positions[array_count(ui_pause_menu_strings)] = {};

    bool disabled_actions[array_count(ui_pause_menu_strings)] = {};
    {
        if (game_state->combat_state.active_combat) {
            disabled_actions[PAUSE_MENU_PARTY_ITEMS]     = true;
            disabled_actions[PAUSE_MENU_PARTY_EQUIPMENT] = true;
        }
    }

    for (unsigned index = 0; index < array_count(item_positions); ++index) {
        item_positions[index].y = 18 * (index+0.75);
    }

    f32 offscreen_x = -240;
    f32 final_x     = 20;

    u32 blur_samples = 2;
    f32 max_blur = 1.0;
    f32 max_grayscale = 0.8;

    f32 timescale = 1.45f;

    bool selection_down    = is_action_down_with_repeat(INPUT_ACTION_MOVE_DOWN);
    bool selection_up      = is_action_down_with_repeat(INPUT_ACTION_MOVE_UP);
    bool selection_right   = is_action_down_with_repeat(INPUT_ACTION_MOVE_RIGHT);
    bool selection_pause   = is_action_down_with_repeat(INPUT_ACTION_PAUSE);
    bool selection_confirm = is_action_down_with_repeat(INPUT_ACTION_CONFIRMATION);

    if (menu_state->need_to_select_party_member || menu_state->editing_party_lineup) {
        selection_confirm = selection_pause = selection_up = selection_down = false;
    }

    switch (menu_state->animation_state) {
        case UI_PAUSE_MENU_TRANSITION_IN: {
            menu_state->transition_t   += dt * timescale;

            f32 effective_t = menu_state->transition_t;
            if (effective_t > 1) effective_t = 1;
            if (effective_t < 0) effective_t = 0;

            for (unsigned index = 0; index < array_count(item_positions); ++index) {
                item_positions[index].x = lerp_f32(offscreen_x, final_x, effective_t);
            }

            bool should_blur_fade = (menu_state->last_sub_menu_state == UI_PAUSE_MENU_SUB_MENU_STATE_NONE);

            if (should_blur_fade) {
                game_postprocess_blur(framebuffer, blur_samples, max_blur * (effective_t), BLEND_MODE_ALPHA);
                game_postprocess_grayscale(framebuffer, max_grayscale * (effective_t));
            } else {
                game_postprocess_blur(framebuffer, blur_samples, max_blur, BLEND_MODE_ALPHA);
                game_postprocess_grayscale(framebuffer, max_grayscale);
            }

            do_party_member_edits_or_selections(state, framebuffer, lerp_f32(400, 0, effective_t), dt, false);

            if (menu_state->transition_t >= 1.0f) {
                menu_state->animation_state += 1;
                menu_state->transition_t = 0;
            }
        } break;
        case UI_PAUSE_MENU_NO_ANIM: {
            game_postprocess_blur(framebuffer, blur_samples, max_blur, BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale);

            if (menu_state->sub_menu_state == UI_PAUSE_MENU_SUB_MENU_STATE_NONE) {
                for (unsigned index = 0; index < array_count(item_positions); ++index) {
                    item_positions[index].x = final_x;
                }

                for (unsigned index = 0; index < array_count(item_positions); ++index) {
                    if (index != menu_state->selection) {
                        menu_state->shift_t[index] -= dt*4;
                    }
                }
                menu_state->shift_t[menu_state->selection] += dt*4;
                for (unsigned index = 0; index < array_count(item_positions); ++index) {
                    menu_state->shift_t[index] = clamp_f32(menu_state->shift_t[index], 0, 1);
                }

                if ((selection_pause)) {
                    close_pause_menu();
                }        

                if (selection_up) {
                    do {
                        menu_state->selection -= 1;
                        if (menu_state->selection < 0) {
                            menu_state->selection = array_count(item_positions)-1;
                        }
                    } while (disabled_actions[menu_state->selection]);
                    play_sound(ui_blip);
                } else if (selection_down) {
                    do {
                        menu_state->selection += 1;
                        if (menu_state->selection >= array_count(item_positions)) {
                            menu_state->selection = 0;
                        }
                    } while (disabled_actions[menu_state->selection]);
                    play_sound(ui_blip);
                }

                do_party_member_edits_or_selections(state, framebuffer, 0, dt, menu_state->editing_party_lineup || menu_state->need_to_select_party_member);
                if (selection_right) {
                    menu_state->editing_party_lineup       = true;
                    menu_state->allow_party_lineup_editing = false;
                    menu_state->party_member_index         = 0;
                } else if (selection_confirm) {
                    if (!disabled_actions[menu_state->selection]) {
                        switch (menu_state->selection) {
                            case PAUSE_MENU_RESUME: {
                                close_pause_menu();
                            } break;
                            case PAUSE_MENU_PARTY_EQUIPMENT: {
                                menu_state->party_queued_to             = PAUSE_MENU_PARTY_EQUIPMENT;
                                menu_state->need_to_select_party_member = true;
                                menu_state->party_member_index          = 0;
                            } break;
                            case PAUSE_MENU_PARTY_ITEMS: {
                                pause_menu_transition_to(UI_PAUSE_MENU_SUB_MENU_STATE_INVENTORY);
                                open_party_inventory_screen();
                            } break;
                            case PAUSE_MENU_OPTIONS: {
                                pause_menu_transition_to(UI_PAUSE_MENU_SUB_MENU_STATE_OPTIONS);
                            } break;
                            case PAUSE_MENU_QUIT: {
                                initialize_main_menu();
                                set_game_screen_mode(GAME_SCREEN_MAIN_MENU);
                            } break;
                            case PAUSE_MENU_RETURN_TO_DESKTOP: {
                                global_game_running = false;
                            } break;
                        }
                    }
                }

            } else {
                for (unsigned index = 0; index < array_count(item_positions); ++index) {
                    item_positions[index].x = -99999999;
                }

                /* sub menus have their own state. I'm just nesting these. Which is fine since each menu unit is technically "atomic" */
                update_and_render_sub_menu_states(state, framebuffer, dt);
            }
        } break;
        case UI_PAUSE_MENU_TRANSITION_CLOSING: {
            menu_state->transition_t   += dt * timescale;

            f32 effective_t = menu_state->transition_t;
            if (effective_t > 1) effective_t = 1;
            if (effective_t < 0) effective_t = 0;

            for (unsigned index = 0; index < array_count(item_positions); ++index) {
                item_positions[index].x = lerp_f32(final_x, offscreen_x, effective_t);
            }

            bool should_blur_fade = (menu_state->sub_menu_state == UI_PAUSE_MENU_SUB_MENU_STATE_NONE);

            if (should_blur_fade) {
                game_postprocess_blur(framebuffer, blur_samples, max_blur * (1-effective_t), BLEND_MODE_ALPHA);
                game_postprocess_grayscale(framebuffer, max_grayscale * (1-effective_t));
            } else {
                game_postprocess_blur(framebuffer, blur_samples, max_blur, BLEND_MODE_ALPHA);
                game_postprocess_grayscale(framebuffer, max_grayscale);
            }

            do_party_member_edits_or_selections(state, framebuffer, lerp_f32(0, 400, effective_t), dt, false);

            if (menu_state->transition_t >= 1.0f) {
                menu_state->transition_t = 0;
                switch (menu_state->sub_menu_state) {
                    case UI_PAUSE_MENU_SUB_MENU_STATE_NONE: {
                        game_state_set_ui_state(game_state, state->last_ui_state);
                    } break;
                        /* 
                           I don't really think any other UI state requires other special casing behavior
                           just set to NO ANIM and just don't display the normal menu choices... Basically
                           just hand off control to our friends.
                        */
                    default: {
                        menu_state->animation_state = UI_PAUSE_MENU_NO_ANIM;
                    } break;
                }
            }
        } break;
    }

    {
        for (unsigned index = 0; index < array_count(item_positions); ++index) {
            v2f32 draw_position = item_positions[index];
            draw_position.x += lerp_f32(0, 20, menu_state->shift_t[index]);
            draw_position.y += 220/2;
            /* custom string drawing routine */
            struct font_cache* font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_STEEL]);
            if (index == menu_state->selection) {
                font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]);
            }

            union color32f32 color = color32f32_WHITE;
            if (disabled_actions[index]) {
                color.a = 0.5;
            }
            draw_ui_breathing_text(framebuffer, draw_position, font, font_scale, ui_pause_menu_strings[index], index, color);
        }
    }
}
