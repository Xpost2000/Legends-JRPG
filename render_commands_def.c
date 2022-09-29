#ifndef RENDER_COMMANDS_DEF_C
#define RENDER_COMMANDS_DEF_C

#include "camera_def.c"

enum render_command_type{
    RENDER_COMMAND_DRAW_QUAD,
    RENDER_COMMAND_DRAW_IMAGE,
    RENDER_COMMAND_DRAW_TEXT,
    RENDER_COMMAND_DRAW_LINE
};
#define ALWAYS_ON_TOP (INFINITY)
struct render_command {
    /* easier to mix using a floating point value. */
    shader_fn shader;
    void*     shader_ctx;

    union {
        struct {
            struct font_cache*   font;
            string               text;
        };
        struct image_buffer* image;
    };

    struct rectangle_f32 destination;
    struct rectangle_f32 source;
    union {
        v2f32 start;
        v2f32 xy;
    };

    v2f32 end;

    f32 scale;
    f32 sort_key;
    u32 flags;
    union {
        union color32u8  modulation_u8;
    };

    s16 type;
    u8  blend_mode;
};

struct render_commands {
    struct camera          camera;
    u8                     should_clear_buffer;
    union color32u8        clear_buffer_color;
    struct render_command* commands;
    s32                    command_count;
    s32                    command_capacity;
};

struct render_commands render_commands(struct memory_arena* arena, s32 capacity, struct camera camera);

void render_commands_push_quad(struct render_commands* commands, struct rectangle_f32 destination, union color32u8 rgba, u8 blend_mode);
void render_commands_push_image(struct render_commands* commands, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 source, union color32f32 rgba, u32 flags, u8 blend_mode);
void render_commands_push_line(struct render_commands* commands, v2f32 start, v2f32 end, union color32u8 rgba, u8 blend_mode);
void render_commands_push_text(struct render_commands* commands, struct font_cache* font, f32 scale, v2f32 xy, string cstring, union color32f32 rgba, u8 blend_mode);

/* NOTE: weird backwards compatible method... Just changes the last shader */
/* This is probably how opengl evolved. Anyways let's hope I change this to be more proper. Chances are probably not */
void render_commands_set_shader(struct render_commands* commands, shader_fn shader, void* context);
void render_commands_clear(struct render_commands* commands);

#endif
