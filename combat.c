local void sort_combat_participants(void) {
    /* also an insertion sort, there's a very small amount of entities and this happens once per round */
    struct game_state_combat_state* combat_state = &game_state->combat_state;

    for (s32 participant_index = 1; participant_index < combat_state->count; ++participant_index) {
        s32       insertion_index = participant_index;

        entity_id key_entity      = combat_state->participants[participant_index];
        s32       key_priority    = combat_state->participant_priorities[participant_index];

        for (; insertion_index > 0; --insertion_index) {
            s32 compared_priority = combat_state->participant_priorities[insertion_index-1];

            if (compared_priority > key_priority) {
                break;
            } else {
                combat_state->participants[insertion_index]           = combat_state->participants[insertion_index-1];
                combat_state->participant_priorities[insertion_index] = combat_state->participant_priorities[insertion_index-1];
            }
        }

        combat_state->participants[insertion_index]           = key_entity;
        combat_state->participant_priorities[insertion_index] = key_priority;
    }
}

local void add_all_combat_participants(struct game_state* state) {
    /* TODO for now we don't have initiative so we'll just add in the same order */  
    struct game_state_combat_state* combat_state = &state->combat_state;
    combat_state->count                          = 0;
    combat_state->active_combatant               = 0;

    struct entity_iterator it = game_entity_iterator(state);

    s32 index = 0;
    for (struct entity* current_entity = entity_iterator_begin(&it); !entity_iterator_finished(&it); current_entity = entity_iterator_advance(&it)) {
        /* snap everyone to the combat grid (might be a bit jarring, which is okay for the demo.) */
        if (current_entity->flags & ENTITY_FLAGS_ALIVE) {
            entity_snap_to_grid_position(current_entity);
            current_entity->waiting_on_turn                   = true;
            current_entity->ai.attack_animation_timer         = 0;

            s32 priority = index;
            {
                combat_state->participants[combat_state->count]           = it.current_id;
                combat_state->participant_priorities[combat_state->count] = priority;
                combat_state->count++;
            }
        }
        index++;
    }

    sort_combat_participants();
}

/* check for any nearby conflicts for any reason */
/* for now just check if there are any enemies in the play area. */
/* no need for anything fancy right now. That comes later. */
/* (hard coding data is a real pain in my ass, so until I can specify NPCs through data, I just
   want the quickest way of doing stuff) */
local bool should_be_in_combat(struct game_state* state) {
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

    return should_be_in_combat;
}

local void determine_if_combat_should_begin(struct game_state* state) {
    if (should_be_in_combat(state)) {
        state->combat_state.active_combat    = true;
        state->combat_state.active_combatant = 0;
        start_combat_ui();
        add_all_combat_participants(state);
    }
}

local struct entity* find_current_combatant(struct game_state* state) {
    struct game_state_combat_state* combat_state        = &state->combat_state;
    entity_id                       active_combatant_id = combat_state->participants[combat_state->active_combatant];
    return game_dereference_entity(state, active_combatant_id);
}

void update_combat(struct game_state* state, f32 dt) {
    struct game_state_combat_state* combat_state = &state->combat_state;
    struct entity* combatant = find_current_combatant(state);

    bool should_end_combat = !should_be_in_combat(state);

    if (should_end_combat) {
        end_combat_ui();
        /* Notify the Battle UI to display the after action report. */
    } else {
        if (!(combatant->flags & ENTITY_FLAGS_ALIVE)) {
            combat_state->active_combatant += 1;
        }

        if (global_battle_ui_state.phase == BATTLE_UI_IDLE) {
            if (combatant->waiting_on_turn) {
                entity_think_combat_actions(combatant, state, dt);
            } else {
                if (!combatant->ai.current_action) {
                    combat_state->active_combatant += 1;

                    if (combat_state->active_combatant >= combat_state->count) {
                        add_all_combat_participants(state);
                    }
                }
            }
        }
    }
}
