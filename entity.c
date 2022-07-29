#include "entities_def.c"

struct rectangle_f32 entity_rectangle_collision_bounds(struct entity* entity) {
    return rectangle_f32(entity->position.x,
                         entity->position.y,
                         entity->scale.x,
                         entity->scale.y);
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

    return (entity_id){0,0};
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
void entity_handle_player_controlled(struct game_state* state, struct entity_list* entities, s32 entity_index, f32 dt) {
    struct entity* entity = entities->entities + entity_index;


    /* all the input blockers. */
    {
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

    if (move_up)    entity->velocity.y  = -100;
    if (move_down)  entity->velocity.y  = 100;
    if (move_left)  entity->velocity.x  = -100;
    if (move_right) entity->velocity.x  = 100;

    player_handle_radial_interactables(state, entities, entity_index, dt);
}

void entity_list_update_entities(struct game_state* state, struct entity_list* entities, f32 dt, struct level_area* area) {
/* void entity_list_update_entities(struct entity_list* entities, f32 dt, s32* tilemap, s32 w, s32 h) { */
    for (s32 index = 0; index < entities->capacity; ++index) {
        struct entity* current_entity = entities->entities + index;

        if (!(current_entity->flags & ENTITY_FLAGS_ACTIVE)) {
            continue;
        }

        if (current_entity->flags & ENTITY_FLAGS_PLAYER_CONTROLLED) {
            entity_handle_player_controlled(state, entities, index, dt);
        }

        if (!(current_entity->flags & ENTITY_FLAGS_NOCLIP)) {
            /* tile intersection */
            {
                current_entity->position.x += current_entity->velocity.x * dt;
                {
                    bool stop_horizontal_movement = false;

                    if (!stop_horizontal_movement) {
                        for (s32 index = 0; index < area->tile_count; ++index) {
                            struct tile* current_tile = area->tiles + index;
                            struct tile_data_definition* tile_data = tile_table_data + current_tile->id;

                            if (Get_Bit(tile_data->flags, TILE_DATA_FLAGS_SOLID)) {
                                f32 tile_right_edge  = (current_tile->x + 1) * TILE_UNIT_SIZE;
                                f32 tile_left_edge   = (current_tile->x) * TILE_UNIT_SIZE;
                                f32 tile_top_edge    = (current_tile->y) * TILE_UNIT_SIZE;
                                f32 tile_bottom_edge = (current_tile->y + 1) * TILE_UNIT_SIZE;

                                f32 entity_left_edge   = current_entity->position.x;
                                f32 entity_right_edge  = current_entity->position.x + current_entity->scale.x;
                                f32 entity_top_edge    = current_entity->position.y;
                                f32 entity_bottom_edge = current_entity->position.y + current_entity->scale.y;

                                /* x */
                                if (rectangle_f32_intersect(entity_rectangle_collision_bounds(current_entity),
                                                            rectangle_f32(tile_left_edge, tile_top_edge, tile_right_edge - tile_left_edge, tile_bottom_edge - tile_top_edge)))
                                {
                                    if (entity_right_edge > tile_right_edge) {
                                        current_entity->position.x = tile_right_edge;
                                    } else if (entity_right_edge > tile_left_edge) {
                                        current_entity->position.x = tile_left_edge - current_entity->scale.x;
                                    }

                                    stop_horizontal_movement = true;
                                }
                            }
                        }
                    }

                    if (!stop_horizontal_movement) {
                        for (s32 index = 0; index < area->entity_chest_count; ++index) {
                            struct entity_chest* chest = area->chests + index;

                            f32 chest_right_edge  = (chest->position.x + 1) * TILE_UNIT_SIZE;
                            f32 chest_left_edge   = (chest->position.x)     * TILE_UNIT_SIZE;
                            f32 chest_bottom_edge = (chest->position.y + 1) * TILE_UNIT_SIZE;
                            f32 chest_top_edge    = (chest->position.y)     * TILE_UNIT_SIZE;

                            f32 entity_left_edge   = current_entity->position.x;
                            f32 entity_right_edge  = current_entity->position.x + current_entity->scale.x;
                            f32 entity_top_edge    = current_entity->position.y;
                            f32 entity_bottom_edge = current_entity->position.y + current_entity->scale.y;

                            if (rectangle_f32_intersect(entity_rectangle_collision_bounds(current_entity),
                                                        rectangle_f32(chest_left_edge, chest_top_edge, chest_right_edge - chest_left_edge, chest_bottom_edge - chest_top_edge))) {
                                if (entity_right_edge > chest_right_edge) {
                                    current_entity->position.x = chest_right_edge;
                                } else if (entity_right_edge > chest_left_edge) {
                                    current_entity->position.x = chest_left_edge - current_entity->scale.x;
                                }

                                stop_horizontal_movement = true;
                            }
                        }
                    }

                    if (stop_horizontal_movement) current_entity->velocity.x = 0;
                }

                current_entity->position.y += current_entity->velocity.y * dt;
                {
                    bool stop_vertical_movement = false;

                    if (!stop_vertical_movement) {
                        for (s32 index = 0; index < area->tile_count; ++index) {
                            struct tile* current_tile = area->tiles + index;
                            struct tile_data_definition* tile_data = tile_table_data + current_tile->id;

                            if (Get_Bit(tile_data->flags, TILE_DATA_FLAGS_SOLID)) {
                                f32 tile_right_edge  = (current_tile->x + 1) * TILE_UNIT_SIZE;
                                f32 tile_left_edge   = (current_tile->x) * TILE_UNIT_SIZE;
                                f32 tile_top_edge    = (current_tile->y) * TILE_UNIT_SIZE;
                                f32 tile_bottom_edge = (current_tile->y + 1) * TILE_UNIT_SIZE;

                                f32 entity_left_edge   = current_entity->position.x;
                                f32 entity_right_edge  = current_entity->position.x + current_entity->scale.x;
                                f32 entity_top_edge    = current_entity->position.y;
                                f32 entity_bottom_edge = current_entity->position.y + current_entity->scale.y;

                                /* x */
                                if (rectangle_f32_intersect(rectangle_f32(current_entity->position.x, current_entity->position.y, current_entity->scale.x, current_entity->scale.y),
                                                            rectangle_f32(tile_left_edge, tile_top_edge, tile_right_edge - tile_left_edge, tile_bottom_edge - tile_top_edge)))
                                {
                                    if (entity_bottom_edge > tile_top_edge && entity_bottom_edge < tile_bottom_edge) {
                                        current_entity->position.y = tile_top_edge - current_entity->scale.y;
                                    } else if (entity_top_edge < tile_bottom_edge && entity_bottom_edge > tile_bottom_edge) {
                                        current_entity->position.y = tile_bottom_edge;
                                    }

                                    stop_vertical_movement = true;
                                }
                            }
                        }
                    }

                    if (!stop_vertical_movement) {
                        for (s32 index = 0; index < area->entity_chest_count; ++index) {
                            struct entity_chest* chest = area->chests + index;

                            f32 chest_right_edge  = (chest->position.x + 1) * TILE_UNIT_SIZE;
                            f32 chest_left_edge   = (chest->position.x)     * TILE_UNIT_SIZE;
                            f32 chest_bottom_edge = (chest->position.y + 1) * TILE_UNIT_SIZE;
                            f32 chest_top_edge    = (chest->position.y)     * TILE_UNIT_SIZE;

                            f32 entity_left_edge   = current_entity->position.x;
                            f32 entity_right_edge  = current_entity->position.x + current_entity->scale.x;
                            f32 entity_top_edge    = current_entity->position.y;
                            f32 entity_bottom_edge = current_entity->position.y + current_entity->scale.y;

                            if (rectangle_f32_intersect(entity_rectangle_collision_bounds(current_entity),
                                                        rectangle_f32(chest_left_edge, chest_top_edge,
                                                                      chest_right_edge - chest_left_edge, chest_bottom_edge - chest_top_edge))) {
                                if (entity_bottom_edge > chest_top_edge && entity_bottom_edge < chest_bottom_edge) {
                                    current_entity->position.y = chest_top_edge - current_entity->scale.y;
                                } else if (entity_top_edge < chest_bottom_edge && entity_bottom_edge > chest_bottom_edge) {
                                    current_entity->position.y = chest_bottom_edge;
                                }

                                stop_vertical_movement = true;
                            }
                        }
                    }

                    if (stop_vertical_movement) current_entity->velocity.y = 0;
                }
            }

            /* handle trigger interactions */
            /* NPCs should not be able to leave areas for now */
            handle_entity_level_trigger_interactions(state, current_entity, area->trigger_level_transition_count, area->trigger_level_transitions, dt);
        } else {
            current_entity->position.x += current_entity->velocity.x * dt;
            current_entity->position.y += current_entity->velocity.y * dt;
        }
    }
}

void entity_think_combat_actions(struct entity* entity, struct game_state* state, f32 dt) {
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

        if (!(current_entity->flags & ENTITY_FLAGS_ACTIVE)) {
            continue;
        }

        /* TODO sprite model anchor */
        render_commands_push_image(commands,
                                   graphics_assets_get_image_by_id(graphics_assets, guy_img),
                                   rectangle_f32(current_entity->position.x,
                                                 current_entity->position.y - (TILE_UNIT_SIZE),
                                                 TILE_UNIT_SIZE,
                                                 TILE_UNIT_SIZE * 2),
                                   RECTANGLE_F32_NULL, color32f32(1,1,1,1), NO_FLAGS, BLEND_MODE_ALPHA);

#ifndef RELEASE
        render_commands_push_quad(commands, rectangle_f32(current_entity->position.x, current_entity->position.y, current_entity->scale.x, current_entity->scale.y), color32u8(255, 0, 0, 64), BLEND_MODE_ALPHA);
#endif
    }
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

    current_item->count -= 1;

    if (current_item->count <= 0) {
        inventory->items[item_index] = inventory->items[--inventory->count];
    }

    /* or region? */
    item_apply_to_entity(item_base, target);
}
