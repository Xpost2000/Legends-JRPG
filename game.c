/* #define RELEASE */
/* Needs lots of clean up. (Man I keep saying this every time I come back here, but it doesn't seem to matter too much.) */
/* TODO fix coordinate system <3 */
/* virtual pixels */
#define TILE_UNIT_SIZE                      (32) /* measured with a reference of 640x480 */
#define REFERENCE_TILE_UNIT_SIZE            (16) /* What most tiles should be */
#define GAME_COMMAND_CONSOLE_LINE_INPUT_MAX (512)
#include "rich_text_def.c"
#include "cutscene_def.c"
#include "game_def.c"
#include "save_data_def.c"
#include "game_script_def.c"
#include "save_menu_ui_def.c"
#include "equipment_ui_def.c"
local void start_dialogue_ui(void);
local void close_dialogue_ui(void);
#include "dialogue_script_parse.c"
#include "storyboard_presentation_def.c" /* need to check the rot on this... */

#include "fade_transitions.c"

local void announce_battle_action(struct entity_id who, string what);

local void set_scrollable_ui_bounds(s32 selection, s32* bottom_index, s32* top_index, s32 max_limit, s32 FIRST_SCROLLING_Y, s32 MAX_DISPLAYABLE_ITEMS) {
    assertion(bottom_index && top_index && "need pointers for those values");
    if ((MAX_DISPLAYABLE_ITEMS % 2) == 0) {
        *bottom_index = selection-MAX_DISPLAYABLE_ITEMS/2;
        *top_index    = selection+MAX_DISPLAYABLE_ITEMS/2;
    } else {
        *bottom_index = selection-MAX_DISPLAYABLE_ITEMS/2;
        *top_index    = selection+MAX_DISPLAYABLE_ITEMS/2+1;
    }

    if (max_limit <= MAX_DISPLAYABLE_ITEMS) {
        *bottom_index = 0;
        *top_index    = MAX_DISPLAYABLE_ITEMS;
    } else {
        for (s32 selection_index = 0; selection_index < FIRST_SCROLLING_Y; ++selection_index) {
            if (selection == selection_index) {
                *bottom_index = 0;
                *top_index    = MAX_DISPLAYABLE_ITEMS;
                break;
            }
        }
    }

    if (*top_index >= max_limit) {
        *top_index = max_limit;
    }

    return;
}

void serialize_tile(struct binary_serializer* serializer, s32 version, struct tile* tile) {
    switch (version) {
        default:
        case CURRENT_LEVEL_AREA_VERSION: {
            serialize_s32(serializer, &tile->id);
            serialize_u32(serializer, &tile->flags);
            serialize_s16(serializer, &tile->x);
            serialize_s16(serializer, &tile->y);
            /* remove */
            serialize_s16(serializer, &tile->reserved_);
            serialize_s16(serializer, NULL);
        } break;
    }
}

void serialize_tile_layer(struct binary_serializer* serializer, s32 version, s32* counter, struct tile* tile) {
    serialize_s32(serializer, counter);
    _debugprintf("%d tiles in this layer", *counter);
    for (s32 index = 0; index < *counter; ++index) {
        serialize_tile(serializer, version, tile+index);
    }
}

void level_area_entity_unpack(struct level_area_entity* entity, struct entity* unpack_target);
void level_area_entity_savepoint_unpack(struct level_area_savepoint* entity, struct entity_savepoint* unpack_target);

/* assorted random function prototype forward declarations */
void render_cutscene_entities(struct sortable_draw_entities* draw_entities);
void game_initialize_game_world(void);
void game_push_reported_entity_death(entity_id id);
void game_report_entity_death(entity_id id);

local f32 GLOBAL_GAME_TIMESTEP_MODIFIER = 1;

local void game_over_ui_setup(void);
local void game_produce_damaging_explosion(v2f32 where, f32 radius, s32 effect_explosion_id, s32 damage_amount, s32 team_origin, u32 explosion_flags);
struct game_state* game_state         = 0;
local bool         disable_game_input = false;

image_id drop_shadow           = {};
image_id ui_battle_defend_icon = {};
sound_id ui_blip               = {};
sound_id hit_sounds[3]         = {};
sound_id ui_blip_bad           = {};

local void play_random_hit_sound(void) {
    play_sound(hit_sounds[random_ranged_integer(&game_state->rng, 0, 1)]);
}

sound_id test_safe_music = {};
sound_id test_battle_music = {};

/* compile out */
#ifdef USE_EDITOR
struct editor_state* editor_state = 0;
static struct memory_arena editor_arena = {};
#endif

/* Single line command console for gamescript. */
local bool game_command_console_enabled                                         = false;
local char game_command_console_line_input[GAME_COMMAND_CONSOLE_LINE_INPUT_MAX] = {};

local void update_and_render_pause_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt);
local void update_and_render_gameover_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt);
local void update_and_render_save_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt);
local void update_and_render_ingame_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt);

/* defined in battle_ui.c */
local bool is_entity_under_ability_selection(entity_id who);

local struct light_def* game_dereference_dynamic_light(s32 light_id) {
    if (light_id == 0) {
        return NULL;
    }

    return (&game_state->dynamic_lights[light_id-1]);
}

local s32 game_allocate_dynamic_light(void) {
    if (game_state->dynamic_light_count < MAX_GAME_DYNAMIC_LIGHT_POOL) {
        return (game_state->dynamic_light_count++) + 1;
    }

    return 0;
}

local void game_kill_all_dynamic_lights(void) {
    game_state->dynamic_light_count = 0;
}

void game_free_dynamic_light(s32 light_id) {
    game_state->dynamic_lights[light_id] = game_state->dynamic_lights[--game_state->dynamic_light_count];
}

local void game_focus_camera_to_entity(struct entity* entity) {
    v2f32 focus_point = entity->position;
    camera_set_point_to_interpolate(&game_state->camera, focus_point);
}

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
            string filepath = format_temp_s("%.*s/%.*s.png", directory.length, directory.data, it->length, it->data);
            ui_skin.textures[index] = DEBUG_CALL(graphics_assets_load_image(graphics_assets, filepath));
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

local s32 nine_patch_estimate_fitting_extents_width(struct game_ui_nine_patch ui_skin, f32 scaling_factor, f32 width) {
    f32 tile_width  = ui_skin.tile_width * scaling_factor;
    return ceilf(width/tile_width);
}
local s32 nine_patch_estimate_fitting_extents_height(struct game_ui_nine_patch ui_skin, f32 scaling_factor, f32 height) {
    f32 tile_height  = ui_skin.tile_height * scaling_factor;
    return ceilf(height/tile_height);
}

v2f32 nine_patch_estimate_fitting_extents(struct game_ui_nine_patch ui_skin, f32 scaling_factor, f32 width, f32 height) {
    return v2f32(nine_patch_estimate_fitting_extents_width(ui_skin, scaling_factor, width),
                 nine_patch_estimate_fitting_extents_height(ui_skin, scaling_factor, height));
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

/* only used in the render command system which is smart enough to apply this stuff */
local struct tile_data_definition* tile_table_data;
local struct autotile_table*       auto_tile_info;

local sound_id get_current_sound_theme(void) {
    switch (game_state->current_theme_track_type) {
        case THEME_SAFE_TRACK: {
            /* the safe theme track should be settable */
            return test_safe_music;
        } break;
        case THEME_BATTLE_TRACK: {
            /* the battle theme track should be settable */
            return test_battle_music;
        } break;
    }
    return test_battle_music;
}

const u32 FADEIN_TIME  = 2000;
const u32 FADEOUT_TIME = 2000;

local void fadein_track(void) {
    game_state->music_fadein_timer = FADEIN_TIME/1000.0f;
    game_state->started_fading_in = false;
}

local void set_theme_track(u32 new_theme_track) {
    if (game_state->current_theme_track_type != new_theme_track) {
        game_state->last_theme_track_type    = game_state->current_theme_track_type;
        game_state->current_theme_track_type = new_theme_track;

        game_state->music_fadeout_timer = FADEOUT_TIME/1000.0f;
        game_state->started_fading_out = false;
        _debugprintf("set new theme track");
    }
}

local void game_continue_music(f32 dt);

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
    /* TODO: Wait until camera finishes interpolating */
    camera_set_point_to_interpolate(&state->camera, state->before_conversation_camera.xy);
    state->is_conversation_active = false;
    for (s32 node_index = 0; node_index < MAX_CONVERSATION_NODES; ++node_index) {
        state->current_conversation.nodes[node_index].script_code = (string){};
        for (s32 choice_index = 0; choice_index < MAX_CONVERSATION_CHOICES; ++choice_index) {
            state->current_conversation.nodes[node_index].choices[choice_index].script_code = (string){};
        }
    }
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

        switch (layer) {
            case TILE_LAYER_GROUND: {
                render_commands_set_shader(commands, game_background_things_shader, NULL);
            } break;
            default: {
                render_commands_set_shader(commands, game_foreground_things_shader, NULL);
            } break;
        }
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
    /*
      NOTE: if I didn't know what I meant by that earlier, I meant just render the entire level image once and just blit the
      prerendered layers, not needed but okay.
    */

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
}

struct entity* game_dereference_entity(struct game_state* state, entity_id id) {
    switch (id.store_type) {
        case ENTITY_LIST_STORAGE_TYPE_PER_LEVEL_CUTSCENE: {
            /* assertion(false && "DO NOT KNOW HOW TO HANDLE THIS RIGHT NOW"); */
            if (cutscene_viewing_separate_area()) {
                struct level_area* area = cutscene_view_area();
                return entity_list_dereference_entity(&area->entities, id);
            } else {
                return entity_list_dereference_entity(&state->permenant_entities, id);
            }
        } break;

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
entity_id entity_list_create_party_member(struct entity_list* entities) {
    entity_id      result  = entity_list_create_entity(entities);
    struct entity* player  = entity_list_dereference_entity(entities, result);

    assertion(player->flags & ENTITY_FLAGS_ACTIVE);
    player->flags    |= ENTITY_FLAGS_ALIVE;
    player->flags    |= ENTITY_FLAGS_PLAYER_CONTROLLED;
    player->flags    |= ENTITY_FLAGS_HIDDEN;
    player->health.value = 100;
    player->health.min = 100;
    player->health.max = 100;
    /* align collision boxes a bit better later :) */
    player->scale.x = TILE_UNIT_SIZE*0.8;
    player->scale.y = TILE_UNIT_SIZE*0.8;

    player->name    = string_literal("Lloyd Irving");
    return result;
}
entity_id entity_list_create_niceguy(struct entity_list* entities, v2f32 position) {
    entity_id result = entity_list_create_entity(entities);
    struct entity* e = entity_list_dereference_entity(entities, result);

    e->flags               |= ENTITY_FLAGS_ALIVE;
    e->position             = position;
    e->scale.x              = TILE_UNIT_SIZE-2;
    e->scale.y              = TILE_UNIT_SIZE-2;
    e->health.value         = 100;
    e->health.min           = 100;
    e->health.max           = 100;
    e->stat_block.xp_value  = 30;
    e->name                 = string_literal("Guy");
    entity_set_dialogue_file(e, string_literal("linear_test"));

    return result;
}
entity_id entity_list_create_badguy(struct entity_list* entities, v2f32 position) {
    entity_id result = entity_list_create_entity(entities);
    struct entity* e = entity_list_dereference_entity(entities, result);

    e->flags               |= ENTITY_FLAGS_ALIVE;
    e->position             = position;
    e->scale.x              = TILE_UNIT_SIZE-2;
    e->scale.y              = TILE_UNIT_SIZE-2;
    e->health.value         = 100;
    e->health.min           = 100;
    e->health.max           = 100;
    e->stat_block.xp_value  = 30;
    e->ai.flags             = ENTITY_AI_FLAGS_AGGRESSIVE_TO_PLAYER;
    e->name                 = string_literal("Ruffian");
    e->loot_table_id_index  = entity_database_loot_table_find_id_by_name(&game_state->entity_database, string_literal("bandit_loot0"));

    return result;
}

/* used to gauge the "flock distance" */
void game_set_all_party_members_to(v2f32 where) {
    for (s32 party_member_index = 0; party_member_index < game_state->party_member_count; ++party_member_index) {
        struct entity* party_member = game_dereference_entity(game_state, game_state->party_members[party_member_index]);
        party_member->position      = where;
    }
}
s32 game_get_party_index(struct entity* entity) {
    for (s32 party_member_index = 0; party_member_index < game_state->party_member_count; ++party_member_index) {
        struct entity* party_member = game_dereference_entity(game_state, game_state->party_members[party_member_index]);
        if (entity == party_member) {
            return party_member_index;
        }
    }
    return -1;
}
s32 game_get_party_number(struct entity* entity) {
    s32 n = 1;
    for (s32 party_member_index = 0; party_member_index < game_state->party_member_count; ++party_member_index) {
        struct entity* party_member = game_dereference_entity(game_state, game_state->party_members[party_member_index]);
        if (entity == party_member) {
            if (party_member_index == game_state->leader_index) {
                return 0;
            }
            return n;
        }
        n++;
    }
    return n;
}
/*
  Party Members don't know how to handle dismissing unless I store where entities used to exist, which is possible to do by updating the save record,
  I suppose.

  I really just need to store party member deltas...
*/
struct entity_iterator game_permenant_entity_iterator(struct game_state* state);
local struct entity* game_allocate_new_party_member(void) {
    struct entity_iterator iterator = game_permenant_entity_iterator(game_state);

    for (struct entity* current_entity = entity_iterator_begin(&iterator); !entity_iterator_finished(&iterator); current_entity = entity_iterator_advance(&iterator)) {
        if (!(current_entity->flags & ENTITY_FLAGS_ACTIVE)) {
            continue;
        }
        if (current_entity->flags & ENTITY_FLAGS_HIDDEN) {
            game_state->party_members[game_state->party_member_count++] = iterator.current_id;
            break;
        }
    }

    entity_id new_id = game_state->party_members[game_state->party_member_count-1];
    struct entity* party_member = game_dereference_entity(game_state, new_id);
    return party_member;
}
void game_add_party_member_from_existing(struct entity* entity, bool remove_from_world) {
    unimplemented("copy from the world, but selectively!");
}
void game_add_party_member(string basename) {
    struct entity* new_guy = game_allocate_new_party_member();
    struct entity_base_data* base_data = entity_database_find_by_name(&game_state->entity_database, basename);
    entity_base_data_unpack(&game_state->entity_database, base_data, new_guy);
    new_guy->flags &= ~(ENTITY_FLAGS_HIDDEN);
}
void game_remove_party_member(s32 index) {
    struct entity* to_remove  = game_dereference_entity(game_state, game_state->party_members[index]);
    to_remove->flags         |= ENTITY_FLAGS_HIDDEN;
    for (s32 other_index = index; other_index < game_state->party_member_count; ++other_index) {
        game_state->party_members[other_index] = game_state->party_members[other_index+1];
    }
    game_state->party_member_count--;
}
void game_set_party_leader(s32 index) {
    struct entity* last_leader = game_dereference_entity(game_state, game_state->party_members[game_state->leader_index]);
    struct entity* new_leader  = game_dereference_entity(game_state, game_state->party_members[game_state->leader_index]);
    Swap(last_leader->position, new_leader->position, v2f32);
    game_state->leader_index = index;
}

void game_swap_party_member_index(s32 first, s32 second) {
    if (first == game_state->leader_index) {
        game_state->leader_index = second;
    } else if (second == game_state->leader_index) {
        game_state->leader_index = first;
    }

    Swap(game_state->party_members[first], game_state->party_members[second], entity_id);
}

struct entity* game_get_party_leader(void) {
    return game_dereference_entity(game_state, game_state->party_members[game_state->leader_index]);
}

struct entity* game_get_player(struct game_state* state) {
    return game_dereference_entity(state, state->party_members[state->leader_index]);
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

        navigation_map->tiles                 = memory_arena_push(arena, sizeof(*navigation_map->tiles) * navigation_map->width * navigation_map->height);
        level->combat_movement_visibility_map = memory_arena_push(arena, navigation_map->width * navigation_map->height);
        _debugprintf("Navigation map is estimated to be %d x %d", navigation_map->width, navigation_map->height);

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

/*
  Shopping will be a bit more management. I could just save a checkpoint pointer for the usage, and restore
  from there.

  Since technically it's a form of stack allocation. Eh.
*/
local void shopping_ui_begin(void); /* UI will end itself. */
local void game_stop_shopping(void) {
    game_state->shopping = false;
    /* TODO: unload shop contents */
}
local void game_begin_shopping(string storename) {
    if (!game_state->shopping) {
        game_state->shopping = true;
        game_state->active_shop = load_shop_definition(game_state->arena, storename);
        shopping_ui_begin();
    }
}

struct entity* game_any_entity_at_tile_point(v2f32 xy);
struct entity* game_any_entity_at_tile_point_except(v2f32 xy, struct entity** filter, s32 filter_count);

/* This is just going to start as a breadth first search */
/* NOTE: add a version with obstacle parameters, since the AI doesn't obey those rules! */
/* NOTE: this can fail if BFS doesn't find a finished path...  */
s32 level_area_navigation_map_tile_type_at(struct level_area* area, s32 x, s32 y) {
    struct level_area_navigation_map* navigation_map = &area->navigation_data;

    if (level_area_navigation_map_is_point_in_bounds(navigation_map, v2f32(x, y))) {
        s32                               map_width               = navigation_map->width;
        s32                               map_height              = navigation_map->height;
        s32                               total_elements          = (map_width * map_height);
        struct level_area_navigation_map_tile* tile = &navigation_map->tiles[((s32)y - navigation_map->min_y) * map_width + ((s32)x - navigation_map->min_x)];
        return tile->type;
    }

    return 0;
}
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

                        s32 tile_type = level_area_navigation_map_tile_type_at(area, new_point->x, new_point->y);
                        if (tile_type != 0) {
                            can_bresenham_trace = false;
                        }

                        if (game_any_entity_at_tile_point(*new_point)) {
                            can_bresenham_trace = false;
                        }
                    }

                    if (x1 == x2 && y1 == y2) {
                        done_tracing = true;  
                        results.success = true;
                    } 

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

            while (exploration_queue_start < exploration_queue_end && !found_end) {
                v2f32 current_point = exploration_queue[exploration_queue_start++];
                /* _debugprintf("current point: <%d, %d>", (s32)current_point.x, (s32)current_point.y); */
                explored_points[((s32)current_point.y - navigation_map->min_y) * map_width + ((s32)current_point.x - navigation_map->min_x)] = true;

                /* add neighbors */
                /* might have to make four neighbors. We can configure it anyhow */
                /* _debugprintf("try to find neighbors"); */
                {
                    local v2s32 neighbor_offsets[] = {
                        v2s32(1, 0),
                        v2s32(0, 1),
                        v2s32(-1, 0),
                        v2s32(0, -1),
                    };

                    for (s32 index = 0; index < array_count(neighbor_offsets); ++index) {
                        v2f32 proposed_point  = current_point;
                        proposed_point.x     += neighbor_offsets[index].x;
                        proposed_point.y     += neighbor_offsets[index].y;

                        /* _debugprintf("neighbor <%d, %d> (origin as: <%d, %d>) (%d, %d offset) proposed", (s32)proposed_point.x, (s32)proposed_point.y, (s32)current_point.x, (s32)current_point.y, x_cursor, y_cursor); */
                        if (level_area_navigation_map_is_point_in_bounds(navigation_map, proposed_point)) {
                            s32 tile_type = level_area_navigation_map_tile_type_at(area, proposed_point.x, proposed_point.y);

                            if (tile_type == 0 && !game_any_entity_at_tile_point(proposed_point)) {
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
                    results.success          = true;
                    Reverse_Array_Inplace(results.points, path_length, v2f32);
                }
            }
        }
    } else {
        _debugprintf("there's a point that's outside the map... Can't nav!");
    }

    return results;
}

void _serialize_level_area(struct memory_arena* arena, struct binary_serializer* serializer, struct level_area* level, s32 level_type) {
    memory_arena_set_allocation_region_top(arena); {
        _debugprintf("%llu memory used", arena->used + arena->used_top);
        memory_arena_clear_top(arena);
        _debugprintf("reading version");
        serialize_u32(serializer, &level->version);
        _debugprintf("V: %d", level->version);
        _debugprintf("reading default player spawn");
        serialize_f32(serializer, &level->default_player_spawn.x);
        serialize_f32(serializer, &level->default_player_spawn.y);
        _debugprintf("reading tiles");

        /* I should try to move this into one spot since it's a little annoying */
        if (level->version >= 4) {
#define Serialize_Tile_Layer(LAYER)                                     \
            do {                                                        \
                 {                                                      \
                     serialize_s32(serializer, &level->tile_counts[LAYER]); \
                     _debugprintf("%d tiles in this layer", level->tile_counts[LAYER]); \
                     level->tile_layers[LAYER] = memory_arena_push(arena, level->tile_counts[LAYER]*sizeof(*level->tile_layers[LAYER])); \
                     for (s32 _index = 0; _index < level->tile_counts[LAYER]; ++_index) { \
                         serialize_tile(serializer, level->version, &level->tile_layers[LAYER][_index]); \
                     }                                                  \
                 }                                                      \
            } while(0)
            if (level->version < CURRENT_LEVEL_AREA_VERSION) {
                /* for older versions I have to know what the tile layers were and assign them like this. */
                switch (level->version) {
                    case 4: {
                        Serialize_Tile_Layer(TILE_LAYER_GROUND);
                        Serialize_Tile_Layer(TILE_LAYER_OBJECT);
                        Serialize_Tile_Layer(TILE_LAYER_ROOF);
                        Serialize_Tile_Layer(TILE_LAYER_FOREGROUND);
                    } break;
                    default: {
                        goto didnt_change_level_tile_format_from_current;
                    } break;
                }
            } else {
                /* the current version of the tile layering, we can just load them in order. */
            didnt_change_level_tile_format_from_current:
                for (s32 I = 0; I < TILE_LAYER_COUNT; ++I) {
                    Serialize_Tile_Layer(I);
                }
            }
        } else {
            Serialize_Tile_Layer(TILE_LAYER_OBJECT);
        }

        if (level->version >= 1) {
            _debugprintf("reading level transitions");
            serialize_s32(serializer, &level->trigger_level_transition_count);
            _debugprintf("%d level transitions to read", level->trigger_level_transition_count);
            level->trigger_level_transitions = memory_arena_push(arena, sizeof(*level->trigger_level_transitions) * level->trigger_level_transition_count);
            for (s32 trigger_index = 0; trigger_index < level->trigger_level_transition_count; ++trigger_index) {
                serialize_trigger_level_transition(serializer, level->version, level->trigger_level_transitions + trigger_index);
            }
        }
        if (level->version >= 2) {
            _debugprintf("reading containers");
            serialize_s32(serializer, &level->entity_chest_count);
            _debugprintf("%d chest entities to read", level->entity_chest_count);
            level->chests = memory_arena_push(arena, sizeof(*level->chests) * level->entity_chest_count);
            for (s32 chest_index = 0; chest_index < level->entity_chest_count; ++chest_index) {
                serialize_entity_chest(serializer, level->version, level->chests + chest_index);
            }
        }

        if (level->version >= 3) {
            _debugprintf("reading scriptable triggers");
            serialize_s32(serializer, &level->script_trigger_count);
            _debugprintf("%d scriptable triggers to read", level->script_trigger_count);
            level->script_triggers = memory_arena_push(arena, sizeof(*level->script_triggers) * level->script_trigger_count);
            for (s32 trigger_index = 0; trigger_index < level->script_trigger_count; ++trigger_index) {
                serialize_generic_trigger(serializer, level->version, level->script_triggers + trigger_index);
            }
        }
        if (level->version >= 5) {
            _debugprintf("reading and unpacking entities");

            s32 entity_count = 0;
            serialize_s32(serializer, &entity_count);
            _debugprintf("Seeing %d entities to read", entity_count);

            level->entities = entity_list_create(arena, (entity_count+1), level_type);

            struct level_area_entity current_packed_entity = {};
            for (s32 entity_index = 0; entity_index < entity_count; ++entity_index) {
                entity_id      new_ent        = entity_list_create_entity(&level->entities);
                struct entity* current_entity = entity_list_dereference_entity(&level->entities, new_ent);
                serialize_level_area_entity(serializer, level->version, &current_packed_entity);
                level_area_entity_unpack(&current_packed_entity, current_entity);
            }
        }
        if (level->version >= 6) {
            _debugprintf("loading lights");
            serialize_s32(serializer, &level->light_count);
            _debugprintf("seeing %d lights to load", level->light_count);
            level->lights = memory_arena_push(arena, sizeof(*level->lights) * level->light_count);
            for (s32 light_index = 0; light_index < level->light_count; ++light_index) {
                serialize_light(serializer, level->version, level->lights + light_index);
            }
        }

        if (level->version >= 9) {
            serialize_s32(serializer, &level->entity_savepoint_count);
            level->savepoints = memory_arena_push(arena, sizeof(*level->savepoints) * level->entity_savepoint_count);

            struct level_area_savepoint packed_entity = {};
            _debugprintf("Will need to unpack %d savepoints!", level->entity_savepoint_count);
            for (s32 entity_savepoint_index = 0; entity_savepoint_index < level->entity_savepoint_count; ++entity_savepoint_index) {
                struct entity_savepoint* current_savepoint = level->savepoints + entity_savepoint_index;

                serialize_level_area_entity_savepoint(serializer, level->version, &packed_entity);
                level_area_entity_savepoint_unpack(&packed_entity, current_savepoint);
            }
        }

        build_navigation_map_for_level_area(arena, level);
    } memory_arena_set_allocation_region_bottom(arena);
}
void serialize_level_area(struct game_state* state, struct binary_serializer* serializer, struct level_area* level, bool use_default_spawn) {
    _serialize_level_area(state->arena, serializer, level, ENTITY_LIST_STORAGE_TYPE_PER_LEVEL);
    struct entity* player = game_get_player(game_state);
    if (use_default_spawn) {
        game_set_all_party_members_to(v2f32(
                                          level->default_player_spawn.x,
                                          level->default_player_spawn.y
                                      ));
    }

    state->camera.xy.x = player->position.x;
    state->camera.xy.y = player->position.y;
}

local void load_area_script(struct memory_arena* arena, struct level_area* area, string script_name) {
    /* NOTE: all of this stuff needs to allocate memory from the top!!! */
    struct level_area_script_data* script_data = &area->script;

    memory_arena_set_allocation_region_top(arena);
    if (file_exists(script_name)) {
        script_data->present     = true;
        script_data->buffer      = read_entire_file(memory_arena_allocator(arena), script_name);
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
                            _debugprintf("found an event listener: \"%s\"", level_area_listen_event_form_names[name_index].data);
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
    memory_arena_set_allocation_region_bottom(arena);
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
        zero_memory(area, sizeof(*area));
    }
}

#include "entity.c"

/* this is a game level load, changes zone */
void load_level_from_file(struct game_state* state, string filename) {
    passive_speaking_dialogue_clear_all();
    game_script_clear_all_awaited_scripts();
    entity_particle_emitter_kill_all(&game_state->permenant_particle_emitters);
    particle_list_kill_all_particles(&global_particle_list);
    cutscene_stop();
    level_area_clean_up(&state->loaded_area);
    memory_arena_clear_top(state->arena);
    /* clear scripts to prevent resident memory from doing things */

    state->loaded_area.on_enter_triggered = false;

    set_theme_track(THEME_SAFE_TRACK);
    fadein_track();

    string fullpath = string_concatenate(&scratch_arena, string_literal("areas/"), filename);
    if (file_exists(fullpath)) {
#ifdef EXPERIMENTAL_VFS
        /* This is slow path. I should write a native backend for the VFS instead of this weird hack */
        struct file_buffer file = read_entire_file(memory_arena_allocator(&scratch_arena), fullpath);
        struct binary_serializer serializer = open_read_memory_serializer(file.buffer, file.length);
        file_buffer_free(&file);
#else
        struct binary_serializer serializer = open_read_file_serializer(fullpath);
#endif
        serialize_level_area(state, &serializer, &state->loaded_area, true);
        load_area_script(game_state->arena, &state->loaded_area, string_concatenate(&scratch_arena, string_slice(fullpath, 0, (fullpath.length+1)-sizeof("area")), string_literal("area_script")));
        {
            cstring_copy(filename.data, state->loaded_area_name, filename.length);
        }
        serializer_finish(&serializer);
    } else {
        _debugprintf("level does not exist! (%.*s)", filename.length, filename.data);
    }
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

local void register_all_entities_to_save_record(void);
local void game_open_save_menu(void) {
    save_menu_open_for_saving();
    game_state_set_ui_state(game_state, UI_STATE_SAVEGAME);
    game_state->ui_save.effects_timer = 0;
    game_state->ui_save.phase         = UI_SAVE_MENU_PHASE_FADEIN;
    register_all_entities_to_save_record();
}

local void _transition_callback_game_over(void*) {
    game_state_set_ui_state(game_state, UI_STATE_GAMEOVER);
    game_over_ui_setup();
}

local bool global_game_initiated_death_ui = false;

void game_postprocess_blur(struct software_framebuffer* framebuffer, s32 quality_scale, f32 t, u32 blend_mode) {
#ifdef NO_POSTPROCESSING
    return;
#endif

    f32 box_blur[] = {
        1,1,1,
        1,1,1,
        1,1,1,
    };

    struct software_framebuffer blur_buffer = software_framebuffer_create_from_arena(&scratch_arena, framebuffer->width/quality_scale, framebuffer->height/quality_scale);
    software_framebuffer_copy_into(&blur_buffer, framebuffer);
    software_framebuffer_kernel_convolution_ex(&scratch_arena, &blur_buffer, box_blur, 3, 3, 12, t, 2);
    software_framebuffer_draw_image_ex(framebuffer, (struct image_buffer*)&blur_buffer, rectangle_f32(0,0,framebuffer->width, framebuffer->height), RECTANGLE_F32_NULL, color32f32(1,1,1,1), NO_FLAGS, blend_mode);
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
    struct software_framebuffer blur_buffer = software_framebuffer_create_from_arena(&scratch_arena, framebuffer->width/quality_scale, framebuffer->height/quality_scale);
    software_framebuffer_copy_into(&blur_buffer, framebuffer);
    software_framebuffer_kernel_convolution_ex(&scratch_arena, &blur_buffer, box_blur, 3, 3, 10, t, 1);
    software_framebuffer_draw_image_ex(framebuffer, (struct image_buffer*)&blur_buffer, rectangle_f32(0,0,framebuffer->width, framebuffer->height), RECTANGLE_F32_NULL, color32f32(1,1,1,1), NO_FLAGS, blend_mode);
}

union color32f32 grayscale_shader(struct software_framebuffer* framebuffer, union color32f32 source_pixel, v2f32 pixel_position, void* context) {
    f32 t = 1.0;
    if (context) {
        t = *(float*)context; 
    }
    f32 average = (source_pixel.r + source_pixel.g + source_pixel.b) / 3.0f;

    f32 new_r = lerp_f32(source_pixel.r, average, t);
    f32 new_g = lerp_f32(source_pixel.g, average, t);
    f32 new_b = lerp_f32(source_pixel.b, average, t);
    return color32f32(new_r, new_g, new_b, source_pixel.a);
}

union color32f32 lighting_shader(struct software_framebuffer* framebuffer, union color32f32 source_pixel, v2f32 pixel_position, void* context) {
    struct level_area* loaded_area = context;

    if (lightmask_buffer_is_lit(&global_lightmask_buffer, pixel_position.x, pixel_position.y)) {
        f32 forced_r = source_pixel.r;
        f32 forced_g = source_pixel.g;
        f32 forced_b = source_pixel.b;
        f32 light_percentage = lightmask_buffer_lit_percent(&global_lightmask_buffer, pixel_position.x, pixel_position.y);
        forced_r *= light_percentage;
        forced_g *= light_percentage;
        forced_b *= light_percentage;

        if (forced_r > 255) forced_r = 255;
        if (forced_g > 255) forced_g = 255;
        if (forced_b > 255) forced_b = 255;

        return color32f32(forced_r, forced_g, forced_b, 1.0);
    }

    if (special_effects_active() && special_full_effects.type == SPECIAL_EFFECT_INVERSION_1) {
        return source_pixel;
    }

    union color32f32 result = source_pixel;

    f32 r_accumulation = 0;
    f32 g_accumulation = 0;
    f32 b_accumulation = 0;

    for (s32 light_index = 0; light_index < loaded_area->light_count; ++light_index) {
        struct light_def* current_light = loaded_area->lights + light_index;
        if (current_light->flags & LIGHT_FLAGS_HIDDEN) {
            continue;
        }

        v2f32 light_screenspace_position = current_light->position;
        /* recentering lights */
        light_screenspace_position.x += 0.5;
        light_screenspace_position.y += 0.5;
        light_screenspace_position.x *= TILE_UNIT_SIZE;
        light_screenspace_position.y *= TILE_UNIT_SIZE;

        light_screenspace_position = camera_transform(&game_state->camera, light_screenspace_position, SCREEN_WIDTH, SCREEN_HEIGHT);
        {
            f32 distance_squared = v2f32_magnitude_sq(v2f32_sub(pixel_position, light_screenspace_position));
            f32 attenuation      = 1/(distance_squared+1 + (sqrtf(distance_squared)/1.5));

            f32 power = current_light->power * game_state->camera.zoom;
            r_accumulation += attenuation * power * TILE_UNIT_SIZE * current_light->color.r/255.0f;
            g_accumulation += attenuation * power * TILE_UNIT_SIZE * current_light->color.g/255.0f;
            b_accumulation += attenuation * power * TILE_UNIT_SIZE * current_light->color.b/255.0f;
        }
    }

    for (s32 light_index = 0; light_index < game_state->dynamic_light_count; ++light_index) {
        struct light_def* current_light = game_state->dynamic_lights + light_index;
        if (current_light->flags & LIGHT_FLAGS_HIDDEN) {
            continue;
        }

        v2f32 light_screenspace_position = current_light->position;
        /* recentering lights */
        light_screenspace_position.x += 0.5;
        light_screenspace_position.y += 0.5;
        light_screenspace_position.x *= TILE_UNIT_SIZE;
        light_screenspace_position.y *= TILE_UNIT_SIZE;

        light_screenspace_position = camera_transform(&game_state->camera, light_screenspace_position, SCREEN_WIDTH, SCREEN_HEIGHT);
        {
            f32 distance_squared = v2f32_magnitude_sq(v2f32_sub(pixel_position, light_screenspace_position));
            f32 attenuation      = 1/(distance_squared+1 + (sqrtf(distance_squared)/1.5));

            f32 power = current_light->power * game_state->camera.zoom;
            r_accumulation += attenuation * power * TILE_UNIT_SIZE * current_light->color.r/255.0f;
            g_accumulation += attenuation * power * TILE_UNIT_SIZE * current_light->color.g/255.0f;
            b_accumulation += attenuation * power * TILE_UNIT_SIZE * current_light->color.b/255.0f;
        }
    }

    f32 overbright_r = source_pixel.r * 2.0;
    f32 overbright_g = source_pixel.g * 2.0;
    f32 overbright_b = source_pixel.b * 2.0;

    if (r_accumulation > overbright_r) { r_accumulation = overbright_r; }
    if (g_accumulation > overbright_g) { g_accumulation = overbright_g; }
    if (b_accumulation > overbright_b) { b_accumulation = overbright_b; }

    result.r = r_accumulation + global_color_grading_filter.r/255.0f * source_pixel.r;
    result.g = g_accumulation + global_color_grading_filter.g/255.0f * source_pixel.g;
    result.b = b_accumulation + global_color_grading_filter.b/255.0f * source_pixel.b;

    return result;
}

local s32 game_allocate_particle_emitter_one_shot(v2f32 where) {
    s32                             particle_emitter = entity_particle_emitter_allocate(&game_state->permenant_particle_emitters);
    struct entity_particle_emitter* emitter          = entity_particle_emitter_dereference(&game_state->permenant_particle_emitters, particle_emitter);

    emitter->max_spawn_batches = 1;
    emitter->position          = where;

    return particle_emitter;
}
/* coordinates a tile units */
enum damaging_explosion_flags {
    DAMAGING_EXPLOSION_FLAGS_NONE = 0,
    DAMAGING_EXPLOSION_FLAGS_NOHURT_PLAYER = BIT(0),
};

local void game_produce_damaging_explosion(v2f32 where, f32 radius, s32 effect_explosion_id, s32 damage_amount, s32 team_origin, u32 explosion_flags) {
    s32                             explosion_particle_emitter = game_allocate_particle_emitter_one_shot(where);
    struct entity_particle_emitter* emitter                    = entity_particle_emitter_dereference(&game_state->permenant_particle_emitters, explosion_particle_emitter);

    switch (effect_explosion_id) {
        default: {
            emitter->time_per_spawn                  = 0.02;
            emitter->position                        = where;
            emitter->burst_amount                    = 512;
            emitter->max_spawn_per_batch             = 1024;
            emitter->color                           = color32u8(226, 88, 34, 255);
            emitter->target_color                    = color32u8(59, 59, 56, 127);
            emitter->starting_acceleration           = v2f32(0, -15.6);
            emitter->starting_acceleration_variance  = v2f32(4, 4);
            emitter->starting_velocity_variance      = v2f32(10,10);
            emitter->lifetime                        = 0.2;
            emitter->lifetime_variance               = 0.15;

            /* emitter->particle_type = ENTITY_PARTICLE_TYPE_FIRE; */
            emitter->particle_feature_flags = ENTITY_PARTICLE_FEATURE_FLAG_FLAMES;

            emitter->scale_uniform = 0.35;
            emitter->scale_variance_uniform = 0.12;
            emitter->spawn_shape = emitter_spawn_shape_circle(v2f32(0,0), radius, 0.0, false);
            entity_particle_emitter_start_emitting(&game_state->permenant_particle_emitters, explosion_particle_emitter);
        } break;
    }

    { /* should be extracted else where later? */
        struct entity_query_list potential_targets = find_entities_within_radius(&scratch_arena, game_state, v2f32_scale(where, TILE_UNIT_SIZE), radius * TILE_UNIT_SIZE, true);

        for (u32 entity_id_index = 0; entity_id_index < potential_targets.count; ++entity_id_index) {
            struct entity* current_entity = game_dereference_entity(game_state, potential_targets.ids[entity_id_index]);

            if (explosion_flags & DAMAGING_EXPLOSION_FLAGS_NOHURT_PLAYER) {
                if (current_entity->flags & ENTITY_FLAGS_PLAYER_CONTROLLED) {
                    continue;
                }
            }

            entity_do_physical_hurt(current_entity, damage_amount);
        }

        camera_traumatize(&game_state->camera, 0.35);
        controller_rumble(get_gamepad(0), 1, 1, 500);
    }
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

local void draw_ui_breathing_text_word_wrapped(struct software_framebuffer* framebuffer, v2f32 where, f32 wrap_width, struct font_cache* font, f32 scale, string text, s32 seed_displacement, union color32f32 modulation) {
    f32 font_height = font_cache_text_height(font) * scale;

    f32 x_cursor = where.x;
    f32 y_cursor = where.y;

    for (unsigned character_index = 0; character_index < text.length; ++character_index) {
        s32 current_word_start = character_index;
        s32 current_word_end   = current_word_start;
        {
            while (current_word_end < text.length && text.data[current_word_end] != ' ') {
                current_word_end++;
            }
        }
        string word = string_slice(text, current_word_start, current_word_end);
        f32 word_width = font_cache_text_width(font, word, scale);

        if ((x_cursor-where.x) + word_width >= wrap_width || text.data[character_index] == '\n') {
            x_cursor  = where.x;
            y_cursor += font_height;
        }

        f32 character_displacement_y = sinf((global_elapsed_time*2) + ((character_index+seed_displacement) * 2381.2318)) * 3;

        v2f32 glyph_position = v2f32(x_cursor, y_cursor + character_displacement_y);
        x_cursor += font->tile_width * scale;

        software_framebuffer_draw_text(framebuffer, font, scale, glyph_position, string_slice(text, character_index, character_index+1), modulation, BLEND_MODE_ALPHA);
    }
}

local void draw_ui_breathing_text_word_wrapped_centered(struct software_framebuffer* framebuffer, struct rectangle_f32 bounds, f32 wrap_width, struct font_cache* font, f32 scale, string text, s32 seed_displacement, union color32f32 modulation) {
    f32 text_width  = font_cache_text_width(font, text, scale);
    f32 text_height = font_cache_calculate_height_of(font, text, wrap_width, scale);
    f32 font_height = font_cache_text_height(font) * scale;


    v2f32 centered_starting_position = v2f32(0,0);

    centered_starting_position.x = bounds.x + (bounds.w/2) - (wrap_width/2);
    centered_starting_position.y = bounds.y + (bounds.h/2) - (text_height/2);

    f32 x_cursor = centered_starting_position.x;
    f32 y_cursor = centered_starting_position.y;

    for (unsigned character_index = 0; character_index < text.length; ++character_index) {
        s32 current_word_start = character_index;
        s32 current_word_end   = current_word_start;
        {
            while (current_word_end < text.length && text.data[current_word_end] != ' ') {
                current_word_end++;
            }
        }
        string word = string_slice(text, current_word_start, current_word_end);
        f32 word_width = font_cache_text_width(font, word, scale);

        if ((x_cursor-centered_starting_position.x) + word_width >= wrap_width || text.data[character_index] == '\n') {
            x_cursor  = centered_starting_position.x;
            y_cursor += font_height;
        }

        f32 character_displacement_y = sinf((global_elapsed_time*2) + ((character_index+seed_displacement) * 2381.2318)) * 3;

        v2f32 glyph_position = v2f32(x_cursor, y_cursor + character_displacement_y);
        x_cursor += font->tile_width * scale;

        software_framebuffer_draw_text(framebuffer, font, scale, glyph_position, string_slice(text, character_index, character_index+1), modulation, BLEND_MODE_ALPHA);
    }
}

/* NOTE: specific variation used by the game over screen, it's "better" for slowly displaying text. The dialogue system doesn't need it, since it looks fine, but it's annoying on the death screen */
/* wrapupon is the same as text, but upto the next word */
local void draw_ui_breathing_text_word_wrapped_centered1(struct software_framebuffer* framebuffer, struct rectangle_f32 bounds, f32 wrap_width, struct font_cache* font, f32 scale, string wrap_upon, string text, s32 seed_displacement, union color32f32 modulation) {
    f32 text_width  = font_cache_text_width(font, text, scale);
    f32 text_height = font_cache_calculate_height_of(font, text, wrap_width, scale);
    f32 font_height = font_cache_text_height(font) * scale;


    v2f32 centered_starting_position = v2f32(0,0);

    centered_starting_position.x = bounds.x + (bounds.w/2) - (wrap_width/2);
    centered_starting_position.y = bounds.y + (bounds.h/2) - (text_height/2);

    f32 x_cursor = centered_starting_position.x;
    f32 y_cursor = centered_starting_position.y;

    for (unsigned character_index = 0; character_index < text.length; ++character_index) {
        s32 current_word_start = character_index;
        s32 current_word_end   = current_word_start;
        {
            while (current_word_end < wrap_upon.length && wrap_upon.data[current_word_end] != ' ') {
                current_word_end++;
            }
        }
        string word = string_slice(wrap_upon, current_word_start, current_word_end);
        f32 word_width = font_cache_text_width(font, word, scale);

        if ((x_cursor-centered_starting_position.x) + word_width >= wrap_width || text.data[character_index] == '\n') {
            x_cursor  = centered_starting_position.x;
            y_cursor += font_height;
        }

        f32 character_displacement_y = sinf((global_elapsed_time*2) + ((character_index+seed_displacement) * 2381.2318)) * 3;

        v2f32 glyph_position = v2f32(x_cursor, y_cursor + character_displacement_y);
        x_cursor += font->tile_width * scale;

        software_framebuffer_draw_text(framebuffer, font, scale, glyph_position, string_slice(text, character_index, character_index+1), modulation, BLEND_MODE_ALPHA);
    }
}


#ifdef USE_EDITOR
void editor_initialize(struct editor_state* state);
#include "editor.c"
#endif

#include "tile_data.c"

local void initialize_main_menu(void);
local void _unlock_game_input(void*) {
    disable_game_input = false;
}
local void fade_into_game(void) {
    /* TODO: This has to vary a bit depending on when this is called! */
    game_state->ui_state = UI_STATE_INGAME;
    game_initialize_game_world();
    disable_game_input             = true;
    global_game_initiated_death_ui = false;
    do_color_transition_out(color32f32(0, 0, 0, 1), 0.2, 0.35);
    transition_register_on_finish(_unlock_game_input, NULL, 0);
}

local void game_VFS_mount_archives(void) {
    /* main databigfile */
    mount_bigfile_archive(&game_arena, string_literal("./data.bigfile"));
    mount_bigfile_archive(&game_arena, string_literal("./mod1.bigfile"));
}

#define Report_Memory_Status_Region(ARENA, WHY) \
    do {                                        \
        _debugprintf(WHY);                      \
        _memory_arena_peak_usages(ARENA);       \
    } while (0)

#define PrintStructSize(s) _debugprintf(#s " is %d bytes", (s32)sizeof(s))
void game_initialize(void) {
    PrintStructSize(struct entity);
    PrintStructSize(struct entity_sequence_state);
    PrintStructSize(struct entity_animation_state);
    PrintStructSize(struct entity_actor_inventory);
    PrintStructSize(struct entity_ai_data);
    PrintStructSize(struct used_battle_action_stack);
    game_arena   = memory_arena_create_from_heap("Game Memory", Megabyte(64));
    scratch_arena = memory_arena_create_from_heap("Scratch Buffer", Megabyte(8));
#ifdef USE_EDITOR
    editor_arena = memory_arena_create_from_heap("Editor Memory", Megabyte(32));
#endif

    game_VFS_mount_archives();
    game_state                      = memory_arena_push(&game_arena, sizeof(*game_state));
    game_state->variables           = game_variables(&game_arena);
    game_state->conversation_arena  = memory_arena_push_sub_arena(&game_arena, Kilobyte(64));
    Report_Memory_Status_Region(&game_arena, "Conversation Arena & Game Variables Initialization");

    cutscene_initialize(&game_arena);
    game_state->entity_database     = entity_database_create(&game_arena);
    Report_Memory_Status_Region(&game_arena, "Entity Database allocation");
    
    initialize_save_data();
    initialize_main_menu();

    game_state->rng   = random_state();
    game_state->arena = &game_arena;
    graphics_assets   = graphics_assets_create(&game_arena, 16, 1024);
    Report_Memory_Status_Region(&game_arena, "Graphics assets allocation");

    combat_square_unselected = DEBUG_CALL(graphics_assets_load_image(&graphics_assets, string_literal(GAME_DEFAULT_RESOURCE_PATH "img/cmbt/cmbt_grid_sq.png")));
    combat_square_selected   = DEBUG_CALL(graphics_assets_load_image(&graphics_assets, string_literal(GAME_DEFAULT_RESOURCE_PATH "img/cmbt/cmbt_selected_sq.png")));
    drop_shadow              = DEBUG_CALL(graphics_assets_load_image(&graphics_assets, string_literal(GAME_DEFAULT_RESOURCE_PATH "img/dropshadow.png")));
    ui_battle_defend_icon    = DEBUG_CALL(graphics_assets_load_image(&graphics_assets, string_literal(GAME_DEFAULT_RESOURCE_PATH "img/ui/battle/defend.png")));

    chest_closed_img      = DEBUG_CALL(graphics_assets_load_image(&graphics_assets, string_literal(GAME_DEFAULT_RESOURCE_PATH "img/chestclosed.png")));
    chest_open_bottom_img = DEBUG_CALL(graphics_assets_load_image(&graphics_assets, string_literal(GAME_DEFAULT_RESOURCE_PATH "img/chestopenbottom.png")));
    chest_open_top_img    = DEBUG_CALL(graphics_assets_load_image(&graphics_assets, string_literal(GAME_DEFAULT_RESOURCE_PATH "img/chestopentop.png")));
    ui_chunky             = DEBUG_CALL(game_ui_nine_patch_load_from_directory(&graphics_assets, string_literal(GAME_DEFAULT_RESOURCE_PATH "img/ui/chunky"), 16, 16));
    /* TODO: Load from file */
    global_entity_models = entity_model_database_create(&game_arena, 512);
    Report_Memory_Status_Region(&game_arena, "Entity models database");

    game_script_initialize(&game_arena);

    passive_speaking_dialogues = memory_arena_push(&game_arena, MAX_PASSIVE_SPEAKING_DIALOGUES * sizeof(*passive_speaking_dialogues));
    Report_Memory_Status_Region(&game_arena, "Passive Dialogues & Game Script initialization");

    ui_blip     = load_sound(string_literal(GAME_DEFAULT_RESOURCE_PATH "snds/ui_select.wav"), false);
    ui_blip_bad = load_sound(string_literal(GAME_DEFAULT_RESOURCE_PATH "snds/ui_bad.wav"), false);
    hit_sounds[0] = load_sound(string_literal(GAME_DEFAULT_RESOURCE_PATH "snds/hit1.wav"), false);
    hit_sounds[1] = load_sound(string_literal(GAME_DEFAULT_RESOURCE_PATH "snds/hit2.wav"), false);
    hit_sounds[2] = load_sound(string_literal(GAME_DEFAULT_RESOURCE_PATH "snds/hitcrit.wav"), false);

    test_battle_music = load_sound(string_literal(GAME_DEFAULT_RESOURCE_PATH "snds/battleThemeA.ogg"), true);
    test_safe_music   = load_sound(string_literal(GAME_DEFAULT_RESOURCE_PATH "snds/field_of_dreams.ogg"), true);
    /* play_sound(test_mus); */

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
    game_state->permenant_entities          = entity_list_create(&game_arena, GAME_MAX_PERMENANT_ENTITIES, ENTITY_LIST_STORAGE_TYPE_PERMENANT_STORE);
    game_state->permenant_particle_emitters = entity_particle_emitter_list(&game_arena, GAME_MAX_PERMENANT_PARTICLE_EMITTERS);
    initialize_particle_pools(&game_arena, PARTICLE_POOL_MAX_SIZE);
    Report_Memory_Status_Region(&game_arena, "Permenant entity pools");

    {
        for (s32 party_member_index = 0; party_member_index < MAX_PARTY_MEMBERS; ++party_member_index) {
            entity_id* id = game_state->party_members + party_member_index;
            /* *id = entity_list_create_player(&game_state->permenant_entities, v2f32(140, 300)); */
            *id = entity_list_create_party_member(&game_state->permenant_entities);
        }
        game_add_party_member(string_literal("player"));
        game_add_party_member(string_literal("brother"));
        game_state->leader_index       = 0;
    }

    game_state->camera.rng = &game_state->rng;

    {
        {
            /* TODO should add more frame granualarity. Oh well. */
            
            /* He should have EVERY usable generic animation. Use as a fall back if animation is not drawn yet. */
            s32 base_guy = entity_model_database_add_model(&game_arena, string_literal("guy"));
            entity_model_add_animation(base_guy, string_literal("idle_down"),       1, 0);
            entity_model_add_animation(base_guy, string_literal("idle_up"),         1, 0);
            entity_model_add_animation(base_guy, string_literal("idle_left"),       1, 0);
            entity_model_add_animation(base_guy, string_literal("idle_right"),      1, 0);

            const f32 WALK_TIMINGS = 0.13;
            /* TODO: rename animation files to be more consistent */
            entity_model_add_animation(base_guy, string_literal("walk_up"),  3, WALK_TIMINGS);
            entity_model_add_animation(base_guy, string_literal("walk_down"),    3, WALK_TIMINGS);
            entity_model_add_animation(base_guy, string_literal("walk_right"),  3, WALK_TIMINGS);
            entity_model_add_animation(base_guy, string_literal("walk_left"), 3, WALK_TIMINGS);

            entity_model_add_animation(base_guy, string_literal("kneel_down"), 2, 0.4);
            entity_model_add_animation(base_guy, string_literal("dead"),       1, 0);
        }
    }

#ifdef USE_EDITOR
    editor_state                = memory_arena_push(&editor_arena, sizeof(*editor_state));
    editor_initialize(editor_state);
#endif
    initialize_static_table_data();
    initialize_items_database();
    initialize_input_mapper_with_bindings();

    assertion(verify_no_item_id_name_hash_collisions());
}

local void game_clear_party_inventory(void) {
    zero_memory(&game_state->inventory, sizeof(game_state->inventory));
}

bool game_entity_is_party_member(struct entity* entity) {
    for (s32 party_member_index = 0; party_member_index < game_state->party_member_count; ++party_member_index) {
        struct entity* deref = game_dereference_entity(game_state, game_state->party_members[party_member_index]);
        if (deref == entity) {
            return true;
        }
    }

    return false;
}

void game_initialize_game_world(void) {
    game_clear_party_inventory();
    game_kill_all_dynamic_lights();
    game_variables_clear_all_state();
    entity_clear_all_abilities(game_get_player(game_state));
    entity_particle_emitter_kill_all(&game_state->permenant_particle_emitters);
    particle_list_kill_all_particles(&global_particle_list);

    {
        for (s32 party_member_index = 0; party_member_index < game_state->party_member_count; ++party_member_index) {
            struct entity* player = game_dereference_entity(game_state, game_state->party_members[party_member_index]);
            player->health.value = player->health.max;
            player->interacted_script_trigger_write_index = 0;
        }
    }

    /* clear all game variable state */
    {
        
    }


    if (file_exists(string_literal(GAME_DEFAULT_STARTUP_FILE))) {
        _debugprintf("Trying to execute game startup script!");
        struct lisp_list startup_forms = lisp_read_entire_file_into_forms(&scratch_arena, string_literal(GAME_DEFAULT_STARTUP_FILE));

        /* NOTE: none of the code here */
        for (unsigned form_index = 0; form_index < startup_forms.count; ++form_index) {
            struct lisp_form* current_form = startup_forms.forms + form_index;
            game_script_evaluate_form(&scratch_arena, game_state, current_form);
        }
    } else {
        _debugprintf("no startup file!");
    }

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

    /* Possibly dead code path? */
    if (chest->flags & ENTITY_CHEST_FLAGS_REQUIRES_ITEM_FOR_UNLOCK) {
        if (entity_inventory_has_item((struct entity_inventory*)&state->inventory, chest->key_item)) {
            {
                char tmp[512]= {};
                struct item_def* item_base = item_database_find_by_id(chest->key_item);
                snprintf(tmp, 512, "Unlocked using \"%.*s\"", item_base->name.length, item_base->name.data);
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

        u32 chest_index = chest - state->loaded_area.chests;
        chest->flags |= ENTITY_CHEST_FLAGS_UNLOCKED;

        struct lisp_form listener_body = level_area_find_listener_for_object(state, &state->loaded_area, LEVEL_AREA_LISTEN_EVENT_ON_LOOT, GAME_SCRIPT_TARGET_CHEST, chest_index);
        game_script_enqueue_form_to_execute(listener_body);
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

        f32                  message_text_height       = font_cache_calculate_height_of(font, message_str, framebuffer->width * 0.5, 2);
        struct rectangle_f32 message_region            = rectangle_f32_centered(rectangle_f32(0, 0, framebuffer->width, framebuffer->height), framebuffer->width * 0.5, message_text_height*1.15);
        v2f32                estimated_nine_patch_size = nine_patch_estimate_fitting_extents(ui_chunky, 2, message_region.w, message_region.h);

        {
            draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 2, v2f32(message_region.x, message_region.y), (s32)estimated_nine_patch_size.x, (s32)estimated_nine_patch_size.y, UI_DEFAULT_COLOR);
            #if 1
            game_ui_render_rich_text_wrapped(framebuffer, message_str, v2f32(message_region.x+10, message_region.y+10), framebuffer->width * 0.5);
            #else
            /* software_framebuffer_draw_text_bounds(framebuffer, font, 2, v2f32(message_region.x+10, message_region.y+10), framebuffer->width * 0.5, message_str, color32f32(1,1,1,1), BLEND_MODE_ALPHA); */
            #endif
        }

        /* dismiss current message */
        if (is_action_pressed(INPUT_ACTION_CONFIRMATION) || is_action_pressed(INPUT_ACTION_CANCEL)) {
            global_popup_state.message_count -= 1;
        }

        return true;
    }


    return false;
}

void game_display_and_update_message_notifications(struct render_commands* commands, f32 dt) {
    struct camera* camera = &game_state->camera;

    for (s32 index = 0; index < global_message_notification_count; ++index) {
        struct notifier_message* notifier = global_message_notifications + index;

        {
            notifier->position.y -= 50 * dt;
            notifier->timer         += dt;
            notifier->flicker_timer += dt;

            if (notifier->timer >= MESSAGE_NOTIFIER_MAX_TIME) {
                global_message_notifications[index] = global_message_notifications[--global_message_notification_count];
                continue;
            }

            if (notifier->flicker_timer >= MESSAGE_NOTIFIER_FLICKER_TIME) {
                notifier->alternative_color ^= 1;
                notifier->flicker_timer      = 0;
            }
        }

        v2f32 draw_position = notifier->position;

        switch (notifier->type) {
            case NOTIFIER_MESSAGE_STANDARD: {
                struct font_cache* painting_font               = game_get_font(MENU_FONT_COLOR_STEEL);
                if (notifier->alternative_color) painting_font = game_get_font(notifier->message_color);
                    string tmp2 = memory_arena_push_string(&scratch_arena, notifier->text);
                render_commands_push_text(commands, painting_font, 4, draw_position, tmp2, color32f32_WHITE, BLEND_MODE_ALPHA);
            } break;
            case NOTIFIER_MESSAGE_DAMAGE: {
                struct font_cache* painting_font               = game_get_font(MENU_FONT_COLOR_STEEL);
                if (notifier->alternative_color) painting_font = game_get_font(MENU_FONT_COLOR_ORANGE);

                if (notifier->amount == 0) {
                    render_commands_push_text(commands, painting_font, 4, draw_position, string_literal("NO DMG!"), color32f32_WHITE, BLEND_MODE_ALPHA);
                } else {
                    string tmp = format_temp_s("%d!", notifier->amount);
                    string tmp2 = memory_arena_push_string(&scratch_arena, tmp);
                    render_commands_push_text(commands, painting_font, 4, draw_position, tmp2, color32f32_WHITE, BLEND_MODE_ALPHA);
                }
            } break;
            case NOTIFIER_MESSAGE_HEALING: {
                struct font_cache* painting_font               = game_get_font(MENU_FONT_COLOR_STEEL);
                if (notifier->alternative_color) painting_font = game_get_font(MENU_FONT_COLOR_LIME);

                if (notifier->amount == 0) {
                    render_commands_push_text(commands, painting_font, 4, draw_position, string_literal("RESIST?!"), color32f32_WHITE, BLEND_MODE_ALPHA);
                } else {
                    string tmp = format_temp_s("%d!", notifier->amount);
                    string tmp2 = memory_arena_push_string(&scratch_arena, tmp);
                    render_commands_push_text(commands, painting_font, 4, draw_position, tmp2, color32f32_WHITE, BLEND_MODE_ALPHA);
                }
            } break;
                bad_case;
        }
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

        struct lisp_list* list_contents = &code.list;
        for (unsigned index = 1; index < list_contents->count; ++index) {
            struct lisp_form* form = list_contents->forms + index;
            game_script_evaluate_form(&scratch_arena, state, form);
        }
    } else {
        _debugprintf("no dialogue script present");
    }
}
local void dialogue_choice_try_to_execute_script_actions(struct game_state* state, struct conversation_choice* choice, s32 choice_index) {
    if (choice->script_code.length) {
        _debugprintf("dialogue choice script present");
        _debugprintf("choice script-code: %.*s", choice->script_code.length, choice->script_code.data);
        struct lisp_form code = lisp_read_form(&scratch_arena, choice->script_code);
        
        struct lisp_form* relevant_form = code.list.forms + (choice_index+1);
        /* relevant_form = &relevant_form->list.forms[1]; */
        {
            struct lisp_form* do_block = lisp_list_nth(relevant_form, 1);
            _debugprintf("Relevant form");
            _debug_print_out_lisp_code(do_block);
            /* start at 1, skip the *DO* */
            if (!do_block) return;
#if 1
            for (unsigned form_index = 1; form_index < do_block->list.count; ++form_index) {
                struct lisp_form* subform = do_block->list.forms + form_index;

                _debug_print_out_lisp_code(subform);
                bool can_eval = true;
                if (subform->type == LISP_FORM_LIST) {
                    if (lisp_form_symbol_matching(subform->list.forms[0], string_literal("next"))) {
                        _debugprintf("Found a next");
                        s32 offset = 1;

                        if (subform->list.count > 1) {
                            struct lisp_form* action_form_second = &subform->list.forms[1];
                            if (action_form_second->type == LISP_FORM_NUMBER) {
                                if (action_form_second->is_real == false) {
                                    offset += action_form_second->integer;
                                } else {
                                    _debugprintf("found a (next) instruction but bad second arg");
                                }
                            } else {
                                _debugprintf("found a (next) instruction but bad second arg");
                            }
                        }

                        dialogue_ui_set_target_node(state->current_conversation_node_id+offset);
                        can_eval = false;
                    } else if (lisp_form_symbol_matching(subform->list.forms[0], string_literal("bye"))) {
                        _debugprintf("Found a bye");
                        game_finish_conversation(state);
                        can_eval = false;
                    } else if (lisp_form_symbol_matching(subform->list.forms[0], string_literal("goto"))) {
                        _debugprintf("Found a goto");
                        if (subform->list.count > 1) {
                            struct lisp_form* action_form_second = &subform->list.forms[1];

                            if (action_form_second->type == LISP_FORM_NUMBER) {
                                if (action_form_second->is_real == false) {
                                    dialogue_ui_set_target_node(action_form_second->integer);
                                } else {
                                    _debugprintf("found a (next) instruction but bad second arg");
                                }
                            } else {
                                _debugprintf("found a (next) instruction but bad second arg");
                            }
                        } else {
                            _debugprintf("found a (goto) instruction but bad second arg");
                        }
                        can_eval = false;
                    }

                    if (can_eval) {
                        _debugprintf("can eval");
                        game_script_evaluate_form(&scratch_arena, state, subform);
                    }
                }
            }
#endif
        }
    } else {
        _debugprintf("no dialogue choice script present");
    }
}

local void game_setup_death_ui(void);
#include "battle_ui.c"
#include "conversation_ui.c"
#include "shopping_ui.c"

local void game_setup_death_ui(void) {
    if (!global_game_initiated_death_ui) {
        game_state->combat_state.active_combat = false;
        game_focus_camera_to_entity(game_get_player(game_state));
        global_battle_ui_state.phase      = 0;

        {
            do_color_transition_in(color32f32(0,0,0,1), 1.5, 3.12345);
            transition_register_on_finish(_transition_callback_game_over, 0, 0);
        }

        global_game_initiated_death_ui = true;
    }
}

local void update_and_render_ingame_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {

    /* I seem to have a pretty inconsistent UI priority state thing. */
    if (state->shopping) {
        game_display_and_update_shop_ui(framebuffer, dt);
        return;
    }

    if (game_display_and_update_storyboard(framebuffer, dt))
        return;

    if (update_and_render_region_zone_change(state, framebuffer, dt))
        return;

    if (game_display_and_update_messages(framebuffer, dt))
        return;

    if (state->is_conversation_active) {
        update_and_render_conversation_ui(state, framebuffer, dt);
        return;
    }

    if (cutscene_active()) {
        return;
    }

    if (state->interactable_state.interactable_type != INTERACTABLE_TYPE_NONE) {
        struct font_cache* font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_STEEL]);

        switch (state->interactable_state.interactable_type) {
            case INTERACTABLE_TYPE_CHEST: {
                struct entity_chest* chest = state->interactable_state.context;
                software_framebuffer_draw_text_bounds_centered(framebuffer, font, 2, rectangle_f32(0, 400, framebuffer->width, framebuffer->height - 400),
                                                               string_literal("open chest"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);

                if (is_action_pressed(INPUT_ACTION_CONFIRMATION)) {
                    game_loot_chest(state, chest);
                }
            } break;

            case INTERACTABLE_TYPE_ENTITY_CONVERSATION: {
                struct entity* to_speak = state->interactable_state.context;

                software_framebuffer_draw_text_bounds_centered(framebuffer, font, 2, rectangle_f32(0, 400, framebuffer->width, framebuffer->height - 400),
                                                               string_literal("speak"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);

                assertion(entity_has_dialogue(to_speak) && "I'm not sure how this was possible...");
                if (is_action_pressed(INPUT_ACTION_CONFIRMATION)) {
                    string conversation_path = format_temp_s(GAME_DEFAULT_DIALOGUE_PATH "/%s.txt", to_speak->dialogue_file);
                    game_open_conversation_file(state, conversation_path);

                    game_focus_camera_to_entity(to_speak);
                }
            } break;

            case INTERACTABLE_TYPE_ENTITY_SAVEPOINT: {
                struct entity_savepoint* savepoint = state->interactable_state.context;
                software_framebuffer_draw_text_bounds_centered(framebuffer, font, 2, rectangle_f32(0, 400, framebuffer->width, framebuffer->height - 400),
                                                               string_literal("save"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                if (is_action_pressed(INPUT_ACTION_CONFIRMATION)) {
                    game_open_save_menu();
                }
            } break;
                bad_case;
        }
    }

    /* decide playing_music */
    {
        if (game_state->combat_state.active_combat) {
            set_theme_track(THEME_BATTLE_TRACK);
        } else {
            set_theme_track(THEME_SAFE_TRACK);
        }
    }
    game_continue_music(dt);

    /* I'm aware this causes a lot of pop in, which is okay. I can manually code transitions for those... but whatever */
    /* okay, here I can render the normal game UI stuff */
    if (game_state->combat_state.active_combat) {
        update_and_render_battle_ui(state, framebuffer, dt);
    } else {
        /* normal UI which I apparently don't seem to have. */
    }

    /* TODO Region zone change does not prevent pausing right now! */
    {
        if (is_action_pressed(INPUT_ACTION_PAUSE)) {
            if (!(game_get_player(state)->flags & ENTITY_FLAGS_ALIVE)) {
                /* do nothing if dead */
            } else {
                game_state_set_ui_state(state, UI_STATE_PAUSE);
                open_pause_menu();
            }
        }
    }
}

#include "sub_menu_state_inventory.c"
#include "sub_menu_state_equipment.c"

/* NOTE: Might need to overhaul the pause menu... */
#include "pause_menu_ui.c"
#include "game_gameover_ui.c"

void update_and_render_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    /* fader/transitions is rendered under the game ui */
    {
        transitions_update_and_render(state, framebuffer, dt);
    }

    switch (state->ui_state) {
        case UI_STATE_INGAME: {
            update_and_render_ingame_game_menu_ui(state, framebuffer, dt);
        } break;
        case UI_STATE_GAMEOVER: {
            update_and_render_gameover_game_menu_ui(state, framebuffer, dt);
        } break;
        case UI_STATE_PAUSE: {
            update_and_render_pause_game_menu_ui(state, framebuffer, dt);
        } break;
        case UI_STATE_SAVEGAME: {
            update_and_render_save_game_menu_ui(state, framebuffer, dt);
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

local void game_stop_constantly_traumatizing_camera(void) {
    game_state->constantly_traumatizing_camera = false;
    game_state->constant_camera_trauma_value   = 0;
}

local void game_apply_constant_camera_trauma_as(f32 amount) {
    game_state->constantly_traumatizing_camera = true;
    game_state->constant_camera_trauma_value   = amount;
}

local void update_game_camera_exploration_mode(struct game_state* state, f32 dt) {
    struct camera* camera = &state->camera;

    struct entity* player = game_get_player(state);

    /* NOTE/TODO/FINISHLATER hacky death camera */
    /*
      In reality, I want to do more with the camera, but right now I'm just going to leave this hacky code in
      here so I can do stuff later.
    */
    if (!(game_get_player(state)->flags & ENTITY_FLAGS_ALIVE)) {
        camera->xy.y -= dt * 25;
        return;
    }

    if (!cutscene_active()) {
        camera->tracking_xy = player->position;
    }

    /*
      NOTE(jerry):
      Make a separate cutscene camera.
    */
    if (!cutscene_viewing_separate_area()) {
        /* kind of like a project on everythign */
        v2f32                projected_rectangle_position = camera_project(camera, v2f32(camera->travel_bounds.x, camera->travel_bounds.y), SCREEN_WIDTH, SCREEN_HEIGHT);
        struct rectangle_f32 projected_rectangle          = rectangle_f32(projected_rectangle_position.x, projected_rectangle_position.y, camera->travel_bounds.w / camera->zoom, camera->travel_bounds.h / camera->zoom);
        struct rectangle_f32 player_rectangle             = entity_rectangle_collision_bounds(player);
        f32                  new_w                        = projected_rectangle.w * 0.6;
        f32                  new_h                        = projected_rectangle.h * 0.6;
        f32                  delta_w                      = projected_rectangle.w - new_w;
        f32                  delta_h                      = projected_rectangle.w - new_h;

        projected_rectangle.x += delta_w/2;
        projected_rectangle.y += delta_h/2;
        projected_rectangle.w = new_w;
        projected_rectangle.h = new_h;
/* #define OLD_CAMERA_BEHAVIOR */

        bool pushing_left_edge   = (player_rectangle.x < projected_rectangle.x);
        bool pushing_right_edge  = (player_rectangle.x + player_rectangle.w > projected_rectangle.x + projected_rectangle.w);
        bool pushing_top_edge    = (player_rectangle.y < projected_rectangle.y);
        bool pushing_bottom_edge = (player_rectangle.y + player_rectangle.h > projected_rectangle.y + projected_rectangle.h);

#ifdef OLD_CAMERA_BEHAVIOR
        if (!camera->try_interpolation[0]) {
            if (pushing_right_edge || pushing_left_edge) {
                camera->interpolation_t[0] = 0;
                camera->try_interpolation[0] = true;
                camera->start_interpolation_values[0] = camera->xy.x;
            }
        }

        if (!camera->try_interpolation[1]) {
            if (pushing_bottom_edge || pushing_top_edge) {
                camera->interpolation_t[1] = 0;
                camera->try_interpolation[1] = true;
                camera->start_interpolation_values[1] = camera->xy.y;
            }
        }
#else

        if (pushing_right_edge) {
            camera->xy.x += DEFAULT_VELOCITY*dt;
        }

        if (pushing_left_edge) {
            camera->xy.x -= DEFAULT_VELOCITY*dt;
        }


        if (pushing_bottom_edge) {
            camera->xy.y += DEFAULT_VELOCITY*dt;
        }

        if (pushing_top_edge) {
            camera->xy.y -= DEFAULT_VELOCITY*dt;
        }
    }
#endif
}

local void update_game_camera(struct game_state* state, f32 dt) {
    struct camera* camera = &state->camera;

    struct entity* player = game_get_player(state);
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

    if (state->constantly_traumatizing_camera) {
        camera_set_trauma(camera, state->constant_camera_trauma_value);
    }

    {
        if (camera->trauma > 0) {
            camera->trauma -= dt;
        } else {
            camera->trauma = 0;
        }
    }

    if (game_state->combat_state.active_combat) {
        update_game_camera_combat(state, dt);
    } else {
        if (!game_state->is_conversation_active) {
            update_game_camera_exploration_mode(state, dt);
        }
    }

    {
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
}

struct queued_load_level_data {
    bool  use_default_spawn_location;
    v2f32 spawn_location;
    u8    new_facing_direction;
    int   length_of_destination;
    char  destination_string_buffer[256];
};

local void setup_level_common(void) {
    apply_save_data(game_state);
    /* set the stage */
    {
        _debugprintf("setting up stage");
        
        struct entity_iterator iterator = game_entity_iterator(game_state);
        for (struct entity* current_entity = entity_iterator_begin(&iterator);
             !entity_iterator_finished(&iterator);
             current_entity = entity_iterator_advance(&iterator)) {
            /* entities that are already dead, are reported to avoid triggering their on-death trigger since it makes no sense */
            /* and also skip the death animation */
            _debugprintf("ENTITY HP: %d", current_entity->health.value);
            if (current_entity->health.value <= 0 ||
                current_entity->health.max   <= 0 ||
                !(current_entity->flags & ENTITY_FLAGS_ALIVE)) {
                game_push_reported_entity_death(iterator.current_id);
                current_entity->flags                    &= ~(ENTITY_FLAGS_ALIVE);
                current_entity->ai.death_animation_phase  = DEATH_ANIMATION_DIE;
            }
        }
    }
}

local void load_level_from_file_with_setup(struct game_state* state, string where) {
    load_level_from_file(state, where);
    setup_level_common();
}

local void register_all_entities_to_save_record(void) {
    {
        struct entity_iterator iterator = game_entity_iterator(game_state);
        for (struct entity* current_entity = entity_iterator_begin(&iterator);
             !entity_iterator_finished(&iterator);
             current_entity = entity_iterator_advance(&iterator)) {
            save_data_register_entity(iterator.current_id);
        }

        for (u32 chest_index = 0; chest_index < game_state->loaded_area.entity_chest_count; ++chest_index) {
            save_data_register_chest(chest_index);
        }

        for (u32 light_index = 0; light_index < game_state->loaded_area.light_count; ++light_index) {
            save_data_register_light(light_index);
        }

        for (u32 savepoint_index = 0; savepoint_index < game_state->loaded_area.entity_savepoint_count; ++savepoint_index) {
            save_data_register_savepoint(savepoint_index);
        }
    }    
}

void load_level_queued_for_transition(void* callback_data) {
    struct queued_load_level_data* data   = callback_data;

    /* and maybe add friends later */
    struct entity*                 player = game_get_player(game_state);
    string assembled_string = string_from_cstring_length_counted(data->destination_string_buffer, data->length_of_destination);

    /* register entities to the save entry before changing to maintain state persistence. */
    register_all_entities_to_save_record();
    load_level_from_file_with_setup(game_state, assembled_string);

    if (!data->use_default_spawn_location) {
        game_set_all_party_members_to(v2f32(
                                          data->spawn_location.x * TILE_UNIT_SIZE,
                                          data->spawn_location.y * TILE_UNIT_SIZE
                                      ));
    }

    if (data->new_facing_direction != DIRECTION_RETAINED) {
        player->facing_direction = data->new_facing_direction;
    }

    if (!transition_fading()) {
        if (transition_faded_in()) {
            do_color_transition_out(color32f32(0, 0, 0, 1), 0.2, 0.35);
            disable_game_input = false;
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
            string copy = format_temp_s("%s", current_trigger->target_level);

            if (!transition_fading()) {
                if (!transition_faded_in()) {
                    struct queued_load_level_data data = (struct queued_load_level_data) {
                        .use_default_spawn_location = false,
                        .spawn_location             = spawn_location,
                        .new_facing_direction       = new_facing_direction,
                        .length_of_destination      = copy.length,
                    };
                    if (copy.length > array_count(data.destination_string_buffer)) {
                        copy.length = array_count(data.destination_string_buffer);
                    }

                    cstring_copy(copy.data, data.destination_string_buffer, copy.length);

                    do_color_transition_in(color32f32(0, 0, 0, 1), 0.0, 0.45);
                    transition_register_on_finish(load_level_queued_for_transition, (u8*)&data, sizeof(data));
                    disable_game_input = true;
                }
            }

            return;
        }
    }
}

void game_push_reported_entity_death(entity_id id) {
    struct level_area* area                                           = &game_state->loaded_area;
    area->reported_entity_deaths[area->reported_entity_death_count++] = id;
    assertion(area->reported_entity_death_count < MAX_REPORTED_ENTITY_DEATHS && "Too many dead entities? Need to bump the number?");
    return;
}

void game_report_entity_death(entity_id id) {
    struct level_area*          area           = &game_state->loaded_area;
    struct level_area_listener* listener_lists = area->script.listeners + LEVEL_AREA_LISTEN_EVENT_ON_DEATH;

    for (s32 existing_reported_dead_entity_index = 0; existing_reported_dead_entity_index < area->reported_entity_death_count; ++existing_reported_dead_entity_index) {
        if (entity_id_equal(area->reported_entity_deaths[existing_reported_dead_entity_index], id)) {
            _debugprintf("Already known to be dead. Bugger off!");
            return;
        }
    }

    for (s32 subscriber_index = 0; subscriber_index < listener_lists->subscribers; ++subscriber_index) {
        struct lisp_form* event_action_form = listener_lists->subscriber_codes + subscriber_index;
        struct lisp_form* object_handle     = lisp_list_nth(event_action_form, 1);

        struct game_script_typed_ptr ptr   = game_script_object_handle_decode(*object_handle);
        struct entity*               dead  = game_dereference_entity(game_state, ptr.entity_id);

        if (entity_id_equal(ptr.entity_id, id)) {
            struct lisp_form body = lisp_list_sliced(*event_action_form, 2, -1);
            game_script_enqueue_form_to_execute(body);
            break;
        }
    }

    game_push_reported_entity_death(id);
    return;
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
    switch (state->interactable_state.interactable_type) {
        case INTERACTABLE_TYPE_ENTITY_SAVEPOINT: {
            struct entity_savepoint* savepoint = state->interactable_state.context;
            savepoint->player_ontop            = false;
        } break;
        default: {
            
        } break;
    }

    state->interactable_state.interactable_type = 0;
    state->interactable_state.context           = NULL;
}

local void mark_interactable(struct game_state* state, s32 type, void* ptr) {
    switch (type) {
        case INTERACTABLE_TYPE_ENTITY_SAVEPOINT: {
            struct entity_savepoint* savepoint = ptr;
            savepoint->player_ontop            = true;
        } break;
        default: {
            
        } break;
    }

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

    f32 closest_interactive_distance = INFINITY;

    if (found_any_interactable) { return; }
    {
        Array_For_Each(it, struct entity_chest, area->chests, area->entity_chest_count) {
            f32 distance_sq = v2f32_distance_sq(entity->position, v2f32_scale(it->position, TILE_UNIT_SIZE));
            if (it->flags & ENTITY_CHEST_FLAGS_HIDDEN) {
                continue;
            }

            if (it->flags & ENTITY_CHEST_FLAGS_UNLOCKED) {
                continue;   
            }

            if (distance_sq <= (ENTITY_CHEST_INTERACTIVE_RADIUS*ENTITY_CHEST_INTERACTIVE_RADIUS)) {
                if (distance_sq < closest_interactive_distance) {
                    mark_interactable(state, INTERACTABLE_TYPE_CHEST, it);
                    found_any_interactable = true;
                    closest_interactive_distance = distance_sq;
                    break;
                }
            }
        }
    }

    if (found_any_interactable) { return; }
    {
        struct entity_iterator iterator = game_entity_iterator(state);

        for (struct entity* current_entity = entity_iterator_begin(&iterator); !entity_iterator_finished(&iterator); current_entity = entity_iterator_advance(&iterator)) {
            if (!(current_entity->flags & ENTITY_FLAGS_ALIVE)) {
                continue;
            }

            if (current_entity->flags & ENTITY_FLAGS_HIDDEN) {
                continue;
            }

            f32 distance_sq = v2f32_distance_sq(entity->position, current_entity->position);

            if (distance_sq <= (ENTITY_TALK_INTERACTIVE_RADIUS*ENTITY_TALK_INTERACTIVE_RADIUS)) {
                if ((current_entity->flags & ENTITY_FLAGS_ALIVE) && entity_has_dialogue(current_entity)) {
                    if (distance_sq < closest_interactive_distance) {
                        mark_interactable(state, INTERACTABLE_TYPE_ENTITY_CONVERSATION, current_entity);
                        found_any_interactable = true;
                        closest_interactive_distance = distance_sq;
                        break;
                    }
                } 
            }
        }
    }
    if (found_any_interactable) { return; }
    {
        Array_For_Each(it, struct entity_savepoint, area->savepoints, area->entity_savepoint_count) {
            f32 distance_sq = v2f32_distance_sq(entity->position, v2f32_scale(it->position, TILE_UNIT_SIZE));

            if (it->flags & ENTITY_SAVEPOINT_FLAGS_DISABLED) {
                continue;
            }

            if (distance_sq <= (ENTITY_SAVEPOINT_INTERACTIVE_RADIUS*ENTITY_SAVEPOINT_INTERACTIVE_RADIUS)) {
                if (distance_sq < closest_interactive_distance) {
                    mark_interactable(state, INTERACTABLE_TYPE_ENTITY_SAVEPOINT, it);
                    found_any_interactable = true;
                    distance_sq = distance_sq;
                    break;
                }
            }
        }
    }

    if (!found_any_interactable) unmark_any_interactables(state);
}

/* Obviously this can be polished later. */
local void  do_ui_passive_speaking_dialogue(struct render_commands* commands, f32 dt) {
    passive_speaking_dialogue_cleanup();

    /* TODO: I want word wrap here at some point. */
    for (s32 message_index = 0; message_index < passive_speaking_dialogue_count; ++message_index) {
        struct passive_speaking_dialogue* current_dialogue = passive_speaking_dialogues + message_index;

        if (current_dialogue->showed_characters >= current_dialogue->text.length) {
            current_dialogue->time_until_death -= dt;
        } else {
            current_dialogue->character_type_timer += dt;
            if (current_dialogue->character_type_timer >= PASSIVE_SPEAKING_DIALOGUE_TYPING_SPEED) {
                current_dialogue->character_type_timer = 0;
                current_dialogue->showed_characters += 1;
            }
        }

        string display_text = string_slice(current_dialogue->text, 0, current_dialogue->showed_characters);
        /* TODO Need to have breathing text render commands. This would heavily benefit from that */
        /* I mean I can render breathing glyphs one at a time, but that may overflow the command buffer... */
        struct font_cache* font = game_get_font(current_dialogue->game_font_id);

        struct entity* speaker = game_dereference_entity(game_state, current_dialogue->speaker_entity);
        f32 text_width         = font_cache_text_width(font, display_text, 2);
        f32 text_height         = font_cache_text_height(font)*2;
        render_commands_push_text(commands, font, 2, v2f32_sub(speaker->position, v2f32(text_width/2, text_height+speaker->scale.y*2.3)), display_text, color32f32_WHITE, BLEND_MODE_ALPHA);
    }

}

#include "combat.c"

#include "game_demo_alert.c"
#include "save_menu_ui.c"
#include "game_main_menu.c"

local void execute_area_scripts(struct game_state* state, struct level_area* area, f32 dt) {
    struct level_area_script_data* script_data = &area->script;

    if (script_data->present) {
        struct lisp_form* on_frame_script    = script_data->on_frame;

        if (!state->loaded_area.on_enter_triggered) {
            state->loaded_area.on_enter_triggered = true;
            struct lisp_form* on_enter_script     = script_data->on_enter;

            if (on_enter_script) {
                game_script_enqueue_form_to_execute(lisp_list_sliced(*on_enter_script, 1, -1));
            }
        } else {
#if 0
            if (on_frame_script) {
                for (s32 index = 1; index < on_frame_script->list.count; ++index) {
                    struct lisp_form* current_form = lisp_list_nth(on_frame_script, index);
                    game_script_evaluate_form(&scratch_arena, state, current_form);
                }
            }
#endif
        }

        if (!game_state->is_conversation_active) {
            struct level_area_listener* routine_listeners = &script_data->listeners[LEVEL_AREA_LISTEN_EVENT_ROUTINE];
            for (s32 routine_script_index = 0; routine_script_index < routine_listeners->subscribers; ++routine_script_index) {
                struct lisp_form* routine_forms = routine_listeners->subscriber_codes + routine_script_index;
                struct game_script_script_instance* context = game_script_enqueue_form_to_execute_ex(lisp_list_sliced(*routine_forms, 2, -1), routine_script_index);

                if (context) {
                    _debugprintf("pushed new routine script!");
                    game_script_instance_set_contextual_binding(context, CONTEXT_BINDING_SELF, *lisp_list_nth(routine_forms, 1));
                }
            } 
        }
    }
}

local void execute_current_area_scripts(struct game_state* state, f32 dt) {
    if (!cutscene_active()) {
        execute_area_scripts(state, &state->loaded_area, dt);
    } else {
        if (cutscene_viewing_separate_area()) {
            execute_area_scripts(state, cutscene_view_area(), dt);
        }
    }
}

void update_and_render_game_console(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
#if 1
    if (is_key_pressed(KEY_F2)) {
        game_command_console_enabled ^= 1;
    }

    if (game_command_console_enabled) {
        start_text_edit(game_command_console_line_input, cstring_length(game_command_console_line_input));
        /* single line of inputs */
        software_framebuffer_draw_quad(framebuffer, rectangle_f32(0, 0, framebuffer->width, 17), color32u8(0, 0, 25, 128), BLEND_MODE_ALPHA);

        string draw_string = string_from_cstring(game_command_console_line_input);

        if (is_editing_text()) {
            draw_string = string_from_cstring(current_text_buffer());
        }

        software_framebuffer_draw_text(framebuffer, game_get_font(MENU_FONT_COLOR_GOLD), 1, v2f32(0, 5), draw_string, color32f32_WHITE, BLEND_MODE_ALPHA);

        if (is_key_pressed(KEY_RETURN)) {
            end_text_edit(game_command_console_line_input, GAME_COMMAND_CONSOLE_LINE_INPUT_MAX-1);

            struct lisp_form code = lisp_read_form(&scratch_arena, string_from_cstring(game_command_console_line_input));
            /* Since the console doesn't really need to wait ever. We can just evaluate forms straight up like this. */ game_script_evaluate_form(&scratch_arena, state, &code);
            zero_memory(game_command_console_line_input, GAME_COMMAND_CONSOLE_LINE_INPUT_MAX);
        }
    }
#endif
}

void update_and_render_game(struct software_framebuffer* framebuffer, f32 dt) {
    dt *= GLOBAL_GAME_TIMESTEP_MODIFIER;

    if (is_key_pressed(KEY_F12)) {
        image_buffer_write_to_disk((struct image_buffer*)framebuffer, string_literal("scr"));
    }

    /* This is a bit harder to replicate through hardware acceleration... */
    /* if (is_key_pressed(KEY_F10)) { */
    /*     special_effect_start_crossfade_scene(1.5f, 1.2f); */
    /* } */

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
            case GAME_SCREEN_PREVIEW_DEMO_ALERT: {
                update_and_render_preview_demo_alert(game_state, framebuffer, dt);
            } break;
            case GAME_SCREEN_INGAME: {
                struct entity* player_entity = game_get_player(game_state);
                update_game_camera(game_state, dt);

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

                if (game_state->ui_state != UI_STATE_PAUSE) {
                    update_entities(game_state, dt, game_entity_iterator(game_state), &game_state->loaded_area);

                    /*...*/
                    particle_list_update_particles(&global_particle_list, dt);
                    entity_particle_emitter_list_update(&game_state->permenant_particle_emitters, dt);
                    /*...*/

                    game_script_execute_awaiting_scripts(&scratch_arena, game_state, dt);
                    game_script_run_all_timers(dt);

                    if (!cutscene_active() && !game_state->is_conversation_active) {
                        if (!(game_get_player(game_state)->flags & ENTITY_FLAGS_ALIVE)) {
                            game_setup_death_ui();
                        }
                    
                        if (!game_state->combat_state.active_combat) {
                            determine_if_combat_should_begin(game_state);
                        } else {
                            update_combat(game_state, dt);
                        }
                    }

                    global_elapsed_game_time += dt;
                    game_state->weather.timer += dt;
                }

                struct render_commands        commands      = render_commands(&scratch_arena, 16384, game_state->camera);
                struct sortable_draw_entities draw_entities = sortable_draw_entities(&scratch_arena, 8192*4);
                commands.should_clear_buffer = true;
                commands.clear_buffer_color  = color32u8(100, 128, 148, 255);

                if (cutscene_viewing_separate_area()) {
                    /* might need to rethink this a little... */
                    /* TODO: NO PARTICLES HERE */
                    update_entities(game_state, dt, game_cutscene_entity_iterator(), cutscene_view_area());
                    render_ground_area(game_state, &commands, cutscene_view_area());
                    render_cutscene_entities(&draw_entities);
                    sortable_draw_entities_submit(&commands, &graphics_assets, &draw_entities, dt);
                    render_foreground_area(game_state, &commands, cutscene_view_area());
                } else {
                    render_ground_area(game_state, &commands, &game_state->loaded_area);
                    render_entities(game_state, &draw_entities);
                    render_particles_list(&global_particle_list, &draw_entities);
                    sortable_draw_entities_submit(&commands, &graphics_assets, &draw_entities, dt);
                    render_foreground_area(game_state, &commands, &game_state->loaded_area);
#if 0 
                    DEBUG_render_particle_emitters(&commands, &game_state->permenant_particle_emitters);
#endif
                }

                software_framebuffer_render_commands(framebuffer, &commands);
                {
                    struct level_area* area = &game_state->loaded_area;
                    if (cutscene_viewing_separate_area()) {
                        area = cutscene_view_area();
                    }
                    software_framebuffer_run_shader(framebuffer, rectangle_f32(0, 0, framebuffer->width, framebuffer->height), lighting_shader, area);
                }
                game_postprocess_blur_ingame(framebuffer, 2, 0.63, BLEND_MODE_ALPHA);
                /* game_postprocess_blur_ingame(framebuffer, 1, 0.65, BLEND_MODE_ALPHA); */

                {
                    struct render_commands commands = render_commands(&scratch_arena, 1024, game_state->camera);
                    do_ui_passive_speaking_dialogue(&commands, dt);
                    game_display_and_update_message_notifications(&commands, dt);
                    software_framebuffer_render_commands(framebuffer, &commands);
                }

                game_do_special_effects(framebuffer, dt);

                do_weather(framebuffer, game_state, dt);
                update_and_render_game_menu_ui(game_state, framebuffer, dt);
                update_and_render_game_console(game_state, framebuffer, dt);
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

struct game_variables game_variables(struct memory_arena* arena) {
    struct game_variables result = {};
    result.first       = result.last = NULL;
    result.arena       = arena;
    return result;
}

s32 game_variables_count_all(void) {
    s32 result = 0;

    struct game_variable_chunk* current = game_state->variables.first;
    while (current) {
        result  += current->variable_count;
        current  = current->next;
    }

    return result;
}

local struct game_variable* game_variables_allocate_next(void) {
    struct game_variables* variables = &game_state->variables;
    struct game_variable* result = NULL;

    /* first doesn't exist? */
    if (!variables->last) {
        struct game_variable_chunk* initial_chunk = memory_arena_push(variables->arena, sizeof(*variables->first));
        initial_chunk->variable_count = 0;
        initial_chunk->next           = NULL;
        variables->first = variables->last = initial_chunk;
    }

    /* overflowing current count? */
    {
        struct game_variable_chunk* chunk_with_avaliable_slots = variables->first;

        while (chunk_with_avaliable_slots) {
            if (chunk_with_avaliable_slots->variable_count < MAX_GAME_VARIABLES_PER_CHUNK) {
                return &chunk_with_avaliable_slots->variables[chunk_with_avaliable_slots->variable_count++];
            } else {
                chunk_with_avaliable_slots = chunk_with_avaliable_slots->next;
            }
        }
    }

    {
        if (variables->last->variable_count >= MAX_GAME_VARIABLES_PER_CHUNK) {
            struct game_variable_chunk* next_chunk = memory_arena_push(variables->arena, sizeof(*variables->first));
            next_chunk->next                       = NULL;
            next_chunk->variable_count             = 0;
            variables->last->next                  = next_chunk;
            variables->last                        = next_chunk;
        }
    }

    struct game_variable_chunk* current_chunk = variables->last;
    result                                    = &current_chunk->variables[current_chunk->variable_count++];

    return result;
}

struct game_variable* lookup_game_variable(string name, bool create_when_not_found) {
    struct game_variables* variables = &game_state->variables;

    {
        struct game_variable_chunk* cursor = variables->first;

        while (cursor) {
            for (s32 variable_index = 0; variable_index < cursor->variable_count; ++variable_index) {
                struct game_variable* variable = cursor->variables + variable_index;

                _debugprintf("Trying to match %.*s vs %s", name.length, name.data, variable->name);
                if (string_equal(name, string_from_cstring(variable->name))) {
                    return variable;
                }
            }

            cursor = cursor->next;
        }
    }

    if (create_when_not_found) {
        _debugprintf("Creating new game var: \"%.*s\"", name.length, name.data);
        struct game_variable* new_variable = game_variables_allocate_next();;
        cstring_copy(name.data, new_variable->name, array_count(new_variable->name));
        new_variable->name[name.length] = 0;
        new_variable->value             = 0;
        return new_variable;
    }
    return NULL;
}

void game_variable_set(string name, s32 value) {
    struct game_variable* to_set = lookup_game_variable(name, true);
    to_set->value = value;
}

s32 game_variable_get(string name) {
    struct game_variable* to_set = lookup_game_variable(name, true);
    return to_set->value;
}

void game_variables_clear_all_state(void) {
    struct game_variable_chunk* cursor = game_state->variables.first;

    while (cursor) {
        cursor->variable_count = 0;
        cursor = cursor->next;
    }
}

struct entity* game_any_entity_at_tile_point_except(v2f32 xy, struct entity** filter, s32 filter_count) {
    struct entity_iterator iterator = game_entity_iterator(game_state);

    for (struct entity* current_entity = entity_iterator_begin(&iterator); !entity_iterator_finished(&iterator); current_entity = entity_iterator_advance(&iterator)) {
        v2f32 position = current_entity->position;
        position.x /= TILE_UNIT_SIZE;
        position.y /= TILE_UNIT_SIZE;

        position.x = roundf(position.x);
        position.y = roundf(position.y);

        if (!(current_entity->flags & ENTITY_FLAGS_ALIVE)) {
            continue;
        }

        if (current_entity->flags & ENTITY_FLAGS_HIDDEN) {
            continue;
        }
        
        if (fabs(position.x - xy.x) <= 0.15 &&
            fabs(position.y - xy.y) <= 0.15) {
            bool passed_filter = true;

            for (s32 filtered_entity_index = 0; filtered_entity_index < filter_count; ++filtered_entity_index) {
                if (filter[filtered_entity_index] == current_entity) {
                    passed_filter = false;
                }
            }

            if (passed_filter) {
                return current_entity;
            } else {
                continue;
            }
        }
    }

    return NULL;
}

struct entity* game_any_entity_at_tile_point(v2f32 xy) {
    return game_any_entity_at_tile_point_except(xy, 0, 0);
}

local void update_and_render_save_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    struct ui_save_menu* ui_state = &state->ui_save;
    u32 blur_samples = 2;
    f32 max_blur = 1.0;
    f32 max_grayscale = 0.8;

    switch (ui_state->phase) {
        case UI_SAVE_MENU_PHASE_FADEIN: {
            const f32 MAX_T = 0.7;
            f32 effective_t = ui_state->effects_timer / MAX_T;

            if (effective_t > 1)      effective_t = 1;
            else if (effective_t < 0) effective_t = 0;

            game_postprocess_blur(framebuffer, blur_samples, max_blur * (effective_t), BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale * (effective_t));

            if (ui_state->effects_timer >= MAX_T) {
                ui_state->phase         = UI_SAVE_MENU_PHASE_IDLE;
                ui_state->effects_timer = 0;
                save_menu_open_for_saving();
            }

            ui_state->effects_timer += dt;
            disable_game_input = true;
        } break;

        case UI_SAVE_MENU_PHASE_IDLE: {
            game_postprocess_blur(framebuffer, blur_samples, max_blur, BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale);

            s32 save_menu_result = do_save_menu(framebuffer, dt);

            switch (save_menu_result) {
                case SAVE_MENU_PROCESS_ID_SAVED_EXIT: {
                    ui_state->phase = UI_SAVE_MENU_PHASE_FADEOUT;
                    ui_state->effects_timer = 0;
                } break;
                case SAVE_MENU_PROCESS_ID_LOADED_EXIT: {
                    ui_state->phase = UI_SAVE_MENU_PHASE_FADEOUT;
                    ui_state->effects_timer = 0;
                } break;
                case SAVE_MENU_PROCESS_ID_EXIT: {
                    ui_state->phase = UI_SAVE_MENU_PHASE_FADEOUT;
                    ui_state->effects_timer = 0;
                } break;
            }
            disable_game_input = true;
        } break;

        case UI_SAVE_MENU_PHASE_FADEOUT: {
            const f32 MAX_T = 0.7;
            f32 effective_t = ui_state->effects_timer / MAX_T;

            if (effective_t > 1)      effective_t = 1;
            else if (effective_t < 0) effective_t = 0;

            game_postprocess_blur(framebuffer, blur_samples, max_blur * (1-effective_t), BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale * (1-effective_t));

            if (ui_state->effects_timer >= MAX_T) {
                game_state_set_ui_state(game_state, state->last_ui_state);
                disable_game_input = false;
            }

            ui_state->effects_timer += dt;
        } break;
    }

    return;
}

local void game_continue_music(f32 dt) {
    if (global_game_initiated_death_ui) {
        stop_music();
        return;
    }

    if (game_state->music_fadeout_timer > 0) {
        if (!game_state->started_fading_out) {
            game_state->started_fading_out = true;
            stop_music_fadeout(FADEOUT_TIME);
        }

        game_state->music_fadeout_timer -= dt;
    } else if (game_state->music_fadein_timer > 0) {
        if (!game_state->started_fading_in) {
            game_state->started_fading_in = true;
            play_sound_fadein(get_current_sound_theme(), FADEIN_TIME);
        }

        game_state->music_fadein_timer -= dt;
    } else {
        if (!music_playing()) {
            play_sound(get_current_sound_theme());
        }
    }
}

#include "game_script.c"
#include "item_data.c"
#include "save_data.c"
#include "storyboard_presentation.c"
#include "entity_model.c"
#include "shop.c"
#include "cutscene.c"
#include "special_effects.c"
#include "rich_text.c"
