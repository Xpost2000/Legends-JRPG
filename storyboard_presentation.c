/*
 A basic type of cutscene

 for text exposition, and some moving image? or something.

 This is probably not going to be used very frequently (in-fact outside of the intro/end it might be of no use... So this is going to only have one
 specific format of appearance and that's about it...)
*/


/*
  It's kind of like a VM since it compiles the lisp format into a bytecode,
  that's executed.

  NOTE: The storyboarder does not know how to fade.

  You're actually supposed to fade separately!
*/

#define STORYBOARD_CHARACTER_TYPE_TIMER (0.065)

enum storyboard_instruction_type {
    STORYBOARD_INSTRUCTION_LINE,
    STORYBOARD_INSTRUCTION_SPACER, /* will space by N font heights */
    STORYBOARD_INSTRUCTION_WAIT,
    STORYBOARD_INSTRUCTION_WAIT_FOR_CONTINUE,
    STORYBOARD_INSTRUCTION_SET_FONT_ID,
#if 0
    STORYBOARD_CENTER_NEXT_LINE,
#endif
};

struct storyboard_instruction_line {
    string line;
};

struct storyboard_instruction_set_font_id {
    s32 font_id;
};

struct storyboard_instruction_spacer {
    s32 units;
};

struct storyboard_instruction_wait {
    f32 time;
};

local struct memory_arena storyboard_arena = {};
struct storyboard_instruction {
    s32 type;
    union {
        struct storyboard_instruction_set_font_id set_font;
        struct storyboard_instruction_line        line;
        struct storyboard_instruction_spacer      spacer;
        struct storyboard_instruction_wait        wait;
    };
};
struct storyboard_line {
    f32    y_cusor;
    f32    timer;
    s32    font_id;
    string text;
};

struct storyboard_page {
    s32                            instruction_count;
    struct storyboard_instruction* instructions;
};

/* runtime constructs  */
struct storyboard_line_object {
    string text;
    s32    font_id;
    f32    x;
    f32    y;

    s32    shown_characters;
};

struct storyboard_line_object_list {
    struct storyboard_line_object* lines;
    s32                            count;
    s32                            capacity;
};

struct storyboard_line_object_list storyboard_line_object_list_reserve(struct memory_arena* arena, s32 capacity) {
    struct storyboard_line_object_list result = {};
    result.lines    = memory_arena_push(arena, sizeof(*result.lines) * capacity);
    result.capacity = capacity;
    return result;
}

struct storyboard_line_object* storyboard_line_object_list_alloc(struct storyboard_line_object_list* list) {
    assertion(list->count < list->capacity && "Too many runtime line objects allocated!");
    struct storyboard_line_object* result = &list->lines[list->count++];
    return result;
}

void storyboard_line_object_list_clear(struct storyboard_line_object_list* list) {
    list->count = 0;
}

/* end of runtime constructs */

struct storyboard_page_list {
    s32                     count;
    s32                     capacity;
    struct storyboard_page* pages;
};

struct storyboard_page_list storyboard_page_list_reserve(struct memory_arena* arena, s32 capacity) {
    struct storyboard_page_list result = {};
    result.pages                       = memory_arena_push(arena, capacity * sizeof(*result.pages));
    result.count                       = 0; /* we don't need to really "allocate anything..." */
    result.capacity                    = capacity;
    return result;
}

struct storyboard_page* storyboard_page_list_allocate_page(struct storyboard_page_list* list) {
    struct storyboard_page* result = &list->pages[list->count++];
    return result;
}

struct {
    struct storyboard_page_list pages;

    s32 current_page;
    s32 character_index;
    f32 character_timer;

    f32 timer;

    /* current context objects */
    f32  y_cursor;
    s32  currently_bound_font_id;
    f32  wait_timer;
    f32  character_type_timer;
    bool wait_for_continue;
    s32  instruction_cursor;
    s32  current_line_index;

    /* current display objects */
    struct storyboard_line_object_list line_objects;
} storyboard;

void storyboard_next_page(void) {
    storyboard.current_page    += 1;
    storyboard.character_timer  = 0;
    storyboard.character_index  = 0;
    storyboard.timer            = 0;

    storyboard.currently_bound_font_id = MENU_FONT_COLOR_STEEL;
    storyboard.y_cursor                = 0;
    storyboard.wait_timer              = 0;
    storyboard.character_type_timer    = 0;
    storyboard.instruction_cursor      = 0;
    storyboard.current_line_index      = 0;

    storyboard_line_object_list_clear(&storyboard.line_objects);
    _debugprintf("Next page");

    {
        if (storyboard.current_page >= storyboard.pages.count) {
            storyboard_active = false;
            _debugprintf("Storyboard finished.");
        }
    }
}
void start_storyboard(void) {
    storyboard_active          = true;
    storyboard.timer           = 0;
    storyboard.current_page    = 0;
    storyboard.character_timer = 0;
    storyboard.character_index = 0;

    _debugprintf("Begin storyboard");
    memory_arena_clear(&storyboard_arena);
}

void initialize_storyboard(struct memory_arena* arena) {
    storyboard_arena        = memory_arena_push_sub_arena(arena, Kilobyte(16));
    storyboard.line_objects = storyboard_line_object_list_reserve(arena, 256);
}

void storyboard_reserve_pages(s32 count) {
    storyboard.pages = storyboard_page_list_reserve(&storyboard_arena, count);
}

void load_storyboard_page(struct lisp_form* form) {
    struct storyboard_page* new_page = storyboard_page_list_allocate_page(&storyboard.pages);
    _debugprintf("Loading storyboard page.");
    {
        new_page->instructions = memory_arena_push(&storyboard_arena, sizeof(*new_page->instructions) * (form->list.count-1));
        for (s32 form_object_index = 1; form_object_index < form->list.count; ++form_object_index) {
            struct lisp_form* current_object_form = form->list.forms + form_object_index;
            struct lisp_form* object_header       = lisp_list_nth(current_object_form, 0);

            struct storyboard_instruction* current_instruction = &new_page->instructions[new_page->instruction_count++];

            if (lisp_form_symbol_matching(*object_header, string_literal("wait-for-continue"))) {
                current_instruction->type = STORYBOARD_INSTRUCTION_WAIT_FOR_CONTINUE;
                _debugprintf("Found wait-for-continue instruction");
            } else if (lisp_form_symbol_matching(*object_header, string_literal("line"))) {
                current_instruction->type = STORYBOARD_INSTRUCTION_LINE;
                struct lisp_form* text_line = lisp_list_nth(current_object_form, 1);
                string str = {};
                assertion(lisp_form_get_string(*text_line, &str) && "Text line is not a string");
                current_instruction->line.line = string_clone(&storyboard_arena, str);
                _debugprintf("Found line instruction");
            } else if (lisp_form_symbol_matching(*object_header, string_literal("wait"))) {
                current_instruction->type = STORYBOARD_INSTRUCTION_WAIT;
                struct lisp_form* wait_time = lisp_list_nth(current_object_form, 1);
                assertion(lisp_form_get_f32(*wait_time, &current_instruction->wait.time) && "Wait time parameter is not a float-convertible value");
                _debugprintf("Found wait instruction");
            } else if (lisp_form_symbol_matching(*object_header, string_literal("spacer"))) {
                current_instruction->type = STORYBOARD_INSTRUCTION_SPACER;
                struct lisp_form* units_form = lisp_list_nth(current_object_form, 1);

                s32 units = 1;
                if (units_form) {
                    assertion(lisp_form_get_s32(*units_form, &units) && "Spacer units parameter is not a integer-convertible value");
                }
                current_instruction->spacer.units = units;
                _debugprintf("Found spacer instruction");
            } else if (lisp_form_symbol_matching(*object_header, string_literal("set-font-id"))) {
                current_instruction->type = STORYBOARD_INSTRUCTION_SET_FONT_ID;
                struct lisp_form* font_id_form = lisp_list_nth(current_object_form, 1);

                string str = {};
                assertion(lisp_form_get_string(*font_id_form, &str) && "font id form is not a string");

                s32 font_id = fontname_string_to_id(str);
                current_instruction->set_font.font_id = font_id;
                _debugprintf("Found set font_id instruction");
            }
        }
    }
}

/* I'm a bit too lazy to fully animate this right now so I'm just going to blurt some text. We'll animate it more later */
s32 game_display_and_update_storyboard(struct software_framebuffer* framebuffer, f32 dt) {
    if (storyboard_active) {
        struct storyboard_page* current_page = &storyboard.pages.pages[storyboard.current_page];

        s32 instruction_count  = current_page->instruction_count;

        struct font_cache* font             = game_get_font(storyboard.currently_bound_font_id);
        f32                text_scale       = 2;
        f32                font_base_height = font_cache_text_height(font) * text_scale;

        if (storyboard.instruction_cursor < instruction_count) {
            struct storyboard_instruction* current_instruction = current_page->instructions + storyboard.instruction_cursor;

            switch (current_instruction->type) {
                case STORYBOARD_INSTRUCTION_LINE: {
                    _debugprintf("Constructing new line object");
                    struct storyboard_instruction_line* line = &current_instruction->line;
                    struct storyboard_line_object* new_line = storyboard_line_object_list_alloc(&storyboard.line_objects);

                    {
                        new_line->text             = line->line;
                        new_line->font_id          = storyboard.currently_bound_font_id;
                        new_line->x                = 0;
                        new_line->y                = storyboard.y_cursor;
                        new_line->shown_characters = 0;
                    }

                    storyboard.y_cursor += font_base_height;
                } break;
                case STORYBOARD_INSTRUCTION_SPACER: {
                    _debugprintf("Adding spacer");
                    struct storyboard_instruction_spacer* spacer = &current_instruction->spacer;
                    storyboard.y_cursor += spacer->units * font_base_height;
                } break;
                case STORYBOARD_INSTRUCTION_WAIT: {
                    struct storyboard_instruction_wait* wait = &current_instruction->wait;
                    _debugprintf("Set new wait for %3.3f", wait->time);
                    storyboard.wait_timer = wait->time;
                } break;
                case STORYBOARD_INSTRUCTION_WAIT_FOR_CONTINUE: {
                    _debugprintf("Set new wait for continue");
                    storyboard.wait_for_continue = true;
                } break;
                case STORYBOARD_INSTRUCTION_SET_FONT_ID: {
                    struct storyboard_instruction_set_font_id* set_font_id = &current_instruction->set_font;
                    storyboard.currently_bound_font_id = set_font_id->font_id;
                    _debugprintf("Set new font id to %d", storyboard.currently_bound_font_id);
                } break;
            }
        } else {
            storyboard_next_page();
        }

        {
            bool action_confirm = is_action_pressed(INPUT_ACTION_CONFIRMATION);

            if (storyboard.wait_for_continue) {
                if (action_confirm) {
                    storyboard.instruction_cursor += 1;
                }
            } else if (storyboard.wait_timer > 0) {
                storyboard.wait_timer -= dt;
            } else {
                struct storyboard_line_object* last_line = storyboard.line_objects.lines + (storyboard.current_line_index);

                bool allow_instruction_cursor_advance = false;
                {
                    if (last_line->shown_characters >= last_line->text.length) {
                        allow_instruction_cursor_advance = true;
                        storyboard.current_line_index += 1;
                        _debugprintf("current line is finished.");
                    }
                }

                if (allow_instruction_cursor_advance) {
                    _debugprintf("Allowing instruction cursor advance");
                    storyboard.instruction_cursor += 1;
                }
            }

            for (s32 line_object_index = 0; line_object_index < storyboard.line_objects.count; ++line_object_index) {
                struct storyboard_line_object* line           = storyboard.line_objects.lines + line_object_index;
                struct font_cache*             line_font      = game_get_font(line->font_id);
                string                         visible_string = string_slice(line->text, 0, line->shown_characters);

                if (storyboard.character_type_timer <= 0) {
                    if (line->shown_characters < line->text.length)
                    line->shown_characters          += 1;
                    storyboard.character_type_timer  = STORYBOARD_CHARACTER_TYPE_TIMER;
                } else {
                    storyboard.character_type_timer -= dt;
                }

                software_framebuffer_draw_text_bounds(
                    framebuffer,
                    line_font,
                    text_scale,
                    v2f32(30 + line->x, 30 + line->y),
                    framebuffer->width-30,
                    visible_string,
                    color32f32(1,1,1,1),
                    BLEND_MODE_ALPHA
                );
            }
        }
        return 1;
    } else {
        return 0;
    }
}
