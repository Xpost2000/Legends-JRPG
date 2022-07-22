local void update_and_render_conversation_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
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
