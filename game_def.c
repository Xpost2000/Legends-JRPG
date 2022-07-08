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
/* tiles.c */

struct tile {
    s32 id;
    /* NOTE, remove? */
    s32 flags; /* acts as a XOR against it's parent? (tile definitions elsewhere.) */
    s16 x;
    s16 y;
};

/* allow this to be associated to an actor */
/* NOTE Does not allow conditional dialogue yet. */
/* NOTE Does not allow anything to happen other than dialogue... */
#define MAX_CONVERSATION_CHOICES (16)
struct conversation_choice {
    string text;
    /* does not count bartering */
    u32    target; /* 0 == END_CONVERSATION */
};
struct conversation_node {
    image_id portrait;
    string   speaker_name;
    string   text;

    struct conversation_choice choices[MAX_CONVERSATION_CHOICES];
};
/* simple? */
struct conversation {
    /* assume 0 is the start of the node always */
    u16 node_count;
    struct conversation_node* nodes;
};
/* END OF NOT CHECKED CODE */
#define CURRENT_LEVEL_AREA_VERSION (0)
struct level_area {
    u32          version;
    v2f32        default_player_spawn;

    s32          tile_count;
    struct tile* tiles;
};

enum editor_tool_mode {
    EDITOR_TOOL_TILE_PAINTING,
    EDITOR_TOOL_SPAWN_PLACEMENT,
    EDITOR_TOOL_ENTITY_PLACEMENT,
    EDITOR_TOOL_TRIGGER_PLACEMENT,
    EDITOR_TOOL_COUNT,
};
static string editor_tool_mode_strings[]=  {
    string_literal("Tile"),
    string_literal("Default spawn"),
    string_literal("Entity"),
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
    f32 serialize_menu_t;

    /* also a cstring... Ughhhh */
    char current_save_name[128];
};

#include "weather_def.c"
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

    enum ui_pause_menu_animation_state{
        UI_PAUSE_MENU_TRANSITION_IN,
        UI_PAUSE_MENU_NO_ANIM,
        UI_PAUSE_MENU_TRANSITION_CLOSING,
    };

    /* share this for all types of similar menu types. */
    struct ui_pause_menu {
        u8  animation_state; /* 0 = OPENING, 1 = NONE, 2 = CLOSING */
        f32 transition_t;
        s16 selection;
        /* reserved space */
        f32 shift_t[128];
    } ui_pause;

    struct entity_list entities;
    struct weather     weather;
};

#endif GAME_DEF_C