struct level_area_navigation_map_tile {
    f32 score_modifier; 
    s32 type;          /* 0 ground, 1 solid, 2 obstacle(removable?) */
};

struct level_area_navigation_map {
    s32 width;
    s32 height;

    /* keep this here if I have to remap coordinates or something */
    s32 min_x;
    s32 min_y;

    s32 max_x;
    s32 max_y;

    struct level_area_navigation_map_tile* tiles;
};

/* NOTE: prepare for level data format migration
   
   We're going to add a few fixed layers, which are used for visual purposes,
   and likely separate our collision layer.
   
   The old format will not work anymore!
*/

/* We're going to listen for a few prescribed events. */
enum level_area_listen_event {
    LEVEL_AREA_LISTEN_EVENT_ON_TOUCH,
    LEVEL_AREA_LISTEN_EVENT_ON_ACTIVATE,
    LEVEL_AREA_LISTEN_EVENT_ON_DEATH,
#if 0
    LEVEL_AREA_LISTEN_EVENT_ON_HIT,
#endif
    LEVEL_AREA_LISTEN_EVENT_COUNT
};
struct level_area_listener {
    /*
      Should record trigger target here,
      
      Although it can be interpreted live as everything else... Might be easier
      to prestore targets later. However for now I guess I'll interpret it.
    */
    s32 subscribers;
    struct lisp_form* subscriber_codes;
};
local string level_area_listen_event_form_names[] = {
    string_literal("on-touch"),
    string_literal("on-activate"),
    string_literal("on-death"),
#if 0
    string_literal("on-hit"),
#endif
    string_literal("(count)"),
};

struct level_area_script_data {
    bool present;
    struct file_buffer buffer;
    struct lisp_list*  code_forms;

    struct lisp_form*  on_enter;
    struct lisp_form*  on_frame;
    struct lisp_form*  on_exit;

    struct level_area_listener listeners[LEVEL_AREA_LISTEN_EVENT_COUNT];
};

struct level_area {
    /* keep reference of a name. */
    u32          version;
    v2f32        default_player_spawn;

    s32          tile_count;
    struct tile* tiles;

    s32 trigger_level_transition_count;
    struct trigger_level_transition* trigger_level_transitions;

    s32 entity_chest_count;
    struct entity_chest* chests;

    /* runtime data */
    struct level_area_script_data script;
    struct level_area_navigation_map navigation_data;
    /* used for displaying what tiles you can walk to. */
    u8*                              combat_movement_visibility_map;
};
