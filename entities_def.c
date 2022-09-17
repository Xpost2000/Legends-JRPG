/* includes tiles and general entity types */
#ifndef ENTITY_DEF_C
#define ENTITY_DEF_C

#define MAX_SELECTED_ENTITIES_FOR_ABILITIES (GAME_MAX_PERMENANT_ENTITIES + 256)

#include "entity_stat_block_def.c"

/* This needs to be augmented more... Oh well. Not my issue. */
/* We only really have one of each list type, so I could store the list
   pointer itself, but that don't fit in small things. */
enum entity_list_storage_type {
    ENTITY_LIST_STORAGE_TYPE_PERMENANT_STORE,
    /* for summons, but otherwise stuff that is expected to disappear. */
    ENTITY_LIST_STORAGE_TYPE_TEMPORARY_STORE,
    /* for levels which are disappearing. */
    ENTITY_LIST_STORAGE_TYPE_PER_LEVEL,
};
typedef struct entity_id {
    /* use this flag to reference a companion instead. */
    /* the game does not know how to handle this yet. */
    union {
        /* 
           64 bit id is unnecessary. But too lazy to bitmask
           
           So this generally reduces code changes.
        */

        /*
          NOTE: these flags / extra bits do not influence the look up from entity list!
          The flags are checked individually in the game code.
         */
        struct {
            /* though not sure if this is ever important as the script code has special handle
               types for permenant entities. (PERM-ENT 0) == PLAYER, and stuff like that */
            u8  store_type;
            u8  flags[3];
            u32  index;
        };
        u64 full_id;
    };
    s32  generation;
} entity_id;
bool entity_id_equal(entity_id a, entity_id b) {
    /* same list origin */
    if (a.full_id == b.full_id) {
        if (a.generation == b.generation) {
            return true;
        }
    }

    return false;
}

#include "entity_ability_def.c"
/* forward decl */
void battle_notify_killed_entity(entity_id);

enum entity_ai_flags {
    ENTITY_AI_FLAGS_NONE                 = 0,
    ENTITY_AI_FLAGS_AGGRESSIVE_TO_PLAYER = BIT(0),
    ENTITY_AI_FLAGS_AGGRESSIVE_TO_OTHERS = BIT(1),
    ENTITY_AI_FLAGS_FRIENDLY_TO_PLAYER   = BIT(2),
    ENTITY_AI_FLAGS_FRIENDLY_TO_OTHERS   = BIT(3),
    ENTITY_AI_FLAGS_NEUTRAL_TO_OTHERS    = BIT(4),
    ENTITY_AI_FLAGS_NEUTRAL_TO_PLAYER    = BIT(5),
};

enum entity_flags {
    ENTITY_FLAGS_NONE              = 0,
    
    /* entity is allocated */
    ENTITY_FLAGS_ACTIVE            = BIT(0),

    ENTITY_FLAGS_ALIVE             = BIT(1),

    ENTITY_FLAGS_NOCLIP            = BIT(2),

    /* no multiplayer, only default player bindings */
    ENTITY_FLAGS_PLAYER_CONTROLLED = BIT(3),

    /*
     * ....
     */
    ENTITY_FLAGS_HIDDEN            = BIT(31), /*TODO not used yet*/

    ENTITY_FLAGS_RUNTIME_RESERVATION = (ENTITY_FLAGS_ACTIVE | ENTITY_FLAGS_ALIVE | ENTITY_FLAGS_NOCLIP | ENTITY_FLAGS_PLAYER_CONTROLLED),
};

/* generousity is well rewarded */
#define MAX_PARTY_ITEMS (8192)
struct player_party_inventory {
    s32           item_count;
    struct item_instance items[MAX_PARTY_ITEMS];
};

/* NOTE for AI use in combat? */
#define MAX_ACTOR_AVALIABLE_ITEMS (8)
struct entity_actor_inventory {
    s32           item_count;
    struct item_instance items[MAX_ACTOR_AVALIABLE_ITEMS];
};

struct entity_chest_inventory {
    s32           item_count;
    struct item_instance items[16];
};

/* both types above cast into this, and hopefully decay correctly without exploding */
struct entity_inventory{s32 count; struct item_instance items[1];};
void                  entity_inventory_add(struct entity_inventory* inventory, s32 limits, item_id item);
void                  entity_inventory_add_multiple(struct entity_inventory* inventory, s32 limits, item_id item, s32 count);
void                  entity_inventory_remove_item(struct entity_inventory* inventory, s32 item_index, bool remove_all);
bool                  entity_inventory_has_item(struct entity_inventory* inventory, item_id item);

/* assume only player/npc style entity for now */
/* so every entity is really just a guy. We don't care about anything else at the moment. */
enum entity_chest_flags {
    ENTITY_CHEST_NONE                           = 0,
    ENTITY_CHEST_FLAGS_REQUIRES_ITEM_FOR_UNLOCK = BIT(0),
    ENTITY_CHEST_FLAGS_UNLOCKED                 = BIT(1), /* redundant if it does not require an item for unlock */
    ENTITY_CHEST_FLAGS_HIDDEN                   = ENTITY_FLAGS_HIDDEN,
};
/* in tiles */
#define ENTITY_CHEST_INTERACTIVE_RADIUS ((f32)1.8565 * TILE_UNIT_SIZE)
struct entity_chest {
    v2f32                         position;
    v2f32                         scale;
    u32                           flags;
    struct entity_chest_inventory inventory;
    item_id                       key_item;
};
struct entity_navigation_path {
    /* NOTE I am trying to design the game so that this is not needed... Implement this later if needed. */
#if 0
    v2f32 end_point; /* so we can refetch the path if need be. */
    s32   full_path_count;
#endif
    s32   count;
    /* paths are supposed to be based on tile coordinates anyways... */
    /* although hopefully we aren't using this up... */
    v2f32 path_points[64];
};

struct entity_ai_attack_tracker {
    entity_id attacker;
    u16       times;
};
#define MAXIMUM_REMEMBERED_ATTACKERS (32)

enum entity_combat_action {
    ENTITY_ACTION_NONE,
    ENTITY_ACTION_MOVEMENT,
    ENTITY_ACTION_ATTACK,
    ENTITY_ACTION_ABILITY,
    /* ENTITY_ACTION_SKIP_TURN, */
};

struct entity_ai_data {
    /*
      NOTE()
      
      This should not actually be combat actions... However I'm not using
      it for anything else right now.
    */
    s32                           current_action;

    u32                           flags;
    entity_id                     attack_target_id;

    bool                          following_path;
    struct entity_navigation_path navigation_path;
    s32                           current_path_point_index;

    /* TODO, unused! */
    s32                             tracked_attacker_write_cursor;
    struct entity_ai_attack_tracker tracked_attackers[MAXIMUM_REMEMBERED_ATTACKERS];

    /* for abilities */
    s32                             targeted_entity_count;
    entity_id                       targeted_entities[MAX_SELECTED_ENTITIES_FOR_ABILITIES];
    s32                             using_ability_index;

    /* used for determining when to aggro. */
    s32                             aggro_tolerance;

    f32 wait_timer;
};

struct entity_animation_state {
    string name; /* look up name */

    s32    current_frame_index;
    s32    iterations;
    f32    timer;
};

enum entity_equip_slot_index {
    ENTITY_EQUIP_SLOT_INDEX_HEAD,
    ENTITY_EQUIP_SLOT_INDEX_CHEST,
    ENTITY_EQUIP_SLOT_INDEX_HANDS,
    ENTITY_EQUIP_SLOT_INDEX_LEGS,
    ENTITY_EQUIP_SLOT_INDEX_ACCESSORY1,
    ENTITY_EQUIP_SLOT_INDEX_ACCESSORY2,
    ENTITY_EQUIP_SLOT_INDEX_WEAPON1,
    ENTITY_EQUIP_SLOT_INDEX_COUNT,
};

#define ENTITY_MAX_ABILITIES (2048)

/* time information I guess */
/* mostly used by animation sequences or whatever we need to animate */
/* NOTE: comeback later. */

/*
  Sequences are expected to be linear btw, we don't need overkill animations like
  Disgaea... Though I really want that...
 */
struct entity_sequence_state {
    /* This should be an array technically for more involved animations but who cares I guess? */
    v2f32 start_position; 

    bool initialized_state;
    v2f32 start_position_interpolation;
    v2f32 end_position_interpolation;

    /* primarily using for interpolation */
    f32 time;
    s32 current_sequence_index;
};

struct entity {
    string name;

    /* used for abilities primarily. */
    struct entity_sequence_state  sequence_state;
    struct entity_animation_state animation;

    s32   base_id_index; /* use this to look up some shared information */
                         /* that isn't intended to change. Mostly just for the loot tables. */
    s32   loot_table_id_index;

    s32   model_index;
    u8    facing_direction;

    v2f32 position;
    v2f32 scale;
    v2f32 velocity;
    u32   flags;

    /*
      TODO: store a team data block to identify across factions.
     */
    u32   team_id;

    struct entity_actor_inventory inventory;

    union {
        struct {
            item_id head;
            item_id chest;
            item_id hands;
            item_id legs;
            item_id accessory1;
            item_id accessory2;
            item_id weapon1;
        };
        item_id equip_slots[ENTITY_EQUIP_SLOT_INDEX_COUNT];
    };

    s16 ability_count;
    struct entity_ability_slot abilities[ENTITY_MAX_ABILITIES];

    s32_range                     health;
    s32_range                     magic;
    struct entity_stat_block      stat_block;

    /* Will search in dlg text */
    bool                          has_dialogue;
    char                          dialogue_file[64];
    /* mostly runtime data. */
    struct entity_ai_data         ai;
    bool                          waiting_on_turn;

    /* to avoid double fires */
    /* NOTE: ids are internally (index+1), it's a bit confusing as the editor and engine otherwise have id as 0 indices */
    /* this is just so I can zero out this thing and have expected behavior. */
    s32                           interacted_script_trigger_write_index;
    s32                           interacted_script_trigger_ids[32];
};

void entity_snap_to_grid_position(struct entity* entity) {
    v2f32 position   = entity->position;
    position.x       = floorf(position.x / TILE_UNIT_SIZE) * TILE_UNIT_SIZE;
    position.y       = floorf(position.y / TILE_UNIT_SIZE) * TILE_UNIT_SIZE;
    entity->position = position;
}

void entity_add_ability_by_name(struct entity* entity, string id);

void entity_set_dialogue_file(struct entity* entity, string str) {
    
}

#define ENTITY_MAX_LOOT_TABLE_ENTRIES (32)
struct entity_loot {
    item_id   item;
    s32       count_min;
    s32       count_max;
    f32       normalized_chance; /* 0 - 1 */
};
struct entity_loot_table {
    s32                loot_count;
    struct entity_loot loot_items[ENTITY_MAX_LOOT_TABLE_ENTRIES];
};

/* temporary and copy somewhere. */
struct item_instance* entity_loot_table_find_loot(struct memory_arena* arena, struct entity_loot_table* table, s32* out_count);

struct entity_base_data {
    string                        name;
    s32                           model_index;
    u32                           flags;
    u32                           ai_flags;
    struct entity_stat_block      stats;
    s32                           health;
    s32                           magic;
    s32                           loot_table_id_index; /* if -1, don't use loot table. Need to have override in level_area_entity */
    item_id                       equip_slots[ENTITY_EQUIP_SLOT_INDEX_COUNT];
    struct entity_actor_inventory inventory;
};
struct entity_database {
    struct memory_arena*            arena;
    s32                             entity_capacity;
    s32                             entity_count;
    s32                             loot_table_capacity;
    s32                             loot_table_count;
    s32                             ability_capacity;
    s32                             ability_count;
    /* I think I really should be hashing a lot of things I do this for. TODO. Can always change */
    string*                         entity_key_strings;
    string*                         loot_table_key_strings;
    string*                         ability_key_strings;
    struct entity_ability*          abilities;
    struct entity_base_data*        entities;
    struct entity_loot_table*       loot_tables;
};

struct entity_loot_table* entity_lookup_loot_table(struct entity_database* entity_database, struct entity* entity);
/* for debug reasons, in reality it is always built from a file. */

void   entity_base_data_unpack(struct entity_database* entity_database, struct entity_base_data* data, struct entity* unpack_destination);

struct entity_database    entity_database_create(struct memory_arena* arena);
struct entity_base_data*  entity_database_find_by_index(struct entity_database* entity_database, s32 index);
struct entity_base_data*  entity_database_find_by_name(struct entity_database* entity_database, string name);
struct entity_loot_table* entity_database_loot_table_find_by_index(struct entity_database* entity_database, s32 index);
struct entity_loot_table* entity_database_loot_table_find_by_name(struct entity_database* entity_database, string name);
struct entity_ability*    entity_database_ability_find_by_index(struct entity_database* entity_database, s32 index);
struct entity_ability*    entity_database_ability_find_by_name(struct entity_database* entity_database, string name);
s32                       entity_database_find_id_by_name(struct entity_database* entity_database, string name);
s32                       entity_database_loot_table_find_id_by_name(struct entity_database* entity_database, string name);
s32                       entity_database_ability_find_id_by_name(struct entity_database* entity_database, string name);

bool is_entity_aggressive_to_player(struct entity* entity);
void entity_play_animation(struct entity* entity, string name);

s32 entity_find_effective_stat_value(struct entity* entity, s32 stat_index);

/* these don't do error checking, they assume the item index is within bounds */
/* NOTE: The inventory does not know it's own limits, since I use type-punning to allow these to work on different fixed sized instances. */
void entity_inventory_equip_item(struct entity_inventory* inventory, s32 limits, s32 item_index, s32 equipment_index, struct entity* target);
void entity_inventory_unequip_item(struct entity_inventory* inventory, s32 limits, s32 equipment_index, struct entity* target);
void entity_inventory_use_item(struct entity_inventory* inventory, s32 item_index, struct entity* target);

s32  entity_inventory_count_instances_of(struct entity_inventory* inventory, string item_name);
s32  entity_inventory_get_gold_count(struct entity_inventory* inventory);
void entity_inventory_set_gold_count(struct entity_inventory* inventory, s32 amount);

void entity_combat_submit_movement_action(struct entity* entity, v2f32* path_points, s32 path_count);
void entity_combat_submit_attack_action(struct entity* entity, entity_id target_id);

/* This only obviously allows you to use abilities that you have. Cannot force them! */
/* though that'd be an interesting thing to allow. */
void entity_combat_submit_ability_action(struct entity* entity, entity_id* targets, s32 target_count, s32 user_ability_index);
#if 0
void entity_combat_submit_item_use_action(struct entity* entity, s32 item_index, struct entity* target);
#endif

struct entity_list {
    u8             store_type;
    s32*           generation_count;
    struct entity* entities;
    s32            capacity;
};

#define MAX_ENTITY_LIST_COUNT (32)
struct entity_iterator {
    u8  done;
    s32 list_count;
    s32 index;
    s32 entity_list_index;
    entity_id current_id;
    struct entity_list* entity_lists[MAX_ENTITY_LIST_COUNT];
};

struct entity_iterator entity_iterator_create(void);
struct entity_iterator entity_iterator_clone(struct entity_iterator* base);
void                   entity_iterator_push(struct entity_iterator* iterator, struct entity_list* list);
struct entity*         entity_iterator_begin(struct entity_iterator* iterator);
bool                   entity_iterator_finished(struct entity_iterator* iterator);
s32                    entity_iterator_count_all_entities(struct entity_iterator* entities);
struct entity*         entity_iterator_advance(struct entity_iterator* iterator);


struct entity_list entity_list_create(struct memory_arena* arena, s32 capacity, u8 storage_mark);
struct entity_list entity_list_create_top(struct memory_arena* arena, s32 capacity, u8 storage_mark);
entity_id          entity_list_create_entity(struct entity_list* entities);
entity_id          entity_list_get_id(struct entity_list* entities, s32 index);
struct entity*     entity_list_dereference_entity(struct entity_list* entities, entity_id id);
void               entity_list_clear(struct entity_list* entities);
bool               entity_bad_ref(struct entity* e);

void               update_entities(struct game_state* state, f32 dt, struct level_area* area);
void               render_entities(struct game_state* state, struct graphics_assets* graphics_assets, struct render_commands* commands, f32 dt);

struct rectangle_f32 entity_rectangle_collision_bounds(struct entity* entity);

struct entity_query_list {
    entity_id* ids;
    s32        count;
};
struct entity_query_list find_entities_within_radius(struct memory_arena* arena, struct game_state* state, v2f32 position, f32 radius);

#endif
