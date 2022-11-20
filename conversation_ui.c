/*
  INPROGRESS: Rich text system, which is kind of annoying to implement.
*/
#define RICHTEXT_EXPERIMENTAL

#define DIALOGUE_UI_CHARACTER_TYPE_TIMER     (0.045)
/* Honestly this is more than can actually show up to the game anyways. */
#define RICH_TEXT_CONVERSATION_UI_MAX_LENGTH (512)

struct rich_text_state {
    f32 x_cursor;
    f32 y_cursor;
    f32 tallest_glyph_height_on_line;

    f32 breath_magnitude;
    f32 breath_speed;
    s32 font_id;
    f32 text_scale;

#if 0
    /* Do later, not sure how to work this one out honestly. */
    f32 delay_timer;
    f32 text_type_speed;
#endif
};

struct rich_glyph {
    f32  x;
    f32  y;
    f32  scale;
    f32  breath_magnitude;
    f32  breath_speed;
    char character;
    s32  font_id;
};

struct {
    f32 speak_timer;
    s32 visible_characters;

    /* rich string */
    s32                rich_text_length;
    struct rich_glyph  rich_text[RICH_TEXT_CONVERSATION_UI_MAX_LENGTH];

    struct rich_text_state rich_text_state;
} dialogue_ui;

struct rich_text_state rich_text_state_default(void) {
    return (struct rich_text_state) {
        .x_cursor         = 0,
        .y_cursor         = 0,
        .breath_speed     = 0,
        .breath_magnitude = 0,
        /* .text_delay       = DIALOGUE_UI_CHARACTER_TYPE_TIMER, */
        .font_id          = MENU_FONT_COLOR_YELLOW,
        .text_scale       = 2,
    };
}

/* I would like to vary dialogue, but that takes a lot of extra mark up and rewriting... */
/* for now just keep it like this. */
void dialogue_ui_setup_for_next_line_of_dialogue(void) {
    dialogue_ui.speak_timer        = 0;
    dialogue_ui.visible_characters = 0;

    dialogue_ui.rich_text_length = 0;
    dialogue_ui.rich_text_state  = rich_text_state_default();
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
#ifndef RICHTEXT_EXPERIMENTAL
        {
            draw_ui_breathing_text_word_wrapped(framebuffer, v2f32(dialogue_box_start_position.x + 30, dialogue_box_start_position.y + 50), dialogue_box_extents.x * 0.76, font, 2, string_slice(current_conversation_node->text, 0, dialogue_ui.visible_characters), 1492, color32f32(1,1,1,1));
        }
#else
        for (s32 rich_glyph_index = 0; rich_glyph_index < dialogue_ui.rich_text_length; ++rich_glyph_index) {
            struct rich_glyph* current_glyph          = dialogue_ui.rich_text + rich_glyph_index;
            struct font_cache* glyph_font             = game_get_font(current_glyph->font_id);
            v2f32              glyph_where            = v2f32(current_glyph->x, current_glyph->y);
            f32                glyph_scale            = current_glyph->scale;
            f32                glyph_breath_magnitude = current_glyph->breath_magnitude;
            f32                glyph_breath_speed     = current_glyph->breath_speed;
            char               glyph_character        = current_glyph->character;

            glyph_where.x += dialogue_box_start_position.x + 30;
            glyph_where.y += dialogue_box_start_position.y + 50;

            software_framebuffer_draw_glyph(framebuffer, glyph_font, glyph_scale, glyph_where, glyph_character-32, color32f32_WHITE, BLEND_MODE_ALPHA);
        }
#endif

        if (dialogue_ui.visible_characters < current_conversation_node->text.length) {
            dialogue_ui.speak_timer += dt;

            if (dialogue_ui.speak_timer >= DIALOGUE_UI_CHARACTER_TYPE_TIMER) {
                dialogue_ui.speak_timer = 0;

#ifdef RICHTEXT_EXPERIMENTAL
                { /* rich text render path, this is adding / updating glyphs */
                    {
                        struct rich_text_state* rich_state = &dialogue_ui.rich_text_state;

                        bool needs_newline = false;
                        bool skip_glyph    = false;

                        string current_character_string = string_slice(current_conversation_node->text,
                                                                       dialogue_ui.visible_characters, dialogue_ui.visible_characters+1);
                        /* TODO word wrap */

                        if (current_character_string.data[0] == '\n') {
                            needs_newline = true;
                            skip_glyph    = true;
                        }

                        /* readding word wrap */
                        if (!needs_newline) {
                            string current_viewable_slice = string_slice(current_conversation_node->text, 0, dialogue_ui.visible_characters);
                            f32 current_x_cursor = rich_state->x_cursor;
                            f32 wrap_bounds_w = dialogue_box_extents.x * 0.70;
                            {
                                if (current_x_cursor > wrap_bounds_w) {
                                    needs_newline = true;
                                }
                            }
                        }

                        if (needs_newline) {
                            rich_state->x_cursor                      = 0;
                            rich_state->y_cursor                     += rich_state->tallest_glyph_height_on_line;
                            rich_state->tallest_glyph_height_on_line  = 0;
                        }

                        if (!skip_glyph) {
                            struct rich_glyph* current_rich_glyph = &dialogue_ui.rich_text[dialogue_ui.rich_text_length++];
                            _debugprintf("HI! NEW GLYPH (%d)(%c)\n", dialogue_ui.rich_text_length, current_character_string.data[0]);
                            assertion(dialogue_ui.rich_text_length <= RICH_TEXT_CONVERSATION_UI_MAX_LENGTH && "Your text is too big!");

                            current_rich_glyph->x                = rich_state->x_cursor;
                            current_rich_glyph->y                = rich_state->y_cursor;
                            current_rich_glyph->scale            = rich_state->text_scale;
                            current_rich_glyph->breath_magnitude = rich_state->breath_magnitude;
                            current_rich_glyph->breath_speed     = rich_state->breath_speed;
                            current_rich_glyph->character        = current_character_string.data[0];
                            current_rich_glyph->font_id          = rich_state->font_id;

                            struct font_cache* glyph_font = game_get_font(current_rich_glyph->font_id);
                            f32 font_scale   = current_rich_glyph->scale;
                            f32 glyph_width  = font_cache_text_width(glyph_font, current_character_string, font_scale);
                            f32 glyph_height = font_cache_text_height(glyph_font) * font_scale;

                            if (glyph_height > rich_state->tallest_glyph_height_on_line) {
                                rich_state->tallest_glyph_height_on_line = glyph_height;
                            }

                            rich_state->x_cursor += glyph_width;
                        }
                    }
                }
#endif
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
                if (current_conversation_node->choices[state->currently_selected_dialogue_choice].script_code.length) {
                    dialogue_choice_try_to_execute_script_actions(state, &current_conversation_node->choices[state->currently_selected_dialogue_choice], state->currently_selected_dialogue_choice);
                } else {
                    dialogue_ui_set_target_node(target);
                }
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
