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
/* only thin lines */
void software_framebuffer_draw_line(struct software_framebuffer* framebuffer, v2f32 start, v2f32 end, union color32u8 rgba);


#endif