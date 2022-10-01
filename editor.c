/*
  Level testing behavior may no longer work, as they may now require dependence on a script file,
  which bares their name.
  
  I can modify it slightly I guess...
  Or I can try to include the script as part of the level which would be a pretty herculean job
  considering the relative complexity of the tools...
*/

/*
  NOTE: would be nice ot make the rendering apply lighting later.
  Not doing it right now though.
*/

/*
  WHOCARES: May crash if touching the last_selected pointer incorrectly?
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

local v2f32 editor_get_world_space_mouse_location(void) {
    s32 mouse_location[2];
    get_mouse_location(mouse_location, mouse_location+1);
    return camera_project(&editor_state->camera, v2f32(mouse_location[0], mouse_location[1]), SCREEN_WIDTH, SCREEN_HEIGHT);
}

#include "editor_imgui.c"

void editor_clear_all_allocations(struct editor_state* state) {
    zero_array(state->tile_counts);
    state->trigger_level_transition_count = 0;
    state->entity_chest_count             = 0;
    state->generic_trigger_count          = 0;
    state->entity_count                   = 0;
    state->light_count                    = 0;
}

void editor_clear_all(struct editor_state* state) {
    editor_clear_all_allocations(state);

    state->camera.xy       = v2f32(0,0);
    state->camera.zoom     = 1;
    /* not centered to simplify code */
    state->camera.centered = 1;
}

void editor_initialize(struct editor_state* state) {
    state->arena = &editor_arena;
    for (s32 index = 0; index < TILE_LAYER_COUNT; ++index) {
        state->tile_capacities[index] = 8192;
    }
    state->trigger_level_transition_capacity = 1024;
    state->entity_chest_capacity             = 1024;
    state->generic_trigger_capacity          = 1024;
    state->entity_capacity                   = 512;
    state->light_capacity                    = 256;

    for (s32 index = 0; index < TILE_LAYER_COUNT; ++index) {
        state->tile_layers[index] = memory_arena_push(state->arena, state->tile_capacities[index] * sizeof(*state->tile_layers[0]));
    }
    state->trigger_level_transitions                = memory_arena_push(state->arena, state->trigger_level_transition_capacity * sizeof(*state->trigger_level_transitions));
    state->entity_chests                            = memory_arena_push(state->arena, state->entity_chest_capacity             * sizeof(*state->entity_chests));
    state->generic_triggers                         = memory_arena_push(state->arena, state->generic_trigger_capacity          * sizeof(*state->generic_triggers));
    state->entities                                 = memory_arena_push(state->arena, state->entity_capacity                   * sizeof(*state->entities));
    state->lights                                   = memory_arena_push(state->arena, state->light_capacity                    * sizeof(*state->lights));
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

    if (version_id >= 4) {
        if (version_id < CURRENT_LEVEL_AREA_VERSION) {
            /* for older versions I have to know what the tile layers were and assign them like this. */
            switch (version_id) {
                case 4: {
                    Serialize_Fixed_Array(serializer, s32, editor_state->tile_counts[TILE_LAYER_GROUND],     editor_state->tile_layers[TILE_LAYER_GROUND]);
                    Serialize_Fixed_Array(serializer, s32, editor_state->tile_counts[TILE_LAYER_OBJECT],     editor_state->tile_layers[TILE_LAYER_OBJECT]);
                    Serialize_Fixed_Array(serializer, s32, editor_state->tile_counts[TILE_LAYER_ROOF],       editor_state->tile_layers[TILE_LAYER_ROOF]);
                    Serialize_Fixed_Array(serializer, s32, editor_state->tile_counts[TILE_LAYER_FOREGROUND], editor_state->tile_layers[TILE_LAYER_FOREGROUND]);
                } break;
                default: {
                    goto didnt_change_level_tile_format_from_current;
                } break;
            }
        } else {
            /* the current version of the tile layering, we can just load them in order. */
        didnt_change_level_tile_format_from_current:
            for (s32 index = 0; index < TILE_LAYER_COUNT; ++index) {
                Serialize_Fixed_Array(serializer, s32, editor_state->tile_counts[index], editor_state->tile_layers[index]);
            }
        }
    } else {
        Serialize_Fixed_Array(serializer, s32, editor_state->tile_counts[TILE_LAYER_OBJECT], editor_state->tile_layers[TILE_LAYER_OBJECT]);
    }

    if (version_id >= 1) {
        Serialize_Fixed_Array(serializer, s32, editor_state->trigger_level_transition_count, editor_state->trigger_level_transitions);
    }
    if (version_id >= 2) {
        Serialize_Fixed_Array(serializer, s32, editor_state->entity_chest_count, editor_state->entity_chests);
    }
    if (version_id >= 3) {
        Serialize_Fixed_Array(serializer, s32, editor_state->generic_trigger_count, editor_state->generic_triggers);
    }
    if (version_id >= 5) {
        serialize_s32(serializer, &editor_state->entity_count);
        for (s32 entity_index = 0; entity_index < editor_state->entity_count; ++entity_index) {
            serialize_level_area_entity(serializer, version_id, &editor_state->entities[entity_index]);
        }
    }
    if (version_id >= 6) {
        serialize_s32(serializer, &editor_state->light_count);
        for (s32 light_index = 0; light_index < editor_state->light_count; ++light_index) {
            Serialize_Structure(serializer, editor_state->lights[light_index]);
        }
    }
}

void editor_remove_tile_at(v2f32 point_in_tilespace) {
    s32 where_x = point_in_tilespace.x;
    s32 where_y = point_in_tilespace.y;

    for (s32 index = 0; index < editor_state->tile_counts[editor_state->current_tile_layer]; ++index) {
        struct tile* current_tile = editor_state->tile_layers[editor_state->current_tile_layer] + index;

        /* override */
        if (current_tile->x == where_x && current_tile->y == where_y) {
            if (current_tile == editor_state->last_selected) editor_state->last_selected = 0;

            current_tile->id = 0;
            editor_state->tile_layers[editor_state->current_tile_layer][index] = editor_state->tile_layers[editor_state->current_tile_layer][--editor_state->tile_counts[editor_state->current_tile_layer]];
            return;
        }
    }
}
void editor_remove_scriptable_transition_trigger_at(v2f32 point_in_tilespace) {
    for (s32 index = 0; index < editor_state->generic_trigger_count; ++index) {
        struct trigger* current_trigger = editor_state->generic_triggers + index;

        if (rectangle_f32_intersect(current_trigger->bounds, rectangle_f32(point_in_tilespace.x, point_in_tilespace.y, 0.25, 0.25))) {
            editor_state->generic_triggers[index] = editor_state->generic_triggers[--editor_state->generic_trigger_count];
            return;
        }
    }
}
void editor_remove_level_transition_trigger_at(v2f32 point_in_tilespace) {
    for (s32 index = 0; index < editor_state->trigger_level_transition_count; ++index) {
        struct trigger_level_transition* current_trigger = editor_state->trigger_level_transitions + index;

        if (rectangle_f32_intersect(current_trigger->bounds, rectangle_f32(point_in_tilespace.x, point_in_tilespace.y, 0.25, 0.25))) {
            editor_state->trigger_level_transitions[index] = editor_state->trigger_level_transitions[--editor_state->trigger_level_transition_count];
            return;
        }
    }
}

void editor_place_tile_at(v2f32 point_in_tilespace) {
    s32 where_x = point_in_tilespace.x;
    s32 where_y = point_in_tilespace.y;

    for (s32 index = 0; index < editor_state->tile_counts[editor_state->current_tile_layer]; ++index) {
        struct tile* current_tile = editor_state->tile_layers[editor_state->current_tile_layer] + index;

        /* override */
        if (current_tile->x == where_x && current_tile->y == where_y) {
            current_tile->id = editor_state->painting_tile_id;
            editor_state->last_selected = current_tile;
            return;
        }
    }

    struct tile* new_tile = editor_state->tile_layers[editor_state->current_tile_layer] + (editor_state->tile_counts[editor_state->current_tile_layer]++);
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

            if (rectangle_f32_intersect(current_trigger->bounds, rectangle_f32(point_in_tilespace.x, point_in_tilespace.y, 0.25, 0.25))) {
                editor_state->last_selected = current_trigger;
                set_drag_candidate_rectangle(current_trigger, get_mouse_in_tile_space(&editor_state->camera, SCREEN_WIDTH, SCREEN_HEIGHT),
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

/*
  NOTE: this is recording level_area_entity entities, which are not like the other entities. These actually have normal coordinate systems.
 */
void editor_place_or_drag_actor(v2f32 point_in_tilespace) {
    if (is_dragging()) {
        return; 
    }

    {
        for (s32 index = 0; index < editor_state->entity_count; ++index) {
            struct level_area_entity* current_entity = editor_state->entities + index;

            /* The size should really depend on the model. Oh well. */
            if (rectangle_f32_intersect(rectangle_f32(current_entity->position.x, current_entity->position.y-1, 1, 2), rectangle_f32(point_in_tilespace.x, point_in_tilespace.y, 0.25, 0.25))) {
                editor_state->last_selected = current_entity;
                set_drag_candidate(current_entity, get_mouse_in_tile_space(&editor_state->camera, SCREEN_WIDTH, SCREEN_HEIGHT), current_entity->position);
                return;
            }
        }


        /* otherwise no touch, place a new one at default size 1 1 */
        struct level_area_entity* new_entity = &editor_state->entities[editor_state->entity_count++];
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

local const f32 LIGHT_GRAB_SQ_SZ = 1 * TILE_UNIT_SIZE;
void editor_place_or_drag_light(v2f32 point_in_tilespace) {
    if (is_dragging()) {
        return;
    }

    {
        for (s32 light_index = 0; light_index < editor_state->light_count; ++light_index) {
            struct light_def* current_light = editor_state->lights + light_index;

            if (rectangle_f32_intersect(rectangle_f32((current_light->position.x), (current_light->position.y), 1, 1),
                                        rectangle_f32(point_in_tilespace.x, point_in_tilespace.y, 0.25, 0.25))) {
                editor_state->last_selected = current_light;
                set_drag_candidate(current_light, get_mouse_in_tile_space(&editor_state->camera, SCREEN_WIDTH, SCREEN_HEIGHT), current_light->position);
                return;
            }
        }

        struct light_def* new_light = &editor_state->lights[editor_state->light_count++];
        new_light->position         = point_in_tilespace;
        new_light->power            = 3;
        new_light->color            = color32u8_WHITE;
        new_light->flags            = 0;
        new_light->scale            = v2f32(1, 1); /* for the dragging code which always expects a rectangle. */
        zero_array(new_light->reserved_bytes);

        editor_state->last_selected = new_light;
    }
}

void editor_remove_light_at(v2f32 point_in_tilespace) {
    for (s32 light_index = 0; light_index < editor_state->light_count; ++light_index) {
        struct light_def* current_light = editor_state->lights + light_index;

        if (rectangle_f32_intersect(rectangle_f32((current_light->position.x), (current_light->position.y), 1, 1),
                                    rectangle_f32(point_in_tilespace.x, point_in_tilespace.y, 0.25, 0.25))) {
            editor_state->lights[light_index] = editor_state->lights[--editor_state->light_count];
            return;
        }
    }
}

void editor_remove_actor_at(v2f32 point_in_tilespace) {
    for (s32 index = 0; index < editor_state->entity_count; ++index) {
        struct level_area_entity* current_entity = editor_state->entities + index;

        if (rectangle_f32_intersect(rectangle_f32(current_entity->position.x, current_entity->position.y-1, 1, 2), rectangle_f32(point_in_tilespace.x, point_in_tilespace.y, 0.25, 0.25))) {
            editor_state->entities[index] = editor_state->entities[--editor_state->entity_count];
            return;
        }
    }
}

void editor_place_or_drag_scriptable_transition_trigger(v2f32 point_in_tilespace) {
    if (is_dragging()) {
        return; 
    }

    {
        for (s32 index = 0; index < editor_state->generic_trigger_count; ++index) {
            struct trigger* current_trigger = editor_state->generic_triggers + index;

            if (rectangle_f32_intersect(current_trigger->bounds, rectangle_f32(point_in_tilespace.x, point_in_tilespace.y, 0.25, 0.25))) {
                editor_state->last_selected = current_trigger;
                set_drag_candidate_rectangle(current_trigger, get_mouse_in_tile_space(&editor_state->camera, SCREEN_WIDTH, SCREEN_HEIGHT),
                                             v2f32(current_trigger->bounds.x, current_trigger->bounds.y),
                                             v2f32(current_trigger->bounds.w, current_trigger->bounds.h));
                return;
            }
        }


        /* otherwise no touch, place a new one at default size 1 1 */
        struct trigger* new_trigger = &editor_state->generic_triggers[editor_state->generic_trigger_count++];
        new_trigger->bounds.x       = point_in_tilespace.x;
        new_trigger->bounds.y       = point_in_tilespace.y;
        new_trigger->bounds.w       = 1;
        new_trigger->bounds.h       = 1;
        editor_state->last_selected = new_trigger;
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
                set_drag_candidate(current_chest, get_mouse_in_tile_space(&editor_state->camera, SCREEN_WIDTH, SCREEN_HEIGHT),
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

            v2f32 mouse_position_in_tilespace_rounded = get_mouse_in_tile_space_integer(&editor_state->camera, SCREEN_WIDTH, SCREEN_HEIGHT);

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
                case ENTITY_PLACEMENT_TYPE_ACTOR: {
                    if (left_clicked) {
                        editor_place_or_drag_actor(tile_space_mouse_location);
                    } else if (right_clicked) {
                        editor_remove_actor_at(tile_space_mouse_location);
                    }
                } break;
                case ENTITY_PLACEMENT_TYPE_CHEST: {
                    if (left_clicked) {
                        editor_place_or_drag_chest(tile_space_mouse_location);
                    } else if (right_clicked) {
                        editor_remove_chest_at(tile_space_mouse_location);
                    }
                } break;
                case ENTITY_PLACEMENT_TYPE_LIGHT: {
                    if (left_clicked) {
                        editor_place_or_drag_light(tile_space_mouse_location);
                    } else if (right_clicked) {
                        editor_remove_light_at(tile_space_mouse_location);
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

                            /* Is there a reason this is here? */
                            v2f32 world_space_mouse_location =
                                camera_project(&editor_state->camera, v2f32(mouse_location[0], mouse_location[1]), SCREEN_WIDTH, SCREEN_HEIGHT);

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
                case TRIGGER_PLACEMENT_TYPE_SCRIPTABLE_TRIGGER: {
                    if (left_clicked) {
                        /* NOTE check the trigger mode */
                        editor_place_or_drag_scriptable_transition_trigger(tile_space_mouse_location);
                    } else if (right_clicked) {
                        editor_remove_scriptable_transition_trigger_at(tile_space_mouse_location);
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
                                /*NOTE: 
                                  level testing code
                                  
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

    v2f32 world_space_mouse_location = editor_get_world_space_mouse_location();

    /* for tiles */
    v2f32 tile_space_mouse_location = world_space_mouse_location; {
        tile_space_mouse_location.x = floorf(world_space_mouse_location.x / TILE_UNIT_SIZE);
        tile_space_mouse_location.y = floorf(world_space_mouse_location.y / TILE_UNIT_SIZE);
    }

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
            editor_state->camera.zoom += 1;
            if (editor_state->camera.zoom >= 2) {
                editor_state->camera.zoom = 2;
            }
        } else if (is_mouse_wheel_down()) {
            editor_state->camera.zoom -= 1;
            if (editor_state->camera.zoom <= 0) {
                editor_state->camera.zoom = 1;
            }
        }

        if (last_zoom != editor_state->camera.zoom) {
            v2f32 focused_pixel_a = world_space_mouse_location;
            v2f32 focused_pixel_b = world_space_mouse_location;
            /* pixel in last zoom level */

            /* NOTE: will figure out proper math later, only works for one level of zoom lol, I'm bad at these transforms */
            if (last_zoom > editor_state->camera.zoom) {
                focused_pixel_b.x /= editor_state->camera.zoom;
                focused_pixel_b.y /= editor_state->camera.zoom;
                /* same pixel in current zoom level */
                focused_pixel_b.x *= last_zoom;
                focused_pixel_b.y *= last_zoom;
            } else {
                focused_pixel_a.x /= last_zoom;
                focused_pixel_a.y /= last_zoom;
                /* same pixel in current zoom level */
                focused_pixel_a.x *= editor_state->camera.zoom;
                focused_pixel_a.y *= editor_state->camera.zoom;
            }

            f32 delta_x = focused_pixel_b.x - focused_pixel_a.x;
            f32 delta_y = focused_pixel_b.y - focused_pixel_a.y;

            _debugprintf("delta: %f(%f, %f), %f(%f, %f)", delta_x, focused_pixel_b.x, focused_pixel_a.x,
                         delta_y, focused_pixel_b.y, focused_pixel_a.y);

            editor_state->camera.xy.x -= delta_x;
            editor_state->camera.xy.y -= delta_y;
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
            snprintf(tmp_text, 1024, "mode: %.*s", editor_tool_mode_strings[editor_state->tool_mode].length, editor_tool_mode_strings[editor_state->tool_mode].data);
            software_framebuffer_draw_text(framebuffer,
                                           graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]),
                                           1, v2f32(0,y_cursor), string_from_cstring(tmp_text), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
        }
        switch (editor_state->tool_mode) {
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
            
            software_framebuffer_draw_text(framebuffer,
                                           graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]),
                                           1, v2f32(0,y_cursor), string_from_cstring(format_temp("Mouse: (tx: %d, ty: %d)", (s32)mouse_tilespace.x, (s32)mouse_tilespace.y)), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
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
            if (!editor_state->last_selected) {
                editor_state->tab_menu_open = 0;
            } else {
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
                            case TRIGGER_PLACEMENT_TYPE_SCRIPTABLE_TRIGGER: {
                                f32 draw_cursor_y = 30;
                                struct trigger* trigger = editor_state->last_selected;
                                s32 trigger_id          = trigger - editor_state->generic_triggers;

                                string activation_type_string = activation_type_strings[trigger->activation_method];
                                if(EDITOR_imgui_button(framebuffer, font, highlighted_font, 2, v2f32(16, draw_cursor_y),
                                                       string_from_cstring(format_temp("Activation Type: %.*s", activation_type_string.length, activation_type_string.data)))) {
                                    trigger->activation_method += 1;
                                    if (trigger->activation_method >= ACTIVATION_TYPE_COUNT) {
                                        trigger->activation_method = 0;
                                    }
                                }
                            } break;
                        }
                    } break;
                    case EDITOR_TOOL_ENTITY_PLACEMENT: {
                        switch (editor_state->entity_placement_type) {
                            case ENTITY_PLACEMENT_TYPE_ACTOR: {
                                f32 draw_cursor_y = 70;
                                const f32 text_scale = 1;

                                struct level_area_entity* entity = editor_state->last_selected;

                                {
                                    string s = string_clone(&scratch_arena, string_from_cstring(format_temp("base_id: %s", entity->base_name)));
                                    if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 2, v2f32(10, draw_cursor_y), s)) {
                                        editor_state->actor_property_menu.picking_entity_base ^= 1;
                                        editor_state->actor_property_menu.item_list_scroll_y   = 0;
                                    }
                                    draw_cursor_y += 16 * 2 * 1.5 + TILE_UNIT_SIZE*2.3;
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

                                        s32 ENTRIES_PER_ROW = (500 / largest_name_width);
                                        s32 row_count     = (tile_table_data_count / ENTRIES_PER_ROW)+1;

                                        for (s32 row_index = 0; row_index < row_count; ++row_index) {
                                            f32 draw_cursor_x = 0;

                                            for (s32 index = 0; index < ENTRIES_PER_ROW; ++index) {
                                                s32 entity_data_index = row_index * ENTRIES_PER_ROW + index;
                                                if (!(entity_data_index < entities->entity_count)) {
                                                    break;
                                                }

                                                struct entity_base_data* data = entity_database_find_by_index(entities, entity_data_index);
                                                struct entity_animation* anim = find_animation_by_name(data->model_index, facing_direction_strings_normal[editor_state->actor_property_menu.facing_direction_index_for_animation]);

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

                                            draw_cursor_y += TILE_UNIT_SIZE*2 * 1.2 * 2;
                                        }
                                    }
                                } else {
                                    string facing_direction_string = facing_direction_strings[entity->facing_direction];
                                    {
                                        string s = string_clone(&scratch_arena, string_from_cstring(format_temp("facing direction: %.*s", facing_direction_string.length, facing_direction_string.data)));
                                        s32 result = EDITOR_imgui_button(framebuffer, font, highlighted_font, 2, v2f32(10, draw_cursor_y), s);

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
                                        draw_cursor_y += 16 * 2 * 1.5;
                                    }

                                    {
                                        string s = string_clone(&scratch_arena, string_from_cstring(format_temp("hidden: %s", cstr_yesno[(entity->flags & ENTITY_FLAGS_HIDDEN) > 0])));
                                        if (EDITOR_imgui_button(framebuffer, font, highlighted_font, 2, v2f32(10, draw_cursor_y), s)) {
                                            entity->flags ^= ENTITY_FLAGS_HIDDEN;
                                        }
                                        draw_cursor_y += 16 * 2 * 1.5;
                                    }
                                    {
                                    
                                        draw_cursor_y += 16 * 2 * 1.5;
                                    }
                                }

                            } break;
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
                            case ENTITY_PLACEMENT_TYPE_LIGHT: {
                                f32 draw_cursor_y = 70;
                                struct light_def* current_light = editor_state->last_selected;

                                EDITOR_imgui_text_edit_f32(framebuffer, font, highlighted_font, 2, v2f32(15, draw_cursor_y), string_literal("Light Power: "), &current_light->power);
                                draw_cursor_y += 16 * 2.5;
                                EDITOR_imgui_text_edit_u8(framebuffer, font, highlighted_font, 2, v2f32(15, draw_cursor_y), string_literal("Light R: "), &current_light->color.r);
                                draw_cursor_y += 16 * 2.5;
                                EDITOR_imgui_text_edit_u8(framebuffer, font, highlighted_font, 2, v2f32(15, draw_cursor_y), string_literal("Light G: "), &current_light->color.g);
                                draw_cursor_y += 16 * 2.5;
                                EDITOR_imgui_text_edit_u8(framebuffer, font, highlighted_font, 2, v2f32(15, draw_cursor_y), string_literal("Light B: "), &current_light->color.b);
                                draw_cursor_y += 16 * 2.5;
                                EDITOR_imgui_text_edit_u8(framebuffer, font, highlighted_font, 2, v2f32(15, draw_cursor_y), string_literal("Light A: "), &current_light->color.a);
                            } break;
                        }
                    } break;
                }
            }
        } else {
            switch (editor_state->tool_mode) {
                /* I would show images, but this is easier for now */
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

                    s32 TILES_PER_ROW = (500 / largest_name_width);
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

                        draw_cursor_y += 24 * 1.2 * 2;
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
    struct render_commands commands = render_commands(&scratch_arena, 4096,  editor_state->camera);

    commands.should_clear_buffer = true;
    commands.clear_buffer_color  = color32u8(100, 128, 148, 255);

    if (editor_state->viewing_loaded_area) {
        /* yeah this is a big mess */
        render_ground_area(game_state, &commands, &editor_state->loaded_area);
        if (editor_state->last_selected && editor_state->tool_mode == EDITOR_TOOL_TRIGGER_PLACEMENT) {
            struct trigger_level_transition* trigger = editor_state->last_selected;
            render_commands_push_quad(&commands, rectangle_f32(trigger->spawn_location.x * TILE_UNIT_SIZE, trigger->spawn_location.y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE),
                                      color32u8(0, 255, 255, normalized_sinf(global_elapsed_time*4) * 0.5*255 + 64), BLEND_MODE_ALPHA);
        }
        render_foreground_area(game_state, &commands, &editor_state->loaded_area);
    } else {
        /* Render world */
        {
            for (s32 layer_index = 0; layer_index < array_count(editor_state->tile_layers); ++layer_index) {
                for (s32 tile_index = 0; tile_index < editor_state->tile_counts[layer_index]; ++tile_index) {
                    struct tile*                 current_tile = editor_state->tile_layers[layer_index] + tile_index;
                    s32                          tile_id      = current_tile->id;
                    struct tile_data_definition* tile_data    = tile_table_data + tile_id;
                    image_id                     tex          = get_tile_image_id(tile_data); 

                    f32 alpha = 1;
                    if (layer_index != editor_state->current_tile_layer) {
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

            for (s32 entity_index = 0; entity_index < editor_state->entity_count; ++entity_index) {
                struct level_area_entity* current_entity = editor_state->entities + entity_index;
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
                    struct entity_animation* anim = find_animation_by_name(base_def->model_index, facing_direction_strings_normal[current_entity->facing_direction]);

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
                s32 entity_id          = current_entity - editor_state->entities;
                render_commands_push_text(&commands, font, 1, v2f32(bounds.x * TILE_UNIT_SIZE, bounds.y * TILE_UNIT_SIZE), string_from_cstring(format_temp("(entity %d)", entity_id)), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            }

            for (s32 generic_trigger_index = 0; generic_trigger_index < editor_state->generic_trigger_count; ++generic_trigger_index) {
                struct trigger* current_trigger = editor_state->generic_triggers + generic_trigger_index;

                if (editor_state->last_selected == current_trigger) {
                    render_commands_push_quad(&commands, rectangle_f32(current_trigger->bounds.x * TILE_UNIT_SIZE, current_trigger->bounds.y * TILE_UNIT_SIZE,
                                                                       current_trigger->bounds.w * TILE_UNIT_SIZE, current_trigger->bounds.h * TILE_UNIT_SIZE),
                                              color32u8(255, 255, 255, normalized_sinf(global_elapsed_time*2) * 0.5*255 + 64), BLEND_MODE_ALPHA);
                } else {
                    render_commands_push_quad(&commands, rectangle_f32(current_trigger->bounds.x * TILE_UNIT_SIZE, current_trigger->bounds.y * TILE_UNIT_SIZE,
                                                                       current_trigger->bounds.w * TILE_UNIT_SIZE, current_trigger->bounds.h * TILE_UNIT_SIZE),
                                              color32u8(255, 0, 0, normalized_sinf(global_elapsed_time*2) * 0.5*255 + 64), BLEND_MODE_ALPHA);
                }
                s32 trigger_id          = current_trigger - editor_state->generic_triggers;
                render_commands_push_text(&commands, font, 1, v2f32(current_trigger->bounds.x * TILE_UNIT_SIZE, current_trigger->bounds.y * TILE_UNIT_SIZE),
                                          copy_cstring_to_scratch(format_temp("(trigger %d)", trigger_id)), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
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

            for (s32 light_index = 0; light_index < editor_state->light_count; ++light_index) {
                struct light_def* current_light = editor_state->lights + light_index;
                
                if (editor_state->last_selected == current_light) {
                    render_commands_push_quad(&commands, rectangle_f32(current_light->position.x * TILE_UNIT_SIZE, current_light->position.y * TILE_UNIT_SIZE,
                                                                       current_light->scale.x * TILE_UNIT_SIZE,    current_light->scale.y * TILE_UNIT_SIZE),
                                              color32u8(current_light->color.r, current_light->color.g, current_light->color.b, normalized_sinf(global_elapsed_time*2) * 0.5*255 + 64), BLEND_MODE_ALPHA);
                } else {
                    render_commands_push_quad(&commands, rectangle_f32(current_light->position.x * TILE_UNIT_SIZE, current_light->position.y * TILE_UNIT_SIZE,
                                                                       current_light->scale.x * TILE_UNIT_SIZE,    current_light->scale.y * TILE_UNIT_SIZE),
                                              color32u8(255, 0, 0, normalized_sinf(global_elapsed_time*2) * 0.5*255 + 64), BLEND_MODE_ALPHA);
                }
                s32 light_id          = current_light - editor_state->lights;
                render_commands_push_text(&commands, font, 1, v2f32(current_light->position.x * TILE_UNIT_SIZE, current_light->position.y * TILE_UNIT_SIZE),
                                          copy_cstring_to_scratch(format_temp("(light %d)", light_id)), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            }
        }
        
        render_commands_push_quad(&commands, rectangle_f32(editor_state->default_player_spawn.x, editor_state->default_player_spawn.y, TILE_UNIT_SIZE/4, TILE_UNIT_SIZE/4),
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

                    render_commands_push_quad(&commands, rectangle_f32(tile_space_mouse_location.x * TILE_UNIT_SIZE, tile_space_mouse_location.y * TILE_UNIT_SIZE,
                                                                       TILE_UNIT_SIZE, TILE_UNIT_SIZE),
                                              color32u8(0, 0, 255, normalized_sinf(global_elapsed_time*4) * 0.5*255 + 64), BLEND_MODE_ALPHA);
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
    }

    /* cursor ghost */
    software_framebuffer_render_commands(framebuffer, &commands);
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

