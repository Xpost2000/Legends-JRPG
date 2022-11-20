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
    f32  scale;
    f32  breath_magnitude;
    f32  breath_speed;
    char character;
    s32  font_id;
};

struct {
    f32 speak_timer;
    s32 visible_characters;
    s32 parse_character_cursor;

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
    dialogue_ui.speak_timer            = 0;
    dialogue_ui.visible_characters     = 0;
    dialogue_ui.parse_character_cursor = 0;

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

s32 fontname_string_to_id(string name) {
    local struct {
        string first;
        s32    result;
    } string_table_mapping_to_font_id[] = {
        {string_literal("gold"),     MENU_FONT_COLOR_GOLD},
        {string_literal("orange"),   MENU_FONT_COLOR_ORANGE},
        {string_literal("lime"),     MENU_FONT_COLOR_LIME},
        {string_literal("skyblue"),  MENU_FONT_COLOR_SKYBLUE},
        {string_literal("purple"),   MENU_FONT_COLOR_PURPLE},
        {string_literal("blue"),     MENU_FONT_COLOR_BLUE},
        {string_literal("steel"),    MENU_FONT_COLOR_STEEL},
        {string_literal("white"),    MENU_FONT_COLOR_WHITE},
        {string_literal("yellow"),   MENU_FONT_COLOR_YELLOW},
        {string_literal("bloodred"), MENU_FONT_COLOR_BLOODRED},
        {string_literal("red"),      MENU_FONT_COLOR_BLOODRED},
    };

    for (s32 mapping_index = 0; mapping_index < array_count(string_table_mapping_to_font_id); ++mapping_index) {
        if (string_equal(name, string_table_mapping_to_font_id[mapping_index].first)) {
            return string_table_mapping_to_font_id[mapping_index].result;
        }
    }

    return MENU_FONT_COLOR_GOLD;
}

local void parse_markup_details(struct rich_text_state* rich_text_state, string text_data) {
    /* lisp forms */ 
    struct lisp_list markup_forms = lisp_read_string_into_forms(&scratch_arena, text_data);

    /* NOTE: no error checking */
    for (s32 markup_form_index = 0; markup_form_index < markup_forms.count; ++markup_form_index) {
        struct lisp_form* current_form  = markup_forms.forms + markup_form_index;
        struct lisp_form* name_part     = lisp_list_nth(current_form, 0);
        struct lisp_form* argument_part = lisp_list_nth(current_form, 1);

        if (lisp_form_symbol_matching(*name_part, string_literal("font"))) {
            string font_name = {};
            assertion(lisp_form_get_string(*argument_part, &font_name) && "Bad font string");
            s32 new_font_id = fontname_string_to_id(font_name);
            rich_text_state->font_id = new_font_id;
        } else if (lisp_form_symbol_matching(*name_part, string_literal("breath_speed"))) {
            assertion(lisp_form_get_f32(*argument_part, &rich_text_state->breath_speed) && "Bad breath speed value");
        } else if (lisp_form_symbol_matching(*name_part, string_literal("breath_magnitude"))) {
            assertion(lisp_form_get_f32(*argument_part, &rich_text_state->breath_magnitude) && "Bad breath magnitude value");
        } else if (lisp_form_symbol_matching(*name_part, string_literal("text_scale"))) {
            assertion(lisp_form_get_f32(*argument_part, &rich_text_state->text_scale) && "Bad text scale value");
        } else if (lisp_form_symbol_matching(*name_part, string_literal("reset"))) {
            *rich_text_state = rich_text_state_default();
        }
    }
    
}

local s32 text_length_without_dialogue_rich_markup_length(string text) {
    s32 length = 0;
    for (s32 character_index = 0; character_index < text.length; ++character_index) {
        switch (text.data[character_index]) {
            case '\\': {
                character_index += 2;
                length          += 1;
            } break;
            case '[': {
                s32 bracket_counter = 1;
                character_index++;
                while (bracket_counter > 0) {
                    if (text.data[character_index] == '[') {
                        bracket_counter += 1;
                    } else if (text.data[character_index] == ']') {
                        bracket_counter -= 1;
                    }
                    character_index++;
                }
            } break;
            default: {
                length += 1;
            } break;
        }
    }
    return length;
}

/* returns amount of characters to read */
local s32 conversation_ui_advance_character(void) {
    struct conversation* conversation = &game_state->current_conversation;
    struct conversation_node* current_conversation_node = &conversation->nodes[game_state->current_conversation_node_id-1];

    s32 fake_length = text_length_without_dialogue_rich_markup_length(current_conversation_node->text);

    dialogue_ui.speak_timer = 0;

#ifdef RICHTEXT_EXPERIMENTAL
    /* NOTE: This doesn't do correct bounds checks */
    if (current_conversation_node->text.data[dialogue_ui.parse_character_cursor] == '\\') {
        dialogue_ui.parse_character_cursor++;
    } else if (current_conversation_node->text.data[dialogue_ui.parse_character_cursor] == '[') {
        s32 bracket_count = 1;

        dialogue_ui.parse_character_cursor += 1;

        s32 start_of_rich_markup = dialogue_ui.parse_character_cursor;

        while (bracket_count > 0) {
            if (current_conversation_node->text.data[dialogue_ui.parse_character_cursor] == '[') {
                bracket_count += 1;
            } else if (current_conversation_node->text.data[dialogue_ui.parse_character_cursor] == ']') {
                bracket_count -= 1;
            }

            dialogue_ui.parse_character_cursor += 1;
        }

        s32 end_of_rich_markup = dialogue_ui.parse_character_cursor-1;

        string markup_text = string_slice(current_conversation_node->text, start_of_rich_markup, end_of_rich_markup);
        parse_markup_details(&dialogue_ui.rich_text_state, markup_text);
    }

    { /* rich text render path, this is adding / updating glyphs */
        {
            struct rich_text_state* rich_state = &dialogue_ui.rich_text_state;
            bool skip_glyph    = false;

            string current_character_string = string_slice(current_conversation_node->text,
                                                           dialogue_ui.parse_character_cursor, dialogue_ui.parse_character_cursor+1);
            /* char  current_character         =  */

            struct rich_glyph* current_rich_glyph = &dialogue_ui.rich_text[dialogue_ui.rich_text_length++];
            assertion(dialogue_ui.rich_text_length <= RICH_TEXT_CONVERSATION_UI_MAX_LENGTH && "Your text is too big!");

            current_rich_glyph->scale            = rich_state->text_scale;
            current_rich_glyph->breath_magnitude = rich_state->breath_magnitude;
            current_rich_glyph->breath_speed     = rich_state->breath_speed;
            current_rich_glyph->character        = current_character_string.data[0];
            current_rich_glyph->font_id          = rich_state->font_id;
        }
    }
#endif
    dialogue_ui.visible_characters     += 1;
    dialogue_ui.parse_character_cursor += 1;

    return fake_length - dialogue_ui.visible_characters;
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
        /* NOTE: this can be moved out when I'm "done" */
        f32 start_x_cursor        = dialogue_box_start_position.x + 30;
        f32 start_y_cursor        = dialogue_box_start_position.y + 50;
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
#endif

        if (dialogue_ui.visible_characters < text_length_without_dialogue_rich_markup_length(current_conversation_node->text)) {
            dialogue_ui.speak_timer += dt;

            if (dialogue_ui.speak_timer >= DIALOGUE_UI_CHARACTER_TYPE_TIMER) {
                conversation_ui_advance_character();
            }
        }
    }


    if (current_conversation_node->choice_count == 0) {
        draw_ui_breathing_text(framebuffer, v2f32(dialogue_box_start_position.x + 30, dialogue_box_start_position.y + dialogue_box_extents.y - 10), font2, 2, string_literal("(proceed)"), 0451, color32f32(1,1,1,1));

        if (is_action_pressed(INPUT_ACTION_CONFIRMATION)) {
            if (conversation_ui_advance_character()) {
                while (conversation_ui_advance_character()) {
                    (void)(0);
                }
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
