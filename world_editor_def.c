#ifndef WORLD_EDITOR_DEF_C
#define WORLD_EDITOR_DEF_C

/* yep this is a separate editor... Shares some structure with editor_state */
struct world_editor_pause_menu {
    s32 screen;
};
enum world_editor_tool_mode {
    WORLD_EDITOR_TOOL_TILE_PAINTING,
    WORLD_EDITOR_TOOL_SPAWN_PLACEMENT,
    WORLD_EDITOR_TOOL_ENTITY_PLACEMENT,
    WORLD_EDITOR_TOOL_LEVEL_SETTINGS,
    WORLD_EDITOR_TOOL_COUNT,
};
static string world_editor_tool_mode_strings[]=  {
    [WORLD_EDITOR_TOOL_TILE_PAINTING]       = string_literal("Tile mode"),
    [WORLD_EDITOR_TOOL_SPAWN_PLACEMENT]     = string_literal("Place default spawn mode"),
    [WORLD_EDITOR_TOOL_ENTITY_PLACEMENT]      = string_literal("Entity placement"),
    [WORLD_EDITOR_TOOL_LEVEL_SETTINGS]      = string_literal("Level Settings"),
    [WORLD_EDITOR_TOOL_COUNT]               = string_literal("(count)")
};
enum world_entity_placement_type { 
    /*
      NOTE: contrasts with remaining styles but this code is like this to allow a simple
      macro in the editor code.
     */
    WORLD_ENTITY_PLACEMENT_TYPE_location,
    WORLD_ENTITY_PLACEMENT_TYPE_trigger,
    WORLD_ENTITY_PLACEMENT_TYPE_position_marker,
    WORLD_ENTITY_PLACEMENT_TYPE_count,
};
static string world_entity_placement_type_strings[] = {
    [WORLD_ENTITY_PLACEMENT_TYPE_location]        = string_literal("Location Trigger"),
    [WORLD_ENTITY_PLACEMENT_TYPE_trigger]         = string_literal("Trigger"),
    [WORLD_ENTITY_PLACEMENT_TYPE_position_marker] = string_literal("Position Marker"),
    [WORLD_ENTITY_PLACEMENT_TYPE_count]           = string_literal("(count)"),
};
struct world_editor_state {
    struct memory_arena* arena;
    s32    painting_tile_id;
    s32    tool_mode;
    s32    tab_menu_open;
    s32           entity_placement_type;

    s32          tile_counts[WORLD_TILE_LAYER_COUNT];
    s32          tile_capacities[WORLD_TILE_LAYER_COUNT];
    struct tile* tile_layers[WORLD_TILE_LAYER_COUNT];
    s32          current_tile_layer;
    struct position_marker_list position_markers;

    struct camera camera;
    v2f32         default_player_spawn;

    struct editor_drag_data drag_data;
    struct tile_painting_property_menu          tile_painting_property_menu;

    bool  was_already_camera_dragging;
    v2f32 initial_mouse_position;
    v2f32 initial_camera_position;

    void* last_selected;

    char current_save_name[128];
    u16 editor_brush_pattern;

    /* Just for you to gauge the size of the world */
    s32 current_min_x;
    s32 current_min_y;
    s32 current_max_x;
    s32 current_max_y;

    /* ??? */
    /* for edits that may require cross area interaction */
    /* we just load another full level and display it normally without editor mode stuff */
    char loaded_area_name[260]; /* level_areas don't know where they come from... */
    struct level_area loaded_area;
    bool       viewing_loaded_area;

    /* we're going to use a custom pause menu for this since I want the editor code to generally be self-contained */
    /* also because hooking into the normal pause menu is weird */
    struct world_editor_pause_menu pause_menu;
    string map_script_string;
};

#endif
