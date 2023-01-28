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
        if (string_equal_case_insensitive(name, string_table_mapping_to_font_id[mapping_index].first)) {
            return string_table_mapping_to_font_id[mapping_index].result;
        }
    }

    return MENU_FONT_COLOR_GOLD;
}

local string game_script_formatting_preprocess_string(struct memory_arena* arena, string text) {
    /* thankfully this is pretty simple... */ 
    string result = {
        .data = memory_arena_push(arena, 0),
    };

#define Add_Character(S, C)                     \
    do {                                        \
        memory_arena_push(arena, 1);            \
        S.data[S.length++] = C;                   \
    } while(0);

    s32 character_index = 0;
    while (character_index < text.length) {
        if (text.data[character_index] == '$') {
            s32 start_of_format_result = character_index+1;
            character_index++;

            while (text.data[character_index] != '$' && character_index < text.length) {
                character_index++;
            }

            s32 end_of_format_result = character_index;
            character_index++;

            /*
              NOTE:
              This is a safeguard just in case arena == scratch_arena, as it would break the string I'm allocating there...

              Have to carefully pingpong between region areas, but it should be okay.
            */
            memory_arena_set_allocation_region_top(&scratch_arena); {
                string           format_result = string_slice(text, start_of_format_result, end_of_format_result);
                _debugprintf("Hi I'm decoding: %.*s", format_result.length, format_result.data);
                struct lisp_list markup_forms  = lisp_read_string_into_forms(&scratch_arena, format_result);

                for (s32 form_index = 0; form_index < markup_forms.count; ++form_index) {
                    struct lisp_form* current_form     = markup_forms.forms + form_index;
                    struct lisp_form  evaluated_form   = game_script_evaluate_form(&scratch_arena, game_state, current_form);
                    string            resulting_string = lisp_primitive_form_into_string(&scratch_arena, &evaluated_form);

                    memory_arena_set_allocation_region_bottom(&scratch_arena);
                    {
                        for (s32 resulting_string_index = 0; resulting_string_index < resulting_string.length; ++resulting_string_index) {
                            Add_Character(result, resulting_string.data[resulting_string_index]);
                        }
                    }
                    memory_arena_set_allocation_region_top(&scratch_arena);
                }
            } memory_arena_set_allocation_region_bottom(&scratch_arena);
        } else {
            Add_Character(result, text.data[character_index]);
            character_index++;
        }
    }
#undef Add_Character
    return result;
}

local void parse_markup_details(struct rich_text_state* rich_text_state, string text_data, bool skipping_mode) {
    /* lisp forms */ 
    struct lisp_list markup_forms = lisp_read_string_into_forms(&scratch_arena, text_data);

    /* NOTE: no error checking */
    for (s32 markup_form_index = 0; markup_form_index < markup_forms.count; ++markup_form_index) {
        struct lisp_form* current_form  = markup_forms.forms + markup_form_index;
        struct lisp_form* name_part     = lisp_list_nth(current_form, 0);
        struct lisp_form* argument_part = lisp_list_nth(current_form, 1);

        { /* Purely visual or non-affecting actions */
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
            } else if (lisp_form_symbol_matching(*name_part, string_literal("type_speed"))) {
                assertion(lisp_form_get_f32(*argument_part, &rich_text_state->text_type_speed) && "Bad type speed");
            }
        }

        if (!skipping_mode) { /* Affecting actions */
            if (lisp_form_symbol_matching(*name_part, string_literal("delay_timer"))) {
                assertion(lisp_form_get_f32(*argument_part, &rich_text_state->delay_timer) && "Bad delay timer");
            } else if (lisp_form_symbol_matching(*name_part, string_literal("type_trauma"))) {
                assertion(lisp_form_get_f32(*argument_part, &rich_text_state->type_trauma) && "Bad trauma value");
            }
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
                        character_index--;
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

struct rich_text_state rich_text_state_default(void) {
    return (struct rich_text_state) {
        .breath_speed     = 0,
        .breath_magnitude = 0,
        .delay_timer      = 0,
        .text_type_speed  = DIALOGUE_UI_CHARACTER_TYPE_TIMER,
        .font_id          = MENU_FONT_COLOR_YELLOW,
#ifdef EXPERIMENTAL_320
        .text_scale       = 1,
#else
        .text_scale       = 2,
#endif
        .type_trauma      = 0,
    };
}

local struct rich_glyph rich_text_parse_next_glyph(struct rich_text_state* rich_text_state, string text, s32* parse_character_cursor, bool skipping_mode) {
    struct rich_glyph result = {};

    if (text.data[*parse_character_cursor] == '\\') {
        (*parse_character_cursor)++;
    } else if (text.data[*parse_character_cursor] == '[') {
        s32 bracket_count = 1;

        *parse_character_cursor += 1;

        s32 start_of_rich_markup = *parse_character_cursor;

        while (bracket_count > 0) {
            if (text.data[*parse_character_cursor] == '[') {
                bracket_count += 1;
            } else if (text.data[*parse_character_cursor] == ']') {
                bracket_count -= 1;
            }

            *parse_character_cursor += 1;
        }

        s32 end_of_rich_markup = (*parse_character_cursor)-1;

        string markup_text = string_slice(text, start_of_rich_markup, end_of_rich_markup);
        parse_markup_details(rich_text_state, markup_text, skipping_mode);
    }

    {
        {
            struct rich_text_state* rich_state               = rich_text_state;
            bool                    skip_glyph               = false;
            string                  current_character_string = string_slice(text, *parse_character_cursor, (*parse_character_cursor)+1);

            result.scale            = rich_state->text_scale;
            result.breath_magnitude = rich_state->breath_magnitude;
            result.breath_speed     = rich_state->breath_speed;
            result.character        = current_character_string.data[0];
            result.font_id          = rich_state->font_id;
        }
    }

    return result;
}

local void game_ui_render_rich_text_wrapped(struct software_framebuffer* framebuffer, string text, v2f32 xy, f32 width) {
    struct rich_glyph*     rich_text        = memory_arena_push(&scratch_arena, sizeof(*rich_text) * 2048);
    s32                    rich_text_length = 0;
    struct rich_text_state rich_text_state  = rich_text_state_default();
    {
        s32 parse_character_cursor = 0;
        s32 fake_length = text_length_without_dialogue_rich_markup_length(text);

        while (parse_character_cursor < text.length) {
            {
                {
                    struct rich_glyph* current_rich_glyph = &rich_text[rich_text_length++];
                    assertion(rich_text_length <= 2048 && "Your text is too big!");
                    *current_rich_glyph = rich_text_parse_next_glyph(&rich_text_state, text, &parse_character_cursor, false);
                }
            }
            parse_character_cursor++;
        }
    }

    f32 x_cursor              = xy.x;
    f32 y_cursor              = xy.y;
    f32 start_x_cursor        = x_cursor;
    f32 start_y_cursor        = y_cursor;
    f32 tallest_glyph_on_line = 0;

    for (s32 rich_glyph_index = 0; rich_glyph_index < rich_text_length; ++rich_glyph_index) {
        struct rich_glyph* current_glyph          = rich_text + rich_glyph_index;
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

                for (; word_end < rich_text_length; ++word_end) {
                    if (is_whitespace(rich_text[word_end].character)) {
                        break;
                    }
                }

            }

            {
                for (s32 glyph_index = word_start; glyph_index < word_end; ++glyph_index) {
                    struct rich_glyph* current_glyph          = rich_text + glyph_index;
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

        bool is_newline       = glyph_character == '\n';
        bool should_wrap_line = width != 0 && (x_cursor-start_x_cursor+estimated_width_of_current_word) > width;

        if (is_newline || should_wrap_line) {
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
}

local void game_ui_render_rich_text(struct software_framebuffer* framebuffer, string text, v2f32 xy) {
    game_ui_render_rich_text_wrapped(framebuffer, text, xy, 0);
}
