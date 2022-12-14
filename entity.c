#include "entities_def.c"

/* TODO: remove checking for active entities since the iterator should take care of it now */

/*
  NOTE:

  Quads are not shaded with shaders, which causes particles to look weird right now since they are just quads.

  To be fair particles should be round and this will fix itself soon enough when I draw a round blob.
 */

/*
 CLEANUP/TODO: As models are now the source of truth for entity collision sizes,
 and thus arbitrary scales are not needed (entities can just be represented as points with an implied rectangle),
 I can remove those later.

 TODO: add usage of the damage formulas.
 TODO: Add weapon animation types for the base model so that I can start reskinning stuff
 TODO: Obviously add placeholder weapon types.
 */

local void entity_advance_ability_sequence(struct entity* entity);

#include "handle_specialfx_sequence_action.c"
#include "handle_hardcoded_animation_sequence_action.c"

bool entity_bad_ref(struct entity* e);
void entity_do_healing(struct entity* entity, s32 healing);
void entity_do_physical_hurt(struct entity* entity, s32 damage);
void entity_do_absolute_hurt(struct entity* entity, s32 damage);
/* some status effects may have non-local effects so those are handled elsewhere. */
void entity_update_all_status_effects(struct entity* entity, f32 dt);

/* LOLOLOLOLOL THIS IS SLOW */
entity_id game_find_id_for_entity(struct entity* entity);

local s32 entity_get_physical_damage_raw(struct entity* entity) {
    s32 strength = entity_find_effective_stat_value(entity, STAT_STRENGTH);
    s32 agility  = entity_find_effective_stat_value(entity, STAT_AGILITY);

    s32 result = ceilf(strength * 0.45 + agility * 0.37);

    if (result < 0) {
        result = 0;
    }

    return result;
}
local s32 entity_get_physical_damage(struct entity* entity) {
    /* TODO special effects for critical */
    s32 raw               = entity_get_physical_damage_raw(entity);
    f32 random_percentage = random_ranged_float(&game_state->rng, 0.4, 0.6);
    s32 raw2              = entity_get_physical_damage_raw(entity)*random_percentage;

    s32 variance_random = random_ranged_integer(&game_state->rng, -raw2, raw2);

    return raw + variance_random;
}

/* PARTICLES START */
/*
  NOTE: Will move into game state when it is more stable.
*/
local struct entity_particle_list global_particle_list = {};

void initialize_particle_pools(struct memory_arena* arena, s32 particles_total_count) {
    global_particle_list.capacity  = particles_total_count;
    global_particle_list.particles = memory_arena_push(arena,
                                                       sizeof(*global_particle_list.particles) *
                                                       particles_total_count);
}

local struct entity_particle* particle_list_allocate_particle(struct entity_particle_list* particle_list) {
    for (s32 particle_index = 0; particle_index < particle_list->capacity; ++particle_index) {
        struct entity_particle* current_particle = particle_list->particles + particle_index;

        if (!(current_particle->flags & ENTITY_PARTICLE_FLAG_ALIVE)) {
            current_particle->flags |= ENTITY_PARTICLE_FLAG_ALIVE;
            return current_particle;
        }
    }

    return NULL;
}

local v2f32 entity_particle_emitter_spawn_shape_find_position(v2f32 offset, struct random_state* rng, struct entity_particle_emitter_spawn_shape* spawn_shape) {
    switch (spawn_shape->type) {
        case ENTITY_PARTICLE_EMITTER_SPAWN_SHAPE_POINT: {
            struct entity_particle_emitter_spawn_shape_point point = spawn_shape->point;

            point.center.x += offset.x;
            point.center.y += offset.y;

            f32 x = random_ranged_float(rng, point.center.x - point.thickness, point.center.x + point.thickness);
            f32 y = random_ranged_float(rng, point.center.y - point.thickness, point.center.y + point.thickness);

            return v2f32(x, y);
        } break;
        case ENTITY_PARTICLE_EMITTER_SPAWN_SHAPE_LINE: {
            struct entity_particle_emitter_spawn_shape_line line = spawn_shape->line;

            line.start.x += offset.x;
            line.start.y += offset.y;
            line.end.x   += offset.x;
            line.end.y   += offset.y;

            f32 x = random_ranged_float(rng, line.start.x, line.end.x);
            x += random_ranged_float(rng, -line.thickness, line.thickness);
            f32 y = random_ranged_float(rng, line.start.y, line.end.y);
            y += random_ranged_float(rng, -line.thickness, line.thickness);

            return v2f32(x, y);
        } break;
        case ENTITY_PARTICLE_EMITTER_SPAWN_SHAPE_RECTANGLE: {
            struct entity_particle_emitter_spawn_shape_rectangle rectangle = spawn_shape->rectangle;

            rectangle.center.x += offset.x;
            rectangle.center.y += offset.y;

            if (spawn_shape->outline) {
                s32 side = random_ranged_integer(rng, 0, 3);

                f32 x = 0;
                f32 y = 0;
                switch (side) {
                    case 0: { /* left */
                        x  = random_ranged_float(rng, rectangle.center.x - rectangle.half_widths.x - rectangle.thickness, rectangle.center.x - rectangle.half_widths.x + rectangle.thickness);
                        y  = random_ranged_float(rng, rectangle.center.y - rectangle.half_widths.y, rectangle.center.y + rectangle.half_widths.y);
                        y += random_ranged_float(rng, -rectangle.thickness, rectangle.thickness);
                    } break;
                    case 1: { /* top */
                        y  = random_ranged_float(rng, rectangle.center.y + rectangle.half_widths.y - rectangle.thickness, rectangle.center.y + rectangle.half_widths.y + rectangle.thickness);
                        x  = random_ranged_float(rng, rectangle.center.x - rectangle.half_widths.x, rectangle.center.x + rectangle.half_widths.x);
                        x += random_ranged_float(rng, -rectangle.thickness, rectangle.thickness);
                    } break;
                    case 2: { /* right */
                        x  = random_ranged_float(rng, rectangle.center.x + rectangle.half_widths.x - rectangle.thickness, rectangle.center.x + rectangle.half_widths.x + rectangle.thickness);
                        y  = random_ranged_float(rng, rectangle.center.y - rectangle.half_widths.y, rectangle.center.y + rectangle.half_widths.y);
                        y += random_ranged_float(rng, -rectangle.thickness, rectangle.thickness);
                    } break;
                    case 3: { /* bottom */
                        y  = random_ranged_float(rng, rectangle.center.y + rectangle.half_widths.y - rectangle.thickness, rectangle.center.y + rectangle.half_widths.y + rectangle.thickness);
                        x  = random_ranged_float(rng, rectangle.center.x - rectangle.half_widths.x, rectangle.center.x + rectangle.half_widths.x);
                        x += random_ranged_float(rng, -rectangle.thickness, rectangle.thickness);
                    } break;
                }

                return v2f32(x, y);
            } else {
                f32 x = random_ranged_float(rng, rectangle.center.x - rectangle.half_widths.x, rectangle.center.x + rectangle.half_widths.x);
                f32 y = random_ranged_float(rng, rectangle.center.y - rectangle.half_widths.y, rectangle.center.y + rectangle.half_widths.y);

                return v2f32(x, y);
            }
        } break;
        case ENTITY_PARTICLE_EMITTER_SPAWN_SHAPE_CIRCLE: {
            struct entity_particle_emitter_spawn_shape_circle circle = spawn_shape->circle;

            circle.center.x += offset.x;
            circle.center.y += offset.y;

            f32 random_angle = random_ranged_float(rng, 0, 360);
            f32 cosine = cos(random_angle);
#ifdef EXPERIMENTAL_PARTICLES_TEST_ELLIPSOID_PERSPECTIVE
            f32 sine = sin(random_angle) * 0.65;
#else
            f32 sine = sin(random_angle);
#endif

            if (spawn_shape->outline) {
                f32 x = random_ranged_float(rng, circle.center.x+circle.radius*cosine-circle.thickness, circle.center.x+circle.radius*cosine+circle.thickness);
                f32 y = random_ranged_float(rng, circle.center.y+circle.radius*sine-circle.thickness, circle.center.y+circle.radius*sine+circle.thickness);
                return v2f32(x, y);
            } else {
                /* NOTE: this method is technically not correct due to distribution or something but it might look good enough. */
                f32 radius = random_ranged_float(rng, 0, circle.radius);
                f32 x = random_ranged_float(rng, circle.center.x+radius*cosine-circle.thickness, circle.center.x+radius*cosine+circle.thickness);
                f32 y = random_ranged_float(rng, circle.center.y+radius*sine-circle.thickness, circle.center.y+radius*sine+circle.thickness);
                return v2f32(x, y);
            }
        } break;
        bad_case;
    }

    return offset;
}


void entity_particle_emitter_list_update(struct entity_particle_emitter_list* particle_emitters, f32 dt) {
    struct random_state* rng = &game_state->rng;
    /* TODO: Does not have dying particle emitters yet! */

    for (unsigned particle_emitter_index = 0; particle_emitter_index < particle_emitters->capacity; ++particle_emitter_index) {
        struct entity_particle_emitter* current_emitter = particle_emitters->emitters + particle_emitter_index;

        if (!(current_emitter->flags & ENTITY_PARTICLE_EMITTER_ACTIVE) ||
            !(current_emitter->flags & ENTITY_PARTICLE_EMITTER_ON)) {
            continue;
        }

        if (current_emitter->delay_time > 0) {
            current_emitter->delay_time -= dt;
        } else {
            if (current_emitter->time <= 0) {
                current_emitter->time = current_emitter->time_per_spawn; 

                if (current_emitter->burst_amount == 0) {
                    current_emitter->burst_amount = 1;
                }

                for (s32 emitted = 0; emitted < current_emitter->burst_amount; ++emitted) {
                    /* new particle */
                    struct entity_particle* particle = particle_list_allocate_particle(&global_particle_list);
                    current_emitter->currently_spawned_batch_amount++;

                    if (!particle) {
                        break;
                    }

                    particle->associated_particle_emitter_index = particle_emitter_index;

                    {
                        /* depends on spawn shape... */
                        particle->position = entity_particle_emitter_spawn_shape_find_position(current_emitter->position, rng, &current_emitter->spawn_shape);
                    }

                    f32 scale = random_ranged_float(rng, current_emitter->scale_uniform - current_emitter->scale_variance_uniform, current_emitter->scale_uniform + current_emitter->scale_variance_uniform);
                    particle->scale = v2f32(scale, scale);

                    f32 lifetime             = random_ranged_float(rng, current_emitter->lifetime - current_emitter->lifetime_variance, current_emitter->lifetime_variance + current_emitter->lifetime_variance);
                    particle->color          = current_emitter->color;
                    particle->target_color   = current_emitter->target_color;
                    particle->feature_flags  = current_emitter->particle_feature_flags;
                    particle->lifetime       = particle->lifetime_max = lifetime;
                    particle->velocity.x     = random_ranged_float(rng, current_emitter->starting_velocity.x - current_emitter->starting_velocity_variance.x, current_emitter->starting_velocity.x + current_emitter->starting_velocity_variance.x);
                    particle->velocity.y     = random_ranged_float(rng, current_emitter->starting_velocity.y - current_emitter->starting_velocity_variance.y, current_emitter->starting_velocity.y + current_emitter->starting_velocity_variance.y);
                    particle->acceleration.x = random_ranged_float(rng, current_emitter->starting_acceleration.x - current_emitter->starting_acceleration_variance.x, current_emitter->starting_acceleration.x + current_emitter->starting_acceleration_variance.x);
                    particle->acceleration.y = random_ranged_float(rng, current_emitter->starting_acceleration.y - current_emitter->starting_acceleration_variance.y, current_emitter->starting_acceleration.y + current_emitter->starting_acceleration_variance.y);
                }

                if (current_emitter->max_spawn_per_batch != -1) {
                    if (current_emitter->currently_spawned_batch_amount > current_emitter->max_spawn_per_batch) {
                        current_emitter->currently_spawned_batch_amount = 0; 
                        current_emitter->spawned_batches++;
                        current_emitter->delay_time = current_emitter->delay_time_per_batch;
                    }
                }

                if (current_emitter->max_spawn_batches != -1) {
                    if (current_emitter->spawned_batches >= current_emitter->max_spawn_batches) {
                        current_emitter->flags = 0;
                    }
                }
            } else {
                current_emitter->time -= dt;
            }
        }
    }
}

local void particle_list_cleanup_dead_particles(struct entity_particle_list* particle_list) {
    for (s32 particle_index = 0; particle_index < particle_list->capacity; ++particle_index) {
        struct entity_particle* current_particle = particle_list->particles + particle_index;
        if (current_particle->lifetime <= 0) {
            particle_list->particles[particle_index].flags = 0;
        }
    }
}

local void particle_list_kill_all_particles(struct entity_particle_list* particle_list) {
    for (unsigned particle_index = 0; particle_index < particle_list->capacity; ++particle_index) {
        struct entity_particle* current_particle = particle_list->particles + particle_index;
        current_particle->lifetime = 0;
    }
}

local void particle_list_update_particles(struct entity_particle_list* particle_list, f32 dt) {
    for (s32 particle_index = 0; particle_index < particle_list->capacity; ++particle_index) {
        struct entity_particle* current_particle = particle_list->particles + particle_index;
        assertion(particle_index < particle_list->capacity && "WTF is happening?");

        if (!(current_particle->flags & ENTITY_PARTICLE_FLAG_ALIVE)) {
            continue;
        }

        if (current_particle->feature_flags & ENTITY_PARTICLE_FEATURE_FLAG_ACCELERATION) {
            current_particle->velocity.x += current_particle->acceleration.x * dt;
            current_particle->velocity.y += current_particle->acceleration.y * dt;
        }

        if (current_particle->feature_flags & ENTITY_PARTICLE_FEATURE_FLAG_VELOCITY) {
            current_particle->position.x += current_particle->velocity.x * dt;
            current_particle->position.y += current_particle->velocity.y * dt;
        }

        current_particle->lifetime   -= dt;
    }

    particle_list_cleanup_dead_particles(particle_list);
}

void DEBUG_render_particle_emitters(struct render_commands* commands, struct entity_particle_emitter_list* emitters) {
#ifndef RELEASE
    for (unsigned emitter_index = 0; emitter_index < emitters->capacity; ++emitter_index) {
        struct entity_particle_emitter* current_emitter = emitters->emitters + emitter_index;
        if ((current_emitter->flags & ENTITY_PARTICLE_EMITTER_ACTIVE)) {
            render_commands_push_quad(commands, rectangle_f32(current_emitter->position.x * TILE_UNIT_SIZE,
                                                              current_emitter->position.y * TILE_UNIT_SIZE,
                                                              TILE_UNIT_SIZE,
                                                              TILE_UNIT_SIZE),
                                      color32u8(255, 0, 255, 255), BLEND_MODE_ALPHA);
            render_commands_set_shader(commands, game_foreground_things_shader, NULL);
        }
    }
#endif
}

void render_particles_list(struct entity_particle_list* particle_list, struct sortable_draw_entities* draw_entities) {
    for (unsigned particle_index = 0; particle_index < particle_list->capacity; ++particle_index) {
        struct entity_particle* current_particle = particle_list->particles + particle_index;
        if (current_particle->flags & ENTITY_PARTICLE_FLAG_ALIVE) {
            sortable_draw_entities_push_particle(draw_entities, current_particle->position.y*TILE_UNIT_SIZE, current_particle);
        }
    }
}

void entity_particle_emitter_kill(struct entity_particle_emitter_list* emitters, s32 particle_emitter_id) {
    if (particle_emitter_id == 0) {
        return;
    }
    struct entity_particle_emitter* emitter = emitters->emitters + (particle_emitter_id-1);
    emitter->flags = 0;
}

void entity_particle_emitter_kill_all(struct entity_particle_emitter_list* emitters) {
    for (s32 emitter_index = 0; emitter_index < emitters->capacity; ++emitter_index) {
        entity_particle_emitter_kill(emitters, emitter_index+1);
    }
}

s32 entity_particle_emitter_allocate(struct entity_particle_emitter_list* emitters) {
    for (s32 emitter_index = 0; emitter_index < emitters->capacity; ++emitter_index) {
        struct entity_particle_emitter* current_emitter = emitters->emitters + emitter_index;
        
        if (!(current_emitter->flags & ENTITY_PARTICLE_EMITTER_ACTIVE)) {
            s32 result = emitter_index+1;
            entity_particle_emitter_retain(emitters, result);
            _debugprintf("New particle emitter (%d)", result);
            return result;
        }
    }

    return 0;
}

void entity_particle_emitter_kill_all_particles(s32 particle_emitter_id) {
    for (s32 particle_index = 0; particle_index < global_particle_list.capacity; ++particle_index) {
        struct entity_particle* current_particle = global_particle_list.particles + particle_index;

        if (current_particle->associated_particle_emitter_index == (particle_emitter_id-1)) {
            current_particle->lifetime = 0;
            current_particle->flags = 0;
        }
    }
}

void entity_particle_emitter_start_emitting(struct entity_particle_emitter_list* emitters, s32 particle_emitter_id) {
    if (particle_emitter_id == 0) {
        return;
    }
    struct entity_particle_emitter* emitter = emitters->emitters + (particle_emitter_id-1);
    emitter->flags |= ENTITY_PARTICLE_EMITTER_ON;
}

void entity_particle_emitter_stop_emitting(struct entity_particle_emitter_list* emitters, s32 particle_emitter_id) {
    if (particle_emitter_id == 0) {
        return;
    }
    struct entity_particle_emitter* emitter = emitters->emitters + (particle_emitter_id-1);
    emitter->flags &= ~(ENTITY_PARTICLE_EMITTER_ON);
}

void entity_particle_emitter_retain(struct entity_particle_emitter_list* emitters, s32 particle_emitter_id) {
    if (particle_emitter_id == 0) {
        return;
    }
    struct entity_particle_emitter* emitter = emitters->emitters + (particle_emitter_id-1);
    emitter->flags |= ENTITY_PARTICLE_EMITTER_ACTIVE;
}

struct entity_particle_emitter* entity_particle_emitter_dereference(struct entity_particle_emitter_list* emitters, s32 particle_emitter_id) {
    if (particle_emitter_id == 0) {
        return NULL;
    }

    struct entity_particle_emitter* result = &emitters->emitters[particle_emitter_id-1];
    if (result->flags & ENTITY_PARTICLE_EMITTER_ACTIVE) {
        return result;
    }

    return NULL;
}
/* PARTICLES END */

void entity_savepoint_active_particle_emitter(struct entity_particle_emitter* emitter) {
    /* ideally I'd like a more advanced effect but this will do for now. I'll tweak it once I can get it to work. */
    {
        emitter->time_per_spawn                  = 0.05;
        emitter->burst_amount                    = 128;
        emitter->max_spawn_per_batch             = 1024;
        emitter->max_spawn_batches               = -1;
        emitter->color                           = color32u8(34, 88, 226, 255);
        emitter->target_color                    = color32u8(59, 59, 56, 127);
        emitter->starting_acceleration           = v2f32(0, -15.6);
        emitter->starting_acceleration_variance  = v2f32(1.2, 1.2);
        emitter->starting_velocity_variance      = v2f32(1.3, 0);
        emitter->lifetime                        = 0.6;
        emitter->lifetime_variance               = 0.35;
        emitter->spawn_shape                     = emitter_spawn_shape_circle(v2f32(0,0), 0.5, 0.03, true);

        /* emitter->particle_type = ENTITY_PARTICLE_TYPE_FIRE; */
        emitter->particle_feature_flags = ENTITY_PARTICLE_FEATURE_FLAG_FLAMES;

        emitter->scale_uniform = 0.09;
        emitter->scale_variance_uniform = 0.05;
    }
}
void entity_savepoint_inactive_particle_emitter(struct entity_particle_emitter* emitter) {
    /* ideally I'd like a more advanced effect but this will do for now. I'll tweak it once I can get it to work. */
    {
        emitter->time_per_spawn                  = 0.15;
        emitter->burst_amount                    = 24;
        emitter->max_spawn_per_batch             = 512;
        emitter->max_spawn_batches               = -1;
        emitter->color                           = color32u8(34, 88, 226, 255);
        emitter->target_color                    = color32u8(59, 59, 56, 127);
        emitter->starting_acceleration           = v2f32(0, -4.6);
        emitter->starting_acceleration_variance  = v2f32(1.2, 1.2);
        emitter->starting_velocity_variance      = v2f32(1.3, 0);
        emitter->lifetime                        = 0.6;
        emitter->lifetime_variance               = 0.55;
        emitter->spawn_shape                     = emitter_spawn_shape_circle(v2f32(0,0), 0.7, 0.05, false);

        /* emitter->particle_type = ENTITY_PARTICLE_TYPE_FIRE; */
        emitter->particle_feature_flags = ENTITY_PARTICLE_FEATURE_FLAG_FLAMES;

        emitter->scale_uniform = 0.04;
        emitter->scale_variance_uniform = 0.03;
    }
}
void entity_savepoint_initialize(struct entity_savepoint* savepoint) {
    /* initialize particle systems here */
    savepoint->particle_emitter_id          = entity_particle_emitter_allocate(&game_state->permenant_particle_emitters);
    struct entity_particle_emitter* emitter = entity_particle_emitter_dereference(&game_state->permenant_particle_emitters, savepoint->particle_emitter_id);
    emitter->position                       = savepoint->position;
    entity_savepoint_inactive_particle_emitter(emitter);
}

void battle_ui_stalk_entity_with_camera(struct entity*);
void battle_ui_stop_stalk_entity_with_camera(void);
void _debug_print_id(entity_id id) {
    _debugprintf("ent id[g:%d]: %d, %d", id.generation, id.store_type, id.index);
}

void entity_play_animation(struct entity* entity, string name, bool with_direction) {
    if (with_direction) {
        string facing_direction_string = facing_direction_strings_normal[entity->facing_direction];
        string directional_counterpart = format_temp_s("%.*s_%.*s", name.length, name.data, facing_direction_string.length, facing_direction_string.data);

        if (find_animation_by_name(entity->model_index, directional_counterpart)) {
            name = directional_counterpart;
        }
    }

    if (string_equal(entity->animation.name, name)) {
        return;
    }

    s32 smaller_length = name.length;
    if (smaller_length >= array_count(entity->animation.name_buffer)) {
        smaller_length = array_count(entity->animation.name_buffer);
    }
    cstring_copy(name.data, entity->animation.name_buffer, smaller_length);

    entity->animation.name                = string_from_cstring(entity->animation.name_buffer);
    entity->animation.current_frame_index = 0;
    entity->animation.iterations          = 0;
    entity->animation.timer               = 0;
}

void entity_play_animation_no_direction(struct entity* entity, string name) {
    entity_play_animation(entity, name, 0);
}

void entity_play_animation_with_direction(struct entity* entity, string name) {
    entity_play_animation(entity, name, 1);
}

struct rectangle_f32 entity_rectangle_collision_bounds(struct entity* entity) {
    f32 model_width_units = entity_model_get_width_units(entity->model_index);

    return rectangle_f32(entity->position.x, /* collision oddities, this removes sticking */
                         entity->position.y,
                         entity->scale.x * model_width_units,
                         entity->scale.y-10);
}

void entity_snap_to_grid_position(struct entity* entity) {
    struct rectangle_f32 rectangle = entity_rectangle_collision_bounds(entity);

    v2f32 position   = entity->position;
    position.x      += rectangle.w/2; /* center position */
    position.y      += rectangle.h/2; /* hopefully this looks better when snapping */
    position.x       = floorf((position.x) / TILE_UNIT_SIZE) * TILE_UNIT_SIZE;
    position.y       = floorf((position.y) / TILE_UNIT_SIZE) * TILE_UNIT_SIZE;
    entity->position = position;
}


void entity_look_at(struct entity* entity, v2f32 position) {
    v2f32 position_delta = v2f32_direction(entity->position, position);

    /* _debugprintf("look at delta: %f, %f\n", position_delta.x, position_delta.y); */
    if (fabs(position_delta.x) > fabs(position_delta.y)) {
        if (position_delta.x < 0) {
            entity->facing_direction = DIRECTION_LEFT;
        } else if (position_delta.x > 0) {
            entity->facing_direction = DIRECTION_RIGHT;
        }
    } else {
        if (position_delta.y < 0) {
            entity->facing_direction = DIRECTION_UP;
        } else if (position_delta.y > 0) {
            entity->facing_direction = DIRECTION_DOWN;
        }
    }
}
/* weirdo */
void entity_look_at_and_set_animation_state(struct entity* entity, v2f32 position) {
    entity_look_at(entity, position);
    entity_play_animation_with_direction(entity, string_literal("idle"));
}

struct entity_list entity_list_create(struct memory_arena* arena, s32 capacity, u8 store_mark) {
    struct entity_list result = {
        .entities         = memory_arena_push(arena, capacity * sizeof(*result.entities)),
        .generation_count = memory_arena_push(arena, capacity * sizeof(*result.generation_count)),
        .capacity         = capacity,
        .store_type       = store_mark,
    };

    for (s32 entity_index = 0; entity_index < capacity; ++entity_index) {
        struct entity* current_entity = result.entities + entity_index;
        current_entity->flags = 0;
    }

    zero_memory(result.generation_count, result.capacity * sizeof(*result.generation_count));
    return result;
}

struct entity_list entity_list_clone(struct memory_arena* arena, struct entity_list original) {
    struct entity_list clone_result = {
        .store_type = original.store_type,
        .capacity   = original.capacity,
    };

    clone_result.generation_count = memory_arena_push(arena, sizeof(*clone_result.generation_count) * original.capacity);
    clone_result.entities         = memory_arena_push(arena, sizeof(*clone_result.entities)         * original.capacity);

    entity_list_copy(&original, &clone_result);
    return clone_result;
}

void entity_list_copy(struct entity_list* source, struct entity_list* destination) {
    assertion(source->generation_count && "Incomplete source list! (missing generation count?)");
    assertion(source->entities         && "Incomplete source list! (missing entities)");
    memory_copy(source->generation_count, destination->generation_count, sizeof(*destination->generation_count) * destination->capacity);
    memory_copy(source->entities,         destination->entities,         sizeof(*destination->entities)         * destination->capacity);
}

entity_id entity_list_create_entity(struct entity_list* entities) {
    for (s32 index = 0; index < entities->capacity; ++index) {
        struct entity* current_entity = entities->entities + index;

        if (!(current_entity->flags & ENTITY_FLAGS_ACTIVE)) {
            zero_memory(current_entity, sizeof(*current_entity));
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

entity_id entity_list_find_entity_id_with_scriptname(struct entity_list* list, string scriptname) {
    entity_id result = {};

#if 0
    _debugprintf("Trying to find entity: %.*s\n", scriptname.length, scriptname.data);
#endif
    for (s32 entity_index = 0; entity_index < list->capacity; ++entity_index) {
        struct entity* current_entity = list->entities + entity_index;
        string script_name_target = string_from_cstring(current_entity->script_name);
#if 0
        _debugprintf("\tcomparing against: %.*s\n", script_name_target.length, script_name_target.data);
#endif

        if (string_equal(script_name_target, scriptname)) {
            result = entity_list_get_id(list, entity_index);
            break;
        }
    }

    return result;
}

local f32 entity_health_percentage(struct entity* entity) {
    int current = entity->health.value;
    int max     = entity->health.max;

    return (f32)current/(f32)max;
}

local bool entity_is_dead(struct entity* entity) {
    if (!(entity->flags & ENTITY_FLAGS_ALIVE)) {
        return true;
    }

    return false;
}

/* requires tilemap world... */
/* TODO fix implicit decls, linker hasn't killed game yet */
void player_handle_radial_interactables(struct game_state* state, struct entity* entity, f32 dt);

bool allow_explore_player_entity_movement(struct game_state* state) {
    if (game_command_console_enabled) {
        return false;
    }

    if (disable_game_input) {
        return false;
    }

    if (cutscene_active()) {
        return false;
    }

    if (global_popup_state.message_count > 0) {
        return false;
    }

    /* combat has it's own special movement rules. */
    if (state->combat_state.active_combat) {
        return false;
    }

    if (region_zone_animation_block_input) {
        return false;
    }
    if (storyboard_active) {
        return false;
    }
    /* conversations should be in it's own "module" like region_change_presentation.c */
    if (state->is_conversation_active) {
        return false;
    }

    return true;
}

void entity_handle_player_controlled(struct game_state* state, struct entity* entity, f32 dt) {
    /* all the input blockers. */
    if (!allow_explore_player_entity_movement(state)) {
        return;
    }

    {
        if (game_get_party_leader() != entity) {
            f32            flock_radius    = TILE_UNIT_SIZE*0.85 * game_get_party_number(entity)*1.12;
            f32            flock_radius_sq = flock_radius*flock_radius;
            struct entity* leader          = game_get_party_leader();

            v2f32 direction = v2f32_direction(entity->position, leader->position);

            if (v2f32_distance_sq(entity->position, leader->position) > flock_radius_sq) {
                entity->velocity.x = direction.x * DEFAULT_VELOCITY;
                entity->velocity.y = direction.y * DEFAULT_VELOCITY;
            }
            entity_look_at(entity, leader->position);
            return;
        }
    }

    f32 move_up    = is_action_down(INPUT_ACTION_MOVE_UP);
    f32 move_down  = is_action_down(INPUT_ACTION_MOVE_DOWN);
    f32 move_left  = is_action_down(INPUT_ACTION_MOVE_LEFT);
    f32 move_right = is_action_down(INPUT_ACTION_MOVE_RIGHT);

    /* some hacky bullshit to fix out later */
    
    struct game_controller* gamepad = get_gamepad(0);
    f32 analog_move_vertically   = 0;
    f32 analog_move_horizontally = 0;

    /* ?? */
    if (gamepad) {
        analog_move_horizontally = gamepad->left_stick.axes[0];
        analog_move_vertically   = gamepad->left_stick.axes[1];

        const f32 MOVE_THRESHOLD = 0.4;

        if (analog_move_vertically > MOVE_THRESHOLD) {
            move_down = true;
        } else if (analog_move_vertically < -MOVE_THRESHOLD) {
            move_up   = true;
        }

        if (analog_move_horizontally > MOVE_THRESHOLD) {
            move_right = true;
        } else if (analog_move_horizontally < -MOVE_THRESHOLD) {
            move_left   = true;
        }
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


struct rectangle_f32 push_out_horizontal_edges(struct rectangle_f32 collider, struct rectangle_f32 collidant, bool* stop_horizontal_movement) {
    struct rectangle_f32 entity_bounds = collider;

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
            collider.x = collidant_right_edge;
        } else if (entity_right_edge > collidant_left_edge) {
            collider.x = collidant_left_edge - entity_bounds.w;
        }
        *stop_horizontal_movement = true;
    }

    return collider;
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
struct rectangle_f32 push_out_vertical_edges(struct rectangle_f32 collider, struct rectangle_f32 collidant, bool* stop_vertical_movement) {
    struct rectangle_f32 entity_bounds = collider;

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
            collider.y = collidant_top_edge - entity_bounds.h;
        } else if (entity_top_edge < collidant_bottom_edge && entity_bottom_edge > collidant_bottom_edge) {
            collider.y = collidant_bottom_edge;
        }
        *stop_vertical_movement = true;
    }

    return collider;
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
struct entity_iterator game_permenant_entity_iterator(struct game_state* state) {
    struct entity_iterator result = {};

    entity_iterator_push(&result, &state->permenant_entities);

    return result;
}

/* Slow, but gets called only once rarely. */
s32 entity_get_usable_ability_indices(struct entity* entity, s32 max_limit, s32* ability_indices) {
    s32 wrote = 0;

    if (max_limit == -1) {
        max_limit = ENTITY_MAX_ABILITIES;
    }

    _debugprintf("Counting : %d", entity->ability_count);
    for (s32 ability_index = 0; ability_index < entity->ability_count; ++ability_index) {
        struct entity_ability_slot* current_ability_slot = entity->abilities + ability_index;
        bool                        usable               = true;
        s32                         ability_id           = current_ability_slot->ability;
        struct entity_ability*      ability_to_use       = entity_database_ability_find_by_index(&game_state->entity_database, ability_id);

        {
            if (!ability_to_use->innate) {
                _debugprintf("This ability is not innate. Need to check for qualification");
                s32                      ability_class_group = ability_to_use->item_class_group;
                usable                                       = false;

                /* check if an item provides the ability directly, which bypasses innate lock */
                {
                    for (s32 item_index = 0; item_index < ENTITY_EQUIP_SLOT_INDEX_COUNT && !usable; ++item_index) {
                        struct item_id        item_id = entity->equip_slots[item_index];
                        struct item_def*      item_base = item_database_find_by_id(item_id);

                        if (item_base) {
                            for (s32 item_ability_index = 0; item_ability_index < item_base->ability_count && !usable; ++item_ability_index) {
                                if (item_base->abilities[item_ability_index] == ability_id) {
                                    usable = true;
                                    _debugprintf("This ability was found on this item.");
                                }
                            }
                        }
                    }
                }
                /* check if an item provides item group unlock */
                {
                    if (ability_class_group != 0) {
                        _debugprintf("This ability has a class... Let's look for it");
                        for (s32 item_index = 0; item_index < ENTITY_EQUIP_SLOT_INDEX_COUNT && !usable; ++item_index) {
                            struct item_id        item_id = entity->equip_slots[item_index];
                            struct item_def*      item_base = item_database_find_by_id(item_id);

                            if (item_base) {
                                if (item_base->ability_class_group_id == ability_class_group) {
                                    usable = true;
                                    _debugprintf("Item class matching found.");
                                }
                            }
                        }
                    }
                }
            }
        }


        if (usable) {
            if (wrote < max_limit) {
                if (ability_indices) {
                    _debugprintf("Can add ability");
                    ability_indices[wrote++] = ability_index;
                } else {
                    wrote += 1;
                }
            }
        }
    }

    return wrote;
}

s32 entity_usable_ability_count(struct entity* entity) {
    return entity_get_usable_ability_indices(entity, -1, NULL);
}

void update_entities(struct game_state* state, f32 dt, struct entity_iterator it, struct level_area* area) {
    for (struct entity* current_entity = entity_iterator_begin(&it); !entity_iterator_finished(&it); current_entity = entity_iterator_advance(&it)) {
        if (!(current_entity->flags & ENTITY_FLAGS_ACTIVE)) {
            continue;
        }

        if (current_entity->flags & ENTITY_FLAGS_HIDDEN) {
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

                        if (!current_entity->ai.current_action) {
                            for (s32 index = 0; index < area->tile_layers[TILE_LAYER_OBJECT].count && !stop_horizontal_movement; ++index) {
                                struct tile* current_tile = area->tile_layers[TILE_LAYER_OBJECT].tiles + index;
                                struct tile_data_definition* tile_data = tile_table_data + current_tile->id;

                                if (Get_Bit(tile_data->flags, TILE_DATA_FLAGS_SOLID)) {
                                    stop_horizontal_movement |=
                                        entity_push_out_horizontal_edges(current_entity, rectangle_f32(current_tile->x * TILE_UNIT_SIZE, current_tile->y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE));
                                }
                            }

                            /* solid objects in the scriptable layer */
                            {
                                for (s32 layer_index = TILE_LAYER_SCRIPTABLE_0; layer_index <= TILE_LAYER_SCRIPTABLE_31; ++layer_index) {
                                    struct scriptable_tile_layer_property* layer_properties = area->scriptable_layer_properties + (layer_index - TILE_LAYER_SCRIPTABLE_0);

                                    if (layer_properties->draw_layer != TILE_LAYER_OBJECT) {
                                        continue;
                                    }

                                    if (layer_properties->flags & SCRIPTABLE_TILE_LAYER_FLAGS_HIDDEN) {
                                        continue;
                                    }

                                    if (layer_properties->flags & SCRIPTABLE_TILE_LAYER_FLAGS_NOCOLLIDE) {
                                        continue;
                                    }

                                    for (s32 index = 0; index < area->tile_layers[layer_index].count && !stop_horizontal_movement; ++index) {
                                        struct tile* current_tile = area->tile_layers[layer_index].tiles + index;
                                        struct tile_data_definition* tile_data = tile_table_data + current_tile->id;

                                        if (Get_Bit(tile_data->flags, TILE_DATA_FLAGS_SOLID)) {
                                            stop_horizontal_movement |=
                                                entity_push_out_horizontal_edges(current_entity, rectangle_f32(current_tile->x * TILE_UNIT_SIZE, current_tile->y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE));
                                        }
                                    }
                                }
                            }

                            for (s32 index = 0; index < area->tile_layers[TILE_LAYER_GROUND].count && !stop_horizontal_movement; ++index) {
                                struct tile* current_tile = area->tile_layers[TILE_LAYER_GROUND].tiles + index;
                                struct tile_data_definition* tile_data = tile_table_data + current_tile->id;

                                if (Get_Bit(tile_data->flags, TILE_DATA_FLAGS_SOLID)) {
                                    stop_horizontal_movement |=
                                        entity_push_out_horizontal_edges(current_entity, rectangle_f32(current_tile->x * TILE_UNIT_SIZE, current_tile->y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE));
                                }
                            }

                            for (s32 index = 0; index < area->chests.count && !stop_horizontal_movement; ++index) {
                                struct entity_chest* chest = area->chests.chests + index;

                                stop_horizontal_movement |=
                                    entity_push_out_horizontal_edges(current_entity, rectangle_f32(chest->position.x * TILE_UNIT_SIZE, chest->position.y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE));
                            }

                            if (stop_horizontal_movement) current_entity->velocity.x = 0;
                        }
                    }


                    current_entity->position.y += current_entity->velocity.y * dt;
                    {
                        bool stop_vertical_movement = false;

                        if (!current_entity->ai.current_action) {
                            for (s32 index = 0; index < area->tile_layers[TILE_LAYER_OBJECT].count && !stop_vertical_movement; ++index) {
                                struct tile* current_tile = area->tile_layers[TILE_LAYER_OBJECT].tiles + index;
                                struct tile_data_definition* tile_data = tile_table_data + current_tile->id;

                                if (Get_Bit(tile_data->flags, TILE_DATA_FLAGS_SOLID)) {
                                    stop_vertical_movement |=
                                        entity_push_out_vertical_edges(current_entity, rectangle_f32(current_tile->x * TILE_UNIT_SIZE, current_tile->y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE));
                                }
                            }

                            /* solid objects in the scriptable layer */
                            {
                                for (s32 layer_index = TILE_LAYER_SCRIPTABLE_0; layer_index <= TILE_LAYER_SCRIPTABLE_31; ++layer_index) {
                                    struct scriptable_tile_layer_property* layer_properties = area->scriptable_layer_properties + (layer_index - TILE_LAYER_SCRIPTABLE_0);

                                    if (layer_properties->draw_layer != TILE_LAYER_OBJECT) {
                                        continue;
                                    }

                                    if (layer_properties->flags & SCRIPTABLE_TILE_LAYER_FLAGS_HIDDEN) {
                                        continue;
                                    }

                                    if (layer_properties->flags & SCRIPTABLE_TILE_LAYER_FLAGS_NOCOLLIDE) {
                                        continue;
                                    }

                                    for (s32 index = 0; index < area->tile_layers[layer_index].count && !stop_vertical_movement; ++index) {
                                        struct tile* current_tile = area->tile_layers[layer_index].tiles + index;
                                        struct tile_data_definition* tile_data = tile_table_data + current_tile->id;

                                        if (Get_Bit(tile_data->flags, TILE_DATA_FLAGS_SOLID)) {
                                            stop_vertical_movement |=
                                                entity_push_out_vertical_edges(current_entity, rectangle_f32(current_tile->x * TILE_UNIT_SIZE, current_tile->y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE));
                                        }
                                    }
                                }
                            }

                            for (s32 index = 0; index < area->tile_layers[TILE_LAYER_GROUND].count && !stop_vertical_movement; ++index) {
                                struct tile* current_tile = area->tile_layers[TILE_LAYER_GROUND].tiles + index;
                                struct tile_data_definition* tile_data = tile_table_data + current_tile->id;

                                if (Get_Bit(tile_data->flags, TILE_DATA_FLAGS_SOLID)) {
                                    stop_vertical_movement |=
                                        entity_push_out_vertical_edges(current_entity, rectangle_f32(current_tile->x * TILE_UNIT_SIZE, current_tile->y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE));
                                }
                            }

                            for (s32 index = 0; index < area->chests.count && !stop_vertical_movement; ++index) {
                                struct entity_chest* chest = area->chests.chests + index;

                                stop_vertical_movement |=
                                    entity_push_out_vertical_edges(current_entity, rectangle_f32(chest->position.x * TILE_UNIT_SIZE, chest->position.y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE));
                            }

                            if (stop_vertical_movement) current_entity->velocity.y = 0;
                        }
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
                handle_entity_level_trigger_interactions(state, current_entity, &area->trigger_level_transitions, dt);
                handle_entity_scriptable_trigger_interactions(state, current_entity, &area->triggers, dt);
            } else {
                current_entity->position.x += current_entity->velocity.x * dt;
                current_entity->position.y += current_entity->velocity.y * dt;
            }

            /* implicit animation state setting for now. */
            if (!(current_entity->flags & ENTITY_FLAGS_ALIVE)) {
                game_report_entity_death(it.current_id);
                if (current_entity->ai.hurt_animation_phase == ENTITY_HURT_ANIMATION_ON) {
                    /* Need to figure this... */
                } else {
                    switch (current_entity->ai.death_animation_phase) {
                        case DEATH_ANIMATION_KNEEL: {
                            entity_play_animation_no_direction(current_entity, string_literal("kneel_down"));
                            if (current_entity->animation.iterations > DEATH_ANIMATION_MAX_KNEEL_HUFFS) {
                                current_entity->ai.death_animation_phase = DEATH_ANIMATION_DIE;
                            }
                        } break;
                            /* can try to grow pool of blood... */
                        case DEATH_ANIMATION_DIE: {
                            entity_play_animation_no_direction(current_entity, string_literal("dead"));
                        } break;
                    }
                }
            } else {
                /* animation state will be controlled by the action while it happens */
                entity_update_all_status_effects(current_entity, dt);
                if (!current_entity->ai.current_action) {
                    if (current_entity->velocity.x != 0 || current_entity->velocity.y != 0) {
                        entity_play_animation_with_direction(current_entity, string_literal("walk"));
                    } else {
                        entity_play_animation_with_direction(current_entity, string_literal("idle"));
                    }
                }
            }

            /* damage shake + flash */
            if (current_entity->ai.hurt_animation_phase == ENTITY_HURT_ANIMATION_ON) {
                current_entity->ai.hurt_animation_timer += dt;

                if (current_entity->ai.hurt_animation_timer >= HURT_ANIMATION_TIME_BETWEEN_SHAKES) {
                    current_entity->ai.hurt_animation_timer   = 0;
                    current_entity->ai.hurt_animation_shakes += 1;

                    current_entity->ai.hurt_animation_shake_offset.x = random_ranged_integer(&game_state->rng, -HURT_ANIMATION_MAX_SHAKE_OFFSET, HURT_ANIMATION_MAX_SHAKE_OFFSET);
                    current_entity->ai.hurt_animation_shake_offset.y = random_ranged_integer(&game_state->rng, -HURT_ANIMATION_MAX_SHAKE_OFFSET/2, HURT_ANIMATION_MAX_SHAKE_OFFSET/2);
                }

                if (current_entity->ai.hurt_animation_shakes >= HURT_ANIMATION_MAX_SHAKE_COUNT) {
                    current_entity->ai.hurt_animation_shake_offset.x = 0;
                    current_entity->ai.hurt_animation_shake_offset.y = 0;
                    current_entity->ai.hurt_animation_phase = ENTITY_HURT_ANIMATION_OFF;
                }
            } else {
            }
        }
    }

    {
        Array_For_Each(it, struct entity_savepoint, area->savepoints, area->entity_savepoint_count) {
            struct entity_particle_emitter* emitter_object = entity_particle_emitter_dereference(&game_state->permenant_particle_emitters, it->particle_emitter_id);
            emitter_object->position                       = it->position;

            if (it->player_ontop) {
                entity_savepoint_active_particle_emitter(emitter_object);
            } else {
                entity_savepoint_inactive_particle_emitter(emitter_object);
            }

            entity_particle_emitter_start_emitting(&game_state->permenant_particle_emitters, it->particle_emitter_id);
        }
    }
}

local void entity_think_basic_zombie_combat_actions(struct entity* entity, struct game_state* state);
void entity_think_combat_actions(struct entity* entity, struct game_state* state, f32 dt) {
    /* This should only think about submitting actions... Oh well */
    if (entity->flags & ENTITY_FLAGS_PLAYER_CONTROLLED) {
        return;
    }
    {
        if (entity->ai.wait_timer < 1.1) {
            entity->ai.wait_timer += dt;
            return;
        }

        {entity_think_basic_zombie_combat_actions(entity, state);}

        entity->ai.wait_timer = 0;
    }
}

/*
  NOTE: In terms of render commands, I don't seem to actually need to use them for sorting as I know most of the
  order and the few things that genuinely need to be sorted can just be done... Where they need to be sorted here.

  (Honestly, in retrospect the best thing to do would just to be allowing the render_commands system sort it themselves. But that's
  for a later date since this obviously works fine.)
*/

struct sortable_draw_entities sortable_draw_entities(struct memory_arena* arena, s32 capacity) {
    struct sortable_draw_entities result;
    result.count = 0;
    result.entities = memory_arena_push(arena, sizeof(*result.entities) * capacity);
    return result;
}
void sortable_draw_entities_push(struct sortable_draw_entities* entities, u8 type, f32 y_sort_key, void* ptr) {
    struct sortable_draw_entity* current_draw_entity = &entities->entities[entities->count++];
    current_draw_entity->type                        = type;
    current_draw_entity->y_sort_key                  = y_sort_key;
    current_draw_entity->pointer                     = ptr;
}
void sortable_draw_entities_push_entity(struct sortable_draw_entities* entities, f32 y_sort_key, entity_id id) {
    struct sortable_draw_entity* current_draw_entity = &entities->entities[entities->count++];
    current_draw_entity->type                        = SORTABLE_DRAW_ENTITY_ENTITY;
    current_draw_entity->y_sort_key                  = y_sort_key;
    current_draw_entity->entity_id                   = id;
}
void sortable_draw_entities_push_chest(struct sortable_draw_entities* entities, f32 y_sort_key, void* ptr) {
    sortable_draw_entities_push(entities, SORTABLE_DRAW_ENTITY_CHEST, y_sort_key, ptr);
}
void sortable_draw_entities_push_particle(struct sortable_draw_entities* entities, f32 y_sort_key, void* ptr) {
    /* this will allow particles to tend to be on top of entities */
    if (((struct entity_particle*)ptr)->feature_flags & ENTITY_PARTICLE_FEATURE_FLAG_HIGHERSORTBIAS) {
        y_sort_key += 2.56;
    }

    if (((struct entity_particle*)ptr)->feature_flags & ENTITY_PARTICLE_FEATURE_FLAG_ALWAYSFRONT) {
        y_sort_key += 10; 
    }

    sortable_draw_entities_push(entities, SORTABLE_DRAW_ENTITY_PARTICLE, y_sort_key, ptr);
}
void sortable_draw_entities_push_savepoint(struct sortable_draw_entities* entities, f32 y_sort_key, void* ptr) {
    sortable_draw_entities_push(entities, SORTABLE_DRAW_ENTITY_SAVEPOINT, y_sort_key, ptr);
}

void sortable_draw_entities_sort_keys(struct sortable_draw_entities* entities) {
    /* insertion sort */
    for (s32 draw_entity_index = 1; draw_entity_index < entities->count; ++draw_entity_index) {
        s32 sorted_insertion_index = draw_entity_index;
        struct sortable_draw_entity key_entity = entities->entities[draw_entity_index];

        for (; sorted_insertion_index > 0; --sorted_insertion_index) {
            struct sortable_draw_entity comparison_entity = entities->entities[sorted_insertion_index-1];

            if (comparison_entity.y_sort_key < key_entity.y_sort_key) {
                /* found insertion spot */
                break;
            } else {
                /* push everything forward */
                entities->entities[sorted_insertion_index] = entities->entities[sorted_insertion_index-1];
            }
        }

        entities->entities[sorted_insertion_index] = key_entity;
    }
}

local void sortable_entity_draw_entity(struct render_commands* commands, struct graphics_assets* assets, entity_id id, f32 dt) {
    struct entity* current_entity = game_dereference_entity(game_state, id);

    {
        s32 facing_direction = current_entity->facing_direction;
        s32 model_index      = current_entity->model_index;

        if (!(current_entity->flags & ENTITY_FLAGS_ACTIVE)) {
            return;
        }

        struct entity_animation* anim = find_animation_by_name(model_index, current_entity->animation.name);

        if (!anim) {
            _debugprintf("cannot find anim: %.*s. Falling back to \"down direction\"", current_entity->animation.name.length, current_entity->animation.name.data);
            entity_play_animation_with_direction(current_entity, string_literal("idle"));
            anim = find_animation_by_name(model_index, current_entity->animation.name);
            assertion(anim && "Okay, if this still failed something is really wrong...");
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
        f32 scale_x = TILE_UNIT_SIZE/REFERENCE_TILE_UNIT_SIZE;
        
        v2f32 real_dimensions  = v2f32(sprite_dimensions.x * scale_x, sprite_dimensions.y * scale_x);

        bool should_shift_up = (real_dimensions.y / TILE_UNIT_SIZE) >= 1;

        v2f32 alignment_offset = v2f32(0, real_dimensions.y * should_shift_up * 0.8);

        v2f32 other_offsets;
        other_offsets.x = current_entity->ai.hurt_animation_shake_offset.x;
        other_offsets.y = current_entity->ai.hurt_animation_shake_offset.y;

        f32 SHADOW_SPRITE_WIDTH  = TILE_UNIT_SIZE * roundf(real_dimensions.x/TILE_UNIT_SIZE);
        f32 SHADOW_SPRITE_HEIGHT = TILE_UNIT_SIZE * max(roundf(real_dimensions.y/(TILE_UNIT_SIZE*2)), 1);

        bool water_tile_submergence = false;

        {
            const f32 X_BIAS = 0;
            const f32 Y_BIAS = 0.123;
            struct tile* floor_tile = level_area_get_tile_at(&game_state->loaded_area,
                                                             TILE_LAYER_GROUND, roundf((current_entity->position.x/TILE_UNIT_SIZE)-X_BIAS), roundf((current_entity->position.y/TILE_UNIT_SIZE)-Y_BIAS));
            if (floor_tile) {
                s32 get_tile_id_by_name(string);
                if (floor_tile->id == get_tile_id_by_name(string_literal("water [solid]")) ||
                    floor_tile->id == get_tile_id_by_name(string_literal("water"))) {
                    water_tile_submergence = true;
                }
            }
        }

        if (!water_tile_submergence) {
            render_commands_push_image(commands,
                                       graphics_assets_get_image_by_id(assets, drop_shadow),
                                       rectangle_f32(current_entity->position.x - alignment_offset.x + other_offsets.x,
                                                     current_entity->position.y - TILE_UNIT_SIZE*0.4 + other_offsets.y,
                                                     SHADOW_SPRITE_WIDTH,
                                                     SHADOW_SPRITE_HEIGHT),
                                       RECTANGLE_F32_NULL,
                                       color32f32(1,1,1,0.72), NO_FLAGS, BLEND_MODE_ALPHA);
            render_commands_set_shader(commands, game_background_things_shader, NULL);
        }


        union color32f32 modulation_color = color32f32_WHITE;

        /* fire hurts */
        switch (current_entity->burning_char_effect_level) {
            case 1:
            case 2: {
                modulation_color.r = modulation_color.g = modulation_color.b = 0.75;
            } break;
            case 3: {
                modulation_color.r = modulation_color.g = modulation_color.b = 0.50;
            } break;
            case 4: {
                modulation_color.r = modulation_color.g = modulation_color.b = 0.34;
            } break;
        }

        /* Highlighted for target selection */
        {
            bool me = is_entity_under_ability_selection(id) || current_entity->under_selection;
            if (me) {
                /* red for now. We want better effects maybe? */
                modulation_color.g = modulation_color.b = 0;
            }
        }
        current_entity->under_selection = false;

        f32 height_trim = 0.0787;


        { /*HACKME special alignment case for death animations, this is after shadows since the shadow should not move */
            if (string_equal(current_entity->animation.name, string_literal("dead"))) {
                other_offsets.y += TILE_UNIT_SIZE/2;
                height_trim = 0.32;
            }
        }

        if (!water_tile_submergence) {
            height_trim = 0;
        }

        {
            struct image_buffer* used_image = graphics_assets_get_image_by_id(assets, sprite_to_use);
            struct rectangle_f32 rectangle  = rectangle_f32(current_entity->position.x - alignment_offset.x + other_offsets.x,
                                                            current_entity->position.y - alignment_offset.y + other_offsets.y,
                                                            real_dimensions.x,
                                                            real_dimensions.y*(1 - height_trim));
            render_commands_push_image(commands,
                                       used_image,
                                       rectangle,
                                       rectangle_f32(0, 0, sprite_dimensions.x, sprite_dimensions.y * (1 - height_trim)),
                                       modulation_color, NO_FLAGS, BLEND_MODE_ALPHA);
            v2f32 rectangle_position_transformed = v2f32(rectangle.x, rectangle.y);
            rectangle_position_transformed       = camera_transform(&commands->camera, rectangle_position_transformed, SCREEN_WIDTH, SCREEN_HEIGHT);
            rectangle.x                          = rectangle_position_transformed.x;
            rectangle.y                          = rectangle_position_transformed.y;

            lightmask_buffer_blit_image(&global_lightmask_buffer, used_image, rectangle, RECTANGLE_F32_NULL, LIGHTMASK_BLEND_NONE, BLEND_MODE_ALPHA, 0);
        }
        if (current_entity->ai.hurt_animation_phase == ENTITY_HURT_ANIMATION_ON) {
            if ((current_entity->ai.hurt_animation_shakes % 2) == 0) {
                struct image_buffer* used_image = graphics_assets_get_image_by_id(assets, sprite_to_use);
                struct rectangle_f32 rectangle  = rectangle_f32(current_entity->position.x - alignment_offset.x + other_offsets.x,
                                                                current_entity->position.y - alignment_offset.y + other_offsets.y,
                                                                real_dimensions.x,
                                                                real_dimensions.y*(1 - height_trim));
                v2f32 rectangle_position_transformed = v2f32(rectangle.x, rectangle.y);
                rectangle_position_transformed       = camera_transform(&commands->camera, rectangle_position_transformed, SCREEN_WIDTH, SCREEN_HEIGHT);
                rectangle.x                          = rectangle_position_transformed.x;
                rectangle.y                          = rectangle_position_transformed.y;

                lightmask_buffer_blit_image(&global_lightmask_buffer, used_image, rectangle, RECTANGLE_F32_NULL, LIGHTMASK_BLEND_NONE, BLEND_MODE_ALPHA, 255);
            }
        }
        render_commands_set_shader(commands, game_foreground_entity_things_shader, current_entity);

        if (current_entity->flags & ENTITY_FLAGS_ALIVE && entity_is_in_defense_stance(current_entity)) {
            struct image_buffer* used_image = graphics_assets_get_image_by_id(assets, ui_battle_defend_icon);
            struct rectangle_f32 rectangle  = rectangle_f32(current_entity->position.x - alignment_offset.x,
                                                            current_entity->position.y - alignment_offset.y - TILE_UNIT_SIZE*1.2 + (sinf(global_elapsed_time * 1.7) * TILE_UNIT_SIZE * 0.2),
                                                            TILE_UNIT_SIZE,
                                                            TILE_UNIT_SIZE);

            render_commands_push_image(commands, used_image, rectangle, RECTANGLE_F32_NULL, color32f32_WHITE, NO_FLAGS, BLEND_MODE_ALPHA);

            v2f32 rectangle_position_transformed = v2f32(rectangle.x, rectangle.y);
            rectangle_position_transformed       = camera_transform(&commands->camera, rectangle_position_transformed, SCREEN_WIDTH, SCREEN_HEIGHT);
            rectangle.x                          = rectangle_position_transformed.x;
            rectangle.y                          = rectangle_position_transformed.y;

            lightmask_buffer_blit_image(&global_lightmask_buffer, used_image, rectangle, RECTANGLE_F32_NULL, LIGHTMASK_BLEND_OR, BLEND_MODE_ALPHA, 255);
            render_commands_set_shader(commands, game_foreground_things_shader, NULL);
        }

#ifndef RELEASE
#if 1
        struct rectangle_f32 collision_bounds = entity_rectangle_collision_bounds(current_entity);
        render_commands_push_quad(commands, collision_bounds, color32u8(255, 0, 0, 128), BLEND_MODE_ALPHA);
        render_commands_set_shader(commands, game_foreground_things_shader, NULL);
#endif
#endif
    }
}

local void sortable_entity_draw_chest(struct render_commands* commands, struct graphics_assets* assets, struct entity_chest* chest, f32 dt) {
    struct entity_chest* it = chest;

    if (it->flags & ENTITY_CHEST_FLAGS_UNLOCKED) {
        render_commands_push_image(commands,
                                   graphics_assets_get_image_by_id(assets, chest_open_top_img),
                                   rectangle_f32(it->position.x * TILE_UNIT_SIZE,
                                                 (it->position.y-0.5) * TILE_UNIT_SIZE,
                                                 it->scale.x * TILE_UNIT_SIZE,
                                                 it->scale.y * TILE_UNIT_SIZE),
                                   RECTANGLE_F32_NULL,
                                   color32f32(1,1,1,1), NO_FLAGS, BLEND_MODE_ALPHA);
        render_commands_set_shader(commands, game_foreground_things_shader, NULL);
        render_commands_push_image(commands,
                                   graphics_assets_get_image_by_id(assets, chest_open_bottom_img),
                                   rectangle_f32(it->position.x * TILE_UNIT_SIZE,
                                                 it->position.y * TILE_UNIT_SIZE,
                                                 it->scale.x * TILE_UNIT_SIZE,
                                                 it->scale.y * TILE_UNIT_SIZE),
                                   RECTANGLE_F32_NULL,
                                   color32f32(1,1,1,1), NO_FLAGS, BLEND_MODE_ALPHA);
        render_commands_set_shader(commands, game_foreground_things_shader, NULL);
    } else {
        render_commands_push_image(commands,
                                   graphics_assets_get_image_by_id(assets, chest_closed_img),
                                   rectangle_f32(it->position.x * TILE_UNIT_SIZE,
                                                 it->position.y * TILE_UNIT_SIZE,
                                                 it->scale.x * TILE_UNIT_SIZE,
                                                 it->scale.y * TILE_UNIT_SIZE),
                                   RECTANGLE_F32_NULL,
                                   color32f32(1,1,1,1), NO_FLAGS, BLEND_MODE_ALPHA);
        render_commands_set_shader(commands, game_foreground_things_shader, NULL);
    }
}

local void sortable_entity_draw_particle(struct render_commands* commands, struct graphics_assets* assets, struct entity_particle* particle, f32 dt) {
    struct entity_particle* it = particle;

    f32 draw_x = it->position.x;
    f32 draw_y = it->position.y;
    f32 draw_w = it->scale.x;
    f32 draw_h = it->scale.y;

    draw_x *= TILE_UNIT_SIZE;
    draw_y *= TILE_UNIT_SIZE;
    draw_w *= TILE_UNIT_SIZE;
    draw_h *= TILE_UNIT_SIZE;

    /* good enough for now */
    union color32f32 color = color32u8_to_color32f32(it->color);
    f32 effective_t = (it->lifetime/it->lifetime_max);
    if (effective_t < 0) effective_t = 0;
    if (effective_t > 1) effective_t = 1;

    if (it->feature_flags & ENTITY_PARTICLE_FEATURE_FLAG_COLORFADE) {
        union color32f32 target_color = color32u8_to_color32f32(it->target_color);
        color.r = lerp_f32(color.r, target_color.r, 1-effective_t);
        color.g = lerp_f32(color.g, target_color.g, 1-effective_t);
        color.b = lerp_f32(color.b, target_color.b, 1-effective_t);
        color.a = lerp_f32(color.a, target_color.a, 1-effective_t);
    }

    if (it->feature_flags & ENTITY_PARTICLE_FEATURE_FLAG_ALPHAFADE) {
        color.a *= effective_t;
    }

    if (it->feature_flags & ENTITY_PARTICLE_FEATURE_FLAG_FLAMELIKE) {
        render_commands_push_quad(commands, rectangle_f32(draw_x, draw_y, draw_w, draw_h), color32f32_to_color32u8(color), BLEND_MODE_ALPHA);
        render_commands_set_shader(commands, game_foreground_things_shader, NULL);
        render_commands_push_quad(commands, rectangle_f32(draw_x, draw_y, draw_w, draw_h), color32f32_to_color32u8(color), BLEND_MODE_ADDITIVE);
        render_commands_set_shader(commands, game_foreground_things_shader, NULL);
    } else {
        if (it->feature_flags & ENTITY_PARTICLE_FEATURE_FLAG_ADDITIVEBLEND) {
            render_commands_push_quad(commands, rectangle_f32(draw_x, draw_y, draw_w, draw_h), color32f32_to_color32u8(color), BLEND_MODE_ADDITIVE);
            render_commands_set_shader(commands, game_foreground_things_shader, NULL);
        } else {
            render_commands_push_quad(commands, rectangle_f32(draw_x, draw_y, draw_w, draw_h), color32f32_to_color32u8(color), BLEND_MODE_ALPHA);
            render_commands_set_shader(commands, game_foreground_things_shader, NULL);
        }
    }

    if (it->feature_flags & ENTITY_PARTICLE_FEATURE_FLAG_FULLBRIGHT) {
        v2f32 draw_point = v2f32(draw_x, draw_y);
        draw_point       = camera_transform(&commands->camera, draw_point, SCREEN_WIDTH, SCREEN_HEIGHT);
        lightmask_buffer_blit_rectangle(&global_lightmask_buffer, rectangle_f32(draw_point.x, draw_point.y, draw_w, draw_h), LIGHTMASK_BLEND_OR, 255);
    }
}

local void sortable_entity_draw_savepoint(struct render_commands* commands, struct graphics_assets* assets, struct entity_savepoint* savepoint, f32 dt) {
    /*
      Savepoints are mainly powered by their particle systems.
      So there's no drawing method per say.

      But I'm putting it here just in case I ever decide to "draw them"
    */
    return;
}

void sortable_draw_entities_submit(struct render_commands* commands, struct graphics_assets* graphics_assets, struct sortable_draw_entities* entities, f32 dt) {
    sortable_draw_entities_sort_keys(entities);

    for (s32 draw_entity_index = 0; draw_entity_index < entities->count; ++draw_entity_index) {
        struct sortable_draw_entity* current_draw_entity = entities->entities + draw_entity_index;

        switch (current_draw_entity->type) {
            case SORTABLE_DRAW_ENTITY_ENTITY: {
                sortable_entity_draw_entity(commands, graphics_assets, current_draw_entity->entity_id, dt);
            } break;
            case SORTABLE_DRAW_ENTITY_CHEST: {
                struct entity_chest* it = current_draw_entity->pointer;
                sortable_entity_draw_chest(commands, graphics_assets, it, dt);
            } break;
            case SORTABLE_DRAW_ENTITY_PARTICLE: {
                struct entity_particle* it = current_draw_entity->pointer;
                sortable_entity_draw_particle(commands, graphics_assets, it, dt);
            } break;
            case SORTABLE_DRAW_ENTITY_SAVEPOINT: {
                struct entity_savepoint* it = current_draw_entity->pointer;
                sortable_entity_draw_savepoint(commands, graphics_assets, it, dt);
            } break;
                bad_case;
        }
    }
}

void render_entities_from_area_and_iterator(struct sortable_draw_entities* draw_entities, struct entity_iterator it, struct level_area* area) {
    {
        for (struct entity* current_entity = entity_iterator_begin(&it); !entity_iterator_finished(&it); current_entity = entity_iterator_advance(&it)) {
            if (!(current_entity->flags & ENTITY_FLAGS_ACTIVE)) {
                continue;
            }
            
            if (current_entity->flags & ENTITY_FLAGS_HIDDEN) {
                continue;
            }

            sortable_draw_entities_push_entity(draw_entities, current_entity->position.y, it.current_id);
        }
    }

    {
        Array_For_Each(it, struct entity_chest, area->chests.chests, area->chests.count) {
            if (it->flags & ENTITY_FLAGS_HIDDEN) {
                continue;
            }
            sortable_draw_entities_push_chest(draw_entities, it->position.y*TILE_UNIT_SIZE, it);
        }

        Array_For_Each(it, struct entity_savepoint, area->savepoints, area->entity_savepoint_count) {
            if (it->flags & ENTITY_FLAGS_HIDDEN) {
                continue;
            }
            sortable_draw_entities_push_savepoint(draw_entities, it->position.y*TILE_UNIT_SIZE, it);
        }
    }
}

void render_entities(struct game_state* state, struct sortable_draw_entities* draw_entities) {
    struct entity_iterator iterator = game_entity_iterator(state);
    struct level_area*     area     = &state->loaded_area;
    render_entities_from_area_and_iterator(draw_entities, iterator, area);
}

struct entity_query_list find_entities_within_radius(struct memory_arena* arena, struct game_state* state, v2f32 position, f32 radius, bool obstacle) {
    return find_entities_within_radius_without_obstacles(arena, state, position, radius);
    /* buggy */
#if 0 
    if (obstacle) {
        return find_entities_within_radius_with_obstacles(arena, state, position, radius);
    } else {
        return find_entities_within_radius_without_obstacles(arena, state, position, radius);
    }
#endif
}

struct entity_query_list find_entities_within_radius_without_obstacles(struct memory_arena* arena, struct game_state* state, v2f32 position, f32 radius) {
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

/* NOTE: Need to special case spears or projectiles which can go over entities, or add versions with filters */
/* NOTE: not accurate enough? This is lightly broken */
struct entity_query_list find_entities_within_radius_with_obstacles(struct memory_arena* arena, struct game_state* state, v2f32 position, f32 radius) {
    struct entity_query_list result          = {};
    struct entity_iterator   it              = game_entity_iterator(state);
    entity_id*               candidates      = memory_arena_push(arena, 0);
    s32                      candidate_count = 0;
    f32                      radius_sq       = radius * radius;

    for (struct entity* target = entity_iterator_begin(&it); !entity_iterator_finished(&it); target = entity_iterator_advance(&it)) {
        if (!(target->flags & ENTITY_FLAGS_ACTIVE))
            continue;

        f32 entity_distance_sq = v2f32_distance_sq(position, target->position);

        if (entity_distance_sq <= radius_sq) {
            memory_arena_push(arena, sizeof(*candidates));
            candidates[candidate_count++] = it.current_id;
        }
    }

    /* marker pointer */
    result.ids = memory_arena_push(arena, 0);
    struct level_area* area = &state->loaded_area;

    struct entity* whoeverisonme = game_any_entity_at_tile_point(v2f32_scale(position, 1.0/TILE_UNIT_SIZE));
    for (s32 candidate_index = 0; candidate_index < candidate_count; ++candidate_index) {
        entity_id      candidate = candidates[candidate_index];
        struct entity* entity    = game_dereference_entity(game_state, candidate);

        v2f32          direction = v2f32_direction(position, entity->position);
        direction.x              *= TILE_UNIT_SIZE;
        direction.y              *= TILE_UNIT_SIZE;

        s32 iteration_count = (s32)ceilf(radius/TILE_UNIT_SIZE);

        struct entity* first_entity = 0;
        for (s32 iteration = 0; iteration < iteration_count; ++iteration) {
            v2f32 current_position = v2f32_add(position, v2f32_scale(direction, iteration+1));
            v2f32 tile_position    = v2f32_scale(current_position, 1.0 / TILE_UNIT_SIZE);

            if (!first_entity) {
                first_entity = game_any_entity_at_tile_point(tile_position);

                if (first_entity && first_entity != whoeverisonme) {
                    _debugprintf("hit guy that is not me [%p %p]", whoeverisonme, first_entity);
                    memory_arena_push(arena, sizeof(*result.ids));
                    result.ids[result.count++] = game_find_id_for_entity(first_entity);
                    break;
                }
            }

            {
                if (level_area_any_obstructions_at(area, tile_position.x, tile_position.y)) {
                    break;
                }
            }
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

void entity_inventory_remove_item_by_name(struct entity_inventory* inventory, string item, bool remove_all) {
    item_id id = item_id_make(item);

    for (s32 item_index = inventory->count-1; item_index >= 0; --item_index) {
        if (item_id_equal(inventory->items[item_index].item, id)) {
            entity_inventory_remove_item(inventory, item_index, remove_all);
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
            _debugprintf("Can try to use consumable item");
            _debugprintf("(%.*s) HP VALUE: %d", item->name.length, item->name.data, item->health_restoration_value);
            if (item->health_restoration_value != 0) {
                entity_do_healing(target, item->health_restoration_value);
                _debugprintf("ITEMUSED: (%p) (item %p) restored: %d health", item, target, item->health_restoration_value);
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

        /* register any abilities that the item has */
        {
            for (s32 item_ability_index = 0; item_ability_index < item_base->ability_count; ++item_ability_index) {
                entity_add_ability_by_id(target, item_base->abilities[item_ability_index]);
            }

            if (item_base->ability_class_group_id != 0) {
                _debugprintf("Trying to learn ability with class: %d", item_base->ability_class_group_id);
                for (s32 entity_ability_index = 0; entity_ability_index < game_state->entity_database.ability_count; ++entity_ability_index) {
                    struct entity_ability* current_ability = game_state->entity_database.abilities + entity_ability_index;

                    _debugprintf("Ability class group: %d", current_ability->item_class_group);
                    if (current_ability->item_class_group == item_base->ability_class_group_id) {
                        _debugprintf("Okay! I should be adding an ability!");
                        entity_add_ability_by_id(target, entity_ability_index);
                    }
                }
            }
        }
    }
}

void entity_inventory_unequip_item(struct entity_inventory* inventory, s32 limits, s32 equipment_index, struct entity* target) {
    if (target->equip_slots[equipment_index].id_hash != 0) {
        _debugprintf("have an item to unequip");
        entity_inventory_add(inventory, limits, target->equip_slots[equipment_index]);
        target->equip_slots[equipment_index] = (item_id) {};
    }
}

bool entity_any_equipped_item(struct entity* target, s32 equipment_index) {
    if (target->equip_slots[equipment_index].id_hash != 0) {
        _debugprintf("has equipped item");
        return true;
    }

    return false;
}

void entity_inventory_use_item(struct entity_inventory* inventory, s32 item_index, struct entity* target) {
    struct item_instance* current_item = &inventory->items[item_index];
    struct item_def* item_base         = item_database_find_by_id(current_item->item);

    if (item_base->type == ITEM_TYPE_CONSUMABLE_ITEM) {
        _debugprintf("Trying to use (item %d) from (inventory %p)", item_index, inventory);
        entity_inventory_remove_item(inventory, item_index, false);
        item_apply_to_entity(item_base, target);
    }
}

local void entity_copy_path_array_into_navigation_data(struct entity* entity, v2f32* path_points, s32 path_count) {
    s32 minimum_count = 0;

    if (path_count < array_count(entity->ai.navigation_path.path_points)) {
        minimum_count = path_count;
    } else {
        minimum_count = array_count(entity->ai.navigation_path.path_points);
    }

    entity->ai.navigation_path.count = minimum_count;
    for (s32 index = 0; index < minimum_count; ++index) {
        entity->ai.navigation_path.path_points[index] = path_points[index];
    }
}

/*
  NOTE: Game Design notes,

  I want to follow the system set by Disgaea more than FFT, since if the game wasn't focused on stat
  grinding, it would actually be a good tactics game...

  Movement can be undone/redone, but attacks/abilities are permenant actions.
*/

void entity_combat_submit_movement_action(struct entity* entity, v2f32* path_points, s32 path_count) {
    if (entity->ai.current_action != ENTITY_ACTION_NONE) {
        _debugprintf("%.*s shant walk!", entity->name.length, entity->name.data);
        return;
    }
    
    entity_copy_path_array_into_navigation_data(entity, path_points, path_count);

    entity->ai.following_path = true;
    entity->ai.current_action  = ENTITY_ACTION_MOVEMENT;
    /* entity->waiting_on_turn                     = 0; */
    entity_add_used_battle_action(entity, battle_action_movement(entity));

    _debugprintf("Okay... %.*s should walk!", entity->name.length, entity->name.data);
}

void entity_combat_submit_defend_action(struct entity* entity) {
    if (entity->ai.current_action != ENTITY_ACTION_NONE) {
        return;
    }

    entity_add_used_battle_action(entity, battle_action_defend(entity));
}

bool entity_is_in_defense_stance(struct entity* entity) {
    return entity_already_used(entity, LAST_USED_ENTITY_ACTION_DEFEND);
}

void entity_combat_submit_item_use_action(struct entity* entity, s32 item_index, bool from_player_inventory) {
    if (entity->ai.current_action != ENTITY_ACTION_NONE) {
        return;
    }
    /*
      This isn't exactly the same as an action because the results happen "immediately",
      and are not queued.
    */
    entity_add_used_battle_action(entity, battle_action_item_usage(entity, item_index));
    entity->ai.current_action                = ENTITY_ACTION_ITEM;
    entity->ai.used_item_index               = item_index;
    entity->ai.sourced_from_player_inventory = from_player_inventory;
}

void entity_combat_submit_attack_action(struct entity* entity, entity_id target_id) {
    if (entity->ai.current_action != ENTITY_ACTION_NONE) {
        return;
    }

    entity->ai.current_action                      = ENTITY_ACTION_ATTACK;
    entity->ai.attack_target_id                    = target_id;
    entity->waiting_on_turn                        = 0;
    entity->ai.attack_animation_timer              = 0;
    entity->ai.attack_animation_phase              = ENTITY_ATTACK_ANIMATION_PHASE_MOVE_TO_TARGET;
    entity->ai.attack_animation_preattack_position = grid_snapped_v2f32(entity->position);
    {
        struct entity* target_entity = game_dereference_entity(game_state, target_id);
        entity_look_at_and_set_animation_state(entity, target_entity->position);
    }
    announce_battle_action(game_find_id_for_entity(entity), string_literal("Attack"));
    _debugprintf("attacku");
}

void entity_combat_submit_ability_action(struct entity* entity, entity_id* targets, s32 target_count, s32 user_ability_index) {
    if (entity->ai.current_action != ENTITY_ACTION_NONE)
        return;

    entity->ai.current_action = ENTITY_ACTION_ABILITY;

    entity->ai.using_ability_index   = user_ability_index;
    entity->ai.targeted_entity_count = target_count;

    zero_memory(&entity->sequence_state, sizeof(entity->sequence_state));

    entity->sequence_state.start_position        = entity->position;

    for (s32 target_index = 0; target_index < target_count; ++target_index) {
        entity->ai.targeted_entities[target_index] = targets[target_index];
    }

    {
        struct entity_ability* ability = entity_database_ability_find_by_index(&game_state->entity_database, entity->abilities[user_ability_index].ability);
        announce_battle_action(game_find_id_for_entity(entity), ability->name);
    }
    entity->waiting_on_turn = 0;
    _debugprintf("ability");
}

bool entity_validate_death(struct entity* entity) {
    if (entity->health.value <= 0) {
        entity->flags &= ~ENTITY_FLAGS_ALIVE;
        return true;
    }

    entity->flags |= ENTITY_FLAGS_ALIVE;
    return false;
}

void entity_do_healing(struct entity* entity, s32 healing) {
    entity->health.value += healing;
    notify_healing(v2f32_sub(entity->position, v2f32(0, TILE_UNIT_SIZE)), healing);
}

local void entity_on_hurt_finish(struct entity* entity, s32 damage) {
    if (entity->flags & ENTITY_FLAGS_ALIVE) {
        entity->health.value -= damage;

        entity->ai.hurt_animation_timer              = 0;
        entity->ai.hurt_animation_shakes             = 0;
        entity->ai.hurt_animation_phase              = ENTITY_HURT_ANIMATION_ON;

        notify_damage(v2f32_sub(entity->position, v2f32(0, TILE_UNIT_SIZE)), damage);
        (entity_validate_death(entity));
        play_random_hit_sound();
    }
}

void entity_do_physical_hurt(struct entity* entity, s32 damage) {
    /* maybe do a funny animation */
    s32 constitution = entity_find_effective_stat_value(entity, STAT_CONSTITUTION);
    s32 vigor        = entity_find_effective_stat_value(entity, STAT_VIGOR);
    s32 damage_reduction = constitution * 0.33 + vigor * 0.25;

    if (entity_is_in_defense_stance(entity)) {
        damage *= 0.865;
    }

    if (entity_has_any_status_effect_of_type(entity, ENTITY_STATUS_EFFECT_TYPE_IGNITE)) {
        damage *= 1.25;
    }

    damage -= damage_reduction;

    if (damage < 0) {
        damage = 0;
    }

    entity_on_hurt_finish(entity, damage);
}

void entity_do_absolute_hurt(struct entity* entity, s32 damage) {
    entity_on_hurt_finish(entity, damage);
}

/* NOTE: does not really do turns. */
/* set the entity->waiting_on_turn flag to 0 to finish their turn in the combat system. Not done right now cause I need all the actions. */
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

struct entity* decode_sequence_action_target_entity_into_entity(struct game_state* state, struct entity* self, struct sequence_action_target_entity* target) {
    switch (target->entity_target_type) {
        case ENTITY_TARGET_ID_USER: {
            _debugprintf("Fascinating this is a self target.");
            return self;
        } break;
        case ENTITY_TARGET_ID_TARGET: {
            _debugprintf("ENTITY_TARGET_ID_TARGET(%d)", target->entity_target_index); 
            if (target->entity_target_index < 0) {
                target->entity_target_index = self->ai.targeted_entity_count + target->entity_target_index;
            }

            if (target->entity_target_index >= self->ai.targeted_entity_count || target->entity_target_index < 0) {
                _debugprintf("warning: invalid entity handle");
                return NULL;
            } 

            entity_id target_id = self->ai.targeted_entities[target->entity_target_index];
            struct entity* result = game_dereference_entity(state, target_id);
            _debugprintf("Decoded entity target %d (%p)", target->entity_target_index, result);

            return result;
        } break;
    }

    return NULL;
}

local void entity_advance_ability_sequence(struct entity* entity) {
    struct entity_sequence_state* sequence_state      = &entity->sequence_state;
    sequence_state->current_sequence_index += 1;
    sequence_state->time = 0;
    sequence_state->initialized_state = false;
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
            if (target_entity->velocity.x != 0 || target_entity->velocity.y != 0) {
                entity_play_animation_with_direction(target_entity, string_literal("walk"));
            } else {
                entity_play_animation_with_direction(target_entity, string_literal("idle"));
            }

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

        case ENTITY_ACTION_ABILITY: {
            /* woah this list is big, lots of dereferences... */
            struct entity_ai_data*                 ai_data             = &target_entity->ai;
            struct entity_sequence_state*          sequence_state      = &target_entity->sequence_state;
            struct entity_animation_state*         animation           = &target_entity->animation;
            struct entity_ability_slot*            active_ability_slot = &target_entity->abilities[ai_data->using_ability_index];
            assertion(active_ability_slot && "active slot is null? That can't be true!");
            struct entity_ability*                 ability_to_use      = entity_database_ability_find_by_index(&game_state->entity_database, active_ability_slot->ability);
            assertion(ability_to_use      && "Ability not found??? That can't be true!");
            s32                                    target_count        = ai_data->targeted_entity_count;
            entity_id*                             targets             = ai_data->targeted_entities;
            struct entity_ability_sequence*        ability_sequence    = &ability_to_use->sequence;
            assertion(ability_sequence    && "The sequence is bad? That can't be true!");
            struct entity_ability_sequence_action* sequence_action     = &ability_sequence->sequence_actions[sequence_state->current_sequence_index];

            assertion(sequence_action     && "The action is bad?   That can't be true!");
            _debugprintf("Current sequence index: %d / %d", sequence_state->current_sequence_index+1, ability_sequence->sequence_action_count);

            bool should_continue = true;

            if (sequence_action->type != SEQUENCE_ACTION_REQUIRE_BLOCK) {
                struct sequence_action_require_block requirements = sequence_state->current_requirements;
                if (requirements.needed) {
                    _debugprintf("checking execution requirements");
                    _debugprintf("Needs %d entities to be present", requirements.required_entity_count);
                    for (s32 required_entity_index = 0; required_entity_index < requirements.required_entity_count && should_continue; ++required_entity_index) {
                        struct sequence_action_target_entity* current_target_entity_handle = requirements.required_entities + required_entity_index;
                        struct entity* actual_entity = decode_sequence_action_target_entity_into_entity(game_state, target_entity, current_target_entity_handle);

                        if (!actual_entity) {
                            _debugprintf("Missing entity! Skipping!");
                            should_continue = false;
                            break;
                        }
                    }
                } else {
                    _debugprintf("No entity needed");
                }
            }

            if (!should_continue) {
                entity_advance_ability_sequence(target_entity);
            } else {
                switch (sequence_action->type) {
                    case SEQUENCE_ACTION_APPLY_STATUS: {
                        struct sequence_action_apply_status* apply_status               = &sequence_action->apply_status;
                        struct entity*                       apply_status_target_entity = decode_sequence_action_target_entity_into_entity(state, target_entity, &apply_status->target);

                        /* probably redundant but whatever, it works right now */
                        struct entity_status_effect actual_effect;
                        switch (apply_status->effect.type) {
                            case ENTITY_STATUS_EFFECT_TYPE_IGNITE: {
                                actual_effect = status_effect_ignite(apply_status->effect.turn_duration);
                            } break;
                            case ENTITY_STATUS_EFFECT_TYPE_POISON: {
                                actual_effect = status_effect_poison(apply_status->effect.turn_duration);
                            } break;
                        }

                        entity_add_status_effect(apply_status_target_entity, actual_effect);
                        entity_advance_ability_sequence(target_entity);
                    } break;
                    case SEQUENCE_ACTION_CAMERA_TRAUMA: {
                        struct sequence_action_camera_trauma* camera_trauma = &sequence_action->camera_trauma;
                        camera_traumatize(&game_state->camera, camera_trauma->amount);
                        entity_advance_ability_sequence(target_entity);
                    } break;
                    case SEQUENCE_ACTION_LOOK_AT: {
                        struct sequence_action_look_at* look_at = &sequence_action->look_at;
                        switch (look_at->look_target_type) {
                            case LOOK_TARGET_TYPE_RELATIVE_DIRECTION: {
                                unimplemented("LOOK_TARGET_TYPE_RELATIVE_DIRECTION");
                            } break;
                            case LOOK_TARGET_TYPE_ENTITY: {
                                struct entity* look_target_entity = decode_sequence_action_target_entity_into_entity(state, target_entity, &look_at->look_target);
                                /* _debugprintf("Trying to look at entity %p looktarget, %p self", target_entity, look_target_entity); */
                                entity_look_at_and_set_animation_state(target_entity, look_target_entity->position);
                            } break;
                            case LOOK_TARGET_TYPE_DIRECTION: {
                                unimplemented("LOOK_TARGET_TYPE_DIRECTION");
                            } break;
                        }
                        entity_advance_ability_sequence(target_entity);
                    } break;
                    case SEQUENCE_ACTION_MOVE_TO: {
                        struct sequence_action_move_to* move_to = &sequence_action->move_to;

                        /* NOTE: This is in pixel positions. Kind of confusing, I know... I'm not very consistent with these honestly... I should rectify this later. */
                        if (!sequence_state->initialized_state) {
                            sequence_state->start_position_interpolation = target_entity->position;
                            _debugprintf("Starting interpolation from: %f, %f", sequence_state->start_position_interpolation.x, sequence_state->start_position_interpolation.y);

                            v2f32 end_position = v2f32(0,0);
                            {
                                switch (move_to->move_target_type) {
                                    case MOVE_TARGET_RELATIVE_DIRECTION_TO: {
                                        struct entity* move_target_entity = decode_sequence_action_target_entity_into_entity(state, target_entity, &move_to->move_target.entity.target);
                                        _debugprintf("%p(move_target_entity) %p(self) move target entity", move_target_entity, target_entity);

                                        v2f32 direction_between_target_and_self = v2f32_sub(move_target_entity->position, target_entity->position);
                                        direction_between_target_and_self       = v2f32_normalize(direction_between_target_and_self);
                                        _debugprintf("asdf %f, %f (from %f, %f, vs %f, %f)", direction_between_target_and_self.x, direction_between_target_and_self.y, target_entity->position.x, target_entity->position.y, move_target_entity->position.x, move_target_entity->position.y);

                                        v2f32 displacement_offset = v2f32_scale(direction_between_target_and_self, move_to->move_target.entity.move_past * TILE_UNIT_SIZE);
                                        end_position = v2f32_add(target_entity->position, displacement_offset);
                                    } break;
                                    case MOVE_TARGET_ENTITY: {
                                        struct entity* move_target_entity = decode_sequence_action_target_entity_into_entity(state, target_entity, &move_to->move_target.entity.target);

                                        v2f32 direction_between_target_and_self = v2f32_sub(move_target_entity->position, target_entity->position);
                                        direction_between_target_and_self       = v2f32_normalize(direction_between_target_and_self);
                                        _debugprintf("easdf %f, %f (from %f, %f, vs %f, %f)", direction_between_target_and_self.x, direction_between_target_and_self.y, target_entity->position.x, target_entity->position.y, move_target_entity->position.x, move_target_entity->position.y);

                                        v2f32 displacement_offset = v2f32_scale(direction_between_target_and_self, move_to->move_target.entity.move_past * TILE_UNIT_SIZE);
                                        end_position = v2f32_add(move_target_entity->position, displacement_offset);
                                    } break;
                                    case MOVE_TARGET_START: {
                                        end_position = sequence_state->start_position;
                                    } break;
                                }
                            }

                            sequence_state->end_position_interpolation = end_position;
                            _debugprintf("Ending interpolation at: %f, %f", sequence_state->end_position_interpolation.x, sequence_state->end_position_interpolation.y);
                            sequence_state->initialized_state          = true;
                        } else {
                            if (sequence_state->time >= 1.0) {
                                _debugprintf("Okay I'm at : %f, %f", target_entity->position.x, target_entity->position.y);
                                target_entity->position.x = sequence_state->end_position_interpolation.x;
                                target_entity->position.y = sequence_state->end_position_interpolation.y;
                                entity_advance_ability_sequence(target_entity);
                            } else {
                                /* Let's rock! */
                                f32 effective_t = sequence_state->time;
                                if (effective_t >= 1) {
                                    effective_t = 1.0f;
                                }

                                switch (move_to->interpolation_type) {
                                    case SEQUENCE_LINEAR: {
                                        target_entity->position.x = lerp_f32(sequence_state->start_position_interpolation.x, sequence_state->end_position_interpolation.x, effective_t);
                                        target_entity->position.y = lerp_f32(sequence_state->start_position_interpolation.y, sequence_state->end_position_interpolation.y, effective_t);
                                    } break;
                                    case SEQUENCE_CUBIC_EASE_IN: {
                                        target_entity->position.x = cubic_ease_in_f32(sequence_state->start_position_interpolation.x, sequence_state->end_position_interpolation.x, effective_t);
                                        target_entity->position.y = cubic_ease_in_f32(sequence_state->start_position_interpolation.y, sequence_state->end_position_interpolation.y, effective_t);
                                    } break;
                                    case SEQUENCE_CUBIC_EASE_OUT: {
                                        target_entity->position.x = cubic_ease_out_f32(sequence_state->start_position_interpolation.x, sequence_state->end_position_interpolation.x, effective_t);
                                        target_entity->position.y = cubic_ease_out_f32(sequence_state->start_position_interpolation.y, sequence_state->end_position_interpolation.y, effective_t);
                                    } break;
                                    case SEQUENCE_CUBIC_EASE_IN_OUT: {
                                        target_entity->position.x = cubic_ease_in_out_f32(sequence_state->start_position_interpolation.x, sequence_state->end_position_interpolation.x, effective_t);
                                        target_entity->position.y = cubic_ease_in_out_f32(sequence_state->start_position_interpolation.y, sequence_state->end_position_interpolation.y, effective_t);
                                    } break;
                                    case SEQUENCE_QUADRATIC_EASE_IN: {
                                        target_entity->position.x = quadratic_ease_in_f32(sequence_state->start_position_interpolation.x, sequence_state->end_position_interpolation.x, effective_t);
                                        target_entity->position.y = quadratic_ease_in_f32(sequence_state->start_position_interpolation.y, sequence_state->end_position_interpolation.y, effective_t);
                                    } break;
                                    case SEQUENCE_QUADRATIC_EASE_OUT: {
                                        target_entity->position.x = quadratic_ease_out_f32(sequence_state->start_position_interpolation.x, sequence_state->end_position_interpolation.x, effective_t);
                                        target_entity->position.y = quadratic_ease_out_f32(sequence_state->start_position_interpolation.y, sequence_state->end_position_interpolation.y, effective_t);
                                    } break;
                                    case SEQUENCE_QUADRATIC_EASE_IN_OUT: {
                                        target_entity->position.x = quadratic_ease_in_out_f32(sequence_state->start_position_interpolation.x, sequence_state->end_position_interpolation.x, effective_t);
                                        target_entity->position.y = quadratic_ease_in_out_f32(sequence_state->start_position_interpolation.y, sequence_state->end_position_interpolation.y, effective_t);
                                    } break;
                                    case SEQUENCE_BACK_EASE_IN: {
                                        target_entity->position.x = ease_in_back_f32(sequence_state->start_position_interpolation.x, sequence_state->end_position_interpolation.x, effective_t);
                                        target_entity->position.y = ease_in_back_f32(sequence_state->start_position_interpolation.y, sequence_state->end_position_interpolation.y, effective_t);
                                    } break;
                                    case SEQUENCE_BACK_EASE_OUT: {
                                        target_entity->position.x = ease_out_back_f32(sequence_state->start_position_interpolation.x, sequence_state->end_position_interpolation.x, effective_t);
                                        target_entity->position.y = ease_out_back_f32(sequence_state->start_position_interpolation.y, sequence_state->end_position_interpolation.y, effective_t);
                                    } break;
                                    case SEQUENCE_BACK_EASE_IN_OUT: {
                                        target_entity->position.x = ease_in_out_back_f32(sequence_state->start_position_interpolation.x, sequence_state->end_position_interpolation.x, effective_t);
                                        target_entity->position.y = ease_in_out_back_f32(sequence_state->start_position_interpolation.y, sequence_state->end_position_interpolation.y, effective_t);
                                    } break;
                                }

                                /* TODO: should allow velocity to be specced else where */
                                /* or some other speed metric, since this is too limiting */
                                f32 effective_step = 1;
                                {
                                    const f32 DESIRED_VELOCITY     = move_to->desired_velocity_magnitude * TILE_UNIT_SIZE; 
                                    f32       distance_to_position = v2f32_distance(sequence_state->start_position_interpolation, sequence_state->end_position_interpolation);
                                    effective_step                 = DESIRED_VELOCITY / distance_to_position;
                                }

                                sequence_state->time += effective_step * dt;

                                if (sequence_state->time >= 1.0) {
                                    sequence_state->time = 1;
                                }
                            }
                        }
                    } break;

                    case SEQUENCE_ACTION_FOCUS_CAMERA: {
                        struct sequence_action_focus_camera* focus_camera = &sequence_action->focus_camera;
                        struct entity* focus_entity = decode_sequence_action_target_entity_into_entity(state, target_entity, &focus_camera->target);

                        battle_ui_stalk_entity_with_camera(focus_entity);
                        entity_advance_ability_sequence(target_entity);
                    } break;

                    case SEQUENCE_ACTION_HURT: {
                        struct sequence_action_hurt* hurt_sequence = &sequence_action->hurt;

                        s32 entities_to_hurt = 0;
                        entity_id* attacking_entity_ids = memory_arena_push(&scratch_arena, 0);

                        if (hurt_sequence->hurt_target_flags) {
                            if (hurt_sequence->hurt_target_flags & HURT_TARGET_FLAG_ALL_SELECTED) {
                                for (s32 target_index = 0; target_index < target_entity->ai.targeted_entity_count; ++target_index) {
                                    memory_arena_push(&scratch_arena, sizeof(*attacking_entity_ids));
                                    entity_id targeted_entity_id = target_entity->ai.targeted_entities[target_index];
                                    attacking_entity_ids[entities_to_hurt++] = targeted_entity_id; 
                                }
                            } else if (hurt_sequence->hurt_target_flags & HURT_TARGET_FLAG_EVERY_ENEMY) {
                                struct game_state_combat_state* combat_state = &state->combat_state;
                                for (s32 index = combat_state->active_combatant; index < combat_state->count; ++index) {
                                    struct entity* entity = game_dereference_entity(state, combat_state->participants[index]);

                                    if (entity->flags & ENTITY_FLAGS_PLAYER_CONTROLLED) {
                                        continue;
                                    } else {
                                        if (entity->ai.flags & ENTITY_AI_FLAGS_AGGRESSIVE_TO_PLAYER) {
                                            memory_arena_push(&scratch_arena, sizeof(*attacking_entity_ids));
                                            attacking_entity_ids[entities_to_hurt++] = combat_state->participants[index];
                                        }
                                    }
                                }
                            }
                        } else {
                            for (s32 target_index = 0; target_index < hurt_sequence->target_count; ++target_index) {
                                s32 proposed_target_index = hurt_sequence->targets[target_index].entity_target_index;
                                if (proposed_target_index >= target_entity->ai.targeted_entity_count) {
                                    break;
                                }
                                entity_id attacked_target_id = target_entity->ai.targeted_entities[proposed_target_index];
                                memory_arena_push(&scratch_arena, sizeof(*attacking_entity_ids));
                                attacking_entity_ids[entities_to_hurt++] = attacked_target_id;
                            }
                        }

                        for (s32 entity_to_hurt_index = 0; entity_to_hurt_index < entities_to_hurt; ++entity_to_hurt_index) {
                            entity_id      entity_to_hurt_id = attacking_entity_ids[entity_to_hurt_index];
                            struct entity* entity_to_hurt    = game_dereference_entity(game_state, entity_to_hurt_id);

                            /* TODO: REPLACE */
                            entity_do_physical_hurt(entity_to_hurt, 2500);
                            battle_notify_killed_entity(entity_to_hurt_id);
                        }
                        entity_advance_ability_sequence(target_entity);
                    } break;

                    case SEQUENCE_ACTION_START_SPECIAL_FX: {
                        struct sequence_action_special_fx* special_fx = &sequence_action->special_fx;
                        s32 effect_id = special_fx->effect_id; 
                        handle_specialfx_sequence_action(effect_id);
                        entity_advance_ability_sequence(target_entity);
                    } break;

                    case SEQUENCE_ACTION_DO_HARDCODED_ANIM: {
                        struct sequence_action_hardcoded_animation* hardcoded_anim = &sequence_action->hardcoded_anim;
                        s32 effect_id = hardcoded_anim->id;
                        handle_hardcoded_animation_sequence_action(effect_id, entity);
                    } break;

                    case SEQUENCE_ACTION_STOP_SPECIAL_FX: {
                        special_effect_stop_effects();
                        entity_advance_ability_sequence(target_entity);
                    } break;

                    case SEQUENCE_ACTION_WAIT_SPECIAL_FX_TO_FINISH: {
                        if (special_effects_active()) {
                            /*...*/
                        } else {
                            entity_advance_ability_sequence(target_entity);
                        }
                    } break;

                    case SEQUENCE_ACTION_EXPLOSION: {
                        struct sequence_action_explosion* explosion = &sequence_action->explosion;
                        struct entity* start_explosion_at           = decode_sequence_action_target_entity_into_entity(game_state, target_entity, &explosion->where_to_explode);
                        f32 explosion_radius                        = explosion->explosion_radius;
                        s32 explosion_damage                        = explosion->explosion_damage;
                        s32 explosion_effect_id                     = explosion->explosion_effect_id;

                        game_produce_damaging_explosion(v2f32_scale(start_explosion_at->position, 1.0/TILE_UNIT_SIZE), explosion_radius, explosion_effect_id, explosion_damage, 0, 0);
                        entity_advance_ability_sequence(target_entity);
                    } break;

                    case SEQUENCE_ACTION_REQUIRE_BLOCK: {
                        struct sequence_action_require_block* requirements = &sequence_action->require_block;
                        sequence_state->current_requirements               = *requirements;
                        entity_advance_ability_sequence(target_entity);
                    } break;

                    default: {
                        entity_advance_ability_sequence(target_entity);
                    } break;
                }
            }

            if (sequence_state->current_sequence_index >= ability_sequence->sequence_action_count) {
                _debugprintf("Ability sequence complete");
                target_entity->ai.current_action = 0; 
                battle_ui_stop_stalk_entity_with_camera();
            }
        } break;

        case ENTITY_ACTION_ATTACK: {
            struct entity* attacked_entity = game_dereference_entity(state, target_entity->ai.attack_target_id);

            switch (target_entity->ai.attack_animation_phase) {
                case ENTITY_ATTACK_ANIMATION_PHASE_MOVE_TO_TARGET: {
                    v2f32     position_delta       = v2f32_sub(attacked_entity->position, target_entity->position);
                    v2f32     direction            = v2f32_direction(attacked_entity->position, target_entity->position);
                    /* TODO: THIS SHOULD VARY */
                    const f32 TARGET_MOVE_VELOCITY = TILE_UNIT_SIZE*3.5;

                    if (v2f32_magnitude(position_delta) < TILE_UNIT_SIZE * 0.85) {
                        target_entity->ai.attack_animation_phase = ENTITY_ATTACK_ANIMATION_PHASE_REEL_BACK;
                        target_entity->ai.attack_animation_timer = 0;
                        target_entity->ai.attack_animation_interpolation_start_position = target_entity->position;
                        target_entity->ai.attack_animation_interpolation_end_position = v2f32_add(target_entity->position, v2f32_scale(direction, TILE_UNIT_SIZE/0.8));
                    } else {
                        target_entity->position.x += -TARGET_MOVE_VELOCITY*dt * direction.x;
                        target_entity->position.y += -TARGET_MOVE_VELOCITY*dt * direction.y;
                    }
                } break;
                case ENTITY_ATTACK_ANIMATION_PHASE_REEL_BACK: {
                    f32 MAX_T       = 0.457;
                    f32 effective_t = target_entity->ai.attack_animation_timer/MAX_T;

                    if (effective_t > 1) effective_t = 1;

                    if (target_entity->ai.attack_animation_timer >= MAX_T) {
                        target_entity->ai.attack_animation_timer = 0;
                        target_entity->ai.attack_animation_phase = ENTITY_ATTACK_ANIMATION_PHASE_HIT;

                        v2f32 direction = v2f32_direction(attacked_entity->position, target_entity->position);

                        target_entity->ai.attack_animation_interpolation_start_position = target_entity->position;
                        target_entity->ai.attack_animation_interpolation_end_position = v2f32_sub(target_entity->position, v2f32_scale(direction, TILE_UNIT_SIZE/0.6));
                    } else {
                        target_entity->position.x = lerp_f32(target_entity->ai.attack_animation_interpolation_start_position.x, target_entity->ai.attack_animation_interpolation_end_position.x, effective_t);
                        target_entity->position.y = lerp_f32(target_entity->ai.attack_animation_interpolation_start_position.y, target_entity->ai.attack_animation_interpolation_end_position.y, effective_t);
                    }

                    target_entity->ai.attack_animation_timer += dt;
                } break;
                case ENTITY_ATTACK_ANIMATION_PHASE_HIT: {
                    f32 MAX_T       = 0.0735;
                    f32 effective_t = target_entity->ai.attack_animation_timer/MAX_T;

                    if (effective_t > 1) effective_t = 1;

                    target_entity->position.x = lerp_f32(target_entity->ai.attack_animation_interpolation_start_position.x, target_entity->ai.attack_animation_interpolation_end_position.x, effective_t);
                    target_entity->position.y = lerp_f32(target_entity->ai.attack_animation_interpolation_start_position.y, target_entity->ai.attack_animation_interpolation_end_position.y, effective_t);

                    if (target_entity->ai.attack_animation_timer >= MAX_T) {
                        target_entity->ai.attack_animation_timer = 0;
                        target_entity->ai.attack_animation_phase = ENTITY_ATTACK_ANIMATION_PHASE_RECOVER_FROM_HIT;

                        {
                            s32 damage = entity_get_physical_damage(target_entity);
                            entity_do_physical_hurt(attacked_entity, damage);

                            {
                                if (attacked_entity->health.value <= 0) {
                                    if (target_entity->flags & ENTITY_FLAGS_PLAYER_CONTROLLED) {
                                        /* maybe not a good idea right now, but okay. */
                                        battle_notify_killed_entity(target_entity->ai.attack_target_id);
                                    }
                                } else {
                                    /* TODO more involved invariants for counter attacking */
                                    struct entity* targeted_entity = game_dereference_entity(game_state, target_entity->ai.attack_target_id);

                                    if (targeted_entity->ai.used_counter_attacks < entity_find_effective_stat_value(targeted_entity, STAT_COUNTER)) {
                                        add_counter_attack_entry(target_entity->ai.attack_target_id, game_find_id_for_entity(target_entity));
                                        targeted_entity->ai.used_counter_attacks += 1;
                                    }
                                }
                            }
                        }

                        target_entity->ai.attack_animation_interpolation_start_position = target_entity->position;
                        target_entity->ai.attack_animation_interpolation_end_position   = target_entity->ai.attack_animation_preattack_position;
                    }

                    target_entity->ai.attack_animation_timer += dt;
                } break;
                case ENTITY_ATTACK_ANIMATION_PHASE_RECOVER_FROM_HIT: {
                    f32 MAX_T       = 0.3843;
                    f32 effective_t = (target_entity->ai.attack_animation_timer-0.089)/MAX_T;

                    if (effective_t > 1) effective_t = 1;
                    if (effective_t < 0) effective_t = 0;

                    if (target_entity->ai.attack_animation_timer >= MAX_T) {
                        target_entity->position = grid_snapped_v2f32(target_entity->ai.attack_animation_preattack_position);
                        target_entity->ai.current_action = 0;
                        /* TODO snap to grid valid position to make sure no bugs happen */
                    } else {
                        target_entity->position.x = lerp_f32(target_entity->ai.attack_animation_interpolation_start_position.x, target_entity->ai.attack_animation_interpolation_end_position.x, effective_t);
                        target_entity->position.y = lerp_f32(target_entity->ai.attack_animation_interpolation_start_position.y, target_entity->ai.attack_animation_interpolation_end_position.y, effective_t);
                    }

                    target_entity->ai.attack_animation_timer += dt;
                } break;
            }
        } break;

        case ENTITY_ACTION_ITEM: {
            s32  item_index            = target_entity->ai.used_item_index;
            bool from_player_inventory = target_entity->ai.sourced_from_player_inventory;

            struct entity_inventory* inventory = NULL;
            if (from_player_inventory) {
                inventory = (struct entity_inventory*)&game_state->inventory;
            } else {
                inventory = (struct entity_inventory*)&target_entity->inventory;
            }

            /* struct item_instance* current_item_instance = inventory->items + item_use->selectable_items[item_index]; */
            /* struct item_def*      item_base             = item_database_find_by_id(current_item_instance->item); */
            entity_inventory_use_item(inventory, item_index, target_entity);
            target_entity->ai.current_action = 0;
        } break;

            bad_case;
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

    if (stat_index == STAT_COUNTER) {
        if (entity_is_in_defense_stance(entity)) {
            base_value /= 2;
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
    struct file_buffer   data_file_buffer = read_entire_file(memory_arena_allocator(&scratch_arena), string_literal(GAME_DEFAULT_LOOT_TABLES_FILE));
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
    struct file_buffer   data_file_buffer = read_entire_file(memory_arena_allocator(&scratch_arena), string_literal(GAME_DEFAULT_ENTITY_DEF_FILE));
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

    _debugprintf("NOTE: ability count : %d", database->ability_count);
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
            struct lisp_form* ability_block_form    = lisp_list_nth(current_entity_form, 11);

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
                        } else if (lisp_form_symbol_matching(*stat_name, string_literal("counter"))) {
                            lisp_form_get_s32(*param, &current_entity->stats.counter);
                        }
                    }
                }
            }

            {
                _debugprintf("Ability list form count: %d", ability_block_form->list.count);
                for (s32 ability_block_form_index = 0; ability_block_form_index < ability_block_form->list.count; ++ability_block_form_index) {
                    struct lisp_form* ability_form        = lisp_list_nth(ability_block_form, ability_block_form_index);
                    {
                        struct lisp_form* ability_string_form = lisp_list_nth(ability_form, 0);
                        struct lisp_form* ability_level_form  = lisp_list_nth(ability_form, 1);
                        string            ability_string_name = {};
                        s32               ability_usage_level = 0;
                        assertion(lisp_form_get_string(*ability_string_form, &ability_string_name) && "Ability string name not found?");
                        assertion(lisp_form_get_s32(*ability_level_form, &ability_usage_level) && "Ability usage level not found?");
                        s32                                   new_ability  = entity_database_ability_find_id_by_name(database, ability_string_name);
                        _debugprintf("%.*s trying to find ability", ability_string_name.length, ability_string_name.data);
                        struct entity_base_data_ability_slot* current_slot = &current_entity->abilities[current_entity->ability_count++];
                        current_slot->ability                              = new_ability;
                        current_slot->level                                = ability_usage_level;
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
    struct lisp_list     ability_forms     = lisp_read_entire_file_into_forms(&scratch_arena, string_literal(GAME_DEFAULT_ENTITY_ABILITY_FILE));

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
            s32 outer_form_index = 0;
            struct lisp_form* id_name_form                  = lisp_list_nth(current_ability_form, outer_form_index++);
            struct lisp_form* name_form                     = lisp_list_nth(current_ability_form, outer_form_index++);
            struct lisp_form* description_form              = lisp_list_nth(current_ability_form, outer_form_index++);
            struct lisp_form* requires_no_obstructions_form = lisp_list_nth(current_ability_form, outer_form_index++);
            struct lisp_form* ability_class_id              = lisp_list_nth(current_ability_form, outer_form_index++);
            struct lisp_form* innate_ability_flag           = lisp_list_nth(current_ability_form, outer_form_index++);
            {
                string id_name_string     = {};
                string name_string        = {};
                string description_string = {};

                assertion(lisp_form_get_string(*id_name_form, &id_name_string) && "ability format needs nameid string!");
                assertion(lisp_form_get_string(*name_form, &name_string) && "ability format needs name string");
                assertion(lisp_form_get_string(*description_form, &description_string) && "ability format needs description string");
                assertion(lisp_form_get_boolean(*requires_no_obstructions_form, &current_ability->requires_no_obstructions) && "ability needs requires_no_obstruction flag");
                assertion(lisp_form_get_s32(*ability_class_id, &current_ability->item_class_group) && "ability format requires item_class_group s32");
                assertion(lisp_form_get_boolean(*innate_ability_flag, &current_ability->innate) && "ability format requires innate flag");

                _debugprintf("No obstructions? : %d", current_ability->requires_no_obstructions);
                _debugprintf("Innate?: %d", current_ability->innate);
                _debugprintf("Ability Class ID?: %d", current_ability->item_class_group);

                id_name_string     = string_clone(arena, id_name_string);
                name_string        = string_clone(arena, name_string);
                description_string = string_clone(arena, description_string);

                current_ability->id_name     = id_name_string;
                current_ability->name        = name_string;
                current_ability->description = description_string;
                *current_ability_key_string      = id_name_string;
            }

            struct lisp_form* selection_form               = lisp_list_nth(current_ability_form, outer_form_index++);
            struct lisp_form* field_form                   = lisp_list_nth(current_ability_form, outer_form_index++);
            struct lisp_form* animation_sequence_list_form = lisp_list_nth(current_ability_form, outer_form_index++);

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

    _debugprintf("INIT LOOTTABLES");
    entity_database_initialize_loot_tables(&result);
    _debugprintf("INIT ABILITIES");
    entity_database_initialize_abilities(&result);
    _debugprintf("INIT ENTITIES");
    entity_database_initialize_entities(&result);

    return result;
}

void level_area_entity_savepoint_unpack(struct level_area_savepoint* savepoint, struct entity_savepoint* unpack_target) {
    unpack_target->position = savepoint->position;
    unpack_target->flags   |= savepoint->flags;
    entity_savepoint_initialize(unpack_target);
}

bool entity_has_dialogue(struct entity* entity) {
    return cstring_length(entity->dialogue_file) > 0;
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
    _debugprintf("scriptfile %s, %s\n", entity->script_name, entity->dialogue_file);
    cstring_copy(entity->script_name, unpack_target->script_name, array_count(unpack_target->script_name));
    cstring_copy(entity->dialogue_file, unpack_target->dialogue_file, array_count(unpack_target->dialogue_file));

    if(cstring_length(entity->dialogue_file)) {
        unpack_target->has_dialogue = true;
    }

    if (entity->health_override != -1) {unpack_target->health.value = entity->health_override;}
    if (entity->magic_override != -1)  {unpack_target->magic.value  = entity->magic_override;}


    /* I should really use IDs more. Oh well. Who knows how many lines of redundant dereferencing code I have in this codebase. */
    struct entity_base_data* base_data = entity_database_find_by_name(&game_state->entity_database, string_from_cstring(entity->base_name));
    entity_base_data_unpack(&game_state->entity_database, base_data, unpack_target);
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

    destination->ability_count = data->ability_count;
    for (s32 ability_index = 0; ability_index < data->ability_count; ++ability_index) {
        struct entity_ability_slot*           current_ability_slot      = destination->abilities + ability_index;
        struct entity_base_data_ability_slot* current_base_ability_slot = data->abilities + ability_index;
        current_ability_slot->usages                                    = current_base_ability_slot->level;
        current_ability_slot->ability                                   = current_base_ability_slot->ability;
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
    if (list && list->entities) {
        iterator->entity_lists[iterator->list_count++] = list;
    }
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
    struct entity_list* current_iteration_list  ;
    struct entity*      current_iterated_entity ;
    iterator->current_id                        ;

    do {
        current_iteration_list  = iterator->entity_lists[iterator->index];
        current_iterated_entity = current_iteration_list->entities + iterator->entity_list_index;
        iterator->current_id    = entity_list_get_id(current_iteration_list, iterator->entity_list_index);

        iterator->entity_list_index += 1;

        if (iterator->entity_list_index > current_iteration_list->capacity) {
            iterator->index += 1;
            iterator->entity_list_index = 0;
        }

        if (iterator->index >= iterator->list_count) {
            iterator->done = true;
        }
    } while (!(current_iterated_entity->flags & ENTITY_FLAGS_ACTIVE) && !iterator->done);

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

struct position_marker* position_marker_list_find_marker_with_name(struct position_marker_list* list, string name) {
    for (s32 marker_index = 0; marker_index < list->count; ++marker_index) {
        struct position_marker* marker = list->markers + marker_index;

        if (string_equal(string_from_cstring(marker->name), name)) {
            return marker;
        }
    }

    return NULL;
}

struct position_marker* position_marker_list_find_marker_at_with_rect(struct position_marker_list* list, v2f32 where) {
    for (s32 marker_index = 0; marker_index < list->count; ++marker_index) {
        struct position_marker* marker = list->markers + marker_index;

        if (rectangle_f32_intersect(rectangle_f32(where.x, where.y, 0.05, 0.05), rectangle_f32(marker->position.x, marker->position.y, 1, 1))) {
            return marker;
        }
    }

    return NULL;
}

struct position_marker* position_marker_list_find_marker_at(struct position_marker_list* list, v2f32 where) {
    for (s32 marker_index = 0; marker_index < list->count; ++marker_index) {
        struct position_marker* marker = list->markers + marker_index;

        if (marker->position.x == where.x && marker->position.y == where.y) {
            return marker;
        }
    }

    return NULL;
}

void serialize_world_location(struct binary_serializer* serializer, s32 version, struct world_location* location) {
    /* version is a world map version */
    switch (version) {
        default:
        case 3: {
            serialize_f32(serializer,   &location->position.x);
            serialize_f32(serializer,   &location->position.y);
            serialize_f32(serializer,   &location->scale.x);
            serialize_f32(serializer,   &location->scale.y);
            serialize_u32(serializer,   &location->flags);
            serialize_bytes(serializer, location->preview_name, array_count(location->preview_name));
            {
                serialize_bytes(serializer, location->entrance.area_name, array_count(location->entrance.area_name));
                serialize_s8(serializer,    &location->entrance.facing_direction);
                serialize_f32(serializer,   &location->entrance.where.x);
                serialize_f32(serializer,   &location->entrance.where.y);
            }
        } break;
    }
}

void serialize_position_marker(struct binary_serializer* serializer, s32 version, struct position_marker* marker) {
    marker->scale = v2f32(1,1);
    switch (version) {
        case 13: {
            serialize_f32(serializer, &marker->position.x);
            serialize_f32(serializer, &marker->position.y);
            serialize_bytes(serializer, marker->name, array_count(marker->name));
        } break;
        default: {
            unimplemented("No support for position markers yet");
        } break;
    }
}

void serialize_trigger_level_transition(struct binary_serializer* serializer, s32 version, struct trigger_level_transition* trigger) {
    switch (version) {
        default: {
            serialize_f32(serializer,   &trigger->bounds.x);
            serialize_f32(serializer,   &trigger->bounds.y);

            serialize_f32(serializer,   &trigger->bounds.w);
            serialize_f32(serializer,   &trigger->bounds.h);

            serialize_bytes(serializer, trigger->target_level, 128);

            serialize_u8(serializer,    &trigger->new_facing_direction); /* should not be here */
            /* padding bytes */
            serialize_u8(serializer, 0);
            serialize_u8(serializer, 0);
            serialize_u8(serializer, 0);

            serialize_f32(serializer,   &trigger->spawn_location.x);
            serialize_f32(serializer,   &trigger->spawn_location.y);
        } break;
        case CURRENT_LEVEL_AREA_VERSION: {
            serialize_f32(serializer,   &trigger->bounds.x);
            serialize_f32(serializer,   &trigger->bounds.y);

            serialize_f32(serializer,   &trigger->bounds.w);
            serialize_f32(serializer,   &trigger->bounds.h);

            serialize_bytes(serializer, trigger->target_level, 128);

            serialize_u8(serializer,    &trigger->new_facing_direction); /* should not be here */
            serialize_u8(serializer,    &trigger->type);

            serialize_f32(serializer,   &trigger->spawn_location.x);
            serialize_f32(serializer,   &trigger->spawn_location.y);
        } break;
    }
}
void serialize_entity_chest(struct binary_serializer* serializer, s32 version, struct entity_chest* chest) {
    switch (version) {
        default:
        case CURRENT_LEVEL_AREA_VERSION: {
            serialize_f32(serializer,   &chest->position.x);
            serialize_f32(serializer,   &chest->position.y);
            /* should be gone */
            serialize_f32(serializer,   &chest->scale.x);
            serialize_f32(serializer,   &chest->scale.y);
            serialize_u32(serializer,   &chest->flags);
            serialize_s32(serializer,   &chest->inventory.item_count);
            for (s32 index = 0; index < chest->inventory.item_count; ++index) {
                serialize_u32(serializer,   &chest->inventory.items[index].item.id_hash);
                serialize_s32(serializer,   &chest->inventory.items[index].count);
            }
            serialize_u32(serializer,   &chest->key_item.id_hash); /* should not be here */
        } break;
    }
}
void serialize_generic_trigger(struct binary_serializer* serializer, s32 version, struct trigger* trigger) {
    switch (version) {
        default:
        case CURRENT_LEVEL_AREA_VERSION: {
            serialize_f32(serializer, &trigger->bounds.x);
            serialize_f32(serializer, &trigger->bounds.y);
            serialize_f32(serializer, &trigger->bounds.w);
            serialize_f32(serializer, &trigger->bounds.h);
            serialize_u32(serializer, &trigger->activations); /* should not be here */
            serialize_u8(serializer, &trigger->active); /* should be a flags bitfield */
            /* padding */
            serialize_u8(serializer, 0);
            serialize_u8(serializer, 0);
            serialize_u8(serializer, 0);
            serialize_bytes(serializer, trigger->unique_name, 32);
        } break;
    }
}

void serialize_level_area_entity_savepoint(struct binary_serializer* serializer, s32 version, struct level_area_savepoint* entity) {
    switch (version) {
        default:
        case CURRENT_LEVEL_AREA_VERSION: {
            serialize_f32(serializer, &entity->position.x);
            serialize_f32(serializer, &entity->position.y);
            serialize_u32(serializer, &entity->flags);
        } break;
    }
}
void serialize_battle_safe_square(struct binary_serializer* serializer, s32 version, struct level_area_battle_safe_square* square) {
    switch (version) {
        default:
        case CURRENT_LEVEL_AREA_VERSION: {
            serialize_s32(serializer, &square->x);
            serialize_s32(serializer, &square->y);
        } break;
    }
}

void serialize_light(struct binary_serializer* serializer, s32 version, struct light_def* light) {
    switch (version) {
        case 9: {
            serialize_f32(serializer, &light->position.x);
            serialize_f32(serializer, &light->position.y);
            serialize_f32(serializer, &light->scale.x);
            serialize_f32(serializer, &light->scale.y);
            serialize_u8(serializer,  &light->color.r);
            serialize_u8(serializer,  &light->color.g);
            serialize_u8(serializer,  &light->color.b);
            serialize_u8(serializer,  &light->color.a);
            serialize_u32(serializer, &light->flags);
            serialize_u8(serializer, 0);
            serialize_u8(serializer, 0);
            serialize_u8(serializer, 0);
            serialize_u8(serializer, 0);
        } break;
        default:
        case CURRENT_LEVEL_AREA_VERSION: {
            serialize_f32(serializer, &light->position.x);
            serialize_f32(serializer, &light->position.y);
            serialize_f32(serializer, &light->scale.x);
            serialize_f32(serializer, &light->scale.y);
            serialize_f32(serializer, &light->power);
            serialize_u8(serializer,  &light->color.r);
            serialize_u8(serializer,  &light->color.g);
            serialize_u8(serializer,  &light->color.b);
            serialize_u8(serializer,  &light->color.a);
            serialize_u32(serializer, &light->flags);
        } break;
    }
}

void serialize_level_area_entity(struct binary_serializer* serializer, s32 version, struct level_area_entity* entity) {
    switch (version) {
        case 5:
        case 6:
        case 7: {
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

            if (version < 7) { /* old unused */
                for (int i = 0; i < 16; ++i) serialize_u32(serializer, entity->group_ids);
                for (int i = 0; i < 128; ++i){u8 unused[128]; serialize_u8(serializer, unused);}
            }
        } break;
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case CURRENT_LEVEL_AREA_VERSION: {
            /* whoops... */
            serialize_f32(serializer, &entity->position.x);
            serialize_f32(serializer, &entity->position.y);
            serialize_f32(serializer, &entity->scale.x);
            serialize_f32(serializer, &entity->scale.y);
            serialize_bytes(serializer, entity->base_name, ENTITY_BASENAME_LENGTH_MAX);
            serialize_bytes(serializer, entity->script_name, ENTITY_BASENAME_LENGTH_MAX);
            serialize_bytes(serializer, entity->dialogue_file, ENTITY_BASENAME_LENGTH_MAX);
            serialize_s32(serializer, &entity->health_override);
            serialize_s32(serializer, &entity->magic_override);
            serialize_u8(serializer, &entity->facing_direction);
            serialize_u8(serializer, 0);
            serialize_u8(serializer, 0);
            serialize_u8(serializer, 0);
            serialize_u32(serializer, &entity->flags);
            serialize_u32(serializer, &entity->ai_flags);
            serialize_u32(serializer, &entity->spawn_flags);
            for (s32 i = 0; i < 16; ++i)
                serialize_u32(serializer, &entity->group_ids[i]);
            serialize_s32(serializer, &entity->loot_table_id_index);
        } break;
        default: {
            _debugprintf("Either this version doesn't support entities or never existed.");
        } break;
    }
}

void entity_clear_all_abilities(struct entity* entity) {
    entity->ability_count = 0;
}

void entity_add_ability_by_id(struct entity* entity, s32 id) {
    for (s32 existing_ability_index = 0; existing_ability_index < entity->ability_count; ++existing_ability_index) {
        if (entity->abilities[existing_ability_index].ability == id) {
            return;
        }
    }

    if (entity->ability_count < ENTITY_MAX_ABILITIES) {
        struct entity_ability_slot* ability_slot = &entity->abilities[entity->ability_count++];
        ability_slot->ability                    = id;
    }
}

void entity_remove_ability_by_id(struct entity* entity, s32 id) {
    for (s32 ability_index = entity->ability_count-1; ability_index >= 0; --ability_index) {
        struct entity_ability_slot* ability_slot = &entity->abilities[entity->ability_count++];

        if (id == ability_slot->ability) {
            entity->abilities[ability_index] = entity->abilities[--entity->ability_count];
            return;
        }
    }
}

struct used_battle_action_restoration_state used_battle_action_restoration_state(void) {
    struct used_battle_action_restoration_state result = {};
    result.memory_arena_marker = memory_arena_get_cursor(&game_arena);
    result.permenant_entity_state           = entity_list_clone(&game_arena, game_state->permenant_entities);
    result.level_entity_state               = entity_list_clone(&game_arena, game_state->loaded_area.entities);
    result.permenant_particle_emitter_state = entity_particle_emitter_list_clone(&game_arena, game_state->permenant_particle_emitters); 
    {
        result.inventory_state             = memory_arena_push(&game_arena, sizeof(*result.inventory_state));
        memory_copy(&game_state->inventory, result.inventory_state, sizeof(game_state->inventory));
    }

    {
        result.permenant_lights_state       = memory_arena_push(&game_arena, sizeof(*result.permenant_lights_state) * game_state->dynamic_light_count);
        memory_copy(game_state->dynamic_lights, result.permenant_lights_state, sizeof(*game_state->dynamic_lights) * game_state->dynamic_light_count);
        result.permenant_light_count_state = game_state->dynamic_light_count;
    }
    {
        result.level_lights_state      = memory_arena_push(&game_arena, sizeof(*result.level_lights_state) * game_state->loaded_area.lights.count);
        memory_copy(game_state->loaded_area.lights.lights, result.level_lights_state, sizeof(*game_state->loaded_area.lights.lights) * game_state->loaded_area.lights.count);
        result.level_light_count_state = game_state->loaded_area.lights.count;
    }

    memory_arena_set_allocation_region_bottom(&game_arena);
    return result;
}

void entity_add_ability_by_name(struct entity* entity, string id) {
    s32 real_id = entity_database_ability_find_id_by_name(&game_state->entity_database, id);
    entity_add_ability_by_id(entity, real_id);
}

void entity_remove_ability_by_name(struct entity* entity, string id) {
    s32 ability_id = entity_database_ability_find_id_by_name(&game_state->entity_database, id);
    entity_remove_ability_by_id(entity, ability_id);
}

/* NOTE: we need to assert we don't already have the same action, but that *can't* happen on the player menu */
void entity_add_used_battle_action(struct entity* entity, struct used_battle_action action) {
    struct used_battle_action_stack* actions = &entity->used_actions;

    if (actions->top >= LAST_USED_ENTITY_ACTION_COUNT) {
        return;
    }

    actions->actions[actions->top++] = action;
}

void entity_clear_battle_action_stack(struct entity* entity) {
    struct used_battle_action_stack* actions = &entity->used_actions;
    actions->top = 0;
}

bool entity_action_stack_any(struct entity* entity) {
    if (entity->used_actions.top > 0) {
        return true;
    }

    return false;
}

local void restore_game_state_using_restoration_state(struct used_battle_action_restoration_state* restoration_state) {
    struct level_area* loaded_level_area = &game_state->loaded_area;

    { /* recopy everything into memory */
        entity_list_copy(&restoration_state->permenant_entity_state, &game_state->permenant_entities);
        entity_list_copy(&restoration_state->level_entity_state,     &loaded_level_area->entities);

        entity_particle_emitter_list_copy(&restoration_state->permenant_particle_emitter_state, &game_state->permenant_particle_emitters);

        {
            memory_copy(restoration_state->level_lights_state, loaded_level_area->lights.lights, sizeof(*loaded_level_area->lights.lights) * restoration_state->level_light_count_state);
            loaded_level_area->lights.count = restoration_state->level_light_count_state;
            memory_copy(restoration_state->permenant_lights_state, game_state->dynamic_lights, sizeof(*game_state->dynamic_lights) * restoration_state->permenant_light_count_state);
            game_state->dynamic_light_count = restoration_state->permenant_light_count_state;
        }
        {
            memory_copy(restoration_state->inventory_state, &game_state->inventory, sizeof(game_state->inventory));
        }
    }

    memory_arena_set_allocation_region_top(&game_arena); {
        memory_arena_set_cursor(&game_arena, restoration_state->memory_arena_marker);
    } memory_arena_set_allocation_region_bottom(&game_arena);
}

void entity_undo_last_used_battle_action(struct entity* entity) {
    struct used_battle_action_stack* actions          = &entity->used_actions;

    if (actions->top <= 0) {
        return;
    }

    struct used_battle_action* last_used_action  = &actions->actions[actions->top-1];
    actions->top                                -= 1;

    switch (last_used_action->type) {
        case LAST_USED_ENTITY_ACTION_MOVEMENT: {
            entity->position = last_used_action->action_movement.last_movement_position;
            game_focus_camera_to_entity(entity);
        } break;
        case LAST_USED_ENTITY_ACTION_DEFEND: {
            /* does nothing! Removing it from the stack handles this. */
        } break;
        case LAST_USED_ENTITY_ACTION_ITEM_USAGE: {
            struct used_battle_action_item_usage  item_usage = last_used_action->action_item_usage;
            restore_game_state_using_restoration_state(&item_usage.restoration_state);
        } break;
        default: {
            assertion(0 && "Impossible to undo no battle action!");
        } break;
    }
}

bool entity_already_used(struct entity* entity, u8 battle_action_type) {
    s32 stack_top = entity->used_actions.top;

    for (s32 stack_index = 0; stack_index < stack_top; ++stack_index) {
        if (entity->used_actions.actions[stack_index].type == battle_action_type) {
            return true;
        }
    }

    return false;
}

struct used_battle_action battle_action_movement(struct entity* entity) {
    return (struct used_battle_action) {
        .type = LAST_USED_ENTITY_ACTION_MOVEMENT,
        .action_movement.last_movement_position = entity->position
    };
}

struct used_battle_action battle_action_defend(struct entity* entity) {
    return (struct used_battle_action) {
        .type = LAST_USED_ENTITY_ACTION_DEFEND,
    };
}

/* NOTE:
   this is a leaky abstraction,
   but is kind of needed to maintain the engines' memory usage promises.
*/
struct used_battle_action battle_action_item_usage(struct entity* entity, s32 item_use_index) {
    memory_arena_set_allocation_region_top(&game_arena); 
    struct used_battle_action result = {
        .type                                   = LAST_USED_ENTITY_ACTION_ITEM_USAGE,
        .action_item_usage.inventory_item_index = item_use_index,
    };

    struct used_battle_action_item_usage* item_use = &result.action_item_usage;
    item_use->restoration_state                    = used_battle_action_restoration_state();

    return result;
}

/* should have static level up table somewhere... probably here? */
/* knows? */

local s32 get_xp_level_cutoff(s32 current_level) {
    return 300;
}

/*
  Should change stats based off of a table?

  Abilities by level/attribute are technically all predetermined, and
  would just unlock on their own (preallocating *everything*)

  TODO: UI notification
*/
void entity_do_level_up(struct entity* entity) {
    entity->stat_block.level += 1;
}

/* TODO: don't have leveling up curves decided yet. Will do later, like next week????? */
void entity_award_experience(struct entity* entity, s32 xp_amount) {
    entity->stat_block.experience += xp_amount;

    s32 level_cutoff = get_xp_level_cutoff(entity->stat_block.level);
    while (entity->stat_block.experience >= level_cutoff) {
        entity_do_level_up(entity);
        entity->stat_block.experience -= level_cutoff;
        level_cutoff = get_xp_level_cutoff(entity->stat_block.level);
    }
}

struct entity_particle_emitter_list entity_particle_emitter_list(struct memory_arena* arena, s32 capacity) {
    struct entity_particle_emitter_list result = {
        .emitters   = memory_arena_push(arena, capacity * sizeof(*result.emitters)),
        .capacity   = capacity,
    };
    return result;
}

struct entity_particle_emitter_list entity_particle_emitter_list_clone(struct memory_arena* arena, struct entity_particle_emitter_list original) {
    struct entity_particle_emitter_list clone_result = {};

    clone_result.capacity = original.capacity;
    clone_result.emitters = memory_arena_push(arena, sizeof(*clone_result.emitters) * clone_result.capacity);

    entity_particle_emitter_list_copy(&original, &clone_result);

    return clone_result;
}

void entity_particle_emitter_list_copy(struct entity_particle_emitter_list* source, struct entity_particle_emitter_list* destination) {
    memory_copy(source->emitters, destination->emitters, sizeof(*source->emitters) * destination->capacity);
}

/* ENTITY THINK BRAINS */

/*
  This entity just tries to walk as close as possible to an opponent and attacks them.

  This thing doesn't know how to use abilities at all, and is basically just zombie horde attacking
*/

/* copy of battle_ui.c relevant targetting code, need more usage examples before factoring out. */
local void entity_think_basic_zombie_combat_actions(struct entity* entity, struct game_state* state) {
    f32 attack_radius = DEFAULT_ENTITY_ATTACK_RADIUS;
    struct entity_query_list nearby_potential_targets = find_entities_within_radius(&scratch_arena, state, entity->position, attack_radius*2*TILE_UNIT_SIZE, false);
    /* struct entity_query_list nearby_potential_targets = find_entities_within_radius(&scratch_arena, state, entity->position, attack_radius*2*TILE_UNIT_SIZE, true); */

    entity_id closest_valid_entity = {};
    f32 closest_distance           = INFINITY;
    bool found_any_valid_entity    = false;

    bool game_entity_is_party_member(struct entity* entity);
    for (unsigned target_index = 0; target_index < nearby_potential_targets.count; ++target_index) {
        entity_id      current_target_id = nearby_potential_targets.ids[target_index];
        struct entity* target_entity     = game_dereference_entity(state, current_target_id);

        if (!game_entity_is_party_member(target_entity)) {
            continue;
        }

        if (!(target_entity->flags & ENTITY_FLAGS_ALIVE)) {
            continue;
        }
       
        f32 distance = (v2f32_distance(entity->position, target_entity->position));

        if (distance < closest_distance) {
            closest_distance     = distance;
            closest_valid_entity = current_target_id;
            found_any_valid_entity = true;
        }
    }

    if (!found_any_valid_entity) {
        entity->waiting_on_turn = false;
        return;
    }

    attack_radius *= TILE_UNIT_SIZE;
    struct entity* target_entity = game_dereference_entity(state, closest_valid_entity);

    /* try to move closer. */
    if (v2f32_distance(entity->position, target_entity->position) > attack_radius) {
        if (entity_already_used(entity, LAST_USED_ENTITY_ACTION_MOVEMENT)) {
            entity->waiting_on_turn = false;
            return;
        }
        /* TODO LIMIT SIZE OF PATH */
        v2f32 target_entity_position_next_best = v2f32(floorf(target_entity->position.x / TILE_UNIT_SIZE),
                                                       floorf(target_entity->position.y / TILE_UNIT_SIZE));

        /* Find a end point that isn't obstructed... */
        local v2f32 neighbor_offsets[] = {
            v2f32(1, 0),
            v2f32(0, 1),
            v2f32(-1, 0),
            v2f32(0, -1),
            v2f32(1, 1),
            v2f32(1, -1),
            v2f32(-1, 1),
            v2f32(-1, -1),
        };

        for (s32 neighbor_offset_index = 0; neighbor_offset_index < array_count(neighbor_offsets); ++neighbor_offset_index) {
            v2f32 offset = neighbor_offsets[neighbor_offset_index];
            if (level_area_navigation_map_tile_type_at(&game_state->loaded_area, target_entity_position_next_best.x+offset.x, target_entity_position_next_best.y+offset.y) == 0 &&
                !game_any_entity_at_tile_point(v2f32(target_entity_position_next_best.x+offset.x, target_entity_position_next_best.y+offset.y))) {
                target_entity_position_next_best.x += offset.x;
                target_entity_position_next_best.y += offset.y;

                break;
            } 
        }

        struct navigation_path new_path = navigation_path_find(&scratch_arena,
                                                               &state->loaded_area,
                                                               v2f32(entity->position.x / TILE_UNIT_SIZE, entity->position.y / TILE_UNIT_SIZE),
                                                               target_entity_position_next_best);
        const s32 max_path = 4;
        if (new_path.count > max_path) new_path.count = max_path;
        if (new_path.success) {
            entity_combat_submit_movement_action(entity, new_path.points, new_path.count);
        } else {
            entity->waiting_on_turn = false;
            return;
        }
    }

    /* attack if in range */
    if (v2f32_distance(entity->position, target_entity->position) <= attack_radius) {
        entity_combat_submit_attack_action(entity, closest_valid_entity);
    }
}

/* NOTE: This is one of the few serialization procedures that works on the save data version */
void serialize_entity_id(struct binary_serializer* serializer, s32 version, entity_id* id) {
    switch (version) {
        default:
        case CURRENT_SAVE_RECORD_VERSION: {
            serialize_u64(serializer, &id->full_id);
            serialize_s32(serializer, &id->generation);
        } break;
    }
}

struct entity_stat_block entity_find_effective_stat_block(struct entity* entity) {
    struct entity_stat_block result = {};

    for (unsigned index = 0; index < STAT_COUNT; ++index) {
        result.values[index] = entity_find_effective_stat_value(entity, index);
    }

    return result;
}

entity_id game_find_id_for_entity(struct entity* entity) {
    struct entity_iterator it = game_entity_iterator(game_state);

    for (struct entity* current_entity = entity_iterator_begin(&it); !entity_iterator_finished(&it); current_entity = entity_iterator_advance(&it)) {
        if (current_entity == entity) {
            return it.current_id;
        }
    }

    return it.current_id;
}

void entity_add_status_effect(struct entity* entity, struct entity_status_effect effect) {
    if (entity->status_effect_count < MAX_ENTITY_STATUS_EFFECTS) {
        struct entity_status_effect* current_status_effect = &entity->status_effects[entity->status_effect_count++];
        assertion(entity->status_effect_count < MAX_ENTITY_STATUS_EFFECTS && "Too many status effects?");
        *current_status_effect = effect;
    }
}

local void entity_remove_status_effect_at(struct entity* entity, s32 index) {
    struct entity_status_effect* to_remove = entity->status_effects + index;
    switch (to_remove->type) { /* cleanup code */
    }
    struct entity_particle_emitter* emitter = entity_particle_emitter_dereference(&game_state->permenant_particle_emitters, to_remove->particle_emitter_id);
    {
        entity_particle_emitter_stop_emitting(&game_state->permenant_particle_emitters, to_remove->particle_emitter_id);
        entity_particle_emitter_kill(&game_state->permenant_particle_emitters, to_remove->particle_emitter_id);
    }
    entity->status_effects[index] = entity->status_effects[--entity->status_effect_count];
}

void entity_remove_first_status_effect_of_type(struct entity* entity, s32 type) {
    for (s32 index = 0; index < entity->status_effect_count; ++index) {
        struct entity_status_effect* current_status_effect = entity->status_effects + index;
        if (current_status_effect->type == type) {
            entity_remove_status_effect_at(entity, index);
            return;
        }
    }
}

void entity_remove_all_status_effect_of_type(struct entity* entity, s32 type) {
    for (s32 index = 0; index < entity->status_effect_count; ++index) {
        struct entity_status_effect* current_status_effect = entity->status_effects + index;
        if (current_status_effect->type == type) {
            entity_remove_status_effect_at(entity, index);
        }
    }
}

void entity_update_all_status_effects(struct entity* entity, f32 dt) {
    { /* maintain particles and stuff like that I suppose */
        for (s32 status_index = 0; status_index < entity->status_effect_count; ++status_index) {
            struct entity_status_effect*    current_effect = entity->status_effects + status_index;
            struct entity_particle_emitter* emitter        = entity_particle_emitter_dereference(&game_state->permenant_particle_emitters, current_effect->particle_emitter_id);
            emitter->position                              = v2f32_scale(entity->position, 1.0/TILE_UNIT_SIZE);
            emitter->position.x += 0.5;
            entity_particle_emitter_start_emitting(&game_state->permenant_particle_emitters, current_effect->particle_emitter_id);
        }
    }

    if (game_state->combat_state.active_combat) {
        /* nothing, update_combat will handle that */
        return;
    } else {
        entity->status_effect_tic_timer += dt;
        if (entity->status_effect_tic_timer >= REAL_SECONDS_FOR_GAME_TURN) {
            entity->status_effect_tic_timer = 0;
            entity_update_all_status_effects_for_a_turn(entity);
        }
    }
}

void entity_update_all_status_effects_for_a_turn(struct entity* entity) {
    bool found_flame = false;

    for (s32 status_index = 0; status_index < entity->status_effect_count; ++status_index) {
        struct entity_status_effect* current_effect = entity->status_effects + status_index;

        switch (current_effect->type) {
            case ENTITY_STATUS_EFFECT_TYPE_IGNITE: {
                f32 proposed_damage = entity->health.max;
                proposed_damage *= 0.05;
                if (proposed_damage < 30) {
                    proposed_damage = 30;
                } else if (proposed_damage >= 250) {
                    proposed_damage = 250;
                }
                entity_do_absolute_hurt(entity, proposed_damage);
                found_flame = true;
                entity->burning_char_effect_level += 1;
            } break;
            case ENTITY_STATUS_EFFECT_TYPE_POISON: {
                f32 proposed_damage = entity->health.max;
                proposed_damage *= 0.10;
                entity_do_absolute_hurt(entity, proposed_damage);
            } break;
            default: {
                unimplemented("???? Don't know what status effect this is");
            } break;
        }

        current_effect->turn_duration -= 1;
    }

    if (!found_flame) {
        entity->burning_char_effect_level -= 1;
        if (entity->burning_char_effect_level < 0) {
            entity->burning_char_effect_level = 0;
        }
    }

    for (s32 status_index = entity->status_effect_count-1; status_index >= 0; --status_index) {
        struct entity_status_effect* current_effect = entity->status_effects + status_index;

        if (current_effect->turn_duration <= 0) {
            entity_remove_status_effect_at(entity, status_index);
        }
    }
}

bool entity_has_any_status_effect_of_type(struct entity* entity, s32 type) {
    for (s32 status_index = 0; status_index < entity->status_effect_count; ++status_index) {
        struct entity_status_effect* current_effect = entity->status_effects + status_index;
        if (current_effect->type == type) {
            return true;
        }
    }

    return false;
}

struct entity_status_effect status_effect_ignite(s32 turn_duration) {
    s32                             particle_emitter_id = entity_particle_emitter_allocate(&game_state->permenant_particle_emitters);
    struct entity_particle_emitter* emitter             = entity_particle_emitter_dereference(&game_state->permenant_particle_emitters, particle_emitter_id);
    {
        emitter->time_per_spawn                  = 0.07;
        emitter->burst_amount                    = 32;
        emitter->max_spawn_per_batch             = 1024;
        emitter->max_spawn_batches               = -1;
        emitter->color                           = color32u8(226, 88, 34, 255);
        emitter->target_color                    = color32u8(59, 59, 56, 127);

        emitter->starting_acceleration           = v2f32(0, -15.6);
        emitter->starting_acceleration_variance  = v2f32(1.2, 1.2);
        emitter->starting_velocity_variance      = v2f32(1.3, 0);
        emitter->lifetime                        = 0.6;
        emitter->lifetime_variance               = 0.35;
        emitter->spawn_shape                     = emitter_spawn_shape_rectangle(v2f32(0,0), v2f32(0.5, 1), 1, false);

        /* emitter->particle_type = ENTITY_PARTICLE_TYPE_FIRE; */
        emitter->particle_feature_flags = ENTITY_PARTICLE_FEATURE_FLAG_FLAMES;

        emitter->scale_uniform = 0.2;
        emitter->scale_variance_uniform = 0.12;
    }

    return (struct entity_status_effect) {
        .type = ENTITY_STATUS_EFFECT_TYPE_IGNITE,
        .turn_duration = turn_duration,
        .particle_emitter_id = particle_emitter_id,
    };
}

struct entity_status_effect status_effect_poison(s32 turn_duration) {
    s32                             particle_emitter_id = entity_particle_emitter_allocate(&game_state->permenant_particle_emitters);
    struct entity_particle_emitter* emitter             = entity_particle_emitter_dereference(&game_state->permenant_particle_emitters, particle_emitter_id);
    {
        emitter->time_per_spawn                  = 0.30;
        emitter->burst_amount                    = 16;
        emitter->max_spawn_per_batch             = 1024;
        emitter->max_spawn_batches               = -1;
        emitter->color                           = color32u8(0, 0, 255, 255);
        emitter->target_color                    = color32u8(0, 0, 128, 127);
        emitter->starting_acceleration           = v2f32(0, -4);
        emitter->starting_acceleration_variance  = v2f32(1.2, 1.2);
        emitter->starting_velocity_variance      = v2f32(1.3, 0);
        emitter->lifetime                        = 0.3;
        emitter->lifetime_variance               = 0.45;
        emitter->spawn_shape                     = emitter_spawn_shape_rectangle(v2f32(0.05,0), v2f32(0.5, 1), 1, false);

        /* emitter->particle_type = ENTITY_PARTICLE_TYPE_FIRE; */
        emitter->particle_feature_flags = ENTITY_PARTICLE_FEATURE_FLAG_FLAMES;

        emitter->scale_uniform = 0.1;
        emitter->scale_variance_uniform = 0.1;
    }

    return (struct entity_status_effect) {
        .type = ENTITY_STATUS_EFFECT_TYPE_POISON,
        .turn_duration = turn_duration,
        .particle_emitter_id = particle_emitter_id,
    };
}

/*
  Automatically defines *_reserved, *_push, *_remove, *_clear, *_serialize, which are the most common procedures

  This isn't a very "convenient" macro as all the parameters are a bit too strict...

  There's a copied variant with a push that doesn't have a param as some struct objects are too complicated to include that as a parameter
  for IMO.
*/
#define Define_Common_List_Type_Procedures(List_Type_Without_Tag, Main_Type_Without_Tag, DataMember, SerializeFn) \
    struct List_Type_Without_Tag List_Type_Without_Tag##_reserved(struct memory_arena* arena, s32 capacity) { \
        struct List_Type_Without_Tag result = {};                       \
        result.capacity = capacity;                                     \
        result.DataMember = memory_arena_push(arena, sizeof(*result.DataMember) * capacity); \
        return result;                                                  \
    }                                                                   \
    struct Main_Type_Without_Tag* List_Type_Without_Tag##_push(struct List_Type_Without_Tag* list, struct Main_Type_Without_Tag obj) { \
        struct Main_Type_Without_Tag* result = &list->DataMember[list->count++]; \
        *result = obj;                                                  \
        return result;                                                  \
    }                                                                   \
    void List_Type_Without_Tag##_remove(struct List_Type_Without_Tag* list, s32 index) { \
        list->DataMember[index] = list->DataMember[--list->count];      \
    }                                                                   \
    void List_Type_Without_Tag##_clear(struct List_Type_Without_Tag* list) { \
        list->count = 0;                                                \
    }                                                                   \
    void serialize_##List_Type_Without_Tag(struct binary_serializer* serializer, struct memory_arena* arena, s32 version, struct List_Type_Without_Tag* list) { \
        serialize_s32(serializer, &list->count);                        \
        if (list->capacity == 0) {                                      \
            list->DataMember = memory_arena_push(arena, sizeof(*list->DataMember) * list->count); \
        } else {                                                        \
            assertion(list->count < list->capacity);                    \
        }                                                               \
        for (s32 i = 0; i < list->count; ++i) {                         \
            SerializeFn(serializer, version, list->DataMember + i);     \
        }                                                               \
    }
#define Define_Common_List_Type_Procedures_Alloc_Push(List_Type_Without_Tag, Main_Type_Without_Tag, DataMember, SerializeFn) \
    struct List_Type_Without_Tag List_Type_Without_Tag##_reserved(struct memory_arena* arena, s32 capacity) { \
        struct List_Type_Without_Tag result = {};                       \
        result.capacity = capacity;                                     \
        result.DataMember = memory_arena_push(arena, sizeof(*result.DataMember) * capacity); \
        return result;                                                  \
    }                                                                   \
    struct Main_Type_Without_Tag* List_Type_Without_Tag##_push(struct List_Type_Without_Tag* list) { \
        struct Main_Type_Without_Tag* result = &list->DataMember[list->count++]; \
        return result;                                                  \
    }                                                                   \
    void List_Type_Without_Tag##_remove(struct List_Type_Without_Tag* list, s32 index) { \
        list->DataMember[index] = list->DataMember[--list->count];      \
    }                                                                   \
    void List_Type_Without_Tag##_clear(struct List_Type_Without_Tag* list) { \
        list->count = 0;                                                \
    }                                                                   \
    void serialize_##List_Type_Without_Tag(struct binary_serializer* serializer, struct memory_arena* arena, s32 version, struct List_Type_Without_Tag* list) { \
        serialize_s32(serializer, &list->count);                        \
        if (list->capacity == 0) {                                      \
            list->DataMember = memory_arena_push(arena, sizeof(*list->DataMember) * list->count); \
        } else {                                                        \
            assertion(list->count < list->capacity);                    \
        }                                                               \
        for (s32 i = 0; i < list->count; ++i) {                         \
            SerializeFn(serializer, version, list->DataMember + i);     \
        }                                                               \
    }



Define_Common_List_Type_Procedures(position_marker_list, position_marker, markers, serialize_position_marker);
Define_Common_List_Type_Procedures(tile_layer, tile, tiles, serialize_tile);
Define_Common_List_Type_Procedures(trigger_level_transition_list, trigger_level_transition, transitions, serialize_trigger_level_transition);
Define_Common_List_Type_Procedures_Alloc_Push(entity_chest_list, entity_chest, chests, serialize_entity_chest);
Define_Common_List_Type_Procedures(light_list, light_def, lights, serialize_light);
Define_Common_List_Type_Procedures_Alloc_Push(trigger_list, trigger, triggers, serialize_generic_trigger);
Define_Common_List_Type_Procedures(level_area_savepoint_list, level_area_savepoint, savepoints, serialize_level_area_entity_savepoint);
Define_Common_List_Type_Procedures(level_area_battle_safe_square_list, level_area_battle_safe_square, squares, serialize_battle_safe_square);
Define_Common_List_Type_Procedures_Alloc_Push(level_area_entity_list, level_area_entity, entities, serialize_level_area_entity);
Define_Common_List_Type_Procedures(world_location_list, world_location, locations, serialize_world_location);

struct light_def* light_list_find_light_at(struct light_list* list, v2f32 point) {
    for (s32 index = 0; index < list->count; ++index) {
        struct light_def* current_light = list->lights + index;
        if (rectangle_f32_intersect(rectangle_f32(current_light->position.x, current_light->position.y, 1, 1), rectangle_f32(point.x, point.y, 0.05, 0.05))) {
            return current_light;
        }
    }

    return NULL;
}
struct level_area_savepoint* level_area_savepoint_list_find_savepoint_at(struct level_area_savepoint_list* list, v2f32 point) {
    for (s32 index = 0; index < list->count; ++index) {
        struct level_area_savepoint* current_savepoint = list->savepoints + index;
        if (rectangle_f32_intersect(rectangle_f32(current_savepoint->position.x, current_savepoint->position.y, 1, 1), rectangle_f32(point.x, point.y, 0.05, 0.05))) {
            return current_savepoint;
        }
    }

    return NULL;
}
struct level_area_battle_safe_square* level_area_battle_safe_square_list_tile_at(struct level_area_battle_safe_square_list* list, s32 x, s32 y) {
    for (s32 index = 0; index < list->count; ++index) {
        struct level_area_battle_safe_square* current_tile = list->squares + index;

        if (current_tile->x == x && current_tile->y == y) {
            return current_tile;
        }
    }

    return NULL;
}
void level_area_battle_safe_square_list_remove_at(struct level_area_battle_safe_square_list* list, s32 x, s32 y) {
    struct level_area_battle_safe_square* existing_square = level_area_battle_safe_square_list_tile_at(list, x, y);
    if (existing_square) {
        s32 index = existing_square - list->squares;
        level_area_battle_safe_square_list_remove(list, index);
    }
}
struct level_area_entity* level_area_entity_list_find_entity_at(struct level_area_entity_list* list, v2f32 point) {
    for (s32 index = 0; index < list->count; ++index) {
        struct level_area_entity* current_entity = list->entities + index;
        if (rectangle_f32_intersect(rectangle_f32(current_entity->position.x, current_entity->position.y, current_entity->scale.x, current_entity->scale.y), rectangle_f32(point.x, point.y, 0.05, 0.05))) {
            return current_entity;
        }
    }

    return NULL;
}

void tile_layer_bounding_box(struct tile_layer* tile_layer, s32* min_x, s32* min_y, s32* max_x, s32* max_y) {
    for (s32 tile_index = 0; tile_index < tile_layer->count; ++tile_index) {
        struct tile* it = tile_layer->tiles + tile_index;
        if ((s32)it->x < *min_x) *min_x = (s32)(it->x);
        if ((s32)it->y < *min_y) *min_y = (s32)(it->y);
        if ((s32)it->x > *max_x) *max_x = (s32)(it->x);
        if ((s32)it->y > *max_y) *max_y = (s32)(it->y);
    }

    return;
}

struct tile* tile_layer_tile_at(struct tile_layer* tile_layer, s32 x, s32 y) {
    for (s32 index = 0; index < tile_layer->count; ++index) {
        struct tile* current_tile = tile_layer->tiles + index;

        if (current_tile->x == x && current_tile->y == y) {
            return current_tile;
        }
    }

    return NULL;
}

void tile_layer_remove_at(struct tile_layer* tile_layer, s32 x, s32 y) {
    for (s32 index = 0; index < tile_layer->count; ++index) {
        struct tile* current_tile = tile_layer->tiles + index;

        if (current_tile->x == x && current_tile->y == y) {
            tile_layer_remove(tile_layer, index);
            return;
        }
    }
}

struct trigger_level_transition* trigger_level_transition_list_transition_at(struct trigger_level_transition_list* list, v2f32 point) {
    for (s32 index = 0; index < list->count; ++index) {
        struct trigger_level_transition* current_trigger = list->transitions + index;

        if (rectangle_f32_intersect(current_trigger->bounds, rectangle_f32(point.x, point.y, 0.05, 0.05))) {
            return current_trigger;
        }
    }

    return NULL;
}
struct entity_chest* entity_chest_list_find_at(struct entity_chest_list* list, v2f32 position) {
    for (s32 chest_index = 0; chest_index < list->count; ++chest_index) {
        struct entity_chest* current_chest = list->chests + chest_index;

        if (rectangle_f32_intersect(rectangle_f32(current_chest->position.x, current_chest->position.y, current_chest->scale.x, current_chest->scale.y), rectangle_f32(position.x, position.y, 0.05, 0.05))) {
            return current_chest;
        }
    } 

    return NULL;
}

struct trigger* trigger_list_trigger_at(struct trigger_list* list, v2f32 point) {
    for (s32 index = 0; index < list->count; ++index) {
        struct trigger* current_trigger = list->triggers + index;
        if (rectangle_f32_intersect(current_trigger->bounds, rectangle_f32(point.x, point.y, 0.05, 0.05))) {
            return current_trigger;
        }
    }

    return NULL;
}

struct world_location world_location(v2f32 position, v2f32 scale, string name) {
    struct world_location result = {
        .position = position,
        .scale    = scale,
    };

    copy_string_into_cstring(name, result.preview_name, array_count(result.preview_name));
    return result;
}

struct world_location* world_location_list_location_at(struct world_location_list* list, v2f32 position) {
    for (s32 location_index = 0; location_index < list->count; ++location_index) {
        struct world_location* current_location = list->locations + location_index;

        if (rectangle_f32_intersect(rectangle_f32(current_location->position.x, current_location->position.y, current_location->scale.x, current_location->scale.y), rectangle_f32(position.x, position.y, 0.05, 0.05))) {
            return current_location;
        }
    } 

    return NULL;
}

#undef Define_Common_List_Type_Procedures
#undef Define_Common_List_Type_Procedures_Alloc_Push
#include "entity_ability.c"
