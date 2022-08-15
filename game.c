/* TODO fix coordinate system <3 */

/* virtual pixels */
#define TILE_UNIT_SIZE (32) /* measured with a reference of 640x480 */

#include "game_def.c"
#include "save_data_def.c"
#include "dialogue_script_parse.c"
#include "game_script_def.c"
#include "storyboard_presentation_def.c"

struct game_state* game_state         = 0;
local bool         disable_game_input = false;

static struct memory_arena game_arena   = {};
/* compile out */
#ifdef USE_EDITOR
struct editor_state* editor_state = 0;
static struct memory_arena editor_arena = {};
#endif

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

    MENU_FONT_COUNT,
};
static string menu_font_variation_string_names[] = {
    string_literal("res/fonts/gnsh-bitmapfont-colour1.png"),
    string_literal("res/fonts/gnsh-bitmapfont-colour2.png"),
    string_literal("res/fonts/gnsh-bitmapfont-colour3.png"),
    string_literal("res/fonts/gnsh-bitmapfont-colour4.png"),
    string_literal("res/fonts/gnsh-bitmapfont-colour5.png"),
    string_literal("res/fonts/gnsh-bitmapfont-colour6.png"),
    string_literal("res/fonts/gnsh-bitmapfont-colour7.png"),
    string_literal("res/fonts/gnsh-bitmapfont-colour8.png"),
    string_literal("res/fonts/gnsh-bitmapfont-colour9.png"),
    string_literal("res/fonts/gnsh-bitmapfont-colour10.png"),
};
font_id menu_fonts[MENU_FONT_COUNT];
/* replace old occurances of this */
local struct font_cache* game_get_font(s32 variation) {
    struct font_cache* font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[variation]);
    return font;
}

image_id combat_square_unselected;
image_id combat_square_selected;
image_id selection_sword_img;
image_id chest_closed_img, chest_open_bottom_img, chest_open_top_img;

/* TODO, use this */
/* I need a way to color the border and thingy separately... For nicer looks, since white border is better with blue.*/
struct game_ui_nine_patch ui_chunky;

struct game_ui_nine_patch game_ui_nine_patch_load_from_directory(struct graphics_assets* graphics_assets, string directory, s32 tile_width, s32 tile_height) {
    struct game_ui_nine_patch ui_skin = {}; 

    {
        s32 index = 0;

        Array_For_Each(it, string, game_ui_nine_patch_id_strings, array_count(game_ui_nine_patch_id_strings)) {
            string filepath         = string_from_cstring(format_temp("%.*s/%.*s.png", directory.length, directory.data, it->length, it->data));
            ui_skin.textures[index] = graphics_assets_load_image(graphics_assets, filepath);
            index++;
        }

        ui_skin.tile_width  = tile_width;
        ui_skin.tile_height = tile_height;
    }

    return ui_skin;
}
/* NOTE: the nine patch can only be drawn in tile sizes since it won't look stupid that way. */
v2f32 nine_patch_estimate_extents(struct game_ui_nine_patch ui_skin, f32 scaling_factor, s32 tiles_width, s32 tiles_height) {
    v2f32 result = {};
    result.x = ui_skin.tile_width * tiles_width * scaling_factor;
    result.y = ui_skin.tile_height * tiles_height * scaling_factor;
    return result;
}

void draw_nine_patch_ui(struct graphics_assets* graphics_assets, struct software_framebuffer* framebuffer, struct game_ui_nine_patch ui_skin, f32 scaling_factor, v2f32 xy, s32 tiles_width, s32 tiles_height, union color32f32 rgba) {
    struct image_buffer* imgs[9] = {};
    for (s32 index = 0; index < array_count(game_ui_nine_patch_id_strings); ++index) {
        imgs[index] = graphics_assets_get_image_by_id(graphics_assets, ui_skin.textures[index]);
    }

    s32 img_width  = ui_skin.tile_width * scaling_factor;
    s32 img_height = ui_skin.tile_height* scaling_factor;

#if 1
    software_framebuffer_draw_image_ex(framebuffer, imgs[NINE_PATCH_TOP_LEFT],
                                       rectangle_f32(xy.x, xy.y, img_width, img_height),
                                       RECTANGLE_F32_NULL,
                                       rgba, NO_FLAGS, BLEND_MODE_ALPHA);
    software_framebuffer_draw_image_ex(framebuffer, imgs[NINE_PATCH_TOP_RIGHT],
                                       rectangle_f32(xy.x + tiles_width * img_width, xy.y, img_width, img_height),
                                       RECTANGLE_F32_NULL,
                                       rgba, NO_FLAGS, BLEND_MODE_ALPHA);
    software_framebuffer_draw_image_ex(framebuffer, imgs[NINE_PATCH_BOTTOM_LEFT],
                                       rectangle_f32(xy.x, xy.y + img_height * tiles_height, img_width, img_height),
                                       RECTANGLE_F32_NULL,
                                       rgba, NO_FLAGS, BLEND_MODE_ALPHA);
    software_framebuffer_draw_image_ex(framebuffer, imgs[NINE_PATCH_BOTTOM_RIGHT],
                                       rectangle_f32(xy.x + tiles_width * img_width, xy.y + img_height * tiles_height, img_width, img_height),
                                       RECTANGLE_F32_NULL,
                                       rgba, NO_FLAGS, BLEND_MODE_ALPHA);

    for (s32 x = 1; x < tiles_width; ++x) {
        software_framebuffer_draw_image_ex(framebuffer, imgs[NINE_PATCH_TOP],
                                           rectangle_f32(xy.x + x * img_width, xy.y, img_width, img_height),
                                           RECTANGLE_F32_NULL,
                                           rgba, NO_FLAGS, BLEND_MODE_ALPHA);
        software_framebuffer_draw_image_ex(framebuffer, imgs[NINE_PATCH_BOTTOM],
                                           rectangle_f32(xy.x + x * img_width, xy.y + img_height * tiles_height, img_width, img_height),
                                           RECTANGLE_F32_NULL,
                                           rgba, NO_FLAGS, BLEND_MODE_ALPHA);
    }
    for (s32 y = 1; y < tiles_height; ++y) {
        software_framebuffer_draw_image_ex(framebuffer, imgs[NINE_PATCH_LEFT],
                                           rectangle_f32(xy.x, xy.y + img_height * y, img_width, img_height),
                                           RECTANGLE_F32_NULL,
                                           rgba, NO_FLAGS, BLEND_MODE_ALPHA);
        software_framebuffer_draw_image_ex(framebuffer, imgs[NINE_PATCH_RIGHT],
                                           rectangle_f32(xy.x + tiles_width * img_width, xy.y + img_height * y, img_width, img_height),
                                           RECTANGLE_F32_NULL,
                                           rgba, NO_FLAGS, BLEND_MODE_ALPHA);
    }

    for (s32 y = 1; y < tiles_height; ++y) {
        for (s32 x = 1; x < tiles_width; ++x) {
            software_framebuffer_draw_image_ex(framebuffer, imgs[NINE_PATCH_MIDDLE],
                                               rectangle_f32(xy.x + (x) * img_width, xy.y + (y) * img_height, img_width, img_height),
                                               RECTANGLE_F32_NULL,
                                               rgba, NO_FLAGS, BLEND_MODE_ALPHA);
        }
    }
#endif
}

struct tile_data_definition* tile_table_data;
struct autotile_table*       auto_tile_info;

#include "region_change_presentation.c"
local void game_attempt_to_change_area_name(struct game_state* state, string name, string subtitle) {
    /* we don't need to check the subtitle. That doesn't matter. I would think. */
    if (!cstring_equal(name.data, state->current_region_name)) {
        cstring_copy(name.data,     state->current_region_name, Minimum(name.length, array_count(state->current_region_name)));
        if (subtitle.data && subtitle.length) {
            cstring_copy(subtitle.data, state->current_region_subtitle, Minimum(subtitle.length, array_count(state->current_region_subtitle)));
        } else {
            state->current_region_subtitle[0] = '\0';
        }
        initialize_region_zone_change();
    }
}

void game_finish_conversation(struct game_state* state) {
    state->is_conversation_active = false;
    file_buffer_free(&state->conversation_file_buffer);
}

local void render_combat_area_information(struct game_state* state, struct render_commands* commands, struct level_area* area);
local void render_tile_layer(struct render_commands* commands, struct level_area* area, s32 layer) {
    for (s32 index = 0; index < area->tile_counts[layer]; ++index) {
        s32 tile_id = area->tile_layers[layer][index].id;
        struct tile_data_definition* tile_data = tile_table_data + tile_id;

        image_id tex = get_tile_image_id(tile_data);

        render_commands_push_image(commands,
                                   graphics_assets_get_image_by_id(&graphics_assets, tex),
                                   rectangle_f32(area->tile_layers[layer][index].x * TILE_UNIT_SIZE, area->tile_layers[layer][index].y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE),
                                   tile_data->sub_rectangle,
                                   color32f32(1,1,1,1), NO_FLAGS, BLEND_MODE_ALPHA);
    }
}

void render_foreground_area(struct game_state* state, struct render_commands* commands, struct level_area* area) {
    {
        render_tile_layer(commands, area, TILE_LAYER_OVERHEAD);
        render_tile_layer(commands, area, TILE_LAYER_ROOF);
        render_tile_layer(commands, area, TILE_LAYER_FOREGROUND);
    }
}

/* requires player state to handle some specific layers */

void render_ground_area(struct game_state* state, struct render_commands* commands, struct level_area* area) {
    /* TODO do it lazy mode. Once only */

    /* Object & ground layer */
    {
        render_tile_layer(commands, area, TILE_LAYER_GROUND);
        render_tile_layer(commands, area, TILE_LAYER_OBJECT);
        render_tile_layer(commands, area, TILE_LAYER_CLUTTER_DECOR);
    }

    if (state->combat_state.active_combat) {
        /* eh could be named better */
        render_combat_area_information(state, commands, area);
    }

    /* _debugprintf("%d chests", area->entity_chest_count); */
    Array_For_Each(it, struct entity_chest, area->chests, area->entity_chest_count) {
        if (it->flags & ENTITY_CHEST_FLAGS_UNLOCKED) {
            render_commands_push_image(commands,
                                       graphics_assets_get_image_by_id(&graphics_assets, chest_open_top_img),
                                       rectangle_f32(it->position.x * TILE_UNIT_SIZE,
                                                     (it->position.y-0.5) * TILE_UNIT_SIZE,
                                                     it->scale.x * TILE_UNIT_SIZE,
                                                     it->scale.y * TILE_UNIT_SIZE),
                                       RECTANGLE_F32_NULL,
                                       color32f32(1,1,1,1), NO_FLAGS, BLEND_MODE_ALPHA);
            render_commands_push_image(commands,
                                       graphics_assets_get_image_by_id(&graphics_assets, chest_open_bottom_img),
                                       rectangle_f32(it->position.x * TILE_UNIT_SIZE,
                                                     it->position.y * TILE_UNIT_SIZE,
                                                     it->scale.x * TILE_UNIT_SIZE,
                                                     it->scale.y * TILE_UNIT_SIZE),
                                       RECTANGLE_F32_NULL,
                                       color32f32(1,1,1,1), NO_FLAGS, BLEND_MODE_ALPHA);
        } else {
            render_commands_push_image(commands,
                                       graphics_assets_get_image_by_id(&graphics_assets, chest_closed_img),
                                       rectangle_f32(it->position.x * TILE_UNIT_SIZE,
                                                     it->position.y * TILE_UNIT_SIZE,
                                                     it->scale.x * TILE_UNIT_SIZE,
                                                     it->scale.y * TILE_UNIT_SIZE),
                                       RECTANGLE_F32_NULL,
                                       color32f32(1,1,1,1), NO_FLAGS, BLEND_MODE_ALPHA);
        }
    }
}

entity_id player_id;
struct entity* game_dereference_entity(struct game_state* state, entity_id id) {
    switch (id.store_type) {
        case ENTITY_LIST_STORAGE_TYPE_PER_LEVEL: {
            return entity_list_dereference_entity(&state->loaded_area.entities, id);
        } break;

        case ENTITY_LIST_STORAGE_TYPE_PERMENANT_STORE:
        default: {
            return entity_list_dereference_entity(&state->permenant_entities, id);
        } break;
    }

    return NULL;
}

entity_id entity_list_create_player(struct entity_list* entities, v2f32 position) {
    entity_id      result  = entity_list_create_entity(entities);
    struct entity* player  = entity_list_dereference_entity(entities, result);

    assertion(player->flags & ENTITY_FLAGS_ACTIVE);
    player->flags    |= ENTITY_FLAGS_ALIVE;
    player->flags    |= ENTITY_FLAGS_PLAYER_CONTROLLED;
    player->health.value = 100;
    player->health.min = 100;
    player->health.max = 100;
    player->position  = position;
    /* align collision boxes a bit better later :) */
    player->scale.x = TILE_UNIT_SIZE*0.8;
    player->scale.y = TILE_UNIT_SIZE*0.8;

    player->name    = string_literal("Calamir");

    return result;
}
entity_id entity_list_create_badguy(struct entity_list* entities, v2f32 position) {
    entity_id result = entity_list_create_entity(entities);
    struct entity* e = entity_list_dereference_entity(entities, result);

    e->flags    |= ENTITY_FLAGS_ALIVE;
    e->position = position;
    e->scale.x  = TILE_UNIT_SIZE-2;
    e->scale.y  = TILE_UNIT_SIZE-2;
    e->health.value = 100;
    e->health.min = 100;
    e->health.max = 100;
    e->ai.flags = ENTITY_AI_FLAGS_AGGRESSIVE_TO_PLAYER;
    e->name    = string_literal("Ruffian");

    return result;
}

struct entity* game_get_player(struct game_state* state) {
    return entity_list_dereference_entity(&state->permenant_entities, player_id);
}

#include "weather.c"

/* does not account for layers right now. That's okay. */
struct tile* level_area_find_tile(struct level_area* level, s32 x, s32 y, s32 layer) {
    Array_For_Each(it, struct tile, level->tile_layers[layer], level->tile_counts[layer]) {
        if ((s32)(it->x)== x && (s32)(it->y) == y) {
            return it;
        }
    }

    return NULL;
}

/* NOTE: also builds other run time data but I don't want to change the name. */
local void build_navigation_map_for_level_area(struct memory_arena* arena, struct level_area* level) {
    if (level->tile_counts[TILE_LAYER_OBJECT] > 0 ||
        level->tile_counts[TILE_LAYER_GROUND]) {
        struct level_area_navigation_map* navigation_map = &level->navigation_data;

        s32 min_x = INT_MAX;
        s32 min_y = INT_MAX;
        s32 max_x = -INT_MAX;
        s32 max_y = -INT_MAX;

        Array_For_Each(it, struct tile, level->tile_layers[TILE_LAYER_OBJECT], level->tile_counts[TILE_LAYER_OBJECT]) {
            if ((s32)it->x < min_x) min_x = (s32)(it->x);
            if ((s32)it->y < min_y) min_y = (s32)(it->y);
            if ((s32)it->x > max_x) max_x = (s32)(it->x);
            if ((s32)it->y > max_y) max_y = (s32)(it->y);
        }
        Array_For_Each(it, struct tile, level->tile_layers[TILE_LAYER_GROUND], level->tile_counts[TILE_LAYER_GROUND]) {
            if ((s32)it->x < min_x) min_x = (s32)(it->x);
            if ((s32)it->y < min_y) min_y = (s32)(it->y);
            if ((s32)it->x > max_x) max_x = (s32)(it->x);
            if ((s32)it->y > max_y) max_y = (s32)(it->y);
        }

        navigation_map->min_x = min_x;
        navigation_map->min_y = min_y;
        navigation_map->max_x = max_x;
        navigation_map->max_y = max_y;

        navigation_map->width  = (max_x - min_x)+1;
        navigation_map->height = (max_y - min_y)+1;

        navigation_map->tiles                 = memory_arena_push_top(arena, sizeof(*navigation_map->tiles) * navigation_map->width * navigation_map->height);
        level->combat_movement_visibility_map = memory_arena_push_top(arena, navigation_map->width * navigation_map->height);

        for (s32 y_cursor = navigation_map->min_y; y_cursor < navigation_map->max_y; ++y_cursor) {
            for (s32 x_cursor = navigation_map->min_x; x_cursor < navigation_map->max_x; ++x_cursor) {
                struct level_area_navigation_map_tile* nav_tile = &navigation_map->tiles[((y_cursor - navigation_map->min_y) * navigation_map->width + (x_cursor - navigation_map->min_x))];

                struct tile* real_tile    = level_area_find_tile(level, x_cursor, y_cursor, TILE_LAYER_OBJECT);
                if (!real_tile) real_tile = level_area_find_tile(level, x_cursor, y_cursor, TILE_LAYER_GROUND);

                nav_tile->score_modifier = 1;
                if (real_tile) {
                    struct tile_data_definition* tile_data_entry = &tile_table_data[real_tile->id];
                    nav_tile->type                               = (tile_data_entry->flags & TILE_DATA_FLAGS_SOLID) && true;
                } else {
                    nav_tile->type           = 0;
                }
            }
        }
    }
}

local void level_area_clear_movement_visibility_map(struct level_area* level) {
    struct level_area_navigation_map* navigation_map          = &level->navigation_data;
    s32                               map_width               = navigation_map->width;
    s32                               map_height              = navigation_map->height;

    for (s32 y_cursor = 0; y_cursor < map_height; ++y_cursor) {
        for (s32 x_cursor = 0; x_cursor < map_width; ++x_cursor) {
            level->combat_movement_visibility_map[y_cursor * map_width + x_cursor] = 0;
        }
    }
}

local void level_area_build_movement_visibility_map(struct memory_arena* arena, struct level_area* level, s32 x, s32 y, s32 radius) {
    /* not a BFS, just brute force checkings... */
    struct level_area_navigation_map* navigation_map          = &level->navigation_data;
    s32                               map_width               = navigation_map->width;
    s32                               map_height              = navigation_map->height;

    f32 radius_sq = radius*radius;

    for (s32 y_cursor = navigation_map->min_y; y_cursor < navigation_map->max_y; ++y_cursor) {
        for (s32 x_cursor = navigation_map->min_x; x_cursor < navigation_map->max_x; ++x_cursor) {
            f32 distance = v2f32_distance_sq(v2f32(x_cursor, y_cursor), v2f32(x, y));

            if (distance <= radius_sq) {
                struct tile* t = level_area_find_tile(level, x_cursor, y_cursor, TILE_LAYER_OBJECT);
                if (!t) t = level_area_find_tile(level, x_cursor, y_cursor, TILE_LAYER_GROUND);

                if (t) {
                    struct tile_data_definition* tile_data_entry = &tile_table_data[t->id];

                    if (tile_data_entry->flags & TILE_DATA_FLAGS_SOLID)
                        continue;

                    level->combat_movement_visibility_map[(y_cursor - navigation_map->min_y) * map_width + (x_cursor - navigation_map->min_x)] = 1;
                }
            }
        }
    }
}

bool level_area_navigation_map_is_point_in_bounds(struct level_area_navigation_map* navigation_map, v2f32 point) {
    s32 x = point.x;
    s32 y = point.y;

    if (x >= navigation_map->min_x && x <= navigation_map->max_x &&
        y >= navigation_map->min_y && y <= navigation_map->max_y) {
        return true;
    }

    return false;
}

/* This is just going to start as a breadth first search */
struct navigation_path navigation_path_find(struct memory_arena* arena, struct level_area* area, v2f32 start, v2f32 end) {
    struct level_area_navigation_map* navigation_map          = &area->navigation_data;
    struct navigation_path            results                 = {};


    /* we can only path find between two points that exist on our map. If you're out of bounds AI will not work (for obvious reasons...) */
    /* _debugprintf("finding path from <%d, %d> to <%d, %d>", (s32)start.x, (s32)start.y, (s32)end.x, (s32)end.y); */
    if (level_area_navigation_map_is_point_in_bounds(navigation_map, start) &&
        level_area_navigation_map_is_point_in_bounds(navigation_map, end)) {
        s32                               map_width               = navigation_map->width;
        s32                               map_height              = navigation_map->height;
        s32                               total_elements          = (map_width * map_height);

        s32 arena_memory_marker = arena->used;
        /* I don't have a stack based arena, so I'm going to manually store my memory usage and just "undo" if this fails */

        /* get a placeholder pointer */
        results.points = memory_arena_push(arena, 0);
        bool can_bresenham_trace = true;

        /* try to trace a bresenham's line to the destination if possible */
        {
            {
                s32 x1 = start.x;
                s32 x2 = end.x;
                s32 y1 = start.y;
                s32 y2 = end.y;

                s32 delta_x = abs(x2 - x1);
                s32 delta_y = -abs(y2 - y1);
                s32 sign_x  = 0;
                s32 sign_y  = 0;

                if (x1 < x2) sign_x = 1;
                else         sign_x = -1;

                if (y1 < y2) sign_y = 1;
                else         sign_y = -1;

                s32 error_accumulator = delta_x + delta_y;
                bool done_tracing = false;

                for (;!done_tracing && can_bresenham_trace;) {
                    if (x1 < navigation_map->max_x && x1 >= navigation_map->min_x && y1 < navigation_map->max_y && y1 >= navigation_map->min_y) {
                        memory_arena_push(arena, sizeof(*results.points));
                        v2f32* new_point = &results.points[results.count++];

                        new_point->x = x1;
                        new_point->y = y1;

                        struct level_area_navigation_map_tile* tile = &navigation_map->tiles[((s32)new_point->y - navigation_map->min_y) * map_width + ((s32)new_point->x - navigation_map->min_x)];
                        if (tile->type != 0) {
                            can_bresenham_trace = false;
                        }
                    }

                    if (x1 == x2 && y1 == y2) done_tracing = true;

                    s32 old_error_x2 = 2 * error_accumulator;

                    if (old_error_x2 >= delta_y) {
                        if (x1 != x2) {
                            error_accumulator += delta_y;
                            x1 += sign_x;
                        }
                    }

                    if (old_error_x2 <= delta_x) {
                        if (y1 != y2) {
                            error_accumulator += delta_x;
                            y1 += sign_y;
                        }
                    }
                }
            }

            if (!can_bresenham_trace) {
                arena->used = arena_memory_marker;
                results.points = 0;
                results.count  = 0;
            }
        }

        if (!can_bresenham_trace) {
            s32                               exploration_queue_start = 0;
            s32                               exploration_queue_end   = 0;
            v2f32*                            exploration_queue       = memory_arena_push(arena, sizeof(*exploration_queue) * total_elements);

            s32                               origin_path_count       = 0;
            v2f32*                            origin_paths            = memory_arena_push(arena, sizeof(*origin_paths)      * total_elements);

            s32                               explored_point_count    = 0;
            bool*                             explored_points         = memory_arena_push(arena, sizeof(*explored_points)   * total_elements);


            exploration_queue[exploration_queue_end++]     = start;
            origin_paths     [((s32)start.y - navigation_map->min_y) * map_width + ((s32)start.x - navigation_map->min_x)] = start;

            bool found_end = false;

            /* TODO: allow special case, if we can do a bresenham line, just use the trace */

            while (exploration_queue_start <= exploration_queue_end && !found_end) {
                v2f32 current_point = exploration_queue[exploration_queue_start++];
                /* _debugprintf("current point: <%d, %d>", (s32)current_point.x, (s32)current_point.y); */
                explored_points[((s32)current_point.y - navigation_map->min_y) * map_width + ((s32)current_point.x - navigation_map->min_x)] = true;

                /* add neighbors */
                /* might have to make four neighbors. We can configure it anyhow */
                /* _debugprintf("try to find neighbors"); */
                {
                    local struct {
                        s32 x;
                        s32 y;
                    }  neighbor_offsets[] = {
                        {1, 0},
                        {0, 1},
                        {-1, 0},
                        {0, -1},
                    };

                    for (s32 index = 0; index < array_count(neighbor_offsets); ++index) {
                        v2f32 proposed_point  = current_point;
                        proposed_point.x     += neighbor_offsets[index].x;
                        proposed_point.y     += neighbor_offsets[index].y;

                        /* _debugprintf("neighbor <%d, %d> (origin as: <%d, %d>) (%d, %d offset) proposed", (s32)proposed_point.x, (s32)proposed_point.y, (s32)current_point.x, (s32)current_point.y, x_cursor, y_cursor); */
                        if (level_area_navigation_map_is_point_in_bounds(navigation_map, proposed_point)) {
                            struct level_area_navigation_map_tile* tile = &navigation_map->tiles[((s32)proposed_point.y - navigation_map->min_y) * map_width + ((s32)proposed_point.x - navigation_map->min_x)];

                            if (tile->type == 0) {
                                if (!(explored_points[((s32)proposed_point.y - navigation_map->min_y) * map_width + ((s32)proposed_point.x - navigation_map->min_x)])) {
                                    origin_paths     [((s32)proposed_point.y - navigation_map->min_y) * map_width + ((s32)proposed_point.x - navigation_map->min_x)] = current_point;
                                    explored_points  [((s32)proposed_point.y - navigation_map->min_y) * map_width + ((s32)proposed_point.x - navigation_map->min_x)] = true;
                                    exploration_queue[exploration_queue_end++]                                                                                       = proposed_point;
                                    /* _debugprintf("neighbor <%d, %d> (origin as: <%d, %d>) (%d, %d offset) is okay to add", (s32)proposed_point.x, (s32)proposed_point.y, (s32)current_point.x, (s32)current_point.y, x_cursor, y_cursor); */
                                } else {
                                    /* _debugprintf("refused, already visited") ; */
                                }
                            } else {
                                /* _debugprintf("refused, solid!"); */
                            }
                        } else {
                            /* _debugprintf("refused... Not in bounds"); */
                        }

                    }
                }

                if (current_point.x == end.x && current_point.y == end.y) {
                    found_end = true;
                    /* _debugprintf("found end"); */

                    s32 path_length = 1;
                    bool found_start = false;
                    while (!found_start) {
                        v2f32 old_current = current_point;
                        current_point = origin_paths[((s32)current_point.y - navigation_map->min_y) * map_width + ((s32)current_point.x - navigation_map->min_x)];

                        path_length++;

                        if (current_point.x == start.x && current_point.y == start.y) {
                            found_start = true;
                        }
                    }

                    results.count = path_length;
                    results.points  = memory_arena_push(arena, sizeof(*results.points) * results.count);

                    found_start = false;

                    s32 index = 0;
                    current_point = end;
                    while (!found_start) {
                        results.points[index++] = current_point;
                        current_point        = origin_paths[((s32)current_point.y - navigation_map->min_y) * map_width + ((s32)current_point.x - navigation_map->min_x)];

                        if (current_point.x == start.x && current_point.y == start.y) {
                            found_start = true;
                        }
                    }
                    results.points[index++] = start;
                    Reverse_Array_Inplace(results.points, path_length, v2f32);
                }
            }
        }
    } else {
        _debugprintf("there's a point that's outside the map... Can't nav!");
    }

    return results;
}

void level_area_entity_unpack(struct level_area_entity* entity, struct entity* unpack_target);
void serialize_level_area(struct game_state* state, struct binary_serializer* serializer, struct level_area* level, bool use_default_spawn) {
    _debugprintf("%d memory used", state->arena->used + state->arena->used_top);
    memory_arena_clear_top(state->arena);
    _debugprintf("reading version");
    serialize_u32(serializer, &level->version);
    _debugprintf("reading default player spawn");
    serialize_f32(serializer, &level->default_player_spawn.x);
    serialize_f32(serializer, &level->default_player_spawn.y);
    _debugprintf("reading tiles");

    if (level->version >= 4) {
        if (level->version < CURRENT_LEVEL_AREA_VERSION) {
            /* for older versions I have to know what the tile layers were and assign them like this. */
            switch (level->version) {
                case 4: {
                    Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, state->arena, s32, level->tile_counts[TILE_LAYER_GROUND],     level->tile_layers[TILE_LAYER_GROUND]);
                    Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, state->arena, s32, level->tile_counts[TILE_LAYER_OBJECT],     level->tile_layers[TILE_LAYER_OBJECT]);
                    Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, state->arena, s32, level->tile_counts[TILE_LAYER_ROOF],       level->tile_layers[TILE_LAYER_ROOF]);
                    Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, state->arena, s32, level->tile_counts[TILE_LAYER_FOREGROUND], level->tile_layers[TILE_LAYER_FOREGROUND]);
                } break;
                default: {
                    
                } break;
            }
        } else {
            /* the current version of the tile layering, we can just load them in order. */
            for (s32 index = 0; index < TILE_LAYER_COUNT; ++index) {
                Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, state->arena, s32, level->tile_counts[index], level->tile_layers[index]);
            }
        }
    } else {
        Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, state->arena, s32, level->tile_counts[TILE_LAYER_OBJECT], level->tile_layers[TILE_LAYER_OBJECT]);
    }

    if (level->version >= 1) {
        _debugprintf("reading level transitions");
        Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, state->arena, s32, level->trigger_level_transition_count, level->trigger_level_transitions);
        /* this thing is allergic to chest updates. Unfortunately it might happen a lot... */
    }
    if (level->version >= 2) {
        _debugprintf("reading containers");
        Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, state->arena, s32, level->entity_chest_count, level->chests);
    }
    if (level->version >= 3) {
        _debugprintf("reading scriptable triggers");
        Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, state->arena, s32, level->script_trigger_count, level->script_triggers);
    }
    if (level->version >= 5) {
        struct level_area_entity current_packed_entity = {};

        _debugprintf("reading and unpacking entities");

        s32 entity_count = 0;
        serialize_s32(serializer, &entity_count);
        _debugprintf("Seeing %d entities to read", entity_count);

        /* TODO: 
           hack, this is incorrect behavior as we need to more fine-tune the iterator.
           
           Now since this additional "sentinel" entity is never initialized, it makes no difference anyways.
           I'll correct this later.
        */
        level->entities = entity_list_create_top(state->arena, (entity_count+1), ENTITY_LIST_STORAGE_TYPE_PER_LEVEL);

        for (s32 entity_index = 0; entity_index < entity_count; ++entity_index) {
            Serialize_Structure(serializer, current_packed_entity);
            struct entity* current_entity = 0;

            entity_id new_ent = entity_list_create_entity(&level->entities);
            current_entity    = entity_list_dereference_entity(&level->entities, new_ent);

            struct entity_base_data* base_data = entity_database_find_by_name(&game_state->entity_database, string_from_cstring(current_packed_entity.base_name));
            entity_base_data_unpack(base_data, current_entity);
            level_area_entity_unpack(&current_packed_entity, current_entity);
        }
    }

    /* until we have new area transititons or whatever. */
    /* TODO dumb to assume only the player... but okay */
    struct entity* player = entity_list_dereference_entity(&state->permenant_entities, player_id);
    if (use_default_spawn) {
        player->position.x             = level->default_player_spawn.x;
        player->position.y             = level->default_player_spawn.y;
    }

    state->camera.xy.x = player->position.x;
    state->camera.xy.y = player->position.y;

    build_navigation_map_for_level_area(state->arena, level);
}

local void load_area_script(struct memory_arena* arena, struct level_area* area, string script_name) {
    /* NOTE: all of this stuff needs to allocate memory from the top!!! */
    struct level_area_script_data* script_data = &area->script;

    if (file_exists(script_name)) {
        script_data->present     = true;
        /* This can technically be loaded into the arena top! */
        script_data->buffer      = read_entire_file(heap_allocator(), script_name);
        script_data->code_forms  = memory_arena_push(arena, sizeof(*script_data->code_forms));
        *script_data->code_forms = lisp_read_string_into_forms(arena, file_buffer_as_string(&script_data->buffer));

        /* search for on_enter, on_frame, on_exit */
        {
            for (s32 index = 0;
                 index < script_data->code_forms->count &&
                     ((!script_data->on_enter) ||
                      (!script_data->on_frame) ||
                      (!script_data->on_exit));
                 ++index) {
                struct lisp_form* form = script_data->code_forms->forms + index;

                if (lisp_form_as_function_list_check_fn_name(form, string_literal("on-enter"))) {
                    if (script_data->on_enter) {
                        _debugprintf("already assigned event on-enter!");
                    } else {
                        script_data->on_enter = form;
                    }
                } else if (lisp_form_as_function_list_check_fn_name(form, string_literal("on-frame"))) {
                    if (script_data->on_frame) {
                        _debugprintf("already assigned event on-frame");
                    } else {
                        script_data->on_frame = form;
                    }
                } else if (lisp_form_as_function_list_check_fn_name(form, string_literal("on-ext"))) {
                    if (script_data->on_exit) {
                        _debugprintf("already assigned event on-exit");
                    } else {
                        script_data->on_exit = form;
                    }
                }
            }
        }
        /* count the event listeners and handle them */
        /* TODO: handle event listeners */
        {
            s32 event_listener_type_counters[LEVEL_AREA_LISTEN_EVENT_COUNT] = {};

            /* count */
            {
                for (s32 index = 0; index < script_data->code_forms->count; ++index) {
                    struct lisp_form* form = script_data->code_forms->forms + index;

                    for (s32 name_index = 0; name_index < LEVEL_AREA_LISTEN_EVENT_COUNT; ++name_index) {
                        if (lisp_form_as_function_list_check_fn_name(form, level_area_listen_event_form_names[name_index])) {
                            event_listener_type_counters[name_index] += 1;
                            _debugprintf("found an event listener");
                        }
                    }
                }
            }

            /* allocate */
            {
                for (s32 event_listener_type = 0; event_listener_type < LEVEL_AREA_LISTEN_EVENT_COUNT; ++event_listener_type) {
                    script_data->listeners[event_listener_type].subscriber_codes =
                        memory_arena_push(arena, sizeof(*script_data->listeners->subscriber_codes) * event_listener_type_counters[event_listener_type]);
                }
            }

            /* assign */
            {
                for (s32 index = 0; index < script_data->code_forms->count; ++index) {
                    struct lisp_form* form = script_data->code_forms->forms + index;

                    for (s32 name_index = 0; name_index < LEVEL_AREA_LISTEN_EVENT_COUNT; ++name_index) {
                        if (lisp_form_as_function_list_check_fn_name(form, level_area_listen_event_form_names[name_index])) {
                            script_data->listeners[name_index].subscriber_codes[script_data->listeners[name_index].subscribers++] = *form;
                        }
                    }
                }
            }
        }
    } else {
        _debugprintf("NOTE: %.*s does not exist! No script for this level.", script_name.length, script_name.data);
    }
}

struct lisp_form level_area_find_listener_for_object(struct game_state* state, struct level_area* area, s32 listener_type, s32 listener_target_type, s32 listener_target_id) {
    struct level_area_listener* listener_lists = area->script.listeners + listener_type;

    _debugprintf("Finding a listener for script referencing!");
    for (s32 subscriber_index = 0; subscriber_index < listener_lists->subscribers; ++subscriber_index) {
        struct lisp_form* event_action_form = listener_lists->subscriber_codes + subscriber_index;
        struct lisp_form* object_handle     = lisp_list_nth(event_action_form, 1);

        if (game_script_object_handle_matches_object(*object_handle, listener_target_type, listener_target_id)) {
            _debugprintf("found correct listener");
            struct lisp_form body = lisp_list_sliced(*event_action_form, 2, -1);
            return body;
        }
    }

    return LISP_nil;
}

/* this is used for cheating or to setup the game I suppose. */
local void level_area_clean_up(struct level_area* area) {
    if (area->script.present) {
        file_buffer_free(&area->script.buffer);
        area->script.present = false;

        area->script.code_forms = 0;
        area->script.on_enter = 0;
        area->script.on_frame = 0;
        area->script.on_exit = 0;
    }
}

void load_level_from_file(struct game_state* state, string filename) {
    level_area_clean_up(&state->loaded_area);
    memory_arena_clear_top(state->arena);

    state->level_area_on_enter_triggered = false;

    string fullpath = string_concatenate(&scratch_arena, string_literal("areas/"), filename);
    struct binary_serializer serializer = open_read_file_serializer(fullpath);
    serialize_level_area(state, &serializer, &state->loaded_area, true);
    load_area_script(game_state->arena, &state->loaded_area, string_concatenate(&scratch_arena, string_slice(fullpath, 0, (fullpath.length+1)-sizeof("area")), string_literal("area_script")));
    {
        cstring_copy(filename.data, state->loaded_area_name, filename.length);
    }
    serializer_finish(&serializer);
}

/* true - changed, false - same */
bool game_state_set_ui_state(struct game_state* state, u32 new_ui_state) {
    if (state->ui_state != new_ui_state) {
        state->last_ui_state = state->ui_state;
        state->ui_state      = new_ui_state;
        
        return true;
    }

    return false;
}

#include "entity.c"


void game_postprocess_blur(struct software_framebuffer* framebuffer, s32 quality_scale, f32 t, u32 blend_mode) {
#ifdef NO_POSTPROCESSING
    return;
#endif

    f32 box_blur[] = {
        1,1,1,
        1,1,1,
        1,1,1,
    };

    struct software_framebuffer blur_buffer = software_framebuffer_create(&scratch_arena, framebuffer->width/quality_scale, framebuffer->height/quality_scale);
    software_framebuffer_copy_into(&blur_buffer, framebuffer);
    software_framebuffer_kernel_convolution_ex(&scratch_arena, &blur_buffer, box_blur, 3, 3, 12, t, 2);
    software_framebuffer_draw_image_ex(framebuffer, (struct image_buffer*)&blur_buffer, RECTANGLE_F32_NULL, RECTANGLE_F32_NULL, color32f32(1,1,1,1), NO_FLAGS, blend_mode);
}

void game_postprocess_blur_ingame(struct software_framebuffer* framebuffer, s32 quality_scale, f32 t, u32 blend_mode) {
#ifdef NO_POSTPROCESSING
    return;
#endif

    f32 box_blur[] = {
        1,1.5,1,
        1.5,1,1.5,
        1,1.5,1,
    };

    /* output buffer */
    struct software_framebuffer blur_buffer = software_framebuffer_create(&scratch_arena, framebuffer->width/quality_scale, framebuffer->height/quality_scale);
    software_framebuffer_copy_into(&blur_buffer, framebuffer);
    software_framebuffer_kernel_convolution_ex(&scratch_arena, &blur_buffer, box_blur, 3, 3, 10, t, 1);
    software_framebuffer_draw_image_ex(framebuffer, (struct image_buffer*)&blur_buffer, RECTANGLE_F32_NULL, RECTANGLE_F32_NULL, color32f32(1,1,1,1), NO_FLAGS, blend_mode);
}

void grayscale_shader(struct software_framebuffer* framebuffer, s32 pixel_x, s32 pixel_y, void* context) {
    f32 t = *(float*)context;
    u8 r = framebuffer->pixels[pixel_y * framebuffer->width * 4 + pixel_x * 4 + 0];
    u8 g = framebuffer->pixels[pixel_y * framebuffer->width * 4 + pixel_x * 4 + 1];
    u8 b = framebuffer->pixels[pixel_y * framebuffer->width * 4 + pixel_x * 4 + 2];
    f32 average = (r + g + b) / 3.0f;
    framebuffer->pixels[pixel_y * framebuffer->width * 4 + pixel_x * 4 + 0] = framebuffer->pixels[pixel_y * framebuffer->width * 4 + pixel_x * 4 + 0] * (1 - t) + average * t;
    framebuffer->pixels[pixel_y * framebuffer->width * 4 + pixel_x * 4 + 1] = framebuffer->pixels[pixel_y * framebuffer->width * 4 + pixel_x * 4 + 1] * (1 - t) + average * t;
    framebuffer->pixels[pixel_y * framebuffer->width * 4 + pixel_x * 4 + 2] = framebuffer->pixels[pixel_y * framebuffer->width * 4 + pixel_x * 4 + 2] * (1 - t) + average * t;
}

void game_postprocess_grayscale(struct software_framebuffer* framebuffer, f32 t) {
#ifdef NO_POSTPROCESSING
    return;
#endif
    software_framebuffer_run_shader(framebuffer, rectangle_f32(0, 0, framebuffer->width, framebuffer->height), grayscale_shader, (void*)&t);
}

local void draw_ui_breathing_text_centered(struct software_framebuffer* framebuffer, struct rectangle_f32 bounds, struct font_cache* font, f32 scale, string text, s32 seed_displacement, union color32f32 modulation) {
    f32 text_width  = font_cache_text_width(font, text, scale);
    f32 text_height = font_cache_calculate_height_of(font, text, bounds.w, scale);
    f32 font_height = font_cache_text_height(font) * scale;

    v2f32 centered_starting_position = v2f32(0,0);

    centered_starting_position.x = bounds.x + (bounds.w/2) - (text_width/2);
    centered_starting_position.y = bounds.y + (bounds.h/2) - (text_height/2);

    f32 x_cursor = centered_starting_position.x;
    f32 y_cursor = centered_starting_position.y;

    for (unsigned character_index = 0; character_index < text.length; ++character_index) {
        f32 character_displacement_y = sinf((global_elapsed_time*2) + ((character_index+seed_displacement) * 2381.2318)) * 3;

        v2f32 glyph_position = v2f32(x_cursor, y_cursor);
        glyph_position.y += character_displacement_y;
        glyph_position.x += font->tile_width * scale * character_index;

        /* TODO: fix this later */
        /* while (glyph_position.x > bounds.w) { */
        /*     glyph_position.x -= bounds.w; */
        /*     glyph_position.y += font_height; */
        /* } */

        software_framebuffer_draw_text(framebuffer, font, scale, glyph_position, string_slice(text, character_index, character_index+1), modulation, BLEND_MODE_ALPHA);
    }
}

local void draw_ui_breathing_text(struct software_framebuffer* framebuffer, v2f32 where, struct font_cache* font, f32 scale, string text, s32 seed_displacement, union color32f32 modulation) {
    for (unsigned character_index = 0; character_index < text.length; ++character_index) {
        f32 character_displacement_y = sinf((global_elapsed_time*2) + ((character_index+seed_displacement) * 2381.2318)) * 3;

        v2f32 glyph_position = where;
        glyph_position.y += character_displacement_y;
        glyph_position.x += font->tile_width * scale * character_index;

        software_framebuffer_draw_text(framebuffer, font, scale, glyph_position, string_slice(text, character_index, character_index+1), modulation, BLEND_MODE_ALPHA);
    }
}


#ifdef USE_EDITOR
void editor_initialize(struct editor_state* state);
#include "editor.c"
#endif

#include "tile_data.c"

local void initialize_main_menu(void);
void game_initialize(void) {
    game_arena   = memory_arena_create_from_heap("Game Memory", Megabyte(32));
#ifdef USE_EDITOR
    editor_arena = memory_arena_create_from_heap("Editor Memory", Megabyte(32));
#endif

    game_state                      = memory_arena_push(&game_arena, sizeof(*game_state));
    game_state->variables.variables = memory_arena_push(&game_arena, sizeof(*game_state->variables.variables) * 4096);
    game_state->conversation_arena  = memory_arena_push_sub_arena(&game_arena, Kilobyte(64));

    game_state->entity_database     = entity_database_create(&game_arena, 1024);
    
    initialize_save_data();
    initialize_main_menu();

    game_state->rng = random_state();
    game_state->arena = &game_arena;

    graphics_assets = graphics_assets_create(&game_arena, 64, 1024);
    scratch_arena = memory_arena_create_from_heap("Scratch Buffer", Megabyte(4));

    combat_square_unselected = graphics_assets_load_image(&graphics_assets, string_literal("./res/img/cmbt/cmbt_grid_sq.png"));
    combat_square_selected   = graphics_assets_load_image(&graphics_assets, string_literal("./res/img/cmbt/cmbt_selected_sq.png"));

    chest_closed_img      = graphics_assets_load_image(&graphics_assets, string_literal("./res/img/chestclosed.png"));
    chest_open_bottom_img = graphics_assets_load_image(&graphics_assets, string_literal("./res/img/chestopenbottom.png"));
    chest_open_top_img    = graphics_assets_load_image(&graphics_assets, string_literal("./res/img/chestopentop.png"));
    ui_chunky             = game_ui_nine_patch_load_from_directory(&graphics_assets, string_literal("./res/img/ui/chunky/"), 16, 16);
    /* selection_sword_img   = graphics_assets_load_image(&graphics_assets, string_literal("./res/img/selection_sword.png")); */
    global_entity_models = entity_model_database_create(&game_arena, 512);

    game_script_initialize(&game_arena);

    for (unsigned index = 0; index < array_count(menu_font_variation_string_names); ++index) {
        string current = menu_font_variation_string_names[index];
        menu_fonts[index] = graphics_assets_load_bitmap_font(&graphics_assets, current, 5, 12, 5, 20);
    }

    /* this should be the amount of entities/npcs I try to keep in active memory */
    /* the rest are paged to disk, or based off their save delta record */
    /*
      NOTE:
      IE: 
      The main source of truth should go like this:

      Game Active Memory (play zone, and maybe one adjacent level?),
      Save Record        (Saves some delta about entities and map state),
      Game Base Files    (Use if there is no existing save record on that level...)
     */
    game_state->permenant_entities = entity_list_create(&game_arena, GAME_MAX_PERMENANT_ENTITIES, ENTITY_LIST_STORAGE_TYPE_PERMENANT_STORE);
    player_id                      = entity_list_create_player(&game_state->permenant_entities, v2f32(70, 70));
    /* entity_list_create_badguy(&game_state->entities, v2f32(8 * TILE_UNIT_SIZE, 8 * TILE_UNIT_SIZE)); */
    /* entity_list_create_badguy(&game_state->entities, v2f32(11 * TILE_UNIT_SIZE, 8 * TILE_UNIT_SIZE)); */

    {
        {
            s32 base_guy = entity_model_database_add_model(&game_arena, string_literal("guy"));
            entity_model_add_animation(base_guy, string_literal("down"),       1, 0);
            entity_model_add_animation(base_guy, string_literal("up"),         1, 0);
            entity_model_add_animation(base_guy, string_literal("left"),       1, 0);
            entity_model_add_animation(base_guy, string_literal("right"),      1, 0);

            const f32 WALK_TIMINGS = 0.13;
            entity_model_add_animation(base_guy, string_literal("down_walk"),  3, WALK_TIMINGS);
            entity_model_add_animation(base_guy, string_literal("up_walk"),    3, WALK_TIMINGS);
            entity_model_add_animation(base_guy, string_literal("left_walk"),  3, WALK_TIMINGS);
            entity_model_add_animation(base_guy, string_literal("right_walk"), 3, WALK_TIMINGS);
        }
    }

#ifdef USE_EDITOR
    editor_state                = memory_arena_push(&editor_arena, sizeof(*editor_state));
    editor_initialize(editor_state);
#endif
    initialize_static_table_data();
    initialize_items_database();

    assertion(verify_no_item_id_name_hash_collisions());
    void game_initialize_game_world(void);
    game_initialize_game_world();
}

void game_initialize_game_world(void) {
    entity_inventory_add((struct entity_inventory*)&game_state->inventory, MAX_PARTY_ITEMS, item_id_make(string_literal("item_trout_fish_5")));
    entity_inventory_add((struct entity_inventory*)&game_state->inventory, MAX_PARTY_ITEMS, item_id_make(string_literal("item_armor_rags")));
    entity_inventory_add((struct entity_inventory*)&game_state->inventory, MAX_PARTY_ITEMS, item_id_make(string_literal("item_armor_loincloth")));
    entity_inventory_add((struct entity_inventory*)&game_state->inventory, MAX_PARTY_ITEMS, item_id_make(string_literal("item_armor_bandage_wraps")));
    entity_inventory_add((struct entity_inventory*)&game_state->inventory, MAX_PARTY_ITEMS, item_id_make(string_literal("item_accessory_wedding_ring")));

#if 1
    /* game_open_conversation_file(game_state, string_literal("./dlg/linear_test.txt")); */
    /* game_open_conversation_file(game_state, string_literal("./dlg/simple_choices.txt")); */
    load_level_from_file(game_state, string_literal("pf.area"));
#endif
    /* load_level_from_file(game_state, string_literal("bt.area")); */
    /* load_level_from_file(game_state, string_literal("testforest.area")); */
    load_level_from_file(game_state, string_literal("level.area"));
    /* load_level_from_file(game_state, string_literal("testisland.area")); */
    /* game_attempt_to_change_area_name(game_state, string_literal("Old Iyeila"), string_literal("Grave of Stars")); */
#if 0
    int _game_sandbox_testing(void);
    _game_sandbox_testing();
#endif
}

void game_deinitialize(void) {
    game_script_deinitialize();
    level_area_clean_up(&game_state->loaded_area);
    game_finish_conversation(game_state);
    finish_save_data();
    memory_arena_finish(&scratch_arena);
#ifdef USE_EDITOR
    memory_arena_finish(&editor_arena);
#endif
    graphics_assets_finish(&graphics_assets);
    memory_arena_finish(&game_arena);
}

local void game_loot_chest(struct game_state* state, struct entity_chest* chest) {
    struct entity* player = game_get_player(state);
    
    bool permit_unlocking = true;
    if (chest->flags & ENTITY_CHEST_FLAGS_REQUIRES_ITEM_FOR_UNLOCK) {
        if (entity_inventory_has_item((struct entity_inventory*)&state->inventory, chest->key_item)) {
            {
                char tmp[512]= {};
                struct item_def* item_base = item_database_find_by_id(chest->key_item);
                snprintf(tmp, 512, "Unlocked using \".*s\"", item_base->name.length, item_base->name.data);
                game_message_queue(string_from_cstring(tmp));
            }
        } else {
            game_message_queue(string_literal("Chest is locked!"));
            permit_unlocking = false;
        }
    }

    if (permit_unlocking) {
        Array_For_Each(it, struct item_instance, chest->inventory.items, chest->inventory.item_count) {
            entity_inventory_add_multiple((struct entity_inventory*)&state->inventory, MAX_PARTY_ITEMS, it->item, it->count);
            {
                char tmp[512]= {};
                struct item_def* item_base = item_database_find_by_id(it->item);
                snprintf(tmp, 512, "acquired %.*s x%d", item_base->name.length, item_base->name.data, it->count);
                game_message_queue(string_from_cstring(tmp));
            }
        }

        chest->flags |= ENTITY_CHEST_FLAGS_UNLOCKED;
        /* write to save data here */
        /* save_data_write_for(hash_bytes_fnv1a); */
    }
}

/* TODO: Add more advanced type of messages, or specific tutorial prompts.
   (Popups with titles, and images and fun stuff)
*/
void game_message_queue(string message) {
    assertion(global_popup_state.message_count < array_count(global_popup_state.messages));
    struct ui_popup_message_box* current_message = &global_popup_state.messages[global_popup_state.message_count++];
    /* NOTE, in this case this works since I just casted a message from a cstring. This is not really secure */
    s32 min_length = 0;
    if (message.length < array_count(current_message->message_storage)) {
        min_length = message.length;
    } else {
        min_length = array_count(current_message->message_storage);
    }

    cstring_copy(message.data, current_message->message_storage, min_length);
}

bool game_display_and_update_messages(struct software_framebuffer* framebuffer, f32 dt) {
    if (global_popup_state.message_count > 0) {
        /* haven't decided the stack order... Just do first in, last out for now */
        struct ui_popup_message_box* current_message = &global_popup_state.messages[global_popup_state.message_count-1];

        struct font_cache* font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_YELLOW]);
        string message_str      = string_from_cstring(current_message->message_storage);

        f32                  message_text_height = font_cache_calculate_height_of(font, message_str, framebuffer->width * 0.5, 2);
        struct rectangle_f32 message_region      = rectangle_f32_centered(rectangle_f32(0, 0, framebuffer->width, framebuffer->height), framebuffer->width * 0.5, message_text_height*1.15);

        {
            software_framebuffer_draw_quad(framebuffer, message_region, color32u8(30, 30, 200, 255), BLEND_MODE_ALPHA);
            software_framebuffer_draw_text_bounds(framebuffer, font, 2, v2f32(message_region.x+5, message_region.y+5), framebuffer->width * 0.5, message_str, color32f32(1,1,1,1), BLEND_MODE_ALPHA);
        }

        /* dismiss current message */
        if (is_key_pressed(KEY_RETURN)) {
            global_popup_state.message_count -= 1;
        }

        return true;
    }


    return false;
}

void game_display_and_update_damage_notifications(struct software_framebuffer* framebuffer, f32 dt) {
    struct font_cache* font       = game_get_font(MENU_FONT_COLOR_STEEL);
    struct font_cache* other_font = game_get_font(MENU_FONT_COLOR_ORANGE);

    struct camera* camera = &game_state->camera;

    for (s32 index = 0; index < global_damage_notification_count; ++index) {
        struct damage_notifier* notifier = global_damage_notifications + index;

        struct font_cache* painting_font = font;
        if (notifier->alternative_color) painting_font = other_font;

        {
            notifier->position.y -= 50 * dt;
            notifier->timer         += dt;
            notifier->flicker_timer += dt;

            if (notifier->timer >= DAMAGE_NOTIFIER_MAX_TIME) {
                global_damage_notifications[index] = global_damage_notifications[--global_damage_notification_count];
                continue;
            }

            if (notifier->flicker_timer >= DAMAGE_NOTIFIER_FLICKER_TIME) {
                notifier->alternative_color ^= 1;
                notifier->flicker_timer      = 0;
            }
        }

        v2f32 draw_position = notifier->position;
        draw_position       = camera_project(camera, draw_position, framebuffer->width, framebuffer->height);

        _debugprintf("%f, %f", draw_position.x, draw_position.y);

        draw_ui_breathing_text(framebuffer, notifier->position, painting_font, 3, string_from_cstring(format_temp("%d!", notifier->amount)), index, color32f32_WHITE);
    }
}

local void dialogue_try_to_execute_script_actions(struct game_state* state, struct conversation_node* node) {
    /* 
       scratch arena is okay for evaluation
       
       we can keep game/vm game state elsewhere.
     */
    if (node->script_code.length) {
        _debugprintf("dialogue script present");
        _debugprintf("dialogue script-code: %.*s", node->script_code.length, node->script_code.data);
        struct lisp_form code = lisp_read_form(&scratch_arena, node->script_code);
        /* we should already know this is a DO form so just go */

        /* TODO MARKME */
        struct lisp_list* list_contents = &code.list;
        for (unsigned index = 1; index < list_contents->count; ++index) {
            struct lisp_form* form = list_contents->forms + index;
            game_script_evaluate_form(&scratch_arena, state, form);
        }
    } else {
        _debugprintf("no dialogue script present");
    }
}
local void dialogue_choice_try_to_execute_script_actions(struct game_state* state, struct conversation_choice* choice) {
    if (choice->script_code.length) {
        _debugprintf("dialogue choice script present");
        _debugprintf("choice script-code: %.*s", choice->script_code.length, choice->script_code.data);
    } else {
        _debugprintf("no dialogue choice script present");
    }
}

#include "battle_ui.c"
#include "conversation_ui.c"

local void update_and_render_ingame_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    game_display_and_update_damage_notifications(framebuffer, dt);

    if (game_display_and_update_storyboard(framebuffer, dt))
        return;

    if (update_and_render_region_zone_change(state, framebuffer, dt))
        return;

    if (game_display_and_update_messages(framebuffer, dt))
        return;

    if (state->is_conversation_active) {
        update_and_render_conversation_ui(state, framebuffer, dt);
    }

    if (state->interactable_state.interactable_type != INTERACTABLE_TYPE_NONE) {
        struct font_cache* font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_BLUE]);
        switch (state->interactable_state.interactable_type) {
            case INTERACTABLE_TYPE_CHEST: {
                struct entity_chest* chest = state->interactable_state.context;
                software_framebuffer_draw_text_bounds_centered(framebuffer, font, 4, rectangle_f32(0, 400, framebuffer->width, framebuffer->height - 400),
                                                               string_literal("open chest"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);

                if (is_key_pressed(KEY_RETURN)) {
                    game_loot_chest(state, chest);
                }
            } break;
            default: {
                
            } break;
        }
    }

    /* I'm aware this causes a lot of pop in, which is okay. I can manually code transitions for those... but whatever */
    /* okay, here I can render the normal game UI stuff */
    if (game_state->combat_state.active_combat) {
        update_and_render_battle_ui(state, framebuffer, dt);
    } else {
        /* normal UI which I apparently don't seem to have. */
    }

    /* TODO Region zone change does not prevent pausing right now! */
    {
        if (is_key_pressed(KEY_ESCAPE)) {
            game_state_set_ui_state(state, UI_STATE_PAUSE);
            /* ready pause menu */
            {
                game_state->ui_pause.animation_state = 0;
                game_state->ui_pause.transition_t    = 0;
                game_state->ui_pause.selection       = 0;
                zero_array(game_state->ui_pause.shift_t);
            }
        }
    }
}

#include "sub_menu_state_inventory.c"
#include "sub_menu_state_equipment.c"

local void update_and_render_sub_menu_states(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    f32 font_scale = 3;
    struct ui_pause_menu* menu_state = &state->ui_pause;

    assertion(menu_state->sub_menu_state != UI_PAUSE_MENU_SUB_MENU_STATE_NONE && "This should be impossible.");
    switch (menu_state->sub_menu_state) {
        case UI_PAUSE_MENU_SUB_MENU_STATE_OPTIONS: {
            _debugprintf("options not done!");
            menu_state->animation_state     = UI_PAUSE_MENU_TRANSITION_IN;
            menu_state->last_sub_menu_state = menu_state->sub_menu_state;
            menu_state->sub_menu_state      = UI_PAUSE_MENU_SUB_MENU_STATE_NONE;
            menu_state->transition_t = 0;
        } break;
        case UI_PAUSE_MENU_SUB_MENU_STATE_EQUIPMENT: {
            /* We need to focus on the character id we are handling for future reference. */
            update_and_render_character_equipment_screen(state, framebuffer, dt);
        } break;
        case UI_PAUSE_MENU_SUB_MENU_STATE_INVENTORY: {
            update_and_render_party_inventory_screen(state, framebuffer, dt);
        } break;
    }
}

local void update_and_render_pause_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    /* needs a bit of cleanup */
    f32 font_scale = 3;
    /* While the real pause menu is going to be replaced with something else later obviously */
    /* I want a nice looking menu to show off, and also the main menu is likely taking this design */
    struct ui_pause_menu* menu_state = &state->ui_pause;
    v2f32 item_positions[array_count(ui_pause_menu_strings)] = {};

    for (unsigned index = 0; index < array_count(item_positions); ++index) {
        item_positions[index].y = 36 * (index+0.75);
    }

    f32 offscreen_x = -240;
    f32 final_x     = 40;

    u32 blur_samples = 2;
    f32 max_blur = 1.0;
    f32 max_grayscale = 0.8;

    f32 timescale = 1.34f;

    switch (menu_state->animation_state) {
        case UI_PAUSE_MENU_TRANSITION_IN: {
            menu_state->transition_t   += dt * timescale;

            for (unsigned index = 0; index < array_count(item_positions); ++index) {
                item_positions[index].x = lerp_f32(offscreen_x, final_x, menu_state->transition_t);
            }

            bool should_blur_fade = (menu_state->last_sub_menu_state == UI_PAUSE_MENU_SUB_MENU_STATE_NONE);

            if (should_blur_fade) {
                game_postprocess_blur(framebuffer, blur_samples, max_blur * (menu_state->transition_t), BLEND_MODE_ALPHA);
                game_postprocess_grayscale(framebuffer, max_grayscale * (menu_state->transition_t));
            } else {
                game_postprocess_blur(framebuffer, blur_samples, max_blur, BLEND_MODE_ALPHA);
                game_postprocess_grayscale(framebuffer, max_grayscale);
            }

            if (menu_state->transition_t >= 1.0f) {
                menu_state->animation_state += 1;
                menu_state->transition_t = 0;
            }
        } break;
        case UI_PAUSE_MENU_NO_ANIM: {
            game_postprocess_blur(framebuffer, blur_samples, max_blur, BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale);

            if (menu_state->sub_menu_state == UI_PAUSE_MENU_SUB_MENU_STATE_NONE) {
                for (unsigned index = 0; index < array_count(item_positions); ++index) {
                    item_positions[index].x = final_x;
                }

                for (unsigned index = 0; index < array_count(item_positions); ++index) {
                    if (index != menu_state->selection) {
                        menu_state->shift_t[index] -= dt*4;
                    }
                }
                menu_state->shift_t[menu_state->selection] += dt*4;
                for (unsigned index = 0; index < array_count(item_positions); ++index) {
                    menu_state->shift_t[index] = clamp_f32(menu_state->shift_t[index], 0, 1);
                }

                if (is_key_pressed(KEY_ESCAPE)) {
                    menu_state->animation_state = UI_PAUSE_MENU_TRANSITION_CLOSING;
                    menu_state->transition_t = 0;
                }        

                wrap_around_key_selection(KEY_UP, KEY_DOWN, &menu_state->selection, 0, array_count(item_positions));

                if (is_key_pressed(KEY_RETURN)) {
                    switch (menu_state->selection) {
                        case 0: {
                            menu_state->animation_state = UI_PAUSE_MENU_TRANSITION_CLOSING;
                            menu_state->transition_t = 0;
                        } break;
                        case 1: {
                                menu_state->animation_state     = UI_PAUSE_MENU_TRANSITION_CLOSING;
                                menu_state->last_sub_menu_state = UI_PAUSE_MENU_SUB_MENU_STATE_NONE;
                                menu_state->sub_menu_state      = UI_PAUSE_MENU_SUB_MENU_STATE_EQUIPMENT;
                                menu_state->transition_t        = 0;
                                open_equipment_screen(player_id);
                        } break;
                        case 2: {
                            {
                                menu_state->animation_state     = UI_PAUSE_MENU_TRANSITION_CLOSING;
                                menu_state->last_sub_menu_state = UI_PAUSE_MENU_SUB_MENU_STATE_NONE;
                                menu_state->sub_menu_state      = UI_PAUSE_MENU_SUB_MENU_STATE_INVENTORY;
                                open_party_inventory_screen();
                                menu_state->transition_t        = 0;
                            }
                        } break;
                        case 3: {
                            {
                                menu_state->animation_state     = UI_PAUSE_MENU_TRANSITION_CLOSING;
                                menu_state->last_sub_menu_state = UI_PAUSE_MENU_SUB_MENU_STATE_NONE;
                                menu_state->sub_menu_state      = UI_PAUSE_MENU_SUB_MENU_STATE_OPTIONS;
                                menu_state->transition_t        = 0;
                            }
                        } break;
                        case 4: {
                            global_game_running = false;
                        } break;
                    }
                }
            } else {
                for (unsigned index = 0; index < array_count(item_positions); ++index) {
                    item_positions[index].x = -99999999;
                }

                /* sub menus have their own state. I'm just nesting these. Which is fine since each menu unit is technically "atomic" */
                update_and_render_sub_menu_states(state, framebuffer, dt);
            }
        } break;
        case UI_PAUSE_MENU_TRANSITION_CLOSING: {
            menu_state->transition_t   += dt * timescale;

            for (unsigned index = 0; index < array_count(item_positions); ++index) {
                item_positions[index].x = lerp_f32(final_x, offscreen_x, menu_state->transition_t);
            }

            bool should_blur_fade = (menu_state->sub_menu_state == UI_PAUSE_MENU_SUB_MENU_STATE_NONE);

            if (should_blur_fade) {
                game_postprocess_blur(framebuffer, blur_samples, max_blur * (1-menu_state->transition_t), BLEND_MODE_ALPHA);
                game_postprocess_grayscale(framebuffer, max_grayscale * (1-menu_state->transition_t));
            } else {
                game_postprocess_blur(framebuffer, blur_samples, max_blur, BLEND_MODE_ALPHA);
                game_postprocess_grayscale(framebuffer, max_grayscale);
            }


            if (menu_state->transition_t >= 1.0f) {
                menu_state->transition_t = 0;
                switch (menu_state->sub_menu_state) {
                    case UI_PAUSE_MENU_SUB_MENU_STATE_NONE: {
                        game_state_set_ui_state(game_state, state->last_ui_state);
                    } break;
                        /* 
                           I don't really think any other UI state requires other special casing behavior
                           just set to NO ANIM and just don't display the normal menu choices... Basically
                           just hand off control to our friends.
                        */
                    default: {
                        menu_state->animation_state = UI_PAUSE_MENU_NO_ANIM;
                    } break;
                }
            }
        } break;
    }

    {
        for (unsigned index = 0; index < array_count(item_positions); ++index) {
            v2f32 draw_position = item_positions[index];
            draw_position.x += lerp_f32(0, 20, menu_state->shift_t[index]);
            draw_position.y += 255;
            /* custom string drawing routine */
            struct font_cache* font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_STEEL]);
            if (index == menu_state->selection) {
                font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]);
            }

            draw_ui_breathing_text(framebuffer, draw_position, font, font_scale, ui_pause_menu_strings[index], index, color32f32_WHITE);
        }
    }
}

void update_and_render_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    switch (state->ui_state) {
        case UI_STATE_INGAME: {
            update_and_render_ingame_game_menu_ui(state, framebuffer, dt);
        } break;
        case UI_STATE_PAUSE: {
            update_and_render_pause_game_menu_ui(state, framebuffer, dt);
        } break;
            bad_case;
    }
}

local void recalculate_camera_shifting_bounds(struct software_framebuffer* framebuffer) {
    {
        game_state->camera.travel_bounds.x = framebuffer->width  * 0.10;
        game_state->camera.travel_bounds.y = framebuffer->height * 0.10;
        game_state->camera.travel_bounds.w = framebuffer->width  * 0.80;
        game_state->camera.travel_bounds.h = framebuffer->height * 0.80;
    }
}

local void update_game_camera_exploration_mode(struct game_state* state, f32 dt) {
    struct camera* camera = &state->camera;

    struct entity* player = entity_list_dereference_entity(&state->permenant_entities, player_id);
    camera->tracking_xy = player->position;

    {
        /* kind of like a project on everythign */
        v2f32                projected_rectangle_position = camera_project(camera, v2f32(camera->travel_bounds.x, camera->travel_bounds.y),
                                                            640, 480);
        struct rectangle_f32 projected_rectangle          = rectangle_f32(projected_rectangle_position.x, projected_rectangle_position.y,
                                                                 camera->travel_bounds.w * camera->zoom, camera->travel_bounds.h * camera->zoom);
        struct rectangle_f32 player_rectangle             = entity_rectangle_collision_bounds(player);

        if (!camera->try_interpolation[0]) {
            bool pushing_left_edge  = (player_rectangle.x < projected_rectangle.x);
            bool pushing_right_edge = (player_rectangle.x + player_rectangle.w > projected_rectangle.x + projected_rectangle.w);

            if (pushing_right_edge || pushing_left_edge) {
                camera->interpolation_t[0] = 0;
                camera->try_interpolation[0] = true;
                camera->start_interpolation_values[0] = camera->xy.x;
            }
        }

        if (!camera->try_interpolation[1]) {
            bool pushing_top_edge    = (player_rectangle.y < projected_rectangle.y);
            bool pushing_bottom_edge = (player_rectangle.y + player_rectangle.h > projected_rectangle.y + projected_rectangle.h);

            if (pushing_bottom_edge || pushing_top_edge) {
                camera->interpolation_t[1] = 0;
                camera->try_interpolation[1] = true;
                camera->start_interpolation_values[1] = camera->xy.y;
            }
        }
    }
}

local void update_game_camera(struct game_state* state, f32 dt) {
    struct camera* camera = &state->camera;

    struct entity* player = entity_list_dereference_entity(&state->permenant_entities, player_id);
    camera->centered    = true;
    camera->zoom        = 1;

    /* TODO hard coded sprite sizes */
    {
        static bool camera_init = false;
        if (!camera_init) {
            camera->tracking_xy = player->position;
            camera->xy.x        = camera->tracking_xy.x + 16;
            camera->xy.y        = camera->tracking_xy.y + 32;
            camera_init         = true;
        }
    }

    if (game_state->combat_state.active_combat) {
        update_game_camera_combat(state, dt);
    } else {
        update_game_camera_exploration_mode(state, dt);
    }


    const f32 lerp_x_speed = 3;
    const f32 lerp_y_speed = 3;

    const f32 lerp_component_speeds[3] = {
        2.45, 2.8, 1.5
    };

    for (unsigned component_index = 0; component_index < 3; ++component_index) {
        if (camera->try_interpolation[component_index]) {
            if (camera->interpolation_t[component_index] < 1.0) {
                camera->components[component_index] = lerp_f32(camera->start_interpolation_values[component_index],
                                                               camera->tracking_components[component_index],
                                                               camera->interpolation_t[component_index]);
                camera->interpolation_t[component_index] += dt * lerp_component_speeds[component_index];
            } else {
                camera->try_interpolation[component_index] = false;
            }
        }
    }
}

void handle_entity_level_trigger_interactions(struct game_state* state, struct entity* entity, s32 trigger_level_transition_count, struct trigger_level_transition* trigger_level_transitions, f32 dt) {
    if (!(entity->flags & ENTITY_FLAGS_PLAYER_CONTROLLED))
        return;

    struct rectangle_f32 entity_collision_bounds = rectangle_f32_scale(entity_rectangle_collision_bounds(entity), 1.0/TILE_UNIT_SIZE);
    for (s32 index = 0; index < trigger_level_transition_count; ++index) {
        struct trigger_level_transition* current_trigger = trigger_level_transitions + index;
        v2f32 spawn_location       = current_trigger->spawn_location;
        u8    new_facing_direction = current_trigger->new_facing_direction;

        if (rectangle_f32_intersect(current_trigger->bounds, entity_collision_bounds)) {
            /* queue a level transition, animation (god animation sucks... For now instant transition) */
            struct binary_serializer serializer = open_read_file_serializer(string_concatenate(&scratch_arena, string_literal("areas/"), string_from_cstring(current_trigger->target_level)));
            serialize_level_area(state, &serializer, &state->loaded_area, false);
            /* NOTE entity positions are in pixels still.... */
            entity->position.x = spawn_location.x * TILE_UNIT_SIZE;
            entity->position.y = spawn_location.y * TILE_UNIT_SIZE;

            return;
        }
    }
}

void handle_entity_scriptable_trigger_interactions(struct game_state* state, struct entity* entity, s32 trigger_count, struct trigger* triggers, f32 dt) {
    if (!(entity->flags & ENTITY_FLAGS_PLAYER_CONTROLLED))
        return;

    struct rectangle_f32 entity_collision_bounds = rectangle_f32_scale(entity_rectangle_collision_bounds(entity), 1.0/TILE_UNIT_SIZE);

    bool any_intersections = false;
    for (s32 index = 0; index < trigger_count; ++index) {
        struct trigger* current_trigger = triggers + index;

        switch (current_trigger->activation_method) {
            case ACTIVATION_TYPE_TOUCH: {
                if (rectangle_f32_intersect(current_trigger->bounds, entity_collision_bounds)) {
                    any_intersections = true;
                    bool already_activated_trigger = false;

                    for (s32 activated_index = 0; activated_index < array_count(entity->interacted_script_trigger_ids) && !already_activated_trigger; ++activated_index) {
                        if (entity->interacted_script_trigger_ids[activated_index] == (index+1)) {
                            already_activated_trigger = true;
                        }
                    }

                    if (!already_activated_trigger) {
                        if (entity->interacted_script_trigger_write_index < array_count(entity->interacted_script_trigger_ids)) {
                            entity->interacted_script_trigger_ids[entity->interacted_script_trigger_write_index++] = index+1;
                            current_trigger->activations += 1;
                            struct level_area* area = &state->loaded_area;

                            struct lisp_form listener_body = level_area_find_listener_for_object(state, area, LEVEL_AREA_LISTEN_EVENT_ON_TOUCH, GAME_SCRIPT_TARGET_TRIGGER, index);
                            game_script_enqueue_form_to_execute(listener_body);
                        }
                    }
                }
            } break;
        }
    }

    if (!any_intersections) {
        entity->interacted_script_trigger_write_index = 0;
        zero_array(entity->interacted_script_trigger_ids);
    }
}

local void unmark_any_interactables(struct game_state* state) {
    state->interactable_state.interactable_type = 0;
    state->interactable_state.context           = NULL;
}

local void mark_interactable(struct game_state* state, s32 type, void* ptr) {
    if (state->interactable_state.interactable_type == type &&
        state->interactable_state.context == ptr) {
        _debugprintf("same interactable!");
    } else {
        state->interactable_state.interactable_type = type;
        state->interactable_state.context           = ptr;
    }
}

void player_handle_radial_interactables(struct game_state* state, struct entity* entity, f32 dt) {
    bool found_any_interactable = false;
    struct level_area* area   = &state->loaded_area;

    if (!found_any_interactable) {
        Array_For_Each(it, struct entity_chest, area->chests, area->entity_chest_count) {
            f32 distance_sq = v2f32_distance_sq(entity->position, v2f32_scale(it->position, TILE_UNIT_SIZE));
            if (it->flags & ENTITY_CHEST_FLAGS_UNLOCKED) continue;

            if (distance_sq <= (ENTITY_CHEST_INTERACTIVE_RADIUS*ENTITY_CHEST_INTERACTIVE_RADIUS)) {
                mark_interactable(state, INTERACTABLE_TYPE_CHEST, it);
                found_any_interactable = true;
                break;
            }
        }
    }

    if (!found_any_interactable) unmark_any_interactables(state);
}

#include "combat.c"
#include "game_main_menu.c"

local void execute_current_area_scripts(struct game_state* state, f32 dt) {
    /* _debugprintf("START EXECUTING AREA SCRIPTS"); */
    struct level_area_script_data* script_data = &state->loaded_area.script;
    if (script_data->present) {
        struct lisp_form* on_frame_script    = script_data->on_frame;

        if (!state->level_area_on_enter_triggered) {
            state->level_area_on_enter_triggered = true;
            struct lisp_form* on_enter_script    = script_data->on_enter;

            if (on_enter_script) {
                game_script_enqueue_form_to_execute(lisp_list_sliced(*on_enter_script, 1, -1));
            }
        } else {
            /* 
               Wait no onframe is a waste of time. 
               
               Events are best. For this anyways. I don't know what I was thinking
               when I did this.
            */
            if (on_frame_script) {
                /* for (s32 index = 1; index < on_frame_script->list.count; ++index) { */
                /*     struct lisp_form* current_form = lisp_list_nth(on_frame_script, index); */
                /*     game_script_evaluate_form(&scratch_arena, state, current_form); */
                /* } */
            }
        }


        /* onframe is going to hurt but I'm not kidding about this. */
        /* I could invoke multiple every-n-seconds blocks? */
        /* _debugprintf("END EXECUTING AREA SCRIPTS"); */
    }
}

void update_and_render_game(struct software_framebuffer* framebuffer, f32 dt) {
    if (is_key_pressed(KEY_F12)) {
        image_buffer_write_to_disk((struct image_buffer*)framebuffer, string_literal("scr"));
    }

#ifdef USE_EDITOR
    if (is_key_pressed(KEY_F1)) {
        game_state->in_editor ^= 1;
    }
#endif

    recalculate_camera_shifting_bounds(framebuffer);

#ifdef USE_EDITOR
    if (game_state->in_editor) {
        update_and_render_editor(framebuffer, dt);
        update_and_render_editor_menu_ui(game_state, framebuffer, dt);
        return;
    }
#endif
    
    {
        switch (screen_mode) {
            case GAME_SCREEN_INGAME: {
                struct entity* player_entity = game_get_player(game_state);
                update_game_camera(game_state, dt);

                struct render_commands commands = render_commands(game_state->camera);

                commands.should_clear_buffer = true;
                commands.clear_buffer_color  = color32u8(0, 0, 0, 255);

                execute_current_area_scripts(game_state, dt);

                /* update all tile animations */
                {
                    for (s32 index = 0; index < tile_table_data_count; ++index) {
                        struct tile_data_definition* tile_definition = tile_table_data + index;

                        tile_definition->timer += dt;
                        if (tile_definition->timer > tile_definition->time_until_next_frame) {
                            tile_definition->frame_index += 1;

                            if (tile_definition->frame_index >= tile_definition->frame_count) {
                                tile_definition->frame_index = 0;
                            }

                            tile_definition->timer = 0;
                        }
                    }
                }

                render_ground_area(game_state, &commands, &game_state->loaded_area);

                if (game_state->ui_state != UI_STATE_PAUSE) {
                    if (!storyboard_active) {
                        update_entities(game_state, dt, &game_state->loaded_area);

                        if (!game_state->combat_state.active_combat) {
                            determine_if_combat_should_begin(game_state, &game_state->permenant_entities);
                        } else {
                            update_combat(game_state, dt);
                        }
                    }

                    game_state->weather.timer += dt;
                }

                render_entities(game_state, &graphics_assets, &commands, dt);
                render_foreground_area(game_state, &commands, &game_state->loaded_area);

                game_script_execute_awaiting_scripts(&scratch_arena, game_state, dt);
                game_script_run_all_timers(dt);

                render_commands_push_quad(&commands, rectangle_f32(commands.camera.xy.x-500,commands.camera.xy.y-500,9999,9999), global_color_grading_filter, BLEND_MODE_MULTIPLICATIVE);
                software_framebuffer_render_commands(framebuffer, &commands);
                game_postprocess_blur_ingame(framebuffer, 2, 0.62, BLEND_MODE_ALPHA);

                /* color "grading" */
                do_weather(framebuffer, game_state, dt);
                update_and_render_game_menu_ui(game_state, framebuffer, dt);
            } break;
            case GAME_SCREEN_MAIN_MENU: {
                update_and_render_main_menu(game_state, framebuffer, dt);
            } break;
        }
    }

#if 0
    /* camera debug */
    {
        software_framebuffer_draw_quad(framebuffer, game_state->camera.travel_bounds, color32u8(0,0,255,100), BLEND_MODE_ALPHA);
    }
#endif
}

#include "game_script.c"
#include "save_data.c"
#include "storyboard_presentation.c"
#include "entity_model.c"
