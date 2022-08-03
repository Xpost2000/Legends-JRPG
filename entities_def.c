/* includes tiles and general entity types */
#ifndef ENTITY_DEF_C
#define ENTITY_DEF_C

#include "entity_stat_block_def.c"

typedef struct entity_id {
    s32 index;
    s32 generation;
} entity_id;

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
    ENTITY_FLAGS_HIDDEN            = BIT(31),
};

/* generousity is well rewarded */
#define MAX_PARTY_ITEMS (4096)
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
/* TODO implement */
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
};

struct entity_ai_data {
    /*
      NOTE()
      
      This should not actually be combat actions... However I'm not using
      it for anything else right now.
    */
    s32                           current_action;

    u32                           flags;

    bool                          following_path;
    struct entity_navigation_path navigation_path;
    s32                           current_path_point_index;

    s32                             tracked_attacker_write_cursor;
    struct entity_ai_attack_tracker tracked_attackers[MAXIMUM_REMEMBERED_ATTACKERS];

    /* TODO DEBUG */
    f32 wait_timer;
};

struct entity {
    string name;
    /* These two fields should define an AABB */
    /* actual visual information is for now just handled by the rendering procedure */
    /* TODO make this centered */
    v2f32 position;
    v2f32 scale;
    v2f32 velocity;
    u32   flags;

    /*
      TODO: store a team data block to identify across factions.
     */
    u32   team_id;

    struct entity_actor_inventory inventory;

    s32_range                     health;
    struct entity_stat_block      stat_block;

    struct entity_ai_data         ai;
    bool                          waiting_on_turn;
};

void entity_inventory_use_item(struct entity_inventory* inventory, s32 item_index, struct entity* target);

void entity_combat_submit_movement_action(struct entity* entity, v2f32* path_points, s32 path_count);
#if 0
void entity_combat_submit_item_use_action(struct entity* entity, s32 item_index, struct entity* target);
#endif

struct entity_list {
    s32*           generation_count;
    struct entity* entities;
    /* s32            count; */
    s32            capacity;
};

struct entity_list entity_list_create(struct memory_arena* arena, s32 capacity);
entity_id          entity_list_create_entity(struct entity_list* entities);
entity_id          entity_list_get_id(struct entity_list* entities, s32 index);
struct entity*     entity_list_dereference_entity(struct entity_list* entities, entity_id id);

void                 entity_list_update_entities(struct game_state* state, struct entity_list* entities, f32 dt, struct level_area* area);
void                 entity_list_render_entities(struct entity_list* entities, struct graphics_assets* graphics_assets, struct render_commands* commands, f32 dt);
struct rectangle_f32 entity_rectangle_collision_bounds(struct entity* entity);

struct entity_query_list {
    s32* indices;
    s32  count;
};
struct entity_query_list find_entities_within_radius(struct memory_arena* arena, struct entity_list* list, v2f32 position, f32 radius);

#endif
