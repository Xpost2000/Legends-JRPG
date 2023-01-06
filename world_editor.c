void world_editor_clear_all_allocations(void) {
    for (s32 tile_count_index = 0; tile_count_index < WORLD_TILE_LAYER_COUNT; ++tile_count_index) {
        world_editor_state->tile_counts[tile_count_index] = 0;
    }
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

    world_editor_clear_all_allocations();
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

void update_and_render_world_editor(struct software_framebuffer* framebuffer, f32 dt) {
    struct render_commands commands = render_commands(&scratch_arena, 16384, world_editor_state->camera);
    commands.should_clear_buffer = true;
    commands.clear_buffer_color  = color32u8(100, 128, 148, 255);

    software_framebuffer_render_commands(framebuffer, &commands);
    EDITOR_imgui_end_frame();
}

void update_and_render_world_editor_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    if (is_key_pressed(KEY_ESCAPE)) {
        game_state_set_ui_state(game_state, UI_STATE_PAUSE);
    }

    {
        f32 y_cursor = 0;
        software_framebuffer_draw_text(framebuffer,
                                       graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]),
                                       1, v2f32(0,y_cursor), string_literal("World Editor [SHIFT-TAB (tool select), TAB (tool mode select), CTRL-TAB (object setting)]"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
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
