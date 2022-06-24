#ifndef RENDER_COMMANDS_DEF_C
#define RENDER_COMMANDS_DEF_C

#include "camera_def.c"

/* This is a pretty generous number, pretty sure only particles would hit this...  */
/* should match the operations found above */
#define RENDER_COMMANDS_MAX (4096)
enum render_command_type{
    RENDER_COMMAND_DRAW_QUAD,
    RENDER_COMMAND_DRAW_IMAGE,
    RENDER_COMMAND_DRAW_TEXT,
    RENDER_COMMAND_DRAW_LINE
};
#define ALWAYS_ON_TOP (INFINITY)
struct render_command {
    s16 type;
    /* easier to mix using a floating point value. */
    f32 sort_key;

    union {
        union color32f32 modulation;
        union color32u8  modulation_u8;
    };

    struct rectangle_f32 destination;
    struct rectangle_f32 source;

    v2f32 start;
    v2f32 end;

    v2f32 xy;
    f32 scale;

    union {
        struct {
            struct font_cache*   font;
            string               text;
        };
        struct image_buffer* image;
    };

    u32 flags;
};

struct render_commands {
    struct camera         camera;
    u8                    should_clear_buffer;
    union color32u8       clear_buffer_color;
    struct render_command commands[RENDER_COMMANDS_MAX];
    s32                   command_count;
};

struct render_commands render_commands(struct camera camera);

void render_commands_push_quad(struct render_commands* commands, struct rectangle_f32 destination, union color32u8 rgba);
void render_commands_push_image(struct render_commands* commands, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 source, union color32f32 rgba, u32 flags);
void render_commands_push_line(struct render_commands* commands, v2f32 start, v2f32 end, union color32u8 rgba);
void render_commands_push_text(struct render_commands* commands, struct font_cache* font, f32 scale, v2f32 xy, string cstring, union color32f32 rgba);
void render_commands_clear(struct render_commands* commands);

#endif
