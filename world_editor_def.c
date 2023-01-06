#ifndef WORLD_EDITOR_DEF_C
#define WORLD_EDITOR_DEF_C

/* yep this is a separate editor... Shares some structure with editor_state */
struct world_editor_state {
    struct memory_arena* arena;
    s32    painting_tile_id;

    s32          tile_counts[WORLD_TILE_LAYER_COUNT];
    s32          tile_capacities[WORLD_TILE_LAYER_COUNT];
    struct tile* tile_layers[WORLD_TILE_LAYER_COUNT];
    s32          current_tile_layer;

    struct camera camera;
    v2f32         default_player_spawn;

    struct {
        /* NOTE this pointer should always have a rectangle as it's first member! (rectangle_f32) */
        void* context; /* if this pointer is non-zero we are dragging */

        bool has_size;
        v2f32 initial_mouse_position; /* assume this to be in "world/tile" coordinates */
        /* context sensitive information to be filled */
        v2f32 initial_object_position;
        v2f32 initial_object_dimensions;
    } drag_data;

    bool  was_already_camera_dragging;
    v2f32 initial_mouse_position;
    v2f32 initial_camera_position;

    void* last_selected;

    char current_save_name[128];
    u16 editor_brush_pattern;

    /* ??? */
    /* for edits that may require cross area interaction */
    /* we just load another full level and display it normally without editor mode stuff */
    char loaded_area_name[260]; /* level_areas don't know where they come from... */
    struct level_area loaded_area;
    bool       viewing_loaded_area;
};

#endif
