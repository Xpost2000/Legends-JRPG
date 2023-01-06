#ifndef WORLD_MAP_DEF_C
#define WORLD_MAP_DEF_C

#define CURRENT_WORLD_MAP_VERSION (1)

/*
  VERSION 1:
  Tiles only + Player Spawn
*/

#define WORLD_SCRIPTABLE_TILE_LAYER_COUNT (32)
enum world_tile_layers {
    WORLD_TILE_LAYER_GROUND,            /* render below all. dark color? */
    WORLD_TILE_LAYER_OBJECT,            /* render at normal level as entities */
    WORLD_TILE_LAYER_CLUTTER_DECOR,     /* render at normal level as entities */
    WORLD_TILE_LAYER_OVERHEAD,          /* render above entities no fading */
    /* FLEXIBLE EXTRA LAYERS */
    WORLD_TILE_LAYER_SCRIPTABLE_0,
    WORLD_TILE_LAYER_SCRIPTABLE_1,
    WORLD_TILE_LAYER_SCRIPTABLE_2,
    WORLD_TILE_LAYER_SCRIPTABLE_3,
    WORLD_TILE_LAYER_SCRIPTABLE_4,
    WORLD_TILE_LAYER_SCRIPTABLE_5,
    WORLD_TILE_LAYER_SCRIPTABLE_6,
    WORLD_TILE_LAYER_SCRIPTABLE_7,
    WORLD_TILE_LAYER_SCRIPTABLE_8,
    WORLD_TILE_LAYER_SCRIPTABLE_9,
    WORLD_TILE_LAYER_SCRIPTABLE_10,
    WORLD_TILE_LAYER_SCRIPTABLE_11,
    WORLD_TILE_LAYER_SCRIPTABLE_12,
    WORLD_TILE_LAYER_SCRIPTABLE_13,
    WORLD_TILE_LAYER_SCRIPTABLE_14,
    WORLD_TILE_LAYER_SCRIPTABLE_15,
    WORLD_TILE_LAYER_SCRIPTABLE_16,
    WORLD_TILE_LAYER_SCRIPTABLE_17,
    WORLD_TILE_LAYER_SCRIPTABLE_18,
    WORLD_TILE_LAYER_SCRIPTABLE_19,
    WORLD_TILE_LAYER_SCRIPTABLE_20,
    WORLD_TILE_LAYER_SCRIPTABLE_21,
    WORLD_TILE_LAYER_SCRIPTABLE_22,
    WORLD_TILE_LAYER_SCRIPTABLE_23,
    WORLD_TILE_LAYER_SCRIPTABLE_24,
    WORLD_TILE_LAYER_SCRIPTABLE_25,
    WORLD_TILE_LAYER_SCRIPTABLE_26,
    WORLD_TILE_LAYER_SCRIPTABLE_27,
    WORLD_TILE_LAYER_SCRIPTABLE_28,
    WORLD_TILE_LAYER_SCRIPTABLE_29,
    WORLD_TILE_LAYER_SCRIPTABLE_30,
    WORLD_TILE_LAYER_SCRIPTABLE_31,
    WORLD_TILE_LAYER_COUNT
};

/*
  In many ways the world map is extremely similar to the level_area.

  In fact they are nearly identical. Just essentially no runtime data.

  World maps also have scripts just like level areas

  NOTE: because the world map shares many identical sub structures to the level area,
  I'm going to keep a secondary version identifier.

  the world map version id is mainly for structure while the area format is for the sub
  structures.
*/
struct world_map {
    s32   version;
    s32   area_format_version;
    v2f32 default_player_spawn;

    struct       scriptable_tile_layer_property scriptable_layer_properties[WORLD_SCRIPTABLE_TILE_LAYER_COUNT];
    struct tile*                                tile_layers[WORLD_TILE_LAYER_COUNT];
    s32                                         tile_counts[WORLD_TILE_LAYER_COUNT];

    string script_string;
};

#endif
