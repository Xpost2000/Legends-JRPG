/* NOTE
   proposal, to work with the amount of scripting that is inevitable in RPGS,
   I think I should start pushing more of the work to my scripting language.
   
   Now if I were to release a more commercial, user friendly RPG engine or was otherwise
   expecting to create content on a larger scale, I would probably create a block logic
   language.

   However, right now the lisp parser is robust enough to handle whatever dumb crap I shove
   into it.
   
   read "level_script.script" for an example of what I'm needin
   
   Basically we're just going to leverge scripting and the trigger system to do most of the work
   I would normally be doing.
   
   There are a few specialized entities, however for the most part. It would be simpler
   to literally just do a lot of the game as part of the scripting language. This includes things like
   puzzles.
   
   So my current idea is to turn the engine into a "kernel" that has the core game rules, and mechanics.
   Maybe some hard-coded game specific things if I can't think of a great way to fit it into the scripting
   language.
   
   However, a fair amount of effort will be done in the language script.
 */

/* TODO */
/*
  Everything should be represented in terms of virtual tile coordinates.

  Right now there's a weird mishmash, thankfully I have no content so this is not a big deal right now
  just do this the next time I do things.
 */
#ifndef GAME_DEF_C
#define GAME_DEF_C
/* shared structure definitions for both editor and game */

enum entity_game_script_target_types {
    GAME_SCRIPT_TARGET_TRIGGER,
    GAME_SCRIPT_TARGET_ENTITY,
    GAME_SCRIPT_TARGET_CHEST, /* add an "on-loot" trigger */
};
local string entity_game_script_target_type_name[] = {
    string_literal("trigger"),
    string_literal("entity"),
    string_literal("chest"),
};

enum activation_type {
    ACTIVATION_TYPE_TOUCH,
    ACTIVATION_TYPE_ACTIVATE,
    ACTIVATION_TYPE_HIT,
    ACTIVATION_TYPE_COUNT,
};

local string activation_type_strings[] = {
    string_literal("(on-touch)"),
    string_literal("(on-activate)"),
    string_literal("(on-hit?)"),
    string_literal("(count)"),
};

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
    string_literal("ITEMS"),
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

#define COLOR_GRADING_DAY     color32u8(255,255,255,255)
#define COLOR_GRADING_NIGHT   color32u8(178,180,255,255)
#define COLOR_GRADING_DARKEST color32u8(42,42,110,255)
#define COLOR_GRADING_DAWN    color32u8(240,105,26,255)

local union color32u8 global_color_grading_filter = COLOR_GRADING_DAY;

enum facing_direction {
    DIRECTION_DOWN,
    DIRECTION_UP,
    DIRECTION_LEFT,
    DIRECTION_RIGHT,
    DIRECTION_RETAINED,
    DIRECTION_COUNT = 4,
};
static string facing_direction_strings[] = {
    string_literal("(down)"),
    string_literal("(up)"),
    string_literal("(left)"),
    string_literal("(right)"),
    string_literal("(retained)"),
    string_literal("(count)"),
};
static string facing_direction_strings_normal[] = {
    string_literal("down"),
    string_literal("up"),
    string_literal("left"),
    string_literal("right"),
    string_literal("(retained)"),
    string_literal("(count)"),
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

#include "conversation_def.c"
#include "weather_def.c"

struct game_state;

#include "item_def.c"


enum game_ui_nine_patch_id {
    NINE_PATCH_LEFT,
    NINE_PATCH_RIGHT,
    NINE_PATCH_TOP,
    NINE_PATCH_BOTTOM,
    NINE_PATCH_TOP_LEFT,
    NINE_PATCH_TOP_RIGHT,
    NINE_PATCH_BOTTOM_LEFT,
    NINE_PATCH_BOTTOM_RIGHT,
    NINE_PATCH_MIDDLE,
};
static string game_ui_nine_patch_id_strings[] = {
    string_literal("left"),
    string_literal("right"),
    string_literal("top"),
    string_literal("bottom"),
    string_literal("top-left"),
    string_literal("top-right"),
    string_literal("bottom-left"),
    string_literal("bottom-right"),
    string_literal("center"),
};
/* nine patches are only drawn in exact tile measurements*/
struct game_ui_nine_patch {
    s32 tile_width;
    s32 tile_height;
    image_id textures[9];
};
/* TODO render and use ninepatches later :*/
struct game_ui_nine_patch game_ui_nine_patch_load_from_directory(struct graphics_assets* graphics_assets, string directory, s32 tile_width, s32 tile_height);

#include "level_area_def.c"

struct navigation_path {
    s32           count;
    v2f32*        points;
};

void _debug_print_navigation_path(struct navigation_path* path) {
    _debugprintf("path start");
    for (unsigned index = 0; index < path->count; ++index) {
        v2f32 current_point = path->points[index];
        _debugprintf("P[%d](%f, %f)", index, current_point.x, current_point.y);
    }
    _debugprintf("path end");
}

struct navigation_path navigation_path_find(struct memory_arena* arena, struct level_area* area, v2f32 start, v2f32 end);

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
    TRIGGER_PLACEMENT_TYPE_SCRIPTABLE_TRIGGER,
    TRIGGER_PLACEMENT_TYPE_COUNT
};
static string trigger_placement_type_strings[] = {
    string_literal("Level Transition"),
    string_literal("Scriptable Triggers"),
    string_literal("(count)")
};
enum entity_placement_type {
    /* ENTITY_PLACEMENT_TYPE_ACTOR, */
    ENTITY_PLACEMENT_TYPE_CHEST,
    ENTITY_PLACEMENT_TYPE_COUNT,
};
static string entity_placement_type_strings[] = {
    /* string_literal("NPC"), */
    string_literal("Chest"),
    string_literal("(count)"),
};
struct entity_chest_placement_property_menu {
    /* bool item_edit_open; */
    bool adding_item;
    f32  item_list_scroll_y;
    s32  item_sort_filter;
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
    /* SHIFT TAB SHOULD INTRODUCE A TOOL SELECTION MODE, instead of arrow keys */
    s32           tool_mode;

    /* TAB WILL PROVIDE A CONTEXT SPECIFIC SELECTION */
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

    s32                              trigger_level_transition_count;
    s32                              trigger_level_transition_capacity;
    struct trigger_level_transition* trigger_level_transitions;
    s32                              entity_chest_count;
    s32                              entity_chest_capacity;
    struct entity_chest*             entity_chests;
    s32                              generic_trigger_count;
    s32                              generic_trigger_capacity;
    struct trigger*                  generic_triggers;
    
    struct camera camera;

    v2f32 default_player_spawn;

    /* I should force a relative mouse mode? but later */
    /* scroll drag data */
    bool was_already_camera_dragging;
    v2f32 initial_mouse_position;
    v2f32 initial_camera_position;

    struct entity_chest_placement_property_menu chest_property_menu;
    struct tile_painting_property_menu          tile_painting_property_menu;

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

    /* for edits that may require cross area interaction */
    /* we just load another full level and display it normally without editor mode stuff */
    char loaded_area_name[260]; /* level_areas don't know where they come from... */
    struct level_area loaded_area;
    bool       viewing_loaded_area;
};

/* TODO */
/* for now just an informative message box. No prompting. Prompts are probably just going to be different code, and more involved. */
/* I need a UI graphic for this. */
struct ui_popup_message_box {
    /* internal cstring storage unfortunately. */
    char message_storage[128];
    /* should include animation time */
    /* s32 show_character_count; */
};
/* anim state */
struct ui_popup_state {
    s32                         message_count;
    struct ui_popup_message_box messages[256];
};

/* flashing floating texts that represent damage! */
#define DAMAGE_NOTIFIER_LINGER_TIME (0.15f)
#define DAMAGE_NOTIFIER_MAX_TIME (0.6)
#define DAMAGE_NOTIFIER_FLICKER_TIME (0.075)
struct damage_notifier {
    v2f32 position;
    /* u8  type; /\* 0 - PHYSICAL, 1 - MAGIC *\/ */

    s32 amount;
    f32 timer;

    f32 flicker_timer;
    u8  alternative_color;
};
s32                    global_damage_notification_count  = 0;
struct damage_notifier global_damage_notifications[1024] = {};
void notify_damage(/* u8 type, */v2f32 position, s32 amount) {
    if (global_damage_notification_count >= 1024) {
        return;
    }

    struct damage_notifier* notifier = &global_damage_notifications[global_damage_notification_count++];
    zero_memory(notifier, sizeof(*notifier));

    notifier->position               = position;
    notifier->amount                 = amount;
}

/* NOTE: does not render through render commands. Manually does camera projection (no scaling though) */
void game_display_and_update_damage_notifications(struct software_framebuffer* framebuffer, f32 dt);

struct ui_popup_state global_popup_state = {};
void game_message_queue(string message);
bool game_display_and_update_messages(struct software_framebuffer* framebuffer, f32 dt);

enum ui_pause_menu_animation_state{
    UI_PAUSE_MENU_TRANSITION_IN,
    UI_PAUSE_MENU_NO_ANIM,
    UI_PAUSE_MENU_TRANSITION_CLOSING,
};

/* 
   check this when doing the transitions,
       
   Afterwards we have to hand over control to the sub menu states...
   It was either this or spend more weeks making a more elaborate UI system...
       
   I'd rather just tangle this jungle of complexity now I guess.
*/
enum ui_pause_menu_sub_menu_state {
    UI_PAUSE_MENU_SUB_MENU_STATE_NONE      = 0,
    UI_PAUSE_MENU_SUB_MENU_STATE_INVENTORY = 1,
    UI_PAUSE_MENU_SUB_MENU_STATE_OPTIONS   = 2,
};

/* share this for all types of similar menu types. */
/* could do for a renaming. */
struct ui_pause_menu {
    u8  animation_state; /* 0 = OPENING, 1 = NONE, 2 = CLOSING */
    f32 transition_t;
    s32 selection;
    /* NOTE we can use this field instead of that weird thing in the editor_state */
    /* other than the fact that the editor is actually ripping this state from the game state */
    /* in reality, I guess this should actually just be a global variable LOL. Or otherwise shared state */
    /* since preferably the game_state is almost exclusively only serializing stuff. */
    s32 last_sub_menu_state;
    s32 sub_menu_state;
    /* reserved space */
    f32 shift_t[128];
};

enum ui_sub_menu_inventory_cursor_region {
    UI_SUB_MENU_INVENTORY_CURSOR_REGION_FILTER,
    UI_SUB_MENU_INVENTORY_CURSOR_REGION_ITEM_LIST,
};
/* animating this going to be a bitch later... It always is anyways... */
struct ui_sub_menu_inventory {
    s32 cursor_region;
    s32 selection_filter;
    s32 selection_item_list;
};

enum interactable_type {
    INTERACTABLE_TYPE_NONE,
    INTERACTABLE_TYPE_CHEST,
    INTERACTABLE_TYPE_COUNT,
};
static string interactable_type_strings[] = {
    string_literal("(none)"),
    string_literal("(chest)"),
    string_literal("(count)"),
};

#include "entity_model_def.c"
#include "entities_def.c"

struct game_state_combat_state {
    bool      active_combat;
    s32       count;
    /* sorted by initiative */
    entity_id participants[512];

    /* crying for all the animation state */
    s32       active_combatant;
};

enum game_screen_mode {
    GAME_SCREEN_MAIN_MENU,
    GAME_SCREEN_INGAME,
};

s32 screen_mode = GAME_SCREEN_MAIN_MENU;

struct game_variable {
    // big names ****************
    char name[16];
    u8   is_float;

    s32  integer_value;
    f32  float_value;
};
struct game_variables {
    struct game_variable* variables;
    s32                   count;
};

struct game_state {
    struct memory_arena* arena;

    /* NOTE main menu, or otherwise menus that occur in different states are not ui states. */
    u32 last_ui_state;
    u32 ui_state;

    s32 in_editor;

    struct game_variables variables;

    /* name is saved here so we can hash it's name later... */
    char              loaded_area_name[260];
    struct level_area loaded_area;
    bool   level_area_on_enter_triggered;

    struct random_state rng;
    struct camera camera;

    /* TODO, move these out, these don't have to be here. I'm just being inconsistent. */
    struct ui_pause_menu ui_pause;
    struct ui_sub_menu_inventory ui_inventory;

    /* TODO add party member stuff! */
    struct player_party_inventory inventory;
    struct entity_list  entities;
    struct weather      weather;
    /* fread into this */

    /* I am a fan of animation, so we need to animate this. Even though it causes */
    /* state nightmares for me. */
    bool                is_conversation_active;
    bool                viewing_dialogue_choices;
    struct conversation current_conversation;

    /* only player v world conflicts should happen. */
    struct game_state_combat_state combat_state;
    struct file_buffer  conversation_file_buffer;
    /* to store strings and stuff like that. Clear this when dialogue finishes (or opening new dlg)*/
    struct memory_arena conversation_arena;
    u32                 current_conversation_node_id;
    s32                 currently_selected_dialogue_choice;

    /* TODO: Normalize this text. Anyways this should be in a string table. */
    char                current_region_name[96];
    char                current_region_subtitle[128];

    struct {
        s32   interactable_type;
        void* context;

        /* animation state... hahahahahaha */
    } interactable_state;
};

#include "save_data_def.c"
void handle_entity_level_trigger_interactions(struct game_state* state, struct entity* entity, s32 trigger_level_transition_count, struct trigger_level_transition* trigger_level_transitions, f32 dt);
void handle_entity_scriptable_trigger_interactions(struct game_state* state, struct entity* entity, s32 trigger_count, struct trigger* triggers, f32 dt);

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
