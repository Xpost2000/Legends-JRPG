/* 
   little IMGUI
   It's almost like for my sanity I might need a fuller UI. Who knows?
   
   This is obviously not a good implementation, it's very quick and dirty. But functional and surprisingly
   not broken.
   
   While this looks ugly as hell, it's usage is pretty smooth (just remove the obligatory software rendering params,
   and it'd actually looks sane.)
   
   True beauty is obviously on the outside, and not the inside.
*/

/* no layout stuff, mostly just simple widgets to play with */
/* also bad code */
bool _imgui_mouse_button_left_down        = false;
bool _imgui_mouse_button_right_down       = false;
bool _imgui_confirmed_flag_for_text_input = false;
bool _imgui_any_intersection              = false;
s32  _imgui_text_edit_index               = -1;
s32  _imgui_current_widget_index          = 0;

/* 1 - left click, 2 - right click. Might be weird if you're not expecting it... */
/* TODO correct this elsweyr. */
void EDITOR_imgui_end_frame(void) {
    _imgui_any_intersection     = false;
    _imgui_current_widget_index = 0;
    _imgui_confirmed_flag_for_text_input = false;
}
bool EDITOR_imgui_targetted_widget(s32 i) {
    if (i != -1) {
        _debugprintf("The chosen one is widget %d\n", _imgui_text_edit_index);
        if (_imgui_text_edit_index == i) {
            _debugprintf("I widget %d AM the chosen one. : )", i);
            return true;
        }

        _debugprintf("I widget %d am not the chosen one. : (", i);
    }
    
    return false;
}

s32 EDITOR_imgui_button(struct software_framebuffer* framebuffer, struct font_cache* font, struct font_cache* highlighted_font, f32 draw_scale, v2f32 position, string str) {
    _imgui_current_widget_index += 1;
    _debugprintf("I am widget %d I'm a button." , _imgui_current_widget_index);
    f32 text_height = font_cache_text_height(font) * draw_scale;
    f32 text_width  = font_cache_text_width(font, str, draw_scale);

    s32 mouse_positions[2];
    bool left_clicked, right_clicked;
    get_mouse_buttons(&left_clicked, 0, &right_clicked);
    get_mouse_location(mouse_positions, mouse_positions+1);

    struct rectangle_f32 button_bounding_box = rectangle_f32(position.x, position.y, text_width, text_height);
    struct rectangle_f32 interaction_bounding_box = rectangle_f32(mouse_positions[0], mouse_positions[1], 2, 2);

    if (rectangle_f32_intersect(button_bounding_box, interaction_bounding_box)) {
        _imgui_any_intersection = true;
        if (!_imgui_mouse_button_left_down & left_clicked)   _imgui_mouse_button_left_down  = true;
        if (!_imgui_mouse_button_right_down & right_clicked) _imgui_mouse_button_right_down = true;

        software_framebuffer_draw_text(framebuffer, highlighted_font, draw_scale, position, (str), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
        if (!left_clicked && _imgui_mouse_button_left_down) {
            _imgui_mouse_button_left_down = false;
            return 1;
        }
        if (!right_clicked && _imgui_mouse_button_right_down) {
            _imgui_mouse_button_right_down = false;
            return 2;
        }
    } else {
        software_framebuffer_draw_text(framebuffer, font, draw_scale, position, (str), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
    }
    
    return 0;
}

char _imgui_edit_buffer[512];
/* NOTE: may bug out with a few more interactions. I don't care for this case. I'll fix it I find time. */
#define Define_EDITOR_imgui_text_edit(type, formatstr, string_to_v_fn, intostringfn) \
    void EDITOR_imgui_text_edit_##type(struct software_framebuffer* framebuffer, struct font_cache* font, struct font_cache* highlighted_font, f32 draw_scale, v2f32 position, string label, type* value) { \
        _imgui_current_widget_index += 1;                               \
        s32 me = _imgui_current_widget_index;                           \
        _debugprintf("I am widget %d I serve " #type, me); \
        string str = {};                                                \
        if (EDITOR_imgui_targetted_widget(me)) {                          \
            str = string_from_cstring(format_temp("[EDIT]%.*s : %s", label.length, label.data, current_text_buffer())); \
        } else {                                                        \
            str = string_from_cstring(format_temp("%.*s : " formatstr, label.length, label.data, *value)); \
        }                                                               \
        if (EDITOR_imgui_button(framebuffer, font, highlighted_font, draw_scale, position, str) == 1) { \
            if (_imgui_text_edit_index == -1) {                         \
                _imgui_text_edit_index        = me; \
                _debugprintf("I %d(%d) have been selected", _imgui_text_edit_index, me); \
                intostringfn(*value, _imgui_edit_buffer,512);           \
                start_text_edit(_imgui_edit_buffer, cstring_length(_imgui_edit_buffer)); \
            }                                                           \
        }                                                               \
        if (is_key_pressed(KEY_RETURN) && !_imgui_confirmed_flag_for_text_input) { \
            _debugprintf("I want a new value!\n");                      \
            if (EDITOR_imgui_targetted_widget(me)) {                     \
                _imgui_confirmed_flag_for_text_input = true;            \
                _imgui_text_edit_index                                        = -1; \
                end_text_edit(_imgui_edit_buffer, array_count(_imgui_edit_buffer)); \
                *value = string_to_v_fn(_imgui_edit_buffer);            \
                _debugprintf("I widget %d have a new value of " formatstr, me, *value); \
            }                                                           \
        }                                                               \
    }

void IMGUI_inttostring_s32(s32 x, char* str, s32 len) {snprintf(str, len, "%d", x);}
void IMGUI_inttostring_s16(s16 x, char* str, s32 len) {snprintf(str, len, "%d", x);}
void IMGUI_inttostring_s8(s8 x, char* str, s32 len) {snprintf(str, len, "%d", x);}
void IMGUI_inttostring_u32(u32 x, char* str, s32 len) {snprintf(str, len, "%u", x);}
void IMGUI_inttostring_u16(u16 x, char* str, s32 len) {snprintf(str, len, "%u", x);}
void IMGUI_inttostring_u8(u8 x, char* str, s32 len) {snprintf(str, len, "%u", x);}
void IMGUI_floattostring(f32 x, char* str, s32 len) {snprintf(str, len, "%f", x);}

Define_EDITOR_imgui_text_edit(s8, "%d",  atoi, IMGUI_inttostring_s8);
Define_EDITOR_imgui_text_edit(u8, "%u",  atoi, IMGUI_inttostring_u8);
Define_EDITOR_imgui_text_edit(s16, "%d", atoi, IMGUI_inttostring_s16);
Define_EDITOR_imgui_text_edit(u16, "%u", atoi, IMGUI_inttostring_u16);
Define_EDITOR_imgui_text_edit(s32, "%d", atoi, IMGUI_inttostring_s32);
Define_EDITOR_imgui_text_edit(u32, "%u", atoi, IMGUI_inttostring_u32);
Define_EDITOR_imgui_text_edit(f32, "%f", atof, IMGUI_floattostring);
void EDITOR_imgui_text_edit_cstring(struct software_framebuffer* framebuffer, struct font_cache* font, struct font_cache* highlighted_font, f32 draw_scale, v2f32 position, string label, char* value, s32 max_len) {
    _imgui_current_widget_index += 1;
    s32 me = _imgui_current_widget_index;
    string str = {};
    if (EDITOR_imgui_targetted_widget(me)) {
        str = string_from_cstring(format_temp("[EDIT]%.*s : %s", label.length, label.data, current_text_buffer()));
    } else {
        str = string_from_cstring(format_temp("%.*s : %s", label.length, label.data, value));
    }
    if (EDITOR_imgui_button(framebuffer, font, highlighted_font, draw_scale, position, str) == 1) {
        if (_imgui_text_edit_index == -1) {
            _imgui_text_edit_index        = me;
            cstring_copy(value, _imgui_edit_buffer, cstring_length(value));
            start_text_edit(_imgui_edit_buffer, cstring_length(_imgui_edit_buffer));
        }
    }
    if (is_key_pressed(KEY_RETURN) && !_imgui_confirmed_flag_for_text_input) {
        if (EDITOR_imgui_targetted_widget(me)) {
            _imgui_confirmed_flag_for_text_input = true;
            _imgui_text_edit_index                                        = -1;
            end_text_edit(_imgui_edit_buffer, max_len);
            s32 min_len = max_len;
            if (min_len > cstring_length(_imgui_edit_buffer)) {
                min_len = cstring_length(_imgui_edit_buffer);
            }
            cstring_copy(_imgui_edit_buffer, value, min_len);
        }
    }
}

/* little IMGUI  */
