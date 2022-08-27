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
