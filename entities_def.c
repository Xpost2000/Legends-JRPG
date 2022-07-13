/* includes tiles and general entity types */
#ifndef ENTITY_DEF_C
#define ENTITY_DEF_C

enum entity_flags {
    ENTITY_FLAGS_NONE              = 0,
    
    /* entity is allocated */
    ENTITY_FLAGS_ACTIVE            = BIT(0),

    ENTITY_FLAGS_ALIVE             = BIT(1),

    ENTITY_FLAGS_NOCLIP            = BIT(2),

    /* no multiplayer, only default player bindings */
    ENTITY_FLAGS_PLAYER_CONTROLLED = BIT(3),
};

#define MAX_PARTY_ITEMS (512)
struct player_party_inventory {
    s32           item_count;
    struct item_instance items[MAX_PARTY_ITEMS];
};

/* NOTE for AI use in combat? */
#define MAX_ACTOR_AVALIABLE_ITEMS (4)
struct entity_actor_inventory {
    s32           item_count;
    struct item_instance items[MAX_ACTOR_AVALIABLE_ITEMS];
};

/* assume only player/npc style entity for now */
/* so every entity is really just a guy. We don't care about anything else at the moment. */
struct entity {
    /* These two fields should define an AABB */
    /* actual visual information is for now just handled by the rendering procedure */
    /* TODO make this centered */
    v2f32 position;
    v2f32 scale;
    v2f32 velocity;
    u32   flags;

    struct entity_actor_inventory inventory;
    s32_range                     health;
};

typedef struct entity_id {
    s32 index;
    s32 generation;
} entity_id;

struct entity_list {
    s32*           generation_count;
    struct entity* entities;
    /* s32            count; */
    s32            capacity;
};

struct entity_list entity_list_create(struct memory_arena* arena, s32 capacity);
entity_id          entity_list_create_entity(struct entity_list* entities);
struct entity*     entity_list_dereference_entity(struct entity_list* entities, entity_id id);

void                 entity_list_update_entities(struct game_state* state, struct entity_list* entities, f32 dt, struct level_area* area);
void                 entity_list_render_entities(struct entity_list* entities, struct graphics_assets* graphics_assets, struct render_commands* commands, f32 dt);
struct rectangle_f32 entity_rectangle_collision_bounds(struct entity* entity);

#endif
