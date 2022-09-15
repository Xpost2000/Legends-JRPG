#include "entities_def.c"
/*
 * CLEANUP/TODO: As models are now the source of truth for entity collision sizes,
 * and thus arbitrary scales are not needed (entities can just be represented as points with an implied rectangle),
 * I can remove those later.
 */
bool entity_bad_ref(struct entity* e);

void _debug_print_id(entity_id id) {
    _debugprintf("ent id[g:%d]: %d, %d", id.generation, id.store_type, id.index);
}
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
    f32 model_width_units = entity_model_get_width_units(entity->model_index);

    return rectangle_f32(entity->position.x+2.75, /* collision oddities, this removes sticking */
                         entity->position.y,
                         entity->scale.x * model_width_units,
                         entity->scale.y-10);
}

struct entity_list entity_list_create(struct memory_arena* arena, s32 capacity, u8 store_mark) {
    struct entity_list result = {
        .entities         = memory_arena_push(arena, capacity * sizeof(*result.entities)),
        .generation_count = memory_arena_push(arena, capacity * sizeof(*result.generation_count)),
        .capacity         = capacity,
        .store_type       = store_mark,
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
                *stats = entity_stat_block_identity;
            }
            entities->generation_count[index] += 1;

            return (entity_id){
                .generation = entities->generation_count[index],
                .index      = index+1,
                .store_type = entities->store_type,
            };
        }
    }

    _debugprintf("Alloced new entity;");
    return (entity_id){};
}

entity_id entity_list_get_id(struct entity_list* entities, s32 index) {
    entity_id result;
    result.store_type = entities->store_type;
    result.index      = index+1;
    result.generation = entities->generation_count[index];
    return result;
}

static struct entity _entity_sentinel = {};
bool entity_bad_ref(struct entity* e) {
    if (e == &_entity_sentinel) return true;
    if (e == NULL)              return true;

    return false;
}

struct entity* entity_list_dereference_entity(struct entity_list* entities, entity_id id) {
    /* can't have come from the same place. */
    if (id.store_type != entities->store_type) {
        return &_entity_sentinel;
    }

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

s32 entity_iterator_count_all_entities(struct entity_iterator* entities) {
    s32 result = 0;

    for (s32 entity_list_index = 0; entity_list_index < entities->list_count; ++entity_list_index) {
        result += entities->entity_lists[entity_list_index]->capacity;
    }

    return result;
}

/* requires tilemap world... */
/* TODO fix implicit decls, linker hasn't killed game yet */
void player_handle_radial_interactables(struct game_state* state, struct entity* entity, f32 dt);
#define DEFAULT_VELOCITY (TILE_UNIT_SIZE * 6)

void entity_handle_player_controlled(struct game_state* state, struct entity* entity, f32 dt) {
    /* all the input blockers. */
    {
        if (disable_game_input) {
            return;
        }

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

    struct game_controller* pad0 = get_gamepad(0);

    if (pad0) {
        move_up    |= pad0->buttons[DPAD_UP];
        move_down  |= pad0->buttons[DPAD_DOWN];
        move_left  |= pad0->buttons[DPAD_LEFT];
        move_right |= pad0->buttons[DPAD_RIGHT];
    }

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
    player_handle_radial_interactables(state, entity, dt);
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

local void entity_update_and_perform_actions(struct game_state* state, struct entity* entity, struct level_area* area, f32 dt);

struct entity_iterator game_entity_iterator(struct game_state* state) {
    struct entity_iterator result = {};

    entity_iterator_push(&result, &state->permenant_entities);
    entity_iterator_push(&result, &state->loaded_area.entities);

    return result;
}

void update_entities(struct game_state* state, f32 dt, struct level_area* area) {
    struct entity_iterator it = game_entity_iterator(state);

    for (struct entity* current_entity = entity_iterator_begin(&it); !entity_iterator_finished(&it); current_entity = entity_iterator_advance(&it)) {
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
                entity_update_and_perform_actions(state, current_entity, area, dt);

                if (!current_entity->ai.current_action) {
                    /* controller actions, for AI brains. */
                    if (current_entity->flags & ENTITY_FLAGS_PLAYER_CONTROLLED) {
                        entity_handle_player_controlled(state, current_entity, dt);
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
            if (!(current_entity->flags & ENTITY_FLAGS_ALIVE)) {
                /* I want to add the kneeling animation, and we'll obviously require more intense animation state. Anyways... */    
                entity_play_animation(current_entity, string_literal("dead"));
            } else {
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

/*
  NOTE: In terms of render commands, I don't seem to actually need to use them for sorting as I know most of the
  order and the few things that genuinely need to be sorted can just be done... Where they need to be sorted here.
*/

void render_entities(struct game_state* state, struct graphics_assets* graphics_assets, struct render_commands* commands, f32 dt) {
    struct entity_iterator it = game_entity_iterator(state);

    s32 entity_total_capacities = entity_iterator_count_all_entities(&it);
    entity_id* sorted_entity_ids = memory_arena_push(&scratch_arena, sizeof(*sorted_entity_ids) * entity_total_capacities);
    s32        sorted_entity_id_count = 0;

    {
        for (struct entity* current_entity = entity_iterator_begin(&it); !entity_iterator_finished(&it); current_entity = entity_iterator_advance(&it)) {
            if (!(current_entity->flags & ENTITY_FLAGS_ACTIVE)) {
                continue;
            }

            sorted_entity_ids[sorted_entity_id_count++] = it.current_id;
        }

        for (s32 sorted_entity_index = 1; sorted_entity_index < sorted_entity_id_count; ++sorted_entity_index) {
            entity_id key_id = sorted_entity_ids[sorted_entity_index];
            struct entity* key_entity = game_dereference_entity(state, sorted_entity_ids[sorted_entity_index]);

            s32 sorted_entity_insertion_index = sorted_entity_index;
            for (; sorted_entity_insertion_index > 0; --sorted_entity_insertion_index) {
                struct entity* comparison_entity = game_dereference_entity(state, sorted_entity_ids[sorted_entity_insertion_index-1]);

                if (comparison_entity->position.y < key_entity->position.y) {
                    break;
                } else {
                    sorted_entity_ids[sorted_entity_insertion_index] = sorted_entity_ids[sorted_entity_insertion_index-1];
                }
            }

            sorted_entity_ids[sorted_entity_insertion_index] = key_id;

        }
    }

    for (s32 sorted_entity_index = 0; sorted_entity_index < sorted_entity_id_count; ++sorted_entity_index) {
        struct entity* current_entity = game_dereference_entity(state, sorted_entity_ids[sorted_entity_index]);

        s32 facing_direction = current_entity->facing_direction;
        s32 model_index      = current_entity->model_index;

        if (!(current_entity->flags & ENTITY_FLAGS_ACTIVE)) {
            continue;
        }

        struct entity_animation* anim = find_animation_by_name(model_index, current_entity->animation.name);

        if (!anim) {
            _debugprintf("cannot find anim: %.*s. Falling back to \"down direction\"", current_entity->animation.name.length, current_entity->animation.name.data);
            anim = find_animation_by_name(model_index, string_literal("down"));
            assertion(anim && "no fallback anim? That is bad!");
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

        image_id sprite_to_use  = entity_animation_get_sprite_frame(anim, current_entity->animation.current_frame_index);
        v2f32 sprite_dimensions = entity_animation_get_frame_dimensions(anim, current_entity->animation.current_frame_index);

        /*
          Look no one said this was good, I think my brain is a little broken since I can't think of a proper way to scale up everything properly.

          However all the art is expected to be relative to 16x16 tiles
        */
        f32 scale_x = 2;
        
        v2f32 real_dimensions  = v2f32(sprite_dimensions.x * scale_x, sprite_dimensions.y * scale_x);

        bool should_shift_up = (real_dimensions.y / TILE_UNIT_SIZE) >= 1;

        v2f32 alignment_offset = v2f32(0, real_dimensions.y * should_shift_up * 0.8);

        render_commands_push_image(commands,
                                   graphics_assets_get_image_by_id(graphics_assets, drop_shadow),
                                   rectangle_f32(current_entity->position.x - alignment_offset.x,
                                                 current_entity->position.y - TILE_UNIT_SIZE*0.4,
                                                 TILE_UNIT_SIZE * roundf(real_dimensions.x/TILE_UNIT_SIZE),
                                                 TILE_UNIT_SIZE * max(roundf(real_dimensions.y/(TILE_UNIT_SIZE*2)), 1)),
                                   RECTANGLE_F32_NULL, color32f32(1,1,1,0.72), NO_FLAGS, BLEND_MODE_ALPHA);

        union color32f32 modulation_color = color32f32_WHITE;

        {
            if (is_entity_under_ability_selection(it.current_id)) {
                /* red for now. We want better effects maybe? */
                modulation_color.g = modulation_color.b = 0;
            }
        }

        render_commands_push_image(commands,
                                   graphics_assets_get_image_by_id(graphics_assets, sprite_to_use),
                                   rectangle_f32(current_entity->position.x - alignment_offset.x,
                                                 current_entity->position.y - alignment_offset.y,
                                                 real_dimensions.x,
                                                 real_dimensions.y),
                                   RECTANGLE_F32_NULL, modulation_color, NO_FLAGS, BLEND_MODE_ALPHA);

#ifndef RELEASE
        struct rectangle_f32 collision_bounds = entity_rectangle_collision_bounds(current_entity);
        
        render_commands_push_quad(commands, collision_bounds, color32u8(255, 0, 0, 64), BLEND_MODE_ALPHA);
#endif
    }
}

struct entity_query_list find_entities_within_radius(struct memory_arena* arena, struct game_state* state, v2f32 position, f32 radius) {
    struct entity_query_list result = {};
    struct entity_iterator   it     = game_entity_iterator(state);

    /* marker pointer */
    result.ids = memory_arena_push(arena, 0);

    f32 radius_sq = radius * radius;

    for (struct entity* target = entity_iterator_begin(&it); !entity_iterator_finished(&it); target = entity_iterator_advance(&it)) {
        if (!(target->flags & ENTITY_FLAGS_ACTIVE))
            continue;

        f32 entity_distance_sq = v2f32_distance_sq(position, target->position);

        if (entity_distance_sq <= radius_sq) {
            memory_arena_push(arena, sizeof(*result.ids));
            _debug_print_id(it.current_id);
            result.ids[result.count++] = it.current_id;
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
            bool infinite_item = item_base->max_stack_value == -1;

            if (infinite_item || inventory->items[index].count < item_base->max_stack_value) {
                inventory->items[index].count++;
                return;
            }
        }
    }

    if (inventory->count < limits) {
        inventory->items[inventory->count].item     = item;
        /* just in case I forgot to 0 out? */
        inventory->items[inventory->count].count    = 0;
        inventory->items[inventory->count++].count += 1;
    }
}

void entity_inventory_add_multiple(struct entity_inventory* inventory, s32 limits, item_id item, s32 count) {
    for (s32 time = 0; time < count; ++time) {
        entity_inventory_add(inventory, limits, item);
    }
}

s32 entity_inventory_count_instances_of(struct entity_inventory* inventory, string item_name) {
    s32 counter = 0;

    item_id id_to_match = item_id_make(item_name);
    for (s32 index = 0; index < inventory->count; ++index) {
        if (item_id_equal(id_to_match, inventory->items[index].item)) {
            counter += inventory->items[index].count;
        }
    }

    return counter;
}

s32 entity_inventory_get_gold_count(struct entity_inventory* inventory) {
    return entity_inventory_count_instances_of(inventory, string_literal("item_gold"));
}

void entity_inventory_set_gold_count(struct entity_inventory* inventory, s32 amount) {
    if (!entity_inventory_has_item(inventory, item_id_make(string_literal("item_gold")))) {
        /* there should always be *some* room for gold, so not even going to bother getting limits... */
        entity_inventory_add(inventory, 9999, item_id_make(string_literal("item_gold")));
    } 

    for (s32 index = 0; index < inventory->count; ++index) {
        if (item_id_equal(inventory->items[index].item, item_id_make(string_literal("item_gold")))) {
            inventory->items[index].count = amount;
            break;
        }
    }
}

void entity_inventory_remove_item(struct entity_inventory* inventory, s32 item_index, bool remove_all) {
    struct item_instance* item = inventory->items + item_index;
    item->count -= 1;

    if (item->count <= 0 || remove_all) {
        item->count = 0;
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

/* TODO: maybe inventories should know their own limits. */
void entity_inventory_equip_item(struct entity_inventory* inventory, s32 limits, s32 item_index, s32 equipment_index, struct entity* target) {
    struct item_instance* item_to_equip = inventory->items + item_index;
    struct item_def*      item_base     = item_database_find_by_id(item_to_equip->item);

    _debugprintf("equipping: %.*s", item_base->name.length, item_base->name.data);

    if (item_base->type == ITEM_TYPE_WEAPON || item_base->type == ITEM_TYPE_EQUIPMENT) {

        item_id* equip_slot = target->equip_slots + equipment_index;
        entity_inventory_unequip_item(inventory, limits, equipment_index, target);
        *equip_slot = item_to_equip->item;
        entity_inventory_remove_item(inventory, item_index, false);
    }
}

void entity_inventory_unequip_item(struct entity_inventory* inventory, s32 limits, s32 equipment_index, struct entity* target) {
    if (target->equip_slots[equipment_index].id_hash != 0) {
        _debugprintf("have an item to unequip");
        entity_inventory_add(inventory, limits, target->equip_slots[equipment_index]);
        target->equip_slots[equipment_index] = (item_id) {};
    }
}

void entity_inventory_use_item(struct entity_inventory* inventory, s32 item_index, struct entity* target) {
    struct item_instance* current_item = &inventory->items[item_index];
    struct item_def* item_base         = item_database_find_by_id(current_item->item);

    if (item_base->type == ITEM_TYPE_CONSUMABLE_ITEM) {
        entity_inventory_remove_item(inventory, item_index, false);
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

void entity_combat_submit_attack_action(struct entity* entity, entity_id target_id) {
    if (entity->ai.current_action != ENTITY_ACTION_NONE)
        return;

    entity->ai.current_action   = ENTITY_ACTION_ATTACK;
    entity->ai.attack_target_id = target_id;
    entity->waiting_on_turn     = 0;
    _debugprintf("attacku");
}

bool entity_validate_death(struct entity* entity) {
    if (entity->health.value <= 0) {
        entity->flags &= ~ENTITY_FLAGS_ALIVE;
        return true;
    }

    entity->flags |= ENTITY_FLAGS_ALIVE;
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

local void entity_update_and_perform_actions(struct game_state* state, struct entity* entity, struct level_area* area, f32 dt) {
    struct entity* target_entity = entity;

    const f32 CLOSE_ENOUGH_EPISILON = 0.1896;

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
                  
                /* TODO FIX */
                struct entity* attacked_entity = game_dereference_entity(state, target_entity->ai.attack_target_id);
                entity_do_physical_hurt(attacked_entity, 9999);

                {
                    if (attacked_entity->health.value <= 0) {
                        if (target_entity->flags & ENTITY_FLAGS_PLAYER_CONTROLLED) {
                            /* maybe not a good idea right now, but okay. */
                            battle_notify_killed_entity(target_entity->ai.attack_target_id);
                        }
                    }
                }
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

/* have to figure out how health should work */
s32 entity_find_effective_stat_value(struct entity* entity, s32 stat_index) {
    s32 base_value = entity->stat_block.values[stat_index];

    /* apply multiplicative effects */
    {
        /* from spell effects? */

        /* from items */
        for (s32 equip_index = 0; equip_index < array_count(entity->equip_slots); ++equip_index) {
            item_id          equip_slot_item = entity->equip_slots[equip_index];
            struct item_def* item_base       = item_database_find_by_id(equip_slot_item);

            if (item_base) base_value *= item_base->modifiers.values[stat_index];
        }
    }

    /* apply additive effects */
    {
        for (s32 equip_index = 0; equip_index < array_count(entity->equip_slots); ++equip_index) {
            item_id          equip_slot_item = entity->equip_slots[equip_index];
            struct item_def* item_base       = item_database_find_by_id(equip_slot_item);

            if (item_base) base_value += item_base->stats.values[stat_index];
        }
    }

    return base_value;
}

void entity_database_add_entity(struct entity_database* entity_database, struct entity_base_data base_ent, string as_name) {
    s32 next_index                                  = entity_database->entity_count++;
    entity_database->entities[next_index]           = base_ent;
    entity_database->entity_key_strings[next_index] = string_clone(entity_database->arena, as_name);
}

local void entity_database_initialize_loot_tables(struct entity_database* database) {
    _debugprintf("loading form");
    struct memory_arena* arena            = database->arena;
    struct file_buffer   data_file_buffer = read_entire_file(memory_arena_allocator(&scratch_arena), string_literal("./res/loot_tables.txt"));
    struct lisp_list     data_file_forms  = lisp_read_string_into_forms(&scratch_arena, file_buffer_as_string(&data_file_buffer));

    s32 loot_table_count = data_file_forms.count;

    database->loot_table_capacity    = loot_table_count;
    database->loot_table_count       = loot_table_count;
    database->loot_tables            = memory_arena_push(arena, loot_table_count * sizeof(*database->loot_tables));
    database->loot_table_key_strings = memory_arena_push(arena, loot_table_count * sizeof(*database->loot_table_key_strings));

    for (s32 loot_table_form_index = 0; loot_table_form_index < loot_table_count; ++loot_table_form_index) {
        struct entity_loot_table* current_loot_table            = &database->loot_tables[loot_table_form_index];
        string*                   current_loot_table_key_string = &database->loot_table_key_strings[loot_table_form_index];

        struct lisp_form* form = data_file_forms.forms + loot_table_form_index;

        {
            struct lisp_form* name_form          = lisp_list_nth(form, 0);
            struct lisp_form  loot_table_entries = lisp_list_sliced(*form, 1, -1);

            string name_str = {};
            assertion(lisp_form_get_string(*name_form, &name_str) && "Bad name form for loot table");
            *current_loot_table_key_string = string_clone(arena, name_str);
            
            s32 loot_table_entry_count = loot_table_entries.list.count;
            current_loot_table->loot_count = loot_table_entry_count;

            assertion(current_loot_table->loot_count < ENTITY_MAX_LOOT_TABLE_ENTRIES && "Too much loot!");
            for (s32 loot_table_entry_form_index = 0; loot_table_entry_form_index < loot_table_entry_count; ++loot_table_entry_form_index) {
                struct entity_loot* loot_entry         = current_loot_table->loot_items + loot_table_entry_form_index;
                struct lisp_form*   current_entry_form = lisp_list_nth(&loot_table_entries, loot_table_entry_form_index);

                {
                    struct lisp_form* item_name_form       = lisp_list_nth(current_entry_form, 0);
                    struct lisp_form* drop_count_form      = lisp_list_nth(current_entry_form, 1);
                    struct lisp_form* drop_percentage_form = lisp_list_nth(current_entry_form, 2);

                    item_id new_item_id = item_id_make(string_literal("\0"));
                    assertion(item_name_form && "no item name form?");
                    {
                        string id_str = {};
                        assertion(lisp_form_get_string(*item_name_form, &id_str) && "bad name form?");
                        new_item_id = item_id_make(id_str);
                    }

                    s32 drop_range_min = 0;
                    s32 drop_range_max = 0;
                    {
                        if (drop_count_form) {
                            if (drop_count_form->type == LISP_FORM_LIST) {
                                struct lisp_form* min_form = lisp_list_nth(drop_count_form, 0);
                                struct lisp_form* max_form = lisp_list_nth(drop_count_form, 1);

                                assertion(lisp_form_get_s32(*min_form, &drop_range_min) && "error missing min");
                                assertion(lisp_form_get_s32(*max_form, &drop_range_max) && "error missing max");
                            } else if (drop_count_form->type == LISP_FORM_NUMBER) {
                                assertion(lisp_form_get_s32(*drop_count_form, &drop_range_min) && "bad form?");
                                drop_range_max = drop_range_min;
                            }
                        } else {
                            drop_range_min = drop_range_max = 1;
                        }
                    }

                    f32 drop_percentage = 0;
                    {
                        if (drop_percentage_form) {
                            assertion(lisp_form_get_f32(*drop_percentage_form, &drop_percentage) && "bad form?");
                        } else {
                            drop_percentage = 1;
                        }
                    }

                    loot_entry->item              = new_item_id;
                    loot_entry->count_min         = drop_range_min;
                    loot_entry->count_max         = drop_range_max;
                    loot_entry->normalized_chance = drop_percentage;
                }
            }
        }
    }
}

local void entity_database_initialize_entities(struct entity_database* database) {
    struct memory_arena* arena            = database->arena;
    struct file_buffer   data_file_buffer = read_entire_file(memory_arena_allocator(&scratch_arena), string_literal("./res/entities.txt"));
    struct lisp_list     data_file_forms  = lisp_read_string_into_forms(&scratch_arena, file_buffer_as_string(&data_file_buffer));

    s32 entity_count = data_file_forms.count + 1;

    database->entity_capacity    = entity_count;
    database->entities           = memory_arena_push(arena, entity_count * sizeof(*database->entities));
    database->entity_key_strings = memory_arena_push(arena, entity_count * sizeof(*database->entity_key_strings));

    {
        static struct entity_base_data base_data = {
            .name = string_literal("John Doe"),
            .health = 10,
        };

        base_data.stats = entity_stat_block_identity;
        entity_database_add_entity(database, base_data, string_literal("__default__"));
    }

    database->entity_count = entity_count;

    for (s32 entity_form_index = 0; entity_form_index < data_file_forms.count; ++entity_form_index) {
        struct entity_base_data* current_entity      = (database->entities+1) + entity_form_index;
        struct lisp_form*        current_entity_form = data_file_forms.forms + entity_form_index;

        {
            struct lisp_form* id_name_form          = lisp_list_nth(current_entity_form, 0);
            struct lisp_form* name_form             = lisp_list_nth(current_entity_form, 1);
            struct lisp_form* model_id_form         = lisp_list_nth(current_entity_form, 2);
            struct lisp_form* flags_list_form       = lisp_list_nth(current_entity_form, 3);
            struct lisp_form* ai_flags_list_form    = lisp_list_nth(current_entity_form, 4);
            struct lisp_form* health_form           = lisp_list_nth(current_entity_form, 5);
            struct lisp_form* magic_form            = lisp_list_nth(current_entity_form, 6);
            struct lisp_form* loot_table_id_form    = lisp_list_nth(current_entity_form, 7);
            struct lisp_form* equip_slot_list_form  = lisp_list_nth(current_entity_form, 8); unused(equip_slot_list_form);
            struct lisp_form* entity_inventory_form = lisp_list_nth(current_entity_form, 9); unused(entity_inventory_form);
            struct lisp_form* stat_block_form       = lisp_list_nth(current_entity_form, 10);

            {
                string key_name = {};
                lisp_form_get_string(*id_name_form, &key_name);
                database->entity_key_strings[entity_form_index+1] = string_clone(arena, key_name);
            }
            {
                string name_str = {};
                lisp_form_get_string(*name_form, &name_str);
                current_entity->name = string_clone(arena, name_str);
            }
            {lisp_form_get_s32(*model_id_form, &current_entity->model_index);}
            { /* flag parsing */
                _debugprintf("Flag parsing not done yet!");
            }
            {
                lisp_form_get_s32(*health_form, &current_entity->health);
                lisp_form_get_s32(*magic_form, &current_entity->magic);
            }
            {
                string loot_table_id = {};
                if (loot_table_id_form->type != LISP_FORM_STRING) {
                    current_entity->loot_table_id_index = -1;
                } else {
                    lisp_form_get_string(*loot_table_id_form, &loot_table_id);
                    current_entity->loot_table_id_index = entity_database_loot_table_find_id_by_name(database, loot_table_id);
                }
            }
            { /* equip slot and inventory */
                
            }
            { /* stat block parsing */
                s32 stat_block_form_count = stat_block_form->list.count;
                for (s32 stat_block_index = 0; stat_block_index < stat_block_form_count; ++stat_block_index) {
                    struct lisp_form* stat_form = lisp_list_nth(stat_block_form, stat_block_index);

                    {
                        struct lisp_form* stat_name = lisp_list_nth(stat_form, 0);
                        struct lisp_form* param     = lisp_list_nth(stat_form, 1);

                        if (lisp_form_symbol_matching(*stat_name, string_literal("level"))) {
                            lisp_form_get_s32(*param, &current_entity->stats.level);
                        } else if (lisp_form_symbol_matching(*stat_name, string_literal("xp"))) {
                            lisp_form_get_s32(*param, &current_entity->stats.xp_value);
                        } else if (lisp_form_symbol_matching(*stat_name, string_literal("vigor"))) {
                            lisp_form_get_s32(*param, &current_entity->stats.vigor);
                        } else if (lisp_form_symbol_matching(*stat_name, string_literal("strength"))) {
                            lisp_form_get_s32(*param, &current_entity->stats.strength);
                        } else if (lisp_form_symbol_matching(*stat_name, string_literal("constitution"))) {
                            lisp_form_get_s32(*param, &current_entity->stats.constitution);
                        } else if (lisp_form_symbol_matching(*stat_name, string_literal("willpower"))) {
                            lisp_form_get_s32(*param, &current_entity->stats.willpower);
                        } else if (lisp_form_symbol_matching(*stat_name, string_literal("agility"))) {
                            lisp_form_get_s32(*param, &current_entity->stats.agility);
                        } else if (lisp_form_symbol_matching(*stat_name, string_literal("speed"))) {
                            lisp_form_get_s32(*param, &current_entity->stats.speed);
                        } else if (lisp_form_symbol_matching(*stat_name, string_literal("intelligence"))) {
                            lisp_form_get_s32(*param, &current_entity->stats.intelligence);
                        } else if (lisp_form_symbol_matching(*stat_name, string_literal("luck"))) {
                            lisp_form_get_s32(*param, &current_entity->stats.luck);
                        }
                    }
                }
            }
        }
    }
}

/* Obviously this isn't final code. */
void entity_database_initialize_abilities(struct entity_database* database) {
    _debugprintf("loading abilities file");

    struct memory_arena* arena             = database->arena;
    struct file_buffer   ability_data_file = read_entire_file(memory_arena_allocator(&scratch_arena), string_literal("./res/abilities.txt"));
    struct lisp_list     ability_forms     = lisp_read_string_into_forms(&scratch_arena, file_buffer_as_string(&ability_data_file));

    s32 ability_count = ability_forms.count;

    database->ability_capacity    = ability_count;
    database->ability_count       = ability_count;
    database->abilities           = memory_arena_push(arena, ability_count * sizeof(*database->abilities));
    database->ability_key_strings = memory_arena_push(arena, ability_count * sizeof(*database->ability_key_strings));

    for (s32 ability_form_index = 0; ability_form_index < ability_count; ++ability_form_index) {
        struct entity_ability* current_ability            = &database->abilities[ability_form_index];
        string*                current_ability_key_string = &database->ability_key_strings[ability_form_index];
        struct lisp_form*      current_ability_form       = ability_forms.forms + ability_form_index;
        _debugprintf("%p", current_ability_form);

        { 
            struct lisp_form* id_name_form                 = lisp_list_nth(current_ability_form, 0);
            struct lisp_form* name_form                    = lisp_list_nth(current_ability_form, 1);
            struct lisp_form* description_form             = lisp_list_nth(current_ability_form, 2);
            {
                string id_name_string     = {};
                string name_string        = {};
                string description_string = {};
                lisp_form_get_string(*id_name_form, &id_name_string);
                lisp_form_get_string(*name_form, &name_string);
                lisp_form_get_string(*description_form, &description_string);
                id_name_string     = string_clone(arena, id_name_string);
                name_string        = string_clone(arena, name_string);
                description_string = string_clone(arena, description_string);

                current_ability->id_name     = id_name_string;
                current_ability->name        = name_string;
                current_ability->description = description_string;
                *current_ability_key_string      = id_name_string;
            }

            struct lisp_form* selection_form               = lisp_list_nth(current_ability_form, 3);
            struct lisp_form* field_form                   = lisp_list_nth(current_ability_form, 4);
            struct lisp_form* animation_sequence_list_form = lisp_list_nth(current_ability_form, 5);

            {
                {
                    struct lisp_form* selection_form_type            = lisp_list_nth(selection_form, 0);
                    struct lisp_form* selection_form_is_moving_field = lisp_list_nth(selection_form, 1);

                    u8  selection_field_type = ABILITY_SELECTION_TYPE_FIELD;
                    s32 is_moving            = 0;
                    lisp_form_get_s32(*selection_form_is_moving_field, &is_moving);

                    if (lisp_form_symbol_matching(*selection_form_type, string_literal("field-shape"))) {
                        selection_field_type = ABILITY_SELECTION_TYPE_FIELD_SHAPE;
                    } else if (lisp_form_symbol_matching(*selection_form_type, string_literal("field"))) {
                        selection_field_type = ABILITY_SELECTION_TYPE_FIELD;
                    }

                    current_ability->selection_type = selection_field_type;
                    current_ability->moving_field   = (u8)(is_moving);
                }

                {
                    struct lisp_form* field_list = lisp_list_nth(field_form, 0);

                    if (lisp_form_symbol_matching(*field_list, string_literal("all-filled"))) {
                        _debugprintf("Fill all slots!");

                        for (s32 y_cursor = 0; y_cursor < ENTITY_ABILITY_SELECTION_FIELD_MAX_Y; ++y_cursor) {
                            for (s32 x_cursor = 0; x_cursor < ENTITY_ABILITY_SELECTION_FIELD_MAX_X; ++x_cursor) {
                                current_ability->selection_field[y_cursor][x_cursor] = 1;
                            }
                        }
                    } else {
                        /* specified */
                        /* assume it's a 9x9 grid */
                        for (s32 y_cursor = 0; y_cursor < ENTITY_ABILITY_SELECTION_FIELD_MAX_Y; ++y_cursor) {
                            struct lisp_form* current_row = lisp_list_nth(field_list, y_cursor);

                            for (s32 x_cursor = 0; x_cursor < ENTITY_ABILITY_SELECTION_FIELD_MAX_X; ++x_cursor) {
                                struct lisp_form* current_column = lisp_list_nth(current_row, x_cursor);

                                s32 current_value = 0;
                                assertion(current_column);
                                assertion(lisp_form_get_s32(*current_column, &current_value));
                                
                                current_ability->selection_field[y_cursor][x_cursor] = (u8)(current_value);
                            }
                        }
                    }
                }
                {entity_ability_compile_animation_sequence(arena, current_ability, animation_sequence_list_form);}
            }
        }
    }
}

struct entity_database entity_database_create(struct memory_arena* arena) {
    struct entity_database result = {};
    result.arena = arena;

    entity_database_initialize_loot_tables(&result);
    entity_database_initialize_entities(&result);
    entity_database_initialize_abilities(&result);

    return result;
}

void level_area_entity_unpack(struct level_area_entity* entity, struct entity* unpack_target) {
    unpack_target->flags               |= entity->flags;
    unpack_target->ai.flags            |= entity->ai_flags;
    unpack_target->facing_direction     = entity->facing_direction;
    unpack_target->loot_table_id_index  = entity->loot_table_id_index;
    _debugprintf("%f, %f", unpack_target->position.x, unpack_target->position.y);
    unpack_target->position             = v2f32_scale(entity->position, TILE_UNIT_SIZE);
    unpack_target->scale.x              = TILE_UNIT_SIZE* 0.8;
    unpack_target->scale.y              = TILE_UNIT_SIZE* 0.8;

    if (entity->health_override != -1) {unpack_target->health.value = entity->health_override;}
    if (entity->magic_override != -1)  {unpack_target->magic.value  = entity->magic_override;}

    entity_validate_death(unpack_target);
}

void entity_base_data_unpack(struct entity_database* entity_database, struct entity_base_data* data, struct entity* destination) {
    s32 base_id_index = (s32)(data - entity_database->entities); 

    destination->name        = data->name;
    destination->model_index = data->model_index;

    /* don't allow these flags to override. That could be bad. */
    data->flags                      &= ~(ENTITY_FLAGS_RUNTIME_RESERVATION);
    destination->flags               |= data->flags;
    destination->ai.flags            |= data->ai_flags;
    destination->stat_block           = data->stats;
    destination->health.min           = destination->magic.min = 0;
    destination->health.value         = destination->health.max = data->health;
    destination->magic.value          = destination->magic.max  = data->magic;
    destination->inventory            = data->inventory;
    destination->base_id_index        = base_id_index;

    for (s32 index = 0; index < array_count(data->equip_slots); ++index) {
        destination->equip_slots[index] = data->equip_slots[index];
    }
}

struct entity_base_data* entity_database_find_by_index(struct entity_database* entity_database, s32 index) {
    return entity_database->entities + index;
}

struct entity_base_data* entity_database_find_by_name(struct entity_database* entity_database, string name) {
    for (s32 entity_index = 0; entity_index < entity_database->entity_count; ++entity_index) {
        string entity_name = entity_database->entity_key_strings[entity_index];

        if (string_equal(entity_name, name)) {
            return entity_database_find_by_index(entity_database, entity_index);
        }
    }

    return entity_database_find_by_name(entity_database, string_literal("__default__"));
}

struct entity_loot_table* entity_database_loot_table_find_by_index(struct entity_database* entity_database, s32 index) {
    return entity_database->loot_tables + index;
}

struct entity_loot_table* entity_database_loot_table_find_by_name(struct entity_database* entity_database, string name) {
    for (s32 loot_table_index = 0; loot_table_index < entity_database->loot_table_count; ++loot_table_index) {
        string loot_table_name = entity_database->loot_table_key_strings[loot_table_index];

        if (string_equal(loot_table_name, name)) {
            return entity_database_loot_table_find_by_index(entity_database, loot_table_index);
        }
    }

    return NULL;
}

struct entity_ability* entity_database_ability_find_by_index(struct entity_database* entity_database, s32 index) {
    return entity_database->abilities + index;
}

struct entity_ability* entity_database_ability_find_by_name(struct entity_database* entity_database, string name) {
    _debugprintf("Count? : %d", entity_database->ability_count);
    for (s32 entity_ability_index = 0; entity_ability_index < entity_database->ability_count; ++entity_ability_index) {
        string ability_name = entity_database->ability_key_strings[entity_ability_index];
        _debugprintf("%.*s key : vs : %.*s", name.length, name.data, ability_name.length, ability_name.data);

        if (string_equal(ability_name, name)) {
            return entity_database_ability_find_by_index(entity_database, entity_ability_index);
        }
    }

    return NULL;
}

s32 entity_database_find_id_by_name(struct entity_database* entity_database, string name) {
    struct entity_base_data* e = entity_database_find_by_name(entity_database, name);
    if (e) {
        return e-entity_database->entities;
    }
    return 0;
}

s32 entity_database_loot_table_find_id_by_name(struct entity_database* entity_database, string name) {
    struct entity_loot_table* e = entity_database_loot_table_find_by_name(entity_database, name);
    if (e) {
        return e-entity_database->loot_tables;
    }
    return -1;
}

s32 entity_database_ability_find_id_by_name(struct entity_database* entity_database, string name) {
    struct entity_ability* e = entity_database_ability_find_by_name(entity_database, name);
    if (e) {
        return e-entity_database->abilities;
    }
    return -1;
}

struct entity_iterator entity_iterator_create(void) {
    struct entity_iterator iterator = {};

    return iterator;
}

void entity_iterator_push(struct entity_iterator* iterator, struct entity_list* list) {
    iterator->entity_lists[iterator->list_count++] = list;
}

struct entity* entity_iterator_begin(struct entity_iterator* iterator) {
    iterator->done              = 0;
    iterator->index             = 0;
    iterator->entity_list_index = 0;
    struct entity* result = entity_iterator_advance(iterator);
    return result;
}

bool entity_iterator_finished(struct entity_iterator* iterator) {
    return iterator->done;
}

/* TODO: this is slightly wrong. Not in the mood to do correctly. So there is a hack. */
struct entity* entity_iterator_advance(struct entity_iterator* iterator) {
    struct entity_list* current_iteration_list  = iterator->entity_lists[iterator->index];
    struct entity*      current_iterated_entity = current_iteration_list->entities + iterator->entity_list_index;
    iterator->current_id                        = entity_list_get_id(current_iteration_list, iterator->entity_list_index);

    iterator->entity_list_index += 1;

    if (iterator->entity_list_index >= current_iteration_list->capacity) {
        iterator->index += 1;
        iterator->entity_list_index = 0;
    }

    if (iterator->index >= iterator->list_count) {
        iterator->done = true;
    }

    return current_iterated_entity;
}

struct entity_iterator entity_iterator_clone(struct entity_iterator* base) {
    struct entity_iterator iterator = {};

    iterator                   = *base;
    iterator.entity_list_index = 0;
    iterator.index             = 0;
    iterator.done              = 0;

    return iterator;
}

struct item_instance* entity_loot_table_find_loot(struct memory_arena* arena, struct entity_loot_table* loot_table, s32* out_count) {
    if (!loot_table || loot_table->loot_count == 0) {
        _debugprintf("no loot!");
        *out_count = 0;
        return 0;
    }

    struct item_instance* loot_list  = memory_arena_push(arena, 0);
    s32                   loot_count = 0;

    for (s32 loot_index = 0; loot_index < loot_table->loot_count; ++loot_index) {
        struct entity_loot* current_loot_entry = loot_table->loot_items + loot_index;
        f32 loot_chance_roll = random_float(&game_state->rng);

        _debugprintf("Chance (%f) vs Roll (%f)", current_loot_entry->normalized_chance, loot_chance_roll);

        if (loot_chance_roll < current_loot_entry->normalized_chance) {
            s32 drop_amount = random_ranged_integer(&game_state->rng, current_loot_entry->count_min, current_loot_entry->count_max);

            {
                struct item_def* item_base = item_database_find_by_id(current_loot_entry->item);

                if (item_base->max_stack_value != -1) {
                    if (drop_amount > item_base->max_stack_value) {
                        drop_amount = item_base->max_stack_value;
                    }
                }
            }

            if (drop_amount == 0) {
                continue;
            } else {
                memory_arena_push(arena, sizeof(*loot_list));
                struct item_instance* new_loot_item = &loot_list[loot_count++];
                
                new_loot_item->item  = current_loot_entry->item;
                new_loot_item->count = drop_amount;
                _debugprintf("item id: (%u) looted (count %d)", new_loot_item->item.id_hash, new_loot_item->count);
            }

        }
    }

    *out_count = loot_count;
    return loot_list;
}

struct entity_loot_table* entity_lookup_loot_table(struct entity_database* entity_database, struct entity* entity) {
    s32                      base_id          = entity->base_id_index;
    struct entity_base_data* entity_base_data = entity_database->entities + base_id;

    s32 loot_table_id_index = entity->loot_table_id_index;
    if (loot_table_id_index == -1) {
        loot_table_id_index = entity_base_data->loot_table_id_index;
    }

    if (loot_table_id_index == -1) {
        return NULL;
    }

    return entity_database->loot_tables + loot_table_id_index;
}

void serialize_level_area_entity(struct binary_serializer* serializer, s32 version, struct level_area_entity* entity) {
    switch (version) {
        case 5:
        case 6: {
            serialize_f32(serializer, &entity->position.x);
            serialize_f32(serializer, &entity->position.y);

            serialize_f32(serializer, &entity->scale.x);
            serialize_f32(serializer, &entity->scale.y);

            serialize_bytes(serializer, entity->base_name, 64);

            serialize_s32(serializer, &entity->health_override);
            serialize_s32(serializer, &entity->magic_override);

            serialize_u8(serializer, &entity->facing_direction);
            serialize_u32(serializer, &entity->flags);
            serialize_u32(serializer, &entity->ai_flags);
            serialize_u32(serializer, &entity->spawn_flags);

            for (int i = 0; i < 16; ++i) serialize_u32(serializer, entity->group_ids);
            for (int i = 0; i < 128; ++i){u8 unused[128]; serialize_u8(serializer, unused);}
        } break;
        case CURRENT_LEVEL_AREA_VERSION: {
            Serialize_Structure(serializer, *entity);
        } break;
        default: {
            _debugprintf("Either this version doesn't support entities or never existed.");
        } break;
    }
}

void entity_add_ability_by_name(struct entity* entity, string id) {
    if (entity->ability_count < ENTITY_MAX_ABILITIES) {
        struct entity_ability_slot* ability_slot = &entity->abilities[entity->ability_count++];
        ability_slot->ability = entity_database_ability_find_id_by_name(&game_state->entity_database, id);
    }
}

#include "entity_ability.c"
