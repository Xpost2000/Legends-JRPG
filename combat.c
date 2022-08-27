local void add_all_combat_participants(struct game_state* state) {
    /* TODO for now we don't have initiative so we'll just add in the same order */  
    struct game_state_combat_state* combat_state = &state->combat_state;
    combat_state->count                          = 0;
    combat_state->active_combatant               = 0;

    struct entity_iterator it = game_entity_iterator(state);

    for (struct entity* current_entity = entity_iterator_begin(&it); !entity_iterator_finished(&it); current_entity = entity_iterator_advance(&it)) {
        /* snap everyone to the combat grid (might be a bit jarring, which is okay for the demo.) */
        if (current_entity->flags & ENTITY_FLAGS_ALIVE) {
            entity_snap_to_grid_position(current_entity);
            current_entity->waiting_on_turn                   = true;
            current_entity->ai.wait_timer                     = 0;
            combat_state->participants[combat_state->count++] = it.current_id;
        }
    }
}

/* check for any nearby conflicts for any reason */
/* for now just check if there are any enemies in the play area. */
/* no need for anything fancy right now. That comes later. */
/* (hard coding data is a real pain in my ass, so until I can specify NPCs through data, I just
   want the quickest way of doing stuff) */
local void determine_if_combat_should_begin(struct game_state* state) {
    struct entity* player = game_get_player(state);

    bool should_be_in_combat = false;

    struct entity_iterator it = game_entity_iterator(state);

    for (struct entity* current_entity = entity_iterator_begin(&it); !entity_iterator_finished(&it); current_entity = entity_iterator_advance(&it)) {
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
        add_all_combat_participants(state);
    }
}

local struct entity* find_current_combatant(struct game_state* state) {
    struct game_state_combat_state* combat_state        = &state->combat_state;
    entity_id                       active_combatant_id = combat_state->participants[combat_state->active_combatant];
    return entity_list_dereference_entity(&state->permenant_entities, active_combatant_id);
}

void update_combat(struct game_state* state, f32 dt) {
    struct game_state_combat_state* combat_state = &state->combat_state;
    struct entity* combatant = find_current_combatant(state);

    bool should_end_combat = true;

    {
        struct entity* player = game_get_player(state);
        for (s32 index = 0; index < combat_state->count; ++index) {
            struct entity* potential_combatant = entity_list_dereference_entity(&state->permenant_entities, combat_state->participants[index]);

            if (player != potential_combatant) {
                bool aggressive = is_entity_aggressive_to_player(potential_combatant);

                if (aggressive) {
                    should_end_combat = false;
                }
            }
        }
    }

    if (should_end_combat) {
        end_combat_ui();
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
                add_all_combat_participants(state);
            }
        }
    }
}
