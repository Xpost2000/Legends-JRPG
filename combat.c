local void add_all_combat_participants(struct game_state* state, struct entity_list* entities) {
    /* TODO for now we don't have initiative so we'll just add in the same order */  
    struct game_state_combat_state* combat_state = &state->combat_state;
    combat_state->count                          = 0;
    combat_state->active_combatant               = 0;

    for (s32 index = 0; index < entities->capacity; ++index) {
        if (entities->entities[index].flags & ENTITY_FLAGS_ALIVE) {
            entities->entities[index].waiting_on_turn         = true;
            entities->entities[index].ai.wait_timer              = 0;
            combat_state->participants[combat_state->count++] = entity_list_get_id(entities, index);
        }
    }
}

/* check for any nearby conflicts for any reason */
/* for now just check if there are any enemies in the play area. */
/* no need for anything fancy right now. That comes later. */
/* (hard coding data is a real pain in my ass, so until I can specify NPCs through data, I just
   want the quickest way of doing stuff) */
local void determine_if_combat_should_begin(struct game_state* state, struct entity_list* entities) {
    struct entity* player = entity_list_dereference_entity(entities, player_id);

    bool should_be_in_combat = false;

    for (u32 index = 0; index < entities->capacity && !should_be_in_combat; ++index) {
        struct entity* current_entity = entities->entities + index;

        if (!(current_entity->flags & ENTITY_FLAGS_ALIVE))
            continue;

        if (current_entity != player) {
            struct entity_ai_data* ai = &current_entity->ai;

            if (ai->flags & ENTITY_AI_FLAGS_AGGRESSIVE_TO_PLAYER) {
                should_be_in_combat = true;
            }
        }
    }

    if (should_be_in_combat) {
        state->combat_state.active_combat    = true;
        state->combat_state.active_combatant = 0;
        start_combat_ui();
        add_all_combat_participants(state, entities);
    }
}

local struct entity* find_current_combatant(struct game_state* state) {
    struct game_state_combat_state* combat_state        = &state->combat_state;
    entity_id                       active_combatant_id = combat_state->participants[combat_state->active_combatant];
    return entity_list_dereference_entity(&state->entities, active_combatant_id);
}

void update_combat(struct game_state* state, f32 dt) {
    struct game_state_combat_state* combat_state = &state->combat_state;
    struct entity* combatant = find_current_combatant(state);

    bool should_end_combat = true;

    {
        struct entity* player = game_get_player(state);
        for (s32 index = 0; index < combat_state->count; ++index) {
            struct entity* potential_combatant = entity_list_dereference_entity(&state->entities, combat_state->participants[index]);

            if (player != potential_combatant) {
                bool aggressive = is_entity_aggressive_to_player(potential_combatant);
                _debugprintf("%d", aggressive);
                if (aggressive) {
                    should_end_combat = false;
                }
            }
        }
    }

    if (should_end_combat) {
        combat_state->active_combat = false;
        /* Notify the Battle UI to display the after action report. */
    } else {
        if (!(combatant->flags & ENTITY_FLAGS_ALIVE)) {
            combat_state->active_combatant += 1;
        }

        if (combatant->waiting_on_turn) {
            entity_think_combat_actions(combatant, state, dt);
        } else {
            combat_state->active_combatant += 1;

            if (combat_state->active_combatant >= combat_state->count) {
                add_all_combat_participants(state, &state->entities);
            }
        }
    }
}
