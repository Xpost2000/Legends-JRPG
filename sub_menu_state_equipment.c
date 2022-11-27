/* TODO: better UI state handling, since we don't consume events properly. */
/* need separate binding for removing items */
#define EQUIPMENT_SCREEN_SPIN_TIMER_LENGTH (0.2)
#include "equipment_ui_def.c"

enum equipment_screen_phase {
    EQUIPMENT_SCREEN_PHASE_SLIDE_IN,
    EQUIPMENT_SCREEN_PHASE_IDLE,
    EQUIPMENT_SCREEN_PHASE_SLIDE_OUT,
};

struct {
    s32       phase;
    entity_id focus_entity;

    s32 direction_index;
    f32 spin_timer;

    s32 equip_slot_selection;

    /* based on the filters! */
    s32  inventory_slot_selection;
    bool inventory_pick_mode;

    /* this is for copying to the filter */
    /* It isn't a 1-1 mapping to the inventory filtering */
    /* and while I can allocate this from the arena in a sort of stack */
    /* I can just preallocate this. */
    s16  inventory_filtered_size;
    s16  inventory_item_slice[MAX_PARTY_ITEMS];

    /* As I lack a real UI system I don't consume events in an automated way so I'll just do it here. */
    /* easier to do this and less code anyhow. */
    /* of course this should be placed into the game state itself so we can event cancel in more locations */
    bool cancel_pressed;
    bool confirm_pressed;

    f32 animation_timer;
} equipment_screen_state;

local void equipment_screen_set_phase(s32 phase) {
    equipment_screen_state.phase           = phase;
    equipment_screen_state.animation_timer = 0;
}

/* render the entity but spinning their directional animations */
local void open_equipment_screen(entity_id target_id) {
    equipment_screen_state.inventory_slot_selection = 0;
    equipment_screen_state.equip_slot_selection     = 0;
    equipment_screen_state.focus_entity             = target_id;
    equipment_screen_set_phase(EQUIPMENT_SCREEN_PHASE_SLIDE_IN);
}

local void equipment_screen_build_new_filtered_item_list(s32 filter_mask) {
    equipment_screen_state.inventory_filtered_size  = 0;
    for (s32 index = 0; index < game_state->inventory.item_count; ++index) {
        struct item_instance* instance = game_state->inventory.items + index;
        item_id item_handle = instance->item;
        struct item_def* item = item_database_find_by_id(item_handle);

        _debugprintf("item checkings");

        if (item->type == ITEM_TYPE_EQUIPMENT || item->type == ITEM_TYPE_WEAPON) {
            _debugprintf("okay, this (%.*s) passed the basic filter", item->name.length, item->name.data);
            if (item->equipment_slot_flags == filter_mask) {
                _debugprintf("new item %.*s", item->name.length, item->name.data);
                equipment_screen_state.inventory_item_slice[equipment_screen_state.inventory_filtered_size++] = index;
            }
        }
    }
}

local void draw_equipment_tooltips(struct software_framebuffer* framebuffer, f32 bottom_y) {
    struct font_cache* normal_font      = game_get_font(MENU_FONT_COLOR_WHITE);

    union color32f32 modulation_color = color32f32_WHITE;
    union color32f32 ui_color         = UI_BATTLE_COLOR;

    struct entity* entity = game_dereference_entity(game_state, equipment_screen_state.focus_entity);

    string tip = {};
    if (!equipment_screen_state.inventory_pick_mode) {
        item_id slot = entity->equip_slots[equipment_screen_state.equip_slot_selection];
        struct item_def* item = item_database_find_by_id(slot);
        if (item) {
            tip = item->description;
        }
    } else {
        struct item_instance* instance = game_state->inventory.items + equipment_screen_state.inventory_item_slice[equipment_screen_state.inventory_slot_selection];
        item_id item_handle = instance->item;
        struct item_def* item = item_database_find_by_id(item_handle);
        if (item) {
            tip = item->description;
        }
    }

    if (tip.length) {
        draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, v2f32(15, bottom_y - (16*3.15)), 36, 2, ui_color);
        draw_ui_breathing_text(framebuffer, v2f32(30, bottom_y - (16*3.15) + 15), normal_font, 2, tip, 0, modulation_color);
    }
}

local void do_entity_stat_information_panel(struct software_framebuffer* framebuffer, f32 x, f32 y, struct entity* entity) {
    local string info_labels[] = {
        string_literal("NAME:"),
        string_literal("LEVEL:"),

        string_literal("EXP:"),
        string_literal("HP:"),
        string_literal("MP:"),

        string_literal("VIG:"),
        string_literal("STR:"),
        string_literal("CONS:"),
        string_literal("WILL:"),
        string_literal("AGI:"),
        string_literal("SPD:"),
        string_literal("INT:"),
        string_literal("LCK:"),
        string_literal("CNTR:"),
    };

    f32 largest_label_width = 0;

    struct font_cache* label_name_font   = game_get_font(MENU_FONT_COLOR_GOLD);
    struct font_cache* value_font        = game_get_font(MENU_FONT_COLOR_STEEL);
    struct font_cache* value_better_font = game_get_font(MENU_FONT_COLOR_LIME);
    struct font_cache* value_worse_font  = game_get_font(MENU_FONT_COLOR_BLOODRED);

    f32 font_scale = 2;
    f32 font_height = font_cache_text_height(label_name_font) * font_scale;

    draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, v2f32(x,y), 6*2, 6*4, UI_DEFAULT_COLOR);
    {
        for (s32 index = 0; index < array_count(info_labels); ++index) {
            f32 current_width = font_cache_text_width(label_name_font, info_labels[index], font_scale);
            if (current_width > largest_label_width) largest_label_width = current_width;
        }
    }


    {
        f32 y_cursor = y+15;
        {
            software_framebuffer_draw_text(framebuffer, label_name_font, font_scale, v2f32(x+15, y_cursor), info_labels[0], color32f32_WHITE, BLEND_MODE_ALPHA);
            software_framebuffer_draw_text(framebuffer, value_font, font_scale, v2f32(x+15 + largest_label_width, y_cursor), entity->name, color32f32_WHITE, BLEND_MODE_ALPHA);
        }
        y_cursor += font_height;
        {
            software_framebuffer_draw_text(framebuffer, label_name_font, font_scale, v2f32(x+15, y_cursor), info_labels[1], color32f32_WHITE, BLEND_MODE_ALPHA);
            software_framebuffer_draw_text(framebuffer, value_font, font_scale, v2f32(x+15 + largest_label_width, y_cursor), string_from_cstring(format_temp("%d", entity->stat_block.level)), color32f32_WHITE, BLEND_MODE_ALPHA);
        }
        y_cursor += font_height;
        y_cursor += font_height;

        {
            software_framebuffer_draw_text(framebuffer, label_name_font, font_scale, v2f32(x+15, y_cursor), info_labels[2], color32f32_WHITE, BLEND_MODE_ALPHA);
            software_framebuffer_draw_text(framebuffer, value_font, font_scale, v2f32(x+15 + largest_label_width, y_cursor), string_from_cstring(format_temp("%d", entity->stat_block.experience)), color32f32_WHITE, BLEND_MODE_ALPHA);
        }
        y_cursor += font_height;
        {
            software_framebuffer_draw_text(framebuffer, label_name_font, font_scale, v2f32(x+15, y_cursor), info_labels[3], color32f32_WHITE, BLEND_MODE_ALPHA);
            software_framebuffer_draw_text(framebuffer, value_font, font_scale, v2f32(x+15 + largest_label_width, y_cursor), string_from_cstring(format_temp("%d/%d", entity->health.value, entity->health.max)), color32f32_WHITE, BLEND_MODE_ALPHA);
        }
        y_cursor += font_height;
        {
            software_framebuffer_draw_text(framebuffer, label_name_font, font_scale, v2f32(x+15, y_cursor), info_labels[4], color32f32_WHITE, BLEND_MODE_ALPHA);
            software_framebuffer_draw_text(framebuffer, value_font, font_scale, v2f32(x+15 + largest_label_width, y_cursor), string_from_cstring(format_temp("%d/%d", entity->magic.value, entity->magic.max)), color32f32_WHITE, BLEND_MODE_ALPHA);
        }
        y_cursor += font_height;
        y_cursor += font_height;

        /* stats now need delta comparison */
        for (s32 index = 0; index < STAT_COUNT; ++index) {
            software_framebuffer_draw_text(framebuffer, label_name_font, font_scale, v2f32(x+15, y_cursor), info_labels[index+5], color32f32_WHITE, BLEND_MODE_ALPHA);

            s32 current_stat_value = entity_find_effective_stat_value(entity, index);
            software_framebuffer_draw_text(framebuffer, value_font, font_scale, v2f32(x+15 + largest_label_width, y_cursor), string_from_cstring(format_temp("%d", current_stat_value)), color32f32_WHITE, BLEND_MODE_ALPHA);

            if (equipment_screen_state.inventory_pick_mode) {
                struct item_instance* selected_item_id = game_state->inventory.items + equipment_screen_state.inventory_item_slice[equipment_screen_state.inventory_slot_selection];
                struct item_def* item                  = item_database_find_by_id(selected_item_id->item);

                s32 target_stat_value_modification = item->stats.values[index];
                s32 target_stat_value              = target_stat_value_modification + current_stat_value;

                struct font_cache* painting_font = NULL;

                if (target_stat_value == current_stat_value)     painting_font = value_font;
                else if (target_stat_value > current_stat_value) painting_font = value_better_font;
                else if (target_stat_value < current_stat_value) painting_font = value_worse_font;

                software_framebuffer_draw_text(framebuffer, painting_font, font_scale, v2f32(x+15 + largest_label_width + 60, y_cursor), string_from_cstring(format_temp("%d", target_stat_value)), color32f32_WHITE, BLEND_MODE_ALPHA);
            }

            y_cursor += font_height;
        }
    }
}

local s32 filter_mask_slot_table_map[] = {
    EQUIPMENT_SLOT_FLAG_HEAD,
    EQUIPMENT_SLOT_FLAG_BODY,
    EQUIPMENT_SLOT_FLAG_HANDS,
    EQUIPMENT_SLOT_FLAG_LEGS,

    EQUIPMENT_SLOT_FLAG_MISC,
    EQUIPMENT_SLOT_FLAG_MISC,

    EQUIPMENT_SLOT_FLAG_WEAPON,
};

local f32 Equipment_Panel_draw_entries_for_items(string* labels, f32 largest_label_width, struct entity* entity, f32 x, f32 y_cursor, struct software_framebuffer* framebuffer, s32 start_index, s32 end_index) {
    struct font_cache* label_name_font   = game_get_font(MENU_FONT_COLOR_GOLD);
    struct font_cache* value_font        = game_get_font(MENU_FONT_COLOR_STEEL);
    struct font_cache* select_value_font = game_get_font(MENU_FONT_COLOR_ORANGE);

    f32 font_scale = 2;
    f32 font_height = font_cache_text_height(label_name_font) * font_scale;

    for (s32 index = start_index; index < end_index; ++index) {
        f32 offset_indent = 15;
        software_framebuffer_draw_text(framebuffer, label_name_font, font_scale, v2f32(x+offset_indent, y_cursor), labels[index], color32f32_WHITE, BLEND_MODE_ALPHA);
        {
            item_id slot = entity->equip_slots[index];
            struct item_def* item = item_database_find_by_id(slot);
            struct font_cache* painting_font = value_font;
            if (index == equipment_screen_state.equip_slot_selection) {
                painting_font = select_value_font;
            }
            if (!item) {
                software_framebuffer_draw_text(framebuffer, painting_font, font_scale, v2f32(x+offset_indent+largest_label_width, y_cursor), string_literal("------"), color32f32_WHITE, BLEND_MODE_ALPHA);
            } else {
                software_framebuffer_draw_text(framebuffer, painting_font, font_scale, v2f32(x+offset_indent+largest_label_width, y_cursor), item->name, color32f32_WHITE, BLEND_MODE_ALPHA);
            }
        }
        y_cursor += font_height;
    }

    return y_cursor;
}

local void do_entity_equipment_panel(struct software_framebuffer* framebuffer, f32 x, f32 y, struct entity* entity, bool allow_input) {
    local string info_labels[] = {
        string_literal("HEAD:"),
        string_literal("CHST:"),
        string_literal("HNDS:"),
        string_literal("LEGS:"),

        string_literal("OTHR:"),
        string_literal("OTHR:"),

        string_literal("WPN:"),
    };

    bool action_move_down = is_action_down_with_repeat(INPUT_ACTION_MOVE_DOWN);
    bool action_move_up   = is_action_down_with_repeat(INPUT_ACTION_MOVE_UP);
    bool action_pause     = is_action_pressed(INPUT_ACTION_PAUSE);

    if (!allow_input) {
        action_move_down                       = false;
        action_move_up                         = false;
        action_pause                           = false;
        equipment_screen_state.cancel_pressed  = false;
        equipment_screen_state.confirm_pressed = false;
    }

    f32 largest_label_width = 0;

    struct font_cache* label_name_font   = game_get_font(MENU_FONT_COLOR_GOLD);
    struct font_cache* value_font        = game_get_font(MENU_FONT_COLOR_STEEL);
    struct font_cache* select_value_font = game_get_font(MENU_FONT_COLOR_ORANGE);

    f32 font_scale = 2;
    f32 font_height = font_cache_text_height(label_name_font) * font_scale;

    draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, v2f32(x,y), 5*2, 6*2+2, UI_DEFAULT_COLOR);
    {
        for (s32 index = 0; index < array_count(info_labels); ++index) {
            f32 current_width = font_cache_text_width(label_name_font, info_labels[index], font_scale);
            if (current_width > largest_label_width) largest_label_width = current_width;
        }
    }

    f32 y_cursor = y+15;
    y_cursor     = Equipment_Panel_draw_entries_for_items(info_labels, largest_label_width, entity, x, y_cursor, framebuffer, 0, array_count(info_labels)-3);
    y_cursor += font_height;
    y_cursor     = Equipment_Panel_draw_entries_for_items(info_labels, largest_label_width, entity, x, y_cursor, framebuffer, array_count(info_labels)-3, array_count(info_labels)-1);
    y_cursor += font_height;
    y_cursor     = Equipment_Panel_draw_entries_for_items(info_labels, largest_label_width, entity, x, y_cursor, framebuffer, array_count(info_labels)-1, array_count(info_labels));

    {
        static bool first_time_build_filter = false;
        if (!first_time_build_filter) {
            first_time_build_filter = true;
            equipment_screen_build_new_filtered_item_list(filter_mask_slot_table_map[equipment_screen_state.equip_slot_selection]);
        }
    }

    if (!equipment_screen_state.inventory_pick_mode) {
        if (action_move_down) {
            equipment_screen_state.equip_slot_selection += 1;
            if (equipment_screen_state.equip_slot_selection >= array_count(info_labels)) {
                equipment_screen_state.equip_slot_selection = 0;
            }

            equipment_screen_build_new_filtered_item_list(filter_mask_slot_table_map[equipment_screen_state.equip_slot_selection]);
        } else if (action_move_up) {
            equipment_screen_state.equip_slot_selection -= 1;
            if (equipment_screen_state.equip_slot_selection < 0) {
                equipment_screen_state.equip_slot_selection = array_count(info_labels)-1;
            }

            equipment_screen_build_new_filtered_item_list(filter_mask_slot_table_map[equipment_screen_state.equip_slot_selection]);
        }

        bool allow_cancel_input_to_leave_equipment = true;

        if (equipment_screen_state.confirm_pressed) {
            if (equipment_screen_state.inventory_filtered_size > 0) {
                equipment_screen_state.inventory_pick_mode = true;    
                equipment_screen_state.confirm_pressed = false;
            }
        } else if (equipment_screen_state.cancel_pressed) {
            if (entity_any_equipped_item(entity, equipment_screen_state.equip_slot_selection)) {
                entity_inventory_unequip_item((struct entity_inventory*)&game_state->inventory, MAX_PARTY_ITEMS, equipment_screen_state.equip_slot_selection, entity);
                equipment_screen_state.cancel_pressed = false;

                equipment_screen_build_new_filtered_item_list(filter_mask_slot_table_map[equipment_screen_state.equip_slot_selection]);
                allow_cancel_input_to_leave_equipment = false;
            }
        }

        if (action_pause || (equipment_screen_state.cancel_pressed && allow_cancel_input_to_leave_equipment)) {
            equipment_screen_set_phase(EQUIPMENT_SCREEN_PHASE_SLIDE_OUT);
        }
    }
}

/* TODO: draw icons */
local void do_entity_select_equipment_panel(struct software_framebuffer* framebuffer, f32 x, f32 y, struct entity* entity, bool allow_input) {
    if (equipment_screen_state.inventory_filtered_size == 0) return;

    union color32f32 ui_color  = UI_DEFAULT_COLOR;
    union color32f32 mod_color = color32f32_WHITE;

    bool action_move_down = is_action_down_with_repeat(INPUT_ACTION_MOVE_DOWN);
    bool action_move_up   = is_action_down_with_repeat(INPUT_ACTION_MOVE_UP);

    if (!allow_input) {
        action_move_down                       = false;
        action_move_up                         = false;
        equipment_screen_state.cancel_pressed  = false;
        equipment_screen_state.confirm_pressed = false;
    }

    if (!equipment_screen_state.inventory_pick_mode) {
        ui_color.a = 0.5;
        mod_color.a = 0.5;
    }

    draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, v2f32(x,y), 5*2, 5*2, ui_color);

    s32 lower_limit;
    s32 upper_limit;
    
    set_scrollable_ui_bounds(equipment_screen_state.inventory_slot_selection, &lower_limit, &upper_limit, equipment_screen_state.inventory_filtered_size, 1, 4);
    _debugprintf("%d lower, %d upper", lower_limit, upper_limit);

    struct font_cache* value_font        = game_get_font(MENU_FONT_COLOR_STEEL);
    struct font_cache* select_value_font = game_get_font(MENU_FONT_COLOR_ORANGE);
    f32 font_scale  = 2;
    f32 font_height = font_cache_text_height(value_font) * font_scale;

    f32 y_cursor = y + 15;

    for (s32 index = lower_limit; index < upper_limit; ++index) {
        struct item_instance* instance = game_state->inventory.items + equipment_screen_state.inventory_item_slice[index];
        item_id item_handle = instance->item;
        struct item_def* item = item_database_find_by_id(item_handle);
        
        if (item) {
            struct font_cache* painting_font = value_font;
            if (index == equipment_screen_state.inventory_slot_selection) {
                painting_font = select_value_font;
            }
            software_framebuffer_draw_text(framebuffer, painting_font, font_scale, v2f32(x+15, y_cursor), string_from_cstring(format_temp("%.*s (x%d)", item->name.length, item->name.data, instance->count)), mod_color, BLEND_MODE_ALPHA);
            y_cursor += font_height;
        }
    }

    if (equipment_screen_state.inventory_pick_mode) {
        if (action_move_down) {
            play_sound(ui_blip);
            equipment_screen_state.inventory_slot_selection += 1;

            if (equipment_screen_state.inventory_slot_selection >= equipment_screen_state.inventory_filtered_size) {
                equipment_screen_state.inventory_slot_selection = 0;
            }
        } else if (action_move_up) {
            play_sound(ui_blip);
            equipment_screen_state.inventory_slot_selection -= 1;

            if (equipment_screen_state.inventory_slot_selection < 0) {
                equipment_screen_state.inventory_slot_selection = equipment_screen_state.inventory_filtered_size-1;
            }
        }

        if (equipment_screen_state.cancel_pressed) {
            equipment_screen_state.inventory_pick_mode = false;
            equipment_screen_state.cancel_pressed = false;
        } else if (equipment_screen_state.confirm_pressed) {
            entity_inventory_equip_item((struct entity_inventory*)&game_state->inventory, MAX_PARTY_ITEMS,
                                        equipment_screen_state.inventory_item_slice[equipment_screen_state.inventory_slot_selection],
                                        equipment_screen_state.equip_slot_selection, entity);
            equipment_screen_state.inventory_pick_mode = false;
            equipment_screen_build_new_filtered_item_list(filter_mask_slot_table_map[equipment_screen_state.equip_slot_selection]);
            equipment_screen_state.confirm_pressed = false;
        }
    }
}

local void do_spinning_preview_of_character(f32 x, f32 y, struct software_framebuffer* framebuffer, struct entity* target_entity) {
    {
        string facing_direction_string = facing_direction_strings_normal[equipment_screen_state.direction_index];
        struct entity_animation* anim = find_animation_by_name(target_entity->model_index, format_temp_s("idle_%.*s", facing_direction_string.length, facing_direction_string.data));
        image_id sprite_to_use = anim->sprites[0];

        software_framebuffer_draw_image_ex(framebuffer,
                                           graphics_assets_get_image_by_id(&graphics_assets, sprite_to_use),
                                           rectangle_f32(x, y-TILE_UNIT_SIZE/2, TILE_UNIT_SIZE*2, TILE_UNIT_SIZE*4),
                                           RECTANGLE_F32_NULL,
                                           color32f32_WHITE, NO_FLAGS, BLEND_MODE_ALPHA);
    }
}

local void equipment_update_character_spinner(f32 dt) {
    equipment_screen_state.spin_timer += dt;

    if (equipment_screen_state.spin_timer >= EQUIPMENT_SCREEN_SPIN_TIMER_LENGTH) {
        equipment_screen_state.spin_timer       = 0;
        equipment_screen_state.direction_index += 1;

        if (equipment_screen_state.direction_index >= 4) {
            equipment_screen_state.direction_index = 0;
        }
    }
}

s32 do_equipment_menu(struct software_framebuffer* framebuffer, f32 dt) {
    struct entity* target_entity = game_dereference_entity(game_state, equipment_screen_state.focus_entity);

    equipment_screen_state.cancel_pressed  = is_action_pressed(INPUT_ACTION_CANCEL);
    equipment_screen_state.confirm_pressed = is_action_pressed(INPUT_ACTION_CONFIRMATION);

    f32 x_ui_widget         = 0;
    f32 x_character_spinner = 0;

    bool allow_input = false;

    switch (equipment_screen_state.phase) {
        case EQUIPMENT_SCREEN_PHASE_SLIDE_IN: {
            const f32 MAX_T = 1.0;
            f32 effective_t = clamp_f32(equipment_screen_state.animation_timer / MAX_T, 0, 1);

            x_character_spinner = lerp_f32(-100, 100, effective_t);
            x_ui_widget         = lerp_f32(framebuffer->width*2, framebuffer->width, effective_t);

            if (equipment_screen_state.animation_timer >= MAX_T) {
                equipment_screen_set_phase(EQUIPMENT_SCREEN_PHASE_IDLE);
            }
        } break;

        case EQUIPMENT_SCREEN_PHASE_IDLE: {
            x_character_spinner = 100;
            x_ui_widget         = framebuffer->width;
            allow_input         = true;
        } break;

        case EQUIPMENT_SCREEN_PHASE_SLIDE_OUT: {
            const f32 MAX_T = 1.0;
            f32 effective_t = clamp_f32(equipment_screen_state.animation_timer / MAX_T, 0, 1);

            x_character_spinner = lerp_f32(-100, 100, 1-effective_t);
            x_ui_widget         = lerp_f32(framebuffer->width*2, framebuffer->width, 1-effective_t);

            /* NOTE need to generic menu */
            if (equipment_screen_state.animation_timer >= MAX_T+0.1) {
                return EQUIPMENT_MENU_PROCESS_ID_EXIT;
            }
        } break;
    }

    equipment_screen_state.animation_timer += dt;
    do_spinning_preview_of_character(x_character_spinner, 240, framebuffer, target_entity);

    do_entity_stat_information_panel(framebuffer, x_ui_widget - TILE_UNIT_SIZE*13, 30, target_entity);
    do_entity_select_equipment_panel(framebuffer, x_ui_widget - TILE_UNIT_SIZE*6, framebuffer->height-TILE_UNIT_SIZE*6.5, target_entity, allow_input);
    do_entity_equipment_panel(framebuffer, x_ui_widget - TILE_UNIT_SIZE*6, 30, target_entity, allow_input);
    draw_equipment_tooltips(framebuffer, framebuffer->height);

    equipment_update_character_spinner(dt);
    return EQUIPMENT_MENU_PROCESS_ID_OKAY;
}

local void update_and_render_character_equipment_screen(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    s32 menu_result = do_equipment_menu(framebuffer, dt);
    if (menu_result == EQUIPMENT_MENU_PROCESS_ID_EXIT) {
        reexpose_pause_menu_options();
    }
}

