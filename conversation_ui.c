/* 8/26, man this thing has been code rotting... */
struct {
    
} dialogue_ui;
local void update_and_render_conversation_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    /* TODO no layout right now, do that later or tomorrow as warmup */
    /* TODO animation! */
    struct font_cache* font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_YELLOW]);
    struct font_cache* font2 = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]);
    struct conversation* conversation = &game_state->current_conversation;
    struct conversation_node* current_conversation_node = &conversation->nodes[game_state->current_conversation_node_id-1];

    u32 BOX_WIDTH = 36;
    u32 BOX_HEIGHT = 8;
    v2f32 dialogue_box_extents = nine_patch_estimate_extents(ui_chunky, 1, BOX_WIDTH, BOX_HEIGHT);
    v2f32 dialogue_box_start_position = v2f32(SCREEN_WIDTH/2 - dialogue_box_extents.x/2, (SCREEN_HEIGHT * 0.9) - dialogue_box_extents.y);
    {
        draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, dialogue_box_start_position, BOX_WIDTH, BOX_HEIGHT, UI_DEFAULT_COLOR);
        draw_ui_breathing_text(framebuffer, v2f32(dialogue_box_start_position.x + 20, dialogue_box_start_position.y + 15), font2, 3, current_conversation_node->speaker_name, 0, color32f32(1,1,1,1));
        draw_ui_breathing_text(framebuffer, v2f32(dialogue_box_start_position.x + 30, dialogue_box_start_position.y + 60), font, 2, current_conversation_node->text, 1492, color32f32(1,1,1,1));
    }


    if (current_conversation_node->choice_count == 0) {
        software_framebuffer_draw_text(framebuffer, font, 2, v2f32(60, 480-40), string_literal("(next)"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
        if (is_key_pressed(KEY_RETURN)) {
            u32 target = current_conversation_node->target;
            dialogue_try_to_execute_script_actions(game_state, current_conversation_node);
            game_state->current_conversation_node_id = target;

            if (target == 0) {
                game_finish_conversation(state);
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
                dialogue_choice_try_to_execute_script_actions(game_state, &current_conversation_node->choices[state->currently_selected_dialogue_choice]);
                state->current_conversation_node_id = target;

                if (target == 0) {
                    game_finish_conversation(state);
                }
            }
        } else {
            if (is_key_pressed(KEY_RETURN)) {
                state->viewing_dialogue_choices = true;
            }
        }
    }
}
