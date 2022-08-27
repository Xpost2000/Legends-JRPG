#ifndef ENTITY_ABILITY_DEF_C
#define ENTITY_ABILITY_DEF_C

/* These should all have an animation sequence associated with them. */
/* should be stored in a table */
#define ENTITY_ABILITY_SELECTION_FIELD_MAX_X (8+1)
#define ENTITY_ABILITY_SELECTION_FIELD_MAX_Y (8+1)

#define ENTITY_ABILITY_SELECTION_FIELD_CENTER_X (ENTITY_ABILITY_SELECTION_FIELD_MAX_X/2)
#define ENTITY_ABILITY_SELECTION_FIELD_CENTER_Y (ENTITY_ABILITY_SELECTION_FIELD_MAX_Y/2)

/* The preallocation really hurts, but it's not as if I wasn't going to use that memory anyways... */
#define ENTITY_ABILITY_SELECTION_MAX_TARGETTABLE (ENTITY_ABILITY_SELECTION_FIELD_MAX_X*ENTITY_ABILITY_SELECTION_FIELD_MAX_Y)

enum entity_ability_selection_type {
    ABILITY_SELECTION_TYPE_ATTACK_RANGE,
    /* The field *is* the selection. */
    ABILITY_SELECTION_TYPE_FIELD,
    /* allow selecting within the field */
    ABILITY_SELECTION_TYPE_FIELD_SHAPE,
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

    u8 moving_field;   /* Disgaea style selection. Whether the selection field is planted or movable */
    u8 max_move_units_x;
    u8 max_move_units_y;

    /* Should support reflection of 90 degrees for all fields... */

    /* weird centerings */
    /*
      NOTE: If I want the full flexibility that Disgaea has in it's selection fields, I can
      encode the level required to access the slot..

      0 0 0 0 0 0
      0 3 3 3 3 0
      3 3 2 2 3 3
      3 3 2 2 3 3
      0 3 3 3 3 0

      Like the number represents the minimum ability level required to see the slot as selectable or something.
    */
    /* NOTE: This field is relative to direction */
    /* the default shapes should assume the user is facing *up* */
    u8 selection_field[ENTITY_ABILITY_SELECTION_FIELD_MAX_Y][ENTITY_ABILITY_SELECTION_FIELD_MAX_X];
    /* TODO add animation sequence to do things */
};

/*
  0 - north
  1 - west
  2 - south
  3 - east

  Counter clockwise rotations.
 */
void copy_selection_field_rotated_as(struct entity_ability* ability, u8* field_copy, s32 quadrant);

struct entity_ability_database {
    struct memory_arena* arena;
    s32                    ability_count;
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

void                   initialize_ability_database(struct memory_arena* arena);
void                   build_ability_database(void);
entity_ability_id      add_ability_to_database(struct entity_ability ability, string id_name);
entity_ability_id      find_ability_by_name(string id_name);
struct entity_ability* dereference_ability(entity_ability_id id);

#endif
