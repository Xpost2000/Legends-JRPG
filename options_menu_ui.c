enum options_menu_phases {
    OPTIONS_MENU_PHASE_CLOSE,
    OPTIONS_MENU_PHASE_OPEN,
};

local string difficulty_setting_strings[] = {
    string_literal("SIMPLE"),
    string_literal("NORMAL"),
    string_literal("TACTICIAN"),
};

local string message_setting_strings[] = {
    string_literal("SLOW"),
    string_literal("NORMAL"),
    string_literal("FAST"),
    string_literal("INSTANT"),
};

local string ui_theme_setting_strings[] = {
    string_literal("AZURE"),
    string_literal("DEMON"),
    string_literal("ROYAL"),
};

struct options_menu_state {
    s32 phase;
    s32 currently_selected_option;

    s32  difficulty_slider;
    s32  message_slider;
    s32  ui_theme_slider;

    s32  resolution;
    bool is_fullscreen;

    /* 0 to 1.0 */
    f32  music_volume;
    /* 0 to 1.0 */
    f32  sound_volume;
};

struct options_menu_state options_menu_state;

void options_menu_open(void) {
    options_menu_state.phase                     = OPTIONS_MENU_PHASE_OPEN;
    options_menu_state.currently_selected_option = 0;

    /* find screen resolution that matches current setting */
    {
        options_menu_state.resolution = queried_resolution_find_index_of(REAL_SCREEN_WIDTH, REAL_SCREEN_HEIGHT);
        if (options_menu_state.resolution == -1) options_menu_state.resolution = 0;
    }
}

void options_menu_close(void) {
    options_menu_state.phase = OPTIONS_MENU_PHASE_CLOSE;
}

/* optimally this all fits on one screen, since scrolling this would be a pain, since I'd have to scissor and stuff like that... */
local string options_menu_categories[] = {
    string_literal("Game"),
    string_literal("Video"),
    string_literal("Audio"),
};
local string options_menu_options[] = {
    string_literal("Difficulty"),
    string_literal("Message Speed"),
    string_literal("UI Theme"),

    string_literal("Resolution"),

    /* check box */
    string_literal("Fullscreen"),

    /* I want discrete sliders? But have them show up as bars I guess */
    string_literal("Music Volume"),
    string_literal("Sound Volume"),
};

s32 do_options_menu(struct software_framebuffer* framebuffer, f32 dt) {
    struct font_cache* font1              = game_get_font(MENU_FONT_COLOR_WHITE);
    struct font_cache* font2              = game_get_font(MENU_FONT_COLOR_GOLD);

    f32 font_base_height = font_cache_text_height(font1);

    switch (options_menu_state.phase) {
        case OPTIONS_MENU_PHASE_OPEN: {
            const s32          OPTIONS_BOX_WIDTH  = 35;
            const s32          OPTIONS_BOX_HEIGHT = 25;

            v2f32 options_box_extents        = nine_patch_estimate_extents(ui_chunky, 1, OPTIONS_BOX_WIDTH, OPTIONS_BOX_HEIGHT);
            v2f32 options_box_start_position = v2f32((SCREEN_WIDTH/2 - options_box_extents.x/2), (SCREEN_HEIGHT/2) - options_box_extents.y/2);
            draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, options_box_start_position, OPTIONS_BOX_WIDTH, OPTIONS_BOX_HEIGHT, UI_DEFAULT_COLOR);

            {
                if (is_action_down_with_repeat(INPUT_ACTION_MOVE_DOWN)) {
                    options_menu_state.currently_selected_option += 1;
                    if (options_menu_state.currently_selected_option >= array_count(options_menu_options)+3) {
                        options_menu_state.currently_selected_option = 0;
                    }
                } else if (is_action_down_with_repeat(INPUT_ACTION_MOVE_UP)) {
                    options_menu_state.currently_selected_option -= 1;
                    if (options_menu_state.currently_selected_option < 0) {
                        options_menu_state.currently_selected_option = array_count(options_menu_options)-1+3;
                    }
                }
            }

#define Option_Menu_Choice_Label(id)                                    \
            do {                                                        \
                struct font_cache* f = game_get_font(MENU_FONT_COLOR_WHITE); \
                if (options_menu_state.currently_selected_option == id) { \
                    f = game_get_font(MENU_FONT_COLOR_GOLD);            \
                }                                                       \
                software_framebuffer_draw_text(framebuffer, f, 2, v2f32(option_x, layout.y), options_menu_options[id], color32f32(1,1,1,1), BLEND_MODE_ALPHA); \
            } while(0);

            {
                struct common_ui_layout layout = common_ui_vertical_layout(options_box_start_position.x + 15, options_box_start_position.y + 15);

                string longest_option       = longest_string_in_list(options_menu_options, array_count(options_menu_options));
                f32    longest_string_width = font_cache_text_width(font1, longest_option, 2);
                {
                    f32 layout_old_x = layout.x;
                    {
                        software_framebuffer_draw_text(framebuffer, font2, 2, v2f32(layout.x, layout.y), options_menu_categories[0], color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                        layout.y += font_base_height * 1.1 + 15;
                        layout.x += longest_string_width * 1.1 + 30;
                        f32 option_x = layout.x - longest_string_width*1.1;

                        {
                            Option_Menu_Choice_Label(0);
                            common_ui_visual_slider(&layout, framebuffer, 2, difficulty_setting_strings, array_count(difficulty_setting_strings), &options_menu_state.difficulty_slider, 0, &options_menu_state.currently_selected_option, 0);
                        }
                        {
                            Option_Menu_Choice_Label(1);
                            common_ui_visual_slider(&layout, framebuffer, 2, message_setting_strings, array_count(message_setting_strings), &options_menu_state.message_slider, 1, &options_menu_state.currently_selected_option, 0);
                        }
                        {
                            Option_Menu_Choice_Label(2);
                            common_ui_visual_slider(&layout, framebuffer, 2, ui_theme_setting_strings, array_count(ui_theme_setting_strings), &options_menu_state.ui_theme_slider, 2, &options_menu_state.currently_selected_option, 0);
                        }
                    }
                    layout.x = layout_old_x;
                }
                {
                    f32 layout_old_x = layout.x;
                    {
                        software_framebuffer_draw_text(framebuffer, font2, 2, v2f32(layout.x, layout.y), options_menu_categories[1], color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                        layout.y += font_base_height * 1.1 + 15;
                        layout.x += longest_string_width * 1.1 + 30;
                        f32 option_x = layout.x - longest_string_width*1.1;

#ifndef __EMSCRIPTEN__
                        {
                            Option_Menu_Choice_Label(3);
                            common_ui_visual_slider(&layout, framebuffer, 2, resolution_strings, queried_screen_resolution_count, &options_menu_state.resolution, 3, &options_menu_state.currently_selected_option, COMMON_UI_VISUAL_SLIDER_FLAGS_LOTSOFOPTIONS);
                        }
#endif
                        {
                            Option_Menu_Choice_Label(4);
                            common_ui_checkbox(&layout, framebuffer, 4, &options_menu_state.currently_selected_option, &options_menu_state.is_fullscreen, 0);
                        }
                    }
                    layout.x = layout_old_x;
                }
                {
                    f32 layout_old_x = layout.x;
                    {
                        software_framebuffer_draw_text(framebuffer, font2, 2, v2f32(layout.x, layout.y), options_menu_categories[2], color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                        layout.y += font_base_height * 1.1 + 15;
                        layout.x += longest_string_width * 1.1 + 30;
                        f32 option_x = layout.x - longest_string_width*1.1;

                        {
                            Option_Menu_Choice_Label(5);
                            common_ui_f32_slider(&layout, framebuffer, 32*4, &options_menu_state.currently_selected_option, 5, &options_menu_state.music_volume, 0, 1, 0);
                        }
                        {
                            Option_Menu_Choice_Label(6);
                            common_ui_f32_slider(&layout, framebuffer, 32*4, &options_menu_state.currently_selected_option, 6, &options_menu_state.sound_volume, 0, 1, 0);
                        }
                    }
                    layout.x = layout_old_x;
                }

                layout.y += 32;

                if (common_ui_button(&layout, framebuffer, string_literal("Apply"), 2, 7, &options_menu_state.currently_selected_option, 0)) {
                    set_fullscreen(options_menu_state.is_fullscreen);
                    options_menu_close();
                }
                if (common_ui_button(&layout, framebuffer, string_literal("Confirm"), 2, 8, &options_menu_state.currently_selected_option, 0)) {
                    options_menu_close();
                }
                if (common_ui_button(&layout, framebuffer, string_literal("Cancel"), 2, 9, &options_menu_state.currently_selected_option, 0)) {
                    options_menu_close();
                }
            }
#undef Option_Menu_Choice_Label
        } break;
        case OPTIONS_MENU_PHASE_CLOSE: {
            return OPTIONS_MENU_PROCESS_ID_EXIT;
        } break;
    }
    return OPTIONS_MENU_PROCESS_ID_OKAY;
}
