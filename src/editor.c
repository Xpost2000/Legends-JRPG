/*
  NOTE: Level editor does not use the VFS (or it shouldn't anyways.)

  Level testing behavior may no longer work, as they may now require dependence on a script file,
  which bares their name.
  
  I can modify it slightly I guess...
  Or I can try to include the script as part of the level which would be a pretty herculean job
  considering the relative complexity of the tools...

  TODO: Scrollable level loads?
  TODO: Needs a lot of conveniences or a rewrite.

  NOTE: Hmm. I think this is liable to become the first piece of code in this codebase I'm actually
  going to forget about how this thing functions.
*/

/*
  WHOCARES: May crash if touching the last_selected pointer incorrectly?
*/

local bool is_dragging(struct editor_drag_data* dragdata) {
    return dragdata->context != NULL;
}

local void set_drag_candidate_rectangle(struct editor_drag_data* dragdata, void* context, v2f32 initial_mouse_worldspace, v2f32 initial_object_position, v2f32 initial_object_size) {
    dragdata->context                 = context;
    dragdata->initial_mouse_position  = initial_mouse_worldspace;
    dragdata->initial_object_position = initial_object_position;
    dragdata->initial_object_dimensions     = initial_object_size;
    dragdata->has_size                          = true;
}

local void set_drag_candidate(struct editor_drag_data* dragdata, void* context, v2f32 initial_mouse_worldspace, v2f32 initial_object_position) {
    dragdata->context                 = context;
    dragdata->initial_mouse_position  = initial_mouse_worldspace;
    dragdata->initial_object_position = initial_object_position;
    dragdata->has_size                          = false;
}

local void clear_drag_candidate(struct editor_drag_data* dragdata) {
    dragdata->context = 0;
}

local v2f32 editor_get_world_space_mouse_location(void) {
    s32 mouse_location[2];
    get_mouse_location(mouse_location, mouse_location+1);
    return camera_project(&editor_state->camera, v2f32(mouse_location[0], mouse_location[1]), SCREEN_WIDTH, SCREEN_HEIGHT);
}

#include "editor_imgui.c"

void editor_clear_all_allocations(struct editor_state* state) {
    for (s32 index = 0; index < TILE_LAYER_COUNT; ++index) {
        tile_layer_clear(&state->editing_area.tile_layers[index]);
    }
    trigger_level_transition_list_clear(&state->editing_area.trigger_level_transitions);
    entity_chest_list_clear(&state->editing_area.chests);
    trigger_list_clear(&state->editing_area.triggers);
    level_area_entity_list_clear(&state->editing_area.load_entities);
    light_list_clear(&state->editing_area.lights);
    level_area_savepoint_list_clear(&state->editing_area.load_savepoints);
    level_area_battle_safe_square_list_clear(&state->editing_area.battle_safe_squares);
    position_marker_list_clear(&editor_state->editing_area.position_markers);
}

void editor_clear_all(struct editor_state* state) {
    editor_clear_all_allocations(state);

    state->camera.xy       = v2f32(0,0);
    state->camera.zoom     = 1;
    state->camera.centered = 1;
}

void editor_initialize(struct editor_state* state) {
    state->arena = &editor_arena;
    {
        s32 index;
        for (index = 0; index < TILE_LAYER_SCRIPTABLE_0; ++index) {
            state->editing_area.tile_layers[index] = tile_layer_reserved(state->arena, 65535*2);
        }
        for (; index < TILE_LAYER_COUNT; ++index) {
            state->editing_area.tile_layers[index] = tile_layer_reserved(state->arena, 4096);
        }
    }

    state->editing_area.trigger_level_transitions = trigger_level_transition_list_reserved(state->arena, 2048);
    state->editing_area.chests                    = entity_chest_list_reserved(state->arena, 2048);
    state->editing_area.triggers                  = trigger_list_reserved(state->arena, 2048);
    state->editing_area.load_entities             = level_area_entity_list_reserved(state->arena, 1024);
    state->editing_area.load_savepoints           = level_area_savepoint_list_reserved(state->arena, 128);
    state->editing_area.lights                    = light_list_reserved(state->arena, 512);
    state->editing_area.battle_safe_squares       = level_area_battle_safe_square_list_reserved(state->arena, 512*5);
    state->editing_area.position_markers          = position_marker_list_reserved(state->arena, 8192);
    cstring_copy("DefaultAreaName<>", state->level_settings.area_name, array_count(state->level_settings.area_name));
    editor_clear_all(state);
}

/* While I could use one serialization function. In the case the formats deviate slightly... */
void editor_remove_tile_at(v2f32 point_in_tilespace) {
    s32                where_x    = point_in_tilespace.x;
    s32                where_y    = point_in_tilespace.y;
    struct tile_layer* tile_layer = &editor_state->editing_area.tile_layers[editor_state->current_tile_layer];
    struct tile*       tile       = tile_layer_tile_at(tile_layer, where_x, where_y);

    if (tile == editor_state->last_selected) {
        editor_state->last_selected = 0;
    }

    if (tile) tile_layer_remove(tile_layer, tile - tile_layer->tiles);
}

void editor_remove_battle_tile_at(v2f32 point_in_tilespace) {
    s32 where_x = point_in_tilespace.x;
    s32 where_y = point_in_tilespace.y;

    level_area_battle_safe_square_list_remove_at(&editor_state->editing_area.battle_safe_squares, where_x, where_y);
}

local void editor_brush_remove_tile_at(v2f32 tile_space_mouse_location) {
    for (s32 y_index = 0; y_index < EDITOR_BRUSH_SQUARE_SIZE; ++y_index) {
        for (s32 x_index = 0; x_index < EDITOR_BRUSH_SQUARE_SIZE; ++x_index) {
            if (editor_brush_patterns[editor_state->editor_brush_pattern][y_index][x_index] == 1) {
                v2f32 point = tile_space_mouse_location;
                point.x += x_index - HALF_EDITOR_BRUSH_SQUARE_SIZE;
                point.y += y_index - HALF_EDITOR_BRUSH_SQUARE_SIZE;
                editor_remove_tile_at(point);
            }
        }
    }
}
void editor_remove_scriptable_trigger_at(v2f32 point_in_tilespace) {
    struct trigger* existing_trigger = trigger_list_trigger_at(&editor_state->editing_area.triggers, point_in_tilespace);
    if (existing_trigger == editor_state->last_selected)  {
        editor_state->last_selected = 0;
    } else if (existing_trigger) {
        trigger_list_remove(&editor_state->editing_area.triggers, existing_trigger - editor_state->editing_area.triggers.triggers);
    }
}
void editor_remove_level_transition_trigger_at(v2f32 point_in_tilespace) {
    struct trigger_level_transition* existing_trigger = trigger_level_transition_list_transition_at(&editor_state->editing_area.trigger_level_transitions, point_in_tilespace);
    if (existing_trigger == editor_state->last_selected)  {
        editor_state->last_selected = 0;
    } else if (existing_trigger) {
        trigger_level_transition_list_remove(&editor_state->editing_area.trigger_level_transitions, existing_trigger - editor_state->editing_area.trigger_level_transitions.transitions);
    }
}

void editor_place_tile_at(v2f32 point_in_tilespace) {
    s32 where_x = point_in_tilespace.x;
    s32 where_y = point_in_tilespace.y;

    struct tile_layer* tile_layer    = &editor_state->editing_area.tile_layers[editor_state->current_tile_layer];
    struct tile*       existing_tile = tile_layer_tile_at(tile_layer, where_x, where_y);

    if (existing_tile) {
        existing_tile->id           = editor_state->painting_tile_id;
        editor_state->last_selected = existing_tile;
    } else {
        struct tile* new_tile       = tile_layer_push(tile_layer, tile(editor_state->painting_tile_id, 0, where_x, where_y));
        editor_state->last_selected = new_tile;
    }
}

void editor_place_battle_tile_at(v2f32 point_in_tilespace) {
    s32 where_x = point_in_tilespace.x;
    s32 where_y = point_in_tilespace.y;


    struct level_area_battle_safe_square* existing_square = level_area_battle_safe_square_list_tile_at(&editor_state->editing_area.battle_safe_squares, where_x, where_y);
    if (!existing_square) {
        existing_square = level_area_battle_safe_square_list_push(&editor_state->editing_area.battle_safe_squares, level_area_battle_safe_square(where_x, where_y));
    }
}

local void editor_brush_place_tile_at(v2f32 tile_space_mouse_location) {
    for (s32 y_index = 0; y_index < EDITOR_BRUSH_SQUARE_SIZE; ++y_index) {
        for (s32 x_index = 0; x_index < EDITOR_BRUSH_SQUARE_SIZE; ++x_index) {
            if (editor_brush_patterns[editor_state->editor_brush_pattern][y_index][x_index] == 1) {
                v2f32 point = tile_space_mouse_location;
                point.x += x_index - HALF_EDITOR_BRUSH_SQUARE_SIZE;
                point.y += y_index - HALF_EDITOR_BRUSH_SQUARE_SIZE;
                editor_place_tile_at(point);
            }
        }
    }
}

/* TODO find a better way for the camera to get screen dimensions, I know my fixed resolution though so it's fine... Always guarantee we have a multiple of our fixed resolution to simplify things */
void editor_place_or_drag_level_transition_trigger(v2f32 point_in_tilespace) {
    if (is_dragging(&editor_state->drag_data)) {
        return; 
    }

    struct trigger_level_transition* existing_trigger = trigger_level_transition_list_transition_at(&editor_state->editing_area.trigger_level_transitions, point_in_tilespace);
    if (existing_trigger) {
        editor_state->last_selected = existing_trigger;
        set_drag_candidate_rectangle(&editor_state->drag_data, existing_trigger, get_mouse_in_tile_space(&editor_state->camera, SCREEN_WIDTH, SCREEN_HEIGHT),
                                     v2f32(existing_trigger->bounds.x, existing_trigger->bounds.y),
                                     v2f32(existing_trigger->bounds.w, existing_trigger->bounds.h));
    } else {
        editor_state->last_selected = trigger_level_transition_list_push(
            &editor_state->editing_area.trigger_level_transitions,
            trigger_level_transition(
                rectangle_f32(point_in_tilespace.x, point_in_tilespace.y, 1, 1),
                string_literal(""),
                DIRECTION_RETAINED,
                v2f32(0, 0)
            )
        );
    }
}

void editor_place_or_drag_position_marker(v2f32 point_in_tilespace) {
    if (is_dragging(&editor_state->drag_data)) {
        return;
    }

    struct position_marker* marker = position_marker_list_find_marker_at_with_rect(&editor_state->editing_area.position_markers, point_in_tilespace);

    if (!marker) {
        editor_state->last_selected = position_marker_list_push(&editor_state->editing_area.position_markers, position_marker(string_literal(""), point_in_tilespace));
    } else {
        set_drag_candidate(&editor_state->drag_data, marker, get_mouse_in_tile_space(&editor_state->camera, SCREEN_WIDTH, SCREEN_HEIGHT), marker->position);
    }
}

void editor_place_or_drag_savepoint(v2f32 point_in_tilespace) {
    if (is_dragging(&editor_state->drag_data)) {
        return; 
    }

    struct level_area_savepoint* existing_savepoint = level_area_savepoint_list_find_savepoint_at(&editor_state->editing_area.load_savepoints, point_in_tilespace);
    if (existing_savepoint) {
        editor_state->last_selected = existing_savepoint;
        set_drag_candidate(&editor_state->drag_data, existing_savepoint, get_mouse_in_tile_space(&editor_state->camera, SCREEN_WIDTH, SCREEN_HEIGHT), existing_savepoint->position);
    } else {
        editor_state->last_selected = level_area_savepoint_list_push(
            &editor_state->editing_area.load_savepoints,
            level_area_savepoint(point_in_tilespace, 0)
        );
    }
}

void editor_remove_savepoint_at(v2f32 point_in_tilespace) {
    struct level_area_savepoint* savepoint = level_area_savepoint_list_find_savepoint_at(&editor_state->editing_area.load_savepoints, point_in_tilespace);
    
    if (savepoint) {
        level_area_savepoint_list_remove(&editor_state->editing_area.load_savepoints, savepoint - editor_state->editing_area.load_savepoints.savepoints);
    }
}

void editor_remove_position_marker_at(v2f32 point_in_tilespace) {
    struct position_marker* marker = position_marker_list_find_marker_at_with_rect(&editor_state->editing_area.position_markers, point_in_tilespace);
    
    if (marker) {
        if (marker == editor_state->last_selected) {
            editor_state->last_selected = 0;
        }
        position_marker_list_remove(&editor_state->editing_area.position_markers, marker - editor_state->editing_area.position_markers.markers);
    }
}

/*
  NOTE: this is recording level_area_entity entities, which are not like the other entities. These actually have normal coordinate systems.
 */
void editor_place_or_drag_actor(v2f32 point_in_tilespace) { /* NOTE: This isn't fully changed... */
    if (is_dragging(&editor_state->drag_data)) {
        return; 
    }

    {
        for (s32 index = 0; index < editor_state->editing_area.load_entities.count; ++index) {
            struct level_area_entity* current_entity = editor_state->editing_area.load_entities.entities + index;

            /* The size should really depend on the model. Oh well. */
            if (rectangle_f32_intersect(rectangle_f32(current_entity->position.x, current_entity->position.y-1, 1, 2), cursor_rectangle(point_in_tilespace))) {
                editor_state->last_selected = current_entity;
                set_drag_candidate(&editor_state->drag_data, current_entity, get_mouse_in_tile_space(&editor_state->camera, SCREEN_WIDTH, SCREEN_HEIGHT), current_entity->position);
                return;
            }
        }


        /* otherwise no touch, place a new one at default size 1 1 */
        struct level_area_entity* new_entity = &editor_state->editing_area.load_entities.entities[editor_state->editing_area.load_entities.count++];
        level_area_entity_set_base_id(new_entity, string_literal("__default__"));

        new_entity->position.x = point_in_tilespace.x;
        new_entity->position.y = point_in_tilespace.y;
        new_entity->health_override = new_entity->magic_override = new_entity->loot_table_id_index = -1;
        new_entity->flags = new_entity->ai_flags = 0;
        {
            struct entity_actor_placement_property_menu* properties = &editor_state->actor_property_menu;
            struct entity_base_data*                     data       = entity_database_find_by_index(&game_state->entity_database, properties->entity_base_id);

            /* TODO: technically our models should contain their own collision size */
            /* they don't right now which is why I'm going to hardcode all of this. */
            /* in tile sizes. */
            new_entity->scale.x = 1;
            new_entity->scale.y = 2;
        }
        editor_state->last_selected      = new_entity;
    }
}

void editor_place_or_drag_light(v2f32 point_in_tilespace) {
    if (is_dragging(&editor_state->drag_data)) {
        return;
    }

    {
        for (s32 light_index = 0; light_index < editor_state->editing_area.lights.count; ++light_index) {
            struct light_def* current_light = editor_state->editing_area.lights.lights + light_index;

            if (rectangle_f32_intersect(rectangle_f32((current_light->position.x), (current_light->position.y), 1, 1), cursor_rectangle(point_in_tilespace))) {
                editor_state->last_selected = current_light;
                set_drag_candidate(&editor_state->drag_data, current_light, get_mouse_in_tile_space(&editor_state->camera, SCREEN_WIDTH, SCREEN_HEIGHT), current_light->position);
                return;
            }
        }

        struct light_def* new_light = &editor_state->editing_area.lights.lights[editor_state->editing_area.lights.count++];
        new_light->position         = point_in_tilespace;
        new_light->power            = 3;
        new_light->color            = color32u8_WHITE;
        new_light->flags            = 0;
        new_light->scale            = v2f32(1, 1); /* for the dragging code which always expects a rectangle. */

        editor_state->last_selected = new_light;
    }
}

void editor_remove_light_at(v2f32 point_in_tilespace) {
    for (s32 light_index = 0; light_index < editor_state->editing_area.lights.count; ++light_index) {
        struct light_def* current_light = editor_state->editing_area.lights.lights + light_index;

        if (rectangle_f32_intersect(rectangle_f32((current_light->position.x), (current_light->position.y), 1, 1), cursor_rectangle(point_in_tilespace))) {
            editor_state->editing_area.lights.lights[light_index] = editor_state->editing_area.lights.lights[--editor_state->editing_area.lights.count];
            return;
        }
    }
}

void editor_remove_actor_at(v2f32 point_in_tilespace) {
    for (s32 index = 0; index < editor_state->editing_area.load_entities.count; ++index) {
        struct level_area_entity* current_entity = editor_state->editing_area.load_entities.entities + index;

        if (rectangle_f32_intersect(rectangle_f32(current_entity->position.x, current_entity->position.y-1, 1, 2), cursor_rectangle(point_in_tilespace))) {
            editor_state->editing_area.load_entities.entities[index] = editor_state->editing_area.load_entities.entities[--editor_state->editing_area.load_entities.count];
            return;
        }
    }
}

void editor_place_or_drag_scriptable_trigger(v2f32 point_in_tilespace) {
    if (is_dragging(&editor_state->drag_data)) {
        return; 
    }

    {
        for (s32 index = 0; index < editor_state->editing_area.triggers.count; ++index) {
            struct trigger* current_trigger = editor_state->editing_area.triggers.triggers + index;

            if (rectangle_f32_intersect(current_trigger->bounds, cursor_rectangle(point_in_tilespace))) {
                editor_state->last_selected = current_trigger;
                set_drag_candidate_rectangle(&editor_state->drag_data, current_trigger, get_mouse_in_tile_space(&editor_state->camera, SCREEN_WIDTH, SCREEN_HEIGHT),
                                             v2f32(current_trigger->bounds.x, current_trigger->bounds.y),
                                             v2f32(current_trigger->bounds.w, current_trigger->bounds.h));
                return;
            }
        }


        /* otherwise no touch, place a new one at default size 1 1 */
        struct trigger* new_trigger = &editor_state->editing_area.triggers.triggers[editor_state->editing_area.triggers.count++];
        new_trigger->bounds.x       = point_in_tilespace.x;
        new_trigger->bounds.y       = point_in_tilespace.y;
        new_trigger->bounds.w       = 1;
        new_trigger->bounds.h       = 1;
        editor_state->last_selected = new_trigger;
    }
}

void editor_place_or_drag_chest(v2f32 point_in_tilespace) {
    if (is_dragging(&editor_state->drag_data)) {
        return;
    }

    {
        for (s32 index = 0; index < editor_state->editing_area.chests.count; ++index) {
            struct entity_chest* current_chest = editor_state->editing_area.chests.chests + index;

            if (rectangle_f32_intersect(rectangle_f32(current_chest->position.x, current_chest->position.y, current_chest->scale.x, current_chest->scale.y), cursor_rectangle(point_in_tilespace))) {
                /* TODO drag candidate */
                editor_state->last_selected = current_chest;
                set_drag_candidate(&editor_state->drag_data, current_chest, get_mouse_in_tile_space(&editor_state->camera, SCREEN_WIDTH, SCREEN_HEIGHT),
                                   v2f32(current_chest->position.x, current_chest->position.y));
                return;
            }
        }


        /* otherwise no touch, place a new one at default size 1 1 */
        struct entity_chest* new_chest = &editor_state->editing_area.chests.chests[editor_state->editing_area.chests.count++];
        new_chest->position.x = point_in_tilespace.x;
        new_chest->position.y = point_in_tilespace.y;
        new_chest->scale.x = 1;
        new_chest->scale.y = 1;
        editor_state->last_selected      = new_chest;
    }
}

void editor_remove_chest_at(v2f32 point_in_tilespace) {
    for (s32 index = 0; index < editor_state->editing_area.chests.count; ++index) {
        struct entity_chest* current_chest = editor_state->editing_area.chests.chests + index;

        if (rectangle_f32_intersect(rectangle_f32(current_chest->position.x, current_chest->position.y, current_chest->scale.x, current_chest->scale.y), cursor_rectangle(point_in_tilespace))) {
            editor_state->editing_area.chests.chests[index] = editor_state->editing_area.chests.chests[--editor_state->editing_area.chests.count];
            return;
        }
    }
}

/* hopefully I never need to unproject. */
/* I mean I just do the reverse of my projection so it's okay I guess... */

/* camera should know the world dimensions but okay */
local void handle_rectangle_dragging_and_scaling(struct editor_drag_data* dragdata, struct camera* camera) {
    bool left_clicked   = 0;
    bool right_clicked  = 0;
    bool middle_clicked = 0;

    get_mouse_buttons(&left_clicked,
                      &middle_clicked,
                      &right_clicked);

    if (is_dragging(dragdata)) {
        /* when using shift you will write the changes! so be careful! */
        if (left_clicked) {
            /* NOTE this is not used yet because we don't have anything that is *not* grid aligned */
            v2f32                 displacement_delta = v2f32_sub(v2f32_floor(dragdata->initial_mouse_position),
                                                                 dragdata->initial_object_position);
            struct rectangle_f32* object_rectangle   = (struct rectangle_f32*) dragdata->context;

            v2f32 mouse_position_in_tilespace_rounded = get_mouse_in_tile_space_integer(camera, SCREEN_WIDTH, SCREEN_HEIGHT);

            if (is_key_down(KEY_SHIFT) && dragdata->has_size) {
                object_rectangle->w =  mouse_position_in_tilespace_rounded.x - dragdata->initial_object_position.x;
                object_rectangle->h =  mouse_position_in_tilespace_rounded.y - dragdata->initial_object_position.y;

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
                _debugprintf("<%f, %f> initial mpos", dragdata->initial_mouse_position.x, dragdata->initial_mouse_position.y);
                _debugprintf("<%f, %f> initial obj pos", dragdata->initial_object_position.x, dragdata->initial_object_position.y);
                _debugprintf("<%f, %f> displacement delta", displacement_delta.x, displacement_delta.y);
                object_rectangle->x = mouse_position_in_tilespace_rounded.x - displacement_delta.x;
                object_rectangle->y = mouse_position_in_tilespace_rounded.y - displacement_delta.y;
            }
        } else {
            dragdata->context = 0;
        }
    }
}

local void handle_editor_tool_mode_input(struct software_framebuffer* framebuffer) {
    bool left_clicked   = 0;
    bool right_clicked  = 0;
    bool middle_clicked = 0;

    /* do not check for mouse input when our special tab menu is open */
    if (editor_state->screen_state == EDITOR_SCREEN_MAIN) {
        if (editor_state->tab_menu_open == TAB_MENU_CLOSED) {
            get_mouse_buttons(&left_clicked, &middle_clicked, &right_clicked);
        }
    } else {
        get_mouse_buttons(&left_clicked, &middle_clicked, &right_clicked);
    }

    /* refactor later */
    s32 mouse_location[2];
    get_mouse_location(mouse_location, mouse_location+1);

    v2f32 world_space_mouse_location =
        camera_project(&editor_state->camera, v2f32(mouse_location[0], mouse_location[1]), SCREEN_WIDTH, SCREEN_HEIGHT);

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

    switch (editor_state->screen_state) {
        case EDITOR_SCREEN_PLACING_TRANSITION_SPAWN_LEVEL_AREA: {
            if (is_key_pressed(KEY_RETURN)) {
                editor_state->screen_state                  = EDITOR_SCREEN_MAIN;
                editor_state->camera                        = editor_state->transition_placement.camera_before_trying_transition_placement;
                struct trigger_level_transition* transition = editor_state->last_selected;
                copy_string_into_cstring(string_from_cstring(editor_state->loaded_area_name), transition->target_level, array_count(transition->target_level));
            }

            if (left_clicked) {
                struct trigger_level_transition* transition = editor_state->last_selected;
                transition->spawn_location.x = tile_space_mouse_location.x;
                transition->spawn_location.y = tile_space_mouse_location.y;
            }
        } break;
        case EDITOR_SCREEN_PLACING_TRANSITION_SPAWN_WORLD_MAP: {
            if (is_key_pressed(KEY_RETURN)) {
                editor_state->screen_state                  = EDITOR_SCREEN_MAIN;
                editor_state->camera                        = editor_state->transition_placement.camera_before_trying_transition_placement;
                struct trigger_level_transition* transition = editor_state->last_selected;
                copy_string_into_cstring(string_from_cstring(editor_state->loaded_world_map_name), transition->target_level, array_count(transition->target_level));
            }

            if (left_clicked) {
                struct trigger_level_transition* transition = editor_state->last_selected;
                transition->spawn_location.x = tile_space_mouse_location.x;
                transition->spawn_location.y = tile_space_mouse_location.y;
            }
        } break;
        case EDITOR_SCREEN_MAIN: {
            switch (editor_state->tool_mode) {
                case EDITOR_TOOL_BATTLETILE_PAINTING: {
                    if (left_clicked) {
                        editor_place_battle_tile_at(tile_space_mouse_location);
                    } else if (right_clicked) {
                        editor_remove_battle_tile_at(tile_space_mouse_location);
                    }
                } break;
                case EDITOR_TOOL_TILE_PAINTING: {
                    if (is_key_pressed(KEY_1)) {
                        editor_state->editor_brush_pattern = 0;
                    } else if (is_key_pressed(KEY_2)) {
                        editor_state->editor_brush_pattern = 1;
                    } else if (is_key_pressed(KEY_3)) {
                        editor_state->editor_brush_pattern = 2;
                    } else if (is_key_pressed(KEY_4)) {
                        editor_state->editor_brush_pattern = 3;
                    } else if (is_key_pressed(KEY_5)) {
                        editor_state->editor_brush_pattern = 4;
                    }

                    if (left_clicked) {
                        editor_brush_place_tile_at(tile_space_mouse_location);
                    } else if (right_clicked) {
                        editor_brush_remove_tile_at(tile_space_mouse_location);
                    }
                } break;
                case EDITOR_TOOL_SPAWN_PLACEMENT: {
                    if (left_clicked) {
                        editor_state->editing_area.default_player_spawn.x = world_space_mouse_location.x;
                        editor_state->editing_area.default_player_spawn.y = world_space_mouse_location.y;
                    }
                } break;
                case EDITOR_TOOL_ENTITY_PLACEMENT: {
                    switch (editor_state->entity_placement_type) {
                        /* Only adding special case if I really need it but generally seems not needed. */
#define Placement_Procedure_For(type)                                   \
                        case ENTITY_PLACEMENT_TYPE_ ## type: {          \
                            if (left_clicked) {                         \
                                editor_place_or_drag_##type(tile_space_mouse_location); \
                            } else if (right_clicked) {                 \
                                editor_remove_##type##_at(tile_space_mouse_location); \
                            }                                           \
                        } break

                        Placement_Procedure_For(actor);
                        Placement_Procedure_For(chest);
                        Placement_Procedure_For(light);
                        Placement_Procedure_For(savepoint);
                        Placement_Procedure_For(position_marker);
#undef Placement_Procedure_For
                    }
                } break;
                case EDITOR_TOOL_TRIGGER_PLACEMENT: {
                    switch (editor_state->trigger_placement_type) {
                        case TRIGGER_PLACEMENT_TYPE_LEVEL_TRANSITION: {
                            if (left_clicked) {
                                /* NOTE check the trigger mode */
                                editor_place_or_drag_level_transition_trigger(tile_space_mouse_location);
                            } else if (right_clicked) {
                                editor_remove_level_transition_trigger_at(tile_space_mouse_location);
                            }
                        } break;
                        case TRIGGER_PLACEMENT_TYPE_SCRIPTABLE_TRIGGER: {
                            if (left_clicked) {
                                /* NOTE check the trigger mode */
                                editor_place_or_drag_scriptable_trigger(tile_space_mouse_location);
                            } else if (right_clicked) {
                                editor_remove_scriptable_trigger_at(tile_space_mouse_location);
                            }
                        } break;
                    }
                } break;
                default: {
                } break;
            }

            if (editor_state->tab_menu_open == TAB_MENU_CLOSED) {
                handle_rectangle_dragging_and_scaling(&editor_state->drag_data, &editor_state->camera);
            }
        } break;
    }
}

/* copied and pasted for now */
/* This can be compressed, quite easily... However I won't deduplicate this yet, as I've yet to experiment fully with the UI so let's keep it like this for now. */
/* NOTE: animation is hard. Lots of state to keep track of. */
/* NOTE: phase this out and replace it with the simple menu the world editor has */
local void update_and_render_pause_editor_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    /* needs a bit of cleanup */
    f32 font_scale = 1;
    /* While the real pause menu is going to be replaced with something else later obviously */
    /* I want a nice looking menu to show off, and also the main menu is likely taking this design */
    struct ui_pause_menu* menu_state = &state->ui_pause;
    v2f32 item_positions[array_count(ui_pause_editor_menu_strings)] = {};

    for (unsigned index = 0; index < array_count(item_positions); ++index) {
        item_positions[index].y = 18 * (index+0.75);
    }

    f32 offscreen_x = -240;
    f32 final_x     = 20;

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
#if 0
                                /*NOTE: 
                                  level testing code (TODO: unchecked for functionality, and might not be useful anymore... An RPG is really difficult
                                  to playtest levels where it depends on so much game state.)
                                  
                                  as the engine now understands the notion of scripting,
                                  I need to add some functionality to force the scripting to work as intended...
                                  
                                  Just add some barrier code to only allow testing after a savename has been
                                  determined, as the savename is used to find the script.
                                  
                                  Or allow a separate script name to be selected outside of whatever is defaultly
                                  assumed (filename + script extension.) Which would also work fine?
                                */
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
#endif
                            } break;
                                /* oh my. */
                            case 2: {
                                menu_state->animation_state     = UI_PAUSE_MENU_TRANSITION_CLOSING;
                                menu_state->transition_t        = 0;
                                editor_state->serialize_menu_mode = 1;
                                editor_state->serialize_menu_reason = 0;
                            } break;
                            case 3: {
                                menu_state->animation_state     = UI_PAUSE_MENU_TRANSITION_CLOSING;
                                menu_state->transition_t        = 0;
                                editor_state->serialize_menu_mode = 2;
                                editor_state->serialize_menu_reason = 0;
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
                                IAllocator allocator = heap_allocator();
                                struct file_buffer script_filebuffer = OS_read_entire_file(allocator, string_literal("temp/SCRIPT.txt"));
                                editor_state->editing_area.script.internal_buffer = file_buffer_as_string(&script_filebuffer);
                                editor_state->editing_area.version = CURRENT_LEVEL_AREA_VERSION;
                                _serialize_level_area(NULL, &serializer, &editor_state->editing_area);
                                serializer_finish(&serializer);
                                file_buffer_free(&script_filebuffer);
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
        draw_position.y += 110;
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
                                    editor_clear_all_allocations(editor_state);
                                    struct binary_serializer serializer = open_read_file_serializer(string_concatenate(&scratch_arena, string_literal("areas/"), string_from_cstring(current_file->name)));
                                    _serialize_level_area(NULL, &serializer, &editor_state->editing_area);
                                    OS_create_directory(string_literal("temp/"));
                                    write_string_into_entire_file(string_literal("temp/SCRIPT.txt"), editor_state->editing_area.script.internal_buffer);
                                    serializer_finish(&serializer);

                                    cstring_copy(current_file->name, editor_state->current_save_name, array_count(editor_state->current_save_name));

                                    menu_state->animation_state = UI_PAUSE_MENU_TRANSITION_IN;
                                    menu_state->transition_t = 0;
                                    editor_state->serialize_menu_mode = 0;
                                    return;
                                } break;
                                case 1: {
                                    assertion(0 && "impossible");
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

    v2f32 world_space_mouse_location = editor_get_world_space_mouse_location();

    /* for tiles */
    v2f32 tile_space_mouse_location = world_space_mouse_location; {
        tile_space_mouse_location.x = floorf(world_space_mouse_location.x / TILE_UNIT_SIZE);
        tile_space_mouse_location.y = floorf(world_space_mouse_location.y / TILE_UNIT_SIZE);
    }

    if (!is_editing_text()) {
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

    switch (editor_state->screen_state) {
        case EDITOR_SCREEN_MAIN: {
            if (is_key_pressed(KEY_ESCAPE)) {
                if (editor_state->tab_menu_open & TAB_MENU_OPEN_BIT) {
                    editor_state->tab_menu_open = 0;
                } else {
                    game_state_set_ui_state(game_state, UI_STATE_PAUSE);
                    /* ready pause menu */
                    {
                        game_state->ui_pause.animation_state = 0;
                        game_state->ui_pause.transition_t    = 0;
                        game_state->ui_pause.selection       = 0;
                        zero_array(game_state->ui_pause.shift_t);
                    }
                }
            }

            if (is_key_down(KEY_SHIFT)) {
                if (is_key_pressed(KEY_Z)) {
                    editor_state->camera.zoom = 1;
                }

                f32 last_zoom = editor_state->camera.zoom;
                if (is_mouse_wheel_up()) {
                    editor_state->camera.zoom += 0.25;
                    if (editor_state->camera.zoom >= 4) {
                        editor_state->camera.zoom = 4;
                    }
                } else if (is_mouse_wheel_down()) {
                    editor_state->camera.zoom -= 0.25;
                    if (editor_state->camera.zoom <= 0.25) {
                        editor_state->camera.zoom = 0.25;
                    }
                }

                if (last_zoom != editor_state->camera.zoom) {
                    v2f32 world_space_in_zoomed_space_last = world_space_mouse_location;
                    v2f32 world_space_in_zoomed_space_current = world_space_mouse_location;

                    world_space_in_zoomed_space_last.x *= last_zoom;
                    world_space_in_zoomed_space_last.y *= last_zoom;
                    world_space_in_zoomed_space_current.x *= editor_state->camera.zoom;
                    world_space_in_zoomed_space_current.y *= editor_state->camera.zoom;

                    f32 delta_zoom = editor_state->camera.zoom - last_zoom;
                    f32 delta_x    = world_space_in_zoomed_space_last.x - world_space_in_zoomed_space_current.x;
                    f32 delta_y    = world_space_in_zoomed_space_last.y - world_space_in_zoomed_space_current.y;

                    editor_state->camera.xy.x -= delta_x;
                    editor_state->camera.xy.y -= delta_y;
                }
            }

            /* I refuse to code a UI library, unless *absolutely* necessary... Since the editor is the only part that requires this kind of standardized UI... */
            f32 y_cursor = 0;
            {
                software_framebuffer_draw_text(framebuffer,
                                               graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]),
                                               1, v2f32(0,y_cursor), string_literal("Level Editor [SHIFT-TAB (tool select), TAB (tool mode select), CTRL-TAB (object setting)]"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                y_cursor += 12;
                {
                    char tmp_text[1024]={};
                    snprintf(tmp_text, 1024, "mode: %.*s", editor_tool_mode_strings[editor_state->tool_mode].length, editor_tool_mode_strings[editor_state->tool_mode].data);
                    software_framebuffer_draw_text(framebuffer,
                                                   graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]),
                                                   1, v2f32(0,y_cursor), string_from_cstring(tmp_text), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                }
                switch (editor_state->tool_mode) { /* PROPERTY MENU */
                    case EDITOR_TOOL_BATTLETILE_PAINTING: {
                        y_cursor += 12;
                        {
                            char tmp_text[1024]={};
                            snprintf(tmp_text, 1024, "Battle Tile Painting");
                            software_framebuffer_draw_text(framebuffer,
                                                           graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]),
                                                           1, v2f32(0,y_cursor), string_from_cstring(tmp_text), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                        }
                        y_cursor += 12;

                        if (!editor_state->tab_menu_open) {
                            if (is_key_down(KEY_SHIFT)) {
                            } else {
                            }
                        }
                    } break;
                    case EDITOR_TOOL_TILE_PAINTING: {
                        y_cursor += 12;
                        {
                            char tmp_text[1024]={};
                            snprintf(tmp_text, 1024, "current tile id: %d\ncurrent tile layer: %.*s", editor_state->painting_tile_id, tile_layer_strings[editor_state->current_tile_layer].length, tile_layer_strings[editor_state->current_tile_layer].data);
                            software_framebuffer_draw_text(framebuffer,
                                                           graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]),
                                                           1, v2f32(0,y_cursor), string_from_cstring(tmp_text), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                        }
                        y_cursor += 12;

                        if (!editor_state->tab_menu_open) {
                            if (is_key_down(KEY_SHIFT)) {
                            } else {
                                if (is_mouse_wheel_up()) {
                                    editor_state->current_tile_layer -= 1;
                                    if (editor_state->current_tile_layer < 0) {
                                        editor_state->current_tile_layer = TILE_LAYER_COUNT-1;
                                    }
                                } else if (is_mouse_wheel_down()) {
                                    editor_state->current_tile_layer += 1;
                                    if (editor_state->current_tile_layer >= TILE_LAYER_COUNT) {
                                        editor_state->current_tile_layer = 0;
                                    }
                                }
                            }
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
                y_cursor += 12;
                {
                    v2f32 mouse_tilespace = get_mouse_in_tile_space(&editor_state->camera, SCREEN_WIDTH, SCREEN_HEIGHT);
                    v2f32 world_space_mouse_location = editor_get_world_space_mouse_location();
            
                    software_framebuffer_draw_text(framebuffer,
                                                   graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]),
                                                   1, v2f32(0,y_cursor), string_from_cstring(format_temp("Mouse: (tx: %d, ty: %d, wx: %f, wy: %f)", (s32)mouse_tilespace.x, (s32)mouse_tilespace.y, world_space_mouse_location.x, world_space_mouse_location.y)), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
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
                        if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, v2f32(100, draw_cursor_y), editor_tool_mode_strings[index])) {
                            editor_state->tab_menu_open = 0;
                            editor_state->tool_mode     = index;
                            editor_state->last_selected = 0;
                            break;
                        }
                        draw_cursor_y += 12 * 1.5 * 1;
                    }
                } else if (editor_state->tab_menu_open & TAB_MENU_CTRL_BIT) { /* TAB MENU CONTEXT  */
                    if (!editor_state->last_selected) {
                        editor_state->tab_menu_open = 0;
                    } else {
                        switch (editor_state->tool_mode) {
                            /* I would show images, but this is easier for now */
                            case EDITOR_TOOL_BATTLETILE_PAINTING:
                            case EDITOR_TOOL_TILE_PAINTING:
                            case EDITOR_TOOL_LEVEL_SETTINGS: {
                                editor_state->tab_menu_open = 0;
                            } break;
                            case EDITOR_TOOL_TRIGGER_PLACEMENT: {
                                switch (editor_state->trigger_placement_type) {
                                    case TRIGGER_PLACEMENT_TYPE_LEVEL_TRANSITION: {
                                        f32 draw_cursor_y = 30;
                                        struct trigger_level_transition* trigger = editor_state->last_selected;

                                        char tmp_string[1024] = {};
                                        snprintf(tmp_string, 1024, "Transition Area: \"%s\" <%f, %f> (SET?)", trigger->target_level, trigger->spawn_location.x, trigger->spawn_location.y);
                                        if(EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, v2f32(16, draw_cursor_y), string_from_cstring(tmp_string))) {
                                            editor_state->screen_state = EDITOR_SCREEN_FILE_SELECTION_FOR_TRANSITION;
                                        }
                                        draw_cursor_y += 12 * 1.2 * 1;

                                        if(EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, v2f32(16, draw_cursor_y), format_temp_s("target type: %.*s", trigger_level_transition_type_strings[trigger->type].length, trigger_level_transition_type_strings[trigger->type].data))) {
                                            trigger->type += 1;
                                            if (trigger->type > TRIGGER_LEVEL_TRANSITION_TYPE_COUNT) trigger->type = 0;
                                        }

                                        draw_cursor_y += 12 * 1.2 * 1;
                                        if(EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, v2f32(16, draw_cursor_y), string_concatenate(&scratch_arena, string_literal("Facing Direction: "), facing_direction_strings[trigger->new_facing_direction]))) {
                                            trigger->new_facing_direction += 1;
                                            if (trigger->new_facing_direction > 4) trigger->new_facing_direction = 0;
                                        }
                                    } break;
                                    case TRIGGER_PLACEMENT_TYPE_SCRIPTABLE_TRIGGER: {
                                        f32 draw_cursor_y = 30;
                                        struct trigger* trigger = editor_state->last_selected;
                                        s32 trigger_id          = trigger - editor_state->editing_area.triggers.triggers;

                                        string activation_type_string = activation_type_strings[trigger->activation_method];
                                        if(EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, v2f32(16, draw_cursor_y),
                                                               string_from_cstring(format_temp("Activation Type: %.*s", activation_type_string.length, activation_type_string.data)))) {
                                            trigger->activation_method += 1;
                                            if (trigger->activation_method >= ACTIVATION_TYPE_COUNT) {
                                                trigger->activation_method = 0;
                                            }
                                        }
                                    } break;
                                    default: {
                                        editor_state->tab_menu_open = 0;
                                    } break;
                                }
                            } break;
                            case EDITOR_TOOL_ENTITY_PLACEMENT: {
                                switch (editor_state->entity_placement_type) {
                                    case ENTITY_PLACEMENT_TYPE_actor: {
                                        f32 draw_cursor_y = 70;
                                        const f32 text_scale = 1;

                                        struct level_area_entity* entity = editor_state->last_selected;

                                        {
                                            string s = string_clone(&scratch_arena, string_from_cstring(format_temp("base_id: %s", entity->base_name)));
                                            if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 2, v2f32(10, draw_cursor_y), s)) {
                                                editor_state->actor_property_menu.picking_entity_base ^= 1;
                                                editor_state->actor_property_menu.item_list_scroll_y   = 0;
                                            }
                                            draw_cursor_y += 16 * 1 * 1.5 + TILE_UNIT_SIZE*2.3;
                                        }

                                        if (editor_state->actor_property_menu.picking_entity_base) {
                                            const s32 SCROLL_AMOUNT = 45;

                                            if (is_mouse_wheel_up()) {
                                                if (editor_state->actor_property_menu.item_list_scroll_y < 0)
                                                    editor_state->actor_property_menu.item_list_scroll_y += SCROLL_AMOUNT;
                                            } else if (is_mouse_wheel_down()) {
                                                editor_state->actor_property_menu.item_list_scroll_y -= SCROLL_AMOUNT;
                                            }

                                            if (is_key_pressed(KEY_HOME)) {
                                                editor_state->actor_property_menu.item_list_scroll_y = 0;
                                            }

                                            {
                                                struct entity_database* entities = &game_state->entity_database;

                                                f32 largest_name_width = 0;

                                                for (s32 index = 0; index < entities->entity_count; ++index) {
                                                    f32 current_width = font_cache_text_width(font, entities->entity_key_strings[index], text_scale);

                                                    if (largest_name_width < current_width) {
                                                        largest_name_width = current_width;
                                                    }
                                                }

                                                s32 ENTRIES_PER_ROW = (250 / largest_name_width);
                                                s32 row_count     = (tile_table_data_count / ENTRIES_PER_ROW)+1;

                                                for (s32 row_index = 0; row_index < row_count; ++row_index) {
                                                    f32 draw_cursor_x = 0;

                                                    for (s32 index = 0; index < ENTRIES_PER_ROW; ++index) {
                                                        s32 entity_data_index = row_index * ENTRIES_PER_ROW + index;
                                                        if (!(entity_data_index < entities->entity_count)) {
                                                            break;
                                                        }

                                                        struct entity_base_data* data = entity_database_find_by_index(entities, entity_data_index);
                                                        string facing_direction_string = facing_direction_strings_normal[editor_state->actor_property_menu.facing_direction_index_for_animation];
                                                        struct entity_animation* anim = find_animation_by_name(data->model_index, format_temp_s("idle_%.*s", facing_direction_string.length, facing_direction_string.data));

                                                        image_id sprite_to_use = anim->sprites[0];

                                                        software_framebuffer_draw_image_ex(framebuffer, graphics_assets_get_image_by_id(&graphics_assets, sprite_to_use),
                                                                                           rectangle_f32(draw_cursor_x, draw_cursor_y-TILE_UNIT_SIZE*2, TILE_UNIT_SIZE, TILE_UNIT_SIZE*2), RECTANGLE_F32_NULL, color32f32(1,1,1,1), NO_FLAGS, BLEND_MODE_ALPHA);

                                                        if (EDITOR_imgui_button(framebuffer, font, highlighted_font, text_scale, v2f32(draw_cursor_x, draw_cursor_y), entities->entity_key_strings[entity_data_index])) {
                                                            editor_state->tab_menu_open    = 0;
                                                            editor_state->actor_property_menu.entity_base_id = entity_data_index;

                                                            level_area_entity_set_base_id(entity, entities->entity_key_strings[entity_data_index]);
                                                        }

                                                        draw_cursor_x += largest_name_width * 1.1;
                                                    }

                                                    draw_cursor_y += TILE_UNIT_SIZE*2 * 1.2 * 1;
                                                }
                                            }
                                        } else {
                                            string facing_direction_string = facing_direction_strings[entity->facing_direction];
                                            {
                                                string s = string_clone(&scratch_arena, string_from_cstring(format_temp("facing direction: %.*s", facing_direction_string.length, facing_direction_string.data)));
                                                s32 result = EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, v2f32(10, draw_cursor_y), s);

                                                if (result == 1) {
                                                    entity->facing_direction += 1;
                                                    if (entity->facing_direction > 3) entity->facing_direction = 0;
                                                } else if (result == 2) {
                                                    if (entity->facing_direction == 0) {
                                                        entity->facing_direction = 3;
                                                    } else {
                                                        entity->facing_direction -= 1;
                                                    }
                                                }
                                                draw_cursor_y += 16 * 1 * 1.5;
                                            }
                                            {
                                                EDITOR_imgui_text_edit_cstring(framebuffer, font, highlighted_font, 1, v2f32(10, draw_cursor_y), string_literal("scriptname"), entity->script_name, array_count(entity->script_name));
                                                draw_cursor_y += 16 * 1 * 1.5;
                                            }
                                            {
                                                /* this can actually just be a dropdown/modal selection since it's **known** which talk files are in the game, still going to be copied as a string but would be easier than guessing... */
                                                EDITOR_imgui_text_edit_cstring(framebuffer, font, highlighted_font, 1, v2f32(10, draw_cursor_y), string_literal("dialogue file"), entity->dialogue_file, array_count(entity->dialogue_file));
                                                draw_cursor_y += 16 * 1 * 1.5;
                                            }

                                            {
                                                string s = string_clone(&scratch_arena, string_from_cstring(format_temp("hidden: %s", cstr_yesno[(entity->flags & ENTITY_FLAGS_HIDDEN) > 0])));
                                                if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, v2f32(10, draw_cursor_y), s)) {
                                                    entity->flags ^= ENTITY_FLAGS_HIDDEN;
                                                }
                                                draw_cursor_y += 16 * 1 * 1.5;
                                            }
                                            {
                                                draw_cursor_y += 16 * 1 * 1.5;
                                            }
                                        }

                                    } break;
                                    case ENTITY_PLACEMENT_TYPE_position_marker: {
                                        struct position_marker* marker = editor_state->last_selected;
                                        f32 draw_cursor_y = 35;
                                        const f32 text_scale = 1;

                                        {
                                            EDITOR_imgui_text_edit_cstring(framebuffer, font, highlighted_font, 1, v2f32(10, draw_cursor_y), string_literal("name"), marker->name, array_count(marker->name));
                                        }
                                    } break;
                                    case ENTITY_PLACEMENT_TYPE_chest: {
                                        f32 draw_cursor_y = 70;
                                        struct entity_chest* chest = editor_state->last_selected;
                                        software_framebuffer_draw_text(framebuffer, font, 1, v2f32(10, 10), string_literal("Chest Items"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);

                                        {
                                            /* sort bar */
                                            f32 draw_cursor_x = 30;

                                            for (unsigned index = 0; index < array_count(item_type_strings); ++index) {
                                                string text = item_type_strings[index];

                                                if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, v2f32(draw_cursor_x, 35), text)) {
                                                    /* should be mask */
                                                    editor_state->chest_property_menu.item_sort_filter = index;
                                                }

                                                draw_cursor_x += font_cache_text_width(font, text, 1) * 1.15;
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

                                            /* NOTE: The chest is probably code rotting, since I haven't seen it in a good minute. */
                                            /* probably go back and check these things. */
                                            for (unsigned index = 0; index < item_database_count; ++index) {
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

                                                    if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, v2f32(16, draw_cursor_y + editor_state->chest_property_menu.item_list_scroll_y), string_from_cstring(tmp))) {
                                                        entity_inventory_add((struct entity_inventory*)&chest->inventory, 16, item_get_id(item_base));
                                                        editor_state->chest_property_menu.adding_item = false;
                                                        break;
                                                    }

                                                    draw_cursor_y += 12 * 1.3;
                                                }
                                            }
                                        } else {
                                            /* TODO: check this... */
                                            if (chest->inventory.item_count > 0) {
                                                char tmp[255] = {};
                                                for (s32 index = 0; index < chest->inventory.item_count; ++index) {
                                                    struct item_instance* item      = chest->inventory.items + index;
                                                    struct item_def*      item_base = item_database_find_by_id(item->item);
                                                    snprintf(tmp, 255, "(%.*s) %.*s - %d/%d", item_base->id_name.length, item_base->id_name.data, item_base->name.length, item_base->name.data, item->count, item_base->max_stack_value);
                                                    draw_cursor_y += 12 * 1.5;

                                                    s32 button_response = (EDITOR_imgui_button(framebuffer, font, highlighted_font, 1.1, v2f32(16, draw_cursor_y), string_from_cstring(tmp)));
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
                                                software_framebuffer_draw_text(framebuffer, font, 1, v2f32(10, draw_cursor_y), string_literal("(no items)"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                                            }

                                            if(EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, v2f32(150, 10), string_literal("(add item)"))) {
                                                /* pop up should just replace the menu */
                                                /* for now just add a test item */
                                                /* entity_inventory_add(&chest->inventory, 16, item_id_make(string_literal("item_sardine_fish_5"))); */
                                                editor_state->chest_property_menu.adding_item        = true;
                                                editor_state->chest_property_menu.item_list_scroll_y = 0;
                                            }
                                            {
                                                string s = string_clone(&scratch_arena, string_from_cstring(format_temp("hidden: %s", cstr_yesno[(chest->flags & ENTITY_FLAGS_HIDDEN) > 0])));
                                                if(EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, v2f32(270, 10), s)) {
                                                    /* pop up should just replace the menu */
                                                    /* for now just add a test item */
                                                    /* entity_inventory_add(&chest->inventory, 16, item_id_make(string_literal("item_sardine_fish_5"))); */
                                                    chest->flags ^= ENTITY_FLAGS_HIDDEN;
                                                }
                                            }
                                        }
                                    } break;
                                    case ENTITY_PLACEMENT_TYPE_light: {
                                        f32 draw_cursor_y = 35;
                                        struct light_def* current_light = editor_state->last_selected;

                                        EDITOR_imgui_text_edit_f32(framebuffer, font, highlighted_font, 1, v2f32(15, draw_cursor_y), string_literal("Light Power: "), &current_light->power);
                                        draw_cursor_y += 16 * 1.5;
                                        EDITOR_imgui_text_edit_u8(framebuffer, font, highlighted_font, 1, v2f32(15, draw_cursor_y), string_literal("Light R: "), &current_light->color.r);
                                        draw_cursor_y += 16 * 1.5;
                                        EDITOR_imgui_text_edit_u8(framebuffer, font, highlighted_font, 1, v2f32(15, draw_cursor_y), string_literal("Light G: "), &current_light->color.g);
                                        draw_cursor_y += 16 * 1.5;
                                        EDITOR_imgui_text_edit_u8(framebuffer, font, highlighted_font, 1, v2f32(15, draw_cursor_y), string_literal("Light B: "), &current_light->color.b);
                                        draw_cursor_y += 16 * 1.5;
                                        EDITOR_imgui_text_edit_u8(framebuffer, font, highlighted_font, 1, v2f32(15, draw_cursor_y), string_literal("Light A: "), &current_light->color.a);
                                        draw_cursor_y += 16 * 1.5;
                                        {
                                            string s = string_clone(&scratch_arena, string_from_cstring(format_temp("hidden: %s", cstr_yesno[(current_light->flags & ENTITY_FLAGS_HIDDEN) > 0])));
                                            if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, v2f32(10, draw_cursor_y), s)) {
                                                current_light->flags ^= ENTITY_FLAGS_HIDDEN;
                                            }
                                            draw_cursor_y += 16 * 1.5;
                                        }
                                    } break;
                                    case ENTITY_PLACEMENT_TYPE_savepoint: {
                                        f32 draw_cursor_y = 35;
                                        struct entity_savepoint* current_savepoint = editor_state->last_selected;
                                        {
                                            string s = string_clone(&scratch_arena, string_from_cstring(format_temp("hidden: %s", cstr_yesno[(current_savepoint->flags & ENTITY_FLAGS_HIDDEN) > 0])));
                                            if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, v2f32(10, draw_cursor_y), s)) {
                                                current_savepoint->flags ^= ENTITY_FLAGS_HIDDEN;
                                            }
                                            draw_cursor_y += 16 * 1.5;
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
                    switch (editor_state->tool_mode) {
                        /* I would show images, but this is easier for now */
                        case EDITOR_TOOL_LEVEL_SETTINGS: {
                            f32 draw_cursor_y = 35;
                            const f32 text_scale = 1;

                            struct level_area_entity* entity = editor_state->last_selected;

                            /* this is mostly script affecting so yeah. */
                            if (editor_state->level_settings.changing_preview_environment_color) {
                                if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, v2f32(10, draw_cursor_y), string_literal("DAY (0)"))) {
                                    game_set_time_color(0);
                                    editor_state->level_settings.changing_preview_environment_color = false;
                                }
                                draw_cursor_y += 16 * 1 * 1.5 + TILE_UNIT_SIZE*2.3;
                                if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, v2f32(10, draw_cursor_y), string_literal("DAWN (1)"))) {
                                    game_set_time_color(1);
                                    editor_state->level_settings.changing_preview_environment_color = false;
                                }
                                draw_cursor_y += 16 * 1 * 1.5 + TILE_UNIT_SIZE*2.3;
                                if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, v2f32(10, draw_cursor_y), string_literal("NIGHT (2)"))) {
                                    game_set_time_color(2);
                                    editor_state->level_settings.changing_preview_environment_color = false;
                                }
                                draw_cursor_y += 16 * 1 * 1.5 + TILE_UNIT_SIZE*2.3;
                                if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, v2f32(10, draw_cursor_y), string_literal("MIDNIGHT (3)"))) {
                                    game_set_time_color(3);
                                    editor_state->level_settings.changing_preview_environment_color = false;
                                }
                                draw_cursor_y += 16 * 1 * 1.5 + TILE_UNIT_SIZE*2.3;
                            } else {
                                {
                                    EDITOR_imgui_text_edit_cstring(framebuffer, font, highlighted_font, 1, v2f32(10, draw_cursor_y), string_literal("(can scriptoverride)areaname:"), editor_state->level_settings.area_name, array_count(editor_state->level_settings.area_name));
                                    draw_cursor_y += 16 * 1 * 1.5 + TILE_UNIT_SIZE*2.3;
                                }
                                {
                                    string s = string_clone(&scratch_arena, string_from_cstring(format_temp("(preview only, nosave) environment color", entity->base_name)));
                                    if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, v2f32(10, draw_cursor_y), s)) {
                                        editor_state->level_settings.changing_preview_environment_color = true;
                                    }
                                    draw_cursor_y += 16 * 1 * 1.5 + TILE_UNIT_SIZE*2.3;
                                }
                                {
                                    string s = string_clone(&scratch_arena, string_from_cstring(format_temp("open temporary/SCRIPT.txt to edit current level script.")));
                                    if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, v2f32(10, draw_cursor_y), s)) {
                                    }
                                    draw_cursor_y += 16 * 1 * 1.5 + TILE_UNIT_SIZE*2.3;
                                }
                            }
                        } break;
                        case EDITOR_TOOL_TILE_PAINTING: {
                            f32 draw_cursor_y = 30 + editor_state->tile_painting_property_menu.item_list_scroll_y;

                            const s32 SCROLL_AMOUNT = 45;
                            if (is_mouse_wheel_up()) {
                                if (editor_state->tile_painting_property_menu.item_list_scroll_y < 0)
                                    editor_state->tile_painting_property_menu.item_list_scroll_y += SCROLL_AMOUNT;
                            } else if (is_mouse_wheel_down()) {
                                editor_state->tile_painting_property_menu.item_list_scroll_y -= SCROLL_AMOUNT;
                            }

                            if (is_key_pressed(KEY_HOME)) {
                                editor_state->tile_painting_property_menu.item_list_scroll_y = 0;
                            }


                            const f32 text_scale = 1;

                            f32 largest_name_width = 0;

                            /* TODO should be lazy init */
                            for (s32 index = 0; index < tile_table_data_count; ++index) {
                                s32 tile_data_index = index;
                                struct tile_data_definition* tile_data = tile_table_data + tile_data_index;

                                f32 candidate = font_cache_text_width(font, tile_data->name, text_scale);
                                if (largest_name_width < candidate) {
                                    largest_name_width = candidate;
                                }
                            }

                            s32 TILES_PER_ROW = (250 / largest_name_width);
                            s32 row_count     = (tile_table_data_count / TILES_PER_ROW)+1;

                            for (s32 row_index = 0; row_index < row_count; ++row_index) {
                                f32 draw_cursor_x = 0;

                                for (s32 index = 0; index < TILES_PER_ROW; ++index) {
                                    s32 tile_data_index = row_index * TILES_PER_ROW + index;
                                    if (!(tile_data_index < tile_table_data_count)) {
                                        break;
                                    }

                                    struct tile_data_definition* tile_data = tile_table_data + tile_data_index;
                                    image_id tex = get_tile_image_id(tile_data); 

                                    software_framebuffer_draw_image_ex(framebuffer, graphics_assets_get_image_by_id(&graphics_assets, tex),
                                                                       rectangle_f32(draw_cursor_x, draw_cursor_y-16, 32, 32), RECTANGLE_F32_NULL, color32f32(1,1,1,1), NO_FLAGS, BLEND_MODE_ALPHA);

                                    if (EDITOR_imgui_button(framebuffer, font, highlighted_font, text_scale, v2f32(draw_cursor_x, draw_cursor_y), tile_data->name)) {
                                        editor_state->tab_menu_open    = 0;
                                        editor_state->painting_tile_id = tile_data_index;
                                    }

                                    draw_cursor_x += largest_name_width * 1.1;
                                }

                                draw_cursor_y += 24 * 1.2 * 1;
                            }
                        } break;
                        case EDITOR_TOOL_ENTITY_PLACEMENT: {
                            f32 draw_cursor_y = 30;
                            for (s32 index = 0; index < array_count(entity_placement_type_strings)-1; ++index) {
                                if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, v2f32(16, draw_cursor_y), entity_placement_type_strings[index])) {
                                    editor_state->tab_menu_open          = 0;
                                    editor_state->entity_placement_type = index;
                                    break;
                                }
                                draw_cursor_y += 12 * 1.2 * 1;
                            }
                        } break;
                        case EDITOR_TOOL_TRIGGER_PLACEMENT: {
                            f32 draw_cursor_y = 30;
                            for (s32 index = 0; index < array_count(trigger_placement_type_strings)-1; ++index) {
                                if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, v2f32(16, draw_cursor_y), trigger_placement_type_strings[index])) {
                                    editor_state->tab_menu_open          = 0;
                                    editor_state->trigger_placement_type = index;
                                    break;
                                }
                                draw_cursor_y += 12 * 1.2 * 1;
                            }
                        } break;
                        default: {
                            editor_state->tab_menu_open          = 0;
                        } break;
                    }
                }
            }
            /* not using render commands here. I can trivially figure out what order most things should be... */
        } break;
        case EDITOR_SCREEN_FILE_SELECTION_FOR_TRANSITION: {
            struct font_cache* font             = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_STEEL]);
            struct font_cache* highlighted_font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]);
            struct directory_listing listing = {};

            struct trigger_level_transition* transition = editor_state->last_selected;
            if (transition->type == TRIGGER_LEVEL_TRANSITION_TYPE_TO_LEVEL_AREA) {
                listing = directory_listing_list_all_files_in(&scratch_arena, string_literal("areas/"));
            } else if (transition->type == TRIGGER_LEVEL_TRANSITION_TYPE_TO_WORLD_MAP) {
                listing = directory_listing_list_all_files_in(&scratch_arena, string_literal("worldmaps/"));
            }

            if (is_key_pressed(KEY_ESCAPE)) {
                editor_state->screen_state = EDITOR_SCREEN_MAIN;
            }

            v2f32 draw_position = v2f32(10, 10);
            if (listing.count <= 2) {
                software_framebuffer_draw_text(framebuffer, font, 1, draw_position, string_literal("(no areas)"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            } else {
                /* skip . and ../ */
                for (s32 index = 2; index < listing.count; ++index) {
                    struct directory_file* current_file = listing.files + index;
                    draw_position.y += 1 * 12 * 1;
                    if(EDITOR_imgui_button(framebuffer, font, highlighted_font, 1, draw_position, string_from_cstring(current_file->name))) {
                        editor_state->transition_placement.camera_before_trying_transition_placement = editor_state->camera;
                        editor_state->camera.xy = v2f32(0,0);
                        if (transition->type == TRIGGER_LEVEL_TRANSITION_TYPE_TO_LEVEL_AREA) {
                            struct binary_serializer serializer = open_read_file_serializer(string_concatenate(&scratch_arena, string_literal("areas/"), string_from_cstring(current_file->name)));
                            copy_string_into_cstring(string_from_cstring(current_file->name), editor_state->loaded_area_name, array_count(editor_state->loaded_area_name));
                            editor_state->screen_state = EDITOR_SCREEN_PLACING_TRANSITION_SPAWN_LEVEL_AREA;
                            memory_arena_clear_top(editor_state->arena);
                            memory_arena_set_allocation_region_top(editor_state->arena); {
                                _serialize_level_area(editor_state->arena, &serializer, &editor_state->loaded_area);
                            } memory_arena_set_allocation_region_bottom(editor_state->arena);
                            serializer_finish(&serializer);
                        } else if (transition->type == TRIGGER_LEVEL_TRANSITION_TYPE_TO_WORLD_MAP) {
                            struct binary_serializer serializer = open_read_file_serializer(string_concatenate(&scratch_arena, string_literal("worldmaps/"), string_from_cstring(current_file->name)));
                            copy_string_into_cstring(string_from_cstring(current_file->name), editor_state->loaded_world_map_name, array_count(editor_state->loaded_world_map_name));
                            editor_state->screen_state = EDITOR_SCREEN_PLACING_TRANSITION_SPAWN_WORLD_MAP;
                            memory_arena_clear_top(editor_state->arena);
                            memory_arena_set_allocation_region_top(editor_state->arena); {
                                serialize_world_map(editor_state->arena, &serializer, &editor_state->loaded_world_map);
                            } memory_arena_set_allocation_region_bottom(editor_state->arena);
                            serializer_finish(&serializer);
                        }
                        break;
                    }
                }
            }
        } break;
        case EDITOR_SCREEN_PLACING_TRANSITION_SPAWN_LEVEL_AREA: {
            if (is_key_pressed(KEY_ESCAPE)) {
                editor_state->screen_state = EDITOR_SCREEN_MAIN;
            }

            f32 y_cursor = 0;
            software_framebuffer_draw_text(framebuffer,
                                           graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]),
                                           1, v2f32(0,y_cursor), string_literal("Placing level area transition location"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            y_cursor += 12;
        } break;
        case EDITOR_SCREEN_PLACING_TRANSITION_SPAWN_WORLD_MAP: {
            if (is_key_pressed(KEY_ESCAPE)) {
                editor_state->screen_state = EDITOR_SCREEN_MAIN;
            }

            f32 y_cursor = 0;
            software_framebuffer_draw_text(framebuffer,
                                           graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]),
                                           1, v2f32(0,y_cursor), string_literal("Placing world map transition location"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            y_cursor += 12;
        } break;
    }
}

/* slightly different */
union color32f32 editor_lighting_shader(struct software_framebuffer* framebuffer, union color32f32 source_pixel, v2f32 pixel_position, void* context) {
    if (editor_state->fullbright) {
        return source_pixel;
    }

    if (lightmask_buffer_is_lit(&global_lightmask_buffer, pixel_position.x, pixel_position.y)) {
        return source_pixel;
    }

    if (special_effects_active() && special_full_effects.type == SPECIAL_EFFECT_INVERSION_1) {
        return source_pixel;
    }

    union color32f32 result = source_pixel;

    f32 r_accumulation = 0;
    f32 g_accumulation = 0;
    f32 b_accumulation = 0;

    for (s32 light_index = 0; light_index < editor_state->editing_area.lights.count; ++light_index) {
        struct light_def* current_light = editor_state->editing_area.lights.lights + light_index;
        v2f32 light_screenspace_position = current_light->position;
        /* recentering lights */
        light_screenspace_position.x += 0.5;
        light_screenspace_position.y += 0.5;
        light_screenspace_position.x *= TILE_UNIT_SIZE;
        light_screenspace_position.y *= TILE_UNIT_SIZE;

        light_screenspace_position = camera_transform(&editor_state->camera, light_screenspace_position, SCREEN_WIDTH, SCREEN_HEIGHT);
        {
            f32 distance_squared = v2f32_magnitude_sq(v2f32_sub(pixel_position, light_screenspace_position));
            f32 attenuation      = 1/(distance_squared+1 + (sqrtf(distance_squared)/1.5));

            f32 power = current_light->power * editor_state->camera.zoom;
            r_accumulation += attenuation * power * TILE_UNIT_SIZE * current_light->color.r/255.0f;
            g_accumulation += attenuation * power * TILE_UNIT_SIZE * current_light->color.g/255.0f;
            b_accumulation += attenuation * power * TILE_UNIT_SIZE * current_light->color.b/255.0f;
        }
    }

    f32 overbright_r = source_pixel.r * 2.0;
    f32 overbright_g = source_pixel.g * 2.0;
    f32 overbright_b = source_pixel.b * 2.0;

    if (r_accumulation > overbright_r) { r_accumulation = overbright_r; }
    if (g_accumulation > overbright_g) { g_accumulation = overbright_g; }
    if (b_accumulation > overbright_b) { b_accumulation = overbright_b; }

    result.r = r_accumulation + global_color_grading_filter.r/255.0f * source_pixel.r;
    result.g = g_accumulation + global_color_grading_filter.g/255.0f * source_pixel.g;
    result.b = b_accumulation + global_color_grading_filter.b/255.0f * source_pixel.b;

    return result;
}

/*
  NOTE:
  While I would like to reuse render_tile_layer_ex, I don't want to pollute it with more editor logic
  so let me just keep this here.
*/
local void editor_mode_render_tile_layer(s32 palette, struct render_commands* commands, struct tile_layer* tile_layer, s32 layer_index, s32 current_tile_layer) {
    for (s32 tile_index = 0; tile_index < tile_layer->count; ++tile_index) {
        struct tile*                 current_tile = tile_layer->tiles + tile_index;
        s32                          tile_id      = current_tile->id;
        struct tile_data_definition* tile_data    = tile_table_data + tile_id;

        if (palette == TILE_PALETTE_WORLD_MAP) {
            tile_data = world_tile_table_data + tile_id;
        }

        image_id                     tex          = get_tile_image_id(tile_data); 

        f32 alpha = 1;
        if (layer_index == -1) {
            
        } else {
            if (layer_index != current_tile_layer) {
                alpha = 0.5;
            }
        }

        render_commands_push_image(commands,
                                   graphics_assets_get_image_by_id(&graphics_assets, tex),
                                   rectangle_f32(current_tile->x * TILE_UNIT_SIZE,
                                                 current_tile->y * TILE_UNIT_SIZE,
                                                 TILE_UNIT_SIZE,
                                                 TILE_UNIT_SIZE),
                                   tile_data->sub_rectangle,
                                   color32f32(1,1,1,alpha), NO_FLAGS, BLEND_MODE_ALPHA);
    }
}

local void editor_common_render_world_map_for_preview(struct render_commands* commands, struct world_map* world_map) {
    for (s32 layer_index = 0; layer_index < array_count(world_map->tile_layers); ++layer_index) {
        editor_mode_render_tile_layer(TILE_PALETTE_WORLD_MAP, commands, world_map->tile_layers + layer_index, layer_index, -1);
    }
}

local void editor_common_render_level_area_for_preview(struct render_commands* commands, struct level_area* loaded_area) {
    struct font_cache* font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_BLUE]);
    render_ground_area(game_state, commands, loaded_area);
    {
        for (s32 entity_index = 0; entity_index < loaded_area->load_entities.count; ++entity_index) {
            struct level_area_entity* current_entity = loaded_area->load_entities.entities + entity_index;
            struct rectangle_f32 bounds = rectangle_f32(current_entity->position.x, current_entity->position.y-1, 1, 2);

            render_commands_push_quad(commands, rectangle_f32(bounds.x * TILE_UNIT_SIZE, bounds.y * TILE_UNIT_SIZE, bounds.w * TILE_UNIT_SIZE, bounds.h * TILE_UNIT_SIZE),
                                      color32u8(255, 0, 0, normalized_sinf(global_elapsed_time*2) * 0.5*255 + 64), BLEND_MODE_ALPHA);

            {
                    
                struct entity_base_data* base_def = entity_database_find_by_name(&game_state->entity_database, string_from_cstring(current_entity->base_name));
                struct entity_animation* anim = find_animation_by_name(base_def->model_index, string_concatenate(&scratch_arena, string_literal("idle_"), facing_direction_strings_normal[current_entity->facing_direction]));

                if (anim) {
                    image_id sprite_to_use = anim->sprites[0];

                    union color32f32 color = color32f32_WHITE;

                    if (current_entity->flags & ENTITY_FLAGS_HIDDEN) {
                        color.r /= 2;
                        color.g /= 2;
                        color.b /= 2;
                        color.a /= 2;
                    }

                    render_commands_push_image(commands, graphics_assets_get_image_by_id(&graphics_assets, sprite_to_use),
                                               rectangle_f32_scale(bounds, TILE_UNIT_SIZE), RECTANGLE_F32_NULL, color, NO_FLAGS, BLEND_MODE_ALPHA);
                }
            }
            s32 entity_id          = current_entity - loaded_area->load_entities.entities;
            render_commands_push_text(commands, font, 1, v2f32(bounds.x * TILE_UNIT_SIZE, bounds.y * TILE_UNIT_SIZE), string_from_cstring(format_temp("(entity %d)", entity_id)), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
        }

        for (s32 chest_index = 0; chest_index < loaded_area->chests.count; ++chest_index) {
            struct entity_chest* current_chest = loaded_area->chests.chests + chest_index;

            render_commands_push_image(commands,
                                       graphics_assets_get_image_by_id(&graphics_assets, chest_closed_img),
                                       rectangle_f32(current_chest->position.x * TILE_UNIT_SIZE,
                                                     current_chest->position.y * TILE_UNIT_SIZE,
                                                     current_chest->scale.x * TILE_UNIT_SIZE,
                                                     current_chest->scale.y * TILE_UNIT_SIZE),
                                       RECTANGLE_F32_NULL,
                                       color32f32(1,1,1,1), NO_FLAGS, BLEND_MODE_ALPHA);
        }

        for (s32 entity_savepoint_index = 0; entity_savepoint_index < loaded_area->load_savepoints.count; ++entity_savepoint_index) {
            struct level_area_savepoint* current_savepoint = loaded_area->load_savepoints.savepoints + entity_savepoint_index;
            render_commands_push_quad(commands, rectangle_f32(current_savepoint->position.x * TILE_UNIT_SIZE, current_savepoint->position.y * TILE_UNIT_SIZE,
                                                               current_savepoint->scale.x * TILE_UNIT_SIZE,    current_savepoint->scale.y * TILE_UNIT_SIZE),
                                      color32u8(255, 0, 0, 255), BLEND_MODE_ALPHA);
            s32 savepoint_id          = current_savepoint - loaded_area->load_savepoints.savepoints;
            render_commands_push_text(commands, font, 1, v2f32(current_savepoint->position.x * TILE_UNIT_SIZE, current_savepoint->position.y * TILE_UNIT_SIZE),
                                      copy_cstring_to_scratch(format_temp("(savepoint %d)", savepoint_id)), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
        }
    }

    render_foreground_area(game_state, commands, loaded_area);
}

void update_and_render_editor(struct software_framebuffer* framebuffer, f32 dt) {
    struct render_commands commands = render_commands(&scratch_arena, 16384, editor_state->camera);

    commands.should_clear_buffer = true;
    commands.clear_buffer_color  = color32u8(100, 128, 148, 255);

    if (is_key_pressed(KEY_F2)) {
        editor_state->fullbright ^= 1;
    }

    switch (editor_state->screen_state) {
        case EDITOR_SCREEN_MAIN: {
            {
                /* rendering the editor world */
                for (s32 layer_index = 0; layer_index < array_count(editor_state->editing_area.tile_layers); ++layer_index) {
                    editor_mode_render_tile_layer(TILE_PALETTE_OVERWORLD, &commands, &editor_state->editing_area.tile_layers[layer_index], layer_index, editor_state->current_tile_layer);
                }

                struct font_cache* font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_BLUE]);
                for (s32 trigger_level_transition_index = 0; trigger_level_transition_index < editor_state->editing_area.trigger_level_transitions.count; ++trigger_level_transition_index) {
                    struct trigger_level_transition* current_trigger = editor_state->editing_area.trigger_level_transitions.transitions + trigger_level_transition_index;
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

                for (s32 entity_index = 0; entity_index < editor_state->editing_area.load_entities.count; ++entity_index) {
                    struct level_area_entity* current_entity = editor_state->editing_area.load_entities.entities + entity_index;
                    struct rectangle_f32 bounds = rectangle_f32(current_entity->position.x, current_entity->position.y-1, 1, 2);

                    if (editor_state->last_selected == current_entity) {
                        render_commands_push_quad(&commands, rectangle_f32(bounds.x * TILE_UNIT_SIZE, bounds.y * TILE_UNIT_SIZE, bounds.w * TILE_UNIT_SIZE, bounds.h * TILE_UNIT_SIZE),
                                                  color32u8(255, 255, 255, normalized_sinf(global_elapsed_time*2) * 0.5*255 + 64), BLEND_MODE_ALPHA);
                    } else {
                        render_commands_push_quad(&commands, rectangle_f32(bounds.x * TILE_UNIT_SIZE, bounds.y * TILE_UNIT_SIZE, bounds.w * TILE_UNIT_SIZE, bounds.h * TILE_UNIT_SIZE),
                                                  color32u8(255, 0, 0, normalized_sinf(global_elapsed_time*2) * 0.5*255 + 64), BLEND_MODE_ALPHA);
                    }

                    {
                    
                        struct entity_base_data* base_def = entity_database_find_by_name(&game_state->entity_database, string_from_cstring(current_entity->base_name));
                        struct entity_animation* anim = find_animation_by_name(base_def->model_index, string_concatenate(&scratch_arena, string_literal("idle_"), facing_direction_strings_normal[current_entity->facing_direction]));

                        if (anim) {
                            image_id sprite_to_use = anim->sprites[0];

                            union color32f32 color = color32f32_WHITE;

                            if (current_entity->flags & ENTITY_FLAGS_HIDDEN) {
                                color.r /= 2;
                                color.g /= 2;
                                color.b /= 2;
                                color.a /= 2;
                            }

                            render_commands_push_image(&commands, graphics_assets_get_image_by_id(&graphics_assets, sprite_to_use),
                                                       rectangle_f32_scale(bounds, TILE_UNIT_SIZE), RECTANGLE_F32_NULL, color, NO_FLAGS, BLEND_MODE_ALPHA);
                        }
                    }
                    s32 entity_id          = current_entity - editor_state->editing_area.load_entities.entities;
                    render_commands_push_text(&commands, font, 1, v2f32(bounds.x * TILE_UNIT_SIZE, bounds.y * TILE_UNIT_SIZE), string_from_cstring(format_temp("(entity %d)", entity_id)), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                }

                for (s32 generic_trigger_index = 0; generic_trigger_index < editor_state->editing_area.triggers.count; ++generic_trigger_index) {
                    struct trigger* current_trigger = editor_state->editing_area.triggers.triggers + generic_trigger_index;

                    if (editor_state->last_selected == current_trigger) {
                        render_commands_push_quad(&commands, rectangle_f32(current_trigger->bounds.x * TILE_UNIT_SIZE, current_trigger->bounds.y * TILE_UNIT_SIZE,
                                                                           current_trigger->bounds.w * TILE_UNIT_SIZE, current_trigger->bounds.h * TILE_UNIT_SIZE),
                                                  color32u8(255, 255, 255, normalized_sinf(global_elapsed_time*2) * 0.5*255 + 64), BLEND_MODE_ALPHA);
                    } else {
                        render_commands_push_quad(&commands, rectangle_f32(current_trigger->bounds.x * TILE_UNIT_SIZE, current_trigger->bounds.y * TILE_UNIT_SIZE,
                                                                           current_trigger->bounds.w * TILE_UNIT_SIZE, current_trigger->bounds.h * TILE_UNIT_SIZE),
                                                  color32u8(255, 0, 0, normalized_sinf(global_elapsed_time*2) * 0.5*255 + 64), BLEND_MODE_ALPHA);
                    }
                    s32 trigger_id          = current_trigger - editor_state->editing_area.triggers.triggers;
                    render_commands_push_text(&commands, font, 1, v2f32(current_trigger->bounds.x * TILE_UNIT_SIZE, current_trigger->bounds.y * TILE_UNIT_SIZE),
                                              copy_cstring_to_scratch(format_temp("(trigger %d)", trigger_id)), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                }

                for (s32 chest_index = 0; chest_index < editor_state->editing_area.chests.count; ++chest_index) {
                    struct entity_chest* current_chest = editor_state->editing_area.chests.chests + chest_index;

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

                for (s32 light_index = 0; light_index < editor_state->editing_area.lights.count; ++light_index) {
                    struct light_def* current_light = editor_state->editing_area.lights.lights + light_index;
                
                    if (editor_state->last_selected == current_light) {
                        render_commands_push_quad(&commands, rectangle_f32(current_light->position.x * TILE_UNIT_SIZE, current_light->position.y * TILE_UNIT_SIZE,
                                                                           current_light->scale.x * TILE_UNIT_SIZE,    current_light->scale.y * TILE_UNIT_SIZE),
                                                  color32u8(current_light->color.r, current_light->color.g, current_light->color.b, normalized_sinf(global_elapsed_time*2) * 0.5*255 + 64), BLEND_MODE_ALPHA);
                    } else {
                        render_commands_push_quad(&commands, rectangle_f32(current_light->position.x * TILE_UNIT_SIZE, current_light->position.y * TILE_UNIT_SIZE,
                                                                           current_light->scale.x * TILE_UNIT_SIZE,    current_light->scale.y * TILE_UNIT_SIZE),
                                                  color32u8(255, 0, 0, normalized_sinf(global_elapsed_time*2) * 0.5*255 + 64), BLEND_MODE_ALPHA);
                    }
                    s32 light_id          = current_light - editor_state->editing_area.lights.lights;
                    render_commands_push_text(&commands, font, 1, v2f32(current_light->position.x * TILE_UNIT_SIZE, current_light->position.y * TILE_UNIT_SIZE),
                                              copy_cstring_to_scratch(format_temp("(light %d)", light_id)), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                }

                for (s32 entity_savepoint_index = 0; entity_savepoint_index < editor_state->editing_area.load_savepoints.count; ++entity_savepoint_index) {
                    struct level_area_savepoint* current_savepoint = editor_state->editing_area.load_savepoints.savepoints + entity_savepoint_index;
                    if (editor_state->last_selected == current_savepoint) {
                        render_commands_push_quad(&commands, rectangle_f32(current_savepoint->position.x * TILE_UNIT_SIZE, current_savepoint->position.y * TILE_UNIT_SIZE,
                                                                           current_savepoint->scale.x * TILE_UNIT_SIZE,    current_savepoint->scale.y * TILE_UNIT_SIZE),
                                                  color32u8(0, 255, 0, 255), BLEND_MODE_ALPHA);
                    } else {
                        render_commands_push_quad(&commands, rectangle_f32(current_savepoint->position.x * TILE_UNIT_SIZE, current_savepoint->position.y * TILE_UNIT_SIZE,
                                                                           current_savepoint->scale.x * TILE_UNIT_SIZE,    current_savepoint->scale.y * TILE_UNIT_SIZE),
                                                  color32u8(255, 0, 0, 255), BLEND_MODE_ALPHA);
                    }
                    s32 savepoint_id          = current_savepoint - editor_state->editing_area.load_savepoints.savepoints;
                    render_commands_push_text(&commands, font, 1, v2f32(current_savepoint->position.x * TILE_UNIT_SIZE, current_savepoint->position.y * TILE_UNIT_SIZE),
                                              copy_cstring_to_scratch(format_temp("(savepoint %d)", savepoint_id)), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                }

                for (s32 position_marker_index = 0; position_marker_index < editor_state->editing_area.position_markers.count; ++position_marker_index) {
                    struct position_marker* current_marker = editor_state->editing_area.position_markers.markers + position_marker_index;
                    if (editor_state->last_selected == current_marker) {
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

                if (editor_state->tool_mode == EDITOR_TOOL_BATTLETILE_PAINTING) {
                    for (s32 battle_safe_tile_index = 0; battle_safe_tile_index < editor_state->editing_area.battle_safe_squares.count; ++battle_safe_tile_index) {
                        struct level_area_battle_safe_square* current_square = editor_state->editing_area.battle_safe_squares.squares + battle_safe_tile_index;
                        render_commands_push_quad(&commands, rectangle_f32(current_square->x * TILE_UNIT_SIZE, current_square->y * TILE_UNIT_SIZE, 1 * TILE_UNIT_SIZE,    1 * TILE_UNIT_SIZE),
                                                  color32u8(0, 255, 0, 100), BLEND_MODE_ALPHA);
                    }
                }
            }
        
            render_commands_push_quad(&commands, rectangle_f32(editor_state->editing_area.default_player_spawn.x, editor_state->editing_area.default_player_spawn.y, TILE_UNIT_SIZE/4, TILE_UNIT_SIZE/4),
                                      color32u8(0, 255, 0, normalized_sinf(global_elapsed_time*4) * 0.5*255 + 64), BLEND_MODE_ALPHA);

            switch (editor_state->tool_mode) {
                case 0: {
                    {
                        s32 mouse_location[2];
                        get_mouse_location(mouse_location, mouse_location+1);

                        v2f32 world_space_mouse_location = editor_get_world_space_mouse_location();
                        v2f32 tile_space_mouse_location = world_space_mouse_location; {
                            tile_space_mouse_location.x = floorf(world_space_mouse_location.x / TILE_UNIT_SIZE);
                            tile_space_mouse_location.y = floorf(world_space_mouse_location.y / TILE_UNIT_SIZE);
                        }
                        for (s32 y_index = 0; y_index < EDITOR_BRUSH_SQUARE_SIZE; ++y_index) {
                            for (s32 x_index = 0; x_index < EDITOR_BRUSH_SQUARE_SIZE; ++x_index) {
                                if (editor_brush_patterns[editor_state->editor_brush_pattern][y_index][x_index] == 1) {
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
                } break;
            }

            editor_state->actor_property_menu.facing_direction_spin_timer += dt;
            if (editor_state->actor_property_menu.facing_direction_spin_timer >= FACING_DIRECTION_SPIN_TIMER_LENGTH_MAX) {
                editor_state->actor_property_menu.facing_direction_index_for_animation += 1;

                if (editor_state->actor_property_menu.facing_direction_index_for_animation > 3) {
                    editor_state->actor_property_menu.facing_direction_index_for_animation = 0;
                }

                editor_state->actor_property_menu.facing_direction_spin_timer = 0;
            }
        } break;
        case EDITOR_SCREEN_PLACING_TRANSITION_SPAWN_LEVEL_AREA: {
            editor_common_render_level_area_for_preview(&commands, &editor_state->loaded_area);
            struct trigger_level_transition* transition = editor_state->last_selected;
            render_commands_push_quad(&commands,
                                      rectangle_f32(transition->spawn_location.x * TILE_UNIT_SIZE, transition->spawn_location.y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE),
                                      color32u8(0, 255, 255, 255), BLEND_MODE_ALPHA);
        } break;
        case EDITOR_SCREEN_PLACING_TRANSITION_SPAWN_WORLD_MAP: {
            editor_common_render_world_map_for_preview(&commands, &editor_state->loaded_world_map);
            struct trigger_level_transition* transition = editor_state->last_selected;
            render_commands_push_quad(&commands,
                                      rectangle_f32(transition->spawn_location.x * TILE_UNIT_SIZE, transition->spawn_location.y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE),
                                      color32u8(0, 255, 255, 255), BLEND_MODE_ALPHA);
        } break;
    }

    software_framebuffer_render_commands(framebuffer, &commands);
    software_framebuffer_run_shader(framebuffer, rectangle_f32(0, 0, framebuffer->width, framebuffer->height), editor_lighting_shader, NULL);
    EDITOR_imgui_end_frame();
}

void update_and_render_editor_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    switch (state->ui_state) {
        case UI_STATE_INGAME: {
            update_and_render_editor_game_menu_ui(state, framebuffer, dt);
        } break;
        case UI_STATE_PAUSE: {
            update_and_render_pause_editor_menu_ui(state, framebuffer, dt);
        } break;
            bad_case;
    }
}

