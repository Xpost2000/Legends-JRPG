#define REAL_SECONDS_FOR_GAME_TURN (2.5)
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

#include "game_config_defines.c"
#define GAME_MAX_PERMENANT_ENTITIES (64)
#define GAME_MAX_PROJECTILES        (64)

/* TODO */
/*
  Everything should be represented in terms of virtual tile coordinates.

  Right now there's a weird mishmash, thankfully I have no content so this is not a big deal right now
  just do this the next time I do things.
*/
#ifndef GAME_DEF_C
#define GAME_DEF_C
/* shared structure definitions for both editor and game */

/* need to draw the border to be color changable. since it looks ort of dummmb */
#define UI_DEFAULT_COLOR        (color32f32(34/255.0f, 37/255.0f, 143/255.0f, 1.0))
/* #define UI_DEFAULT_COLOR        (color32f32(17/255.0f, 112/255.0f, 76/255.0f, 1.0)) */
#define UI_BATTLE_COLOR UI_DEFAULT_COLOR

#include "special_effects_def.c"

/* using GNSH fonts, which are public domain, but credits to open game art, this font looks cool */
enum menu_font_variation {
    MENU_FONT_COLOR_GOLD,

    MENU_FONT_COLOR_ORANGE,
    MENU_FONT_COLOR_LIME,
    MENU_FONT_COLOR_SKYBLUE,
    MENU_FONT_COLOR_PURPLE,
    MENU_FONT_COLOR_BLUE,
    MENU_FONT_COLOR_STEEL,
    MENU_FONT_COLOR_WHITE,
    MENU_FONT_COLOR_YELLOW,
    MENU_FONT_COLOR_BLOODRED,

    /* I want room to have more fonts though, although GNSH fonts are very very nice. */

    MENU_FONT_COUNT,
};

s32 fontname_string_to_id(string name);

enum entity_game_script_target_types {
    GAME_SCRIPT_TARGET_TRIGGER,
    /* EVENTS
       ON-TOUCH
       ON-ACTIVATE
       ** TODO: ON-LEAVE
     */
    GAME_SCRIPT_TARGET_ENTITY,
    /* EVENTS
       ON-DEATH
       ROUTINE
     */
    GAME_SCRIPT_TARGET_CHEST, 
    /* EVENTS
       ON-LOOT
     */
    GAME_SCRIPT_TARGET_TRANSITION_TRIGGER,
    /* EVENTS
       ON-TOUCH
     */
    GAME_SCRIPT_TARGET_LIGHT,
    /* EVENTS
       NONE
     */
    GAME_SCRIPT_TARGET_SAVEPOINT,
    /* EVENTS
       NONE
     */
    GAME_SCRIPT_TARGET_SCRIPTABLE_LAYER,
    /* EVENTS TODO
       ON-TOUCH
       ON-LEAVE
       ROUTINE???
     */
    GAME_SCRIPT_TARGET_POSITION_MARKER,
    /* EVENTS
       NONE
     */
};
local string entity_game_script_target_type_name[] = {
    [GAME_SCRIPT_TARGET_TRIGGER]            = string_literal("trigger"),
    [GAME_SCRIPT_TARGET_ENTITY]             = string_literal("entity"),
    [GAME_SCRIPT_TARGET_CHEST]              = string_literal("chest"),
    [GAME_SCRIPT_TARGET_TRANSITION_TRIGGER] = string_literal("transition-trigger"),
    [GAME_SCRIPT_TARGET_SAVEPOINT]          = string_literal("savepoint"),
    [GAME_SCRIPT_TARGET_LIGHT]              = string_literal("light"),
    [GAME_SCRIPT_TARGET_SCRIPTABLE_LAYER]   = string_literal("script-layer"),
    [GAME_SCRIPT_TARGET_POSITION_MARKER]    = string_literal("pmarker"),
};

enum activation_type {
    ACTIVATION_TYPE_TOUCH,
    ACTIVATION_TYPE_ACTIVATE,
    ACTIVATION_TYPE_HIT,
    ACTIVATION_TYPE_COUNT,
};

local string activation_type_strings[] = {
    [ACTIVATION_TYPE_TOUCH]    = string_literal("(on-touch)"),
    [ACTIVATION_TYPE_ACTIVATE] = string_literal("(on-activate)"),
    [ACTIVATION_TYPE_HIT]      = string_literal("(on-hit)"),
    [ACTIVATION_TYPE_COUNT]    = string_literal("(count)"),
};

enum ui_state {
    UI_STATE_INGAME,
    UI_STATE_SAVEGAME,
    UI_STATE_PAUSE,
    UI_STATE_GAMEOVER,
    UI_STATE_COUNT,
};
static string ui_state_strings[] = {
    [UI_STATE_INGAME]   = string_literal("ingame"),
    [UI_STATE_PAUSE]    = string_literal("paused"),
    [UI_STATE_SAVEGAME] = string_literal("save-game"),
    [UI_STATE_GAMEOVER] = string_literal("game-over"),
    [UI_STATE_COUNT]    = string_literal("(count)"),
};


#define COLOR_GRADING_DAY     color32u8(250,245,216,255)
/* #define COLOR_GRADING_DAY     color32u8(249,184,165,255) */
#define COLOR_GRADING_DAWN     color32u8(249, 143, 109, 255)
/* #define COLOR_GRADING_DAY     color32u8(192, 231, 250, 255) */
/* #define COLOR_GRADING_DAY     color32u8(208, 254, 238, 255) */
/* #define COLOR_GRADING_DAY     color32u8(213, 215, 152, 255) */
#define COLOR_GRADING_NIGHT   color32u8(178*0.65,180*0.65,255*0.68,255)
#define COLOR_GRADING_DARKEST color32u8(42,42,110,255)

/* local union color32u8 global_color_grading_filter = COLOR_GRADING_DAY; */
local union color32u8 global_color_grading_filter = COLOR_GRADING_DAY;

local string color_grade_preset_strings[] = {
    [0] = string_literal("Day"),
    [1] = string_literal("Dawn"),
    [2] = string_literal("Night"),
    [3] = string_literal("Midnight"),
};
local union color32u8 color_grade_presets[] = {
    [0] = COLOR_GRADING_DAY,
    [1] = COLOR_GRADING_DAWN,
    [2] = COLOR_GRADING_NIGHT,
    [3] = COLOR_GRADING_DARKEST,
};

local void game_set_time_color(s32 time) {
    switch (time) {
        case 0: {
            global_color_grading_filter = COLOR_GRADING_DAY;
        } break;
        case 1: {
            global_color_grading_filter = COLOR_GRADING_DAWN;
        } break;
        case 2: {
            global_color_grading_filter = COLOR_GRADING_NIGHT;
        } break;
        case 3: {
            global_color_grading_filter = COLOR_GRADING_DARKEST;
        } break;
    }
}

enum facing_direction {
    DIRECTION_DOWN,
    DIRECTION_RIGHT,
    DIRECTION_UP,
    DIRECTION_LEFT,
    DIRECTION_RETAINED,
    DIRECTION_COUNT = 4,
};
static string facing_direction_strings[] = {
    [DIRECTION_DOWN]     = string_literal("(down)"),
    [DIRECTION_UP]       = string_literal("(up)"),
    [DIRECTION_LEFT]     = string_literal("(left)"),
    [DIRECTION_RIGHT]    = string_literal("(right)"),
    [DIRECTION_RETAINED] = string_literal("(retained)"),
    string_literal("(count)"),
};
static string facing_direction_strings_normal[] = {
    [DIRECTION_DOWN]     = string_literal("down"),
    [DIRECTION_UP]       = string_literal("up"),
    [DIRECTION_LEFT]     = string_literal("left"),
    [DIRECTION_RIGHT]    = string_literal("right"),
    [DIRECTION_RETAINED] = string_literal("retained"),
    string_literal("(count)"),
};
local u8 facing_direction_from_string(string name) {
    for (unsigned index = 0; index < array_count(facing_direction_strings_normal); ++index) {
        if (string_equal_case_insensitive(name, facing_direction_strings_normal[index])) {
            return index;
        }
    }

    return DIRECTION_DOWN;
}
/* loaded from a table at runtime or compile time? */
/* Since this isn't serialized, I can change this pretty often. */
enum tile_data_flags {
    TILE_DATA_FLAGS_NONE      = 0,
    TILE_DATA_FLAGS_SOLID     = BIT(0),
    TILE_DATA_FLAGS_BOAT_ONLY = BIT(1), /*WORLDMAP FLAG*/
};
/* index = id, reserve sparse array to allow for random access */
struct autotile_table {
    s32 neighbors[256];
};

/* TODO: add damaging steps! */
enum tile_palette {
    TILE_PALETTE_WORLD_MAP,
    TILE_PALETTE_OVERWORLD,
};
struct tile_data_definition {
    string               name;
    string               frames[24];
    s32 frame_index;
    s32 frame_count;
    f32 time_until_next_frame;
    f32 timer;
    struct rectangle_f32 sub_rectangle; /* if 0 0 0 0 assume entire texture */
    /* special flags, like glowing and friends! To allow for multiple render passes */
    s32                  flags;
    /* TODO implement 4 bit marching squares. Useful for graphics */
    struct autotile_table* autotile_info;
};
image_id get_tile_image_id(struct tile_data_definition* tile_def);
static s32 tile_table_data_count       = 0;
static s32 world_tile_table_data_count = 0;
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

/* forward decl avoid linking problems (cause C can't have define anywhere.) */
struct entity_list;
struct level_area;
struct world_map;

bool special_effects_active(void);
void special_effect_start_inversion(void);
void special_effect_stop_effects(void);

enum light_flags {
    LIGHT_FLAGS_NONE = 0,
    LIGHT_FLAGS_HIDDEN = BIT(31),
};
struct light_def {
/*
  NOTE Version 9 forgot
  to serialize the power field, and
  older versions had 4 reserved bytes
*/
    v2f32 position;
    v2f32 scale;  /* Only used for the editor as it expects rectangle_f32s */
    f32   power; /* scale if we add a texture id for light casting? In tile units */
                 /* since the renderer doesn't know how to rotate sprites who knows?  */
    union color32u8 color;
    u32   flags;
};
struct light_def light(v2f32 position, f32 power, union color32u8 color) {
    struct light_def result = {
        .position = position,
        .scale    = v2f32(1,1),
        .power    = power,
        .color    = color,
    };

    return result;
}

/*
  NOTE:
   I'm not replacing the dynamic lights inside of game_state with this list struct yet.
   It's a good idea but this is still a refactor and otherwise it still works.
 */
struct light_list { 
    s32 capacity;
    s32 count;
    struct light_def* lights;
};

struct light_list light_list_reserved(struct memory_arena* arena, s32 capacity);
struct light_def* light_list_push(struct light_list* list, struct light_def light);
struct light_def* light_list_find_light_at(struct light_list* list, v2f32 point);
void              light_list_remove(struct light_list* list, s32 index);
void              light_list_clear(struct light_list* list);
void              serialize_light_list(struct binary_serializer* serializer, struct memory_arena* arena, s32 version, struct light_list* list);

/* These are slightly different as they do different things on the "special_effects" behavior  */
/* also some of these will have code designated for things like entities */
union color32f32 game_background_things_shader(struct software_framebuffer* framebuffer, union color32f32 source_pixel, v2f32 pixel_position, void* context);
union color32f32 game_foreground_things_shader(struct software_framebuffer* framebuffer, union color32f32 source_pixel, v2f32 pixel_position, void* context);
union color32f32 game_foreground_entity_things_shader(struct software_framebuffer* framebuffer, union color32f32 source_pixel, v2f32 pixel_position, void* context);

#include "entity_model_def.c"
#include "entities_def.c"
#include "level_area_def.c"
#include "world_map_def.c"
#include "xp_tables_def.c"

struct rectangle_f32 tile_rectangle(struct tile* t);

/* use this instead of entity_list_dereference */
struct entity* game_dereference_entity(struct game_state* state, entity_id id);

struct navigation_path {
    s32           count;
    v2f32*        points;
    bool          success;
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

/* lol */
string copy_string_to_scratch(string str) {
    string result = memory_arena_push_string(&scratch_arena, str);
    return result;
}
string copy_cstring_to_scratch(char* cstr) {
    string result = memory_arena_push_string(&scratch_arena, string_from_cstring(cstr));
    return result;
}

#ifdef USE_EDITOR
#include "editor_def.c"
#include "world_editor_def.c"
#endif
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
#define MESSAGE_NOTIFIER_LINGER_TIME  (0.15f)
#define MESSAGE_NOTIFIER_MAX_TIME     (0.6)
#define MESSAGE_NOTIFIER_FLICKER_TIME (0.075)
#define MAX_MESSAGE_NOTIFICATIONS     (1024)

enum notifier_type {
    NOTIFIER_MESSAGE_STANDARD,
    NOTIFIER_MESSAGE_DAMAGE,
    NOTIFIER_MESSAGE_HEALING,
    NOTIFIER_MESSAGE_COUNT
};
local string notifier_type_strings[] = {
    [NOTIFIER_MESSAGE_STANDARD] = string_literal("(standard-message)"),
    [NOTIFIER_MESSAGE_DAMAGE]   = string_literal("(damage-message)"),
    [NOTIFIER_MESSAGE_HEALING]  = string_literal("(healing-message)"),
};
struct notifier_message {
    u8    type;
    v2f32 position;

    union {
        s32    amount;
        string text;
    };

    f32 timer;

    f32 flicker_timer;
    u8  message_color;
    u8  alternative_color;
};
s32                    global_message_notification_count  = 0;
struct notifier_message global_message_notifications[MAX_MESSAGE_NOTIFICATIONS+1] = {};

struct notifier_message* _allocate_global_message_notification(u8 type) {
    if (global_message_notification_count >= MAX_MESSAGE_NOTIFICATIONS) {
        return &global_message_notifications[MAX_MESSAGE_NOTIFICATIONS]; 
    }

    struct notifier_message* result = &global_message_notifications[global_message_notification_count++];
    zero_memory(result, sizeof(*result));
    result->type                    = type;
    return result;
}

void notify_message(v2f32 position, string text, u8 color) {
    struct notifier_message* notifier = _allocate_global_message_notification(NOTIFIER_MESSAGE_STANDARD);
    notifier->position                = position;
    notifier->text                    = memory_arena_push_string(&scratch_arena, text);
    notifier->message_color           = color;
}

void notify_healing(v2f32 position, s32 amount) {
    struct notifier_message* notifier = _allocate_global_message_notification(NOTIFIER_MESSAGE_HEALING);
    notifier->position               = position;
    notifier->amount                 = amount;
}

void notify_damage(v2f32 position, s32 amount) {
    struct notifier_message* notifier = _allocate_global_message_notification(NOTIFIER_MESSAGE_DAMAGE);
    notifier->position               = position;
    notifier->amount                 = amount;
}

#include "pause_menu_ui_def.c"
void game_display_and_update_message_notifications(struct render_commands* framebuffer, f32 dt);

struct ui_popup_state global_popup_state = {};
void game_message_queue(string message);
bool game_display_and_update_messages(struct software_framebuffer* framebuffer, f32 dt);

/* Non-pausing text you can read. */
#define MAX_PASSIVE_SPEAKING_DIALOGUES (256)
/* seconds */
#define PASSIVE_SPEAKING_DIALOGUE_TYPING_SPEED (0.054)
#define PASSIVE_SPEAKING_DIALOGUE_FADE_TIME    (0.765)
struct passive_speaking_dialogue {
    entity_id speaker_entity;

    string text;

    s32    showed_characters;
    f32    character_type_timer;
    f32    time_until_death;

    s32    game_font_id;
};

local s32 passive_speaking_dialogue_count                          = 0;
local struct passive_speaking_dialogue *passive_speaking_dialogues = 0;

local void passive_speaking_dialogue_clear_all(void) {
    passive_speaking_dialogue_count = 0;
}

local void passive_speaking_dialogue_cleanup(void) {
    for (s32 index = passive_speaking_dialogue_count-1; index >= 0; --index) {
        struct passive_speaking_dialogue* current_dialogue = (&passive_speaking_dialogues[index]);
        if (current_dialogue->time_until_death <= 0) {
            passive_speaking_dialogues[index] = passive_speaking_dialogues[--passive_speaking_dialogue_count];
        }
    }
}

local void passive_speaking_dialogue_push(entity_id who, string text, s32 font_id) {
    struct passive_speaking_dialogue* new_dialogue = &passive_speaking_dialogues[passive_speaking_dialogue_count++];
    new_dialogue->text                 = text;
    new_dialogue->speaker_entity       = who;
    new_dialogue->showed_characters    = 0;
    new_dialogue->character_type_timer = 0;
    new_dialogue->time_until_death     = PASSIVE_SPEAKING_DIALOGUE_FADE_TIME;
    new_dialogue->game_font_id         = font_id;
}

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
    INTERACTABLE_TYPE_ENTITY_CONVERSATION,
    INTERACTABLE_TYPE_ENTITY_SAVEPOINT,
    INTERACTABLE_TYPE_COUNT,
};
static string interactable_type_strings[] = {
    [INTERACTABLE_TYPE_NONE] = string_literal("(none)"),
    [INTERACTABLE_TYPE_CHEST] = string_literal("(chest)"),
    [INTERACTABLE_TYPE_ENTITY_CONVERSATION] = string_literal("(conversation)"),
    [INTERACTABLE_TYPE_ENTITY_SAVEPOINT] = string_literal("(savepoint)"),
    [INTERACTABLE_TYPE_COUNT] = string_literal("(count)"),
};

#define MAX_STORED_COMBAT_PARTICIPANTS (256)
#define MAX_STORED_COUNTER_ATTACKS     (16)
/* ^ this number is small since it's mostly back to back the same counters. (We can add more attacks later I suppose) */
struct combat_counter_attack_action {
    /*
      NOTE:
      such a small part to use pointers in... That's honestly a bit of a mistake on my part, but the current architecture makes it a pain for that.

      Thankfully pointers are very stable... (exempting level loads)
    */
    entity_id attacker;
    entity_id attacked;
    bool engaged;
};
local void add_counter_attack_entry(entity_id attacker, entity_id attacked);
struct game_state_combat_state {
    bool      active_combat;
    s32       count;
    /* sorted by initiative */
    entity_id participants[MAX_STORED_COMBAT_PARTICIPANTS];
    s32       participant_priorities[MAX_STORED_COMBAT_PARTICIPANTS];

    struct combat_counter_attack_action counter_attack_LIFO[MAX_STORED_COUNTER_ATTACKS];
    s32                                 counter_attack_LIFO_count;

    s32       turn_number;
    /* crying for all the animation state */
    s32       active_combatant;

    /* visual effect */
    f32 battle_zone_dark_fade_t;
};

enum game_screen_mode {
    GAME_SCREEN_PREVIEW_DEMO_ALERT,
    GAME_SCREEN_MAIN_MENU,
    GAME_SCREEN_INGAME,
    GAME_SCREEN_CREDITS,
    GAME_SCREEN_COUNT,
};

enum game_submode {
    GAME_SUBMODE_OVERWORLD,
    GAME_SUBMODE_WORLDMAP,
    GAME_SUBMODE_COUNT,
};

/* s32 screen_mode = GAME_SCREEN_PREVIEW_DEMO_ALERT; */
s32 screen_mode = GAME_SCREEN_MAIN_MENU;
s32 last_screen_mode = GAME_SCREEN_MAIN_MENU;

void set_game_screen_mode(s32 mode) {
    last_screen_mode = screen_mode;
    screen_mode      = mode;
}

s32 submode     = GAME_SUBMODE_OVERWORLD;

/* sized fixed chunks, but still dynamic */
/* this isn't a hashmap FYI, just linear lookups. That's okay too. Since lookup is not frequent. */
/* I can change this to a chunked hashmap I guess, not a big deal... */
#define MAX_GAME_VARIABLES_PER_CHUNK (16)
#define MAX_GAME_VARIABLE_NAME_LENGTH (32)
struct game_variable {
    // big names ****************
    char name[MAX_GAME_VARIABLE_NAME_LENGTH];
    s32  value;
};
struct game_variable_chunk {
    s32                         variable_count;
    struct game_variable        variables[MAX_GAME_VARIABLES_PER_CHUNK];
    struct game_variable_chunk* next;
};
struct game_variables {
    struct memory_arena* arena;
    struct game_variable_chunk* first;
    struct game_variable_chunk* last;
};
struct game_variables game_variables(struct memory_arena* arena);
struct game_variable* lookup_game_variable(string name, bool create_when_not_found);
void game_variable_set(string name, s32 value);
void game_variables_clear_all_state(void); /* does not deallocate memory */
s32  game_variable_get(string name);
s32  game_variables_count_all(void);

#include "shop_def.c"

#define MAX_GAME_DYNAMIC_LIGHT_POOL (128)
enum ui_save_menu_phase {
    UI_SAVE_MENU_PHASE_FADEIN,
    UI_SAVE_MENU_PHASE_IDLE,
    UI_SAVE_MENU_PHASE_FADEOUT,
};
struct ui_save_menu {
    s32 phase;
    f32 effects_timer;
};

/* this is bad... */
enum current_theme_track_type {
    THEME_SAFE_TRACK,
    THEME_BATTLE_TRACK,
};

#define MAX_PARTY_MEMBERS (16)

struct world_map_list {
    s32              count;
    struct world_map world_maps[64];
};
struct world_map* world_map_list_find_existing(struct world_map_list* list, u32 hashid);
struct world_map* world_map_list_push(struct world_map_list* list);

enum world_map_transportation_mode {
    WORLD_MAP_TRANSPORTATION_MODE_FOOT,
    WORLD_MAP_TRANSPORTATION_MODE_BOAT,
};
struct world_map_view_settings {
    f32 scale;
    f32 focus;
    f32 horizon;
    f32 brightness_mod;

    f32 dx;
    f32 dy;
    f32 dw;
    f32 dh;
};
local struct world_map_view_settings world_map_view_settings_interpolate(struct world_map_view_settings a, struct world_map_view_settings b, f32 t) {
    struct world_map_view_settings result = (struct world_map_view_settings) {
        .scale          = lerp_f32(a.scale, b.scale, t),
        .focus          = lerp_f32(a.focus, b.focus, t),
        .horizon        = lerp_f32(a.horizon, b.horizon, t),
        .brightness_mod = lerp_f32(a.brightness_mod, b.brightness_mod, t),

        /* interpolating this is usually not a good idea, so I'd rather just jet it over. */
        .dx = b.dx,
        .dy = b.dy,
        .dw = b.dw,
        .dw = b.dh,
    };
    return result;
}
local struct world_map_view_settings world_map_view_on_foot = (struct world_map_view_settings) {
    .scale   = 200,
    .focus   = 700*2.5,
    .horizon = -660,

    .dx = 262 + TILE_UNIT_SIZE,
    .dy = 272,
    .dw = TILE_UNIT_SIZE*2,
    .dh = TILE_UNIT_SIZE*2,
    .brightness_mod = 0.8,
};
local struct world_map_view_settings world_map_view_on_boat = (struct world_map_view_settings) {
    .scale   = 500,
    .focus   = 400*3,
    .horizon = -1000,

    .dx = 283 + TILE_UNIT_SIZE/2,
    .dy = 343 - TILE_UNIT_SIZE/2,
    .dw = TILE_UNIT_SIZE,
    .dh = TILE_UNIT_SIZE,
    .brightness_mod = 0.95,
};

/*
  TODO: world map needs to wrap
*/
struct world_map_exploration_state {
    v2f32 player_position;
    f32   view_angle;

    /*
      Visit area prompt information
     */
    bool prompt_for_entering;
    s32  current_location_trigger; /* 0 for none. Index minus one addressing */

    /*
      TODO: boat information?
     */
    s32 transportation_mode;
    bool animating;
    f32 animation_t;
    struct world_map_view_settings current_view_settings;

    /*
      cutscene steering information:

      TODO: allow cutscene steering, this is just to test the system works.
     */
    bool cutscene_mode; /* turn off player drawing and UI drawing */
    struct camera precutscene_camera;

    /* NOTE: == -1 == infinite */
    f32 steer_time;
    f32 steer_velocity; /* in TILE_UNIT_SIZE */
    f32 steer_angular_velocity;
};

struct game_state {
    struct memory_arena* arena;

    struct world_map_exploration_state world_map_explore_state;

    /* This can't be guaranteed to be synchronized to whatever audio parallel crap I need */
    /* but I need this state here, which is useful for when I migrate to OpenAL */
    f32 music_fadein_timer;
    f32 music_fadeout_timer;
    bool started_fading_out;
    bool started_fading_in;

    u32 current_theme_track_type;
    u32 last_theme_track_type;

    /* NOTE main menu, or otherwise menus that occur in different states are not ui states. */
    u32 last_ui_state;
    u32 ui_state;

    s32 in_editor;

    struct game_variables variables;
    struct world_map_list world_maps;
    s32                   current_world_map_id;

    /* name is saved here so we can hash it's name later... */
    char              loaded_area_name[260];
    struct level_area loaded_area;

    struct random_state rng;
    struct camera camera;

    struct ui_pause_menu ui_pause;
    struct ui_sub_menu_inventory ui_inventory;

    struct player_party_inventory inventory;

    struct ui_save_menu ui_save;

    bool                 shopping;
    struct memory_arena  shopping_arena;
    struct shop_instance active_shop;

    struct entity_database entity_database;

    /* player and their companions, or familiars or something like that. */
    struct entity_list                  permenant_entities;
    struct entity_particle_emitter_list permenant_particle_emitters;
    struct projectile_entity_list       projectiles;

    s32                                 party_member_count;
    s32                                 leader_index;
    /*
     * 
     NOTE: All characters are intended to be templated off of a base entity, I can write
     character save changes to the save file, and have it override the base entity with
     the master record.

     This allows for character consistency. I can also do cool things like allowing Party Members
     that eventually turn on you, and fight but with all the stuff you taught them.

     Or partially merge them.

     NOTE: only party members will cause the save record to have an override entity database, since
     everyone else can be swapped with higher level shims or something.
     */
    entity_id                           party_members[MAX_PARTY_MEMBERS];

    struct weather      weather;

    /* I am a fan of animation, so we need to animate this. Even though it causes */
    /* state nightmares for me. */
    bool                is_conversation_active;
    /* ideally I'd interpolate all states, but if that's not possible just steal the position from here.
       I'm just saving everything just incase*/
    struct camera       before_conversation_camera;
    bool                viewing_dialogue_choices;
    struct conversation current_conversation;

    /* only player v world conflicts should happen. */
    struct game_state_combat_state combat_state;
    struct file_buffer  conversation_file_buffer;
    /* to store strings and stuff like that. Clear this when dialogue finishes (or opening new dlg)*/
    struct memory_arena conversation_arena;
    u32                 current_conversation_node_id;
    s32                 currently_selected_dialogue_choice;

    char                current_region_name[96];
    char                current_region_subtitle[128];

    /* only one interactable at a time */
    struct {
        s32   interactable_type;
        void* context;

        /* animation state... hahahahahaha */
    } interactable_state;

    /* Needs to be replicated for cutscenes */
    s32              dynamic_light_count;
    struct light_def dynamic_lights[MAX_GAME_DYNAMIC_LIGHT_POOL];

    bool constantly_traumatizing_camera;
    f32  constant_camera_trauma_value;
};


/* NOTE: 3/17/23, I really need to look at this again at some point...*/
bool game_can_load_save(s32 save_slot_id);
void game_write_save_slot(s32 save_slot_id);
void game_load_from_save_slot(s32 save_slot_id);
void dialogue_ui_set_target_node(s32 id);

void game_party_rest(void);

#include "save_data_def.c"
void handle_entity_level_trigger_interactions(struct game_state* state, struct entity* entity, struct trigger_level_transition_list* trigger_level_transitions, f32 dt);
void handle_entity_scriptable_trigger_interactions(struct game_state* state, struct entity* entity, struct trigger_list* list, f32 dt);

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

void serialize_light(struct binary_serializer* serializer, s32 version, struct light_def* light);

#include "credits_mode_def.c"

#endif
