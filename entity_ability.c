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
