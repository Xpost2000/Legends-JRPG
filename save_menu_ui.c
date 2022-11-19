#include "save_menu_ui_def.c"

enum save_menu_phase {
    SAVE_MENU_OFF,
    SAVE_MENU_DROP_DOWN,
    SAVE_MENU_CANCEL,
    SAVE_MENU_IDLE,
};

struct save_slot_widget {
    /* should have some save info */
    /* maybe a bitmap? */
    char name[SAVE_SLOT_WIDGET_SAVE_NAME_LENGTH];
    /*
      mostly Act description probably, this is manually encoded when you make the save game,
      determined elsewhere.
    */
    char descriptor[SAVE_SLOT_WIDGET_DESCRIPTOR_LENGTH];
    /* game timestamp??? */
    s64 unix_timestamp;

    /* same system used for the other stuff */
    f32 lean_in_t;

    /* smoothly seek over to the current save slot to view */
};

enum save_menu_intent {
    SAVE_MENU_INTENT_LOADING,
    SAVE_MENU_INTENT_SAVING,
};

struct save_menu_state {
    u8                      intent;
    s32                     phase;
    s32                     currently_selected_option_choice;
    f32                     scroll_seek_y;
    f32                     timer;
    struct save_slot_widget save_slot_widgets[GAME_MAX_SAVE_SLOTS];
} global_save_menu_state;

local f32 estimate_save_menu_height(void) {
    s32 BOX_WIDTH  = 20;
    s32 BOX_HEIGHT = 8;

    v2f32 nine_patch_extents = nine_patch_estimate_extents(ui_chunky, 1, BOX_WIDTH, BOX_HEIGHT);
    f32 y_cursor = 0;
    y_cursor += nine_patch_extents.y * 1.5 * (array_count(global_save_menu_state.save_slot_widgets)+1);
    return y_cursor;
}

local void fill_all_save_slots(void) {
    /* should be loading from save files, right now don't have that tho */
    /* the save header should be filled with this information so we can just quickly read and copy the display information */
    /* without having to hold on to it in memory. */

    for (s32 save_slot_index = 0; save_slot_index < array_count(global_save_menu_state.save_slot_widgets); ++save_slot_index) {
        string filename_path = filename_from_saveslot_id(save_slot_index);

        struct save_slot_widget* current_save_slot = &global_save_menu_state.save_slot_widgets[save_slot_index];
        if (file_exists(filename_path)) {
            struct save_data_description save_description = get_save_data_description(save_slot_index);

            assertion(save_description.good && "Hmm, corrupted save file? Or doesn't exist?");
            cstring_copy(save_description.name, current_save_slot->name, array_count(current_save_slot->name));
            cstring_copy(save_description.descriptor, current_save_slot->descriptor, array_count(current_save_slot->descriptor));
            current_save_slot->unix_timestamp = save_description.timestamp;
            _debugprintf("Timestamp is %lld", save_description.timestamp);
        } else {
            cstring_copy("NO SAVE", current_save_slot->name, array_count(current_save_slot->name));
            cstring_copy("-------", current_save_slot->descriptor, array_count(current_save_slot->descriptor));
        }

        current_save_slot->lean_in_t = 0;
    }
}

local void save_menu_open_common(void) {
    fill_all_save_slots();
    global_save_menu_state.phase                            = SAVE_MENU_DROP_DOWN;
    global_save_menu_state.currently_selected_option_choice = 0;
    global_save_menu_state.scroll_seek_y                    = 0;
    global_save_menu_state.timer                            = 0;
}

void save_menu_open_for_loading(void) {
    save_menu_open_common();
    global_save_menu_state.intent = SAVE_MENU_INTENT_LOADING;
}

void save_menu_open_for_saving(void) {
    save_menu_open_common();
    global_save_menu_state.intent = SAVE_MENU_INTENT_SAVING;
}

void save_menu_close(void) {
    global_save_menu_state.phase = SAVE_MENU_CANCEL;
    global_save_menu_state.timer = 0;
}

local s32 _do_save_menu_core(struct software_framebuffer* framebuffer, f32 y_offset, f32 dt, bool allow_input) {
    union color32f32 ui_color = UI_DEFAULT_COLOR;
    f32              alpha    = 1;

    if (!allow_input) alpha = 0.5;
    ui_color.a              = alpha;

    bool selection_move_up   = is_action_down_with_repeat(INPUT_ACTION_MOVE_UP);
    bool selection_move_down = is_action_down_with_repeat(INPUT_ACTION_MOVE_DOWN);
    bool selection_cancel    = is_action_pressed(INPUT_ACTION_CANCEL);
    bool selection_confirm   = is_action_pressed(INPUT_ACTION_CONFIRMATION);

    f32 y_cursor = y_offset+25;

    if (!allow_input) {
        selection_confirm = selection_cancel = selection_move_down = selection_move_up = false;
    }

    /* I
       DEALLY we don't have to scroll for any save slots, they should all fit on one screen, but I might as well
       allow scrolling so we'll have lots of saveslots just so I know how to do it for the inventory since that
       doesn't happen yet if you can believe it.
    */
    const f32 MAX_T_FOR_SLOT_LEAN = 0.10;

    struct font_cache* title_font = game_get_font(MENU_FONT_COLOR_STEEL);
    struct font_cache* body_font  = game_get_font(MENU_FONT_COLOR_WHITE);

    f32 start_y_cursor = y_cursor;

    s32 BOX_WIDTH  = 20;
    s32 BOX_HEIGHT = 8;

    v2f32 nine_patch_extents = nine_patch_estimate_extents(ui_chunky, 1, BOX_WIDTH, BOX_HEIGHT);

    for (s32 save_slot_index = 0; save_slot_index < array_count(global_save_menu_state.save_slot_widgets); ++save_slot_index) {
        struct save_slot_widget* current_slot = global_save_menu_state.save_slot_widgets + save_slot_index;

        f32 effective_slot_t = current_slot->lean_in_t / MAX_T_FOR_SLOT_LEAN;

        if (effective_slot_t > 1) effective_slot_t      = 1;
        else if (effective_slot_t < 0) effective_slot_t = 0;

        if (selection_confirm) {
            return save_slot_index;
        }

        if (global_save_menu_state.currently_selected_option_choice == save_slot_index) {
            if (allow_input) {
                current_slot->lean_in_t += dt;
                if (current_slot->lean_in_t > MAX_T_FOR_SLOT_LEAN) current_slot->lean_in_t = MAX_T_FOR_SLOT_LEAN;
            }

            { /* seek smoothly */
                f32 relative_target = (start_y_cursor - y_cursor) + (SCREEN_HEIGHT/2-nine_patch_extents.y);
                f32 relative_distance = fabs(global_save_menu_state.scroll_seek_y - (relative_target));

                if (relative_distance > 1.512838f) {
                    /* smoothen out */
                    const s32 STEPS = 10;
                    for (s32 iters = 0; iters < STEPS; ++iters) {
                        f32 sign_direction = 0;
                        if (global_save_menu_state.scroll_seek_y < relative_target) sign_direction = 1;
                        else                                           sign_direction = -1;

                        relative_distance = fabs(global_save_menu_state.scroll_seek_y - (relative_target));

                        if (relative_distance > 1.512838f) {
                            global_save_menu_state.scroll_seek_y += dt * 100 * sign_direction;
                        } else {
                            break;
                        }
                    }
                }
            }
        } else {
            current_slot->lean_in_t -= dt;
            if (current_slot->lean_in_t < 0) current_slot->lean_in_t = 0;
        }


        f32 x_cursor = 50 + lerp_f32(0, 50, effective_slot_t);


        f32 adjusted_scroll_offset = global_save_menu_state.scroll_seek_y;

        draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, v2f32(x_cursor, y_cursor + adjusted_scroll_offset), BOX_WIDTH, BOX_HEIGHT, ui_color);
        draw_ui_breathing_text(framebuffer, v2f32(x_cursor + 15, y_cursor + 15 + adjusted_scroll_offset), title_font, 2, format_temp_s("%s (%02d)", current_slot->name, save_slot_index), save_slot_index*22, color32f32(1, 1, 1, alpha));

        if (current_slot->unix_timestamp != 0) {
            struct calendar_time calendar_time_info =
                calendar_time_from(current_slot->unix_timestamp);    
            software_framebuffer_draw_text(framebuffer, body_font, 2, v2f32(x_cursor + 20, y_cursor + 15+32 + adjusted_scroll_offset), format_temp_s("%s-%d-%d", month_strings[calendar_time_info.month], calendar_time_info.day, calendar_time_info.year), color32f32(1, 1, 1, alpha), BLEND_MODE_ALPHA);
        }

        /* need to have good word wrap */
        software_framebuffer_draw_text(framebuffer, body_font, 2, v2f32(x_cursor + 20, y_cursor + 24+15+32 + adjusted_scroll_offset), string_from_cstring(current_slot->descriptor), color32f32(1, 1, 1, alpha), BLEND_MODE_ALPHA);

        y_cursor += nine_patch_extents.y * 1.5;
    }

    if (selection_move_down) {
        global_save_menu_state.currently_selected_option_choice += 1;
        if (global_save_menu_state.currently_selected_option_choice >= array_count(global_save_menu_state.save_slot_widgets)) {
            global_save_menu_state.currently_selected_option_choice = 0;
        }
    }
    if (selection_move_up) {
        global_save_menu_state.currently_selected_option_choice -= 1;
        if (global_save_menu_state.currently_selected_option_choice < 0) {
            global_save_menu_state.currently_selected_option_choice = array_count(global_save_menu_state.save_slot_widgets) - 1;
        }
    }

    if (selection_cancel) {
        save_menu_close();
    }

    return -1;
}

s32 do_save_menu(struct software_framebuffer* framebuffer, f32 dt) {
    switch (global_save_menu_state.phase) {
        case SAVE_MENU_OFF: {
            return SAVE_MENU_PROCESS_ID_EXIT;
        } break;

        case SAVE_MENU_DROP_DOWN: {
            const f32 MAX_T = 2.15f;
            f32 effective_t = global_save_menu_state.timer/(MAX_T-0.1);

            if (effective_t > 1)      effective_t = 1;
            else if (effective_t < 0) effective_t = 0;

            f32 height_of_saves = estimate_save_menu_height();
            f32 y_offset = lerp_f32(-height_of_saves, 0, effective_t);
            _do_save_menu_core(framebuffer, y_offset, dt, false);

            if (global_save_menu_state.timer >= MAX_T) {
                global_save_menu_state.timer = 0;
                global_save_menu_state.phase = SAVE_MENU_IDLE;
            }

            global_save_menu_state.timer += dt;
        } break;
        case SAVE_MENU_CANCEL: {
            const f32 MAX_T = 1.25;
            f32 effective_t = global_save_menu_state.timer/(MAX_T-0.1);

            if (effective_t > 1)      effective_t = 1;
            else if (effective_t < 0) effective_t = 0;

            f32 height_of_saves = estimate_save_menu_height();
            f32 y_offset = lerp_f32(global_save_menu_state.scroll_seek_y, -height_of_saves, effective_t);
            _do_save_menu_core(framebuffer, y_offset, dt, false);

            if (global_save_menu_state.timer >= MAX_T) {
                save_menu_close();
            }

            global_save_menu_state.timer += dt;
        } break;
        case SAVE_MENU_IDLE: {
            s32 selected_slot = _do_save_menu_core(framebuffer, 0, dt, true);

            if (selected_slot != -1) {
                switch (global_save_menu_state.intent) {
                    case SAVE_MENU_INTENT_SAVING: {
                        game_load_from_save_slot(selected_slot);
                        /* TODO: Need to have more fancy UI regarding saving! */
                        return SAVE_MENU_PROCESS_ID_SAVED_EXIT;
                    } break;
                    case SAVE_MENU_INTENT_LOADING: {
                        /* load slot and start the game */
                        screen_mode = GAME_SCREEN_INGAME;
                        fade_into_game();
                        game_load_from_save_slot(selected_slot);
                        return SAVE_MENU_PROCESS_ID_LOADED_EXIT;
                    } break;
                }
            }
        } break;
    }

    return SAVE_MENU_PROCESS_ID_OKAY;
}
