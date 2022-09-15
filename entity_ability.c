/*
  Later we can implement our animation code here probably.

  An attack might count as an ability so who knows.
*/

s32 facing_direction_to_quadrant(s32 facing_dir) {
    switch (facing_dir) {
        case DIRECTION_LEFT : return 1;
        case DIRECTION_UP   : return 0;
        case DIRECTION_RIGHT: return 3;
        case DIRECTION_DOWN : return 2;
    }

    return 0;
}

void copy_selection_field_rotated_as(struct entity_ability* ability, u8* field_copy, s32 quadrant) {
    quadrant %= 4;

    for (s32 y = 0; y < ENTITY_ABILITY_SELECTION_FIELD_MAX_Y; ++y) {
        for (s32 x = 0; x < ENTITY_ABILITY_SELECTION_FIELD_MAX_X; ++x) {
            field_copy[y * ENTITY_ABILITY_SELECTION_FIELD_MAX_X + x] = ability->selection_field[y][x];
        }
    }

    /* Only field shapes, and planted fields can rotate. This should be semi intuitive? */
    if (ability->selection_type == ABILITY_SELECTION_TYPE_FIELD && !ability->moving_field) {
        u8* temporary_copy = memory_arena_push(&scratch_arena, sizeof(u8) * ENTITY_ABILITY_SELECTION_FIELD_MAX_Y * ENTITY_ABILITY_SELECTION_FIELD_MAX_X);

        for (s32 rotations = 0; rotations < quadrant; ++rotations) {
            /* copy into temporary */
            {
                for (s32 y = 0; y < ENTITY_ABILITY_SELECTION_FIELD_MAX_Y; ++y) {
                    for (s32 x = 0; x < ENTITY_ABILITY_SELECTION_FIELD_MAX_X; ++x) {
                        temporary_copy[y * ENTITY_ABILITY_SELECTION_FIELD_MAX_X + x] = field_copy[y * ENTITY_ABILITY_SELECTION_FIELD_MAX_X + x];
                    }
                }
            }
            /* -y rotation */
            {
                for (s32 y = 0; y < ENTITY_ABILITY_SELECTION_FIELD_MAX_Y; ++y) {
                    for (s32 x = 0; x < ENTITY_ABILITY_SELECTION_FIELD_MAX_X; ++x) {
                        s32 read_x = -y + (ENTITY_ABILITY_SELECTION_FIELD_MAX_X-1);
                        s32 read_y = x;

                        field_copy[y * ENTITY_ABILITY_SELECTION_FIELD_MAX_X + x] = temporary_copy[read_y * ENTITY_ABILITY_SELECTION_FIELD_MAX_X + read_x];
                    }
                }
            }
        }
    }
}

void entity_ability_compile_animation_sequence(struct memory_arena* arena, struct entity_ability* ability, void* animation_sequence_list) {
    assertion(((struct lisp_form*)(animation_sequence_list))->type == LISP_FORM_LIST && "This is not a list. This is bad!");

    struct entity_ability_sequence* sequence     = &ability->sequence;
    s32                             action_count = ((struct lisp_form*)(animation_sequence_list))->list.count;

    sequence->sequence_action_count = action_count;
    sequence->sequence_actions      = memory_arena_push(arena, action_count * sizeof(*sequence->sequence_actions));

    if (action_count == 0) {
        _debugprintf("No anim sequence... This might be a problem?");
    }

    for (s32 action_form_index = 0; action_form_index < action_count; ++action_form_index) {
        struct entity_ability_sequence_action* action_data         = sequence->sequence_actions + action_form_index;
        struct lisp_form*                      current_action_form = lisp_list_nth(animation_sequence_list, action_form_index);

        {
            struct lisp_form* action_form_header         = lisp_list_nth(current_action_form, 0); 
            struct lisp_form  action_form_rest_arguments = lisp_list_sliced(*current_action_form, 1, -1);

            bool successfully_parsed = true;

            successfully_parsed = false;

            _debugprintf("Do not recognize header: \"%.*s\"", action_form_header->string.length, action_form_header->string.data);
            assertion(successfully_parsed && "Could not parse argument in sequence! Crashing. Please fix!");
        }
    }
}
