/* virtual pixels */
#define TILE_UNIT_SIZE (32)

#include "game_def.c"

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

struct tile_data_definition* tile_table_data;
struct autotile_table*       auto_tile_info;

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
    player->position  = position;
    player->scale.x = TILE_UNIT_SIZE-2;
    player->scale.y = TILE_UNIT_SIZE-2;

    return result;
}


#include "weather.c"

void serialize_level_area(struct game_state* state, struct binary_serializer* serializer, struct level_area* level, bool use_default_spawn) {
    serialize_u32(serializer, &level->version);
    serialize_f32(serializer, &level->default_player_spawn.x);
    serialize_f32(serializer, &level->default_player_spawn.y);
    Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, state->arena, s32, level->tile_count, level->tiles);

    if (level->version >= 1) {
        Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, state->arena, s32, level->trigger_level_transition_count, level->trigger_level_transitions);
        /* this thing is allergic to chest updates. Unfortunately it might happen a lot... */
        if (level->version >= 2) {
            Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, state->arena, s32, level->entity_chest_count, level->chests);
        }
    }

    /* until we have new area transititons or whatever. */
    /* TODO dumb to assume only the player... but okay */
    if (use_default_spawn) {
        struct entity* player = entity_list_dereference_entity(&state->entities, player_id);
        player->position.x             = level->default_player_spawn.x;
        player->position.y             = level->default_player_spawn.y;
    }
}

/* this is used for cheating or to setup the game I suppose. */
void load_level_from_file(struct game_state* state, string filename) {
    struct binary_serializer serializer = open_read_file_serializer(string_concatenate(state->arena, string_literal("areas/"), filename));
    serialize_level_area(state, &serializer, &state->loaded_area, false);
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
    tile_table_data_count = i;
}

void game_initialize(void) {
    game_arena   = memory_arena_create_from_heap("Game Memory", Megabyte(32));
    editor_arena = memory_arena_create_from_heap("Editor Memory", Megabyte(32));

    game_state = memory_arena_push(&game_arena, sizeof(*game_state));
    game_state->rng = random_state();
    game_state->arena = &game_arena;

    graphics_assets = graphics_assets_create(&game_arena, 64, 512);
    scratch_arena = memory_arena_create_from_heap("Scratch Buffer", Megabyte(16));

    guy_img               = graphics_assets_load_image(&graphics_assets, string_literal("./res/img/guy3.png"));
    chest_closed_img      = graphics_assets_load_image(&graphics_assets, string_literal("./res/img/chestclosed.png"));
    chest_open_bottom_img = graphics_assets_load_image(&graphics_assets, string_literal("./res/img/chestopenbottom.png"));
    chest_open_top_img    = graphics_assets_load_image(&graphics_assets, string_literal("./res/img/chestopentop.png"));
    selection_sword_img   = graphics_assets_load_image(&graphics_assets, string_literal("./res/img/selection_sword.png"));
    graphics_assets_load_image(&graphics_assets, string_literal("./res/img/brick.png"));
    graphics_assets_load_image(&graphics_assets, string_literal("./res/img/grass.png"));

    for (unsigned index = 0; index < array_count(menu_font_variation_string_names); ++index) {
        string current = menu_font_variation_string_names[index];
        menu_fonts[index] = graphics_assets_load_bitmap_font(&graphics_assets, current, 5, 12, 5, 20);
    }

    game_state->entities = entity_list_create(&game_arena, 16384);
    player_id = entity_list_create_player(&game_state->entities, v2f32(70, 70));

    editor_state                = memory_arena_push(&editor_arena, sizeof(*editor_state));
    editor_initialize(editor_state);
    initialize_static_table_data();
    initialize_items_database();
    assertion(verify_no_item_id_name_hash_collisions());

    entity_inventory_add((struct entity_inventory*)&game_state->inventory, MAX_PARTY_ITEMS, item_id_make(string_literal("item_trout_fish_5")));
    entity_inventory_add((struct entity_inventory*)&game_state->inventory, MAX_PARTY_ITEMS, item_id_make(string_literal("item_trout_fish_5")));
    entity_inventory_add((struct entity_inventory*)&game_state->inventory, MAX_PARTY_ITEMS, item_id_make(string_literal("item_trout_fish_5")));
    entity_inventory_add((struct entity_inventory*)&game_state->inventory, MAX_PARTY_ITEMS, item_id_make(string_literal("item_sardine_fish_5")));

#if 0
    {
        struct conversation s = {};
        {
            struct conversation_node* a = conversation_push_node(&s, string_literal("Frimor"), string_literal("The stars are certainly blessed tonight..."), 2);
            struct conversation_node* b = conversation_push_node(&s, string_literal("Frimor"), string_literal("Aren't they? Calamir."), 0);
            struct conversation_node* c = conversation_push_node(&s, string_literal("Frimor"), string_literal("There's something special tonight isn't there?"), 5);
            struct conversation_node* d = conversation_push_node(&s, string_literal("Frimor"), string_literal("Heh, maybe it is. I don't spend as much time as you do here."), 5);
            struct conversation_node* e = conversation_push_node(&s, string_literal("Frimor"), string_literal("How have your days been anyways?"), 6);
            struct conversation_node* f = conversation_push_node(&s, string_literal("Frimor"), string_literal("Actually... Nevermind, I didn't mean to disturb you."), 7);
            struct conversation_node* g = conversation_push_node(&s, string_literal("Frimor"), string_literal("I'll be going."), 0);
            conversation_push_choice(b, string_literal("Perhaps."), 3);
            conversation_push_choice(b, string_literal("Same as any other."), 4);
        }
        game_state->is_conversation_active       = true;
        game_state->current_conversation         = s;
        game_state->current_conversation_node_id = 1;
    }
#endif
    load_level_from_file(game_state, string_literal("1.area"));
}

void game_deinitialize(void) {
    graphics_assets_finish(&graphics_assets);
    memory_arena_finish(&editor_arena);
    memory_arena_finish(&game_arena);
    memory_arena_finish(&scratch_arena);
}

local void game_loot_chest(struct game_state* state, struct entity_chest* chest) {
    struct entity* player = entity_list_dereference_entity(&state->entities, player_id);
    
    Array_For_Each(it, struct item_instance, chest->inventory.items, chest->inventory.item_count) {
        entity_inventory_add_multiple(&state->inventory, MAX_PARTY_ITEMS, it->item, it->count);
        {
            char tmp[512]= {};
            struct item_def* item_base = item_database_find_by_id(it->item);
            snprintf(tmp, 512, "acquired %.*s x%d", item_base->name.length, item_base->name.data, it->count);
            game_message_queue(string_from_cstring(tmp));
        }
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

local void update_and_render_ingame_game_menu_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    if (game_display_and_update_messages(framebuffer, dt))
        return;

    if (state->is_conversation_active) {
        /* TODO no layout right now, do that later or tomorrow as warmup */
        /* TODO animation! */

        struct font_cache* font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_YELLOW]);
        struct font_cache* font2 = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]);
        struct conversation* conversation = &game_state->current_conversation;
        struct conversation_node* current_conversation_node = &conversation->nodes[game_state->current_conversation_node_id-1];

        software_framebuffer_draw_quad(framebuffer, rectangle_f32(50, 480-180, 200, 30), color32u8(90, 30, 255, 255), BLEND_MODE_ALPHA);
        software_framebuffer_draw_quad(framebuffer, rectangle_f32(50, 480-150, 640-100, 130), color32u8(90, 30, 255, 255), BLEND_MODE_ALPHA);
        software_framebuffer_draw_text(framebuffer, font, 2, v2f32(60, 480-150+10), current_conversation_node->text, color32f32(1,1,1,1), BLEND_MODE_ALPHA);
        software_framebuffer_draw_text(framebuffer, font2, 3, v2f32(60, 480-180), current_conversation_node->speaker_name, color32f32(1,1,1,1), BLEND_MODE_ALPHA);

        if (current_conversation_node->choice_count == 0) {
            software_framebuffer_draw_text(framebuffer, font, 2, v2f32(60, 480-40), string_literal("(next)"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            if (is_key_pressed(KEY_RETURN)) {
                u32 target = current_conversation_node->target;
                game_state->current_conversation_node_id = target;

                if (target == 0) {
                    state->is_conversation_active = false;
                }
            }
        } else {
            software_framebuffer_draw_text(framebuffer, font, 2, v2f32(60, 480-40), string_literal("(choices)"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);

            if (state->viewing_dialogue_choices) {
                if (is_key_pressed(KEY_ESCAPE)) {
                    state->viewing_dialogue_choices = false;
                }

                wrap_around_key_selection(KEY_UP, KEY_DOWN, &state->currently_selected_dialogue_choice, 0, current_conversation_node->choice_count);
                software_framebuffer_draw_quad(framebuffer, rectangle_f32(100, 100, 640-150, 300), color32u8(90, 30, 200, 255), BLEND_MODE_ALPHA);

                for (u32 index = 0; index < current_conversation_node->choice_count; ++index) {
                    struct conversation_choice* choice = current_conversation_node->choices + index;
                    if (index == state->currently_selected_dialogue_choice) {
                        software_framebuffer_draw_text(framebuffer, font2, 3, v2f32(100, 100 + 16*2.3 * index), choice->text, color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                    } else {
                        software_framebuffer_draw_text(framebuffer, font, 3, v2f32(100, 100 + 16*2.3 * index), choice->text, color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                    }
                }

                if (is_key_pressed(KEY_RETURN)) {
                    state->viewing_dialogue_choices = false;
                    u32 target = current_conversation_node->choices[state->currently_selected_dialogue_choice].target;
                    state->current_conversation_node_id = target;

                    if (target == 0) {
                        state->is_conversation_active = false;
                    }
                }
            } else {
                if (is_key_pressed(KEY_RETURN)) {
                    state->viewing_dialogue_choices = true;
                }
            }
        }
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

    {
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
            /* I need functionality quick cause animation takes a while to code. */
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

void update_and_render_game(struct software_framebuffer* framebuffer, f32 dt) {
    if (is_key_pressed(KEY_F12)) {
        image_buffer_write_to_disk(framebuffer, string_literal("scr"));
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
            game_state->weather.timer += dt;
        }

        entity_list_render_entities(&game_state->entities, &graphics_assets, &commands, dt);
        software_framebuffer_render_commands(framebuffer, &commands);

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
