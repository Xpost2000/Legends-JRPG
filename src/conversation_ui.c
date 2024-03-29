#define RICHTEXT_EXPERIMENTAL

#define DIALOGUE_UI_CHARACTER_TYPE_TIMER     (0.045)
/* Honestly this is more than can actually show up to the game anyways. */
#define RICH_TEXT_CONVERSATION_UI_MAX_LENGTH (512)

enum dialogue_ui_phase {
    DIALOGUE_UI_PHASE_OPEN_UP,
    DIALOGUE_UI_PHASE_IDLE,
    DIALOGUE_UI_PHASE_CLOSE_DOWN,
};

struct {
    f32 speak_timer;
    s32 visible_characters;
    s32 parse_character_cursor;

    /* rich string */
    s32                rich_text_length;
    struct rich_glyph  rich_text[RICH_TEXT_CONVERSATION_UI_MAX_LENGTH];

    struct rich_text_state rich_text_state;

    s32  dialogue_speaker_buffer_count;
    char dialogue_speaker_buffer[64];
    s32  dialogue_buffer_count;
    char dialogue_buffer[256];

    s32 phase;
    f32 phase_animation_timer;

    /* -1 means default choice */
    s32 override_next_node;
} dialogue_ui;

local void start_dialogue_ui(void) {
    dialogue_ui.phase                 = DIALOGUE_UI_PHASE_OPEN_UP;
    dialogue_ui.phase_animation_timer = 0;
}

local void close_dialogue_ui(void) {
    dialogue_ui.phase                 = DIALOGUE_UI_PHASE_CLOSE_DOWN;
    dialogue_ui.phase_animation_timer = 0;
}

/* I would like to vary dialogue, but that takes a lot of extra mark up and rewriting... */
/* for now just keep it like this. */
struct conversation_node* conversation_find_node_with_id(struct conversation* conversation, s32 id) {
    for (s32 index = 0; index < conversation->node_count; ++index) {
        if (conversation->nodes[index].id == id) {
            return &conversation->nodes[index];
        }
    }

    return NULL;
}

void dialogue_ui_setup_for_next_line_of_dialogue(void) {
    dialogue_ui.speak_timer            = 0;
    dialogue_ui.visible_characters     = 0;
    dialogue_ui.parse_character_cursor = 0;
    dialogue_ui.rich_text_length       = 0;
    dialogue_ui.rich_text_state        = rich_text_state_default();
    dialogue_ui.override_next_node     = -1;

    struct conversation*      conversation              = &game_state->current_conversation;
    struct conversation_node* current_conversation_node = conversation_find_node_with_id(conversation, game_state->current_conversation_node_id);

    {
        string preprocessed_string = game_script_formatting_preprocess_string(&scratch_arena, current_conversation_node->speaker_name);
        dialogue_ui.dialogue_speaker_buffer_count = copy_string_into_cstring(preprocessed_string, dialogue_ui.dialogue_speaker_buffer, array_count(dialogue_ui.dialogue_speaker_buffer));
    }
    {
        string preprocessed_string = game_script_formatting_preprocess_string(&scratch_arena, current_conversation_node->text);
        dialogue_ui.dialogue_buffer_count = copy_string_into_cstring(preprocessed_string, dialogue_ui.dialogue_buffer, array_count(dialogue_ui.dialogue_buffer));
    }
}

void dialogue_ui_set_target_node(s32 id) {
    game_state->current_conversation_node_id = id;
    
    if (id == 0) {
        close_dialogue_ui();
    } else {
        struct conversation*      conversation              = &game_state->current_conversation;
        struct conversation_node* current_conversation_node = conversation_find_node_with_id(conversation, game_state->current_conversation_node_id);

        dialogue_ui_setup_for_next_line_of_dialogue();
        dialogue_try_to_execute_script_actions(game_state, current_conversation_node);
    }
}

/* returns amount of characters to read */
local s32 conversation_ui_advance_character(bool skipping_mode) {
    struct conversation*      conversation              = &game_state->current_conversation;
    struct conversation_node* current_conversation_node = conversation_find_node_with_id(conversation, game_state->current_conversation_node_id);
    s32                       fake_length               = text_length_without_dialogue_rich_markup_length(string_from_cstring_length_counted(dialogue_ui.dialogue_buffer, dialogue_ui.dialogue_buffer_count));

    if (dialogue_ui.visible_characters >= fake_length) {
        return 0;
    }

    dialogue_ui.speak_timer                 = 0;
    dialogue_ui.rich_text_state.delay_timer = 0;

    if (!skipping_mode) {
        camera_traumatize(&game_state->camera, dialogue_ui.rich_text_state.type_trauma);
    }

    {
        {
            struct rich_glyph* current_rich_glyph = &dialogue_ui.rich_text[dialogue_ui.rich_text_length++];
            assertion(dialogue_ui.rich_text_length <= RICH_TEXT_CONVERSATION_UI_MAX_LENGTH && "Your text is too big!");
            *current_rich_glyph = rich_text_parse_next_glyph(&dialogue_ui.rich_text_state, string_from_cstring_length_counted(dialogue_ui.dialogue_buffer, dialogue_ui.dialogue_buffer_count),
                                                             &dialogue_ui.parse_character_cursor, skipping_mode);
        }
    }
    dialogue_ui.visible_characters     += 1;
    dialogue_ui.parse_character_cursor += 1;

    return fake_length - dialogue_ui.visible_characters;
}

local void update_and_render_conversation_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    const u32 BOX_WIDTH                      = nine_patch_estimate_fitting_extents_width(ui_chunky, 1, SCREEN_WIDTH*0.85);
    const u32 BOX_HEIGHT                     = nine_patch_estimate_fitting_extents_height(ui_chunky, 1, SCREEN_HEIGHT*0.266667);
    const f32 MAX_DIALOGUE_UI_ANIMATION_TIME = 0.45;
    switch (dialogue_ui.phase) {
        case DIALOGUE_UI_PHASE_OPEN_UP: {
            const f32 MAX_T = MAX_DIALOGUE_UI_ANIMATION_TIME;
            f32 effective_t = dialogue_ui.phase_animation_timer / MAX_T;

            if (effective_t >= 1) effective_t = 1;
            if (effective_t <= 0) effective_t = 0;

            f32 current_box_width  = quadratic_ease_in_f32(1, BOX_WIDTH,  effective_t);
            f32 current_box_height = quadratic_ease_in_f32(1, BOX_HEIGHT, effective_t);
            v2f32 dialogue_box_extents = nine_patch_estimate_extents(ui_chunky, 1, current_box_width, current_box_height);
            /* NOTE: realign */
            v2f32 dialogue_box_start_position = v2f32(SCREEN_WIDTH/2 - dialogue_box_extents.x/2, (SCREEN_HEIGHT * 0.8) - dialogue_box_extents.y/2);
            draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, dialogue_box_start_position, current_box_width, current_box_height, UI_DEFAULT_COLOR);

            dialogue_ui.phase_animation_timer += dt;
            if (dialogue_ui.phase_animation_timer >= MAX_T) {
                dialogue_ui.phase_animation_timer = 0;
                dialogue_ui.phase                 = DIALOGUE_UI_PHASE_IDLE;
            }
        } break;
        case DIALOGUE_UI_PHASE_IDLE: {
            struct font_cache*        font                      = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_YELLOW]);
            struct font_cache*        font2                     = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_GOLD]);
            struct conversation*      conversation              = &game_state->current_conversation;
            struct conversation_node* current_conversation_node = conversation_find_node_with_id(conversation, game_state->current_conversation_node_id);

            v2f32 dialogue_box_extents = nine_patch_estimate_extents(ui_chunky, 1, BOX_WIDTH, BOX_HEIGHT);
            v2f32 dialogue_box_start_position = v2f32(SCREEN_WIDTH/2 - dialogue_box_extents.x/2, (SCREEN_HEIGHT * 0.9) - dialogue_box_extents.y);
            {
                draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, dialogue_box_start_position, BOX_WIDTH, BOX_HEIGHT, UI_DEFAULT_COLOR);
                draw_ui_breathing_text(framebuffer, v2f32(dialogue_box_start_position.x + TILE_UNIT_SIZE, dialogue_box_start_position.y + TILE_UNIT_SIZE/2), font2, 2, string_from_cstring_length_counted(dialogue_ui.dialogue_speaker_buffer, dialogue_ui.dialogue_speaker_buffer_count), 0, color32f32(1,1,1,1));

                f32 start_x_cursor        = dialogue_box_start_position.x + 30;
                f32 start_y_cursor        = dialogue_box_start_position.y + 30;
                f32 x_cursor              = start_x_cursor;
                f32 y_cursor              = start_y_cursor;
                f32 tallest_glyph_on_line = 0;

                f32 wrap_bounds_w = dialogue_box_extents.x * 0.75;

                for (s32 rich_glyph_index = 0; rich_glyph_index < dialogue_ui.rich_text_length; ++rich_glyph_index) {
                    struct rich_glyph* current_glyph          = dialogue_ui.rich_text + rich_glyph_index;
                    struct font_cache* glyph_font             = game_get_font(current_glyph->font_id);
                    f32                glyph_scale            = current_glyph->scale;
                    f32                glyph_breath_magnitude = current_glyph->breath_magnitude;
                    f32                glyph_breath_speed     = current_glyph->breath_speed;
                    char               glyph_character        = current_glyph->character;

                    /* The word wrap here is going to be **very** slow, so I'm very sorry for myself... */
                    /*
                      NOTE: I originally planned to precalculate this (when I pushed glyphs.) If it becomes slow enough
                      I'll revert the changes and try to do it the other way around.
                    */
                    f32 estimated_width_of_current_word = 0;
                    {
                        s32 word_start = rich_glyph_index;
                        s32 word_end   = word_start;
                        {

                            for (; word_end < dialogue_ui.rich_text_length; ++word_end) {
                                if (is_whitespace(dialogue_ui.rich_text[word_end].character)) {
                                    break;
                                }
                            }

                        }

                        {
                            for (s32 glyph_index = word_start; glyph_index < word_end; ++glyph_index) {
                                struct rich_glyph* current_glyph          = dialogue_ui.rich_text + glyph_index;
                                struct font_cache* glyph_font             = game_get_font(current_glyph->font_id);
                                f32                glyph_scale            = current_glyph->scale;
                                char               glyph_character        = current_glyph->character;

                                string glyph_string = {};
                                glyph_string.data   = &glyph_character;
                                glyph_string.length = 1;
                                f32 glyph_width = font_cache_text_width(glyph_font, glyph_string, glyph_scale);
                                estimated_width_of_current_word += glyph_width;
                            }
                        }

                    }

                    string glyph_string = {};
                    glyph_string.data   = &glyph_character;
                    glyph_string.length = 1;

                    f32 glyph_width   = font_cache_text_width(glyph_font, glyph_string, glyph_scale);
                    f32 glyph_height  = font_cache_text_height(glyph_font) * glyph_scale;

                    if (glyph_height > tallest_glyph_on_line) {
                        tallest_glyph_on_line = glyph_height;
                    }

                    if (glyph_character == '\n' || (x_cursor-start_x_cursor)+estimated_width_of_current_word > wrap_bounds_w) {
                        x_cursor =  start_x_cursor;
                        y_cursor += tallest_glyph_on_line;
                        tallest_glyph_on_line = 0;
                    }

                    f32 character_displacement_y = sinf((global_elapsed_time*glyph_breath_speed)) * glyph_breath_magnitude;
                    if (glyph_character != '\n') {
                        software_framebuffer_draw_glyph(framebuffer, glyph_font, glyph_scale, v2f32(x_cursor, y_cursor+character_displacement_y), glyph_character-32, color32f32_WHITE, BLEND_MODE_ALPHA);
                        x_cursor += glyph_width;
                    }
                }

                if (dialogue_ui.visible_characters < text_length_without_dialogue_rich_markup_length(current_conversation_node->text)) {
                    if (dialogue_ui.rich_text_state.delay_timer > 0) {
                        dialogue_ui.rich_text_state.delay_timer -= dt;
                    } else {
                        dialogue_ui.speak_timer += dt;

                        if (dialogue_ui.speak_timer >= dialogue_ui.rich_text_state.text_type_speed) {
                            conversation_ui_advance_character(false);
                        }
                    }
                }
            }


            if (current_conversation_node->choice_count == 0) {
                draw_ui_breathing_text(framebuffer, v2f32(dialogue_box_start_position.x + 30, dialogue_box_start_position.y + dialogue_box_extents.y - 10), font2, 1, string_literal("(proceed)"), 0451, color32f32(1,1,1,1));

                if (is_action_pressed(INPUT_ACTION_CONFIRMATION)) {
                    if (conversation_ui_advance_character(true)) {
                        while (conversation_ui_advance_character(true)) {
                            (void)(0);
                        }
                    } else {
                        s32 target = current_conversation_node->target;
                        if (dialogue_ui.override_next_node != -1) {
                            target = dialogue_ui.override_next_node;
                        }
                        /* TODO verify that this thing is safe. */
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

                        /* TODO check this in 320 mode... Don't have any choices currently */
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
                        s32 target = current_conversation_node->choices[state->currently_selected_dialogue_choice].target;
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
        } break;
        case DIALOGUE_UI_PHASE_CLOSE_DOWN: {
            const f32 MAX_T = MAX_DIALOGUE_UI_ANIMATION_TIME;
            f32 effective_t = dialogue_ui.phase_animation_timer / MAX_T;

            if (effective_t >= 1) effective_t = 1;
            if (effective_t <= 0) effective_t = 0;

            f32 current_box_width  = quadratic_ease_in_f32(1, BOX_WIDTH,  1 - effective_t);
            f32 current_box_height = quadratic_ease_in_f32(1, BOX_HEIGHT, 1 - effective_t);
            v2f32 dialogue_box_extents = nine_patch_estimate_extents(ui_chunky, 1, current_box_width, current_box_height);
            v2f32 dialogue_box_start_position = v2f32(SCREEN_WIDTH/2 - dialogue_box_extents.x/2, (SCREEN_HEIGHT * 0.9) - dialogue_box_extents.y);
            draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, dialogue_box_start_position, current_box_width, current_box_height, UI_DEFAULT_COLOR);

            dialogue_ui.phase_animation_timer += dt;
            if (dialogue_ui.phase_animation_timer >= MAX_T) {
                dialogue_ui.phase_animation_timer = 0;
                game_finish_conversation(game_state);
            }
        } break;
    }
}

void dialogue_ui_set_override_next_target(s32 new_override) {
    dialogue_ui.override_next_node = new_override;
}
