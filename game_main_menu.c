/* global main menu code */

/*
  NOTE: there is a major disadvantage and that is the fact most of the UI code is not reusable right now,
  (IE: the options menu isn't usable in the main game pause menu yet. That can change soon though.)
*/
#define GAME_MAX_SAVE_SLOTS (16)

enum main_menu_animation_phase {
    MAIN_MENU_LIGHTNING_FLASHES,
    MAIN_MENU_TITLE_APPEAR,
    MAIN_MENU_OPTIONS_APPEAR,
    MAIN_MENU_IDLE,

    MAIN_MENU_SHOW_OPTIONS_PAGE_MOVE_AWAY_NORMAL_OPTIONS,
    MAIN_MENU_SHOW_OPTIONS_PAGE_MOVE_IN_OPTIONS,
    MAIN_MENU_OPTIONS_PAGE_MOVE_OUT,
    MAIN_MENU_OPTIONS_PAGE_IDLE,

    MAIN_MENU_SAVE_MENU_DROP_DOWN,
    MAIN_MENU_SAVE_MENU_CANCEL,
    MAIN_MENU_SAVE_MENU_IDLE,
};

/* All option menu code is also placed here. Along with the special *IMGUI* code lol */
#define SAVE_SLOT_WIDGET_SAVE_NAME_LENGTH  (32)
#define SAVE_SLOT_WIDGET_DESCRIPTOR_LENGTH (64)
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
#if 0
    u64 unix_timestamp;
#endif

    /* same system used for the other stuff */
    f32 lean_in_t;

    /* smoothly seek over to the current save slot to view */
};
struct {
    s32 phase;

    s32 lightning_flash_count;
    f32 lightning_delay_timer;
    s32 lightning_flashing   ;

    f32 timer;
    struct random_state rnd;

    s32 currently_selected_option_choice;
    struct save_slot_widget save_slot_widgets[GAME_MAX_SAVE_SLOTS];
    f32 scroll_seek_y;
} main_menu;

local void fill_all_save_slots(void) {
    /* should be loading from save files, right now don't have that tho */
    /* the save header should be filled with this information so we can just quickly read and copy the display information */
    /* without having to hold on to it in memory. */

    for (s32 save_slot_index = 0; save_slot_index < array_count(main_menu.save_slot_widgets); ++save_slot_index) {
        string filename_path = format_temp_s("./saves/sav%d.sav", save_slot_index);

        struct save_slot_widget* current_save_slot = &main_menu.save_slot_widgets[save_slot_index];
        if (file_exists(filename_path)) {
            cstring_copy("NEED TO LOAD", current_save_slot->name, array_count(current_save_slot->name));
            cstring_copy("-------", current_save_slot->descriptor, array_count(current_save_slot->descriptor));
        } else {
            cstring_copy("NO SAVE", current_save_slot->name, array_count(current_save_slot->name));
            cstring_copy("-------", current_save_slot->descriptor, array_count(current_save_slot->descriptor));
        }

        current_save_slot->lean_in_t = 0;
    }
}

enum {
    MAIN_MENU_OPTION_CHOICE,
    MAIN_MENU_OPTION_SLIDER,
    MAIN_MENU_OPTION_CHECKBOX,
    MAIN_MENU_OPTION_SELECTOR,
};
struct main_menu_option {
    char* choice;
    s32    type;
    s32    selected_choice;
    union {
        struct {
            f32 slider_min;
            f32 slider_max;
        };
        bool   value;
        char*  selections[128];
    };
};

local struct main_menu_option main_menu_first_page_options[] = {
    (struct main_menu_option) { .choice = ("continue") },
    (struct main_menu_option) { .choice = ("new game") },
    (struct main_menu_option) { .choice = ("load game") },
    (struct main_menu_option) { .choice = ("options") },
    (struct main_menu_option) { .choice = ("exit") },
};

struct option_resolution_pair {
    s32 w;
    s32 h;
};

struct option_resolution_pair supported_resolutions[] = {
    (struct option_resolution_pair){ 640, 480 },
    (struct option_resolution_pair){ 1280, 720 },
    (struct option_resolution_pair){ 1920, 1080 },
};

local struct main_menu_option main_menu_options_page_options[] = {
    (struct main_menu_option) {
        .choice = ("resolution"),
        .type = MAIN_MENU_OPTION_SELECTOR,
        /* I should quey this later but for now this is the lazy option. */
        .selections[0] = "640x480",
        .selections[1] = "1280x720",
        .selections[2] = "1920x1080",
    },
    (struct main_menu_option) { .choice = ("fullscreen"), .type = MAIN_MENU_OPTION_CHECKBOX },
    (struct main_menu_option) { .choice = ("back"), .type = MAIN_MENU_OPTION_CHOICE },
};

local void main_menu_next_lightning_time(void) {
    if (main_menu.lightning_flashing == true) {
        main_menu.lightning_delay_timer = random_float(&main_menu.rnd) * 0.05 + 0.07;
    } else {
        main_menu.lightning_delay_timer = random_float(&main_menu.rnd) * 0.95 + 0.1;
    }
}
local void initialize_main_menu(void) {
    main_menu.rnd                   = random_state();
    main_menu_next_lightning_time();
    return;
}

local s32 main_menu_do_menu_ui(v2f32 where, struct software_framebuffer* framebuffer, struct main_menu_option* options, s32 count, s32* option_ptr, bool allow_input) {
    const f32 TITLE_FONT_SCALE  = 6.0;
    const f32 NORMAL_FONT_SCALE = 4.0;
    bool up                = is_action_down_with_repeat(INPUT_ACTION_MOVE_UP);
    bool down              = is_action_down_with_repeat(INPUT_ACTION_MOVE_DOWN);
    bool left              = is_action_down_with_repeat(INPUT_ACTION_MOVE_LEFT);
    bool right             = is_action_down_with_repeat(INPUT_ACTION_MOVE_RIGHT);
    bool confirm_selection = is_action_pressed(INPUT_ACTION_CONFIRMATION);

    if (!allow_input) {
        up = down = left = right = confirm_selection = false;
    }

    struct font_cache* font1       = game_get_font(MENU_FONT_COLOR_STEEL);
    struct font_cache* font2       = game_get_font(MENU_FONT_COLOR_ORANGE);
    f32                font_height = font_cache_text_height(font1);

    s32 index    = 0;
    f32 y_cursor = where.y;
    Array_For_Each(it, struct main_menu_option, options, count) {
        struct font_cache* painting_font = font1;
        if (index == (*option_ptr)) {
            painting_font = font2;
        }

        if (it->type == MAIN_MENU_OPTION_SELECTOR) {
            string selected_choice = string_from_cstring(it->selections[it->selected_choice]);
            draw_ui_breathing_text_centered(framebuffer, rectangle_f32(where.x, y_cursor, framebuffer->width, framebuffer->height*0.1), painting_font, NORMAL_FONT_SCALE, string_from_cstring(format_temp("%s : <%s>", it->choice, selected_choice.data)), index, color32f32_WHITE);

            s32 highest_choice = 0;
            for (; highest_choice < array_count(it->selections); ++highest_choice) {
                if (it->selections[highest_choice] == NULL) {
                    break;
                }
            }

            if (left) {
                it->selected_choice -= 1;

                if (it->selected_choice < 0) {
                    it->selected_choice = highest_choice - 1;
                }
            } else if (right) {
                it->selected_choice += 1;

                if (it->selected_choice >= highest_choice) {
                    it->selected_choice = 0;
                }
            }

        } else {
            draw_ui_breathing_text_centered(framebuffer, rectangle_f32(where.x, y_cursor, framebuffer->width, framebuffer->height*0.1), painting_font, NORMAL_FONT_SCALE, string_from_cstring(it->choice), index, color32f32_WHITE);
        }
        y_cursor += font_height * (NORMAL_FONT_SCALE + 0.2);
        index++;
    }

    if (up) {
        (*option_ptr) -= 1;
        play_sound(ui_blip);
    }

    if (down) {
        (*option_ptr) += 1;
        play_sound(ui_blip);
    }

    if ((*option_ptr) < 0) {
        *option_ptr = count-1;
    } else if ((*option_ptr) >= count) {
        *option_ptr = 0;
    }

    if (confirm_selection) {
        return *option_ptr;
    }

    return -1;
}

local s32 do_save_menu(struct software_framebuffer* framebuffer, f32 y_offset, f32 dt, bool allow_input) {
    union color32f32 ui_color = UI_DEFAULT_COLOR;
    f32 alpha = 1;

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

    for (s32 save_slot_index = 0; save_slot_index < array_count(main_menu.save_slot_widgets); ++save_slot_index) {
        struct save_slot_widget* current_slot = main_menu.save_slot_widgets + save_slot_index;

        f32 effective_slot_t = current_slot->lean_in_t / MAX_T_FOR_SLOT_LEAN;

        if (effective_slot_t > 1) effective_slot_t      = 1;
        else if (effective_slot_t < 0) effective_slot_t = 0;

        if (selection_confirm) {
            return save_slot_index;
        }

        if (main_menu.currently_selected_option_choice == save_slot_index) {
            if (allow_input) {
                current_slot->lean_in_t += dt;
                if (current_slot->lean_in_t > MAX_T_FOR_SLOT_LEAN) current_slot->lean_in_t = MAX_T_FOR_SLOT_LEAN;
            }

            { /* seek smoothly */
                f32 relative_target = (start_y_cursor - y_cursor) + (SCREEN_HEIGHT/2-nine_patch_extents.y);
                f32 relative_distance = fabs(main_menu.scroll_seek_y - (relative_target));

                if (relative_distance > 1.512838f) {

                    /* smoothen out */
                    const s32 STEPS = 10;
                    for (s32 iters = 0; iters < STEPS; ++iters) {
                        f32 sign_direction = 0;
                        if (main_menu.scroll_seek_y < relative_target) sign_direction = 1;
                        else                                           sign_direction = -1;

                        f32 displacement = (relative_target - main_menu.scroll_seek_y) * 45;
                        f32 attenuation  = 1 - (1 / (1+(displacement*displacement)));

                        if (relative_distance > 1.512838f) {
                            main_menu.scroll_seek_y += dt * attenuation*50 * sign_direction;
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


        f32 adjusted_scroll_offset = main_menu.scroll_seek_y;

        draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, v2f32(x_cursor, y_cursor + adjusted_scroll_offset), BOX_WIDTH, BOX_HEIGHT, ui_color);
        draw_ui_breathing_text(framebuffer, v2f32(x_cursor + 15, y_cursor + 15 + adjusted_scroll_offset), title_font, 2, format_temp_s("%s (%02d)", current_slot->name, save_slot_index), save_slot_index*22, color32f32(1, 1, 1, alpha));
        software_framebuffer_draw_text(framebuffer, body_font, 1, v2f32(x_cursor + 20, y_cursor + 15+32 + adjusted_scroll_offset), string_from_cstring(current_slot->descriptor), color32f32(1, 1, 1, alpha), BLEND_MODE_ALPHA);

        y_cursor += nine_patch_extents.y * 1.5;
    }

    if (selection_move_down) {
        main_menu.currently_selected_option_choice += 1;
        if (main_menu.currently_selected_option_choice >= array_count(main_menu.save_slot_widgets)) {
            main_menu.currently_selected_option_choice = 0;
        }
    }
    if (selection_move_up) {
        main_menu.currently_selected_option_choice -= 1;
        if (main_menu.currently_selected_option_choice < 0) {
            main_menu.currently_selected_option_choice = array_count(main_menu.save_slot_widgets) - 1;
        }
    }

    if (selection_cancel) {
        main_menu.phase = MAIN_MENU_SAVE_MENU_CANCEL;
        main_menu.timer = 0;
    }

    return -1;
}

local void update_and_render_main_menu(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    const f32 TITLE_FONT_SCALE  = 6.0;
    const f32 NORMAL_FONT_SCALE = 4.0;

    struct font_cache* font  = game_get_font(MENU_FONT_COLOR_GOLD);
    struct font_cache* font1 = game_get_font(MENU_FONT_COLOR_STEEL);

    software_framebuffer_clear_buffer(framebuffer, color32u8(0,0,0,255));

    if (any_key_down() || controller_any_button_down(get_gamepad(0))) {
        if (main_menu.phase < MAIN_MENU_IDLE) {
            main_menu.phase = MAIN_MENU_IDLE;
        }
    }

    f32 font_height = font_cache_text_height(font1);
    switch (main_menu.phase) {
        case MAIN_MENU_LIGHTNING_FLASHES: {
            main_menu.lightning_delay_timer -= dt;

            if (main_menu.lightning_delay_timer <= 0.0) {
                main_menu.lightning_flashing ^= true;

                if (main_menu.lightning_flashing == true)
                    main_menu.lightning_flash_count++;

                main_menu_next_lightning_time();
            }

            if (main_menu.lightning_flashing) {
                software_framebuffer_draw_quad(framebuffer, rectangle_f32(0,0,framebuffer->width,framebuffer->height), color32u8(255,255,255,255), BLEND_MODE_NONE);
            }

            if (main_menu.lightning_flash_count > 4) {
                main_menu.phase = MAIN_MENU_TITLE_APPEAR;
                main_menu.timer = 0;
            }
        } break;

        case MAIN_MENU_TITLE_APPEAR: {
            f32 t        = main_menu.timer / 4.0f;

            if (t >= 1.0) t = 1.0;
            f32 y_cursor = lerp_f32(-999, 100, t);

            software_framebuffer_draw_text_bounds_centered(framebuffer, font, TITLE_FONT_SCALE, rectangle_f32(0, y_cursor, framebuffer->width, framebuffer->height*0.1),
                                                           string_literal("Tendered Shadows"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);

            if (main_menu.timer >= 4.3f) {
                main_menu.phase = MAIN_MENU_OPTIONS_APPEAR;
                main_menu.timer = 0;
            }
        } break;

        case MAIN_MENU_OPTIONS_APPEAR: {
            f32 t = main_menu.timer / 3.5f;
            if (t >= 1.0) t = 1.0;

            f32 y_cursor = 200;
            f32 x_cursor = cubic_ease_in_f32(-999, 0, t);

            software_framebuffer_draw_text_bounds_centered(framebuffer, font, TITLE_FONT_SCALE, rectangle_f32(0, 100, framebuffer->width, framebuffer->height*0.1),
                                                           string_literal("Tendered Shadows"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);

            s32 index = 0;
            main_menu_do_menu_ui(v2f32(x_cursor, y_cursor), framebuffer, main_menu_first_page_options, array_count(main_menu_first_page_options), &main_menu.currently_selected_option_choice, false);

            if (main_menu.timer >= 3.8f) {
                main_menu.phase = MAIN_MENU_IDLE;
            }
        } break;

        case MAIN_MENU_IDLE: {
            software_framebuffer_draw_text_bounds_centered(framebuffer, font, TITLE_FONT_SCALE, rectangle_f32(0, 100, framebuffer->width, framebuffer->height*0.1),
                                                           string_literal("Tendered Shadows"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            
            f32 y_cursor = 200;

            s32 choice = main_menu_do_menu_ui(v2f32(0, y_cursor), framebuffer, main_menu_first_page_options, array_count(main_menu_first_page_options), &main_menu.currently_selected_option_choice, true);
            if (choice != -1) {
                /* play a transition if needed */
                main_menu.currently_selected_option_choice = 0;
                switch (choice) {
                    case 1: {
                        screen_mode = GAME_SCREEN_INGAME;
                        game_initialize_game_world();
                    } break;
                    case 2: {
                        main_menu.phase         = MAIN_MENU_SAVE_MENU_DROP_DOWN;
                        main_menu.timer         = 0;
                        main_menu.scroll_seek_y = 0;
                        fill_all_save_slots();
                    } break;
                    case 3: {
                        main_menu.timer = 0;
                        main_menu.phase = MAIN_MENU_SHOW_OPTIONS_PAGE_MOVE_AWAY_NORMAL_OPTIONS;
                    } break;
                    case 4: {
                        global_game_running = false;
                    } break;
                }
            }
        } break;

        case MAIN_MENU_SHOW_OPTIONS_PAGE_MOVE_AWAY_NORMAL_OPTIONS: {
            software_framebuffer_draw_text_bounds_centered(framebuffer, font, TITLE_FONT_SCALE, rectangle_f32(0, 100, framebuffer->width, framebuffer->height*0.1),
                                                           string_literal("Tendered Shadows"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            f32 t = main_menu.timer / 3.5f;
            if (t >= 1.0) t = 1.0;

            f32 y_cursor = 200;
            f32 x_cursor = cubic_ease_in_f32(-999, 0, (1.0 - t));

            main_menu_do_menu_ui(v2f32(x_cursor, y_cursor), framebuffer, main_menu_first_page_options, array_count(main_menu_first_page_options), &main_menu.currently_selected_option_choice, false);

            if (main_menu.timer >= 3.8f) {
                main_menu.phase = MAIN_MENU_SHOW_OPTIONS_PAGE_MOVE_IN_OPTIONS;
                main_menu.timer = 0;
            }
        } break;

        case MAIN_MENU_SHOW_OPTIONS_PAGE_MOVE_IN_OPTIONS: {
            software_framebuffer_draw_text_bounds_centered(framebuffer, font, TITLE_FONT_SCALE, rectangle_f32(0, 100, framebuffer->width, framebuffer->height*0.1),
                                                           string_literal("Options"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            f32 t = main_menu.timer / 3.5f;
            if (t >= 1.0) t = 1.0;

            f32 y_cursor = 200;
            f32 x_cursor = cubic_ease_in_f32(-999, 0, (t));

            main_menu_do_menu_ui(v2f32(x_cursor, y_cursor), framebuffer, main_menu_options_page_options, array_count(main_menu_options_page_options), &main_menu.currently_selected_option_choice, false);

            if (main_menu.timer >= 3.8f) {
                main_menu.phase = MAIN_MENU_OPTIONS_PAGE_IDLE;
                main_menu.timer = 0;
            }
        } break;

        case MAIN_MENU_OPTIONS_PAGE_MOVE_OUT: {
            software_framebuffer_draw_text_bounds_centered(framebuffer, font, TITLE_FONT_SCALE, rectangle_f32(0, 100, framebuffer->width, framebuffer->height*0.1),
                                                           string_literal("Options"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            f32 t = main_menu.timer / 3.5f;
            if (t >= 1.0) t = 1.0;

            f32 y_cursor = 200;
            f32 x_cursor = cubic_ease_in_f32(-999, 0, (1.0 - t));

            main_menu_do_menu_ui(v2f32(x_cursor, y_cursor), framebuffer, main_menu_options_page_options, array_count(main_menu_options_page_options), &main_menu.currently_selected_option_choice, false);

            if (main_menu.timer >= 3.8f) {
                main_menu.phase = MAIN_MENU_OPTIONS_APPEAR;
                main_menu.timer = 0;
            }
        } break;

        case MAIN_MENU_OPTIONS_PAGE_IDLE: {
            software_framebuffer_draw_text_bounds_centered(framebuffer, font, TITLE_FONT_SCALE, rectangle_f32(0, 100, framebuffer->width, framebuffer->height*0.1),
                                                           string_literal("Options"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            f32 y_cursor = 200;

            s32 choice = main_menu_do_menu_ui(v2f32(0, y_cursor), framebuffer, main_menu_options_page_options, array_count(main_menu_options_page_options), &main_menu.currently_selected_option_choice, true);
            if (choice != -1) {
                /* play a transition if needed */
                main_menu.currently_selected_option_choice = 0;
                struct main_menu_option* selected_choice = &main_menu_options_page_options[choice];

                switch (choice) {
                    case 0: {
                        s32 resolution_id = selected_choice->selected_choice;
                        struct option_resolution_pair* new_resolution = &supported_resolutions[resolution_id];
                        change_resolution(new_resolution->w, new_resolution->h);
                    } break;
                    case 1: {
                        toggle_fullscreen();
                    } break;
                    case 2: {
                        main_menu.phase = MAIN_MENU_OPTIONS_PAGE_MOVE_OUT;
                        main_menu.timer = 0;
                    } break;
                }
            }
        } break;

        case MAIN_MENU_SAVE_MENU_DROP_DOWN: {
            const f32 MAX_T = 3.f;
            f32 effective_t = main_menu.timer/MAX_T;

            if (effective_t > 1)      effective_t = 1;
            else if (effective_t < 0) effective_t = 0;

            f32 y_offset = lerp_f32(-999, 0, effective_t);
            do_save_menu(framebuffer, y_offset, dt, false);

            if (main_menu.timer >= MAX_T) {
                main_menu.phase = MAIN_MENU_SAVE_MENU_IDLE;
            }

            main_menu.timer += dt;
        } break;

        case MAIN_MENU_SAVE_MENU_IDLE: {
            s32 selected_slot = do_save_menu(framebuffer, 0, dt, true);

            if (selected_slot != -1) {
                /* load slot and start the game */
                screen_mode = GAME_SCREEN_INGAME;
                game_initialize_game_world();
                game_load_from_save_slot(selected_slot);
            }
        } break;
        case MAIN_MENU_SAVE_MENU_CANCEL: {
            const f32 MAX_T = 3.f;
            f32 effective_t = main_menu.timer/MAX_T;

            if (effective_t > 1)      effective_t = 1;
            else if (effective_t < 0) effective_t = 0;

            f32 y_offset = lerp_f32(0, -999, effective_t);
            do_save_menu(framebuffer, y_offset, dt, false);

            if (main_menu.timer >= MAX_T) {
                main_menu.phase = MAIN_MENU_TITLE_APPEAR;
                main_menu.timer = 0;
            }

            main_menu.timer += dt;
        } break;
    } 

    main_menu.timer += dt * 2.41; 
}
