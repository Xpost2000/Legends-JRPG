#ifndef EDITOR_DEF_C
#define EDITOR_DEF_C

enum editor_tool_mode {
    EDITOR_TOOL_TILE_PAINTING,
    EDITOR_TOOL_BATTLETILE_PAINTING,
    EDITOR_TOOL_SPAWN_PLACEMENT,
    EDITOR_TOOL_ENTITY_PLACEMENT,
    EDITOR_TOOL_TRIGGER_PLACEMENT,
    EDITOR_TOOL_COUNT,
};
static string editor_tool_mode_strings[]=  {
    [EDITOR_TOOL_TILE_PAINTING]     = string_literal("Tile mode"),
    [EDITOR_TOOL_BATTLETILE_PAINTING]     = string_literal("Battle Safe Tile mode"),
    [EDITOR_TOOL_SPAWN_PLACEMENT]   = string_literal("Place default spawn mode"),
    [EDITOR_TOOL_ENTITY_PLACEMENT]  = string_literal("Entity mode"),
    [EDITOR_TOOL_TRIGGER_PLACEMENT] = string_literal("Trigger"),
    [EDITOR_TOOL_COUNT]             = string_literal("(count)")
};
enum trigger_placement_type {
    TRIGGER_PLACEMENT_TYPE_LEVEL_TRANSITION,
    TRIGGER_PLACEMENT_TYPE_SCRIPTABLE_TRIGGER,
    TRIGGER_PLACEMENT_TYPE_COUNT
};
static string trigger_placement_type_strings[] = {
    [TRIGGER_PLACEMENT_TYPE_LEVEL_TRANSITION]   = string_literal("Level Transition"),
    [TRIGGER_PLACEMENT_TYPE_SCRIPTABLE_TRIGGER] = string_literal("Scriptable Triggers"),
    [TRIGGER_PLACEMENT_TYPE_COUNT]              = string_literal("(count)")
};
enum entity_placement_type { 
    /*
      NOTE: contrasts with remaining styles but this code is like this to allow a simple
      macro in the editor code.
     */
    ENTITY_PLACEMENT_TYPE_actor,
    ENTITY_PLACEMENT_TYPE_chest,
    ENTITY_PLACEMENT_TYPE_light,
    ENTITY_PLACEMENT_TYPE_savepoint,
    ENTITY_PLACEMENT_TYPE_tilemap_object,
    ENTITY_PLACEMENT_TYPE_count,
};
static string entity_placement_type_strings[] = {
    [ENTITY_PLACEMENT_TYPE_actor]     = string_literal("Actor"),
    [ENTITY_PLACEMENT_TYPE_chest]     = string_literal("Chest"),
    [ENTITY_PLACEMENT_TYPE_light]     = string_literal("Light"),
    [ENTITY_PLACEMENT_TYPE_savepoint] = string_literal("Savepoint"),
    [ENTITY_PLACEMENT_TYPE_tilemap_object] = string_literal("Tilemap Object"),
    [ENTITY_PLACEMENT_TYPE_count]     = string_literal("(count)"),
};
struct entity_chest_placement_property_menu {
    /* bool item_edit_open; */
    bool adding_item;
    /* NOTE this does not mouse wheel scroll. How disappointing */
    f32  item_list_scroll_y;
    s32  item_sort_filter;
};

#define FACING_DIRECTION_SPIN_TIMER_LENGTH_MAX (0.08)
struct entity_actor_placement_property_menu {
    s32  entity_base_id;
    f32  item_list_scroll_y;
    bool picking_entity_base;

    f32 facing_direction_spin_timer;
    s32 facing_direction_index_for_animation;
};
struct tile_painting_property_menu {
    f32 item_list_scroll_y;
};
/* tab menus a chord of tab + any of the modifier keys (might want to avoid alt tab though) */
/* naked-tab (mode-specific actions.) */
/* shift-tab (mode selection) */
/* ctrl-tab  (object specific (property editting?)) */
/* ctrl-tab  (object specific (property editting?)) */
enum tab_menu_bit_flags {
    TAB_MENU_CLOSED    = 0,
    TAB_MENU_OPEN_BIT  = BIT(0),
    TAB_MENU_SHIFT_BIT = BIT(1),
    TAB_MENU_CTRL_BIT  = BIT(2),
    TAB_MENU_ALT_BIT   = BIT(3),
};
struct editor_state {
    struct memory_arena* arena;
    s32           tool_mode;

    s32           placing_entity_id;
    s32           painting_tile_id;
    s32           trigger_placement_type;
    s32           entity_placement_type;

#if 0
    s32                              tile_count;
    s32                              tile_capacity;
    struct tile*                     tiles;
#else
    s32                              tile_counts[TILE_LAYER_COUNT];
    s32                              tile_capacities[TILE_LAYER_COUNT];
    struct tile*                     tile_layers[TILE_LAYER_COUNT];
#endif
    s32                              current_tile_layer;

    s32                                      trigger_level_transition_count;
    s32                                      trigger_level_transition_capacity;
    struct trigger_level_transition*         trigger_level_transitions;
    s32                                      entity_chest_count;
    s32                                      entity_chest_capacity;
    struct entity_chest*                     entity_chests;
    s32                                      generic_trigger_count;
    s32                                      generic_trigger_capacity;
    struct trigger*                          generic_triggers;
    s32                                      entity_count;
    s32                                      entity_capacity;
    struct level_area_entity*                entities;
    s32                                      light_capacity;
    s32                                      light_count;
    s32                                      entity_savepoint_count;
    s32                                      entity_savepoint_capacity;
    struct level_area_savepoint*             entity_savepoints;
    struct light_def*                        lights;
    s32                                      battle_safe_square_capacity;
    s32                                      battle_safe_square_count;
    struct level_area_battle_safe_square*    battle_safe_squares;
    s32                                      tilemap_object_capacity;
    s32                                      tilemap_object_count;
    struct level_area_tilemap_object_editor* tilemap_objects;

    s32                                      tilemap_object_tile_pool_capacity;
    struct level_area_tilemap_tile*          tilemap_object_tile_pool;

    bool fullbright;
    
    struct camera camera;
    v2f32 default_player_spawn;

    /* I should force a relative mouse mode? but later */
    /* scroll drag data */
    bool was_already_camera_dragging;
    v2f32 initial_mouse_position;
    v2f32 initial_camera_position;

    struct entity_chest_placement_property_menu chest_property_menu;
    struct tile_painting_property_menu          tile_painting_property_menu;
    struct entity_actor_placement_property_menu actor_property_menu;

    /* ui pause menu animation state */
    /* 0 - none, 1 - save, 2 - load */
    s32 serialize_menu_mode;
    s32 serialize_menu_reason; /* 
                                  0 - save/load file,
                                  1 - select/level transition trigger 
                               */
    f32 serialize_menu_t;

    s32 tab_menu_open;

    /* NOTE I'm aware there is drag data like a few lines above this. */
    struct {
        /* NOTE this pointer should always have a rectangle as it's first member! (rectangle_f32) */
        void* context; /* if this pointer is non-zero we are dragging */

        bool has_size;
        v2f32 initial_mouse_position; /* assume this to be in "world/tile" coordinates */
        /* context sensitive information to be filled */
        v2f32 initial_object_position;
        v2f32 initial_object_dimensions;
    } drag_data;

    /* another context pointer. Use this for the ctrl-tab menu */
    /* NOTE, might've introduced lots of buggies */
    void* last_selected;

    /* also a cstring... Ughhhh */
    char current_save_name[128];

    u16 editor_brush_pattern;

    /* for edits that may require cross area interaction */
    /* we just load another full level and display it normally without editor mode stuff */
    char loaded_area_name[260]; /* level_areas don't know where they come from... */
    struct level_area loaded_area;
    bool       viewing_loaded_area;
};

#endif
