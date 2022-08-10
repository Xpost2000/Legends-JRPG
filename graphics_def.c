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

#define color32u8_WHITE  color32u8(255,255,255,255)
#define color32f32_WHITE color32f32(1,1,1,1)
#define color32f32_BLACK color32f32(0,0,0,1)
#define color32u8_BLACK  color32f32(0,0,0,255)

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

struct font_cache font_cache_load_bitmap_font(string filepath, s32 tile_width, s32 tile_height, s32 atlas_rows, s32 atlas_columns);
void              font_cache_free(struct font_cache* font_cache);
f32               font_cache_text_height(struct font_cache* font_cache);
f32               font_cache_text_width(struct font_cache* font_cache, string text, f32 scale);
f32               font_cache_calculate_height_of(struct font_cache* font_cache, string str, f32 width_bounds, f32 scale);

struct image_buffer image_buffer_load_from_file(string file_path);
void                image_buffer_write_to_disk(struct image_buffer* image, string as);
void                image_buffer_free(struct image_buffer* image);

/* For now to keep sanity, you have to dereference these handles yourself, and none of the
   existing API understands these apis, that comes a bit later. */
typedef struct image_id { s32 index; } image_id;
typedef struct font_id  { s32 index; } font_id;
/* todo make this a hashmap or something? */
struct graphics_assets {
    struct memory_arena* arena;

    u32                  font_capacity;
    u32                  font_count;
    u32                  image_count;
    u32                  image_capacity;
    struct image_buffer* images;
    struct font_cache*   fonts;

    /* 1-1 mapping with images. We don't hashmap yet. */
    string*              image_file_strings;
};

struct graphics_assets graphics_assets_create(struct memory_arena* arena, u32 font_limit, u32 image_limit);
void                   graphics_assets_finish(struct graphics_assets* assets);

image_id               graphics_assets_load_image(struct graphics_assets* assets, string path);
image_id               graphics_assets_get_image_by_filepath(struct graphics_assets* assets, string filepath);
font_id                graphics_assets_load_bitmap_font(struct graphics_assets* assets, string path, s32 tile_width, s32 tile_height, s32 atlas_rows, s32 atlas_columns);
struct font_cache*     graphics_assets_get_font_by_id(struct graphics_assets* assets, font_id font);

struct image_buffer*   graphics_assets_get_image_by_id(struct graphics_assets* assets, image_id image);

struct software_framebuffer software_framebuffer_create(struct memory_arena* arena, u32 width, u32 height);
void                        software_framebuffer_copy_into(struct software_framebuffer* target, struct software_framebuffer* source);

enum software_framebuffer_draw_image_ex_flags {
    SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_HORIZONTALLY = BIT(0),
    SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_VERTICALLY   = BIT(1),
};

enum blit_blend_mode {
    BLEND_MODE_NONE,
    BLEND_MODE_ALPHA,
    BLEND_MODE_ADDITIVE,
    BLEND_MODE_MULTIPLICATIVE,
    BLEND_MODE_COUNT,
};

void software_framebuffer_clear_buffer(struct software_framebuffer* framebuffer, union color32u8 rgba);
void software_framebuffer_draw_quad(struct software_framebuffer* framebuffer, struct rectangle_f32 destination, union color32u8 rgba, u8 blend_mode);
void software_framebuffer_draw_image_ex(struct software_framebuffer* framebuffer, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 src, union color32f32 modulation, u32 flags, u8 blend_mode);
void software_framebuffer_draw_image_ex_clipped(struct software_framebuffer* framebuffer, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 src, union color32f32 modulation, u32 flags, u8 blend_mode, struct rectangle_f32 clip_rect);
/* we do not have a draw glyph */
/* add layout draw_text */
void software_framebuffer_draw_text(struct software_framebuffer* framebuffer, struct font_cache* font, f32 scale, v2f32 xy, string text, union color32f32 modulation, u8 blend_mode);
/* TODO make command buffer version */
void software_framebuffer_draw_text_bounds(struct software_framebuffer* framebuffer, struct font_cache* font, f32 scale, v2f32 xy, f32 bounds_w, string cstring, union color32f32 modulation, u8 blend_mode);
void software_framebuffer_draw_text_bounds_centered(struct software_framebuffer* framebuffer, struct font_cache* font, f32 scale, struct rectangle_f32 bounds, string text, union color32f32 modulation, u8 blend_mode);
/* only thin lines */
void software_framebuffer_draw_line(struct software_framebuffer* framebuffer, v2f32 start, v2f32 end, union color32u8 rgba, u8 blend_mode);

void software_framebuffer_kernel_convolution_ex(struct memory_arena* arena, struct software_framebuffer* framebuffer, f32* kernel, s16 width, s16 height, f32 divisor, f32 blend_t, s32 passes);

/*
  This is intended for threading work, and kernel convolution samples from the neighbors which
  requires the entire unaltered image. I don't have enough stack space to copy the framebuffer every single time
  on the thread 

  (1.2 mb per thread to hold 640x480!!!)
  
  So we'll just pass one singular copy from the above function, to avoid the time to copy multiple slices of the framebuffer which
  would ruin the effect anyways.
*/
void software_framebuffer_kernel_convolution_ex_bounded(struct software_framebuffer before, struct software_framebuffer* framebuffer, f32* kernel, s16 kernel_width, s16 kernel_height, f32 divisor, f32 blend_t, s32 passes, struct rectangle_f32 clip);

#include "render_commands_def.c"
void software_framebuffer_render_commands(struct software_framebuffer* framebuffer, struct render_commands* commands);

#endif
