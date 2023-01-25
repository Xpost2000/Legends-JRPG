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
enum storyboard_instruction_type {
    STORYBOARD_INSTRUCTION_LINE,
    STORYBOARD_INSTRUCTION_SPACER, /* will space by N font heights */
    STORYBOARD_INSTRUCTION_WAIT,
    STORYBOARD_INSTRUCTION_WAIT_FOR_CONTINUE,
    STORYBOARD_SET_FONT_ID,
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
    string text;
};

struct storyboard_page {
    s32                            instruction_count;
    struct storyboard_instruction* instructions;
    s32                            instruction_cursor;
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
    struct storyboard_page* pages;
};

struct storyboard_page_list storyboard_page_list_reserve(struct memory_arena* arena, s32 capacity) {
    struct storyboard_page_list result = {};
    result.pages                       = memory_arena_push(arena, capacity * sizeof(*result.pages));
    result.count                       = capacity; /* we don't need to really "allocate anything..." */
    return result;
}

struct {
    s32 page_count; /* strings are expected to pages. */
    struct storyboard_page* pages;

    s32 current_page;
    s32 character_index;
    f32 character_timer;

    f32 timer;
    f32 y_cursor;

    /* current display objects */
    struct storyboard_line_object_list line_objects;
} storyboard;

void storyboard_next_page(void) {
    storyboard.current_page    += 1;
    storyboard.character_timer  = 0;
    storyboard.character_index  = 0;
    storyboard.timer            = 0;

    if (storyboard.current_page >= storyboard.page_count) {
        storyboard_active = false;
    }
}
void start_storyboard(void) {
    storyboard_active          = true;
    storyboard.timer           = 0;
    storyboard.current_page    = 0;
    storyboard.page_count      = 0;
    storyboard.character_timer = 0;
    storyboard.character_index = 0;
}

void initialize_storyboard(struct memory_arena* arena) {
    storyboard_arena = memory_arena_push_sub_arena(arena, Kilobyte(16));
}

void load_storyboard_page(struct lisp_form* form) {
    
}

/* I'm a bit too lazy to fully animate this right now so I'm just going to blurt some text. We'll animate it more later */
s32 game_display_and_update_storyboard(struct software_framebuffer* framebuffer, f32 dt) {
    if (storyboard_active) {
#if 0
        software_framebuffer_draw_quad(framebuffer, rectangle_f32(0,0,framebuffer->width,framebuffer->height), color32u8(0,0,0,255), BLEND_MODE_ALPHA);

        struct storyboard_page* current_page = &storyboard.pages[storyboard.current_page];

        const f32          NORMAL_FONT_SCALE = 2;
        struct font_cache* font1             = game_get_font(MENU_FONT_COLOR_STEEL);
        {
            if (storyboard.character_index == current_page->string.length-1) {
                if (storyboard.timer >= current_page->linger_time) {
                    storyboard_next_page();
                }

                storyboard.timer += dt;
            } else {
                if (storyboard.character_timer >= STORYBOARD_READ_SPEED) {
                    storyboard.character_timer     = 0;
                    storyboard.character_index += 1;
                }

                f32 modifier = 1;

                if (any_key_down()) {
                    modifier = 2.5f;
                }

                storyboard.character_timer += dt * modifier;
            }
        }

        software_framebuffer_draw_text_bounds(framebuffer, font1, NORMAL_FONT_SCALE, v2f32(10,50), framebuffer->width-10,
                                              string_slice(current_page->string, 0, storyboard.character_index), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
#endif
        return 1;
    } else {
        return 0;
    }
}
