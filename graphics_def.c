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
struct software_framebuffer;
typedef union color32f32 (*shader_fn)(struct software_framebuffer* framebuffer, union color32f32 source_pixel, v2f32 pixel_position, void* context);

/*
  While this isn't strictly necessary for the graphics part,
  this is an auxiliary data structure used to keep my lighting happening in one pass.

  if a pixel is marked as 255, we will not light it, and draw it as full bright.
  Otherwise behavior is just lighting. Not sure what to do with non-255 values.

  Okay it really seems like I'm just using this as a stencil buffer...
*/
enum lightmask_draw_blend {
    LIGHTMASK_BLEND_NONE,
    LIGHTMASK_BLEND_OR,
};
struct lightmask_buffer {
    uint8_t* mask_buffer;
    u32 width;
    u32 height;
};
/*
  NOTE: there is no render command/queueing system for the lightmask since it does not matter
  what order they go in.
 */
struct lightmask_buffer lightmask_buffer_create(u32 buffer_width, u32 buffer_height);
void                    lightmask_buffer_clear(struct lightmask_buffer* buffer);
void                    lightmask_buffer_blit_image_clipped(struct lightmask_buffer* buffer, struct rectangle_f32 clip_rect, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 src, u8 flags, u8 blend_mode, u8 v);
void                    lightmask_buffer_blit_rectangle_clipped(struct lightmask_buffer* buffer, struct rectangle_f32 clip_rect, struct rectangle_f32 destination, u8 blend_mode, u8 v);
void                    lightmask_buffer_blit_image(struct lightmask_buffer* buffer, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 src, u8 flags, u8 blend_mode, u8 v);
void                    lightmask_buffer_blit_rectangle(struct lightmask_buffer* buffer, struct rectangle_f32 destination, u8 blend_mode, u8 v);
bool                    lightmask_buffer_is_lit(struct lightmask_buffer* buffer, s32 x, s32 y);
f32                     lightmask_buffer_lit_percent(struct lightmask_buffer* buffer, s32 x, s32 y);
void                    lightmask_buffer_finish(struct lightmask_buffer* buffer);

struct software_framebuffer {
    Image_Buffer_Base;

    shader_fn active_shader;
    void*     active_shader_context;

    s32 scissor_x;
    s32 scissor_y;
    s32 scissor_w;
    s32 scissor_h;
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

union color32f32 color32u8_to_color32f32(union color32u8 source) {
    return color32f32(source.r / 255.0f, source.g / 255.0f, source.b / 255.0f, source.a / 255.0f);
}

union color32u8 color32f32_to_color32u8(union color32f32 source) {
    return color32u8(source.r * 255, source.g * 255, source.b * 255, source.a * 255);
}

#define color32u8_WHITE  color32u8(255,255,255,255)
#define color32u8_BLACK  color32u8(0,0,0,255)
#define color32f32_WHITE color32f32(1,1,1,1)
#define color32f32_BLACK color32f32(0,0,0,1)

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

/* no longer using memory arenas as we are dynamically able to resize this. */
struct software_framebuffer software_framebuffer_create(u32 width, u32 height);
/* Used for creating temporaries. */
struct software_framebuffer software_framebuffer_create_from_arena(struct memory_arena* arena, u32 width, u32 height);
/* only from non-arenas. Which is okay */
void                        software_framebuffer_finish(struct software_framebuffer* framebuffer);
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

void software_framebuffer_clear_scissor(struct software_framebuffer* framebuffer);
void software_framebuffer_set_scissor(struct software_framebuffer* framebuffer, s32 x, s32 y, s32 w, s32 h);
void software_framebuffer_clear_buffer(struct software_framebuffer* framebuffer, union color32u8 rgba);
void software_framebuffer_draw_quad(struct software_framebuffer* framebuffer, struct rectangle_f32 destination, union color32u8 rgba, u8 blend_mode);
void software_framebuffer_draw_image_ex(struct software_framebuffer* framebuffer, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 src, union color32f32 modulation, u32 flags, u8 blend_mode);
void software_framebuffer_draw_image_ex_clipped(struct software_framebuffer* framebuffer, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 src, union color32f32 modulation, u32 flags, u8 blend_mode, struct rectangle_f32 clip_rect, shader_fn shader, void* shader_ctx);
/* add layout draw_text */
void software_framebuffer_draw_text(struct software_framebuffer* framebuffer, struct font_cache* font, f32 scale, v2f32 xy, string text, union color32f32 modulation, u8 blend_mode);
void software_framebuffer_draw_glyph(struct software_framebuffer* framebuffer, struct font_cache* font, f32 scale, v2f32 xy, char glyph, union color32f32 modulation, u8 blend_mode);
/* TODO make command buffer version */
void software_framebuffer_draw_text_bounds(struct software_framebuffer* framebuffer, struct font_cache* font, f32 scale, v2f32 xy, f32 bounds_w, string cstring, union color32f32 modulation, u8 blend_mode);
void software_framebuffer_draw_text_bounds_centered(struct software_framebuffer* framebuffer, struct font_cache* font, f32 scale, struct rectangle_f32 bounds, string text, union color32f32 modulation, u8 blend_mode);
/* only thin lines */
void software_framebuffer_draw_line(struct software_framebuffer* framebuffer, v2f32 start, v2f32 end, union color32u8 rgba, u8 blend_mode);

void software_framebuffer_kernel_convolution_ex(struct memory_arena* arena, struct software_framebuffer* framebuffer, f32* kernel, s16 width, s16 height, f32 divisor, f32 blend_t, s32 passes);

void software_framebuffer_run_shader(struct software_framebuffer* framebuffer, struct rectangle_f32 src_rect, shader_fn shader, void* context);

/* NOTE:  */
void software_framebuffer_use_shader(struct software_framebuffer* framebuffer, shader_fn shader, void* context);

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
