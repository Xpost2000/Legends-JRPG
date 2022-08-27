#ifndef ENTITY_ABILITY_DEF_C
#define ENTITY_ABILITY_DEF_C

/* These should all have an animation sequence associated with them. */
/* should be stored in a table */
#define ENTITY_ABILITY_SELECTION_FIELD_MAX_X (16)
#define ENTITY_ABILITY_SELECTION_FIELD_MAX_Y (16)

enum entity_ability_selection_type {
    ABILITY_SELECTION_TYPE_ATTACK_RANGE,
    ABILITY_SELECTION_TYPE_FIELD,
    ABILITY_SELECTION_TYPE_EVERYTHING,
};
enum entity_ability_selection_mask {
    ABILITY_SELECTION_MASK_ENEMIES    = BIT(0),
    ABILITY_SELECTION_MASK_SELF       = BIT(1),
    ABILITY_SELECTION_MASK_ALLIES     = BIT(2),

    ABILITY_SELECTION_MASK_EVERYTHING = (ABILITY_SELECTION_MASK_ENEMIES |
                                         ABILITY_SELECTION_MASK_SELF |
                                         ABILITY_SELECTION_MASK_ALLIES)
};

struct entity_ability {
    /* valid target selections */
    string id_name;
    string name;
    string description;
    u8 selection_type;
    u8 selection_field[ENTITY_ABILITY_SELECTION_FIELD_MAX_Y][ENTITY_ABILITY_SELECTION_FIELD_MAX_X];
    /* TODO add animation sequence to do things */
};

struct entity_ability_database {
    struct memory_arena* arena;
    struct entity_ability* abilities;
};

typedef struct entity_ability_id {
    s16 index;
} entity_ability_id;

struct entity_ability_slot {
    entity_ability_id ability;
    s32 usages;
    u8 ability_level;
};

void initialize_ability_database(struct memory_arena* arena);
entity_ability_id add_ability_to_database(struct entity_ability ability);
entity_ability_id find_ability_by_name(string id_name);

#endif
