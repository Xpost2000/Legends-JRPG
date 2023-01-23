enum options_menu_phases {
    OPTIONS_MENU_PHASE_CLOSE,
    OPTIONS_MENU_PHASE_OPEN,
};

struct options_menu_state {
    s32 phase;
    s32 currently_selected_option;
};

struct options_menu_state options_menu_state;

void options_menu_open(void) {
    options_menu_state.phase = OPTIONS_MENU_PHASE_OPEN;
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
    string_literal("Fullscreen"),

    string_literal("Music Volume"),
    string_literal("Sound Volume"),
};

s32 do_options_menu(struct software_framebuffer* framebuffer, f32 dt) {
    struct font_cache* font1              = game_get_font(MENU_FONT_COLOR_WHITE);
    struct font_cache* font2              = game_get_font(MENU_FONT_COLOR_GOLD);

    f32 font_base_height = font_cache_text_height(font1);

    switch (options_menu_state.phase) {
        case OPTIONS_MENU_PHASE_OPEN: {
            const s32          OPTIONS_BOX_WIDTH  = 20;
            const s32          OPTIONS_BOX_HEIGHT = 25;

            v2f32 options_box_extents        = nine_patch_estimate_extents(ui_chunky, 1, OPTIONS_BOX_WIDTH, OPTIONS_BOX_HEIGHT);
            v2f32 options_box_start_position = v2f32((SCREEN_WIDTH/2 - options_box_extents.x/2), (SCREEN_HEIGHT/2) - options_box_extents.y/2);
            draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, options_box_start_position, OPTIONS_BOX_WIDTH, OPTIONS_BOX_HEIGHT, UI_DEFAULT_COLOR);

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
                            software_framebuffer_draw_text(framebuffer, font1, 2, v2f32(option_x, layout.y), options_menu_options[0], color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                            common_ui_button(&layout, framebuffer, string_literal("option"), 2, 0, &options_menu_state.currently_selected_option, 0);
                        }
                        {
                            software_framebuffer_draw_text(framebuffer, font1, 2, v2f32(option_x, layout.y), options_menu_options[1], color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                            common_ui_button(&layout, framebuffer, string_literal("option"), 2, 1, &options_menu_state.currently_selected_option, 0);
                        }
                        {
                            software_framebuffer_draw_text(framebuffer, font1, 2, v2f32(option_x, layout.y), options_menu_options[2], color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                            common_ui_button(&layout, framebuffer, string_literal("option"), 2, 2, &options_menu_state.currently_selected_option, 0);
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

                        {
                            software_framebuffer_draw_text(framebuffer, font1, 2, v2f32(option_x, layout.y), options_menu_options[3], color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                            common_ui_button(&layout, framebuffer, string_literal("option"), 2, 3, &options_menu_state.currently_selected_option, 0);
                        }
                        {
                            software_framebuffer_draw_text(framebuffer, font1, 2, v2f32(option_x, layout.y), options_menu_options[4], color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                            common_ui_button(&layout, framebuffer, string_literal("option"), 2, 4, &options_menu_state.currently_selected_option, 0);
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
                            software_framebuffer_draw_text(framebuffer, font1, 2, v2f32(option_x, layout.y), options_menu_options[4], color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                            common_ui_button(&layout, framebuffer, string_literal("option"), 2, 5, &options_menu_state.currently_selected_option, 0);
                        }
                        {
                            software_framebuffer_draw_text(framebuffer, font1, 2, v2f32(option_x, layout.y), options_menu_options[5], color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                            common_ui_button(&layout, framebuffer, string_literal("option"), 2, 6, &options_menu_state.currently_selected_option, 0);
                        }
                    }
                    layout.x = layout_old_x;
                }

                layout.y += 32;

                if (common_ui_button(&layout, framebuffer, string_literal("Apply"), 2, 7, &options_menu_state.currently_selected_option, 0)) {
                    options_menu_close();
                }
                if (common_ui_button(&layout, framebuffer, string_literal("Confirm"), 2, 8, &options_menu_state.currently_selected_option, 0)) {
                    options_menu_close();
                }
                if (common_ui_button(&layout, framebuffer, string_literal("Cancel"), 2, 9, &options_menu_state.currently_selected_option, 0)) {
                    options_menu_close();
                }
            }
        } break;
        case OPTIONS_MENU_PHASE_CLOSE: {
            return OPTIONS_MENU_PROCESS_ID_EXIT;
        } break;
    }
    return OPTIONS_MENU_PROCESS_ID_OKAY;
}
