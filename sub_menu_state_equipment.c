/* TODO: Equipment animation! */
#define EQUIPMENT_SCREEN_SPIN_TIMER_LENGTH (0.2)

struct {
    entity_id focus_entity;

    s32 direction_index;
    f32 spin_timer;

    s32 equip_slot_selection;
} equipment_screen_state;

/* render the entity but spinning their directional animations */

local void open_equipment_screen(entity_id target_id) {
    equipment_screen_state.focus_entity = target_id;
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

        for (s32 index = 0; index < STAT_COUNT; ++index) {
            software_framebuffer_draw_text(framebuffer, label_name_font, font_scale, v2f32(x+15, y_cursor), info_labels[index+5], color32f32_WHITE, BLEND_MODE_ALPHA);
            software_framebuffer_draw_text(framebuffer, value_font, font_scale, v2f32(x+15 + largest_label_width, y_cursor), string_from_cstring(format_temp("%d", entity->stat_block.values[index])), color32f32_WHITE, BLEND_MODE_ALPHA);
            y_cursor += font_height;
        }
    }
}
local void do_entity_equipment_panel(struct software_framebuffer* framebuffer, f32 x, f32 y, struct entity* entity) {
    local string info_labels[] = {
        string_literal("HEAD:"),
        string_literal("CHST:"),
        string_literal("HNDS:"),
        string_literal("LEGS:"),

        string_literal("OTHR:"),
        string_literal("OTHR:"),
    };

    f32 largest_label_width = 0;

    struct font_cache* label_name_font   = game_get_font(MENU_FONT_COLOR_GOLD);
    struct font_cache* value_font        = game_get_font(MENU_FONT_COLOR_STEEL);
    struct font_cache* select_value_font = game_get_font(MENU_FONT_COLOR_ORANGE);

    f32 font_scale = 2;
    f32 font_height = font_cache_text_height(label_name_font) * font_scale;

    draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, v2f32(x,y), 5*2, 6*2, UI_DEFAULT_COLOR);
    {
        for (s32 index = 0; index < array_count(info_labels); ++index) {
            f32 current_width = font_cache_text_width(label_name_font, info_labels[index], font_scale);
            if (current_width > largest_label_width) largest_label_width = current_width;
        }
    }

    {
        s32 index = 0;
        f32 y_cursor = y+15;

        for (; index < array_count(info_labels)-2; ++index) {
            f32 offset_indent = 15;
            software_framebuffer_draw_text(framebuffer, label_name_font, font_scale, v2f32(x+offset_indent, y_cursor), info_labels[index], color32f32_WHITE, BLEND_MODE_ALPHA);
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
        y_cursor += font_height;
        for (; index < array_count(info_labels); ++index) {
            f32 offset_indent = 15;
            software_framebuffer_draw_text(framebuffer, label_name_font, font_scale, v2f32(x+offset_indent, y_cursor), info_labels[index], color32f32_WHITE, BLEND_MODE_ALPHA);
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
    }

    if (is_key_down_with_repeat(KEY_DOWN)) {
        equipment_screen_state.equip_slot_selection += 1;
        if (equipment_screen_state.equip_slot_selection >= array_count(info_labels)) {
            equipment_screen_state.equip_slot_selection = 0;
        }
    } else if (is_key_down_with_repeat(KEY_UP)) {
        equipment_screen_state.equip_slot_selection -= 1;
        if (equipment_screen_state.equip_slot_selection < 0) {
            equipment_screen_state.equip_slot_selection = array_count(info_labels)-1;
        }
    }
}
local void do_entity_select_equipment_panel(struct software_framebuffer* framebuffer, f32 x, f32 y, struct entity* entity) {
    draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, v2f32(x,y), 18*2, 5*2, UI_DEFAULT_COLOR);
}

local void update_and_render_character_equipment_screen(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    struct entity* target_entity = entity_list_dereference_entity(&state->entities, equipment_screen_state.focus_entity);

    {
        struct entity_animation* anim = find_animation_by_name(target_entity->model_index, facing_direction_strings_normal[equipment_screen_state.direction_index]);
        image_id sprite_to_use = anim->sprites[0];

        software_framebuffer_draw_image_ex(framebuffer,
                                           graphics_assets_get_image_by_id(&graphics_assets, sprite_to_use),
                                           rectangle_f32(100, 240-TILE_UNIT_SIZE/2, TILE_UNIT_SIZE*2, TILE_UNIT_SIZE*4),
                                           RECTANGLE_F32_NULL,
                                           color32f32_WHITE, NO_FLAGS, BLEND_MODE_ALPHA);
    }

    do_entity_stat_information_panel(framebuffer, framebuffer->width - TILE_UNIT_SIZE*13, 30, target_entity);
    do_entity_equipment_panel(framebuffer, framebuffer->width - TILE_UNIT_SIZE*6, 30, target_entity);

    equipment_screen_state.spin_timer += dt;

    if (equipment_screen_state.spin_timer >= EQUIPMENT_SCREEN_SPIN_TIMER_LENGTH) {
        equipment_screen_state.spin_timer       = 0;
        equipment_screen_state.direction_index += 1;

        if (equipment_screen_state.direction_index >= 4) {
            equipment_screen_state.direction_index = 0;
        }
    }
}
