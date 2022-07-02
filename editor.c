/* 
   NOTE
   I am not surrendering the low resolution, that's part of the aesthetic,
   and also the software renderer may struggle at higher resolutions lol (post processing hurts),
   
   I believe on the last project, I wanted to basic "metaprogram" my property editor, which is probably
   the best way to make a level editor without wanting to shoot myself.
   
   I now have the opportunity to try that, although I may have to code specific editors for certain things
   still. Not a big deal, but maybe I can make a simple property editor and just expand the property
   editors as we do something else.
   
   We'll only have level transitions right now, so I might not really get to show that off...
*/

/* TODO World spawn */
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

/* While I could use one serialization function. In the case the formats deviate slightly... */
void editor_serialize_area(struct binary_serializer* serializer) {
    u32 version_id = CURRENT_LEVEL_AREA_VERSION;
    serialize_u32(serializer, &version_id);
    serialize_f32(serializer, &editor_state->default_player_spawn.x);
    serialize_f32(serializer, &editor_state->default_player_spawn.y);
    Serialize_Fixed_Array(serializer, s32, editor_state->tile_count, editor_state->tiles);
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

/* Editor code will always be a little nasty lol */
local void update_and_render_editor_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    s32 mouse_location[2];
    get_mouse_location(mouse_location, mouse_location+1);

    v2f32 world_space_mouse_location =
        camera_project(&editor_state->camera, v2f32(mouse_location[0], mouse_location[1]), framebuffer->width, framebuffer->height);

    /* for tiles */
    v2f32 tile_space_mouse_location = world_space_mouse_location; {
        tile_space_mouse_location.x = floorf(world_space_mouse_location.x / TILE_UNIT_SIZE);
        tile_space_mouse_location.y = floorf(world_space_mouse_location.y / TILE_UNIT_SIZE);
    }

    if (is_key_pressed(KEY_ESCAPE)) {
        game_state_set_ui_state(game_state, UI_STATE_PAUSE);
        /* ready pause menu */
        {
            game_state->ui_pause.animation_state = 0;
            game_state->ui_pause.transition_t    = 0;
            game_state->ui_pause.selection       = 0;
            zero_array(game_state->ui_pause.shift_t);
        }
    }

    if (is_key_down(KEY_W)) {
        editor_state->camera.xy.y -= 160 * dt;
    } else if (is_key_down(KEY_S)) {
        editor_state->camera.xy.y += 160 * dt;
    }
    if (is_key_down(KEY_A)) {
        editor_state->camera.xy.x -= 160 * dt;
    } else if (is_key_down(KEY_D)) {
        editor_state->camera.xy.x += 160 * dt;
    }

    /* Consider using tab + radial/fuzzy menu selection for this */
    if (is_key_down(KEY_SHIFT)) {
        if (is_key_pressed(KEY_LEFT)) {
            editor_state->tool_mode -= 1;
            if (editor_state->tool_mode < 0) editor_state->tool_mode = 2;
        } else if (is_key_pressed(KEY_RIGHT)) {
            editor_state->tool_mode += 1;
            if (editor_state->tool_mode > 2) editor_state->tool_mode = 0;
        }
    } else {
        /* Consider making a visual menu for tile selection. Ought to be less painful.  */
        /* Mouse imo is way faster for level editting tiles than using the keyboard... */
        /* granted it's more programming, but it'll be worth it. */
        if (editor_state->tool_mode == 0) {
            if (is_key_pressed(KEY_LEFT)) {
                editor_state->painting_tile_id -= 1;
                if (editor_state->painting_tile_id < 0) editor_state->painting_tile_id = 1;
            } else if (is_key_pressed(KEY_RIGHT)) {
                editor_state->painting_tile_id += 1;
                if (editor_state->painting_tile_id > 1) editor_state->painting_tile_id = 0;
            }
        }
    }

    bool left_clicked = 0;
    bool right_clicked = 0;
    get_mouse_buttons(&left_clicked, 0, &right_clicked);
    if (editor_state->tool_mode == 0) {
        if (left_clicked) {
            editor_place_tile_at(tile_space_mouse_location);
        } else if (right_clicked) {
            editor_remove_tile_at(tile_space_mouse_location);
        }
    } else if (editor_state->tool_mode == 1) {
        if (left_clicked) {
            editor_state->default_player_spawn.x = world_space_mouse_location.x;
            editor_state->default_player_spawn.y = world_space_mouse_location.y;
        }
    }

    f32 y_cursor = 0;
    {
        software_framebuffer_draw_text(framebuffer,
                                       graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]),
                                       1, v2f32(0,y_cursor), string_literal("EDITOR"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
        y_cursor += 12;
        {
            char tmp_text[1024]={};
            snprintf(tmp_text, 1024, "current tile id: %d", editor_state->painting_tile_id);
            software_framebuffer_draw_text(framebuffer,
                                           graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]),
                                           1, v2f32(0,y_cursor), string_from_cstring(tmp_text), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
        }
        y_cursor += 12;
        {
            char tmp_text[1024]={};
            snprintf(tmp_text, 1024, "mode: %.*s\n", editor_tool_mode_strings[editor_state->tool_mode].length, editor_tool_mode_strings[editor_state->tool_mode].data);
            software_framebuffer_draw_text(framebuffer,
                                           graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]),
                                           1, v2f32(0,y_cursor), string_from_cstring(tmp_text), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
        }
    }
    /* not using render commands here. I can trivially figure out what order most things should be... */
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
    render_commands_push_quad(&commands, rectangle_f32(editor_state->default_player_spawn.x, editor_state->default_player_spawn.y, TILE_UNIT_SIZE/4, TILE_UNIT_SIZE/4),
                              color32u8(0, 255, 0, normalized_sinf(global_elapsed_time*4) * 0.5*255 + 64), BLEND_MODE_ALPHA);

    switch (editor_state->tool_mode) {
        case 0: {
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
        } break;
    }
    /* cursor ghost */
    software_framebuffer_render_commands(framebuffer, &commands);
}
