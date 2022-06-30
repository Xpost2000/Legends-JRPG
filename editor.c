void editor_clear_all(struct editor_state* state) {
    state->tile_count      = 0;

    state->camera.xy       = v2f32(0,0);
    state->camera.zoom     = 1;
    /* not centered to simplify code */
    /* state->camera.centered = 1; */
}

void editor_initialize(struct editor_state* state) {
    editor_state->arena         = &editor_arena;
    editor_state->tile_capacity = 8192;
    state->tiles = memory_arena_push(state->arena, state->tile_capacity * sizeof(*state->tiles));
    editor_clear_all(state);
}

void editor_remove_tile_at(v2f32 point_in_tilespace) {
    s32 where_x = point_in_tilespace.x;
    s32 where_y = point_in_tilespace.y;

    for (s32 index = 0; index < editor_state->tile_count; ++index) {
        struct tile* current_tile = editor_state->tiles + index;

        /* override */
        if (current_tile->x == where_x && current_tile->y == where_y) {
            current_tile->id = 0;
            editor_state->tiles[index] = editor_state->tiles[--editor_state->tile_count];
            return;
        }
    }
}

void editor_place_tile_at(v2f32 point_in_tilespace) {
    s32 where_x = point_in_tilespace.x;
    s32 where_y = point_in_tilespace.y;

    for (s32 index = 0; index < editor_state->tile_count; ++index) {
        struct tile* current_tile = editor_state->tiles + index;

        /* override */
        if (current_tile->x == where_x && current_tile->y == where_y) {
            current_tile->id = editor_state->painting_tile_id;
            return;
        }
    }

    struct tile* new_tile = editor_state->tiles + (editor_state->tile_count++);
    new_tile->id = editor_state->painting_tile_id;
    new_tile->x  = where_x;
    new_tile->y  = where_y;
}

void update_and_render_editor(struct software_framebuffer* framebuffer, f32 dt) {
    struct render_commands commands = render_commands(editor_state->camera);

    commands.should_clear_buffer = true;
    commands.clear_buffer_color  = color32u8(100, 128, 148, 255);

    for (s32 tile_index = 0; tile_index < editor_state->tile_count; ++tile_index) {
        struct tile* current_tile = editor_state->tiles + tile_index;
        image_id tex                     = brick_img;
        if (current_tile->id == 0) tex = grass_img;

        render_commands_push_image(
            &commands, graphics_assets_get_image_by_id(&graphics_assets, tex),
            rectangle_f32(
                current_tile->x * TILE_UNIT_SIZE,
                current_tile->y * TILE_UNIT_SIZE,
                TILE_UNIT_SIZE, TILE_UNIT_SIZE
            ),
            RECTANGLE_F32_NULL,
            color32f32(1,1,1,1),
            NO_FLAGS,
            BLEND_MODE_NONE
        );
    }

    /* cursor ghost */
    {
        s32 mouse_location[2];
        get_mouse_location(mouse_location, mouse_location+1);

        v2f32 world_space_mouse_location =
            camera_project(&editor_state->camera, v2f32(mouse_location[0], mouse_location[1]), framebuffer->width, framebuffer->height);
        v2f32 tile_space_mouse_location = world_space_mouse_location; {
            tile_space_mouse_location.x = floorf(world_space_mouse_location.x / TILE_UNIT_SIZE);
            tile_space_mouse_location.y = floorf(world_space_mouse_location.y / TILE_UNIT_SIZE);
        }

        render_commands_push_quad(&commands, rectangle_f32(tile_space_mouse_location.x * TILE_UNIT_SIZE, tile_space_mouse_location.y * TILE_UNIT_SIZE,
                                                           TILE_UNIT_SIZE, TILE_UNIT_SIZE),
                                  color32u8(0, 0, 255, normalized_sinf(global_elapsed_time*4) * 0.5*255 + 64), BLEND_MODE_ALPHA);
    }
    software_framebuffer_render_commands(framebuffer, &commands);
}
