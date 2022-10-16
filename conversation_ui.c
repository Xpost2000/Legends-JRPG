/* 8/26, man this thing has been code rotting... */
struct {
    f32 timer;
    f32 speak_timer;
    s32 visible_characters;
} dialogue_ui;

/* I would like to vary dialogue, but that takes a lot of extra mark up and rewriting... */
/* for now just keep it like this. */
#define DIALOGUE_UI_CHARACTER_TYPE_TIMER (0.045)
void dialogue_ui_setup_for_next_line_of_dialogue(void) {
    dialogue_ui.speak_timer        = 0;
    dialogue_ui.visible_characters = 0;
}

void dialogue_ui_set_target_node(u32 id) {
    struct conversation*      conversation              = &game_state->current_conversation;
    struct conversation_node* current_conversation_node = &conversation->nodes[game_state->current_conversation_node_id-1];

    dialogue_ui_setup_for_next_line_of_dialogue();
    dialogue_try_to_execute_script_actions(game_state, current_conversation_node);
    game_state->current_conversation_node_id = id;

    if (id == 0) {
        game_finish_conversation(game_state);
    }
}

local void update_and_render_conversation_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
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
        draw_ui_breathing_text_word_wrapped(framebuffer, v2f32(dialogue_box_start_position.x + 30, dialogue_box_start_position.y + 50), dialogue_box_extents.x * 0.76, font, 2, string_slice(current_conversation_node->text, 0, dialogue_ui.visible_characters), 1492, color32f32(1,1,1,1));

        if (dialogue_ui.visible_characters < current_conversation_node->text.length) {
            dialogue_ui.speak_timer += dt;

            if (dialogue_ui.speak_timer >= DIALOGUE_UI_CHARACTER_TYPE_TIMER) {
                dialogue_ui.speak_timer = 0;
                dialogue_ui.visible_characters += 1;
            }
        }
    }


    if (current_conversation_node->choice_count == 0) {
        draw_ui_breathing_text(framebuffer, v2f32(dialogue_box_start_position.x + 30, dialogue_box_start_position.y + dialogue_box_extents.y - 10), font2, 2, string_literal("(proceed)"), 0451, color32f32(1,1,1,1));

        if (is_action_pressed(INPUT_ACTION_CONFIRMATION)) {
            if (dialogue_ui.visible_characters < current_conversation_node->text.length) {
                dialogue_ui.visible_characters = current_conversation_node->text.length;
            } else {
                u32 target = current_conversation_node->target;
                dialogue_ui_set_target_node(target);
            }
        }
    } else {
        draw_ui_breathing_text(framebuffer, v2f32(dialogue_box_start_position.x + 30, dialogue_box_start_position.y + dialogue_box_extents.y - 10), font2, 2, string_literal("(proceed)"), 0451, color32f32(1,1,1,1));

        if (state->viewing_dialogue_choices) {
            if (is_action_pressed(INPUT_ACTION_CANCEL)) {
                state->viewing_dialogue_choices = false;
            }

            if (is_action_down_with_repeat(INPUT_ACTION_MOVE_DOWN)) {
                state->currently_selected_dialogue_choice++;
                if (state->currently_selected_dialogue_choice >= current_conversation_node->choice_count) {
                    state->currently_selected_dialogue_choice = 0;
                }
            } else if (is_action_down_with_repeat(INPUT_ACTION_MOVE_UP)) {
                state->currently_selected_dialogue_choice--;
                if (state->currently_selected_dialogue_choice < 0) {
                    state->currently_selected_dialogue_choice = current_conversation_node->choice_count-1;
                }
            }

            {
                u32 BOX_WIDTH = 8; /* minimum size */
                {
                    for (u32 index = 0; index < current_conversation_node->choice_count; ++index) {
                        struct conversation_choice* choice = current_conversation_node->choices + index;
                        if (choice->text.length > BOX_WIDTH) {
                            BOX_WIDTH = choice->text.length;
                        }
                    }
                }

                u32 BOX_HEIGHT = current_conversation_node->choice_count * 2+1;
                v2f32 dialogue_box_extents = nine_patch_estimate_extents(ui_chunky, 1, BOX_WIDTH, BOX_HEIGHT);
                v2f32 dialogue_box_start_position = v2f32(SCREEN_WIDTH/2 - dialogue_box_extents.x/2, (SCREEN_HEIGHT * 0.5) - dialogue_box_extents.y);

                draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, dialogue_box_start_position, BOX_WIDTH, BOX_HEIGHT, UI_DEFAULT_COLOR);

                for (u32 index = 0; index < current_conversation_node->choice_count; ++index) {
                    struct conversation_choice* choice = current_conversation_node->choices + index;
                    if (index == state->currently_selected_dialogue_choice) {
                        software_framebuffer_draw_text(framebuffer, font2, 3, v2f32(dialogue_box_start_position.x+10, dialogue_box_start_position.y+10 + 16*2.3 * index), choice->text, color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                    } else {
                        software_framebuffer_draw_text(framebuffer, font, 3, v2f32(dialogue_box_start_position.x+10, dialogue_box_start_position.y+10 + 16*2.3 * index), choice->text, color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                    }
                }
            }
            /* software_framebuffer_draw_quad(framebuffer, rectangle_f32(100, 100, 640-150, 300), color32u8(90, 30, 200, 255), BLEND_MODE_ALPHA); */


            if (is_action_pressed(INPUT_ACTION_CONFIRMATION)) {
                state->viewing_dialogue_choices = false;
                u32 target = current_conversation_node->choices[state->currently_selected_dialogue_choice].target;
                dialogue_ui_set_target_node(target);
                dialogue_choice_try_to_execute_script_actions(state, &current_conversation_node->choices[state->currently_selected_dialogue_choice], state->currently_selected_dialogue_choice);
            }
        } else {
            if (is_action_pressed(INPUT_ACTION_CONFIRMATION)) {
                if (dialogue_ui.visible_characters < current_conversation_node->text.length) {
                    dialogue_ui.visible_characters = current_conversation_node->text.length;
                } else {
                    state->viewing_dialogue_choices = true;
                }
            }
        }
    }
}
