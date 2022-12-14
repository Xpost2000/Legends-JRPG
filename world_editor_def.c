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
enum world_editor_screen_state {
    WORLD_EDITOR_SCREEN_MAIN,
    WORLD_EDITOR_SCREEN_SETTING_LOCATION_ENTRANCE,
    WORLD_EDITOR_SCREEN_FILE_SELECTION_FOR_SETTING_LOCATION_ENTRANCE,
    WORLD_EDITOR_SCREEN_COUNT,
};

struct world_editor_screen_state_setting_location_entrance_data {
    v2f32 camera_position_before_trying_to_set_location_entrance;
    struct camera camera_before_trying_to_set_location_entrance;
};

struct world_editor_state {
    struct memory_arena* arena;
    s32    painting_tile_id;
    s32    tool_mode;
    s32    tab_menu_open;
    s32    entity_placement_type;

    s32 screen_state;
    struct world_editor_screen_state_setting_location_entrance_data setting_entrance_location;

    struct world_map world_map;

    struct camera camera;

    struct editor_drag_data drag_data;
    struct tile_painting_property_menu          tile_painting_property_menu;

    bool  was_already_camera_dragging;
    v2f32 initial_mouse_position;
    v2f32 initial_camera_position;

    void* last_selected;

    char current_save_name[128];
    u16 editor_brush_pattern;
    s32 current_tile_layer;

    /* Just for you to gauge the size of the world */
    s32 current_min_x;
    s32 current_min_y;
    s32 current_max_x;
    s32 current_max_y;

    /* ??? */
    /* for edits that may require cross area interaction */
    /* we just load another full level and display it normally without editor mode stuff */
    /* this needs to be changed. */
    char              loaded_area_name[260];
    struct level_area loaded_area;

    /* we're going to use a custom pause menu for this since I want the editor code to generally be self-contained */
    /* also because hooking into the normal pause menu is weird */
    struct world_editor_pause_menu pause_menu;
};

#endif
