#ifndef ENTITY_STATUS_EFFECT_DEF_C
#define ENTITY_STATUS_EFFECT_DEF_C

#define MAX_ENTITY_STATUS_EFFECTS (32)

/*
  NOTE: These status effects are going to be completely hard-coded for now until I think of a better way to combine these,
  with the limited "magical" things in the game it is likely it'll just remain like this
*/
enum entity_status_effect_type {
    ENTITY_STATUS_EFFECT_TYPE_IGNITE,
    ENTITY_STATUS_EFFECT_TYPE_POISON,
    ENTITY_STATUS_EFFECT_TYPE_COUNT,
};

local string entity_status_effect_type_strings[] = {
    string_literal("ignite"),
    string_literal("poison"),
    string_literal("(count)"),
};

s32 entity_status_effect_type_from_string(string value) {
    for (s32 index = 0; index < array_count(entity_status_effect_type_strings); ++index) {
        if (string_equal(entity_status_effect_type_strings[index], value)) {
            return index;
        }
    }

    return -1;
}

struct entity_status_effect {
    s32 type;
    s32 turn_duration;
    s32 particle_emitter_id;
};

struct entity_status_effect status_effect_ignite(s32 turn_duration);
struct entity_status_effect status_effect_poison(s32 turn_duration);
#endif
