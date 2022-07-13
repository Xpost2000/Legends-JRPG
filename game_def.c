/* TODO */
/*
  Everything should be represented in terms of virtual tile coordinates.

  Right now there's a weird mishmash, thankfully I have no content so this is not a big deal right now
  just do this the next time I do things.
 */
#ifndef GAME_DEF_C
#define GAME_DEF_C
/* shared structure definitions for both editor and game */

enum ui_state {
    UI_STATE_INGAME,
    UI_STATE_PAUSE,
    UI_STATE_COUNT,
};
static string ui_state_strings[] = {
    string_literal("ingame"),
    string_literal("paused"),
    string_literal("(count)"),
};

static string ui_pause_menu_strings[] = {
    string_literal("RESUME"),
    string_literal("PARTY"),
    string_literal("OPTIONS"),
    string_literal("QUIT"),
};
static string ui_pause_editor_menu_strings[] = {
    string_literal("RESUME"),
    string_literal("TEST"),
    string_literal("SAVE"),
    string_literal("LOAD"),
    string_literal("OPTIONS"),
    string_literal("QUIT"),
};

/* START OF NON-CHECKED CODE*/
/* TODO level editor for more things, mostly doors as a start. */
/* TODO door/lever test art for activations */

/* NOTE, all the code written right now has not been tested, I don't actually want to really code anything today, but I should write something... */
/* I will not offer hierachical objects, which makes things a bit more complicated */
/* I might have to offer modal selection which can be a bit painful... (sprites and stuff like that) */
/* Generic triggers can be used to produce levers and doors and the like, however */
/* Or I might just offer an image property, and allow it to load it's own images, _activated, _inactive mode (there'd have to be a lot of engine data work for this) */
/* If I'm especially lazy, I can do this and also have a separate "object/prop" type which is just a bunch of hardcoded objects... */
/* Use this for every generic kind of activatable item? */
enum activation_mode {
    ACTIVATION_NONE,
    ACTIVATION_WORLD_ATTACKED, /* attack in the world, for like hidden walls or something */
    ACTIVATION_TOUCH,    /* Actiate on touch */
    ACTIVATION_ACTIVATE, /* Actiate on button activation */
    ACTIVATION_TYPE_COUNT,
};
static string activation_mode_strings[] = {
    string_literal("(none)"),
    string_literal("(world attack)"),
    string_literal("(touch)"),
    string_literal("(activate)"),
    string_literal("(count)"),
};
enum activation_action_type {
    ACTIVATION_ACTION_NONE,
    ACTIVATION_ACTION_MOVE_LEVEL,
    ACTIVATION_ACTION_MOVE_POSITION,
    ACTIVATION_ACTION_OPEN_DIALOGUE,
    ACTIVATION_ACTION_TYPE_COUNT
    /* NOTE more, but okay for now */
};
static string activation_action_type_strings[] = {
    string_literal("(none)"),
    string_literal("(level move)"),
    string_literal("(pos move)"),
    string_literal("(open conversation)"),
    string_literal("(count)"),
};
#define MAX_ACTIVATION_EVENTS 8
struct activation_action {
    /* NOTE, the functions these events call should just accept raw data as doors and levers or what have you may not have these things? */
    u32 event_type;
};
#if 0
struct trigger {
    u8 activation_mode;
    struct activation_action actions[MAX_ACTIVATION_EVENTS];
};
/* not needed right now, we'll just be direct */
#endif
enum facing_direction {
    DIRECTION_DOWN,
    DIRECTION_UP,
    DIRECTION_LEFT,
    DIRECTION_RIGHT,
    DIRECTION_RETAINED,
    DIRECTION_COUNT,
};
static string facing_direction_strings[] = {
    string_literal("(down)"),
    string_literal("(up)"),
    string_literal("(left)"),
    string_literal("(right)"),
    string_literal("(retained)"),
    string_literal("(count)"),
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
/* loaded from a table at runtime or compile time? */
/* Since this isn't serialized, I can change this pretty often. */
enum tile_data_flags {
    TILE_DATA_FLAGS_NONE  = 0,
    TILE_DATA_FLAGS_SOLID = BIT(0),
};
/* index = id, reserve sparse array to allow for random access */
struct autotile_table {s32 neighbors[256];};
struct tile_data_definition {
    string               name;
    string               image_asset_location;
    struct rectangle_f32 sub_rectangle; /* if 0 0 0 0 assume entire texture */
    /* special flags, like glowing and friends! To allow for multiple render passes */
    s32                  flags;
    /* TODO implement 4 bit marching squares. Useful for graphics */
    struct autotile_table* autotile_info;
};
static s32 tile_table_data_count = 0;
/* tiles.c */

struct tile {
    s32 id;
    /* NOTE, remove? */
    s32 flags; /* acts as a XOR against it's parent? (tile definitions elsewhere.) */
    s16 x;
    s16 y;
};

/* END OF NOT CHECKED CODE */
#include "conversation_def.c"

#define CURRENT_LEVEL_AREA_VERSION (1)
struct level_area {
    u32          version;
    v2f32        default_player_spawn;

    s32          tile_count;
    struct tile* tiles;

    s32 trigger_level_transition_count;
    struct trigger_level_transition* trigger_level_transitions;
};

enum editor_tool_mode {
    EDITOR_TOOL_TILE_PAINTING,
    EDITOR_TOOL_SPAWN_PLACEMENT,
    EDITOR_TOOL_ENTITY_PLACEMENT,
    EDITOR_TOOL_TRIGGER_PLACEMENT,
    EDITOR_TOOL_COUNT,
};
static string editor_tool_mode_strings[]=  {
    string_literal("Tile mode"),
    string_literal("Place default spawn mode"),
    string_literal("Entity mode"),
    string_literal("Trigger"),
    string_literal("(count)")
};
enum trigger_placement_type {
    TRIGGER_PLACEMENT_TYPE_LEVEL_TRANSITION,
    TRIGGER_PLACEMENT_TYPE_COUNT
};
static string trigger_placement_type_strings[] = {
    string_literal("Level Transition"),
    string_literal("(count)")
};
struct editor_state {
    struct memory_arena* arena;
    /* SHIFT TAB SHOULD INTRODUCE A TOOL SELECTION MODE, instead of arrow keys */
    s32           tool_mode;

    /* TAB WILL PROVIDE A CONTEXT SPECIFIC SELECTION */
    s32           painting_tile_id;
    s32           trigger_placement_type;

    s32           tile_count;
    s32           tile_capacity;
    struct tile*  tiles;

    s32                              trigger_level_transition_count;
    s32                              trigger_level_transition_capacity;
    struct trigger_level_transition* trigger_level_transitions;
    
    struct camera camera;

    v2f32 default_player_spawn;

    /* I should force a relative mouse mode? but later */
    /* scroll drag data */
    bool was_already_camera_dragging;
    v2f32 initial_mouse_position;
    v2f32 initial_camera_position;

    /* ui pause menu animation state */
    /* 0 - none, 1 - save, 2 - load */
    s32 serialize_menu_mode;
    s32 serialize_menu_reason; /* 
                                  0 - save/load file,
                                  1 - select/level transition trigger 
                               */
    f32 serialize_menu_t;

    /* tab menus a chord of tab + any of the modifier keys (might want to avoid alt tab though) */
    /* naked-tab (mode-specific actions.) */
    /* shift-tab (mode selection) */
    /* ctrl-tab  (object specific (property editting?)) */
    /* ctrl-tab  (object specific (property editting?)) */
    enum {
        TAB_MENU_CLOSED    = 0,
        TAB_MENU_OPEN_BIT  = BIT(0),
        TAB_MENU_SHIFT_BIT = BIT(1),
        TAB_MENU_CTRL_BIT  = BIT(2),
        TAB_MENU_ALT_BIT   = BIT(3),
    };
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

    /* for edits that may require cross area interaction */
    /* we just load another full level and display it normally without editor mode stuff */
    char loaded_area_name[260]; /* level_areas don't know where they come from... */
    struct level_area loaded_area;
    bool       viewing_loaded_area;
};

#include "weather_def.c"

struct game_state;

#include "item_def.c"
#include "entities_def.c"

struct game_state {
    struct memory_arena* arena;

    /* NOTE main menu, or otherwise menus that occur in different states are not ui states. */
    u32 last_ui_state;
    u32 ui_state;

    s32 in_editor;

    /* in "open" regions, allow for regions to be streamed in... Have to set game mode state flag. */
    struct level_area loaded_area;
    struct random_state rng;
    struct camera camera;

    enum ui_pause_menu_animation_state{
        UI_PAUSE_MENU_TRANSITION_IN,
        UI_PAUSE_MENU_NO_ANIM,
        UI_PAUSE_MENU_TRANSITION_CLOSING,
    };

    /* share this for all types of similar menu types. */
    struct ui_pause_menu {
        u8  animation_state; /* 0 = OPENING, 1 = NONE, 2 = CLOSING */
        f32 transition_t;
        s32 selection;
        /* reserved space */
        f32 shift_t[128];
    } ui_pause;

    struct player_party_inventory inventory;
    struct entity_list  entities;
    struct weather      weather;
    /* fread into this */

    /* I am a fan of animation, so we need to animate this. Even though it causes */
    /* state nightmares for me. */
    bool                is_conversation_active;
    bool                viewing_dialogue_choices;
    struct conversation current_conversation;
    u32                 current_conversation_node_id;
    s32                 currently_selected_dialogue_choice;
};

local v2f32 get_mouse_in_world_space(struct camera* camera, s32 screen_width, s32 screen_height) {
    return camera_project(camera, mouse_location(), screen_width, screen_height);
}

local v2f32 get_mouse_in_tile_space(struct camera* camera, s32 screen_width, s32 screen_height) {
    v2f32 world_space = get_mouse_in_world_space(camera, screen_width, screen_height);
    return v2f32(world_space.x / TILE_UNIT_SIZE, world_space.y / TILE_UNIT_SIZE);
}

local v2f32 get_mouse_in_tile_space_integer(struct camera* camera, s32 screen_width, s32 screen_height) {
    v2f32 tile_space_f32 = get_mouse_in_tile_space(camera, screen_width, screen_height); 
    return v2f32(floorf(tile_space_f32.x), floorf(tile_space_f32.y));
}

local void wrap_around_key_selection(s32 decrease_key, s32 increase_key, s32* pointer, s32 min, s32 max) {
    if (is_key_pressed(decrease_key)) {
        *pointer -= 1;
        if (*pointer < min)
            *pointer = max-1;
    } else if (is_key_pressed(increase_key)) {
        *pointer += 1;
        if (*pointer >= max)
            *pointer = min;
    }
}

#endif
