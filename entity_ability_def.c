#ifndef ENTITY_ABILITY_DEF_C
#define ENTITY_ABILITY_DEF_C

#define ENTITY_ABILITY_SEQUENCE_MAX_PARTICIPANTS (32)

#define ENTITY_ABILITY_SELECTION_FIELD_MAX_X (8+1)
#define ENTITY_ABILITY_SELECTION_FIELD_MAX_Y (8+1)

#define ENTITY_ABILITY_SELECTION_FIELD_CENTER_X (ENTITY_ABILITY_SELECTION_FIELD_MAX_X/2)
#define ENTITY_ABILITY_SELECTION_FIELD_CENTER_Y (ENTITY_ABILITY_SELECTION_FIELD_MAX_Y/2)

/* The preallocation really hurts, but it's not as if I wasn't going to use that memory anyways... */
#define ENTITY_ABILITY_SELECTION_MAX_TARGETTABLE (ENTITY_ABILITY_SELECTION_FIELD_MAX_X*ENTITY_ABILITY_SELECTION_FIELD_MAX_Y)

enum entity_ability_selection_type {
    ABILITY_SELECTION_TYPE_ATTACK_RANGE, /* NOTE: unused */

    /* The field *is* the selection. */
    ABILITY_SELECTION_TYPE_FIELD,
    /* allow selecting within the field */
    ABILITY_SELECTION_TYPE_FIELD_SHAPE,

    ABILITY_SELECTION_TYPE_EVERYTHING, /* NOTE: unused */
};
enum entity_ability_selection_mask {
    ABILITY_SELECTION_MASK_ENEMIES    = BIT(0),
    ABILITY_SELECTION_MASK_SELF       = BIT(1),
    ABILITY_SELECTION_MASK_ALLIES     = BIT(2),

    ABILITY_SELECTION_MASK_EVERYTHING = (ABILITY_SELECTION_MASK_ENEMIES |
                                         ABILITY_SELECTION_MASK_SELF |
                                         ABILITY_SELECTION_MASK_ALLIES)
};

/*
  Not going to interpret this just to use less memory and frankly the interpretation
  code is going to be more messier than just "compiling" down and just doing work on my
  raw operations...
 */
/* keep track of time or something later. */
struct entity_ability_sequence_action {
    s32 type;
    /* other data */
};
struct entity_ability_sequence {
    struct entity_ability_sequence_action* sequence_actions;
    s32 sequence_action_count;
};

#if 0
void skip_current_ability_animation(void) {
    for each anim form; {
        if (damage causing) {
            // do all shit
        }

        if (movement causing) {
            // do all shit
        }
    }

    okay done!
}
#endif

/*
  Add equipment filter restriction?
 */
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
    struct entity_ability_sequence sequence;
};

/* NOTE: too lazy to rearrange stuff to be correct. animation_sequence_list is a struct lisp_form* */
void entity_ability_compile_animation_sequence(struct memory_arena* arena, struct entity_ability* ability, void* animation_sequence_list);

/*
  0 - north
  1 - west
  2 - south
  3 - east

  Counter clockwise rotations.
 */
void copy_selection_field_rotated_as(struct entity_ability* ability, u8* field_copy, s32 quadrant);

struct entity_ability_slot {
    s32 ability;
    s32 usages;
    u8 ability_level;
};

#endif
