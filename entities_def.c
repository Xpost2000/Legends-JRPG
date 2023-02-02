/*
  Includes anything that may vaguely be considered an entity like a particle system.

  Not lights apparently. Those don't count for some reason!
*/
#define DEFAULT_VELOCITY (TILE_UNIT_SIZE * 5)
#define DEFAULT_ENTITY_ATTACK_RADIUS (3)

#ifndef ENTITY_DEF_C
#define ENTITY_DEF_C

#define MAX_SELECTED_ENTITIES_FOR_ABILITIES (GAME_MAX_PERMENANT_ENTITIES + 32)

#include "entity_stat_block_def.c"
#define MAX_ENTITY_LIST_COUNT (8)

/* This needs to be augmented more... Oh well. Not my issue. */
/* We only really have one of each list type, so I could store the list
   pointer itself, but that don't fit in small things. */
enum entity_list_storage_type {
    ENTITY_LIST_STORAGE_TYPE_PERMENANT_STORE,
    /* for summons, but otherwise stuff that is expected to disappear. */
    ENTITY_LIST_STORAGE_TYPE_TEMPORARY_STORE,
    /* for levels which are disappearing. */
    ENTITY_LIST_STORAGE_TYPE_PER_LEVEL,
    /* cutscene loaded levels, these entities are even more temporary */
    ENTITY_LIST_STORAGE_TYPE_PER_LEVEL_CUTSCENE,
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
    if (a.store_type == b.store_type && a.index == b.index) {
        if (a.generation == b.generation) {
            return true;
        }
    }

    return false;
}

struct entity_list {
    u8             store_type;
    s32*           generation_count;
    struct entity* entities;
    s32            capacity;
};

struct entity_particle_list {
    s32 capacity;
    struct entity_particle* particles;
};

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

/*remove later?*/
struct entity_iterator game_entity_iterator(struct game_state*);
/*remove later?*/

struct entity_list entity_list_create(struct memory_arena* arena, s32 capacity, u8 storage_mark);
struct entity_list entity_list_clone(struct memory_arena* arena, struct entity_list original);
void               entity_list_copy(struct entity_list* source, struct entity_list* destination);
entity_id          entity_list_create_entity(struct entity_list* entities);
entity_id          entity_list_get_id(struct entity_list* entities, s32 index);
entity_id          entity_list_find_entity_id_with_scriptname(struct entity_list* list, string scriptname);
struct entity*     entity_list_dereference_entity(struct entity_list* entities, entity_id id);
void               entity_list_clear(struct entity_list* entities);
bool               entity_bad_ref(struct entity* e);

#include "entity_status_effects_def.c"
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
    ENTITY_FLAGS_HIDDEN            = BIT(31),

    ENTITY_FLAGS_RUNTIME_RESERVATION = (ENTITY_FLAGS_ACTIVE | ENTITY_FLAGS_ALIVE | ENTITY_FLAGS_NOCLIP | ENTITY_FLAGS_PLAYER_CONTROLLED),
};

/* generousity is well rewarded */
#define MAX_PARTY_ITEMS (1000) /*lol*/
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
    s32           item_count       SERIALIZE_VERSIONS(level, 2 to CURRENT);
    struct item_instance items[16] SERIALIZE_VERSIONS(level, 2 to CURRENT);
};

/* both types above cast into this, and hopefully decay correctly without exploding */
struct entity_inventory{s32 count; struct item_instance items[1];};
void                  entity_inventory_add(struct entity_inventory* inventory, s32 limits, item_id item);
void                  entity_inventory_add_multiple(struct entity_inventory* inventory, s32 limits, item_id item, s32 count);
void                  entity_inventory_remove_item(struct entity_inventory* inventory, s32 item_index, bool remove_all);
void                  entity_inventory_remove_item_by_name(struct entity_inventory* inventory, string item, bool remove_all);
bool                  entity_inventory_has_item(struct entity_inventory* inventory, item_id item);

/* assume only player/npc style entity for now */
/* so every entity is really just a guy. We don't care about anything else at the moment. */
enum entity_chest_flags {
    ENTITY_CHEST_NONE                           = 0,
    ENTITY_CHEST_FLAGS_REQUIRES_ITEM_FOR_UNLOCK = BIT(0),
    ENTITY_CHEST_FLAGS_UNLOCKED                 = BIT(1), /* redundant if it does not require an item for unlock */
    ENTITY_CHEST_FLAGS_HIDDEN                   = BIT(31),
};
/* in tiles */
#define ENTITY_CHEST_INTERACTIVE_RADIUS ((f32)1.8565 * TILE_UNIT_SIZE)
struct entity_chest {
    v2f32                         position  SERIALIZE_VERSIONS(level, 2 to CURRENT);
    v2f32                         scale     SERIALIZE_VERSIONS(level, 2 to CURRENT);
    u32                           flags     SERIALIZE_VERSIONS(level, 2 to CURRENT) SERIALIZE_VERSIONS(save, 1 to CURRENT);
    struct entity_chest_inventory inventory SERIALIZE_VERSIONS(level, 2 to CURRENT);
    item_id                       key_item  SERIALIZE_VERSIONS(level, 2 to CURRENT);
};

struct entity_chest_list {
    s32 capacity;
    s32 count;
    struct entity_chest* chests;
};

struct entity_chest_list entity_chest_list_reserved(struct memory_arena* arena, s32 capacity);
struct entity_chest*     entity_chest_list_push(struct entity_chest_list* list);
struct entity_chest*     entity_chest_list_find_at(struct entity_chest_list* list, v2f32 position);
void                     entity_chest_list_remove(struct entity_chest_list* list, s32 index);
void                     entity_chest_list_clear(struct entity_chest_list* list);

enum entity_savepoint_flags {
    ENTITY_SAVEPOINT_FLAGS_NONE     = 0,
    ENTITY_SAVEPOINT_FLAGS_DISABLED = BIT(31),
    ENTITY_SAVEPOINT_FLAGS_HIDDEN   = BIT(31),
};
#define ENTITY_SAVEPOINT_INTERACTIVE_RADIUS ((f32)1.9565 * TILE_UNIT_SIZE)
struct entity_savepoint {
    v2f32 position;
    u32   flags;
    s32   particle_emitter_id;
    bool  player_ontop;
};

void entity_savepoint_initialize(struct entity_savepoint* savepoint);

/* might need to obey the simulated rules of a system but who cares? */
/* reserved for in engine dynamic things, so burning and such. (or particle emitters that are dynamically spawned from other entities) */
/* levels will still have their own particle emitters if they would like. */

/*
  NOTE: This system allows you to spawn particles on your own for custom reasons,
  so technically you don't need particle emitters as they are. They're just a formal
  generic way to make quick particles. For better control I highly recommend honestly
  just specifying the way you want particles to spawn and allocate them based on your own
  objects...
*/

#define GAME_MAX_PERMENANT_PARTICLE_EMITTERS (4096) 
#define PARTICLE_POOL_MAX_SIZE (16384) /* Seriously how much memory am I gobbling up? */
/* NOTE(): these are point particle emitters to start with */
enum entity_particle_feature_flags {
    ENTITY_PARTICLE_FEATURE_FLAG_VELOCITY       = BIT(0),
    ENTITY_PARTICLE_FEATURE_FLAG_ACCELERATION   = BIT(1),
    ENTITY_PARTICLE_FEATURE_FLAG_FULLBRIGHT     = BIT(2),
    ENTITY_PARTICLE_FEATURE_FLAG_ADDITIVEBLEND  = BIT(3),
    /* Draw the same thing again but under alpha blending */
    /* hope it looks generally pretty good for flamelike effects */
    ENTITY_PARTICLE_FEATURE_FLAG_FLAMELIKE      = BIT(4),
    ENTITY_PARTICLE_FEATURE_FLAG_ALPHAFADE      = BIT(5),
    ENTITY_PARTICLE_FEATURE_FLAG_HIGHERSORTBIAS = BIT(6),
    ENTITY_PARTICLE_FEATURE_FLAG_COLORFADE      = BIT(7),
    ENTITY_PARTICLE_FEATURE_FLAG_ALWAYSFRONT    = BIT(8),


    /* PARTICLE PROFILE FLAGS */
    ENTITY_PARTICLE_FEATURE_FLAG_GENERIC = ENTITY_PARTICLE_FEATURE_FLAG_VELOCITY | ENTITY_PARTICLE_FEATURE_FLAG_ACCELERATION | ENTITY_PARTICLE_FEATURE_FLAG_ALPHAFADE,
    ENTITY_PARTICLE_FEATURE_FLAG_GLOWING = ENTITY_PARTICLE_FEATURE_FLAG_GENERIC | ENTITY_PARTICLE_FEATURE_FLAG_FULLBRIGHT | ENTITY_PARTICLE_FEATURE_FLAG_ADDITIVEBLEND,
    ENTITY_PARTICLE_FEATURE_FLAG_FLAMES  = (ENTITY_PARTICLE_FEATURE_FLAG_GLOWING | ENTITY_PARTICLE_FEATURE_FLAG_FLAMELIKE | ENTITY_PARTICLE_FEATURE_FLAG_HIGHERSORTBIAS | ENTITY_PARTICLE_FEATURE_FLAG_COLORFADE) & ~ENTITY_PARTICLE_FEATURE_FLAG_ALPHAFADE,
};

/* UNITS are TILE_UNITS! */
enum entity_particle_flag {
    ENTITY_PARTICLE_FLAGS_NONE = 0,
    ENTITY_PARTICLE_FLAG_ALIVE = BIT(0),
};
struct entity_particle {
    v2f32           position;
    v2f32           scale;
    u32             flags;
    u32             feature_flags;
    s32             associated_particle_emitter_index;
    f32             lifetime;
    f32             lifetime_max;
    union color32u8 color;
    union color32u8 target_color;
    v2f32           velocity;
    v2f32           acceleration;
};

enum entity_particle_emitter_flags{
    ENTITY_PARTICLE_EMITTER_INACTIVE  = 0,
    ENTITY_PARTICLE_EMITTER_ACTIVE    = BIT(0),
    ENTITY_PARTICLE_EMITTER_ON        = BIT(1),
};

/* POSITION UNITS IN TILE UNITS */
#define ENTITY_PARTCLE_EMITTER_SPAWN_SHAPE_MAX_NGON_TRACES (32)

enum entity_particle_emitter_spawn_shape_type {
    ENTITY_PARTICLE_EMITTER_SPAWN_SHAPE_POINT,
    ENTITY_PARTICLE_EMITTER_SPAWN_SHAPE_LINE,
    ENTITY_PARTICLE_EMITTER_SPAWN_SHAPE_RECTANGLE,
    ENTITY_PARTICLE_EMITTER_SPAWN_SHAPE_CIRCLE,
};

/* NOTE: these positions are offsets to the actual particle emitter position */
struct entity_particle_emitter_spawn_shape_point {
    v2f32 center;
    f32 thickness;
};
struct entity_particle_emitter_spawn_shape_line {
    v2f32 start;
    v2f32 end;
    f32 thickness;
};
struct entity_particle_emitter_spawn_shape_rectangle {
    v2f32 center;
    v2f32 half_widths;
    f32 thickness;
};
struct entity_particle_emitter_spawn_shape_circle {
    v2f32 center;
    f32 radius;
    f32 thickness;
};

struct entity_particle_emitter_spawn_shape {
    s32 type;

    bool  outline;
    union {
        struct entity_particle_emitter_spawn_shape_point     point;
        struct entity_particle_emitter_spawn_shape_line      line;
        struct entity_particle_emitter_spawn_shape_rectangle rectangle;
        struct entity_particle_emitter_spawn_shape_circle    circle;
    };
};

local struct entity_particle_emitter_spawn_shape emitter_spawn_shape_point(v2f32 xy, f32 thickness) {
    return (struct entity_particle_emitter_spawn_shape) {
        .type            = ENTITY_PARTICLE_EMITTER_SPAWN_SHAPE_POINT,
        .point.center    = xy,
        .point.thickness = thickness,
    };
}

local struct entity_particle_emitter_spawn_shape emitter_spawn_shape_line(v2f32 start, v2f32 end, f32 thickness) {
    return (struct entity_particle_emitter_spawn_shape) {
        .type       = ENTITY_PARTICLE_EMITTER_SPAWN_SHAPE_LINE,
        .line.start = start,
        .line.end   = end,
    };
}

local struct entity_particle_emitter_spawn_shape emitter_spawn_shape_rectangle(v2f32 center, v2f32 half_widths, f32 thickness, bool outline) {
    return (struct entity_particle_emitter_spawn_shape) {
        .type                  = ENTITY_PARTICLE_EMITTER_SPAWN_SHAPE_RECTANGLE,
        .outline               = outline,
        .rectangle.center      = center,
        .rectangle.half_widths = half_widths,
        .rectangle.thickness   = thickness,
    };
}

local struct entity_particle_emitter_spawn_shape emitter_spawn_shape_circle(v2f32 center, f32 radius, f32 thickness, bool outline) {
    return (struct entity_particle_emitter_spawn_shape) {
        .type             = ENTITY_PARTICLE_EMITTER_SPAWN_SHAPE_CIRCLE,
        .outline          = outline,
        .circle.center    = center,
        .circle.radius    = radius,
        .circle.thickness = thickness,
    };
}

struct entity_particle_emitter {
    v2f32 position;

    struct entity_particle_emitter_spawn_shape spawn_shape;

    f32   time;
    f32   time_per_spawn;

    u32 flags;
    /* 1or0 == normal... */
    s32   burst_amount;

    f32   delay_time;
    f32   delay_time_per_batch;

    /* f32 time_until_death; // 0, means  */

    s32 currently_spawned_batch_amount;
    s32 max_spawn_per_batch; // cannot be -1

    s32 spawned_batches;
    s32 max_spawn_batches; // -1 == forever!

    /* get schema listing data elsewhere, for now hard code */
    /* this will determine general spawning traits */

    u32 particle_feature_flags; /* some particle types might override stuff... */

    v2f32 starting_velocity;
    v2f32 starting_velocity_variance;

    v2f32 starting_acceleration;
    v2f32 starting_acceleration_variance;

    f32   lifetime;
    f32   lifetime_variance;

    f32   scale_uniform;
    f32   scale_variance_uniform;

    union color32u8 color;
    union color32u8 target_color;
};
struct entity_particle_emitter_list {
    s32 capacity;
    struct entity_particle_emitter* emitters;
};
struct entity_particle_emitter_list entity_particle_emitter_list(struct memory_arena* arena, s32 capacity);
struct entity_particle_emitter_list entity_particle_emitter_list_clone(struct memory_arena* arena, struct entity_particle_emitter_list original);
void                                entity_particle_emitter_list_copy(struct entity_particle_emitter_list* source, struct entity_particle_emitter_list* destination);
/* NOTE: Going  */
void                                initialize_particle_pools(struct memory_arena* arena, s32 particles_total_count);
void                                entity_particle_emitter_list_update(struct entity_particle_emitter_list* particle_emitters, f32 dt);
void                                entity_particle_emitter_kill_all_particles(s32 particle_emitter_id);

void                                entity_particle_emitter_kill(struct entity_particle_emitter_list* emitters, s32 particle_emitter_id);
void                                entity_particle_emitter_kill_all(struct entity_particle_emitter_list* emitters);
void                                entity_particle_emitter_start_emitting(struct entity_particle_emitter_list* emitters, s32 particle_emitter_id);
void                                entity_particle_emitter_stop_emitting(struct entity_particle_emitter_list* emitters, s32 particle_emitter_id);
s32                                 entity_particle_emitter_allocate(struct entity_particle_emitter_list* emitters);
void                                entity_particle_emitter_retain(struct entity_particle_emitter_list* emitters, s32 particle_emitter_id);
struct entity_particle_emitter*     entity_particle_emitter_dereference(struct entity_particle_emitter_list* emitters, s32 particle_emitter_id);

/* needs more parameters */
void entity_particle_emitter_spawn(struct entity_particle_emitter_list* particle_emitter);

struct entity_navigation_path {
    /* NOTE I am trying to design the game so that this is not needed... Implement this later if needed. */
#if 0
    v2f32 end_point; /* so we can refetch the path if need be. */
    s32   full_path_count;
#endif
    /* paths are supposed to be based on tile coordinates anyways... */
    /* although hopefully we aren't using this up... */
    v2f32 path_points[16];
    s32   count;
};

struct entity_ai_attack_tracker {
    entity_id attacker;
    u16       times;
};
#define MAXIMUM_REMEMBERED_ATTACKERS (8)

enum entity_combat_action {
    ENTITY_ACTION_NONE,
    ENTITY_ACTION_MOVEMENT,
    ENTITY_ACTION_ATTACK,
    ENTITY_ACTION_ABILITY,
    ENTITY_ACTION_ITEM,
    /* ENTITY_ACTION_SKIP_TURN, */
};

enum entity_attack_animation_phase {
    ENTITY_ATTACK_ANIMATION_PHASE_MOVE_TO_TARGET,
    ENTITY_ATTACK_ANIMATION_PHASE_REEL_BACK,
    ENTITY_ATTACK_ANIMATION_PHASE_HIT,
    ENTITY_ATTACK_ANIMATION_PHASE_RECOVER_FROM_HIT,
};

enum hurt_animation_phase {
    ENTITY_HURT_ANIMATION_OFF,
    ENTITY_HURT_ANIMATION_ON,
};

/* personally this should depend on the entity... */
/* we should have different death types (dust fade...) (explosion...) (gore explosion...) */
enum death_animation_phase {
    DEATH_ANIMATION_KNEEL,
    DEATH_ANIMATION_DIE,
};

#define HURT_ANIMATION_MAX_SHAKE_COUNT (16+1)
#define HURT_ANIMATION_TIME_BETWEEN_SHAKES (0.045)
#define HURT_ANIMATION_MAX_SHAKE_OFFSET (4)
#define DEATH_ANIMATION_MAX_KNEEL_HUFFS (3)
struct entity_ai_data {
    /*
      NOTE()
      
      This should not actually be combat actions... However I'm not using
      it for anything else right now.
    */
    struct entity_navigation_path navigation_path;
    s32                           current_path_point_index;

    entity_id attack_target_id;
    s32       current_action;
    s32       used_item_index;
    u32       flags;

    /* for item usage */
    u8 sourced_from_player_inventory;
    u8 following_path;

    /* for movement */
    s32                             tracked_attacker_write_cursor;
    struct entity_ai_attack_tracker tracked_attackers[MAXIMUM_REMEMBERED_ATTACKERS]; /* TODO, unused! */

    /* for abilities */
    entity_id                       targeted_entities[MAX_SELECTED_ENTITIES_FOR_ABILITIES];
    s32                             targeted_entity_count;
    s32                             using_ability_index;

    /* TODO, unused used for determining when to aggro. */
    s32                             aggro_tolerance;

    /* HARDCODED ANIMATION_DATA */
    /* hardcoded attack bump animation */
    f32   attack_animation_timer;
    s32   attack_animation_phase;
    v2f32 attack_animation_interpolation_start_position;
    v2f32 attack_animation_interpolation_end_position;
    v2f32 attack_animation_preattack_position;

    /*
      Data for projectile firing since I don't have events on animations for
      this engine
    */
    bool fired_projectile;

    s32   hurt_animation_shakes;
    f32   hurt_animation_timer;
    s32   hurt_animation_phase;
    v2f32 hurt_animation_shake_offset;

    s32 death_animation_phase;
    f32 death_animation_kneel_linger_timer;

    f32 wait_timer;
    s32 used_counter_attacks;
    s32 anim_param[3];
};

struct entity_animation_state {
    char   name_buffer[32];
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

local string entity_equip_slot_index_strings[] = {
    [ENTITY_EQUIP_SLOT_INDEX_HEAD]       = string_literal("head"),
    [ENTITY_EQUIP_SLOT_INDEX_CHEST]      = string_literal("chest"),
    [ENTITY_EQUIP_SLOT_INDEX_HANDS]      = string_literal("hands"),
    [ENTITY_EQUIP_SLOT_INDEX_LEGS]       = string_literal("legs"),
    [ENTITY_EQUIP_SLOT_INDEX_ACCESSORY1] = string_literal("accessory1"),
    [ENTITY_EQUIP_SLOT_INDEX_ACCESSORY2] = string_literal("accessory2"),
    [ENTITY_EQUIP_SLOT_INDEX_WEAPON1]    = string_literal("weapon1"),
};

s32 entity_equip_slot_index_from_string(string id) {
    for (s32 index = 0; index < array_count(entity_equip_slot_index_strings); ++index) {
        if (string_equal(id, entity_equip_slot_index_strings[index])) {
            return index;
        }
    }

    return -1;
}

#define ENTITY_MAX_ABILITIES (512)

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

    struct sequence_action_require_block current_requirements;
};

/*
  I should note this is a list for undoable actions,

  the reason why undoing attacks is not possible is because
  they are randomly rolled, so it would be possible to cheese
  attacks (because you can just undo until you get a critical).

  Item usage doesn't count because items will only do a fixed or otherwise
  deterministic amount of damage without any rolls.

  [NOTE: this can change in the future, so I might specifically filter out specific things]
*/
enum used_battle_action_type {
    LAST_USED_ENTITY_ACTION_NONE,
    LAST_USED_ENTITY_ACTION_MOVEMENT,
    LAST_USED_ENTITY_ACTION_DEFEND,
    LAST_USED_ENTITY_ACTION_ITEM_USAGE,
    LAST_USED_ENTITY_ACTION_COUNT,
};

struct used_battle_action_movement {
    v2f32 last_movement_position;
};

struct used_battle_action_defend {
    /* empty, just here for structure consistency */
};

/*
  Although copying the game state sounds pretty extreme, I promise it's not as bad as it
  sounds.

  We only require copying the entity state basically.

  This is mostly because some items can also affect the "world", or explosives can kill
  guys etc.

  However since items have deterministic results, they should be undoable as it allows for more
  advanced gameplay if someone decides maybe they shouldn't have used something.
*/
/* While particles may remain dormant, that's not too big of a deal to me. */
/* NOTE: Allocated at the top of the game_arena (the same place I store the level data) */
struct used_battle_action_restoration_state {
    s64 memory_arena_marker;
    struct player_party_inventory*      inventory_state;
    struct entity_list                  permenant_entity_state;
    struct entity_particle_emitter_list permenant_particle_emitter_state;
    /* These are not a list type unfortunately lol, have to copy this myself right now */
    struct light_def*                   permenant_lights_state;
    s32                                 permenant_light_count_state;
    struct entity_list                  level_entity_state;
#if 0
    /* NOTE:
       I don't actually directly allocate storage for any particle emitters inside of a level
       since they are all created a runtime by the objects that actually emit particles
    */
    struct entity_particle_emitter_list level_particle_emitter_state;
#endif
    struct light_def*                   level_lights_state;
    s32                                 level_light_count_state;
};

struct used_battle_action_item_usage {
    /* I'll add stuff here as things are affected. */
    s32                                         inventory_item_index; /* this might not be needed, but I'm recording it anyways */
    struct used_battle_action_restoration_state restoration_state;
};

struct used_battle_action {
    u8 type;
    union {
        struct used_battle_action_movement   action_movement;
        struct used_battle_action_defend     action_defend;
        struct used_battle_action_item_usage action_item_usage;
    };
};

struct used_battle_action_stack {
    s32 top;
    struct used_battle_action actions[LAST_USED_ENTITY_ACTION_COUNT];
};

#define ENTITY_TALK_INTERACTIVE_RADIUS ((f32)1.9565 * TILE_UNIT_SIZE)

/* but these structs are here for now to be used later. */
enum projectile_entity_flags {
    PROJECTILE_ENTITY_FLAGS_NONE                  = 0,
    PROJECTILE_ENTITY_FLAGS_ACTIVE                = BIT(1),
    PROJECTILE_ENTITY_FLAGS_COLLIDES_ON_TILE      = BIT(2),
    PROJECTILE_ENTITY_FLAGS_FINISH_TURN_WHEN_DEAD = BIT(3),
};

enum projectile_entity_type {
    /* mostly for visual */
    PROJECTILE_TYPE_SIMPLE, /* a square for testing reasons. */
    PROJECTILE_TYPE_ARROW,
    PROJECTILE_TYPE_BOLT,
    PROJECTILE_TYPE_GRENADE,
};

/*
  Projectiles include:
  - Arrows
  - Bolts
  - Fireballs
  - Grenades
*/
struct projectile_entity {
    struct entity* owner; /* should be id but okay... */

    s32 visual_type; /* for drawing/behavior reasons probably */
    u32 flags;

    v2f32 acceleration;
    v2f32 velocity;

    s32 damage;

    /* shares the same units as the "big" entity */
    v2f32 position;
    v2f32 scale;

    /* projectile params */
    f32 lifetime;

    /* explosion params */
    f32 explosion_radius;

    /* other visual parameters like particle systems */
};

struct projectile_entity_list {
    s32                       capacity;
    struct projectile_entity* projectiles;
};

struct projectile_entity_list projectile_entity_list_reserved(struct memory_arena* arena, s32 capacity);
void                          projectile_entity_list_clear(struct projectile_entity_list* list);
struct projectile_entity*     projectile_entity_list_allocate_projectile(struct projectile_entity_list* list);
void update_projectile_entities(struct game_state* state, struct projectile_entity_list* projectiles, f32 dt, struct entity_iterator it, struct level_area* area);
/* TODO draw */

struct entity {
    string name;
    /* This is unique per entity! */
    /* wish there was good way to do fixed strings */
    char   script_name[64];

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

    /* one for each turn */
    s32 burning_char_effect_level;

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

    bool under_selection;

    /* This is a "main" turn action. Attacking / Using Items / Defending */
    bool                          waiting_on_turn;

    /* Movement is not counted as "main turn action" */
    /* I would add throwing/pickup like Disgaea here, but right now one at a time */
    /* for undoing the action alla Disgaea style. */

    /* NOTE: Honestly I should try to make all actions undoable, since it allows for more thinking and experimenting */
    /* but it might be difficult when you have to essentially rewind all frame state... Thankfully I think I can copy most engine state temporarily... */
    struct used_battle_action_stack used_actions;

    /* to avoid double fires */
    /* NOTE: ids are internally (index+1), it's a bit confusing as the editor and engine otherwise have id as 0 indices */
    /* this is just so I can zero out this thing and have expected behavior. */
    s32                           interacted_script_trigger_write_index;
    s32                           interacted_script_trigger_ids[32];

    struct entity_status_effect status_effects[MAX_ENTITY_STATUS_EFFECTS];
    s32                         status_effect_count;
    f32                         status_effect_tic_timer; /* TODO */
};

void entity_add_status_effect(struct entity* entity, struct entity_status_effect effect);
void entity_update_all_status_effects_for_a_turn(struct entity* entity);
void entity_remove_first_status_effect_of_type(struct entity* entity, s32 type);
void entity_remove_all_status_effect_of_type(struct entity* entity, s32 type);
bool entity_has_any_status_effect_of_type(struct entity* entity, s32 type);

s32  entity_get_usable_ability_indices(struct entity* entity, s32 max_limit, s32* ability_indices);
s32  entity_usable_ability_count(struct entity* entity);

/* should not be using pointers in the future, but whatever... */
void entity_add_used_battle_action(struct entity* entity, struct used_battle_action action);
void entity_undo_last_used_battle_action(struct entity* entity);
void entity_clear_battle_action_stack(struct entity* entity);
bool entity_already_used(struct entity* entity, u8 battle_action_type);
bool entity_action_stack_any(struct entity* entity);

struct used_battle_action battle_action_movement(struct entity* entity);
struct used_battle_action battle_action_defend(struct entity* entity);
struct used_battle_action battle_action_item_usage(struct entity* entity, s32 item_use_index);

void entity_snap_to_grid_position(struct entity* entity);
v2f32 grid_snapped_v2f32(v2f32 in) {
    in.x = roundf(in.x/TILE_UNIT_SIZE)*TILE_UNIT_SIZE;
    in.y = roundf(in.y/TILE_UNIT_SIZE)*TILE_UNIT_SIZE;
    return in;
}

void entity_clear_all_abilities(struct entity* entity);
void entity_add_ability_by_id(struct entity* entity, s32 id);
void entity_remove_ability_by_id(struct entity* entity, s32 id);
void entity_add_ability_by_name(struct entity* entity, string id);
void entity_remove_ability_by_name(struct entity* entity, string id);
void entity_do_level_up(struct entity* entity);
void entity_award_experience(struct entity* entity, s32 xp_amount); /* need to handle level ups */

/* lol how has this not been done yet? Spend too much time in combat methinks */
void entity_set_dialogue_file(struct entity* entity, string str) {
    if (!str.length) {
        entity->has_dialogue = false;
        return;
    }

    entity->has_dialogue = true;
    s32 minimum_count = str.length;
    if (minimum_count > array_count(entity->dialogue_file)) {
        minimum_count = array_count(entity->dialogue_file);
    }
    cstring_copy(str.data, entity->dialogue_file, minimum_count);
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

struct entity_base_data_ability_slot {
    s32 ability;
    u8  level;
};
struct entity_base_data {
    string                        name;
    s32                           model_index;
    u32                           flags;
    u32                           ai_flags;
    /* TODO: unused 32 possible entity teams... I really doubt I'll run out */
    u32                           team_flags;
    struct entity_stat_block      stats;
    s32                           health;
    s32                           magic;
    s32                           loot_table_id_index; /* if -1, don't use loot table. Need to have override in level_area_entity */
    item_id                       equip_slots[ENTITY_EQUIP_SLOT_INDEX_COUNT];
    s32                           ability_count;
    struct entity_base_data_ability_slot abilities[ENTITY_MAX_ABILITIES];
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
void entity_play_animation(struct entity* entity, string name, bool with_direction);
void entity_play_animation_no_direction(struct entity* entity, string name);
void entity_play_animation_with_direction(struct entity* entity, string name);
void entity_look_at(struct entity* entity, v2f32 position);

s32 entity_find_effective_stat_value(struct entity* entity, s32 stat_index);
s32 entity_find_effective_attack_radius(struct entity* entity);
struct entity_stat_block entity_find_effective_stat_block(struct entity* entity);

/* these don't do error checking, they assume the item index is within bounds */
/* NOTE: The inventory does not know it's own limits, since I use type-punning to allow these to work on different fixed sized instances. */
void entity_inventory_equip_item(struct entity_inventory* inventory, s32 limits, s32 item_index, s32 equipment_index, struct entity* target);
void entity_inventory_unequip_item(struct entity_inventory* inventory, s32 limits, s32 equipment_index, struct entity* target);
bool entity_any_equipped_item(struct entity* target, s32 slot);
/* NOTE: use item only technically works on "standard consumables", but some special consumable types such as "bombs/grenades" or projectiles require more work  */
void entity_inventory_use_item(struct entity_inventory* inventory, s32 item_index, struct entity* target);

s32  entity_inventory_count_instances_of(struct entity_inventory* inventory, string item_name);
s32  entity_inventory_get_gold_count(struct entity_inventory* inventory);
void entity_inventory_set_gold_count(struct entity_inventory* inventory, s32 amount);

void entity_combat_submit_movement_action(struct entity* entity, v2f32* path_points, s32 path_count);
void entity_combat_submit_defend_action(struct entity* entity);
void entity_combat_submit_attack_action(struct entity* entity, entity_id target_id);

bool entity_is_in_defense_stance(struct entity* entity);

/* This only obviously allows you to use abilities that you have. Cannot force them! */
/* though that'd be an interesting thing to allow. */
void entity_combat_submit_ability_action(struct entity* entity, entity_id* targets, s32 target_count, s32 user_ability_index);

/* NOTE: (like in a few days when I add explosives or something or molotovs) in the future this may obviously imply a target, but that can be in many places depending on the item type */
void entity_combat_submit_item_use_action(struct entity* entity, s32 item_index, bool from_player_inventory);

void               update_entities(struct game_state* state, f32 dt, struct entity_iterator it, struct level_area* area);

struct rectangle_f32 entity_rectangle_collision_bounds(struct entity* entity);

struct entity_query_list {
    entity_id* ids;
    s32        count;
};
struct entity_query_list find_entities_within_radius_without_obstacles(struct memory_arena* arena, struct game_state* state, v2f32 position, f32 radius);
struct entity_query_list find_entities_within_radius_with_obstacles(struct memory_arena* arena, struct game_state* state, v2f32 position, f32 radius);
struct entity_query_list find_entities_within_radius(struct memory_arena* arena, struct game_state* state, v2f32 position, f32 radius, bool obstacle);

/*
  NOTE: I should be using my sort keys API however it's not very useful IMO, since most things know their
  order, and I find their draw order trivially without sorting.

  It's only these specific entities, that need sorting and they're really just sorted relative to themselves
  not to the rest of the world
 */
enum sortable_draw_entity_type {
    SORTABLE_DRAW_ENTITY_ENTITY,
    SORTABLE_DRAW_ENTITY_CHEST,
    /* This is what's really going to hurt because particles are lots! */
    SORTABLE_DRAW_ENTITY_PARTICLE,
    SORTABLE_DRAW_ENTITY_SAVEPOINT,
    SORTABLE_DRAW_ENTITY_PROJECTILE,
};

struct sortable_draw_entity {
    u8    type;
    /* must be in pixels */
    f32   y_sort_key;
    union {
        void* pointer; /* the pointers are stable for whatever I need. So I'll just use the pointer */
        entity_id entity_id; /* of course if necessary... */
    };
};
struct sortable_draw_entities {
    s32 count;
    struct sortable_draw_entity* entities;
};

local void sortable_entity_draw_entity(struct render_commands* commands, struct graphics_assets* assets, entity_id id, f32 dt);
local void sortable_entity_draw_chest(struct render_commands* commands, struct graphics_assets* assets, struct entity_chest* chest, f32 dt);
local void sortable_entity_draw_savepoint(struct render_commands* commands, struct graphics_assets* assets, struct entity_savepoint* savepoint, f32 dt);
local void sortable_entity_draw_particle(struct render_commands* commands, struct graphics_assets* assets, struct entity_particle* particle, f32 dt);
local void sortable_entity_draw_projectile(struct render_commands* commands, struct graphics_assets* assets, struct projectile_entity* projectile, f32 dt);

struct sortable_draw_entities sortable_draw_entities(struct memory_arena* arena, s32 capacity);
void sortable_draw_entities_push_entity(struct sortable_draw_entities* entities, f32 y_sort_key, entity_id id);
void sortable_draw_entities_push(struct sortable_draw_entities* entities, u8 type, f32 y_sort_key, void* ptr);
void sortable_draw_entities_push_chest(struct sortable_draw_entities* entities, f32 y_sort_key, void* ptr);
void sortable_draw_entities_push_particle(struct sortable_draw_entities* entities, f32 y_sort_key, void* ptr);
void sortable_draw_entities_push_savepoint(struct sortable_draw_entities* entities, f32 y_sort_key, void* ptr);
void sortable_draw_entities_push_projectile(struct sortable_draw_entities* entities, f32 y_sort_key, void* ptr);
/* should not take dt, but just need something to work and there's animation timing code */
void sortable_draw_entities_submit(struct render_commands* commands, struct graphics_assets* graphics_assets, struct sortable_draw_entities* entities, f32 dt);
void sortable_draw_entities_sort_keys(struct sortable_draw_entities* entities);

void render_entities(struct game_state* state, struct sortable_draw_entities* draw_entities);

void serialize_entity_chest_list(struct binary_serializer* serializer, struct memory_arena* arena, s32 version, struct entity_chest_list* list);
void serialize_entity_chest(struct binary_serializer* serializer, s32 version, struct entity_chest* chest);
void serialize_entity_id(struct binary_serializer* serializer, s32 version, entity_id* id);

bool entity_has_dialogue(struct entity* entity);

/*
  Specialized iterator on level_area and world_map types.

  This is mainly the reduce the code duplication on the collision parts of the code so I don't accidently
  make a mistake, whenever I try to add more collidable things.

  NOTE: if I were to include entities I would have to also bake an entity iterator inside, but thankfully entities
  don't have to collide with each other so this is completely okay.
*/
enum collidable_object_iterator_type {
    COLLIDABLE_OBJECT_ITERATOR_WORLD_MAP,
    COLLIDABLE_OBJECT_ITERATOR_LEVEL_AREA,
};
/* I may or may not need more fields, but I don't think I need more than the rectangle usually. */
struct collidable_object { 
    struct rectangle_f32 rectangle;
};
struct collidable_object_iterator_world_map {
    s32 tile_layer_object_index;
    s32 tile_layer_ground_index;

    s32 tile_layer_scriptable_layer_index;
    s32 tile_layer_scriptable_tile_index;
};
struct collidable_object_iterator_level_area {
    s32 tile_layer_object_index;
    s32 tile_layer_ground_index;

    s32 tile_layer_scriptable_layer_index;
    s32 tile_layer_scriptable_tile_index;
    s32 chest_index;
};
struct collidable_object_iterator {
    u8    type;
    void* parent;

    bool done;

    struct collidable_object_iterator_world_map  world_map;
    struct collidable_object_iterator_level_area level_area;
};

struct collidable_object_iterator world_map_collidables_iterator(struct world_map* world_map);
struct collidable_object_iterator level_area_collidables_iterator(struct level_area* level_area);
struct collidable_object collidable_object_iterator_begin(struct collidable_object_iterator* iterator);
bool                     collidable_object_iterator_done(struct collidable_object_iterator* iterator);
struct collidable_object collidable_object_iterator_advance(struct collidable_object_iterator* iterator);

#endif
