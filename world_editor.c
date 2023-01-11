/* maybe i would like to polish these editor tools? */

void world_editor_clear_all_allocations(void) {
    for (s32 layer_index = 0; layer_index < WORLD_TILE_LAYER_COUNT; ++layer_index) {
        tile_layer_clear(world_editor_state->world_map.tile_layers + layer_index);
    }
    position_marker_list_clear(&world_editor_state->world_map.position_markers);
    trigger_list_clear(&world_editor_state->world_map.triggers);
    world_location_list_clear(&world_editor_state->world_map.locations);
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
        for (s32 tile_layer_index = 0; tile_layer_index < WORLD_TILE_LAYER_COUNT; ++tile_layer_index) {
            state->world_map.tile_layers[tile_layer_index] = tile_layer_reserved(state->arena, 32768*4);
        }
    }

    state->world_map.position_markers = position_marker_list_reserved(state->arena, 8192);
    state->world_map.triggers         = trigger_list_reserved(state->arena, 8192);
    state->world_map.locations        = world_location_list_reserved(state->arena, 8192);
    world_editor_clear_all();
}

/* NOTE: I too look forward to my CPU crying when this happens :) */
void world_editor_update_bounding_box_of_world(void) {
    world_editor_state->current_min_x = INT_MAX;
    world_editor_state->current_max_x = INT_MIN;
    world_editor_state->current_min_y = INT_MAX;
    world_editor_state->current_max_y = INT_MIN;

    for (s32 layer_index = 0; layer_index < array_count(world_editor_state->world_map.tile_layers); ++layer_index) {
        tile_layer_bounding_box(
            world_editor_state->world_map.tile_layers + layer_index,
            &world_editor_state->current_min_x,
            &world_editor_state->current_min_y,
            &world_editor_state->current_max_x,
            &world_editor_state->current_max_y
        );
    }
}

void world_editor_place_or_drag_position_marker(v2f32 point_in_tilespace) {
    if (is_dragging(&world_editor_state->drag_data)) {
        return;
    }

    struct position_marker* marker = position_marker_list_find_marker_at_with_rect(&world_editor_state->world_map.position_markers, point_in_tilespace);
    if (!marker) {
        world_editor_state->last_selected = position_marker_list_push(&world_editor_state->world_map.position_markers, position_marker(string_literal("noname"), point_in_tilespace));
    } else {
        set_drag_candidate(&world_editor_state->drag_data, marker, get_mouse_in_tile_space(&world_editor_state->camera, SCREEN_WIDTH, SCREEN_HEIGHT), marker->position);
    }
}
void world_editor_remove_position_marker_at(v2f32 point_in_tilespace) {
    struct position_marker* marker = position_marker_list_find_marker_at_with_rect(&world_editor_state->world_map.position_markers, point_in_tilespace);
    
    if (marker) {
        position_marker_list_remove(&world_editor_state->world_map.position_markers, marker - world_editor_state->world_map.position_markers.markers);
    }
}
void world_editor_place_or_drag_trigger(v2f32 point_in_tilespace) {
    if (is_dragging(&world_editor_state->drag_data)) {
        return; 
    }

    {
        struct trigger* existing_trigger = trigger_list_trigger_at(&world_editor_state->world_map.triggers, point_in_tilespace);
        if (existing_trigger) {
            world_editor_state->last_selected = existing_trigger;
            set_drag_candidate_rectangle(&world_editor_state->drag_data, existing_trigger, get_mouse_in_tile_space(&world_editor_state->camera, SCREEN_WIDTH, SCREEN_HEIGHT),
                                         v2f32(existing_trigger->bounds.x, existing_trigger->bounds.y),
                                         v2f32(existing_trigger->bounds.w, existing_trigger->bounds.h));
            return;
        } else {
            /* otherwise no touch, place a new one at default size 1 1 */
            struct trigger* new_trigger = trigger_list_push(&world_editor_state->world_map.triggers);
            new_trigger->bounds.x       = point_in_tilespace.x;
            new_trigger->bounds.y       = point_in_tilespace.y;
            new_trigger->bounds.w       = 1;
            new_trigger->bounds.h       = 1;
            world_editor_state->last_selected = new_trigger;
            return;
        }
    }
}
void world_editor_remove_trigger_at(v2f32 point_in_tilespace) {
    struct trigger* existing_trigger = trigger_list_trigger_at(&world_editor_state->world_map.triggers, point_in_tilespace);
    if (existing_trigger == world_editor_state->last_selected)  {
        world_editor_state->last_selected = 0;
    } else if (existing_trigger) {
        trigger_list_remove(&world_editor_state->world_map.triggers, existing_trigger - world_editor_state->world_map.triggers.triggers);
    }
}
void world_editor_place_or_drag_location(v2f32 point_in_tilespace) {
    if (is_dragging(&world_editor_state->drag_data)) {
        return; 
    }

    {
        struct world_location* existing_trigger = world_location_list_location_at(&world_editor_state->world_map.locations, point_in_tilespace);
        if (existing_trigger) {
            world_editor_state->last_selected = existing_trigger;
            set_drag_candidate_rectangle(&world_editor_state->drag_data, existing_trigger, get_mouse_in_tile_space(&world_editor_state->camera, SCREEN_WIDTH, SCREEN_HEIGHT),
                                         v2f32(existing_trigger->position.x, existing_trigger->position.y),
                                         v2f32(existing_trigger->scale.x, existing_trigger->scale.y));
            return;
        } else {
            /* otherwise no touch, place a new one at default size 1 1 */
            world_editor_state->last_selected = world_location_list_push(&world_editor_state->world_map.locations, world_location(point_in_tilespace, v2f32(1,1), string_literal("")));
            return;
        }
    }
}
void world_editor_remove_location_at(v2f32 point_in_tilespace) {
    struct world_location* existing_trigger = world_location_list_location_at(&world_editor_state->world_map.locations, point_in_tilespace);
    if (existing_trigger == editor_state->last_selected)  {
        world_editor_state->last_selected = 0;
    } else if (existing_trigger) {
        world_location_list_remove(&world_editor_state->world_map.locations, existing_trigger - world_editor_state->world_map.locations.locations);
    }
}

void world_editor_place_tile_at(v2f32 point_in_tilespace) {
    s32 where_x = point_in_tilespace.x;
    s32 where_y = point_in_tilespace.y;

    struct tile_layer* tile_layer    = &world_editor_state->world_map.tile_layers[world_editor_state->current_tile_layer];
    struct tile*       existing_tile = tile_layer_tile_at(tile_layer, where_x, where_y);

    if (existing_tile) {
        existing_tile->id                 = world_editor_state->painting_tile_id;
        world_editor_state->last_selected = existing_tile;
    } else {
        struct tile* new_tile       = tile_layer_push(tile_layer, tile(world_editor_state->painting_tile_id, 0, where_x, where_y));
        world_editor_state->last_selected = new_tile;
    }
}

void world_editor_remove_tile_at(v2f32 point_in_tilespace) {
    s32                where_x    = point_in_tilespace.x;
    s32                where_y    = point_in_tilespace.y;
    struct tile_layer* tile_layer = &world_editor_state->world_map.tile_layers[world_editor_state->current_tile_layer];
    struct tile*       tile       = tile_layer_tile_at(tile_layer, where_x, where_y);

    if (tile == editor_state->last_selected) {
        editor_state->last_selected = 0;
    }

    if (tile) tile_layer_remove(tile_layer, tile - tile_layer->tiles);
}

local void world_editor_brush_place_tile_at(v2f32 tile_space_mouse_location) {
    for (s32 y_index = 0; y_index < WORLD_EDITOR_BRUSH_SQUARE_SIZE; ++y_index) {
        for (s32 x_index = 0; x_index < WORLD_EDITOR_BRUSH_SQUARE_SIZE; ++x_index) {
            if (world_editor_brush_patterns[world_editor_state->editor_brush_pattern][y_index][x_index] == 1) {
                v2f32 point = tile_space_mouse_location;
                point.x += x_index - HALF_WORLD_EDITOR_BRUSH_SQUARE_SIZE;
                point.y += y_index - HALF_WORLD_EDITOR_BRUSH_SQUARE_SIZE;
                world_editor_place_tile_at(point);
            }
        }
    }
}

local void world_editor_brush_remove_tile_at(v2f32 tile_space_mouse_location) {
    for (s32 y_index = 0; y_index < WORLD_EDITOR_BRUSH_SQUARE_SIZE; ++y_index) {
        for (s32 x_index = 0; x_index < WORLD_EDITOR_BRUSH_SQUARE_SIZE; ++x_index) {
            if (world_editor_brush_patterns[world_editor_state->editor_brush_pattern][y_index][x_index] == 1) {
                v2f32 point = tile_space_mouse_location;
                point.x += x_index - HALF_WORLD_EDITOR_BRUSH_SQUARE_SIZE;
                point.y += y_index - HALF_WORLD_EDITOR_BRUSH_SQUARE_SIZE;
                world_editor_remove_tile_at(point);
            }
        }
    }
}

local void handle_world_editor_tool_mode_input(struct software_framebuffer* framebuffer) {
    bool left_clicked   = 0;
    bool right_clicked  = 0;
    bool middle_clicked = 0;

    /* do not check for mouse input when our special tab menu is open */
    if (world_editor_state->screen_state == WORLD_EDITOR_SCREEN_MAIN) {
        if (world_editor_state->tab_menu_open == TAB_MENU_CLOSED) {
            get_mouse_buttons(&left_clicked,
                              &middle_clicked,
                              &right_clicked);
        }
    } else {
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

    switch (world_editor_state->screen_state) {
        case WORLD_EDITOR_SCREEN_MAIN: {
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
                    } else if (is_key_pressed(KEY_6)) {
                        world_editor_state->editor_brush_pattern = 5;
                    } else if (is_key_pressed(KEY_7)) {
                        world_editor_state->editor_brush_pattern = 6;
                    }

                    if (left_clicked) {
                        world_editor_brush_place_tile_at(tile_space_mouse_location);
                    } else if (right_clicked) {
                        world_editor_brush_remove_tile_at(tile_space_mouse_location);
                    }
                } break;
                case WORLD_EDITOR_TOOL_SPAWN_PLACEMENT: {
                    if (left_clicked) {
                        world_editor_state->world_map.default_player_spawn.x = world_space_mouse_location.x;
                        world_editor_state->world_map.default_player_spawn.y = world_space_mouse_location.y;
                    }
                } break;
                case WORLD_EDITOR_TOOL_ENTITY_PLACEMENT: {
#define Placement_Procedure_For(type)                                   \
                    case WORLD_ENTITY_PLACEMENT_TYPE_ ## type: {        \
                        if (left_clicked) {                             \
                            world_editor_place_or_drag_##type(tile_space_mouse_location); \
                        } else if (right_clicked) {                     \
                            world_editor_remove_##type##_at(tile_space_mouse_location); \
                        }                                               \
                    } break

                    switch (world_editor_state->entity_placement_type) {
                        Placement_Procedure_For(position_marker);
                        Placement_Procedure_For(trigger);
                        Placement_Procedure_For(location);
                    }
#undef Placement_Procedure_For
                } break;
                default: {
                } break;
            }

            if (world_editor_state->tab_menu_open == TAB_MENU_CLOSED) {
                handle_rectangle_dragging_and_scaling(&world_editor_state->drag_data, &world_editor_state->camera);
            }
        } break;
        case WORLD_EDITOR_SCREEN_SETTING_LOCATION_ENTRANCE: {
            if (is_key_pressed(KEY_RETURN)) {
                world_editor_state->screen_state = WORLD_EDITOR_SCREEN_MAIN;
                world_editor_state->camera       = world_editor_state->setting_entrance_location.camera_before_trying_to_set_location_entrance;
                struct world_location* location = world_editor_state->last_selected;
                copy_string_into_cstring(string_from_cstring(world_editor_state->loaded_area_name), location->entrance.area_name, array_count(location->entrance.area_name));
            }

            if (left_clicked) {
                struct world_location* location = world_editor_state->last_selected;
                location->entrance.where        = tile_space_mouse_location;
            }
        } break;
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

    switch (world_editor_state->screen_state) {
        case WORLD_EDITOR_SCREEN_MAIN: {
            for (s32 layer_index = 0; layer_index < array_count(world_editor_state->world_map.tile_layers); ++layer_index) {
                editor_mode_render_tile_layer(TILE_PALETTE_WORLD_MAP,
                                              &commands,
                                              world_editor_state->world_map.tile_layers + layer_index,
                                              layer_index,
                                              world_editor_state->current_tile_layer);
            }

            struct font_cache* font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_BLUE]);
    
            for (s32 location_trigger_index = 0; location_trigger_index < world_editor_state->world_map.locations.count; ++location_trigger_index) {
                struct world_location* current_trigger = world_editor_state->world_map.locations.locations + location_trigger_index;

                if (world_editor_state->last_selected == current_trigger) {
                    render_commands_push_quad(&commands, rectangle_f32(current_trigger->position.x * TILE_UNIT_SIZE, current_trigger->position.y * TILE_UNIT_SIZE,
                                                                       current_trigger->scale.x * TILE_UNIT_SIZE, current_trigger->scale.y * TILE_UNIT_SIZE),
                                              color32u8(255, 255, 255, normalized_sinf(global_elapsed_time*2) * 0.5*255 + 64), BLEND_MODE_ALPHA);
                } else {
                    render_commands_push_quad(&commands, rectangle_f32(current_trigger->position.x * TILE_UNIT_SIZE, current_trigger->position.y * TILE_UNIT_SIZE,
                                                                       current_trigger->scale.x * TILE_UNIT_SIZE, current_trigger->scale.y * TILE_UNIT_SIZE),
                                              color32u8(255, 0, 0, normalized_sinf(global_elapsed_time*2) * 0.5*255 + 64), BLEND_MODE_ALPHA);
                }
                s32 trigger_id          = current_trigger - world_editor_state->world_map.locations.locations;
                render_commands_push_text(&commands, font, 1, v2f32(current_trigger->position.x * TILE_UNIT_SIZE, current_trigger->position.y * TILE_UNIT_SIZE),
                                          copy_cstring_to_scratch(format_temp("(world_location %d)", trigger_id)), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            }

            for (s32 generic_trigger_index = 0; generic_trigger_index < world_editor_state->world_map.triggers.count; ++generic_trigger_index) {
                struct trigger* current_trigger = world_editor_state->world_map.triggers.triggers + generic_trigger_index;

                if (world_editor_state->last_selected == current_trigger) {
                    render_commands_push_quad(&commands, rectangle_f32(current_trigger->bounds.x * TILE_UNIT_SIZE, current_trigger->bounds.y * TILE_UNIT_SIZE,
                                                                       current_trigger->bounds.w * TILE_UNIT_SIZE, current_trigger->bounds.h * TILE_UNIT_SIZE),
                                              color32u8(255, 255, 255, normalized_sinf(global_elapsed_time*2) * 0.5*255 + 64), BLEND_MODE_ALPHA);
                } else {
                    render_commands_push_quad(&commands, rectangle_f32(current_trigger->bounds.x * TILE_UNIT_SIZE, current_trigger->bounds.y * TILE_UNIT_SIZE,
                                                                       current_trigger->bounds.w * TILE_UNIT_SIZE, current_trigger->bounds.h * TILE_UNIT_SIZE),
                                              color32u8(255, 0, 0, normalized_sinf(global_elapsed_time*2) * 0.5*255 + 64), BLEND_MODE_ALPHA);
                }
                s32 trigger_id          = current_trigger - world_editor_state->world_map.triggers.triggers;
                render_commands_push_text(&commands, font, 1, v2f32(current_trigger->bounds.x * TILE_UNIT_SIZE, current_trigger->bounds.y * TILE_UNIT_SIZE),
                                          copy_cstring_to_scratch(format_temp("(trigger %d)", trigger_id)), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            }

            for (s32 position_marker_index = 0; position_marker_index < world_editor_state->world_map.position_markers.count; ++position_marker_index) {
                struct position_marker* current_marker = world_editor_state->world_map.position_markers.markers + position_marker_index;
                if (world_editor_state->last_selected == current_marker) {
                    render_commands_push_quad(&commands, rectangle_f32(current_marker->position.x * TILE_UNIT_SIZE, current_marker->position.y * TILE_UNIT_SIZE,
                                                                       current_marker->scale.x * TILE_UNIT_SIZE,    current_marker->scale.y * TILE_UNIT_SIZE),
                                              color32u8(0, 255, 0, 255), BLEND_MODE_ALPHA);
                } else {
                    render_commands_push_quad(&commands, rectangle_f32(current_marker->position.x * TILE_UNIT_SIZE, current_marker->position.y * TILE_UNIT_SIZE,
                                                                       current_marker->scale.x * TILE_UNIT_SIZE,    current_marker->scale.y * TILE_UNIT_SIZE),
                                              color32u8(255, 0, 0, 255), BLEND_MODE_ALPHA);
                }
                render_commands_push_text(&commands, font, 1, v2f32(current_marker->position.x * TILE_UNIT_SIZE, current_marker->position.y * TILE_UNIT_SIZE),
                                          copy_cstring_to_scratch(format_temp("(pmarker \"%s\")", current_marker->name)), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            }

            /* cursor */
            {
                v2f32 world_space_mouse_location = world_editor_get_world_space_mouse_location();
                v2f32 tile_space_mouse_location  = v2f32_snap_to_grid(world_space_mouse_location); 

                if (world_editor_state->tool_mode == WORLD_EDITOR_TOOL_TILE_PAINTING) {
                    for (s32 y_index = 0; y_index < WORLD_EDITOR_BRUSH_SQUARE_SIZE; ++y_index) {
                        for (s32 x_index = 0; x_index < WORLD_EDITOR_BRUSH_SQUARE_SIZE; ++x_index) {
                            if (world_editor_brush_patterns[world_editor_state->editor_brush_pattern][y_index][x_index] == 1) {
                                v2f32 point = tile_space_mouse_location;
                                point.x += x_index - HALF_WORLD_EDITOR_BRUSH_SQUARE_SIZE;
                                point.y += y_index - HALF_WORLD_EDITOR_BRUSH_SQUARE_SIZE;

                                render_commands_push_quad(&commands, rectangle_f32(point.x * TILE_UNIT_SIZE, point.y * TILE_UNIT_SIZE,
                                                                                   TILE_UNIT_SIZE, TILE_UNIT_SIZE),
                                                          color32u8(0, 0, 255, normalized_sinf(global_elapsed_time*4) * 0.5*255 + 64), BLEND_MODE_ALPHA);
                            }
                        }
                    }
                }
            }
            render_commands_push_quad(&commands, rectangle_f32(world_editor_state->world_map.default_player_spawn.x, world_editor_state->world_map.default_player_spawn.y, TILE_UNIT_SIZE/4, TILE_UNIT_SIZE/4),
                                      color32u8(0, 255, 0, normalized_sinf(global_elapsed_time*4) * 0.5*255 + 64), BLEND_MODE_ALPHA);
        } break;
        case WORLD_EDITOR_SCREEN_SETTING_LOCATION_ENTRANCE: {
            editor_common_render_level_area_for_preview(&commands, &world_editor_state->loaded_area);

            struct world_location* trigger = world_editor_state->last_selected;
            if (trigger) {
                render_commands_push_quad(&commands,
                                          rectangle_f32(trigger->entrance.where.x * TILE_UNIT_SIZE, trigger->entrance.where.y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE),
                                          color32u8(0, 255, 255, 255), BLEND_MODE_ALPHA);
            }

        } break;
    }


    software_framebuffer_render_commands(framebuffer, &commands);
    EDITOR_imgui_end_frame();
}

void update_and_render_world_editor_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    switch (world_editor_state->screen_state) {
        case WORLD_EDITOR_SCREEN_MAIN: {
            if (is_key_pressed(KEY_ESCAPE)) {
                if (world_editor_state->tab_menu_open & TAB_MENU_OPEN_BIT) {
                    world_editor_state->tab_menu_open = 0;
                } else {
                    game_state_set_ui_state(game_state, UI_STATE_PAUSE);
                }
            }

            if (is_key_down(KEY_SHIFT)) {
                if (is_key_pressed(KEY_Z)) {
                    world_editor_state->camera.zoom = 1;
                }

                f32 last_zoom = world_editor_state->camera.zoom;
                if (is_mouse_wheel_up()) {
                    world_editor_state->camera.zoom += 0.25;
                    if (world_editor_state->camera.zoom >= 4) {
                        world_editor_state->camera.zoom = 4;
                    }
                } else if (is_mouse_wheel_down()) {
                    world_editor_state->camera.zoom -= 0.25;
                    if (world_editor_state->camera.zoom <= 0.25) {
                        world_editor_state->camera.zoom = 0.25;
                    }
                }

                if (last_zoom != world_editor_state->camera.zoom) {
                    v2f32 world_space_mouse_location = world_editor_get_world_space_mouse_location();
                    v2f32 world_space_in_zoomed_space_last = world_space_mouse_location;
                    v2f32 world_space_in_zoomed_space_current = world_space_mouse_location;

                    world_space_in_zoomed_space_last.x *= last_zoom;
                    world_space_in_zoomed_space_last.y *= last_zoom;
                    world_space_in_zoomed_space_current.x *= world_editor_state->camera.zoom;
                    world_space_in_zoomed_space_current.y *= world_editor_state->camera.zoom;

                    f32 delta_zoom = world_editor_state->camera.zoom - last_zoom;
                    f32 delta_x    = world_space_in_zoomed_space_last.x - world_space_in_zoomed_space_current.x;
                    f32 delta_y    = world_space_in_zoomed_space_last.y - world_space_in_zoomed_space_current.y;

                    world_editor_state->camera.xy.x -= delta_x;
                    world_editor_state->camera.xy.y -= delta_y;
                }
            }

            if (!world_editor_state->tab_menu_open) {
                if (is_key_down(KEY_SHIFT)) {
                } else {
                    if (is_mouse_wheel_up()) {
                        world_editor_state->current_tile_layer -= 1;
                        if (world_editor_state->current_tile_layer < 0) {
                            world_editor_state->current_tile_layer = WORLD_TILE_LAYER_COUNT-1;
                        }
                    } else if (is_mouse_wheel_down()) {
                        world_editor_state->current_tile_layer += 1;
                        if (world_editor_state->current_tile_layer >= WORLD_TILE_LAYER_COUNT) {
                            world_editor_state->current_tile_layer = 0;
                        }
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
                        world_editor_update_bounding_box_of_world();
                        snprintf(tmp_text, 1024, "current tile id: %d\ncurrent tile layer: %.*s\nmin_x:%d,min_y:%d\n,max_x:%d,max_y:%d,\nw:%d,h:%d",
                                 world_editor_state->painting_tile_id,
                                 world_tile_layer_strings[world_editor_state->current_tile_layer].length,
                                 world_tile_layer_strings[world_editor_state->current_tile_layer].data,
                                 world_editor_state->current_min_x,
                                 world_editor_state->current_min_y,
                                 world_editor_state->current_max_x,
                                 world_editor_state->current_max_y,
                                 world_editor_state->current_max_x - world_editor_state->current_min_x,
                                 world_editor_state->current_max_y - world_editor_state->current_min_y
                        );
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
                        switch (world_editor_state->tool_mode) { /* PER OBJECT PROPERTY VIEW */
                            /* I would show images, but this is easier for now */
                            case WORLD_EDITOR_TOOL_TILE_PAINTING:
                            case WORLD_EDITOR_TOOL_LEVEL_SETTINGS: {
                                world_editor_state->tab_menu_open = 0;
                            } break;
                            case WORLD_EDITOR_TOOL_ENTITY_PLACEMENT: {
                                switch (world_editor_state->entity_placement_type) {
                                    case WORLD_ENTITY_PLACEMENT_TYPE_position_marker: {
                                        struct position_marker* marker = world_editor_state->last_selected;
                                        f32 draw_cursor_y = 70;
                                        const f32 text_scale = 1;

                                        {
                                            EDITOR_imgui_text_edit_cstring(framebuffer, font, highlighted_font, 2, v2f32(10, draw_cursor_y), string_literal("name"), marker->name, array_count(marker->name));
                                        }
                                    } break;
                                    case WORLD_ENTITY_PLACEMENT_TYPE_trigger: { /* there are only generic triggers thankfully. */
                                        struct trigger* trigger = world_editor_state->last_selected;
                                        s32 trigger_id          = trigger - world_editor_state->world_map.triggers.triggers;

                                        f32 draw_cursor_y = 30;
                                        string activation_type_string = activation_type_strings[trigger->activation_method];
                                        if(EDITOR_imgui_button(framebuffer, font, highlighted_font, 2, v2f32(16, draw_cursor_y),
                                                               string_from_cstring(format_temp("Activation Type: %.*s", activation_type_string.length, activation_type_string.data)))) {
                                            trigger->activation_method += 1;
                                            if (trigger->activation_method >= ACTIVATION_TYPE_COUNT) {
                                                trigger->activation_method = 0;
                                            }
                                        }
                                    } break;
                                    case WORLD_ENTITY_PLACEMENT_TYPE_location: { /* there are only generic triggers thankfully. */
                                        struct world_location* trigger = world_editor_state->last_selected;

                                        f32 draw_cursor_y = 30;
                                        {
                                            EDITOR_imgui_text_edit_cstring(framebuffer, font, highlighted_font, 2, v2f32(10, draw_cursor_y), string_literal("preview name"), trigger->preview_name, array_count(trigger->preview_name));
                                            draw_cursor_y += 2 * 16 * 1.5;
                                        }
                                        draw_cursor_y += 2 * 16 * 1.5;
                                        {
                                            string facing_direction_string = facing_direction_strings[trigger->entrance.facing_direction];
                                            string s = string_clone(&scratch_arena, string_from_cstring(format_temp("facing direction: %.*s", facing_direction_string.length, facing_direction_string.data)));
                                            s32 result = EDITOR_imgui_button(framebuffer, font, highlighted_font, 2, v2f32(10, draw_cursor_y), s);

                                            if (result == 1) {
                                                trigger->entrance.facing_direction += 1;
                                                if (trigger->entrance.facing_direction > 3) trigger->entrance.facing_direction = 0;
                                            } else if (result == 2) {
                                                if (trigger->entrance.facing_direction == 0) {
                                                    trigger->entrance.facing_direction = 3;
                                                } else {
                                                    trigger->entrance.facing_direction -= 1;
                                                }
                                            }
                                        }
                                        draw_cursor_y += 2 * 16 * 1.5;
                                        if(EDITOR_imgui_button(framebuffer, font, highlighted_font, 2, v2f32(16, draw_cursor_y), format_temp_s("visit location<%3.3f,%3.3f> : %s", trigger->entrance.where.x, trigger->entrance.where.y, trigger->entrance.area_name))) {
                                            world_editor_state->screen_state = WORLD_EDITOR_SCREEN_FILE_SELECTION_FOR_SETTING_LOCATION_ENTRANCE;
                                            world_editor_state->setting_entrance_location.camera_before_trying_to_set_location_entrance = world_editor_state->camera;
                                        }
                                    } break;
                                    default: {
                                        editor_state->tab_menu_open = 0;
                                    } break;
                                }
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
                        case WORLD_EDITOR_TOOL_ENTITY_PLACEMENT: {
                            f32 draw_cursor_y = 30;
                            for (s32 index = 0; index < array_count(world_entity_placement_type_strings)-1; ++index) {
                                if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 2, v2f32(16, draw_cursor_y), world_entity_placement_type_strings[index])) {
                                    world_editor_state->tab_menu_open          = 0;
                                    world_editor_state->entity_placement_type = index;
                                    break;
                                }
                                draw_cursor_y += 12 * 1.2 * 2;
                            }
                        } break;
                        default: {
                            editor_state->tab_menu_open          = 0;
                        } break;
                    }
                }
            }
        } break;
        case WORLD_EDITOR_SCREEN_FILE_SELECTION_FOR_SETTING_LOCATION_ENTRANCE: {
            struct font_cache* font             = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_STEEL]);
            struct font_cache* highlighted_font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]);
            if (is_key_pressed(KEY_ESCAPE)) {
                world_editor_state->screen_state = WORLD_EDITOR_SCREEN_MAIN;
            }
            struct directory_listing listing = directory_listing_list_all_files_in(&scratch_arena, string_literal("areas/"));

            v2f32 draw_position = v2f32(10, 10);
            if (listing.count <= 2) {
                software_framebuffer_draw_text(framebuffer, font, 2, draw_position, string_literal("(no areas)"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            } else {
                /* skip . and ../ */
                for (s32 index = 2; index < listing.count; ++index) {
                    struct directory_file* current_file = listing.files + index;
                    draw_position.y += 2 * 12 * 1;
                    if(EDITOR_imgui_button(framebuffer, font, highlighted_font, 2, draw_position, string_from_cstring(current_file->name))) {
                        world_editor_state->screen_state = WORLD_EDITOR_SCREEN_SETTING_LOCATION_ENTRANCE;
                        copy_string_into_cstring(string_from_cstring(current_file->name), world_editor_state->loaded_area_name, array_count(world_editor_state->loaded_area_name));

                        struct binary_serializer serializer = open_read_file_serializer(string_concatenate(&scratch_arena, string_literal("areas/"), string_from_cstring(current_file->name)));
                        serialize_level_area(state, &serializer, &world_editor_state->loaded_area, true);
                        serializer_finish(&serializer);
                        break;
                    }
                }
            }
        } break;
        case WORLD_EDITOR_SCREEN_SETTING_LOCATION_ENTRANCE: {
            if (is_key_pressed(KEY_ESCAPE)) {
                world_editor_state->screen_state = WORLD_EDITOR_SCREEN_MAIN;
            }

            assertion(world_editor_state->last_selected && "This should be impossible. You cannot enter this mode unless there is a last selected!");

            f32 y_cursor = 0;
            software_framebuffer_draw_text(framebuffer,
                                           graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]),
                                           1, v2f32(0,y_cursor), string_literal("Placing Entrance for a Location [ENTER to finish.]"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            y_cursor += 12;
        } break;
    }

}

void update_and_render_pause_world_editor_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    software_framebuffer_draw_quad(framebuffer, rectangle_f32(0, 0, framebuffer->width, framebuffer->height), color32u8(0,0,0,200), BLEND_MODE_ALPHA);
    struct font_cache* font             = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_STEEL]);
    struct font_cache* highlighted_font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]);
    f32 y_cursor = 30;

    IAllocator allocator = heap_allocator();

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
                OS_create_directory(string_literal("worldmaps/"));

                struct file_buffer script_filebuffer = OS_read_entire_file(allocator, string_literal("temp/WORLDMAP_SCRIPT.txt"));
                world_editor_state->world_map.script_string = file_buffer_as_string(&script_filebuffer);

                struct binary_serializer serializer = open_write_file_serializer(to_save_as);
                serialize_world_map(NULL, &serializer, &world_editor_state->world_map);
                serializer_finish(&serializer);

                file_buffer_free(&script_filebuffer);
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
                        world_editor_clear_all_allocations();

                        struct binary_serializer serializer = open_read_file_serializer(string_concatenate(&scratch_arena, string_literal(GAME_DEFAULT_WORLDMAPS_PATH), string_from_cstring(current_file->name)));
                        serialize_world_map(NULL, &serializer, &world_editor_state->world_map);
                        OS_create_directory(string_literal("temp/"));
                        write_string_into_entire_file(string_literal("temp/WORLDMAP_SCRIPT.txt"), world_editor_state->world_map.script_string);
                        serializer_finish(&serializer);

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
