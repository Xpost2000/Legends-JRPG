/* TODO Need to clean up  */

/*
  NOTE:

  Doesn't technically account for a different screen resolution.
  (some parts anyways...)

  The framebuffer is always 640x480(or whatever resolution I decide it should always stay at),
  and I do allow the window to change size to scale up. So I have to bake lots of scaling to fix this later.
*/

local bool is_dragging(void) {
    return editor_state->drag_data.context != NULL;
}

local void set_drag_candidate_rectangle(void* context, v2f32 initial_mouse_worldspace, v2f32 initial_object_position, v2f32 initial_object_size) {
    editor_state->drag_data.context                 = context;
    editor_state->drag_data.initial_mouse_position  = initial_mouse_worldspace;
    editor_state->drag_data.initial_object_position = initial_object_position;
    editor_state->drag_data.initial_object_dimensions     = initial_object_size;
    editor_state->drag_data.has_size                          = true;
}

local void set_drag_candidate(void* context, v2f32 initial_mouse_worldspace, v2f32 initial_object_position) {
    editor_state->drag_data.context                 = context;
    editor_state->drag_data.initial_mouse_position  = initial_mouse_worldspace;
    editor_state->drag_data.initial_object_position = initial_object_position;
    editor_state->drag_data.has_size                          = false;
}

local void clear_drag_candidate(void) {
    editor_state->drag_data.context = 0;
}

/* little IMGUI */
/* no layout stuff, mostly just simple widgets to play with */
/* also bad code */
bool _imgui_mouse_button_left_down = false;
bool _imgui_mouse_button_right_down = false;
bool _imgui_any_intersection       = false;

/* 1 - left click, 2 - right click. Might be weird if you're not expecting it... */
/* TODO correct this elsweyr. */
s32 EDITOR_imgui_button(struct software_framebuffer* framebuffer, struct font_cache* font, struct font_cache* highlighted_font, f32 draw_scale, v2f32 position, string str) {
    f32 text_height = font_cache_text_height(font) * draw_scale;
    f32 text_width  = font_cache_text_width(font, str, draw_scale);

    s32 mouse_positions[2];
    bool left_clicked, right_clicked;
    get_mouse_buttons(&left_clicked, 0, &right_clicked);
    get_mouse_location(mouse_positions, mouse_positions+1);

    struct rectangle_f32 button_bounding_box = rectangle_f32(position.x, position.y, text_width, text_height);
    struct rectangle_f32 interaction_bounding_box = rectangle_f32(mouse_positions[0], mouse_positions[1], 2, 2);

    if (rectangle_f32_intersect(button_bounding_box, interaction_bounding_box)) {
        _imgui_any_intersection = true;
        if (!_imgui_mouse_button_left_down & left_clicked)   _imgui_mouse_button_left_down  = true;
        if (!_imgui_mouse_button_right_down & right_clicked) _imgui_mouse_button_right_down = true;

        software_framebuffer_draw_text(framebuffer, highlighted_font, draw_scale, position, (str), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
        if (!left_clicked && _imgui_mouse_button_left_down) {
            _imgui_mouse_button_left_down = false;
            return 1;
        }
        if (!right_clicked && _imgui_mouse_button_right_down) {
            _imgui_mouse_button_right_down = false;
            return 2;
        }
    } else {
        software_framebuffer_draw_text(framebuffer, font, draw_scale, position, (str), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
    }

    return 0;
}
/* little IMGUI  */

void editor_clear_all_allocations(struct editor_state* state) {
    state->tile_count                     = 0;
    state->trigger_level_transition_count = 0;
    state->entity_chest_count             = 0;
}

void editor_clear_all(struct editor_state* state) {
    editor_clear_all_allocations(state);

    state->camera.xy       = v2f32(0,0);
    state->camera.zoom     = 1;
    /* not centered to simplify code */
    /* state->camera.centered = 1; */
}

void editor_initialize(struct editor_state* state) {
    editor_state->arena                             = &editor_arena;
    editor_state->tile_capacity                     = 8192;
    editor_state->trigger_level_transition_capacity = 1024;
    editor_state->entity_chest_capacity             = 1024;
    state->tiles                                    = memory_arena_push(state->arena, state->tile_capacity                     * sizeof(*state->tiles));
    state->trigger_level_transitions                = memory_arena_push(state->arena, state->trigger_level_transition_capacity * sizeof(*state->trigger_level_transitions));
    state->entity_chests                            = memory_arena_push(state->arena, state->entity_chest_capacity             * sizeof(*state->entity_chests));
    editor_clear_all(state);
}

/* While I could use one serialization function. In the case the formats deviate slightly... */
void editor_serialize_area(struct binary_serializer* serializer) {
    if (serializer->mode == BINARY_SERIALIZER_READ)
        editor_clear_all_allocations(editor_state);

    u32 version_id = CURRENT_LEVEL_AREA_VERSION;
    _debugprintf("reading version");
    serialize_u32(serializer, &version_id);
    _debugprintf("reading default player spawn");
    serialize_f32(serializer, &editor_state->default_player_spawn.x);
    serialize_f32(serializer, &editor_state->default_player_spawn.y);
    _debugprintf("reading tiles");
    Serialize_Fixed_Array(serializer, s32, editor_state->tile_count, editor_state->tiles);

    if (version_id >= 1) {
        Serialize_Fixed_Array(serializer, s32, editor_state->trigger_level_transition_count, editor_state->trigger_level_transitions);
        if (version_id >= 2) {
            Serialize_Fixed_Array(serializer, s32, editor_state->entity_chest_count, editor_state->entity_chests);
        }
    }
}

void editor_remove_tile_at(v2f32 point_in_tilespace) {
    s32 where_x = point_in_tilespace.x;
    s32 where_y = point_in_tilespace.y;

    for (s32 index = 0; index < editor_state->tile_count; ++index) {
        struct tile* current_tile = editor_state->tiles + index;

        /* override */
        if (current_tile->x == where_x && current_tile->y == where_y) {
            if (current_tile == editor_state->last_selected) editor_state->last_selected = 0;

            current_tile->id = 0;
            editor_state->tiles[index] = editor_state->tiles[--editor_state->tile_count];
            return;
        }
    }
}
void editor_remove_level_transition_trigger_at(v2f32 point_in_tilespace) {
    for (s32 index = 0; index < editor_state->trigger_level_transition_count; ++index) {
        struct trigger_level_transition* current_trigger = editor_state->trigger_level_transitions + index;

        if (rectangle_f32_intersect(
                current_trigger->bounds,
                rectangle_f32(point_in_tilespace.x, point_in_tilespace.y, 0.25, 0.25)
            )) {
            editor_state->trigger_level_transitions[index] = editor_state->trigger_level_transitions[--editor_state->trigger_level_transition_count];
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
            editor_state->last_selected = current_tile;
            return;
        }
    }

    struct tile* new_tile = editor_state->tiles + (editor_state->tile_count++);
    new_tile->id = editor_state->painting_tile_id;
    new_tile->x  = where_x;
    new_tile->y  = where_y;
    editor_state->last_selected = new_tile;
}

/* TODO find a better way for the camera to get screen dimensions, I know my fixed resolution though so it's fine... Always guarantee we have a multiple of our fixed resolution to simplify things */
void editor_place_or_drag_level_transition_trigger(v2f32 point_in_tilespace) {
    if (is_dragging()) {
        return; 
    }

    {
        for (s32 index = 0; index < editor_state->trigger_level_transition_count; ++index) {
            struct trigger_level_transition* current_trigger = editor_state->trigger_level_transitions + index;

            if (rectangle_f32_intersect(
                    current_trigger->bounds,
                    rectangle_f32(point_in_tilespace.x, point_in_tilespace.y, 0.25, 0.25)
                )) {
                /* TODO drag candidate */
                editor_state->last_selected = current_trigger;
                set_drag_candidate_rectangle(current_trigger, get_mouse_in_tile_space(&editor_state->camera, REAL_SCREEN_WIDTH, REAL_SCREEN_HEIGHT),
                                             v2f32(current_trigger->bounds.x, current_trigger->bounds.y),
                                             v2f32(current_trigger->bounds.w, current_trigger->bounds.h));
                return;
            }
        }


        /* otherwise no touch, place a new one at default size 1 1 */
        struct trigger_level_transition* new_transition_trigger = &editor_state->trigger_level_transitions[editor_state->trigger_level_transition_count++];
        new_transition_trigger->bounds.x = point_in_tilespace.x;
        new_transition_trigger->bounds.y = point_in_tilespace.y;
        new_transition_trigger->bounds.w = 1;
        new_transition_trigger->bounds.h = 1;
        editor_state->last_selected      = new_transition_trigger;
    }
}

void editor_place_or_drag_chest(v2f32 point_in_tilespace) {
    if (is_dragging()) {
        return;
    }

    {
        for (s32 index = 0; index < editor_state->entity_chest_count; ++index) {
            struct entity_chest* current_chest = editor_state->entity_chests + index;

            if (rectangle_f32_intersect(
                    rectangle_f32(current_chest->position.x,
                                  current_chest->position.y,
                                  current_chest->scale.x,
                                  current_chest->scale.y),
                    rectangle_f32(point_in_tilespace.x, point_in_tilespace.y, 0.25, 0.25)
                )) {
                /* TODO drag candidate */
                editor_state->last_selected = current_chest;
                set_drag_candidate(current_chest, get_mouse_in_tile_space(&editor_state->camera, REAL_SCREEN_WIDTH, REAL_SCREEN_HEIGHT),
                                   v2f32(current_chest->position.x, current_chest->position.y));
                return;
            }
        }


        /* otherwise no touch, place a new one at default size 1 1 */
        struct entity_chest* new_chest = &editor_state->entity_chests[editor_state->entity_chest_count++];
        new_chest->position.x = point_in_tilespace.x;
        new_chest->position.y = point_in_tilespace.y;
        new_chest->scale.x = 1;
        new_chest->scale.y = 1;
        editor_state->last_selected      = new_chest;
    }
}

void editor_remove_chest_at(v2f32 point_in_tilespace) {
    for (s32 index = 0; index < editor_state->entity_chest_count; ++index) {
        struct entity_chest* current_chest = editor_state->entity_chests + index;

        if (rectangle_f32_intersect(
                rectangle_f32(current_chest->position.x,
                              current_chest->position.y,
                              current_chest->scale.x,
                              current_chest->scale.y),
                rectangle_f32(point_in_tilespace.x, point_in_tilespace.y, 0.25, 0.25)
            )) {
            editor_state->entity_chests[index] = editor_state->entity_chests[--editor_state->entity_chest_count];
            return;
        }
    }
}

/* hopefully I never need to unproject. */
/* I mean I just do the reverse of my projection so it's okay I guess... */

/* camera should know the world dimensions but okay */
local void handle_rectangle_dragging_and_scaling(void) {
    bool left_clicked   = 0;
    bool right_clicked  = 0;
    bool middle_clicked = 0;

    if (editor_state->tab_menu_open == TAB_MENU_CLOSED) {
        get_mouse_buttons(&left_clicked,
                          &middle_clicked,
                          &right_clicked);
    }

    if (is_dragging()) {
        /* when using shift you will write the changes! so be careful! */
        if (left_clicked) {
            /* NOTE this is not used yet because we don't have anything that is *not* grid aligned */
            v2f32                 displacement_delta = v2f32_sub(v2f32_floor(editor_state->drag_data.initial_mouse_position),
                                                                 editor_state->drag_data.initial_object_position);
            struct rectangle_f32* object_rectangle   = (struct rectangle_f32*) editor_state->drag_data.context;

            v2f32 mouse_position_in_tilespace_rounded = get_mouse_in_tile_space_integer(&editor_state->camera, REAL_SCREEN_WIDTH, REAL_SCREEN_HEIGHT);

            if (is_key_down(KEY_SHIFT) && editor_state->drag_data.has_size) {
                object_rectangle->w =  mouse_position_in_tilespace_rounded.x - editor_state->drag_data.initial_object_position.x;
                object_rectangle->h =  mouse_position_in_tilespace_rounded.y - editor_state->drag_data.initial_object_position.y;

                if (object_rectangle->w <= 0) object_rectangle->w = 1;
                if (object_rectangle->h <= 0) object_rectangle->h = 1;

#if 0
                /* NOTE buggy */
                if (object_rectangle->w < 0) {
                    object_rectangle->w *= -1;
                    object_rectangle->x -= object_rectangle->w;
                }

                if (object_rectangle->h < 0) {
                    object_rectangle->h *= -1;
                    object_rectangle->y -= object_rectangle->h;
                }
#endif
            } else {
                _debugprintf("<%f, %f> initial mpos", editor_state->drag_data.initial_mouse_position.x, editor_state->drag_data.initial_mouse_position.y);
                _debugprintf("<%f, %f> initial obj pos", editor_state->drag_data.initial_object_position.x, editor_state->drag_data.initial_object_position.y);
                _debugprintf("<%f, %f> displacement delta", displacement_delta.x, displacement_delta.y);
                object_rectangle->x = mouse_position_in_tilespace_rounded.x - displacement_delta.x;
                object_rectangle->y = mouse_position_in_tilespace_rounded.y - displacement_delta.y;
            }
        } else {
            editor_state->drag_data.context = 0;
        }
    }
}

local void handle_editor_tool_mode_input(struct software_framebuffer* framebuffer) {
    bool left_clicked   = 0;
    bool right_clicked  = 0;
    bool middle_clicked = 0;

    /* do not check for mouse input when our special tab menu is open */
    if (editor_state->tab_menu_open == TAB_MENU_CLOSED) {
        get_mouse_buttons(&left_clicked,
                          &middle_clicked,
                          &right_clicked);
    }

    /* refactor later */
    s32 mouse_location[2];
    get_mouse_location(mouse_location, mouse_location+1);

    v2f32 world_space_mouse_location =
        camera_project(&editor_state->camera, v2f32(mouse_location[0], mouse_location[1]), REAL_SCREEN_WIDTH, REAL_SCREEN_HEIGHT);

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
            if (!editor_state->viewing_loaded_area) {
                if (left_clicked) {
                    editor_place_tile_at(tile_space_mouse_location);
                } else if (right_clicked) {
                    editor_remove_tile_at(tile_space_mouse_location);
                }
            }
        } break;
        case EDITOR_TOOL_SPAWN_PLACEMENT: {
            if (!editor_state->viewing_loaded_area) {
                if (left_clicked) {
                    editor_state->default_player_spawn.x = world_space_mouse_location.x;
                    editor_state->default_player_spawn.y = world_space_mouse_location.y;
                }
            }
        } break;
        case EDITOR_TOOL_ENTITY_PLACEMENT: {
            switch (editor_state->entity_placement_type) {
                /* NOTE for chest mode we should have a highlight tooltip to peak at the items in the chest. For quick assessment of things */
                /* I guess same for the triggers... */
                case ENTITY_PLACEMENT_TYPE_CHEST: {
                    if (left_clicked) {
                        editor_place_or_drag_chest(tile_space_mouse_location);
                    } else if (right_clicked) {
                        editor_remove_chest_at(tile_space_mouse_location);
                    }
                } break;
            }
        } break;
        case EDITOR_TOOL_TRIGGER_PLACEMENT: {
            /* wrap_around_key_selection(KEY_LEFT, KEY_RIGHT, &editor_state->trigger_placement_type, 0, 2); */
            switch (editor_state->trigger_placement_type) {
                case TRIGGER_PLACEMENT_TYPE_LEVEL_TRANSITION: {
                    if (!editor_state->viewing_loaded_area) {
                        if (left_clicked) {
                            /* NOTE check the trigger mode */
                            editor_place_or_drag_level_transition_trigger(tile_space_mouse_location);
                        } else if (right_clicked) {
                            editor_remove_level_transition_trigger_at(tile_space_mouse_location);
                        }
                    } else {
                        if (left_clicked) {
                            assertion(editor_state->last_selected);
                            struct trigger_level_transition* trigger = editor_state->last_selected;
                            cstring_copy(editor_state->loaded_area_name, trigger->target_level, 128);
                            _debugprintf("\"%s\"", trigger->target_level);

                            s32 mouse_location[2];
                            get_mouse_location(mouse_location, mouse_location+1);

                            v2f32 world_space_mouse_location =
                                camera_project(&editor_state->camera, v2f32(mouse_location[0], mouse_location[1]), REAL_SCREEN_WIDTH, REAL_SCREEN_HEIGHT);

                            /* for tiles */
                            v2f32 tile_space_mouse_location = world_space_mouse_location; {
                                tile_space_mouse_location.x = floorf(world_space_mouse_location.x / TILE_UNIT_SIZE);
                                tile_space_mouse_location.y = floorf(world_space_mouse_location.y / TILE_UNIT_SIZE);
                            }

                            trigger->spawn_location = tile_space_mouse_location;
                        }

                        if (is_key_pressed(KEY_RETURN)) {
                            editor_state->viewing_loaded_area = false;
                        }
                    }
                } break;
            }
        } break;
        default: {
        } break;
    }

    handle_rectangle_dragging_and_scaling();
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

    /* I'm sure the animation is very pretty but in editor mode I'm in a rush lol */
    f32 timescale = 3;

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
                        if (editor_state->serialize_menu_reason == 0) {
                            menu_state->animation_state = UI_PAUSE_MENU_TRANSITION_CLOSING;
                            menu_state->transition_t = 0;
                        } else {
                            game_state_set_ui_state(game_state, state->last_ui_state);
                        }
                    }        

                    wrap_around_key_selection(KEY_UP, KEY_DOWN, &menu_state->selection, 0, array_count(item_positions));

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
                                serialize_level_area(game_state, &serializer1, &game_state->loaded_area, true);
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
                                editor_state->serialize_menu_reason = 0;
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
                                editor_state->serialize_menu_reason = 0;
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

                    /* handle inputs here */
                    if (editor_state->serialize_menu_t >= 1) {
                        editor_state->serialize_menu_t = 1;

                        if (editor_state->serialize_menu_mode == 1) {
                            start_text_edit(editor_state->current_save_name, cstring_length(editor_state->current_save_name));

                            if (is_key_pressed(KEY_RETURN)) {
                                end_text_edit(editor_state->current_save_name, array_count(editor_state->current_save_name));

                                /* NOTE need to be careful, since it doesn't know where the game path is... */
                                string to_save_as = string_concatenate(&scratch_arena, string_literal("areas/"), string_from_cstring(editor_state->current_save_name));
                                _debugprintf("save as: %.*s\n", to_save_as.length, to_save_as.data);
                                struct binary_serializer serializer = open_write_file_serializer(to_save_as);
                                editor_serialize_area(&serializer);
                                serializer_finish(&serializer);
                            }
                        } else {
                        
                        }
                    } else {
                        
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
        struct font_cache* font             = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_STEEL]);
        struct font_cache* highlighted_font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]);
        v2f32 draw_position = v2f32(0, 15);
        draw_position.x = lerp_f32(-200, 80, editor_state->serialize_menu_t);
        
        switch (editor_state->serialize_menu_mode) {
            case 1: {
                software_framebuffer_draw_text(framebuffer, font, font_scale, draw_position, string_literal("SAVE LEVEL"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                draw_position.y += font_scale * 12 * 3;
                {
                    char tmp_text[1024] = {};
                    snprintf(tmp_text, 1024, "SAVE AS: %s", current_text_buffer());
                    _debugprintf("\"%s\"\n", current_text_buffer());
                    software_framebuffer_draw_text(framebuffer, font, font_scale, draw_position, string_from_cstring(tmp_text), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                }
            } break;
            case 2: {
                software_framebuffer_draw_text(framebuffer, font, font_scale, draw_position, string_literal("LOAD LEVEL"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                (draw_position.y += font_scale * 12 * 2);

                /* since this listing is actually done immediately/live, technically we hot reload directories... cool! */
                struct directory_listing listing = directory_listing_list_all_files_in(&scratch_arena, string_literal("areas/"));

                if (listing.count <= 2) {
                    software_framebuffer_draw_text(framebuffer, font, 2, draw_position, string_literal("(no areas)"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                } else {
                    /* skip . and ../ */
                    for (s32 index = 2; index < listing.count; ++index) {
                        struct directory_file* current_file = listing.files + index;
                        draw_position.y += 2 * 12 * 1;
                        if(EDITOR_imgui_button(framebuffer, font, highlighted_font, 2, draw_position, string_from_cstring(current_file->name))) {
                            switch (editor_state->serialize_menu_reason) {
                                case 0: {
                                    struct binary_serializer serializer = open_read_file_serializer(string_concatenate(&scratch_arena, string_literal("areas/"), string_from_cstring(current_file->name)));
                                    editor_serialize_area(&serializer);
                                    serializer_finish(&serializer);

                                    cstring_copy(current_file->name, editor_state->current_save_name, array_count(editor_state->current_save_name));

                                    menu_state->animation_state = UI_PAUSE_MENU_TRANSITION_IN;
                                    menu_state->transition_t = 0;
                                    editor_state->serialize_menu_mode = 0;
                                    return;
                                } break;
                                case 1: {
                                    struct binary_serializer serializer = open_read_file_serializer(string_concatenate(&scratch_arena, string_literal("areas/"), string_from_cstring(current_file->name)));
                                    cstring_copy(current_file->name, editor_state->loaded_area_name, array_count(editor_state->loaded_area_name));
                                    serialize_level_area(state, &serializer, &editor_state->loaded_area, true);
                                    serializer_finish(&serializer);
                                    editor_state->viewing_loaded_area = true;
                                } break;
                            }
                        }
                        /* software_framebuffer_draw_text(framebuffer, font, 2, draw_position, string_from_cstring(current_file->name), color32f32(1,1,1,1), BLEND_MODE_ALPHA); */
                    }
                }
            } break;
        }
    }
}

/* Editor code will always be a little nasty lol */
local void update_and_render_editor_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    s32 mouse_location[2];
    get_mouse_location(mouse_location, mouse_location+1);

    bool left_clicked   = 0; bool right_clicked  = 0; bool middle_clicked = 0;
    get_mouse_buttons(&left_clicked, &middle_clicked, &right_clicked);

    v2f32 world_space_mouse_location =
        camera_project(&editor_state->camera, v2f32(mouse_location[0], mouse_location[1]), REAL_SCREEN_WIDTH, REAL_SCREEN_HEIGHT);

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
    if (is_key_down(KEY_SHIFT) && is_key_pressed(KEY_TAB)) {
        editor_state->tab_menu_open ^= TAB_MENU_OPEN_BIT;
        editor_state->tab_menu_open ^= TAB_MENU_SHIFT_BIT;
    } else if (is_key_down(KEY_CTRL) && is_key_pressed(KEY_TAB)) {
        editor_state->tab_menu_open ^= TAB_MENU_OPEN_BIT;
        editor_state->tab_menu_open ^= TAB_MENU_CTRL_BIT;
    } else if (is_key_pressed(KEY_TAB)) {
        editor_state->tab_menu_open ^= TAB_MENU_OPEN_BIT;

        if (!(editor_state->tab_menu_open & TAB_MENU_OPEN_BIT)) editor_state->tab_menu_open = 0;
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

    if (editor_state->tab_menu_open & TAB_MENU_OPEN_BIT) {
        software_framebuffer_draw_quad(framebuffer,
                                       rectangle_f32(0, 0, framebuffer->width, framebuffer->height),
                                       color32u8(0,0,0,200), BLEND_MODE_ALPHA);
        /* tool selector */
        struct font_cache* font             = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_STEEL]);
        struct font_cache* highlighted_font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]);

        if (editor_state->tab_menu_open & TAB_MENU_SHIFT_BIT) {
            f32 draw_cursor_y = 30;
            for (s32 index = 0; index < array_count(editor_tool_mode_strings)-1; ++index) {
                if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 3, v2f32(100, draw_cursor_y), editor_tool_mode_strings[index])) {
                    editor_state->tab_menu_open = 0;
                    editor_state->tool_mode     = index;
                    editor_state->last_selected = 0;
                    break;
                }
                draw_cursor_y += 12 * 1.5 * 3;
            }
        } else if (editor_state->tab_menu_open & TAB_MENU_CTRL_BIT) {
            switch (editor_state->tool_mode) {
                /* I would show images, but this is easier for now */
                case EDITOR_TOOL_TILE_PAINTING: {} break;
                case EDITOR_TOOL_TRIGGER_PLACEMENT: {
                    switch (editor_state->trigger_placement_type) {
                        case TRIGGER_PLACEMENT_TYPE_LEVEL_TRANSITION: {
                            f32 draw_cursor_y = 30;
                            struct trigger_level_transition* trigger = editor_state->last_selected;

                            char tmp_string[1024] = {};
                            snprintf(tmp_string, 1024, "Transition Area: \"%s\" <%f, %f> (SET?)", trigger->target_level, trigger->spawn_location.x, trigger->spawn_location.y);
                            if(EDITOR_imgui_button(framebuffer, font, highlighted_font, 2, v2f32(16, draw_cursor_y), string_from_cstring(tmp_string))) {
                                /* open another file selection menu */
                                editor_state->serialize_menu_mode   = 2;
                                editor_state->serialize_menu_reason = 1;
                                struct ui_pause_menu* menu_state = &state->ui_pause;
                                game_state_set_ui_state(game_state, UI_STATE_PAUSE);
                                game_state->ui_pause.transition_t    = 0;
                                game_state->ui_pause.selection       = 0;
                                zero_array(game_state->ui_pause.shift_t);
                                menu_state->animation_state = UI_PAUSE_MENU_NO_ANIM;
                                editor_state->tab_menu_open = 0;
                            }

                            draw_cursor_y += 12 * 1.2 * 2;
                            if(EDITOR_imgui_button(framebuffer, font, highlighted_font, 2, v2f32(16, draw_cursor_y), string_concatenate(&scratch_arena, string_literal("Facing Direction: "), facing_direction_strings[trigger->new_facing_direction]))) {
                                trigger->new_facing_direction += 1;
                                if (trigger->new_facing_direction > 4) trigger->new_facing_direction = 0;
                            }
                        } break;
                    }
                } break;
                case EDITOR_TOOL_ENTITY_PLACEMENT: {
                    switch (editor_state->entity_placement_type) {
                        case ENTITY_PLACEMENT_TYPE_CHEST: {
                            f32 draw_cursor_y = 70;
                            struct entity_chest* chest = editor_state->last_selected;
                            software_framebuffer_draw_text(framebuffer, font, 2, v2f32(10, 10), string_literal("Chest Items"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);

                            {
                                /* sort bar */
                                f32 draw_cursor_x = 30;

                                for (unsigned index = 0; index < array_count(item_type_strings); ++index) {
                                    string text = item_type_strings[index];

                                    if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 2, v2f32(draw_cursor_x, 35), text)) {
                                        /* should be mask */
                                        editor_state->chest_property_menu.item_sort_filter = index;
                                    }

                                    draw_cursor_x += font_cache_text_width(font, text, 2) * 1.15;
                                }
                            }

                            if (editor_state->chest_property_menu.adding_item) {
                                if (is_key_down(KEY_UP)) {
                                    editor_state->chest_property_menu.item_list_scroll_y -= 100 * dt;
                                } else if (is_key_down(KEY_DOWN)) {
                                    editor_state->chest_property_menu.item_list_scroll_y += 100 * dt;
                                } else if (is_key_pressed(KEY_HOME)) {
                                    editor_state->chest_property_menu.item_list_scroll_y = 0;
                                }

                                for (unsigned index = 0; index < MAX_ITEMS_DATABASE_SIZE; ++index) {
                                    struct item_def* item_base = item_database + index;

                                    if (editor_state->chest_property_menu.item_sort_filter) {
                                        if (editor_state->chest_property_menu.item_sort_filter != item_base->type)
                                            continue;
                                    }

                                    /* TODO more flags */
                                    if (item_base->id_name.length > 0) {
                                        /* TODO make a temporary printing function or something. God. */
                                        char tmp[255] = {};
                                        snprintf(tmp, 255, "(%.*s) %.*s", item_base->id_name.length, item_base->id_name.data, item_base->name.length, item_base->name.data);

                                        if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 1.3, v2f32(16, draw_cursor_y + editor_state->chest_property_menu.item_list_scroll_y), string_from_cstring(tmp))) {
                                            entity_inventory_add((struct entity_inventory*)&chest->inventory, 16, item_get_id(item_base));
                                            editor_state->chest_property_menu.adding_item = false;
                                            break;
                                        }

                                        draw_cursor_y += 12 * 1.2 * 1.3;
                                    }
                                }
                            } else {
                                if (chest->inventory.item_count > 0) {
                                    char tmp[255] = {};
                                    for (s32 index = 0; index < chest->inventory.item_count; ++index) {
                                        struct item_instance* item      = chest->inventory.items + index;
                                        struct item_def*      item_base = item_database_find_by_id(item->item);
                                        snprintf(tmp, 255, "(%.*s) %.*s - %d/%d", item_base->id_name.length, item_base->id_name.data, item_base->name.length, item_base->name.data, item->count, item_base->max_stack_value);
                                        draw_cursor_y += 12 * 1.2 * 1.5;

                                        s32 button_response = (EDITOR_imgui_button(framebuffer, font, highlighted_font, 1.5, v2f32(16, draw_cursor_y), string_from_cstring(tmp)));
                                        if(button_response == 1) {
                                            /* ?clone? Not exactly expected behavior I'd say lol. */
                                            entity_inventory_add((struct entity_inventory*)&chest->inventory, 16, item->item);
                                        } else if (button_response == 2) {
                                            if (is_key_down(KEY_SHIFT)) {
                                                entity_inventory_remove_item((struct entity_inventory*)&chest->inventory, index, true);
                                            } else {
                                                entity_inventory_remove_item((struct entity_inventory*)&chest->inventory, index, false);
                                            }
                                        }
                                    }
                                } else {
                                    software_framebuffer_draw_text(framebuffer, font, 2, v2f32(10, draw_cursor_y), string_literal("(no items)"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                                }

                                if(EDITOR_imgui_button(framebuffer, font, highlighted_font, 2, v2f32(150, 10), string_literal("(add item)"))) {
                                    /* pop up should just replace the menu */
                                    /* for now just add a test item */
                                    /* entity_inventory_add(&chest->inventory, 16, item_id_make(string_literal("item_sardine_fish_5"))); */
                                    editor_state->chest_property_menu.adding_item        = true;
                                    editor_state->chest_property_menu.item_list_scroll_y = 0;
                                }
                            }
                        } break;
                    }
                } break;
            }
        } else {
            switch (editor_state->tool_mode) {
                /* I would show images, but this is easier for now */
                case EDITOR_TOOL_TILE_PAINTING: {
                    f32 draw_cursor_y = 30 + editor_state->tile_painting_property_menu.item_list_scroll_y;
                    if (is_key_down(KEY_UP)) {
                        editor_state->tile_painting_property_menu.item_list_scroll_y -= 100 * dt;
                    } else if (is_key_down(KEY_DOWN)) {
                        editor_state->tile_painting_property_menu.item_list_scroll_y += 100 * dt;
                    } else if (is_key_pressed(KEY_HOME)) {
                        editor_state->tile_painting_property_menu.item_list_scroll_y = 0;
                    }
                    for (s32 index = 0; index < tile_table_data_count; ++index) {
                        if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 2, v2f32(16, draw_cursor_y), tile_table_data[index].name)) {
                            editor_state->tab_menu_open    = 0;
                            editor_state->painting_tile_id = index;
                            break;
                        }
                        draw_cursor_y += 12 * 1.2 * 2;
                    }
                } break;
                case EDITOR_TOOL_ENTITY_PLACEMENT: {
                    f32 draw_cursor_y = 30;
                    for (s32 index = 0; index < array_count(entity_placement_type_strings)-1; ++index) {
                        if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 2, v2f32(16, draw_cursor_y), entity_placement_type_strings[index])) {
                            editor_state->tab_menu_open          = 0;
                            editor_state->entity_placement_type = index;
                            break;
                        }
                        draw_cursor_y += 12 * 1.2 * 2;
                    }
                } break;
                case EDITOR_TOOL_TRIGGER_PLACEMENT: {
                    f32 draw_cursor_y = 30;
                    for (s32 index = 0; index < array_count(trigger_placement_type_strings)-1; ++index) {
                        if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 2, v2f32(16, draw_cursor_y), trigger_placement_type_strings[index])) {
                            editor_state->tab_menu_open          = 0;
                            editor_state->trigger_placement_type = index;
                            break;
                        }
                        draw_cursor_y += 12 * 1.2 * 2;
                    }
                } break;
            }
        }
    }
    /* not using render commands here. I can trivially figure out what order most things should be... */
}

void update_and_render_editor(struct software_framebuffer* framebuffer, f32 dt) {
    struct render_commands commands = render_commands(editor_state->camera);

    commands.should_clear_buffer = true;
    commands.clear_buffer_color  = color32u8(100, 128, 148, 255);

    if (editor_state->viewing_loaded_area) {
        /* yeah this is a big mess */
        render_area(&commands, &editor_state->loaded_area);
        if (editor_state->last_selected && editor_state->tool_mode == EDITOR_TOOL_TRIGGER_PLACEMENT) {
            struct trigger_level_transition* trigger = editor_state->last_selected;
            render_commands_push_quad(&commands, rectangle_f32(trigger->spawn_location.x * TILE_UNIT_SIZE, trigger->spawn_location.y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE),
                                      color32u8(0, 255, 255, normalized_sinf(global_elapsed_time*4) * 0.5*255 + 64), BLEND_MODE_ALPHA);
        }
    } else {
        /* Maybe change the data structures later? */
        /* TODO do it lazy mode. Once only */
        qsort(editor_state->tiles, editor_state->tile_count, sizeof(*editor_state->tiles), _qsort_tile);

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
        struct font_cache* font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_BLUE]);
        for (s32 trigger_level_transition_index = 0; trigger_level_transition_index < editor_state->trigger_level_transition_count; ++trigger_level_transition_index) {
            struct trigger_level_transition* current_trigger = editor_state->trigger_level_transitions + trigger_level_transition_index;
            if (editor_state->last_selected == current_trigger) {
                render_commands_push_quad(&commands, rectangle_f32(current_trigger->bounds.x * TILE_UNIT_SIZE, current_trigger->bounds.y * TILE_UNIT_SIZE,
                                                                   current_trigger->bounds.w * TILE_UNIT_SIZE, current_trigger->bounds.h * TILE_UNIT_SIZE),
                                          color32u8(255, 255, 255, normalized_sinf(global_elapsed_time*2) * 0.5*255 + 64), BLEND_MODE_ALPHA);
            } else {
                render_commands_push_quad(&commands, rectangle_f32(current_trigger->bounds.x * TILE_UNIT_SIZE, current_trigger->bounds.y * TILE_UNIT_SIZE,
                                                                   current_trigger->bounds.w * TILE_UNIT_SIZE, current_trigger->bounds.h * TILE_UNIT_SIZE),
                                          color32u8(255, 0, 0, normalized_sinf(global_elapsed_time*2) * 0.5*255 + 64), BLEND_MODE_ALPHA);
            }
            /* NOTE display a visual denoting facing direction on transition */
            render_commands_push_text(&commands, font, 1, v2f32(current_trigger->bounds.x * TILE_UNIT_SIZE, current_trigger->bounds.y * TILE_UNIT_SIZE), string_literal("(level\ntransition\ntrigger)"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
        }

        for (s32 chest_index = 0; chest_index < editor_state->entity_chest_count; ++chest_index) {
            struct entity_chest* current_chest = editor_state->entity_chests + chest_index;

            render_commands_push_image(&commands,
                                       graphics_assets_get_image_by_id(&graphics_assets, chest_closed_img),
                                       rectangle_f32(current_chest->position.x * TILE_UNIT_SIZE,
                                                     current_chest->position.y * TILE_UNIT_SIZE,
                                                     current_chest->scale.x * TILE_UNIT_SIZE,
                                                     current_chest->scale.y * TILE_UNIT_SIZE),
                                       RECTANGLE_F32_NULL,
                                       color32f32(1,1,1,1), NO_FLAGS, BLEND_MODE_ALPHA);

            if (editor_state->last_selected == current_chest) {
                render_commands_push_quad(&commands, rectangle_f32(current_chest->position.x * TILE_UNIT_SIZE,
                                                                   current_chest->position.y * TILE_UNIT_SIZE,
                                                                   current_chest->scale.x    * TILE_UNIT_SIZE,
                                                                   current_chest->scale.y    * TILE_UNIT_SIZE),
                                          color32u8(255, 0, 0, normalized_sinf(global_elapsed_time*2) * 0.5 *255 + 64), BLEND_MODE_ALPHA);
            }
        }
        
        render_commands_push_quad(&commands, rectangle_f32(editor_state->default_player_spawn.x, editor_state->default_player_spawn.y, TILE_UNIT_SIZE/4, TILE_UNIT_SIZE/4),
                                  color32u8(0, 255, 0, normalized_sinf(global_elapsed_time*4) * 0.5*255 + 64), BLEND_MODE_ALPHA);

        switch (editor_state->tool_mode) {
            case 0: {
                {
                    s32 mouse_location[2];
                    get_mouse_location(mouse_location, mouse_location+1);

                    v2f32 world_space_mouse_location =
                        camera_project(&editor_state->camera, v2f32(mouse_location[0], mouse_location[1]), REAL_SCREEN_WIDTH, REAL_SCREEN_HEIGHT);
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
    }

    /* cursor ghost */
    software_framebuffer_render_commands(framebuffer, &commands);
    _imgui_any_intersection = false;
}

