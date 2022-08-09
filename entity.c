#include "entities_def.c"

void entity_play_animation(struct entity* entity, string name) {
    if (string_equal(entity->animation.name, name)) {
        return;
    }

    entity->animation.name                = name;
    entity->animation.current_frame_index = 0;
    entity->animation.iterations          = 0;
    entity->animation.timer               = 0;
}

struct rectangle_f32 entity_rectangle_collision_bounds(struct entity* entity) {
    return rectangle_f32(entity->position.x,
                         entity->position.y,
                         entity->scale.x,
                         entity->scale.y-10);
}

struct entity_list entity_list_create(struct memory_arena* arena, s32 capacity) {
    struct entity_list result = {
        .entities = memory_arena_push(arena, capacity * sizeof(*result.entities)),
        .generation_count = memory_arena_push(arena, capacity * sizeof(*result.generation_count)),
        .capacity = capacity,
    };

    zero_memory(result.generation_count, result.capacity * sizeof(*result.generation_count));
    return result;
}

entity_id entity_list_create_entity(struct entity_list* entities) {
    for (s32 index = 0; index < entities->capacity; ++index) {
        struct entity* current_entity = entities->entities + index;

        if (!(current_entity->flags & ENTITY_FLAGS_ACTIVE)) {
            current_entity->flags |= ENTITY_FLAGS_ACTIVE;
            {
                struct entity_stat_block* stats = &current_entity->stat_block;
                stats->level        = 1;
                stats->vigor        = 100;
                stats->strength     = 100;
                stats->agility      = 100;
                stats->speed        = 100;
                stats->intelligence = 100;
                stats->luck         = 100;
                stats->experience   = 0;
                stats->xp_value     = 0;
            }
            entities->generation_count[index] += 1;

            return (entity_id){
                .generation = entities->generation_count[index],
                .index      = index+1,
            };
        }
    }

    return (entity_id){};
}

entity_id entity_list_get_id(struct entity_list* entities, s32 index) {
    entity_id result;
    result.index      = index+1;
    result.generation = entities->generation_count[index];
    return result;
}

static struct entity _entity_sentinel = {};
struct entity* entity_list_dereference_entity(struct entity_list* entities, entity_id id) {
    if (id.index <= 0 || id.index > entities->capacity) {
        return &_entity_sentinel;
    }

    struct entity* target = entities->entities + (id.index-1);

    if (entities->generation_count[id.index-1] != id.generation ||
        !(target->flags & ENTITY_FLAGS_ACTIVE)) {
        return &_entity_sentinel;
    }

    return target;
}

/* requires tilemap world... */
/* TODO fix implicit decls, linker hasn't killed game yet */
void player_handle_radial_interactables(struct game_state* state, struct entity_list* entities, s32 entity_index, f32 dt);
#define DEFAULT_VELOCITY (TILE_UNIT_SIZE * 6)

void entity_handle_player_controlled(struct game_state* state, struct entity_list* entities, s32 entity_index, f32 dt) {
    struct entity* entity = entities->entities + entity_index;


    /* all the input blockers. */
    {
        if (global_popup_state.message_count > 0) {
            return;
        }

        /* combat has it's own special movement rules. */
        if (state->combat_state.active_combat) {
            return;
        }

        if (region_zone_animation_block_input) {
            return;
        }
        if (storyboard_active) {
            return;
        }
        /* conversations should be in it's own "module" like region_change_presentation.c */
        if (state->is_conversation_active) {
            return;
        }
    }

    bool move_up    = is_key_down(KEY_UP);
    bool move_down  = is_key_down(KEY_DOWN);
    bool move_left  = is_key_down(KEY_LEFT);
    bool move_right = is_key_down(KEY_RIGHT);

    entity->velocity.x = 0;
    entity->velocity.y = 0;

    {
        if (move_left || move_right) {
            if (move_left) {
                entity->facing_direction = DIRECTION_LEFT;
            } else {
                entity->facing_direction = DIRECTION_RIGHT;
            }
        }

        if (move_up || move_down) {
            if (move_up) {
                entity->facing_direction = DIRECTION_UP;
            } else {
                entity->facing_direction = DIRECTION_DOWN;
            }
        }
    }

    /* NOTE: should be normalized. This isn't right */
    if (move_up)    entity->velocity.y  = -DEFAULT_VELOCITY;
    if (move_down)  entity->velocity.y  = DEFAULT_VELOCITY;
    if (move_left)  entity->velocity.x  = -DEFAULT_VELOCITY;
    if (move_right) entity->velocity.x  = DEFAULT_VELOCITY;

    /* like chests or something */
    player_handle_radial_interactables(state, entities, entity_index, dt);
}

bool entity_push_out_horizontal_edges(struct entity* entity, struct rectangle_f32 collidant) {
    struct rectangle_f32 entity_bounds = entity_rectangle_collision_bounds(entity);

    f32 entity_left_edge   = entity_bounds.x;
    f32 entity_right_edge  = entity_bounds.x + entity_bounds.w;
    f32 entity_top_edge    = entity_bounds.y;
    f32 entity_bottom_edge = entity_bounds.y + entity_bounds.h;

    f32 collidant_left_edge   = collidant.x;
    f32 collidant_top_edge    = collidant.y;
    f32 collidant_bottom_edge = collidant.y + collidant.h;
    f32 collidant_right_edge  = collidant.x + collidant.w;

    if (rectangle_f32_intersect(entity_bounds, collidant)) {
        if (entity_right_edge > collidant_right_edge) {
            entity->position.x = collidant_right_edge;
        } else if (entity_right_edge > collidant_left_edge) {
            entity->position.x = collidant_left_edge - entity_bounds.w;
        }

        return true;
    }

    return false;
}

bool entity_push_out_vertical_edges(struct entity* entity, struct rectangle_f32 collidant) {
    struct rectangle_f32 entity_bounds = entity_rectangle_collision_bounds(entity);

    f32 entity_left_edge   = entity_bounds.x;
    f32 entity_right_edge  = entity_bounds.x + entity_bounds.w;
    f32 entity_top_edge    = entity_bounds.y;
    f32 entity_bottom_edge = entity_bounds.y + entity_bounds.h;

    f32 collidant_left_edge   = collidant.x;
    f32 collidant_top_edge    = collidant.y;
    f32 collidant_bottom_edge = collidant.y + collidant.h;
    f32 collidant_right_edge  = collidant.x + collidant.w;

    if (rectangle_f32_intersect(entity_bounds, collidant)) {
        if (entity_bottom_edge > collidant_top_edge && entity_bottom_edge < collidant_bottom_edge) {
            entity->position.y = collidant_top_edge - entity_bounds.h;
        } else if (entity_top_edge < collidant_bottom_edge && entity_bottom_edge > collidant_bottom_edge) {
            entity->position.y = collidant_bottom_edge;
        }

        return true;
    }

    return false;
}

local void entity_update_and_perform_actions(struct game_state* state, struct entity_list* entities, s32 index, struct level_area* area, f32 dt);
void entity_list_update_entities(struct game_state* state, struct entity_list* entities, f32 dt, struct level_area* area) {
/* void entity_list_update_entities(struct entity_list* entities, f32 dt, s32* tilemap, s32 w, s32 h) { */
    for (s32 index = 0; index < entities->capacity; ++index) {
        struct entity* current_entity = entities->entities + index;

        if (!(current_entity->flags & ENTITY_FLAGS_ACTIVE)) {
            continue;
        }

        {
            if (!(current_entity->flags & ENTITY_FLAGS_NOCLIP)) {
                /* _debugprintf("cx: %f, %f\n", current_entity->velocity.x, current_entity->velocity.y); */
                /* tile intersection */
                {
                    bool stop_horizontal_movement = false;

                    {
                        current_entity->position.x += current_entity->velocity.x * dt;

                        for (s32 index = 0; index < area->tile_counts[TILE_LAYER_OBJECT] && !stop_horizontal_movement; ++index) {
                            struct tile* current_tile = area->tile_layers[TILE_LAYER_OBJECT] + index;
                            struct tile_data_definition* tile_data = tile_table_data + current_tile->id;

                            if (Get_Bit(tile_data->flags, TILE_DATA_FLAGS_SOLID)) {
                                stop_horizontal_movement |=
                                    entity_push_out_horizontal_edges(current_entity, rectangle_f32(current_tile->x * TILE_UNIT_SIZE, current_tile->y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE));
                            }
                        }

                        for (s32 index = 0; index < area->tile_counts[TILE_LAYER_GROUND] && !stop_horizontal_movement; ++index) {
                            struct tile* current_tile = area->tile_layers[TILE_LAYER_GROUND] + index;
                            struct tile_data_definition* tile_data = tile_table_data + current_tile->id;

                            if (Get_Bit(tile_data->flags, TILE_DATA_FLAGS_SOLID)) {
                                stop_horizontal_movement |=
                                    entity_push_out_horizontal_edges(current_entity, rectangle_f32(current_tile->x * TILE_UNIT_SIZE, current_tile->y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE));
                            }
                        }

                        for (s32 index = 0; index < area->entity_chest_count && !stop_horizontal_movement; ++index) {
                            struct entity_chest* chest = area->chests + index;

                            stop_horizontal_movement |=
                                entity_push_out_horizontal_edges(current_entity, rectangle_f32(chest->position.x * TILE_UNIT_SIZE, chest->position.y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE));
                        }
                    }

                    if (stop_horizontal_movement) current_entity->velocity.x = 0;

                    current_entity->position.y += current_entity->velocity.y * dt;
                    {
                        bool stop_vertical_movement = false;

                        for (s32 index = 0; index < area->tile_counts[TILE_LAYER_OBJECT] && !stop_vertical_movement; ++index) {
                            struct tile* current_tile = area->tile_layers[TILE_LAYER_OBJECT] + index;
                            struct tile_data_definition* tile_data = tile_table_data + current_tile->id;

                            if (Get_Bit(tile_data->flags, TILE_DATA_FLAGS_SOLID)) {
                                stop_vertical_movement |=
                                    entity_push_out_vertical_edges(current_entity, rectangle_f32(current_tile->x * TILE_UNIT_SIZE, current_tile->y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE));
                            }
                        }

                        for (s32 index = 0; index < area->tile_counts[TILE_LAYER_GROUND] && !stop_vertical_movement; ++index) {
                            struct tile* current_tile = area->tile_layers[TILE_LAYER_GROUND] + index;
                            struct tile_data_definition* tile_data = tile_table_data + current_tile->id;

                            if (Get_Bit(tile_data->flags, TILE_DATA_FLAGS_SOLID)) {
                                stop_vertical_movement |=
                                    entity_push_out_vertical_edges(current_entity, rectangle_f32(current_tile->x * TILE_UNIT_SIZE, current_tile->y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE));
                            }
                        }

                        for (s32 index = 0; index < area->entity_chest_count && !stop_vertical_movement; ++index) {
                            struct entity_chest* chest = area->chests + index;

                            stop_vertical_movement |=
                                entity_push_out_vertical_edges(current_entity, rectangle_f32(chest->position.x * TILE_UNIT_SIZE, chest->position.y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE));
                        }

                        if (stop_vertical_movement) current_entity->velocity.y = 0;
                    }
                }


                /* any existing actions or action queues will ALWAYS override manual control */
                entity_update_and_perform_actions(state, entities, index, area, dt);

                if (!current_entity->ai.current_action) {
                    /* controller actions, for AI brains. */
                    if (current_entity->flags & ENTITY_FLAGS_PLAYER_CONTROLLED) {
                        entity_handle_player_controlled(state, entities, index, dt);
                    }
                }

                /* handle trigger interactions */
                /* NPCs should not be able to leave areas for now */
                handle_entity_level_trigger_interactions(state, current_entity, area->trigger_level_transition_count, area->trigger_level_transitions, dt);
                handle_entity_scriptable_trigger_interactions(state, current_entity, area->script_trigger_count, area->script_triggers, dt);
            } else {
                current_entity->position.x += current_entity->velocity.x * dt;
                current_entity->position.y += current_entity->velocity.y * dt;
            }

            /* implicit animation state setting for now. */
            if (current_entity->velocity.x != 0 || current_entity->velocity.y != 0) {
                if (current_entity->velocity.y < 0) {
                    entity_play_animation(current_entity, string_literal("up_walk"));
                } else if (current_entity->velocity.y > 0) {
                    entity_play_animation(current_entity, string_literal("down_walk"));
                } else if (current_entity->velocity.x > 0) {
                    entity_play_animation(current_entity, string_literal("right_walk"));
                } else if (current_entity->velocity.x < 0) {
                    entity_play_animation(current_entity, string_literal("left_walk"));
                }
            } else {
                entity_play_animation(current_entity, facing_direction_strings_normal[current_entity->facing_direction]);
            }
        }
    }
}

void entity_think_combat_actions(struct entity* entity, struct game_state* state, f32 dt) {
    /* This should only think about submitting actions... Oh well */
    if (entity->flags & ENTITY_FLAGS_PLAYER_CONTROLLED) {
        /* let the UI handle this thing */
    } else {
        entity->ai.wait_timer += dt;
        if (entity->ai.wait_timer >= 1.0) {
            /* TODO technically the action should consider the end of waiting on turn. */
            entity->waiting_on_turn = false;
        }
    }
}

void entity_list_render_entities(struct entity_list* entities, struct graphics_assets* graphics_assets, struct render_commands* commands, f32 dt) {
    for (s32 index = 0; index < entities->capacity; ++index) {
        struct entity* current_entity = entities->entities + index;

        s32 facing_direction = current_entity->facing_direction;
        s32 model_index      = current_entity->model_index;

        if (!(current_entity->flags & ENTITY_FLAGS_ACTIVE)) {
            continue;
        }

        /* struct entity_model* model = &global_entity_models.models[model_index]; */
        struct entity_animation* anim = find_animation_by_name(model_index, current_entity->animation.name);

        if (!anim) {
            _debugprintf("cannot find anim: %.*s", current_entity->animation.name.length, current_entity->animation.name.data);
            continue;
        }

        current_entity->animation.timer += dt;
        if (current_entity->animation.timer >= anim->time_until_next_frame) {
            current_entity->animation.current_frame_index++;
            current_entity->animation.timer = 0;

            if (current_entity->animation.current_frame_index >= anim->frame_count) {
                current_entity->animation.current_frame_index = 0;
                current_entity->animation.iterations         += 1;
            }
        }

        image_id sprite_to_use = anim->sprites[current_entity->animation.current_frame_index];

        /* TODO sprite model anchor NOTE: does not account for model size? */
        render_commands_push_image(commands,
                                   graphics_assets_get_image_by_id(graphics_assets, sprite_to_use),
                                   rectangle_f32(current_entity->position.x,
                                                 current_entity->position.y - (TILE_UNIT_SIZE*1.5),
                                                 TILE_UNIT_SIZE,
                                                 TILE_UNIT_SIZE * 2),
                                   RECTANGLE_F32_NULL, color32f32(1,1,1,1), NO_FLAGS, BLEND_MODE_ALPHA);

#ifndef RELEASE
        struct rectangle_f32 collision_bounds = entity_rectangle_collision_bounds(current_entity);
        
        render_commands_push_quad(commands, collision_bounds, color32u8(255, 0, 0, 64), BLEND_MODE_ALPHA);
#endif
    }
}

struct entity_query_list find_entities_within_radius(struct memory_arena* arena, struct entity_list* list, v2f32 position, f32 radius) {
    struct entity_query_list result = {};

    /* marker pointer */
    result.indices = memory_arena_push(arena, 0);

    f32 radius_sq = radius * radius;

    for (s32 index = 0; index < list->capacity; ++index) {
        struct entity* target = list->entities + index;

        if (!(target->flags & ENTITY_FLAGS_ACTIVE))
            continue;

        f32 entity_distance_sq = v2f32_distance_sq(position, target->position);

        if (entity_distance_sq <= radius_sq) {
            memory_arena_push(arena, sizeof(s32));
            result.indices[result.count++] = index;
        }
    }

    return result;
}

void entity_inventory_add(struct entity_inventory* inventory, s32 limits, item_id item) {
    struct item_def* item_base = item_database_find_by_id(item);

    if (!item_base)
        return;

    for (unsigned index = 0; index < inventory->count; ++index) {
        if (inventory->items[index].item.id_hash == item.id_hash) {
            if (inventory->items[index].count < item_base->max_stack_value) {
                inventory->items[index].count++;
                return;
            }
        }
    }

    if (inventory->count < limits) {
        inventory->items[inventory->count].item     = item;
        inventory->items[inventory->count++].count += 1;
    }
}

void entity_inventory_add_multiple(struct entity_inventory* inventory, s32 limits, item_id item, s32 count) {
    for (s32 time = 0; time < count; ++time) {
        entity_inventory_add(inventory, limits, item);
    }
}

void entity_inventory_remove_item(struct entity_inventory* inventory, s32 item_index, bool remove_all) {
    struct item_instance* item = inventory->items + item_index;
    item->count -= 1;

    if (item->count <= 0 || remove_all) {
        inventory->items[item_index] = inventory->items[--inventory->count];
    }
}

bool entity_inventory_has_item(struct entity_inventory* inventory, item_id item) {
    for (unsigned index = 0; index < inventory->count; ++index) {
        struct item_instance* current_item = &(inventory->items[index]);

        if (current_item->item.id_hash == item.id_hash) {
            return true;
        }
    }

    return false;
}

/* from inventory */
void item_apply_to_entity(struct item_def* item, struct entity* target) {
    /* NOTE: obviously we need more stuff here */
    switch (item->type) {
        case ITEM_TYPE_CONSUMABLE_ITEM: {
            if (item->health_restoration_value != 0) {
                target->health.value += item->health_restoration_value;
                _debugprintf("restored: %d health", item->health_restoration_value);
            }
        } break;
    }
}

void entity_inventory_use_item(struct entity_inventory* inventory, s32 item_index, struct entity* target) {
    struct item_instance* current_item = &inventory->items[item_index];
    struct item_def* item_base         = item_database_find_by_id(current_item->item);

    if (item_base->type == ITEM_TYPE_CONSUMABLE_ITEM) {
        current_item->count -= 1;

        if (current_item->count <= 0) {
            inventory->items[item_index] = inventory->items[--inventory->count];
        }
    }

    /* or region? */
    item_apply_to_entity(item_base, target);
}

local void entity_copy_path_array_into_navigation_data(struct entity* entity, v2f32* path_points, s32 path_count) {
    s32 minimum_count = 0;

    if (path_count < array_count(entity->ai.navigation_path.path_points)) {
        minimum_count = path_count;
    } else {
        array_count(entity->ai.navigation_path.path_points);
    }

    entity->ai.navigation_path.count = minimum_count;
    for (s32 index = 0; index < minimum_count; ++index) {
        entity->ai.navigation_path.path_points[index] = path_points[index];
    }
}

void entity_combat_submit_movement_action(struct entity* entity, v2f32* path_points, s32 path_count) {
    if (entity->ai.current_action != ENTITY_ACTION_NONE)
        return;
    
    entity_copy_path_array_into_navigation_data(entity, path_points, path_count);
    entity->ai.following_path = true;
    entity->ai.current_action  = ENTITY_ACTION_MOVEMENT;
}

void entity_combat_submit_attack_action(struct entity* entity, s32 attack_index) {
    if (entity->ai.current_action != ENTITY_ACTION_NONE)
        return;

    entity->ai.current_action      = ENTITY_ACTION_ATTACK;
    entity->ai.attack_target_index = attack_index;
    entity->waiting_on_turn     = 0;
    _debugprintf("attacku");
}

bool entity_validate_death(struct entity* entity) {
    if (entity->health.value <= 0) {
        entity->flags &= ~ENTITY_FLAGS_ALIVE;
        return true;
    }

    return false;
}

void entity_do_physical_hurt(struct entity* entity, s32 damage) {
    /* maybe do a funny animation */
    entity->health.value -= damage;

    notify_damage(entity->position, damage);
    (entity_validate_death(entity));
}

/* NOTE: does not really do turns. */
/* set the entity->waiting_on_turn flag to 0 to finish their turn in the combat system. Not done right now cause I need all the actions. */

/* TODO: slightly broken. */
local s32 find_best_direction_index(v2f32 direction) {
    /* truncate direction here to account for lack of 8 directions */
    _debugprintf("original dir: <%f, %f>", direction.x, direction.y);
    {
        {
            f32 sign = sign_f32(direction.x);
            direction.x = fabs(direction.x);
            direction.x += 0.5;
            direction.x = (s32)direction.x * sign;
        }
        {
            f32 sign = sign_f32(direction.y);
            direction.y = fabs(direction.y);
            direction.y += 0.5;
            direction.y = (s32)direction.y * sign;
        }
    }

    local v2f32 direction_vectors[] = {
        [DIRECTION_DOWN]  = v2f32(0,1),
        [DIRECTION_UP]    = v2f32(0,-1),
        [DIRECTION_LEFT]  = v2f32(-1,0),
        [DIRECTION_RIGHT] = v2f32(1,0),
    };

    f32 best_distance = v2f32_distance_sq(direction, direction_vectors[0]);
    s32 best_index    = 0;

    for (s32 index = 1; index < array_count(direction_vectors); ++index) {
        f32 distance_sq = v2f32_distance_sq(direction, direction_vectors[index]);

        _debugprintf("best dir: <%f, %f>, v D<%f, %f>", direction.x, direction.y, direction_vectors[index].x, direction_vectors[index].y);

        if (distance_sq <= best_distance) {
            best_distance = distance_sq;
            best_index    = index;
        }
    }

    return best_index;
}

local void entity_update_and_perform_actions(struct game_state* state, struct entity_list* entities, s32 index, struct level_area* area, f32 dt) {
    struct entity* target_entity = entities->entities + index;

    const f32 CLOSE_ENOUGH_EPISILON = 0.1556;

    switch (target_entity->ai.current_action) {
        case ENTITY_ACTION_NONE: {
            target_entity->velocity.x                  = 0;
            target_entity->velocity.y                  = 0;
            target_entity->ai.following_path           = false;
        } break;

        case ENTITY_ACTION_MOVEMENT: {

            if (target_entity->ai.current_path_point_index >= target_entity->ai.navigation_path.count) {
                target_entity->ai.current_action = 0;

                target_entity->ai.following_path = false;

                target_entity->position.x = target_entity->ai.navigation_path.path_points[target_entity->ai.navigation_path.count-1].x * TILE_UNIT_SIZE;
                target_entity->position.y = target_entity->ai.navigation_path.path_points[target_entity->ai.navigation_path.count-1].y * TILE_UNIT_SIZE;

                target_entity->ai.current_path_point_index = 0;
                target_entity->ai.navigation_path.count = 0;
            } else {
                v2f32 point              = target_entity->ai.navigation_path.path_points[target_entity->ai.current_path_point_index];
                v2f32 tile_position      = target_entity->position;

                tile_position.x /= TILE_UNIT_SIZE;
                tile_position.y /= TILE_UNIT_SIZE;

                v2f32 displacement_to_point = v2f32_sub(point, tile_position);
                v2f32 direction_to_point    = v2f32_normalize(displacement_to_point); 


                bool already_at_next_point = false;
                
                {
                    if (v2f32_magnitude_sq(displacement_to_point) == 0.0f) {
                        already_at_next_point = true;
                    }

                    if (v2f32_distance(point, tile_position) <= (CLOSE_ENOUGH_EPISILON)) {
                        already_at_next_point = true;
                    }
                }

                if (!already_at_next_point) {
                    direction_to_point.x *= DEFAULT_VELOCITY;
                    direction_to_point.y *= DEFAULT_VELOCITY;

                    target_entity->velocity = direction_to_point;
                } else {
                    target_entity->position.x = target_entity->ai.navigation_path.path_points[target_entity->ai.current_path_point_index].x * TILE_UNIT_SIZE;
                    target_entity->position.y = target_entity->ai.navigation_path.path_points[target_entity->ai.current_path_point_index].y * TILE_UNIT_SIZE;

                    target_entity->ai.current_path_point_index++;

                    if (target_entity->ai.current_path_point_index < target_entity->ai.navigation_path.count) {
                        v2f32 point              = target_entity->ai.navigation_path.path_points[target_entity->ai.current_path_point_index];
                        v2f32 tile_position      = target_entity->position;

                        tile_position.x /= TILE_UNIT_SIZE;
                        tile_position.y /= TILE_UNIT_SIZE;

                        v2f32 displacement_to_point = v2f32_sub(point, tile_position);
                        v2f32 direction_to_point    = v2f32_normalize(displacement_to_point); 
                        target_entity->facing_direction    = find_best_direction_index(direction_to_point);
                    }

                    target_entity->velocity = v2f32(0,0);
                }
            }
        } break;

        case ENTITY_ACTION_ATTACK: {
            target_entity->ai.wait_timer += dt;

            if (target_entity->ai.wait_timer >= 1.0) {
                target_entity->ai.current_action = 0;
                /*
                  NOTE: Need to handle attack animations? Damn.
                  Lots of hardcoding I forsee.
                  
for now just do a fixed amount of damage
                 */
                  
                struct entity* attacked_entity = entities->entities + target_entity->ai.attack_target_index;
                entity_do_physical_hurt(attacked_entity, 9999);
            }
        } break;
    }
}

bool is_entity_aggressive_to_player(struct entity* entity) {
    if (!(entity->flags & ENTITY_FLAGS_ALIVE))
        return false;

    if (entity->ai.flags & ENTITY_AI_FLAGS_AGGRESSIVE_TO_PLAYER) {
        return true;
    }


    return false;
}
