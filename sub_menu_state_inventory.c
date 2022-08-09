/* TODO: Inventory animation! */
enum inventory_ui_animation_phase {
    INVENTORY_UI_ANIMATION_PHASE_OPEN,
    INVENTORY_UI_ANIMATION_PHASE_IDLE,
    INVENTORY_UI_ANIMATION_PHASE_CLOSE,
};

local void open_party_inventory_screen(void) {
    
}

local void update_and_render_party_inventory_screen(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    f32 font_scale = 3;
    struct ui_pause_menu* menu_state = &state->ui_pause;
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
}
