/* TODO Need to clean up  */
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
local void wrap_around_key_selection(s32 decrease_key, s32 increase_key, s32* pointer, s32 min, s32 max) {
    if (is_key_pressed(decrease_key)) {
        *pointer -= 1;
        if (*pointer < min)
            *pointer = max-1;
    } else if (is_key_pressed(increase_key)) {
        *pointer += 1;
        if (*pointer >= max)
            *pointer = min;
    }
}

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

local void handle_editor_tool_mode_input(struct software_framebuffer* framebuffer) {
    bool left_clicked   = 0;
    bool right_clicked  = 0;
    bool middle_clicked = 0;
    get_mouse_buttons(&left_clicked,
                      &middle_clicked,
                      &right_clicked);

    s32 mouse_location[2];
    get_mouse_location(mouse_location, mouse_location+1);

    v2f32 world_space_mouse_location =
        camera_project(&editor_state->camera, v2f32(mouse_location[0], mouse_location[1]), framebuffer->width, framebuffer->height);

    /* for tiles */
    v2f32 tile_space_mouse_location = world_space_mouse_location; {
        tile_space_mouse_location.x = floorf(world_space_mouse_location.x / TILE_UNIT_SIZE);
        tile_space_mouse_location.y = floorf(world_space_mouse_location.y / TILE_UNIT_SIZE);
    }

    /* mouse drag scroll */
    if (middle_clicked) {
        if (!editor_state->was_already_camera_dragging) {
            editor_state->was_already_camera_dragging = true;
            editor_state->initial_mouse_position      = v2f32(mouse_location[0], mouse_location[1]);
            editor_state->initial_camera_position     = editor_state->camera.xy;
        } else {
            v2f32 drag_delta = v2f32_sub(v2f32(mouse_location[0], mouse_location[1]),
                                         editor_state->initial_mouse_position);

            editor_state->camera.xy = v2f32_sub(editor_state->initial_camera_position,
                                                v2f32_sub(v2f32(mouse_location[0], mouse_location[1]), editor_state->initial_mouse_position));
        }
    } else {
        editor_state->was_already_camera_dragging = false;
    }

    switch (editor_state->tool_mode) {
        case EDITOR_TOOL_TILE_PAINTING: {
            /* Consider making a visual menu for tile selection. Ought to be less painful.  */
            /* Mouse imo is way faster for level editting tiles than using the keyboard... */
            /* granted it's more programming, but it'll be worth it. */
            wrap_around_key_selection(KEY_LEFT, KEY_RIGHT, &editor_state->painting_tile_id, 0, 2);

            if (left_clicked) {
                editor_place_tile_at(tile_space_mouse_location);
            } else if (right_clicked) {
                editor_remove_tile_at(tile_space_mouse_location);
            }
        } break;
        case EDITOR_TOOL_SPAWN_PLACEMENT: {
            if (left_clicked) {
                editor_state->default_player_spawn.x = world_space_mouse_location.x;
                editor_state->default_player_spawn.y = world_space_mouse_location.y;
            }
        } break;
        case EDITOR_TOOL_TRIGGER_PLACEMENT: {
            wrap_around_key_selection(KEY_LEFT, KEY_RIGHT, &editor_state->trigger_placement_type, 0, 2);
        } break;
        default: {
        } break;
    }
}

/* copied and pasted for now */
/* This can be compressed, quite easily... However I won't deduplicate this yet, as I've yet to experiment fully with the UI so let's keep it like this for now. */
/* NOTE: animation is hard. Lots of state to keep track of. */
local void update_and_render_pause_editor_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    /* needs a bit of cleanup */
    f32 font_scale = 3;
    /* While the real pause menu is going to be replaced with something else later obviously */
    /* I want a nice looking menu to show off, and also the main menu is likely taking this design */
    struct ui_pause_menu* menu_state = &state->ui_pause;
    v2f32 item_positions[array_count(ui_pause_editor_menu_strings)] = {};

    for (unsigned index = 0; index < array_count(item_positions); ++index) {
        item_positions[index].y = 36 * (index+0.75);
    }

    f32 offscreen_x = -240;
    f32 final_x     = 40;

    u32 blur_samples = 4;
    f32 max_blur = 1.0;
    f32 max_grayscale = 0.8;

    f32 timescale = 1.34f;

    switch (menu_state->animation_state) {
        case UI_PAUSE_MENU_TRANSITION_IN: {
            menu_state->transition_t   += dt * timescale;

            for (unsigned index = 0; index < array_count(item_positions); ++index) {
                item_positions[index].x = lerp_f32(offscreen_x, final_x, menu_state->transition_t);
            }
            
            f32 fade_t = menu_state->transition_t;
            if (editor_state->serialize_menu_mode) {
                fade_t = 1;
                editor_state->serialize_menu_t -= dt;
            }

            game_postprocess_blur(framebuffer, blur_samples, max_blur * fade_t, BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale * fade_t);

            if (menu_state->transition_t >= 1.0f) {
                menu_state->animation_state += 1;
                menu_state->transition_t = 0;

                if (editor_state->serialize_menu_mode) {
                    editor_state->serialize_menu_mode = 0;
                }
            }
        } break;
        case UI_PAUSE_MENU_NO_ANIM: {
            switch (editor_state->serialize_menu_mode) {
                case 0: { /* default state, default pause menu */
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

                    if (is_key_pressed(KEY_ESCAPE)) {
                        menu_state->animation_state = UI_PAUSE_MENU_TRANSITION_CLOSING;
                        menu_state->transition_t = 0;
                    }        

                    if (is_key_down_with_repeat(KEY_DOWN)) {
                        menu_state->selection++;
                        if (menu_state->selection >= array_count(item_positions)) menu_state->selection = 0;
                    }
                    if (is_key_down_with_repeat(KEY_UP)) {
                        menu_state->selection--;
                        if (menu_state->selection < 0) menu_state->selection = array_count(item_positions)-1;
                    }

                    if (is_key_pressed(KEY_RETURN)) {
                        switch (menu_state->selection) {
                            case 0: {
                                menu_state->animation_state = UI_PAUSE_MENU_TRANSITION_CLOSING;
                                menu_state->transition_t = 0;
                            } break;
                            case 1: {
                                /* who knows about this one */ 
                                u8* data;
                                u64 amount;

                                struct binary_serializer serializer = open_write_memory_serializer();
                                editor_serialize_area(&serializer);
                                data = serializer_flatten_memory(&serializer, &amount);
                                struct binary_serializer serializer1 = open_read_memory_serializer(data, amount);
                                serialize_level_area(game_state, &serializer1, &game_state->loaded_area);
                                serializer_finish(&serializer1);
                                serializer_finish(&serializer);
                                system_heap_memory_deallocate(data);

                                game_state->in_editor = false;
                                menu_state->animation_state = UI_PAUSE_MENU_TRANSITION_CLOSING;
                                menu_state->transition_t    = 0;
                            } break;
                            case 2: {
#if 0
                                struct binary_serializer serializer = open_write_file_serializer(string_literal("edit.area"));
                                editor_serialize_area(&serializer);
                                serializer_finish(&serializer);
#else
                                menu_state->animation_state     = UI_PAUSE_MENU_TRANSITION_CLOSING;
                                menu_state->transition_t        = 0;
                                editor_state->serialize_menu_mode = 1;
#endif
                            } break;
                            case 3: {
#if 0
                                struct binary_serializer serializer = open_read_file_serializer(string_literal("edit.area"));
                                editor_serialize_area(&serializer);
                                serializer_finish(&serializer);
#else
                                menu_state->animation_state     = UI_PAUSE_MENU_TRANSITION_CLOSING;
                                menu_state->transition_t        = 0;
                                editor_state->serialize_menu_mode = 2;
#endif
                            } break;
                            case 4: {
                            } break;
                            case 5: {
                                global_game_running = false;
                            } break;
                        }
                    }
                } break;
                case 1:
                case 2: {
                    if (is_key_pressed(KEY_ESCAPE)) {
                        menu_state->animation_state = UI_PAUSE_MENU_TRANSITION_IN;
                        menu_state->transition_t = 0;
                    }        

                    for (unsigned index = 0; index < array_count(item_positions); ++index) {
                        item_positions[index].x = -9999;
                    }

                    if (editor_state->serialize_menu_t < 1) {
                        editor_state->serialize_menu_t += dt;
                    }

                    if (editor_state->serialize_menu_mode == 1) {
                        
                    }
                } break;
            }

            game_postprocess_blur(framebuffer, blur_samples, max_blur, BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale);
        } break;
        case UI_PAUSE_MENU_TRANSITION_CLOSING: {
            menu_state->transition_t   += dt * timescale;

            for (unsigned index = 0; index < array_count(item_positions); ++index) {
                item_positions[index].x = lerp_f32(final_x, offscreen_x, menu_state->transition_t);
            }

            f32 fade_t = (1-menu_state->transition_t);
            if (editor_state->serialize_menu_mode != 0) {
                fade_t = (1);
            }

            game_postprocess_blur(framebuffer, blur_samples, max_blur * fade_t, BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale * fade_t);

            if (menu_state->transition_t >= 1.0f) {
                menu_state->transition_t = 0;
                if (editor_state->serialize_menu_mode != 0) {
                    menu_state->animation_state = UI_PAUSE_MENU_NO_ANIM;
                    editor_state->serialize_menu_t = 0;
                } else {
                    game_state_set_ui_state(game_state, state->last_ui_state);
                }
            }
        } break;
    }
    
    for (unsigned index = 0; index < array_count(item_positions); ++index) {
        v2f32 draw_position = item_positions[index];
        draw_position.x += lerp_f32(0, 20, menu_state->shift_t[index]);
        draw_position.y += 220;
        /* custom string drawing routine */
        struct font_cache* font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_STEEL]);
        if (index == menu_state->selection) {
            font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]);
        }

        for (unsigned character_index = 0; character_index < ui_pause_editor_menu_strings[index].length; ++character_index) {
            f32 character_displacement_y = sinf((global_elapsed_time*2) + ((character_index+index) * 2381.2318)) * 3;

            v2f32 glyph_position = draw_position;
            glyph_position.y += character_displacement_y;
            glyph_position.x += font->tile_width * font_scale * character_index;

            software_framebuffer_draw_text(framebuffer, font, font_scale, glyph_position, string_slice(ui_pause_editor_menu_strings[index], character_index, character_index+1), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
        }
    }

    {
        struct font_cache* font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_STEEL]);
        v2f32 draw_position = v2f32(0,0);
        draw_position.x = lerp_f32(-200, 80, editor_state->serialize_menu_t);
        switch (editor_state->serialize_menu_mode) {
            case 1: {
                software_framebuffer_draw_text(framebuffer, font, font_scale, draw_position, string_literal("SAVE GAME"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                draw_position.y += font_scale * 12 * 3;
                {
                    char tmp_text[1024];
                    snprintf(tmp_text, 1024, "SAVE AS: %s", editor_state->current_save_name);
                    software_framebuffer_draw_text(framebuffer, font, font_scale, draw_position, string_from_cstring(tmp_text), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                }
            } break;
            case 2: {
                software_framebuffer_draw_text(framebuffer, font, font_scale, draw_position, string_literal("LOAD GAME"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            } break;
        }
    }
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
        wrap_around_key_selection(KEY_LEFT, KEY_RIGHT, &editor_state->tool_mode, 0, EDITOR_TOOL_COUNT-1);
    } else {
        handle_editor_tool_mode_input(framebuffer);
    }

    /* I refuse to code a UI library, unless *absolutely* necessary... Since the editor is the only part that requires this kind of standardized UI... */
    f32 y_cursor = 0;
    {
        software_framebuffer_draw_text(framebuffer,
                                       graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]),
                                       1, v2f32(0,y_cursor), string_literal("Level Editor"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
        y_cursor += 12;
        {
            char tmp_text[1024]={};
            snprintf(tmp_text, 1024, "mode: %.*s\n", editor_tool_mode_strings[editor_state->tool_mode].length, editor_tool_mode_strings[editor_state->tool_mode].data);
            software_framebuffer_draw_text(framebuffer,
                                           graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]),
                                           1, v2f32(0,y_cursor), string_from_cstring(tmp_text), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
        }
        switch (editor_state->tool_mode) {
            case EDITOR_TOOL_TILE_PAINTING: {
                y_cursor += 12;
                {
                    char tmp_text[1024]={};
                    snprintf(tmp_text, 1024, "current tile id: %d", editor_state->painting_tile_id);
                    software_framebuffer_draw_text(framebuffer,
                                                   graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]),
                                                   1, v2f32(0,y_cursor), string_from_cstring(tmp_text), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                }
            } break;
            case EDITOR_TOOL_TRIGGER_PLACEMENT: {
                y_cursor += 12;
                {
                    char tmp_text[1024]={};
                    snprintf(tmp_text, 1024, "trigger type: %.*s", trigger_placement_type_strings[editor_state->trigger_placement_type].length, trigger_placement_type_strings[editor_state->trigger_placement_type].data);
                    software_framebuffer_draw_text(framebuffer,
                                                   graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]),
                                                   1, v2f32(0,y_cursor), string_from_cstring(tmp_text), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                    /* specific trigger property menu: might require lots of buttons and stuff. */
                }
            } break;
        }
    }
    /* not using render commands here. I can trivially figure out what order most things should be... */
}

void update_and_render_editor(struct software_framebuffer* framebuffer, f32 dt) {
    struct render_commands commands = render_commands(editor_state->camera);

    commands.should_clear_buffer = true;
    commands.clear_buffer_color  = color32u8(100, 128, 148, 255);

    /* Maybe change the data structures later? */
    for (s32 tile_index = 0; tile_index < editor_state->tile_count; ++tile_index) {
        struct tile*                 current_tile = editor_state->tiles + tile_index;
        s32                          tile_id      = current_tile->id;
        struct tile_data_definition* tile_data    = tile_table_data + tile_id;
        image_id                     tex          = graphics_assets_get_image_by_filepath(&graphics_assets, tile_data->image_asset_location); 

        render_commands_push_image(&commands,
                                   graphics_assets_get_image_by_id(&graphics_assets, tex),
                                   rectangle_f32(current_tile->x * TILE_UNIT_SIZE,
                                                 current_tile->y * TILE_UNIT_SIZE,
                                                 TILE_UNIT_SIZE,
                                                 TILE_UNIT_SIZE),
                                   tile_data->sub_rectangle,
                                   color32f32(1,1,1,1), NO_FLAGS, BLEND_MODE_ALPHA);
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
