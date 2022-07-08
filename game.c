/* virtual pixels */
#define TILE_UNIT_SIZE (64)

static struct memory_arena game_arena   = {};
/* compile out */
static struct memory_arena editor_arena = {};

image_id test_image;

int DEBUG_tilemap[6][6] = {
    1,1,1,1,1,1,
    1,0,1,0,1,1,
    1,0,0,0,0,1,
    1,0,1,0,1,1,
    1,0,1,0,0,1,
    1,1,1,1,1,1,
};

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

image_id brick_img;
image_id grass_img;
image_id guy_img;
image_id selection_sword_img;

void DEBUG_render_tilemap(struct render_commands* commands, int* tilemap, s32 w, s32 h) {
    for (s32 y =0; y < h; ++y) {
        for (s32 x = 0; x < w; ++x) {
            image_id tex = brick_img;
            if (tilemap[y * w + x] == 0) tex = grass_img;
            render_commands_push_image(commands,
                                       graphics_assets_get_image_by_id(&graphics_assets, tex),
                                       rectangle_f32(x * TILE_UNIT_SIZE,
                                                     y * TILE_UNIT_SIZE,
                                                     TILE_UNIT_SIZE,
                                                     TILE_UNIT_SIZE),
                                       RECTANGLE_F32_NULL, color32f32(1,1,1,1), NO_FLAGS, BLEND_MODE_ALPHA);

        }
    }
}

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
struct level_transition_trigger {
    char  target_level[128];
    /* anchoring to an object, might be very niche... */
    u8    use_default;
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
struct tile_data_definition* tile_table_data;
struct autotile_table*       auto_tile_info;
    /* /\* grass-test *\/ */
    /* (struct tile_data_definition){ */
    /*     .name                 = ("(grass test(filled))"), */
    /*     .image_asset_location = ("./res/img/grass.png"), */
    /*     .flags                = TILE_DATA_FLAGS_NONE, */
    /*     /\* .neighbor_ids         = {}, *\/ */
    /* }, */
    /* (struct tile_data_definition){ */
    /*     .name                 = ("(brick test(filled))"), */
    /*     .image_asset_location = ("./res/img/brick.png"), */
    /*     .flags                = TILE_DATA_FLAGS_SOLID, */
    /*     /\* .neighbor_ids         = {}, *\/ */
    /* }, */
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

void render_area(struct render_commands* commands, struct level_area* area) {
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
}

#include "entity.c"
entity_id player_id;
entity_id entity_list_create_player(struct entity_list* entities, v2f32 position) {
    entity_id result = entity_list_create_entity(entities);
    struct entity* player = entity_list_dereference_entity(entities, result);

    assertion(player->flags & ENTITY_FLAGS_ACTIVE);
    player->flags    |= ENTITY_FLAGS_ALIVE;
    player->flags    |= ENTITY_FLAGS_PLAYER_CONTROLLED;
    player->position  = position;
    player->scale.x = TILE_UNIT_SIZE-2;
    player->scale.y = TILE_UNIT_SIZE-2;

    return result;
}

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
    s32           tool_mode;

    s32           painting_tile_id;
    s32           trigger_placement_type;

    s32           tile_count;
    s32           tile_capacity;
    struct tile*  tiles;
    struct camera camera;

    v2f32 default_player_spawn;
};

#include "weather_def.c"

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

#include "weather.c"

void serialize_level_area(struct game_state* state, struct binary_serializer* serializer, struct level_area* level) {
    serialize_u32(serializer, &level->version);
    serialize_f32(serializer, &level->default_player_spawn.x);
    serialize_f32(serializer, &level->default_player_spawn.y);
    Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, state->arena, s32, level->tile_count, level->tiles);

    /* until we have new area transititons or whatever. */
    struct entity* player = entity_list_dereference_entity(&state->entities, player_id);
    player->position.x             = level->default_player_spawn.x;
    player->position.y             = level->default_player_spawn.y;
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
            .image_asset_location = string_literal("./res/img/grass.png"),
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
#undef insert 
#undef AT_insert 
#undef current_AT 
}

void game_initialize(void) {
    game_arena   = memory_arena_create_from_heap("Game Memory", Megabyte(16));
    editor_arena = memory_arena_create_from_heap("Editor Memory", Megabyte(32));

    game_state = memory_arena_push(&game_arena, sizeof(*game_state));
    game_state->rng = random_state();
    game_state->arena = &game_arena;

    graphics_assets = graphics_assets_create(&game_arena, 64, 512);
    scratch_arena = memory_arena_create_from_heap("Scratch Buffer", Megabyte(16));

    test_image          = graphics_assets_load_image(&graphics_assets, string_literal("./res/a.png"));
    brick_img           = graphics_assets_load_image(&graphics_assets, string_literal("./res/img/brick.png"));
    grass_img           = graphics_assets_load_image(&graphics_assets, string_literal("./res/img/grass.png"));
    guy_img             = graphics_assets_load_image(&graphics_assets, string_literal("./res/img/guy2.png"));
    selection_sword_img = graphics_assets_load_image(&graphics_assets, string_literal("./res/img/selection_sword.png"));

    for (unsigned index = 0; index < array_count(menu_font_variation_string_names); ++index) {
        string current = menu_font_variation_string_names[index];
        menu_fonts[index] = graphics_assets_load_bitmap_font(&graphics_assets, current, 5, 12, 5, 20);
    }

    game_state->entities = entity_list_create(&game_arena, 1024);
    player_id = entity_list_create_player(&game_state->entities, v2f32(70, 70));

    editor_state                = memory_arena_push(&editor_arena, sizeof(*editor_state));
    editor_initialize(editor_state);
    initialize_static_table_data();
}

void game_deinitialize(void) {
    graphics_assets_finish(&graphics_assets);
    memory_arena_finish(&editor_arena);
    memory_arena_finish(&game_arena);
    memory_arena_finish(&scratch_arena);
}

void game_postprocess_blur(struct software_framebuffer* framebuffer, s32 quality_scale, f32 t, u32 blend_mode) {
    f32 box_blur[] = {
        1,1,1,
        1,1,1,
        1,1,1,
    };

    struct software_framebuffer blur_buffer = software_framebuffer_create(&scratch_arena, framebuffer->width/quality_scale, framebuffer->height/quality_scale);
    software_framebuffer_copy_into(&blur_buffer, framebuffer);
    software_framebuffer_kernel_convolution_ex(&scratch_arena, &blur_buffer, box_blur, 3, 3, 12, t, 2);
    software_framebuffer_draw_image_ex(framebuffer, &blur_buffer, RECTANGLE_F32_NULL, RECTANGLE_F32_NULL, color32f32(1,1,1,1), NO_FLAGS, blend_mode);
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

local void update_and_render_ingame_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    if (is_key_pressed(KEY_ESCAPE)) {
        game_state_set_ui_state(game_state, UI_STATE_PAUSE);
        /* ready pause menu */
        {
            game_state->ui_pause.animation_state = 0;
            game_state->ui_pause.transition_t    = 0;
            game_state->ui_pause.selection       = 0;
            zero_array(game_state->ui_pause.shift_t);
        }
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

            game_postprocess_blur(framebuffer, blur_samples, max_blur * menu_state->transition_t, BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale * menu_state->transition_t);

            if (menu_state->transition_t >= 1.0f) {
                menu_state->animation_state += 1;
                menu_state->transition_t = 0;
            }
        } break;
        case UI_PAUSE_MENU_NO_ANIM: {
            for (unsigned index = 0; index < array_count(item_positions); ++index) {
                item_positions[index].x = final_x;
            }

            game_postprocess_blur(framebuffer, blur_samples, max_blur, BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale);

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

            if (is_key_down_with_repeat(KEY_DOWN)) {
                menu_state->selection++;
                if (menu_state->selection >= array_count(item_positions)) menu_state->selection = 0;
            }
            if (is_key_down_with_repeat(KEY_UP)) {
                menu_state->selection--;
                if (menu_state->selection < 0) menu_state->selection = array_count(item_positions)-1;
            }

            if (is_key_pressed(KEY_RETURN)) {
                switch (menu_state->selection) {
                    case 0: {
                        menu_state->animation_state = UI_PAUSE_MENU_TRANSITION_CLOSING;
                        menu_state->transition_t = 0;
                    } break;
                    case 1: {
                        
                    } break;
                    case 2: {
                        
                    } break;
                    case 3: {
                        global_game_running = false;
                    } break;
                }
            }
        } break;
        case UI_PAUSE_MENU_TRANSITION_CLOSING: {
            menu_state->transition_t   += dt * timescale;

            for (unsigned index = 0; index < array_count(item_positions); ++index) {
                item_positions[index].x = lerp_f32(final_x, offscreen_x, menu_state->transition_t);
            }

            game_postprocess_blur(framebuffer, blur_samples, max_blur * (1-menu_state->transition_t), BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale * (1-menu_state->transition_t));

            if (menu_state->transition_t >= 1.0f) {
                menu_state->transition_t = 0;
                game_state_set_ui_state(game_state, state->last_ui_state);
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
/* copied and pasted for now */
/* This can be compressed, quite easily... However I won't deduplicate this yet, as I've yet to experiment fully with the UI so let's keep it like this for now. */
local void update_and_render_pause_editor_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    /* needs a bit of cleanup */
    f32 font_scale = 3;
    /* While the real pause menu is going to be replaced with something else later obviously */
    /* I want a nice looking menu to show off, and also the main menu is likely taking this design */
    struct ui_pause_menu* menu_state = &state->ui_pause;
    v2f32 item_positions[array_count(ui_pause_editor_menu_strings)] = {};

    for (unsigned index = 0; index < array_count(item_positions); ++index) {
        item_positions[index].y = 36 * (index+0.75);
    }

    f32 offscreen_x = -240;
    f32 final_x     = 40;

    u32 blur_samples = 4;
    f32 max_blur = 1.0;
    f32 max_grayscale = 0.8;

    f32 timescale = 1.34f;

    switch (menu_state->animation_state) {
        case UI_PAUSE_MENU_TRANSITION_IN: {
            menu_state->transition_t   += dt * timescale;

            for (unsigned index = 0; index < array_count(item_positions); ++index) {
                item_positions[index].x = lerp_f32(offscreen_x, final_x, menu_state->transition_t);
            }

            game_postprocess_blur(framebuffer, blur_samples, max_blur * menu_state->transition_t, BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale * menu_state->transition_t);

            if (menu_state->transition_t >= 1.0f) {
                menu_state->animation_state += 1;
                menu_state->transition_t = 0;
            }
        } break;
        case UI_PAUSE_MENU_NO_ANIM: {
            for (unsigned index = 0; index < array_count(item_positions); ++index) {
                item_positions[index].x = final_x;
            }

            game_postprocess_blur(framebuffer, blur_samples, max_blur, BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale);

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

            if (is_key_down_with_repeat(KEY_DOWN)) {
                menu_state->selection++;
                if (menu_state->selection >= array_count(item_positions)) menu_state->selection = 0;
            }
            if (is_key_down_with_repeat(KEY_UP)) {
                menu_state->selection--;
                if (menu_state->selection < 0) menu_state->selection = array_count(item_positions)-1;
            }

            if (is_key_pressed(KEY_RETURN)) {
                switch (menu_state->selection) {
                    case 0: {
                        menu_state->animation_state = UI_PAUSE_MENU_TRANSITION_CLOSING;
                        menu_state->transition_t = 0;
                    } break;
                    case 1: {
                        /* who knows about this one */ 
                        u8* data;
                        u64 amount;

                        struct binary_serializer serializer = open_write_memory_serializer();
                        editor_serialize_area(&serializer);
                        data = serializer_flatten_memory(&serializer, &amount);
                        struct binary_serializer serializer1 = open_read_memory_serializer(data, amount);
                        serialize_level_area(game_state, &serializer1, &game_state->loaded_area);
                        serializer_finish(&serializer1);
                        serializer_finish(&serializer);
                        system_heap_memory_deallocate(data);

                        game_state->in_editor = false;
                    } break;
                    case 2: {
                        struct binary_serializer serializer = open_write_file_serializer(string_literal("edit.area"));
                        editor_serialize_area(&serializer);
                        serializer_finish(&serializer);
                    } break;
                    case 3: {
                        struct binary_serializer serializer = open_read_file_serializer(string_literal("edit.area"));
                        editor_serialize_area(&serializer);
                        serializer_finish(&serializer);
                    } break;
                    case 4: {
                    } break;
                    case 5: {
                        global_game_running = false;
                    } break;
                }
            }
        } break;
        case UI_PAUSE_MENU_TRANSITION_CLOSING: {
            menu_state->transition_t   += dt * timescale;

            for (unsigned index = 0; index < array_count(item_positions); ++index) {
                item_positions[index].x = lerp_f32(final_x, offscreen_x, menu_state->transition_t);
            }

            game_postprocess_blur(framebuffer, blur_samples, max_blur * (1-menu_state->transition_t), BLEND_MODE_ALPHA);
            game_postprocess_grayscale(framebuffer, max_grayscale * (1-menu_state->transition_t));

            if (menu_state->transition_t >= 1.0f) {
                menu_state->transition_t = 0;
                game_state_set_ui_state(game_state, state->last_ui_state);
            }
        } break;
    }
    
    {
        for (unsigned index = 0; index < array_count(item_positions); ++index) {
            v2f32 draw_position = item_positions[index];
            draw_position.x += lerp_f32(0, 20, menu_state->shift_t[index]);
            draw_position.y += 220;
            /* custom string drawing routine */
            struct font_cache* font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_STEEL]);
            if (index == menu_state->selection) {
                font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]);
            }

            for (unsigned character_index = 0; character_index < ui_pause_editor_menu_strings[index].length; ++character_index) {
                f32 character_displacement_y = sinf((global_elapsed_time*2) + ((character_index+index) * 2381.2318)) * 3;

                v2f32 glyph_position = draw_position;
                glyph_position.y += character_displacement_y;
                glyph_position.x += font->tile_width * font_scale * character_index;

                software_framebuffer_draw_text(framebuffer, font, font_scale, glyph_position, string_slice(ui_pause_editor_menu_strings[index], character_index, character_index+1), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
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

void update_and_render_game(struct software_framebuffer* framebuffer, f32 dt) {
    if (is_key_pressed(KEY_F12)) {
        image_buffer_write_to_disk(framebuffer, string_literal("scr"));
    }

    if (game_state->in_editor) {
        update_and_render_editor(framebuffer, dt);
    } else {
        struct entity* player_entity = entity_list_dereference_entity(&game_state->entities, player_id);
        struct render_commands commands = render_commands(camera_centered(player_entity->position, 1));
        commands.should_clear_buffer = true;
        commands.clear_buffer_color  = color32u8(0, 0, 0, 255);

        /* DEBUG_render_tilemap(&commands, DEBUG_tilemap, 6, 6); */
        render_area(&commands, &game_state->loaded_area);
        if (game_state->ui_state != UI_STATE_PAUSE) {
            entity_list_update_entities(&game_state->entities, dt, &game_state->loaded_area);
            game_state->weather.timer += dt;
        }

        entity_list_render_entities(&game_state->entities, &graphics_assets, &commands, dt);
        software_framebuffer_render_commands(framebuffer, &commands);

        /* Rendering weather atop */
        /* NOTE
           While I would like more advanced weather effects, let's not bite off more than
           I can chew so I can actually have something to play...
           
        */
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

        {
            if (Get_Bit(game_state->weather.features, WEATHER_RAIN)) {
                /* 
                   TODO
                   falling lines
                */
                weather_render_rain(game_state, framebuffer, dt);
            }
            if (Get_Bit(game_state->weather.features, WEATHER_SNOW)) {
                /* 
                   TODO
                   falling pixels, that sprinkle.
                */
                weather_render_snow(game_state, framebuffer, dt);
            }
            if (Get_Bit(game_state->weather.features, WEATHER_FOGGY)) {
                /*           
                  TODO
                  Fog, Too lazy to do stuff for that right now, but I suspect lots of gradients may help
                */       
            }
            if (Get_Bit(game_state->weather.features, WEATHER_STORMY)) {
                
                /* 
                   TODO
                   requires sound
                */
            }
        }
    }
    update_and_render_game_menu_ui(game_state, framebuffer, dt);
}
