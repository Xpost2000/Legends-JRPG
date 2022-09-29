/* need these to be coresponding to the shopping_ui file since it uses similar code. */
enum inventory_ui_animation_phase {
    UNUSED_INVENTORY_UI_STATE_0,
    UNUSED_INVENTORY_UI_STATE_1,
    INVENTORY_UI_ANIMATION_PHASE_SLIDE_IN,
    INVENTORY_UI_ANIMATION_PHASE_IDLE,
    INVENTORY_UI_ANIMATION_PHASE_SLIDE_OUT,
    UNUSED_INVENTORY_UI_STATE_5,
};

/*
  reusing the shopping_ui
  state, which is really just going to be inventory_view_state later.
*/

void open_party_inventory_screen(void) {
    shopping_ui.phase = INVENTORY_UI_ANIMATION_PHASE_IDLE;
    shopping_ui_populate_filtered_page(SHOPPING_MODE_VIEWING);
}
void close_party_inventory_screen(void) {
    zero_memory(&shopping_ui, sizeof(shopping_ui));
}

local void update_and_render_party_inventory_screen(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    const f32 FINAL_SHOPPING_MENU_X = 20;
    struct font_cache* normal_font      = game_get_font(MENU_FONT_COLOR_WHITE);
    struct font_cache* highlighted_font = game_get_font(MENU_FONT_COLOR_GOLD);

    switch (shopping_ui.phase) {
        case INVENTORY_UI_ANIMATION_PHASE_IDLE: {
            do_gold_counter(framebuffer, dt);
            do_shopping_menu(framebuffer, FINAL_SHOPPING_MENU_X, true, SHOPPING_MODE_VIEWING);
            software_framebuffer_draw_text(framebuffer, normal_font, 4, v2f32(10, 10),
                                           string_literal("INVENTORY"), color32f32_WHITE, BLEND_MODE_ALPHA);
        } break;
        default: {
            _debugprintf("bad case");
        } break;;
    }
}
