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
    f32 font_scale = 3;
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

local void do_party_member_edits_or_selections(struct game_state* state, struct software_framebuffer* framebuffer, f32 x, f32 dt, bool allow_inputs) {
    f32 font_scale = 3;
    struct ui_pause_menu* menu_state = &state->ui_pause;

    if (!allow_inputs) {
        return;
    }
}

local void update_and_render_pause_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    /* needs a bit of cleanup */
    f32 font_scale = 3;
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
        item_positions[index].y = 36 * (index+0.75);
    }

    f32 offscreen_x = -240;
    f32 final_x     = 40;

    u32 blur_samples = 2;
    f32 max_blur = 1.0;
    f32 max_grayscale = 0.8;

    f32 timescale = 1.45f;

    bool selection_down    = is_action_down_with_repeat(INPUT_ACTION_MOVE_DOWN);
    bool selection_up      = is_action_down_with_repeat(INPUT_ACTION_MOVE_UP);
    bool selection_pause   = is_action_down_with_repeat(INPUT_ACTION_PAUSE);
    bool selection_confirm = is_action_down_with_repeat(INPUT_ACTION_CONFIRMATION);

    switch (menu_state->animation_state) {
        case UI_PAUSE_MENU_TRANSITION_IN: {
            menu_state->transition_t   += dt * timescale;

            for (unsigned index = 0; index < array_count(item_positions); ++index) {
                item_positions[index].x = lerp_f32(offscreen_x, final_x, menu_state->transition_t);
            }

            bool should_blur_fade = (menu_state->last_sub_menu_state == UI_PAUSE_MENU_SUB_MENU_STATE_NONE);

            do_party_member_edits_or_selections(state, framebuffer, 0, dt, false);

            if (should_blur_fade) {
                game_postprocess_blur(framebuffer, blur_samples, max_blur * (menu_state->transition_t), BLEND_MODE_ALPHA);
                game_postprocess_grayscale(framebuffer, max_grayscale * (menu_state->transition_t));
            } else {
                game_postprocess_blur(framebuffer, blur_samples, max_blur, BLEND_MODE_ALPHA);
                game_postprocess_grayscale(framebuffer, max_grayscale);
            }

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

                if (selection_confirm) {
                    if (!disabled_actions[menu_state->selection]) {
                        switch (menu_state->selection) {
                            case PAUSE_MENU_RESUME: {
                                close_pause_menu();
                            } break;
                            case PAUSE_MENU_PARTY_EQUIPMENT: {
                                menu_state->party_queued_to             = PAUSE_MENU_PARTY_EQUIPMENT;
                                menu_state->need_to_select_party_member = true;
                                pause_menu_transition_to(UI_PAUSE_MENU_SUB_MENU_STATE_EQUIPMENT);
                                open_equipment_screen(game_state->party_members[0]);
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
                                screen_mode = GAME_SCREEN_MAIN_MENU;
                            } break;
                            case PAUSE_MENU_RETURN_TO_DESKTOP: {
                                global_game_running = false;
                            } break;
                        }
                    }
                }

                do_party_member_edits_or_selections(state, framebuffer, 0, dt, false);
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

            for (unsigned index = 0; index < array_count(item_positions); ++index) {
                item_positions[index].x = lerp_f32(final_x, offscreen_x, menu_state->transition_t);
            }

            bool should_blur_fade = (menu_state->sub_menu_state == UI_PAUSE_MENU_SUB_MENU_STATE_NONE);
            do_party_member_edits_or_selections(state, framebuffer, 0, dt, false);

            if (should_blur_fade) {
                game_postprocess_blur(framebuffer, blur_samples, max_blur * (1-menu_state->transition_t), BLEND_MODE_ALPHA);
                game_postprocess_grayscale(framebuffer, max_grayscale * (1-menu_state->transition_t));
            } else {
                game_postprocess_blur(framebuffer, blur_samples, max_blur, BLEND_MODE_ALPHA);
                game_postprocess_grayscale(framebuffer, max_grayscale);
            }

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
            draw_position.y += 220;
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
