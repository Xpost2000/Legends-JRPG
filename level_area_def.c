#ifndef LEVEL_AREA_DEF_C
#define LEVEL_AREA_DEF_C

/*
  NOTE: I don't think there's a good manual way to do something like
  this, but metaprogramming this would be a pita, so I'm just manually
  updating everything which is kind of dumb...
*/

/* NOTE: 
   Change incoming from version >= 4
   
   Expecting to separate the tiles into layers! This
   is to allow level complexity! All old tiles are assumed to
   operate on the "object" tile level.
   
   Version 5: Add entities
   Version 6: Add light entities
   Version 7: Change to the level area entity struct.
   Version 9: Added savepoint entities
*/
#define CURRENT_LEVEL_AREA_VERSION (9)

enum tile_layers {
    TILE_LAYER_GROUND,            /* render below all. dark color? */
                                                /* also collision layer */
    TILE_LAYER_OBJECT,            /* render at normal level as entities */
    TILE_LAYER_CLUTTER_DECOR,     /* render at normal level as entities */
    TILE_LAYER_OVERHEAD,          /* render above entities no fading */
    TILE_LAYER_ROOF,              /* render above entities, allow fading */
    TILE_LAYER_FOREGROUND,        /* render above entities, no fading */
    TILE_LAYER_COUNT
};

local string tile_layer_strings[] = {
    string_literal("(ground)"),
    string_literal("(object)"), 
    string_literal("(clutter)"), 
    string_literal("(overhead)"),
    string_literal("(roof)"),
    string_literal("(foreground)"),
    string_literal("(count)"),
};

struct tile {
    s32 id;
    /* NOTE, remove? */
    s32 flags; /* acts as a XOR against it's parent? (tile definitions elsewhere.) */
    s16 x;
    s16 y;
    /* old: layer field */
    s16 reserved_;
};

struct trigger_level_transition {
    /* assume to be in tile coordinates. */
    struct rectangle_f32 bounds;
    /* for binary structs, I need cstrings unfortunately. Otherwise they are a little too inconvenient to serialize...*/
    char  target_level[128];
    /* anchoring to an object, might be very niche... */
    u8    new_facing_direction;
    v2f32 spawn_location;
};

struct level_area_navigation_map_tile {
    f32 score_modifier; 
    s32 type;          /* 0 ground, 1 solid, 2 obstacle(removable?) */
};

struct level_area_navigation_map {
    s32 width;
    s32 height;

    /* keep this here if I have to remap coordinates or something */
    s32 min_x;
    s32 min_y;

    s32 max_x;
    s32 max_y;

    struct level_area_navigation_map_tile* tiles;
};

struct level_area_savepoint {
    v2f32 position;
    v2f32 scale;
    u32   flags;
};

/* need to determine how to make an accurate id system for this */
#define ENTITY_BASENAME_LENGTH_MAX (64)

/*
  REVISIONS:
  (1): initial.
  (2)(version 7):
       - Added loot table index field.
       - Remove reserved (128 u8) field, fuck it, just handle it manually...
*/
enum level_area_entity_spawn_flags {
    LEVEL_AREA_ENTITY_SPAWN_FLAGS_NONE,
    /*
      Do not spawn if their death is recorded in the save record.
     */
    LEVEL_AREA_ENTITY_SPAWN_FLAGS_KILLONCE = BIT(0),
    /*
      Frankly I want this so we can have semi-random-encounters, where entities
      are allowed to spawn in set locations based on the editor, and we can use that for random
      combat encounters.

      It's a simple thing to do and would be okay for me to do, but requires more editor support.
     */
    /* LEVEL_AREA_ENTITY_SPAWN_FLAGS_RANDOM_SPAWN = BIT(1), */
};
struct level_area_entity {
    /*
      This is only a rectangle because it allows me to use it for the drag candidate system in the
      editor.
      
      we always use the model size when unpacking.
     */
    v2f32 position;
    v2f32 scale;

    /* look this up in the entity dictionary */
    /* I would like to hash but don't want to risk changing hashing later. */
    /* NOTE: Turns out I don't hash in the DB, so we could keep an index but that requires data changes. */
    char  base_name[ENTITY_BASENAME_LENGTH_MAX];
    char  script_name[ENTITY_BASENAME_LENGTH_MAX]; /* Use this to refer for game script reasons */
    char  dialogue_file[ENTITY_BASENAME_LENGTH_MAX];

    /* not editted */
    /* Currently not editted. */
    s32 health_override; /* -1 == default */
    s32 magic_override;  /* -1 == default */

    u8  facing_direction;

    /* TODO: none of the flags are editted */
    /* for the most part look at enum entity_flags */
    u32 flags;
    /* look at enum entity_ai_flags */
    u32 ai_flags;
    u32 spawn_flags;

    /* TODO not editted yet */
    /* use for quicker script referencing */
    /* not used? */
    u32 group_ids[16];
    /* ???  */
    s32 loot_table_id_index;
};

void serialize_level_area_entity_savepoint(struct binary_serializer* serializer, s32 version, struct level_area_savepoint* entity);
void serialize_level_area_entity(struct binary_serializer* serializer, s32 version, struct level_area_entity* entity);

void level_area_entity_set_base_id(struct level_area_entity* entity, string name) {
    s32 copy_amount = name.length;
    if (copy_amount > array_count(entity->base_name)) {
        copy_amount = array_count(entity->base_name);
    }

    cstring_copy(name.data, entity->base_name, copy_amount);
}

/* We're going to listen for a few prescribed events. */
enum level_area_listen_event {
    LEVEL_AREA_LISTEN_EVENT_ON_TOUCH,
    LEVEL_AREA_LISTEN_EVENT_ON_ACTIVATE,
    LEVEL_AREA_LISTEN_EVENT_ON_DEATH,
    LEVEL_AREA_LISTEN_EVENT_ON_LOOT,
    /* this isn't really a listener but it's whatever. */
    LEVEL_AREA_LISTEN_EVENT_ROUTINE,
#if 0
    LEVEL_AREA_LISTEN_EVENT_ON_HIT,
#endif
    LEVEL_AREA_LISTEN_EVENT_COUNT
};
struct level_area_listener {
    /*
      Should record trigger target here,
      
      Although it can be interpreted live as everything else... Might be easier
      to prestore targets later. However for now I guess I'll interpret it.
    */
    s32 subscribers;
    struct lisp_form* subscriber_codes;
};
local string level_area_listen_event_form_names[] = {
    [LEVEL_AREA_LISTEN_EVENT_ON_TOUCH]    = string_literal("on-touch"),
    [LEVEL_AREA_LISTEN_EVENT_ON_ACTIVATE] = string_literal("on-activate"),
    [LEVEL_AREA_LISTEN_EVENT_ON_DEATH]    = string_literal("on-death"),
    [LEVEL_AREA_LISTEN_EVENT_ON_LOOT]    = string_literal("on-loot"),
    [LEVEL_AREA_LISTEN_EVENT_ROUTINE]     = string_literal("routine"),
#if 0
    string_literal("on-hit"),
#endif
    string_literal("(count)"),
};

struct level_area_script_data {
    bool present;
    struct file_buffer buffer;
    struct lisp_list*  code_forms;

    struct lisp_form*  on_enter;
    struct lisp_form*  on_frame;
    struct lisp_form*  on_exit;

    struct level_area_listener listeners[LEVEL_AREA_LISTEN_EVENT_COUNT];
};

struct trigger {
    /* NOTE: reserve this for the future, in case I need this */
    struct rectangle_f32 bounds;
    u32                  activations;
    u8                   activation_method;
    u8                   active;
    char                 unique_name[32];
};

struct level_area {
    /* keep reference of a name. */
    u32          version;
    v2f32        default_player_spawn;

    s32          tile_counts[TILE_LAYER_COUNT];
    struct tile* tile_layers[TILE_LAYER_COUNT];

    /*
      Address in script file as (trigger (id) or (name-string?(when supported.)))
     */

    s32                              light_count;
    struct light_def*                lights;
    s32                              trigger_level_transition_count;
    struct trigger_level_transition* trigger_level_transitions;
    s32                              script_trigger_count;
    struct trigger*                  script_triggers;
    s32                              entity_chest_count;
    struct entity_chest*             chests;
    s32                              entity_savepoint_count;
    struct entity_savepoint*         savepoints;

    /* runtime data */
    struct entity_list               entities;
    struct level_area_script_data    script;
    struct level_area_navigation_map navigation_data;
    s32                              reported_entity_death_count;
    entity_id                        reported_entity_deaths[1024];
    /* used for displaying what tiles you can walk to. */
    u8*                              combat_movement_visibility_map;

    bool   on_enter_triggered;
};

/* grid coordinate searches */
struct entity_chest* level_area_get_chest_at(struct level_area* area, s32 x, s32 y) {
    for (s32 chest_index = 0; chest_index < area->entity_chest_count; ++chest_index) {
        struct entity_chest* current_chest = area->chests + chest_index;

        s32 chest_x = floorf(current_chest->position.x);
        s32 chest_y = floorf(current_chest->position.y);

        if (chest_x == x && chest_y == y) {
            return current_chest;
        }
    }

    return NULL;
}

struct tile* level_area_get_tile_at(struct level_area* area, s32 tile_layer, s32 x, s32 y) {
    s32 tile_count = area->tile_counts[tile_layer];

    for (s32 tile_index = 0; tile_index < tile_count; ++tile_index) {
        struct tile* current_tile = area->tile_layers[tile_layer] + tile_index;

        s32 tile_x = current_tile->x;
        s32 tile_y = current_tile->y;

        if (tile_x == x && tile_y == y) {
            return current_tile;
        }
    }

    return NULL;
}

bool level_area_any_obstructions_at(struct level_area* area, s32 x, s32 y) {
    if (level_area_get_chest_at(area, x, y)) {
        return true;
    }

    if (level_area_get_tile_at(area, TILE_LAYER_OBJECT, x, y)) {
        return true;
    }

    return false;
}

#endif
