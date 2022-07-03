#include "entities_def.c"

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
            entities->generation_count[index] += 1;

            return (entity_id){
                .generation = entities->generation_count[index],
                .index      = index+1,
            };
        }
    }

    return (entity_id){0,0};
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
void entity_handle_player_controlled(struct entity_list* entities, s32 entity_index, f32 dt) {
    struct entity* entity = entities->entities + entity_index;

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
}

void entity_list_update_entities(struct entity_list* entities, f32 dt, struct level_area* area) {
/* void entity_list_update_entities(struct entity_list* entities, f32 dt, s32* tilemap, s32 w, s32 h) { */
    for (s32 index = 0; index < entities->capacity; ++index) {
        struct entity* current_entity = entities->entities + index;

        if (!(current_entity->flags & ENTITY_FLAGS_ACTIVE)) {
            continue;
        }

        if (current_entity->flags & ENTITY_FLAGS_PLAYER_CONTROLLED) {
            entity_handle_player_controlled(entities, index, dt);
        }

        if (!(current_entity->flags & ENTITY_FLAGS_NOCLIP)) {
            /* tile intersection */
            {
                current_entity->position.x += current_entity->velocity.x * dt;
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
                            if (entity_right_edge > tile_right_edge) {
                                current_entity->position.x = tile_right_edge;
                            } else if (entity_right_edge > tile_left_edge) {
                                current_entity->position.x = tile_left_edge - current_entity->scale.x;
                            }
                            current_entity->velocity.x = 0;
                        }
                    }
                }

                current_entity->position.y += current_entity->velocity.y * dt;
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
                            current_entity->velocity.y = 0;
                        }
                    }
                }
            }
        } else {
            current_entity->position.x += current_entity->velocity.x * dt;
            current_entity->position.y += current_entity->velocity.y * dt;
        }
    }
}

void entity_list_render_entities(struct entity_list* entities, struct graphics_assets* graphics_assets, struct render_commands* commands, f32 dt) {
    for (s32 index = 0; index < entities->capacity; ++index) {
        struct entity* current_entity = entities->entities + index;

        if (!(current_entity->flags & ENTITY_FLAGS_ACTIVE)) {
            continue;
        }

        render_commands_push_image(commands,
                                   graphics_assets_get_image_by_id(graphics_assets, guy_img),
                                   rectangle_f32(current_entity->position.x,
                                                 current_entity->position.y,
                                                 current_entity->scale.x,
                                                 current_entity->scale.y),
                                   RECTANGLE_F32_NULL, color32f32(1,1,1,1), NO_FLAGS, BLEND_MODE_ALPHA);
    }
}
