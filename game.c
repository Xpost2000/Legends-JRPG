/* virtual pixels */
#define TILE_UNIT_SIZE (32) /* measured with a reference of 640x480 */
#define MAX_TILE_LAYERS (4) 

#include "game_def.c"
#include "save_data_def.c"
#include "dialogue_script_parse.c"
#include "game_script_def.c"

static struct memory_arena game_arena   = {};
/* compile out */
static struct memory_arena editor_arena = {};

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
};
font_id menu_fonts[9];

image_id guy_img;
image_id selection_sword_img;
image_id chest_closed_img, chest_open_bottom_img, chest_open_top_img;

/* TODO, use this */
struct game_ui_nine_patch game_ui_nine_patch_load_from_directory(struct graphics_assets* graphics_assets, string directory, s32 tile_width, s32 tile_height) {
    struct game_ui_nine_patch ui_skin = {}; 

    {
        s32 index = 0;

        Array_For_Each(it, string, game_ui_nine_patch_id_strings, array_count(game_ui_nine_patch_id_strings)) {
            index++;
            string filepath         = string_from_cstring(format_temp("%.*s/%.*s.png", directory.length, directory.data, it->length, it->data));
            ui_skin.textures[index] = graphics_assets_load_image(graphics_assets, filepath);
        }

        ui_skin.tile_width  = tile_width;
        ui_skin.tile_height = tile_height;
    }

    return ui_skin;
}

struct tile_data_definition* tile_table_data;
struct autotile_table*       auto_tile_info;

#include "region_change_presentation.c"
local void game_attempt_to_change_area_name(struct game_state* state, string name, string subtitle) {
    /* we don't need to check the subtitle. That doesn't matter. I would think. */
    if (!cstring_equal(name.data, state->current_region_name)) {
        cstring_copy(name.data,     state->current_region_name    , array_count(state->current_region_name));
        cstring_copy(subtitle.data, state->current_region_subtitle, array_count(state->current_region_subtitle));
        initialize_region_zone_change();
    }
}

void game_finish_conversation(struct game_state* state) {
    state->is_conversation_active = false;
    file_buffer_free(&state->conversation_file_buffer);
}

void render_area(struct render_commands* commands, struct level_area* area) {
    /* TODO do it lazy mode. Once only */

    for (s32 index = 0; index < area->tile_count; ++index) {
        s32 tile_id = area->tiles[index].id;
        struct tile_data_definition* tile_data = tile_table_data + tile_id;

        image_id tex = graphics_assets_get_image_by_filepath(&graphics_assets, tile_data->image_asset_location); 

        render_commands_push_image(commands,
                                   graphics_assets_get_image_by_id(&graphics_assets, tex),
                                   rectangle_f32(area->tiles[index].x * TILE_UNIT_SIZE,
                                                 area->tiles[index].y * TILE_UNIT_SIZE,
                                                 TILE_UNIT_SIZE,
                                                 TILE_UNIT_SIZE),
                                   tile_data->sub_rectangle,
                                   color32f32(1,1,1,1), NO_FLAGS, BLEND_MODE_ALPHA);

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
entity_id entity_list_create_player(struct entity_list* entities, v2f32 position) {
    entity_id result = entity_list_create_entity(entities);
    struct entity* player = entity_list_dereference_entity(entities, result);

    assertion(player->flags & ENTITY_FLAGS_ACTIVE);
    player->flags    |= ENTITY_FLAGS_ALIVE;
    player->flags    |= ENTITY_FLAGS_PLAYER_CONTROLLED;
    player->health.value = 100;
    player->health.min = 100;
    player->health.max = 100;
    player->position  = position;
    player->scale.x = TILE_UNIT_SIZE-2;
    player->scale.y = TILE_UNIT_SIZE-2;

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

    return result;
}


#include "weather.c"

/* does not account for layers right now. That's okay. */
struct tile* level_area_find_tile(struct level_area* level, s32 x, s32 y) {
    Array_For_Each(it, struct tile, level->tiles, level->tile_count) {
        if ((s32)(it->x)== x && (s32)(it->y) == y) {
            return it;
        }
    }

    return NULL;
}

local void build_navigation_map_for_level_area(struct memory_arena* arena, struct level_area* level) {
    if (level->tile_count > 0) {
        struct level_area_navigation_map* navigation_map = &level->navigation_data;

        s32 min_x = level->tiles[0].x;
        s32 min_y = level->tiles[0].y;
        s32 max_x = level->tiles[0].x;
        s32 max_y = level->tiles[0].y;

        Array_For_Each(it, struct tile, level->tiles, level->tile_count) {
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

        navigation_map->tiles = memory_arena_push_top(arena, sizeof(*navigation_map->tiles) * navigation_map->width * navigation_map->height);

        for (s32 y_cursor = navigation_map->min_y; y_cursor < navigation_map->max_y; ++y_cursor) {
            for (s32 x_cursor = navigation_map->min_x; x_cursor < navigation_map->max_x; ++x_cursor) {
                struct level_area_navigation_map_tile* nav_tile = &navigation_map->tiles[((y_cursor - navigation_map->min_y) * navigation_map->width + (x_cursor - navigation_map->min_x))];
                struct tile*                          real_tile = level_area_find_tile(level, x_cursor, y_cursor);

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
    if (level_area_navigation_map_is_point_in_bounds(navigation_map, start) &&
        level_area_navigation_map_is_point_in_bounds(navigation_map, end)) {
        s32                               map_width               = navigation_map->width;
        s32                               map_height              = navigation_map->height;
        s32                               total_elements          = (map_width * map_height);

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
            _debugprintf("try to find neighbors");
            for (s32 y_cursor = -1; y_cursor <= 1; ++y_cursor) {
                for (s32 x_cursor = -1; x_cursor <= 1; ++x_cursor) {
                    v2f32 proposed_point  = current_point;
                    proposed_point.x     += x_cursor;
                    proposed_point.y     += y_cursor;

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
                            /* _debugprintf("refused, solid!") */
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

    return results;
}

void serialize_level_area(struct game_state* state, struct binary_serializer* serializer, struct level_area* level, bool use_default_spawn) {
    _debugprintf("%d memory used", state->arena->used + state->arena->used_top);
    memory_arena_clear_top(state->arena);
    _debugprintf("reading version");
    serialize_u32(serializer, &level->version);
    _debugprintf("reading default player spawn");
    serialize_f32(serializer, &level->default_player_spawn.x);
    serialize_f32(serializer, &level->default_player_spawn.y);
    _debugprintf("reading tiles");
    /* just going to cheat and used fixed size allocations... */
    Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, state->arena, s32, level->tile_count, level->tiles);

    if (level->version >= 1) {
        _debugprintf("reading level transitions");
        Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, state->arena, s32, level->trigger_level_transition_count, level->trigger_level_transitions);
        /* this thing is allergic to chest updates. Unfortunately it might happen a lot... */
        if (level->version >= 2) {
            _debugprintf("reading containers");
            Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, state->arena, s32, level->entity_chest_count, level->chests);
        }
    }

    /* until we have new area transititons or whatever. */
    /* TODO dumb to assume only the player... but okay */
    struct entity* player = entity_list_dereference_entity(&state->entities, player_id);
    if (use_default_spawn) {
        player->position.x             = level->default_player_spawn.x;
        player->position.y             = level->default_player_spawn.y;
    }

    state->camera.xy.x = player->position.x;
    state->camera.xy.y = player->position.y;
    qsort(level->tiles, level->tile_count, sizeof(*level->tiles), _qsort_tile);

    build_navigation_map_for_level_area(state->arena, level);

    _debugprintf("%d memory used", state->arena->used + state->arena->used_top);
}

/* this is used for cheating or to setup the game I suppose. */
void load_level_from_file(struct game_state* state, string filename) {
    struct binary_serializer serializer = open_read_file_serializer(string_concatenate(state->arena, string_literal("areas/"), filename));
    serialize_level_area(state, &serializer, &state->loaded_area, true);
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
    f32 box_blur[] = {
        1,1.5,1,
        1.5,1,1.5,
        1,1.5,1,
    };

    struct software_framebuffer blur_buffer = software_framebuffer_create(&scratch_arena, framebuffer->width/quality_scale, framebuffer->height/quality_scale);
    software_framebuffer_copy_into(&blur_buffer, framebuffer);
    software_framebuffer_kernel_convolution_ex(&scratch_arena, &blur_buffer, box_blur, 3, 3, 10, t, 1);
    software_framebuffer_draw_image_ex(framebuffer, (struct image_buffer*)&blur_buffer, RECTANGLE_F32_NULL, RECTANGLE_F32_NULL, color32f32(1,1,1,1), NO_FLAGS, blend_mode);
}

void game_postprocess_grayscale(struct software_framebuffer* framebuffer, f32 t) {
    for (s32 y_cursor = 0; y_cursor < framebuffer->height; ++y_cursor) {
        for (s32 x_cursor = 0; x_cursor < framebuffer->width; ++x_cursor) {
            u8 r = framebuffer->pixels[y_cursor * framebuffer->width * 4 + x_cursor * 4 + 0];
            u8 g = framebuffer->pixels[y_cursor * framebuffer->width * 4 + x_cursor * 4 + 1];
            u8 b = framebuffer->pixels[y_cursor * framebuffer->width * 4 + x_cursor * 4 + 2];
            f32 average = (r + g + b) / 3.0f;
            framebuffer->pixels[y_cursor * framebuffer->width * 4 + x_cursor * 4 + 0] = framebuffer->pixels[y_cursor * framebuffer->width * 4 + x_cursor * 4 + 0] * (1 - t) + average * t;
            framebuffer->pixels[y_cursor * framebuffer->width * 4 + x_cursor * 4 + 1] = framebuffer->pixels[y_cursor * framebuffer->width * 4 + x_cursor * 4 + 1] * (1 - t) + average * t;
            framebuffer->pixels[y_cursor * framebuffer->width * 4 + x_cursor * 4 + 2] = framebuffer->pixels[y_cursor * framebuffer->width * 4 + x_cursor * 4 + 2] * (1 - t) + average * t;
        }
    }
}


struct game_state*   game_state = 0;
struct editor_state* editor_state = 0;
void editor_initialize(struct editor_state* state);
#include "editor.c"

static void initialize_static_table_data(void) {
    /* a very generous amount of table data... */
    tile_table_data = memory_arena_push(&game_arena, sizeof(*tile_table_data) * 2048);
    auto_tile_info  = memory_arena_push(&game_arena, sizeof(*auto_tile_info)  * 1024);

    s32 i = 0;
    s32 j = 0;

    /* NOTE this table may be subject to change, and a lot of things may explode? */
#define insert(x)    tile_table_data[i++] = (x)
#define AT_insert(x) auto_tile_info[j++] = (x)
#define current_AT   &auto_tile_info[j] 
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(grass test(filled))"),
            .image_asset_location = string_literal("./res/img/land/grass.png"),
            .flags                = TILE_DATA_FLAGS_NONE,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(brick test(filled))"),
            .image_asset_location = string_literal("./res/img/brick.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(dirt)"),
            .image_asset_location = string_literal("./res/img/land/dirt.png"),
            .flags                = TILE_DATA_FLAGS_NONE,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(water (unpassable))"),
            .image_asset_location = string_literal("./res/img/land/water.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(water (passable))"),
            .image_asset_location = string_literal("./res/img/land/water.png"),
            .flags                = TILE_DATA_FLAGS_NONE,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(cave wall)"),
            .image_asset_location = string_literal("./res/img/cave/cavewall.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(cave wall opening)"),
            .image_asset_location = string_literal("./res/img/cave/cavewall_opening.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(cave wall mossy"),
            .image_asset_location = string_literal("./res/img/cave/cavewall1.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(cobble floor1)"),
            .image_asset_location = string_literal("./res/img/cave/cobble_floor1.png"),
            .flags                = TILE_DATA_FLAGS_NONE,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(bush)"),
            .image_asset_location = string_literal("./res/img/land/bush.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(house block)"),
            .image_asset_location = string_literal("./res/img/building/home_block.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(house block top)"),
            .image_asset_location = string_literal("./res/img/building/home_top_block.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(house block window)"),
            .image_asset_location = string_literal("./res/img/building/home_block_window.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(carpet red fuzz)"),
            .image_asset_location = string_literal("./res/img/building/home_red_fuzz_pit.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(home wood floor)"),
            .image_asset_location = string_literal("./res/img/building/home_wood_floor.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(home wood wall)"),
            .image_asset_location = string_literal("./res/img/building/home_wood_wall.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(home wood wall top)"),
            .image_asset_location = string_literal("./res/img/building/home_wood_wall_top.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(door)"),
            .image_asset_location = string_literal("./res/img/building/door.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(wood_log_side)"),
            .image_asset_location = string_literal("./res/img/cave/wood_log_side.png"),
            .flags                = TILE_DATA_FLAGS_NONE,
        })
    );
#undef insert 
#undef AT_insert 
#undef current_AT 
    tile_table_data_count = i;
}

void game_initialize(void) {
    game_arena   = memory_arena_create_from_heap("Game Memory", Megabyte(32));
    editor_arena = memory_arena_create_from_heap("Editor Memory", Megabyte(32));

    game_state                     = memory_arena_push(&game_arena, sizeof(*game_state));
    game_state->conversation_arena = memory_arena_push_sub_arena(&game_arena, Kilobyte(64));
    
    initialize_save_data();

    game_state->rng = random_state();
    game_state->arena = &game_arena;

    graphics_assets = graphics_assets_create(&game_arena, 64, 1024);
    scratch_arena = memory_arena_create_from_heap("Scratch Buffer", Megabyte(4));

    guy_img               = graphics_assets_load_image(&graphics_assets, string_literal("./res/img/guy3.png"));
    chest_closed_img      = graphics_assets_load_image(&graphics_assets, string_literal("./res/img/chestclosed.png"));
    chest_open_bottom_img = graphics_assets_load_image(&graphics_assets, string_literal("./res/img/chestopenbottom.png"));
    chest_open_top_img    = graphics_assets_load_image(&graphics_assets, string_literal("./res/img/chestopentop.png"));
    /* selection_sword_img   = graphics_assets_load_image(&graphics_assets, string_literal("./res/img/selection_sword.png")); */

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
    game_state->entities = entity_list_create(&game_arena, 512);
    player_id            = entity_list_create_player(&game_state->entities, v2f32(70, 70));
    entity_list_create_badguy(&game_state->entities, v2f32(8 * TILE_UNIT_SIZE, 8 * TILE_UNIT_SIZE));
    entity_list_create_badguy(&game_state->entities, v2f32(11 * TILE_UNIT_SIZE, 8 * TILE_UNIT_SIZE));

    editor_state                = memory_arena_push(&editor_arena, sizeof(*editor_state));
    editor_initialize(editor_state);
    initialize_static_table_data();
    initialize_items_database();

    for (unsigned index = 0; index < 2048; ++index) {
        struct tile_data_definition* item = tile_table_data + index;
        if (item->image_asset_location.length && item->image_asset_location.data) {
            graphics_assets_load_image(&graphics_assets, item->image_asset_location);
        }
    }

    assertion(verify_no_item_id_name_hash_collisions());
    void game_initialize_game_world(void);
    game_initialize_game_world();
}

void game_initialize_game_world(void) {
    entity_inventory_add((struct entity_inventory*)&game_state->inventory, MAX_PARTY_ITEMS, item_id_make(string_literal("item_trout_fish_5")));
    entity_inventory_add((struct entity_inventory*)&game_state->inventory, MAX_PARTY_ITEMS, item_id_make(string_literal("item_trout_fish_5")));
    entity_inventory_add((struct entity_inventory*)&game_state->inventory, MAX_PARTY_ITEMS, item_id_make(string_literal("item_trout_fish_5")));
    entity_inventory_add((struct entity_inventory*)&game_state->inventory, MAX_PARTY_ITEMS, item_id_make(string_literal("item_sardine_fish_5")));

#if 0
    /* game_open_conversation_file(game_state, string_literal("./dlg/linear_test.txt")); */
    game_open_conversation_file(game_state, string_literal("./dlg/simple_choices.txt"));
    load_level_from_file(game_state, string_literal("pf.area"));
#endif
    load_level_from_file(game_state, string_literal("bt.area"));
    /* load_level_from_file(game_state, string_literal("testisland.area")); */
    /* game_attempt_to_change_area_name(game_state, string_literal("Old Iyeila"), string_literal("Grave of Stars")); */
#if 0
    int _game_sandbox_testing(void);
    _game_sandbox_testing();
#endif
}

void game_deinitialize(void) {
    game_finish_conversation(game_state);
    finish_save_data();
    memory_arena_finish(&scratch_arena);
    memory_arena_finish(&editor_arena);
    graphics_assets_finish(&graphics_assets);
    memory_arena_finish(&game_arena);
}

local void game_loot_chest(struct game_state* state, struct entity_chest* chest) {
    struct entity* player = entity_list_dereference_entity(&state->entities, player_id);
    
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

void game_message_queue(string message) {
    assertion(global_popup_state.message_count < array_count(global_popup_state.messages));
    struct ui_popup_message_box* current_message = &global_popup_state.messages[global_popup_state.message_count++];
    /* NOTE, in this case this works since I just casted a message from a cstring. This is not really secure */
    cstring_copy(message.data, current_message->message_storage, array_count(current_message->message_storage));
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
        case UI_PAUSE_MENU_SUB_MENU_STATE_INVENTORY: {
            /* for now this inventory is just going to be flat. Simple display with item selection. */
            /* TODO no item usage yet. */
            /* TODO need more item types for usage */
            software_framebuffer_draw_quad(framebuffer, rectangle_f32(100, 100, 400, 300), color32u8(0,0,255, 190), BLEND_MODE_ALPHA);


            struct entity_inventory* inventory = (struct entity_inventory*)&state->inventory;

            f32 y_cursor = 110;
            f32 item_font_scale = 2;
            struct font_cache* font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_YELLOW]);
            struct font_cache* font2 = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]);

            wrap_around_key_selection(KEY_UP, KEY_DOWN, &state->ui_inventory.selection_item_list, 0, inventory->count);

            if (is_key_pressed(KEY_ESCAPE)) {
                menu_state->animation_state     = UI_PAUSE_MENU_TRANSITION_IN;
                menu_state->last_sub_menu_state = menu_state->sub_menu_state;
                menu_state->sub_menu_state      = UI_PAUSE_MENU_SUB_MENU_STATE_NONE;
                menu_state->transition_t = 0;
            }

            if (is_key_pressed(KEY_RETURN)) {
                s32 index = state->ui_inventory.selection_item_list;
                struct item_def* item = item_database_find_by_id(inventory->items[index].item);

                if (item->type != ITEM_TYPE_MISC) {
                    _debugprintf("use item \"%.*s\"", item->name.length, item->name.data);

                    struct entity* player = entity_list_dereference_entity(&state->entities, player_id);
                    entity_inventory_use_item(inventory, index, player);
                }
            }

            for (unsigned index = 0; index < inventory->count; ++index) {
                struct item_def* item = item_database_find_by_id(inventory->items[index].item);
                if (index == state->ui_inventory.selection_item_list) {
                    software_framebuffer_draw_text(framebuffer, font2, item_font_scale, v2f32(110, y_cursor), item->name, color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                } else {
                    software_framebuffer_draw_text(framebuffer, font, item_font_scale, v2f32(110, y_cursor), item->name, color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                }

                char tmp[255] = {};
                snprintf(tmp,255,"x%d/%d", inventory->items[index].count, item->max_stack_value);
                software_framebuffer_draw_text(framebuffer, font2, item_font_scale, v2f32(350, y_cursor), string_from_cstring(tmp), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                y_cursor += item_font_scale * 16;
            }

            /* _debugprintf("inventory not done!"); */
            /* menu_state->animation_state     = UI_PAUSE_MENU_TRANSITION_IN; */
            /* menu_state->last_sub_menu_state = menu_state->sub_menu_state; */
            /* menu_state->sub_menu_state      = UI_PAUSE_MENU_SUB_MENU_STATE_NONE; */
            /* menu_state->transition_t = 0; */
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
                        } break;
                        case 2: {
                            {
                                menu_state->animation_state = UI_PAUSE_MENU_TRANSITION_CLOSING;
                                menu_state->last_sub_menu_state  = UI_PAUSE_MENU_SUB_MENU_STATE_NONE;
                                menu_state->sub_menu_state  = UI_PAUSE_MENU_SUB_MENU_STATE_INVENTORY;
                                menu_state->transition_t = 0;
                            }
                        } break;
                        case 3: {
                            {
                                menu_state->animation_state = UI_PAUSE_MENU_TRANSITION_CLOSING;
                                menu_state->last_sub_menu_state  = UI_PAUSE_MENU_SUB_MENU_STATE_NONE;
                                menu_state->sub_menu_state  = UI_PAUSE_MENU_SUB_MENU_STATE_INVENTORY;
                                menu_state->transition_t = 0;
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

            for (unsigned character_index = 0; character_index < ui_pause_menu_strings[index].length; ++character_index) {
                f32 character_displacement_y = sinf((global_elapsed_time*2) + ((character_index+index) * 2381.2318)) * 3;

                v2f32 glyph_position = draw_position;
                glyph_position.y += character_displacement_y;
                glyph_position.x += font->tile_width * font_scale * character_index;

                software_framebuffer_draw_text(framebuffer, font, font_scale, glyph_position, string_slice(ui_pause_menu_strings[index], character_index, character_index+1), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            }
        }
    }
}

void update_and_render_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    if (is_key_pressed(KEY_F1)) {
        state->in_editor ^= 1;
    }

    switch (state->ui_state) {
        case UI_STATE_INGAME: {
            if (state->in_editor) {
                update_and_render_editor_game_menu_ui(state, framebuffer, dt);
            } else {
                update_and_render_ingame_game_menu_ui(state, framebuffer, dt);
                update_and_render_region_zone_change(state, framebuffer, dt);
            }
        } break;
        case UI_STATE_PAUSE: {
            if (state->in_editor) {
                update_and_render_pause_editor_menu_ui(state, framebuffer, dt);
            } else {
                update_and_render_pause_game_menu_ui(state, framebuffer, dt);
            }
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

local void update_game_camera(struct game_state* state, f32 dt) {
    struct camera* camera = &state->camera;
    struct entity* player = entity_list_dereference_entity(&state->entities, player_id);
    camera->centered    = true;
    camera->zoom        = 1;
    camera->tracking_xy = player->position;

    /* TODO hard coded sprite sizes */
    {
        static bool camera_init = false;
        if (!camera_init) {
            camera->xy.x = camera->tracking_xy.x + 16;
            camera->xy.y = camera->tracking_xy.y + 32;
            camera_init  = true;
        }
    }

    const f32 lerp_x_speed = 3;
    const f32 lerp_y_speed = 3;

    const f32 lerp_component_speeds[3] = {
        2.45, 2.8, 1.5
    };

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

        for (unsigned component_index = 0; component_index < 3; ++component_index) {
            if (camera->try_interpolation[component_index]) {
                if (camera->interpolation_t[component_index] < 1.0) {
                    camera->components[component_index] = quadratic_ease_in_f32(camera->start_interpolation_values[component_index],
                                                                                camera->tracking_components[component_index],
                                                                                camera->interpolation_t[component_index]);
                    camera->interpolation_t[component_index] += dt * lerp_component_speeds[component_index];
                } else {
                    camera->try_interpolation[component_index] = false;
                }
            }
        }
    }
}

void handle_entity_level_trigger_interactions(struct game_state* state, struct entity* entity, s32 trigger_level_transition_count, struct trigger_level_transition* trigger_level_transitions, f32 dt) {
    if (!(entity->flags & ENTITY_FLAGS_PLAYER_CONTROLLED))
        return;

    for (s32 index = 0; index < trigger_level_transition_count; ++index) {
        struct trigger_level_transition* current_trigger = trigger_level_transitions + index;
        v2f32 spawn_location       = current_trigger->spawn_location;
        u8    new_facing_direction = current_trigger->new_facing_direction;

        struct rectangle_f32 entity_collision_bounds = rectangle_f32_scale(entity_rectangle_collision_bounds(entity), 1.0/TILE_UNIT_SIZE);
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

void player_handle_radial_interactables(struct game_state* state, struct entity_list* entities, s32 entity_index, f32 dt ) {
    bool found_any_interactable = false;
    struct level_area* area   = &state->loaded_area;
    struct entity*     entity = entities->entities + entity_index;

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

/* check for any nearby conflicts for any reason */
/* for now just check if there are any enemies in the play area. */
/* no need for anything fancy right now. That comes later. */
/* (hard coding data is a real pain in my ass, so until I can specify NPCs through data, I just
   want the quickest way of doing stuff) */
local void determine_if_combat_should_begin(struct game_state* state, struct entity_list* entities) {
    struct entity* player = entity_list_dereference_entity(entities, player_id);

    bool should_be_in_combat = false;

    for (u32 index = 0; index < entities->count && !should_be_in_combat; ++index) {
        struct entity* current_entity = entities->entities + index;

        if (current_entity != player) {
            struct entity_ai_data* ai = &current_entity->ai;

            if (ai->flags & ENTITY_AI_FLAGS_AGGRESSIVE_TO_PLAYER) {
                should_be_in_combat = true;
            }
        }
    }

    if (should_be_in_combat) {
        state->combat_state.active_combat = true;
    }
}

void update_and_render_game(struct software_framebuffer* framebuffer, f32 dt) {
    if (is_key_pressed(KEY_F12)) {
        image_buffer_write_to_disk((struct image_buffer*)framebuffer, string_literal("scr"));
    }

    recalculate_camera_shifting_bounds(framebuffer);

    if (game_state->in_editor) {
        update_and_render_editor(framebuffer, dt);
    } else {
        struct entity* player_entity = entity_list_dereference_entity(&game_state->entities, player_id);
        update_game_camera(game_state, dt);

        struct render_commands commands = render_commands(game_state->camera);

        commands.should_clear_buffer = true;
        commands.clear_buffer_color  = color32u8(0, 0, 0, 255);

        render_area(&commands, &game_state->loaded_area);
        if (game_state->ui_state != UI_STATE_PAUSE) {
            entity_list_update_entities(game_state,&game_state->entities, dt, &game_state->loaded_area);

            if (!state->combat_state.active_combat) {
                determine_if_combat_should_begin(game_state, &game_state->entities);
            } else {
                update_combat(game_state, dt);
            }

            game_state->weather.timer += dt;
        }

        entity_list_render_entities(&game_state->entities, &graphics_assets, &commands, dt);
        software_framebuffer_render_commands(framebuffer, &commands);
        game_postprocess_blur_ingame(framebuffer, 2, 0.34, BLEND_MODE_ALPHA);

        /* color "grading" */
        software_framebuffer_draw_quad(framebuffer, rectangle_f32(0,0,999,999), color32u8(178,180,255,255), BLEND_MODE_MULTIPLICATIVE);

        /* Rendering weather atop */
        /* NOTE
           While I would like more advanced weather effects, let's not bite off more than
           I can chew so I can actually have something to play...
           
        */
#if 0
        if (is_key_pressed(KEY_E)) {
            if (weather_any_active(game_state)) {
                weather_clear(game_state);
            } else {
                weather_start_rain(game_state);
            }
        }
        if (is_key_pressed(KEY_R)) {
            if (weather_any_active(game_state)) {
                weather_clear(game_state);
            } else {
                weather_start_snow(game_state);
            }
        }
#endif
    }

    do_weather(framebuffer, game_state, dt);
#if 0
    /* camera debug */
    {
        software_framebuffer_draw_quad(framebuffer, game_state->camera.travel_bounds, color32u8(0,0,255,100), BLEND_MODE_ALPHA);
    }
#endif
    update_and_render_game_menu_ui(game_state, framebuffer, dt);
}

#include "game_script.c"
#include "save_data.c"
