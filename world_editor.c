void world_editor_clear_all_allocations(void) {
    for (s32 tile_count_index = 0; tile_count_index < WORLD_TILE_LAYER_COUNT; ++tile_count_index) {
        world_editor_state->tile_counts[tile_count_index] = 0;
    }
}
local v2f32 world_editor_get_world_space_mouse_location(void) {
    s32 mouse_location[2];
    get_mouse_location(mouse_location, mouse_location+1);
    return camera_project(&world_editor_state->camera, v2f32(mouse_location[0], mouse_location[1]), SCREEN_WIDTH, SCREEN_HEIGHT);
}

void world_editor_clear_all(void) {
    world_editor_state->camera.xy       = v2f32(0,0);
    world_editor_state->camera.zoom     = 1;
    world_editor_state->camera.centered = 1;
    world_editor_clear_all_allocations();
}

void world_editor_initialize(struct world_editor_state* state) {
    state->arena = &world_editor_arena;

    {
        for (s32 tile_count_index = 0; tile_count_index < WORLD_TILE_LAYER_COUNT; ++tile_count_index) {
            state->tile_capacities[tile_count_index] = 32768 * 2;
        }
        for (s32 tile_layer_index = 0; tile_layer_index < WORLD_TILE_LAYER_COUNT; ++tile_layer_index) {
            state->tile_layers[tile_layer_index] = memory_arena_push(state->arena, sizeof(*state->tile_layers) * state->tile_capacities[tile_layer_index]);
        }
    }

    world_editor_clear_all();
}

void world_editor_place_tile_at(v2f32 point_in_tilespace) {
    s32 where_x = point_in_tilespace.x;
    s32 where_y = point_in_tilespace.y;

    for (s32 index = 0; index < world_editor_state->tile_counts[world_editor_state->current_tile_layer]; ++index) {
        struct tile* current_tile = world_editor_state->tile_layers[world_editor_state->current_tile_layer] + index;

        /* override */
        if (current_tile->x == where_x && current_tile->y == where_y) {
            current_tile->id = world_editor_state->painting_tile_id;
            world_editor_state->last_selected = current_tile;
            return;
        }
    }

    struct tile* new_tile = world_editor_state->tile_layers[world_editor_state->current_tile_layer] + (world_editor_state->tile_counts[world_editor_state->current_tile_layer]++);
    new_tile->id = world_editor_state->painting_tile_id;
    new_tile->x  = where_x;
    new_tile->y  = where_y;
    world_editor_state->last_selected = new_tile;
}
void world_editor_remove_tile_at(v2f32 point_in_tilespace) {
    s32 where_x = point_in_tilespace.x;
    s32 where_y = point_in_tilespace.y;

    for (s32 index = 0; index < world_editor_state->tile_counts[world_editor_state->current_tile_layer]; ++index) {
        struct tile* current_tile = world_editor_state->tile_layers[world_editor_state->current_tile_layer] + index;

        /* override */
        if (current_tile->x == where_x && current_tile->y == where_y) {
            if (current_tile == world_editor_state->last_selected) world_editor_state->last_selected = 0;

            current_tile->id = 0;
            world_editor_state->tile_layers[world_editor_state->current_tile_layer][index] = world_editor_state->tile_layers[world_editor_state->current_tile_layer][--world_editor_state->tile_counts[world_editor_state->current_tile_layer]];
            return;
        }
    }
}

local void world_editor_brush_place_tile_at(v2f32 tile_space_mouse_location) {
    for (s32 y_index = 0; y_index < EDITOR_BRUSH_SQUARE_SIZE; ++y_index) {
        for (s32 x_index = 0; x_index < EDITOR_BRUSH_SQUARE_SIZE; ++x_index) {
            if (editor_brush_patterns[world_editor_state->editor_brush_pattern][y_index][x_index] == 1) {
                v2f32 point = tile_space_mouse_location;
                point.x += x_index - HALF_EDITOR_BRUSH_SQUARE_SIZE;
                point.y += y_index - HALF_EDITOR_BRUSH_SQUARE_SIZE;
                editor_place_tile_at(point);
            }
        }
    }
}

local void world_editor_brush_remove_tile_at(v2f32 tile_space_mouse_location) {
    for (s32 y_index = 0; y_index < EDITOR_BRUSH_SQUARE_SIZE; ++y_index) {
        for (s32 x_index = 0; x_index < EDITOR_BRUSH_SQUARE_SIZE; ++x_index) {
            if (editor_brush_patterns[world_editor_state->editor_brush_pattern][y_index][x_index] == 1) {
                v2f32 point = tile_space_mouse_location;
                point.x += x_index - HALF_EDITOR_BRUSH_SQUARE_SIZE;
                point.y += y_index - HALF_EDITOR_BRUSH_SQUARE_SIZE;
                editor_remove_tile_at(point);
            }
        }
    }
}

local void world_editor_serialize_map(struct binary_serializer* serializer) {
    s32 version_id      = CURRENT_WORLD_MAP_VERSION;
    s32 area_version_id = CURRENT_LEVEL_AREA_VERSION;

    serialize_s32(serializer, &version_id);
    serialize_s32(serializer, &version_id);

    switch (version_id) {
        case 1: {
            serialize_f32(serializer, &world_editor_state->default_player_spawn.x);
            serialize_f32(serializer, &world_editor_state->default_player_spawn.y);

            for (s32 tile_layer = 0; tile_layer < WORLD_TILE_LAYER_COUNT; ++tile_layer) {
                serialize_tile_layer(serializer, area_version_id, &world_editor_state->tile_counts[tile_layer], world_editor_state->tile_layers[tile_layer]);
            }

            IAllocator allocator = heap_allocator();

            if (serializer->mode == BINARY_SERIALIZER_READ) {
                serialize_string(&allocator, serializer, &world_editor_state->map_script_string);
                OS_create_directory(string_literal("temp/"));
                write_string_into_entire_file(string_literal("temp/WORLDMAP_SCRIPT.txt"), world_editor_state->map_script_string);
            } else {
                struct file_buffer script_filebuffer = OS_read_entire_file(allocator, string_literal("temp/SCRIPT.txt"));
                world_editor_state->map_script_string = file_buffer_as_string(&script_filebuffer);
                serialize_string(&allocator, serializer, &world_editor_state->map_script_string);
                file_buffer_free(&script_filebuffer);
            }
        } break;
        default: {
            unimplemented("This is impossible?");
        } break;
    }
}

local void handle_world_editor_tool_mode_input(struct software_framebuffer* framebuffer) {
    bool left_clicked   = 0;
    bool right_clicked  = 0;
    bool middle_clicked = 0;

    /* do not check for mouse input when our special tab menu is open */
    if (world_editor_state->tab_menu_open == TAB_MENU_CLOSED) {
        get_mouse_buttons(&left_clicked,
                          &middle_clicked,
                          &right_clicked);
    }

    /* refactor later */
    s32 mouse_location[2];
    get_mouse_location(mouse_location, mouse_location+1);

    v2f32 world_space_mouse_location =
        camera_project(&world_editor_state->camera, v2f32(mouse_location[0], mouse_location[1]), SCREEN_WIDTH, SCREEN_HEIGHT);

    /* for tiles */
    v2f32 tile_space_mouse_location = world_space_mouse_location; {
        tile_space_mouse_location.x = floorf(world_space_mouse_location.x / TILE_UNIT_SIZE);
        tile_space_mouse_location.y = floorf(world_space_mouse_location.y / TILE_UNIT_SIZE);
    }

    /* mouse drag scroll */
    if (middle_clicked) {
        if (!world_editor_state->was_already_camera_dragging) {
            world_editor_state->was_already_camera_dragging = true;
            world_editor_state->initial_mouse_position      = v2f32(mouse_location[0], mouse_location[1]);
            world_editor_state->initial_camera_position     = world_editor_state->camera.xy;
        } else {
            v2f32 drag_delta = v2f32_sub(v2f32(mouse_location[0], mouse_location[1]),
                                         world_editor_state->initial_mouse_position);

            world_editor_state->camera.xy = v2f32_sub(world_editor_state->initial_camera_position,
                                                v2f32_sub(v2f32(mouse_location[0], mouse_location[1]), world_editor_state->initial_mouse_position));
        }
    } else {
        world_editor_state->was_already_camera_dragging = false;
    }

    switch (world_editor_state->tool_mode) {
        case WORLD_EDITOR_TOOL_TILE_PAINTING: {
            if (is_key_pressed(KEY_1)) {
                world_editor_state->editor_brush_pattern = 0;
            } else if (is_key_pressed(KEY_2)) {
                world_editor_state->editor_brush_pattern = 1;
            } else if (is_key_pressed(KEY_3)) {
                world_editor_state->editor_brush_pattern = 2;
            } else if (is_key_pressed(KEY_4)) {
                world_editor_state->editor_brush_pattern = 3;
            } else if (is_key_pressed(KEY_5)) {
                world_editor_state->editor_brush_pattern = 4;
            }

            if (!world_editor_state->viewing_loaded_area) {
                if (left_clicked) {
                    editor_brush_place_tile_at(tile_space_mouse_location);
                } else if (right_clicked) {
                    editor_brush_remove_tile_at(tile_space_mouse_location);
                }
            }
        } break;
        case WORLD_EDITOR_TOOL_SPAWN_PLACEMENT: {
            if (!world_editor_state->viewing_loaded_area) {
                if (left_clicked) {
                    world_editor_state->default_player_spawn.x = world_space_mouse_location.x;
                    world_editor_state->default_player_spawn.y = world_space_mouse_location.y;
                }
            }
        } break;
        default: {
        } break;
    }

    if (world_editor_state->tab_menu_open == TAB_MENU_CLOSED) {
        handle_rectangle_dragging_and_scaling(&world_editor_state->drag_data, &world_editor_state->camera);
    }
}

void update_and_render_world_editor(struct software_framebuffer* framebuffer, f32 dt) {
    struct render_commands commands = render_commands(&scratch_arena, 16384, world_editor_state->camera);
    commands.should_clear_buffer = true;
    commands.clear_buffer_color  = color32u8(100, 128, 148, 255);

    if (game_state->ui_state == UI_STATE_INGAME) {
        if (is_key_down(KEY_W)) {
            world_editor_state->camera.xy.y -= 160 * dt;
        } else if (is_key_down(KEY_S)) {
            world_editor_state->camera.xy.y += 160 * dt;
        }
        if (is_key_down(KEY_A)) {
            world_editor_state->camera.xy.x -= 160 * dt;
        } else if (is_key_down(KEY_D)) {
            world_editor_state->camera.xy.x += 160 * dt;
        }

        if (is_key_down(KEY_SHIFT) && is_key_pressed(KEY_TAB)) {
            world_editor_state->tab_menu_open ^= TAB_MENU_OPEN_BIT;
            world_editor_state->tab_menu_open ^= TAB_MENU_SHIFT_BIT;
        } else if (is_key_down(KEY_CTRL) && is_key_pressed(KEY_TAB)) {
            world_editor_state->tab_menu_open ^= TAB_MENU_OPEN_BIT;
            world_editor_state->tab_menu_open ^= TAB_MENU_CTRL_BIT;
        } else if (is_key_pressed(KEY_TAB)) {
            world_editor_state->tab_menu_open ^= TAB_MENU_OPEN_BIT;

            if (!(world_editor_state->tab_menu_open & TAB_MENU_OPEN_BIT)) world_editor_state->tab_menu_open = 0;
        } else {
            handle_world_editor_tool_mode_input(framebuffer);
        }
    }

    /* cursor */
    {
        v2f32 world_space_mouse_location = world_editor_get_world_space_mouse_location();
        v2f32 tile_space_mouse_location  = v2f32_snap_to_grid(world_space_mouse_location); 

        if (world_editor_state->tool_mode == WORLD_EDITOR_TOOL_TILE_PAINTING) {
            for (s32 y_index = 0; y_index < EDITOR_BRUSH_SQUARE_SIZE; ++y_index) {
                for (s32 x_index = 0; x_index < EDITOR_BRUSH_SQUARE_SIZE; ++x_index) {
                    if (editor_brush_patterns[world_editor_state->editor_brush_pattern][y_index][x_index] == 1) {
                        v2f32 point = tile_space_mouse_location;
                        point.x += x_index - HALF_EDITOR_BRUSH_SQUARE_SIZE;
                        point.y += y_index - HALF_EDITOR_BRUSH_SQUARE_SIZE;

                        render_commands_push_quad(&commands, rectangle_f32(point.x * TILE_UNIT_SIZE, point.y * TILE_UNIT_SIZE,
                                                                           TILE_UNIT_SIZE, TILE_UNIT_SIZE),
                                                  color32u8(0, 0, 255, normalized_sinf(global_elapsed_time*4) * 0.5*255 + 64), BLEND_MODE_ALPHA);
                    }
                }
            }
        }
    }

    for (s32 layer_index = 0; layer_index < array_count(world_editor_state->tile_layers); ++layer_index) {
        for (s32 tile_index = 0; tile_index < world_editor_state->tile_counts[layer_index]; ++tile_index) {
            struct tile*                 current_tile = world_editor_state->tile_layers[layer_index] + tile_index;
            s32                          tile_id      = current_tile->id;
            struct tile_data_definition* tile_data    = world_tile_table_data + tile_id;
            image_id                     tex          = get_tile_image_id(tile_data); 

            f32 alpha = 1;
            if (layer_index != world_editor_state->current_tile_layer) {
                alpha = 0.5;
            }

            render_commands_push_image(&commands,
                                       graphics_assets_get_image_by_id(&graphics_assets, tex),
                                       rectangle_f32(current_tile->x * TILE_UNIT_SIZE,
                                                     current_tile->y * TILE_UNIT_SIZE,
                                                     TILE_UNIT_SIZE,
                                                     TILE_UNIT_SIZE),
                                       tile_data->sub_rectangle,
                                       color32f32(1,1,1,alpha), NO_FLAGS, BLEND_MODE_ALPHA);
        }
    }
    render_commands_push_quad(&commands, rectangle_f32(world_editor_state->default_player_spawn.x, world_editor_state->default_player_spawn.y, TILE_UNIT_SIZE/4, TILE_UNIT_SIZE/4),
                              color32u8(0, 255, 0, normalized_sinf(global_elapsed_time*4) * 0.5*255 + 64), BLEND_MODE_ALPHA);

    software_framebuffer_render_commands(framebuffer, &commands);
    EDITOR_imgui_end_frame();
}

void update_and_render_world_editor_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    if (is_key_pressed(KEY_ESCAPE)) {
        if (world_editor_state->tab_menu_open & TAB_MENU_OPEN_BIT) {
            world_editor_state->tab_menu_open = 0;
        } else {
            game_state_set_ui_state(game_state, UI_STATE_PAUSE);
        }
    }

    if (!world_editor_state->tab_menu_open) {
        if (is_mouse_wheel_up()) {
            world_editor_state->current_tile_layer -= 1;
            if (world_editor_state->current_tile_layer < 0) {
                world_editor_state->current_tile_layer = TILE_LAYER_COUNT-1;
            }
        } else if (is_mouse_wheel_down()) {
            world_editor_state->current_tile_layer += 1;
            if (world_editor_state->current_tile_layer >= TILE_LAYER_COUNT) {
                world_editor_state->current_tile_layer = 0;
            }
        }

        {
            f32 y_cursor = 0;
            software_framebuffer_draw_text(framebuffer,
                                           graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]),
                                           1, v2f32(0,y_cursor), string_literal("World Editor [SHIFT-TAB (tool select), TAB (tool mode select), CTRL-TAB (object setting)]"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            y_cursor += 12;
            {
                char tmp_text[1024]={};
                snprintf(tmp_text, 1024, "current tile id: %d\ncurrent tile layer: %.*s", world_editor_state->painting_tile_id, world_tile_layer_strings[world_editor_state->current_tile_layer].length, world_tile_layer_strings[world_editor_state->current_tile_layer].data);
                software_framebuffer_draw_text(framebuffer,
                                               graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]),
                                               1, v2f32(0,y_cursor), string_from_cstring(tmp_text), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            }
            y_cursor += 12;
        }
    }
    if (world_editor_state->tab_menu_open & TAB_MENU_OPEN_BIT) {
        software_framebuffer_draw_quad(framebuffer,
                                       rectangle_f32(0, 0, framebuffer->width, framebuffer->height),
                                       color32u8(0,0,0,200), BLEND_MODE_ALPHA);
        /* tool selector */
        struct font_cache* font             = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_STEEL]);
        struct font_cache* highlighted_font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]);

        if (world_editor_state->tab_menu_open & TAB_MENU_SHIFT_BIT) {
            f32 draw_cursor_y = 30;
            for (s32 index = 0; index < array_count(world_editor_tool_mode_strings)-1; ++index) {
                if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 3, v2f32(100, draw_cursor_y), world_editor_tool_mode_strings[index])) {
                    world_editor_state->tab_menu_open = 0;
                    world_editor_state->tool_mode     = index;
                    world_editor_state->last_selected = 0;
                    break;
                }
                draw_cursor_y += 12 * 1.5 * 3;
            }
        } else if (world_editor_state->tab_menu_open & TAB_MENU_CTRL_BIT) {
            if (!world_editor_state->last_selected) {
                world_editor_state->tab_menu_open = 0;
            } else {
                switch (world_editor_state->tool_mode) {
                    /* I would show images, but this is easier for now */
                    case WORLD_EDITOR_TOOL_TILE_PAINTING:
                    case WORLD_EDITOR_TOOL_LEVEL_SETTINGS: {
                        world_editor_state->tab_menu_open = 0;
                    } break;
                }
            }
        } else {
            switch (world_editor_state->tool_mode) {
                /* I would show images, but this is easier for now */
                case WORLD_EDITOR_TOOL_LEVEL_SETTINGS: {
                    world_editor_state->tab_menu_open = 0;
                } break;
                case WORLD_EDITOR_TOOL_TILE_PAINTING: {
                    f32 draw_cursor_y = 30 + world_editor_state->tile_painting_property_menu.item_list_scroll_y;

                    const s32 SCROLL_AMOUNT = 45;
                    if (is_mouse_wheel_up()) {
                        if (world_editor_state->tile_painting_property_menu.item_list_scroll_y < 0)
                            world_editor_state->tile_painting_property_menu.item_list_scroll_y += SCROLL_AMOUNT;
                    } else if (is_mouse_wheel_down()) {
                        world_editor_state->tile_painting_property_menu.item_list_scroll_y -= SCROLL_AMOUNT;
                    }

                    if (is_key_pressed(KEY_HOME)) {
                        world_editor_state->tile_painting_property_menu.item_list_scroll_y = 0;
                    }


                    const f32 text_scale = 1;

                    f32 largest_name_width = 0;

                    /* TODO should be lazy init */
                    for (s32 index = 0; index < world_tile_table_data_count; ++index) {
                        s32 tile_data_index = index;
                        struct tile_data_definition* tile_data = world_tile_table_data + tile_data_index;

                        f32 candidate = font_cache_text_width(font, tile_data->name, text_scale);
                        if (largest_name_width < candidate) {
                            largest_name_width = candidate;
                        }
                    }

                    s32 TILES_PER_ROW = (500 / largest_name_width);
                    s32 row_count     = (world_tile_table_data_count / TILES_PER_ROW)+1;

                    for (s32 row_index = 0; row_index < row_count; ++row_index) {
                        f32 draw_cursor_x = 0;

                        for (s32 index = 0; index < TILES_PER_ROW; ++index) {
                            s32 tile_data_index = row_index * TILES_PER_ROW + index;
                            if (!(tile_data_index < world_tile_table_data_count)) {
                                break;
                            }

                            struct tile_data_definition* tile_data = world_tile_table_data + tile_data_index;
                            image_id tex = get_tile_image_id(tile_data); 

                            software_framebuffer_draw_image_ex(framebuffer, graphics_assets_get_image_by_id(&graphics_assets, tex),
                                                               rectangle_f32(draw_cursor_x, draw_cursor_y-16, 32, 32), RECTANGLE_F32_NULL, color32f32(1,1,1,1), NO_FLAGS, BLEND_MODE_ALPHA);

                            if (EDITOR_imgui_button(framebuffer, font, highlighted_font, text_scale, v2f32(draw_cursor_x, draw_cursor_y), tile_data->name)) {
                                world_editor_state->tab_menu_open    = 0;
                                world_editor_state->painting_tile_id = tile_data_index;
                            }

                            draw_cursor_x += largest_name_width * 1.1;
                        }

                        draw_cursor_y += 24 * 1.2 * 2;
                    }
                } break;
                default: {
                    editor_state->tab_menu_open          = 0;
                } break;
            }
        }
    }
}

void update_and_render_pause_world_editor_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    software_framebuffer_draw_quad(framebuffer, rectangle_f32(0, 0, framebuffer->width, framebuffer->height), color32u8(0,0,0,200), BLEND_MODE_ALPHA);
    struct font_cache* font             = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_STEEL]);
    struct font_cache* highlighted_font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]);
    f32 y_cursor = 30;

    if (is_key_pressed(KEY_ESCAPE)) {
        if (world_editor_state->pause_menu.screen != 0) {
            world_editor_state->pause_menu.screen = 0;
        } else {
            game_state_set_ui_state(game_state, UI_STATE_INGAME);
            return;
        }
    }

    switch (world_editor_state->pause_menu.screen) {
        case 0: {
            {
                if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 3, v2f32(100, y_cursor), string_literal("SAVE"))) {
                    world_editor_state->pause_menu.screen = 1;
                }
                y_cursor += 16 * 1.2*3;
                if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 3, v2f32(100, y_cursor), string_literal("LOAD"))) {
                    world_editor_state->pause_menu.screen = 2;
                }
                y_cursor += 16 * 1.2*3;
            }
        } break;
        case 1: {
            EDITOR_imgui_text_edit_cstring(framebuffer, font, highlighted_font, 2, v2f32(100, y_cursor), string_literal("savename"), world_editor_state->current_save_name, array_count(world_editor_state->current_save_name));
            y_cursor += 2 * 12 * 1;

            if((is_key_pressed(KEY_RETURN) && !is_editing_text()) ||
               EDITOR_imgui_button(framebuffer, font, highlighted_font, 2, v2f32(100, y_cursor), string_literal("CONFIRM"))) {
                world_editor_state->pause_menu.screen = 0;
                string to_save_as = string_concatenate(&scratch_arena, string_literal(GAME_DEFAULT_WORLDMAPS_PATH), string_from_cstring(world_editor_state->current_save_name));
#if 1
                struct binary_serializer serializer = open_write_file_serializer(to_save_as);
                world_editor_serialize_map(&serializer);
                serializer_finish(&serializer);
#endif
            }
        } break;
        case 2: {
            struct directory_listing listing = directory_listing_list_all_files_in(&scratch_arena, string_literal(GAME_DEFAULT_WORLDMAPS_PATH));
            if (listing.count <= 2) {
                software_framebuffer_draw_text(framebuffer, font, 2, v2f32(100, y_cursor), string_literal("(no maps)"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            } else {
                /* skip . and ../ */
                for (s32 index = 2; index < listing.count; ++index) {
                    struct directory_file* current_file = listing.files + index;
                    y_cursor += 2 * 12 * 1;
                    if(EDITOR_imgui_button(framebuffer, font, highlighted_font, 2, v2f32(100, y_cursor), string_from_cstring(current_file->name))) {
#if 1
                        struct binary_serializer serializer = open_read_file_serializer(string_concatenate(&scratch_arena, string_literal(GAME_DEFAULT_WORLDMAPS_PATH), string_from_cstring(current_file->name)));
                        world_editor_serialize_map(&serializer);
                        serializer_finish(&serializer);
#endif

                        cstring_copy(current_file->name, world_editor_state->current_save_name, array_count(world_editor_state->current_save_name));
                        world_editor_state->pause_menu.screen = 0;
                    }
                }
            }
        } break;
    }

}

void update_and_render_world_editor_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    switch (state->ui_state) {
        case UI_STATE_INGAME: {
            update_and_render_world_editor_game_menu_ui(state, framebuffer, dt);
        } break;
        case UI_STATE_PAUSE: {
            update_and_render_pause_world_editor_menu_ui(state, framebuffer, dt);
        } break;
            bad_case;
    }
}
