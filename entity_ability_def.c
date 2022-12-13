/*
  NOTE: there are a few niche ability types that are just not implemented yet, or
  some edge-cases for abilities that are just never used, since it never occured to me
  what would happen if they existed...
 */
#ifndef ENTITY_ABILITY_DEF_C
#define ENTITY_ABILITY_DEF_C

#define ENTITY_ABILITY_SEQUENCE_MAX_PARTICIPANTS (32)

#define ENTITY_ABILITY_SELECTION_FIELD_MAX_X (8+1)
#define ENTITY_ABILITY_SELECTION_FIELD_MAX_Y (8+1)

#define ENTITY_ABILITY_MAX_SELECTABLE_ENTITIES (ENTITY_ABILITY_SELECTION_FIELD_MAX_X * ENTITY_ABILITY_SELECTION_FIELD_MAX_Y)

#define ENTITY_ABILITY_SELECTION_FIELD_CENTER_X (ENTITY_ABILITY_SELECTION_FIELD_MAX_X/2)
#define ENTITY_ABILITY_SELECTION_FIELD_CENTER_Y (ENTITY_ABILITY_SELECTION_FIELD_MAX_Y/2)

/* The preallocation really hurts, but it's not as if I wasn't going to use that memory anyways... */
#define ENTITY_ABILITY_SELECTION_MAX_TARGETTABLE (ENTITY_ABILITY_SELECTION_FIELD_MAX_X*ENTITY_ABILITY_SELECTION_FIELD_MAX_Y)

/* Require (PARALLELS) block, which requires the next n blocks to execute in sequence without waiting */

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
  NOTE: the reason I chose to make sequence actions actual data constructs, as opposed
  to purely interpreted lisp structures, is that this makes them smaller which is what I need
  to maintain as this is long term data. I think almost all of the scripts are short lived
  or otherwise temporary allocations.

  This is a permenant allocation and those strings are space that I shouldn't have to waste.
 */
enum sequence_action_type {
    SEQUENCE_ACTION_FOCUS_CAMERA,
    SEQUENCE_ACTION_MOVE_TO,
    SEQUENCE_ACTION_LOOK_AT,
    SEQUENCE_ACTION_HURT,
    SEQUENCE_ACTION_DO_HARDCODED_ANIM,
    SEQUENCE_ACTION_START_SPECIAL_FX,
    SEQUENCE_ACTION_STOP_SPECIAL_FX,
    SEQUENCE_ACTION_WAIT_SPECIAL_FX_TO_FINISH,
    SEQUENCE_ACTION_EXPLOSION,
    SEQUENCE_ACTION_REQUIRE_BLOCK,
};

/* does not allow ally targetting */
enum sequence_action_entity_target_type {
    ENTITY_TARGET_ID_USER,
    ENTITY_TARGET_ID_TARGET,
};
struct sequence_action_target_entity {
    s16 entity_target_type;
    s16 entity_target_index; /* store the target list in the attacker and use this to figure stuff out */
};

/* force the game camera to do things, lots of state overriding going to scare me. */
struct sequence_action_focus_camera {
    struct sequence_action_target_entity target;
};

enum sequence_interpolation_type {
    SEQUENCE_LINEAR,
    SEQUENCE_CUBIC_EASE_IN,
    SEQUENCE_CUBIC_EASE_OUT,
    SEQUENCE_CUBIC_EASE_IN_OUT,
    SEQUENCE_QUADRATIC_EASE_IN,
    SEQUENCE_QUADRATIC_EASE_OUT,
    SEQUENCE_QUADRATIC_EASE_IN_OUT,
    SEQUENCE_BACK_EASE_IN,
    SEQUENCE_BACK_EASE_OUT,
    SEQUENCE_BACK_EASE_IN_OUT,
};

enum move_target_type {
    MOVE_TARGET_ENTITY,
    MOVE_TARGET_START,
    MOVE_TARGET_RELATIVE_DIRECTION_TO,
};
struct sequence_action_move_to {
    struct sequence_action_target_entity to_move; /* TODO: UNUSED */

    s32 interpolation_type;
    s32 move_target_type;

    /*  in TILE_UNIT_SIZES */
    f32 desired_velocity_magnitude; /* add interpolation max time field, since it's always by one */

    union {
        struct {
            struct sequence_action_target_entity target;
            s32    move_past;
        } entity;
    } move_target;
};

enum look_target_type {
    LOOK_TARGET_TYPE_DIRECTION,
    LOOK_TARGET_TYPE_RELATIVE_DIRECTION,
    LOOK_TARGET_TYPE_ENTITY,
};
struct sequence_action_look_at {
    struct sequence_action_target_entity to_look; /* TODO: UNUSED */
    s32 look_target_type;
    s32 direction_target;
    struct sequence_action_target_entity look_target;
};

struct sequence_action_explosion {
    struct sequence_action_target_entity where_to_explode;

    f32 explosion_radius;
    /*
      NOTE:
      I want this to be scalable by a formula later, or have some way to
      adjust this, since it's something that might happen a lot.
    */
    s32 explosion_damage;
    s32 explosion_effect_id;
};

/* HURT should be given some scaling modifiers */
/* for now it just does 9999 dmg */
enum hurt_target_flags {
    HURT_TARGET_FLAG_NONE         = 0,
    HURT_TARGET_FLAG_ALL_SELECTED = BIT(0),
    HURT_TARGET_FLAG_EVERY_ENEMY  = BIT(1),
};
struct sequence_action_hurt {
    /* if these flags are toggled, they override targets with special behavior */
    u8 hurt_target_flags; /* special */

    s32 target_count;
    /* you use this only if you want to do something more cinematic */
    struct sequence_action_target_entity targets[ENTITY_ABILITY_MAX_SELECTABLE_ENTITIES];
};

/*
  Using this for escape hatches if it's a bit **too** difficult
  to express using the data format.
*/
struct sequence_action_hardcoded_animation {
    s32 id;
};

struct sequence_action_special_fx {
    s32 effect_id;
};

struct sequence_action_require_block {
    u8                                   needed;
    s16                                  required_entity_count;
    struct sequence_action_target_entity required_entities[ENTITY_ABILITY_MAX_SELECTABLE_ENTITIES];
};

struct entity_ability_sequence_action {
    s32 type;
    union {
        struct sequence_action_hurt                hurt;
        struct sequence_action_move_to             move_to;
        struct sequence_action_focus_camera        focus_camera;
        struct sequence_action_special_fx          special_fx;
        struct sequence_action_hardcoded_animation hardcoded_anim;
        struct sequence_action_explosion           explosion;
        struct sequence_action_look_at             look_at;
        struct sequence_action_require_block       require_block;
    };
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
    u8     selection_type;

    u8 moving_field;   /* Disgaea style selection. Whether the selection field is planted or movable */
    u8 max_move_units_x;
    u8 max_move_units_y;

    bool innate;
    s32  item_class_group; /* an identifier to share amongst all similar abilities */
    bool requires_no_obstructions;

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

      Also this field doesn't really determine the gameplay characteristics of anything! (unless we skip
      animations, in which case we use this as a shortcut for everything... But this is a purely
      visual construct!)
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
