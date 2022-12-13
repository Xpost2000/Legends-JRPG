#ifndef RICH_TEXT_DEF_C
#define RICH_TEXT_DEF_C

struct rich_text_state {
    f32 tallest_glyph_height_on_line;

    f32 breath_magnitude;
    f32 breath_speed;
    s32 font_id;
    f32 text_scale;

    /* these are for dialogue */
    f32 delay_timer;
    f32 text_type_speed;
    f32 type_trauma;
};

struct rich_glyph {
    f32  scale;
    f32  breath_magnitude;
    f32  breath_speed;
    char character;
    s32  font_id;
};

local struct rich_glyph rich_text_parse_next_glyph(struct rich_text_state* rich_text_state, string text, s32* parse_character_cursor, bool skipping_mode);
local void parse_markup_details(struct rich_text_state* rich_text_state, string text_data, bool skipping_mode);
local s32 text_length_without_dialogue_rich_markup_length(string text);
struct rich_text_state rich_text_state_default(void);

local void game_ui_render_rich_text(struct software_framebuffer* framebuffer, string text, v2f32 xy);
local void game_ui_render_rich_text_wrapped(struct software_framebuffer* framebuffer, string text, v2f32 xy, f32 width);

#endif
