/* TODO I'd like to use IDs instead of pointers, but that requires a more developed system. */
#ifndef GRAPHICS_DEF_C
#define GRAPHICS_DEF_C

#include "common.c"

#define STBI_MALLOC(x)        system_heap_memory_allocate(x)
#define STBI_REALLOC(x, nsz)  system_heap_memory_reallocate(x, nsz)
#define STBI_FREE(x)          system_heap_memory_deallocate(x)

#define STBIW_MALLOC(x)        system_heap_memory_allocate(x)
#define STBIW_REALLOC(x, nsz)  system_heap_memory_reallocate(x, nsz)
#define STBIW_FREE(x)          system_heap_memory_deallocate(x)

#define STBTT_malloc(x, u)       ((void)(u), system_heap_memory_allocate(x))
#define STBTT_free(x, u)         ((void)(u), system_heap_memory_deallocate(x))

#define STBTT_memcpy(a, b, s)      memory_copy(b, a, s)
#define STBTT_memset(a, v, b)      memory_set8(a, b, v)
#define STBTT_assert(x)            assertion(x)

#define STBTT_strlen(x)            cstring_length(x)

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

/* (I do recognize the intent of the engine is to eventually support paletted rendering but that comes later.) */
/* the image is expected to be rgba32, and must be converted to as such before rendering. */
#define Image_Buffer_Base \
    u32 width;            \
    u32 height;           \
    union {               \
        u8* pixels;       \
        u32* pixels_u32;  \
    }
    
/* NOTE image_buffers and software framebuffers are binary compatible so you can reuse their functions on each other. */
struct image_buffer {
    Image_Buffer_Base;
};

/* always an RGBA32 bit framebuffer */
struct software_framebuffer {
    Image_Buffer_Base;
};

union color32u8 {
    struct { u8 r, g, b, a; };
    u8  rgba[4];
    u32 rgba_packed;
};

/* for percentage and modulations */
union color32f32 {
    struct { f32 r, g, b, a; };
    f32  rgba[4];
};

#define color32u8(R,G,B,A)  (union color32u8){.r  = R,.g=G,.b=B,.a=A}
#define color32f32(R,G,B,A) (union color32f32){.r = R,.g=G,.b=B,.a=A}

/* Structured to work for both ttf and raster fonts, as they are both cached here */
/* Built as a texture atlas. Should work with hardware acceleration if I'm feeling it */

/* TODO These atlases only support ASCII for now. */
/* Maybe forever? I don't plan on this thing having a long life cycle past this summer :/ */
enum font_cache_type {
    FONT_CACHE_ATLAS_FIXED_ASCII,
    FONT_CACHE_ATLAS_VARIABLE,
};
struct font_cache {
    /* atlas */
    Image_Buffer_Base;

    s8 type;
    union {
        struct {
            s32 tile_width;
            s32 tile_height;
            s32 atlas_rows;
            s32 atlas_cols;
        };
        struct rectangle_s16 glyphs[128];
    };
};

struct font_cache font_cache_load_bitmap_font(char* filepath, s32 tile_width, s32 tile_height, s32 atlas_rows, s32 atlas_columns);
void              font_cache_free(struct font_cache* font_cache);

struct image_buffer image_buffer_load_from_file(const char* file_path);
void                image_buffer_write_to_disk(struct image_buffer* image, const char* as);
void                image_buffer_free(struct image_buffer* image);

struct software_framebuffer software_framebuffer_create(struct memory_arena* arena, u32 width, u32 height);
void                        software_framebuffer_copy_into(struct software_framebuffer* target, struct software_framebuffer* source);

enum software_framebuffer_draw_image_ex_flags {
    SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_HORIZONTALLY = BIT(0),
    SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_VERTICALLY   = BIT(1),
};

void software_framebuffer_clear_buffer(struct software_framebuffer* framebuffer, union color32u8 rgba);
void software_framebuffer_draw_quad(struct software_framebuffer* framebuffer, struct rectangle_f32 destination, union color32u8 rgba);
void software_framebuffer_draw_image_ex(struct software_framebuffer* framebuffer, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 src, union color32f32 modulation, u32 flags);
/* we do not have a draw glyph */
void software_framebuffer_draw_text(struct software_framebuffer* framebuffer, struct font_cache* font, f32 scale, v2f32 xy, char* cstring, union color32f32 modulation);
/* only thin lines */
void software_framebuffer_draw_line(struct software_framebuffer* framebuffer, v2f32 start, v2f32 end, union color32u8 rgba);

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
            char*                text;
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
void render_commands_push_text(struct render_commands* commands, struct font_cache* font, f32 scale, v2f32 xy, char* cstring, union color32f32 rgba);
void software_framebuffer_render_commands(struct software_framebuffer* framebuffer, struct render_commands* commands);

void software_framebuffer_kernel_convolution(struct memory_arena* arena, struct software_framebuffer* framebuffer, f32* kernel, s16 width, s16 height);

#endif
