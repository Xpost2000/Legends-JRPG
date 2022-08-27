local struct entity_ability_database global_ability_database = {};
local entity_ability_id null_ability = {};

void initialize_ability_database(struct memory_arena* arena) {
    global_ability_database.arena = arena;
    global_ability_database.abilities = memory_arena_push(global_ability_database.arena, 0);
}

entity_ability_id add_ability_to_database(struct entity_ability ability, string id_name) {
    memory_arena_push(global_ability_database.arena, sizeof(ability));

    struct entity_ability* new_ability = &global_ability_database.abilities[global_ability_database.ability_count++];

    *new_ability = ability;
    new_ability->id_name = id_name;

    return null_ability;
}

entity_ability_id find_ability_by_name(string id_name) {
    for (s32 ability_index = 0; ability_index < global_ability_database.ability_count; ++ability_index) {
        struct entity_ability* ability = global_ability_database.abilities + ability_index;

        if (string_equal(ability->id_name, id_name)) {
            return (entity_ability_id) { .index = ability_index };
        }
    }
    return null_ability;
}

struct entity_ability* dereference_ability(entity_ability_id id) {
    return global_ability_database.abilities + id.index;
}

void build_ability_database(void) {
    {
        struct entity_ability sword_rush = {};
        sword_rush.name = string_literal("Sword Rush");
        sword_rush.description = string_literal("Move in a flash, slashing through multiple enemies at once.");
        sword_rush.selection_type = ABILITY_SELECTION_TYPE_FIELD;
        sword_rush.moving_field = 0;

        sword_rush.selection_field[ENTITY_ABILITY_SELECTION_FIELD_CENTER_Y][ENTITY_ABILITY_SELECTION_FIELD_CENTER_X] = true;
        sword_rush.selection_field[ENTITY_ABILITY_SELECTION_FIELD_CENTER_Y-1][ENTITY_ABILITY_SELECTION_FIELD_CENTER_X] = true;
        sword_rush.selection_field[ENTITY_ABILITY_SELECTION_FIELD_CENTER_Y-2][ENTITY_ABILITY_SELECTION_FIELD_CENTER_X] = true;
        sword_rush.selection_field[ENTITY_ABILITY_SELECTION_FIELD_CENTER_Y-3][ENTITY_ABILITY_SELECTION_FIELD_CENTER_X] = true;

        add_ability_to_database(sword_rush, string_literal("ability_sword_rush"));
    }
    {
        struct entity_ability shock = {}; 
        shock.name = string_literal("Shock");
        shock.description = string_literal("Electrocute your enemies.");
        shock.selection_type = ABILITY_SELECTION_TYPE_FIELD_SHAPE;
        shock.moving_field   = 0;

        for (s32 y = 0; y < ENTITY_ABILITY_SELECTION_FIELD_MAX_Y; ++y) {
            for (s32 x = 0; x < ENTITY_ABILITY_SELECTION_FIELD_MAX_X; ++x) {
                shock.selection_field[y][x] = true;
            }
        }

        add_ability_to_database(shock, string_literal("ability_shock"));
    }
}

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
                    s32 read_x = -y + ENTITY_ABILITY_SELECTION_FIELD_MAX_X;
                    s32 read_y = x;

                    field_copy[y * ENTITY_ABILITY_SELECTION_FIELD_MAX_X + x] = temporary_copy[read_y * ENTITY_ABILITY_SELECTION_FIELD_MAX_X + read_x];
                }
            }
        }
    }
}
