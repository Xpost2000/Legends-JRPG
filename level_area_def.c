#ifndef LEVEL_AREA_DEF_C
#define LEVEL_AREA_DEF_C

#define MAX_REPORTED_ENTITY_DEATHS (1024)

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
   Version 10: Format fix
   Version 11: Battle Safe Square
   Verions 12: AreaName BuiltIn & BuiltIn scripts & Extra Tile Layers
*/
#define CURRENT_LEVEL_AREA_VERSION (12)

#define SCRIPTABLE_TILE_LAYER_COUNT (32)
enum scriptable_tile_layer_flags {
    SCRIPTABLE_TILE_LAYER_FLAGS_NONE      = 0,
    SCRIPTABLE_TILE_LAYER_FLAGS_NOCOLLIDE = BIT(0),
    SCRIPTABLE_TILE_LAYER_FLAGS_HIDDEN    = ENTITY_FLAGS_HIDDEN,
};
struct scriptable_tile_layer_property { /* these are all script editable properties... Thank god! I don't have to write more editor code */
    /* in tile units */
    f32 offset_x;
    f32 offset_y;
    u32 flags;
    s32 draw_layer; /* refer to enum tile_layers */
};

enum tile_layers {
    TILE_LAYER_GROUND,            /* render below all. dark color? */
                                                /* also collision layer */
    TILE_LAYER_OBJECT,            /* render at normal level as entities */
    TILE_LAYER_CLUTTER_DECOR,     /* render at normal level as entities */
    TILE_LAYER_OVERHEAD,          /* render above entities no fading */
    TILE_LAYER_ROOF,              /* render above entities, allow fading */
    TILE_LAYER_FOREGROUND,        /* render above entities, no fading */
    /* FLEXIBLE EXTRA LAYERS */
    TILE_LAYER_SCRIPTABLE_0,
    TILE_LAYER_SCRIPTABLE_1,
    TILE_LAYER_SCRIPTABLE_2,
    TILE_LAYER_SCRIPTABLE_3,
    TILE_LAYER_SCRIPTABLE_4,
    TILE_LAYER_SCRIPTABLE_5,
    TILE_LAYER_SCRIPTABLE_6,
    TILE_LAYER_SCRIPTABLE_7,
    TILE_LAYER_SCRIPTABLE_8,
    TILE_LAYER_SCRIPTABLE_9,
    TILE_LAYER_SCRIPTABLE_10,
    TILE_LAYER_SCRIPTABLE_11,
    TILE_LAYER_SCRIPTABLE_12,
    TILE_LAYER_SCRIPTABLE_13,
    TILE_LAYER_SCRIPTABLE_14,
    TILE_LAYER_SCRIPTABLE_15,
    TILE_LAYER_SCRIPTABLE_16,
    TILE_LAYER_SCRIPTABLE_17,
    TILE_LAYER_SCRIPTABLE_18,
    TILE_LAYER_SCRIPTABLE_19,
    TILE_LAYER_SCRIPTABLE_20,
    TILE_LAYER_SCRIPTABLE_21,
    TILE_LAYER_SCRIPTABLE_22,
    TILE_LAYER_SCRIPTABLE_23,
    TILE_LAYER_SCRIPTABLE_24,
    TILE_LAYER_SCRIPTABLE_25,
    TILE_LAYER_SCRIPTABLE_26,
    TILE_LAYER_SCRIPTABLE_27,
    TILE_LAYER_SCRIPTABLE_28,
    TILE_LAYER_SCRIPTABLE_29,
    TILE_LAYER_SCRIPTABLE_30,
    TILE_LAYER_SCRIPTABLE_31,
    TILE_LAYER_COUNT
};

local string tile_layer_strings[] = {
    string_literal("(ground)"),
    string_literal("(object)"), 
    string_literal("(clutter)"), 
    string_literal("(overhead)"),
    string_literal("(roof)"),
    string_literal("(foreground)"),
    string_literal("(scriptable 0)"),
    string_literal("(scriptable 1)"),
    string_literal("(scriptable 2)"),
    string_literal("(scriptable 3)"),
    string_literal("(scriptable 4)"),
    string_literal("(scriptable 5)"),
    string_literal("(scriptable 6)"),
    string_literal("(scriptable 7)"),
    string_literal("(scriptable 8)"),
    string_literal("(scriptable 9)"),
    string_literal("(scriptable 10)"),
    string_literal("(scriptable 11)"),
    string_literal("(scriptable 12)"),
    string_literal("(scriptable 13)"),
    string_literal("(scriptable 14)"),
    string_literal("(scriptable 15)"),
    string_literal("(scriptable 16)"),
    string_literal("(scriptable 17)"),
    string_literal("(scriptable 18)"),
    string_literal("(scriptable 19)"),
    string_literal("(scriptable 20)"),
    string_literal("(scriptable 21)"),
    string_literal("(scriptable 22)"),
    string_literal("(scriptable 23)"),
    string_literal("(scriptable 24)"),
    string_literal("(scriptable 25)"),
    string_literal("(scriptable 26)"),
    string_literal("(scriptable 27)"),
    string_literal("(scriptable 28)"),
    string_literal("(scriptable 29)"),
    string_literal("(scriptable 30)"),
    string_literal("(scriptable 31)"),
    string_literal("(count)"),
};

struct tile {
    s32 id;
    /* NOTE, remove? */
    u32 flags; /* acts as a XOR against it's parent? (tile definitions elsewhere.) */
    s16 x;
    s16 y;
    /* old: layer field */
    s16 reserved_;
};
void serialize_tile(struct binary_serializer* serializer, s32 version, struct tile* tile);

struct trigger_level_transition { /* TODO: Allow these to also be registered "on-trigger" events, except these can return a final value (T/F) for whether you can allow the transition */
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
/* I should honestly reduce the size, but on average these things(The level_areas on disk) are small anyways, so I'm not going to be too concerned about this */
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
struct level_area_entity UNPACK_INTO(struct entity) {
    /*
      This is only a rectangle because it allows me to use it for the drag candidate system in the
      editor.
      
      we always use the model size when unpacking.
     */
    SERIALIZE_VERSIONS(level, 5 to CURRENT) v2f32 position;
    SERIALIZE_VERSIONS(level, 5 to CURRENT) v2f32 scale;

    /* look this up in the entity dictionary */
    /* I would like to hash but don't want to risk changing hashing later. */
    /* NOTE: Turns out I don't hash in the DB, so we could keep an index but that requires data changes. */
    SERIALIZE_VERSIONS(level, 5 to CURRENT) char  base_name[ENTITY_BASENAME_LENGTH_MAX];
    SERIALIZE_VERSIONS(level, 5 to CURRENT) char  script_name[ENTITY_BASENAME_LENGTH_MAX]; /* Use this to refer for game script reasons */
    SERIALIZE_VERSIONS(level, 5 to CURRENT) char  dialogue_file[ENTITY_BASENAME_LENGTH_MAX];

    /* not editted */
    /* Currently not editted. */
    SERIALIZE_VERSIONS(level, 5 to CURRENT) s32 health_override UNPACK_INTO(health.value, DEFAULT(-1, 0)); /* -1 == default */
    SERIALIZE_VERSIONS(level, 5 to CURRENT) s32 magic_override  UNPACK_INTO(magic.value,  DEFAULT(-1, 0));  /* -1 == default */

    SERIALIZE_VERSIONS(level, 5 to CURRENT) u8  facing_direction;

#if 0
    SERIALIZE_VERSIONS(level, 8 to 10)u8 pad0;
    SERIALIZE_VERSIONS(level, 8 to 10)u8 pad1;
    SERIALIZE_VERSIONS(level, 8 to 10)u8 pad2;
#endif

    /* TODO: none of the flags are editted */
    /* for the most part look at enum entity_flags */
    SERIALIZE_VERSIONS(level, 5 to CURRENT) u32 flags;
    /* look at enum entity_ai_flags */
    SERIALIZE_VERSIONS(level, 5 to CURRENT) u32 ai_flags;
    SERIALIZE_VERSIONS(level, 5 to CURRENT) u32 spawn_flags;

    /* TODO not editted yet */
    /* use for quicker script referencing */
    /* not used? */
    SERIALIZE_VERSIONS(level, 5,6,8 to CURRENT) u32 group_ids[16];
#if 0
    SERIALIZE_VERSIONS(level, 5,6) u8 unused[128];
#endif
    /* ???  */
    SERIALIZE_VERSIONS(level, 8 to CURRENT) s32 loot_table_id_index;
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
    bool isbuiltin;
    union {
        struct file_buffer buffer;
        string             internal_buffer;
    };
    struct lisp_list*  code_forms;

    struct lisp_form*  on_enter;
    struct lisp_form*  on_frame;
    struct lisp_form*  on_exit;

    struct level_area_listener listeners[LEVEL_AREA_LISTEN_EVENT_COUNT];
};

/* TODO: use a flags field. */
struct trigger {
    /* NOTE: reserve this for the future, in case I need this */
    struct rectangle_f32 bounds;
    u32                  activations;
    u8                   activation_method;
    u8                   active;
    char                 unique_name[32];
};

/*
  Since levels may have triggers and lots of other things, we want to avoid
  the player from walking into them inadvertently during combat and causing weird things to happen.

  So I have to disable triggers in the traditional sense, otherwise stuff will break.
  I don't know how a game like Divinity Original Sin 2 does it, but it does help that they have an open world with
  obvious boundaries like oceans. This game isn't open world, so it has to cut off combat areas otherwise we walk to the
  border of game levels, which is obviously bad.
*/
struct level_area_battle_zone_bounding_box {
    s32 min_x;
    s32 min_y;
    s32 max_x;
    s32 max_y;

    /* cache the island indices here. */
    s32  square_count;
    s32* squares;
    /* linked list */
    struct level_area_battle_zone_bounding_box* next;
};

struct level_area_battle_safe_square { /* thankfully these are obvious to implement, and don't require too much work */
    s32 x;
    s32 y;

    /* runtime data, associate with a battle zone */
    u16                            island_index;
};


struct level_area_tilemap_tile {
    s32 id;
    s16 x;
    s16 y;
};

/*
  NOTE: these are probably the most complicated to fit into a normal combat zone, since the navigation map
  is fixed at load time.

  Level Area Tilemaps are really designed to just be props though. Maybe with enough creativity they can be used as moving platforms
  or something, but since all combat positions are gridlocked it's a PITA to guarantee these will not break during combat since these
  things can move outside of the grid
 */
struct level_area { /* this cannot be automatically serialized because of the unpack stage. I can use macros to reduce the burden though */
    /* keep reference of a name. */
    u32          version SERIALIZE_VERSIONS(level, 1 to CURRENT);
    v2f32        default_player_spawn;

    struct       scriptable_tile_layer_property scriptable_layer_properties[SCRIPTABLE_TILE_LAYER_COUNT];
    s32                                         tile_counts[TILE_LAYER_COUNT];
    struct tile*                                tile_layers[TILE_LAYER_COUNT];


    char area_name[260];
    /*
      Address in script file as (trigger (id) or (name-string?(when supported.)))
     */

    s32                                   trigger_level_transition_count;
    struct trigger_level_transition*      trigger_level_transitions SERIALIZE_VERSIONS(level, 1 to CURRENT) VARIABLE_ARRAY(trigger_level_transition_count);
    s32                                   entity_chest_count;
    struct entity_chest*                  chests SERIALIZE_VERSIONS(level, 2 to CURRENT) VARIABLE_ARRAY(entity_chest_count);
    s32                                   script_trigger_count;
    struct trigger*                       script_triggers SERIALIZE_VERSIONS(level, 3 to CURRENT) VARIABLE_ARRAY(script_trigger_count);
    struct entity_list                    entities SERIALIZE_VERSIONS(level, 5 to CURRENT) VARIABLE_ARRAY(create=entity_list_create alloc=entity_list_create_entity) PACKED_AS(struct level_area_entity);
    s32                                   light_count;
    struct light_def*                     lights SERIALIZE_VERSIONS(level, 6 to CURRENT) VARIABLE_ARRAY(light_count);
    s32                                   entity_savepoint_count;
    struct entity_savepoint*              savepoints SERIALIZE_VERSIONS(level, 9 to CURRENT) VARIABLE_ARRAY(entity_savepoint_count) PACKED_AS(struct level_area_savepoint);
    s32                                   battle_safe_square_count;
    struct level_area_battle_safe_square* battle_safe_squares SERIALIZE_VERSIONS(level, 11 to CURRENT) VARIABLE_ARRAY(battle_safe_square_count);

    /* runtime data */
    struct level_area_script_data    script;
    struct level_area_navigation_map navigation_data;
    s32                              reported_entity_death_count;
    entity_id                        reported_entity_deaths[MAX_REPORTED_ENTITY_DEATHS];
    /* used for displaying what tiles you can walk to. */
    u8*                              combat_movement_visibility_map;
    /* We segregate the battle squares into islands */
    /* this is for the visual effect primarily. (fade everything not in this zone to black, or treat the squares as a giant distance field) */
    struct level_area_battle_zone_bounding_box* first_battle_zone;
    struct level_area_battle_zone_bounding_box* last_battle_zone;
    s32                                         battle_zone_count;

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

        if (tile_index >= TILE_LAYER_SCRIPTABLE_0 && tile_index <= TILE_LAYER_SCRIPTABLE_31) {
            struct scriptable_tile_layer_property* layer_properties = area->scriptable_layer_properties + (tile_layer - TILE_LAYER_SCRIPTABLE_0);

            if (layer_properties->flags & SCRIPTABLE_TILE_LAYER_FLAGS_HIDDEN) {
                return NULL;
            }

            f32 tile_xf32 = tile_x + layer_properties->offset_x;
            f32 tile_yf32 = tile_y + layer_properties->offset_y;

            tile_x = roundf(tile_xf32);
            tile_y = roundf(tile_yf32);
        }

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

    /* check qualifying scriptable layers */
    {
        for (s32 layer_index = TILE_LAYER_SCRIPTABLE_0; layer_index <= TILE_LAYER_SCRIPTABLE_31; ++layer_index) {
            struct scriptable_tile_layer_property* layer_properties = area->scriptable_layer_properties + (layer_index - TILE_LAYER_SCRIPTABLE_0);

            if (layer_properties->draw_layer != TILE_LAYER_OBJECT) {
                continue;
            }

            if (layer_properties->flags & SCRIPTABLE_TILE_LAYER_FLAGS_HIDDEN) {
                continue;
            }

            if (layer_properties->flags & SCRIPTABLE_TILE_LAYER_FLAGS_NOCOLLIDE) {
                continue;
            }

            if (level_area_get_tile_at(area, layer_index, x, y)) {
                return true;
            }
        }
    }

    return false;
}

/* this thing is variably sized, so it needs an arena */
/* also because the game doesn't use dynamic memory often, there has to be a slightly different allocator and container for the editor version */
/* void serialize_tilemap_object_level_editor(struct binary_serializer* serializer, s32 version, struct level_area_tilemap_object* tilemap_object, struct memory_arena* arena); */

void serialize_tile(struct binary_serializer* serializer, s32 version, struct tile* tile);
void serialize_tile_layer(struct binary_serializer* serializer, s32 version, s32* counter, struct tile* tile);
void serialize_battle_safe_square(struct binary_serializer* serializer, s32 version, struct level_area_battle_safe_square* square);
void serialize_level_area_entity_savepoint(struct binary_serializer* serializer, s32 version, struct level_area_savepoint* entity);
void serialize_generic_trigger(struct binary_serializer* serializer, s32 version, struct trigger* trigger);
void serialize_trigger_level_transition(struct binary_serializer* serializer, s32 version, struct trigger_level_transition* trigger);
void serialize_level_area_entity(struct binary_serializer* serializer, s32 version, struct level_area_entity* entity);
#endif
