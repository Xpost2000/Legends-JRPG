/* 
   TODO:
   Actual Renderer System (render layers and drawing text, with a camera)
*/
#define STBI_MALLOC(x)       system_heap_memory_allocate(x)
#define STBI_REALLOC(x, nsz) system_heap_memory_reallocate(x, nsz)
#define STBI_FREE(x)         system_heap_memory_deallocate(x)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

/* (I do recognize the intent of the engine is to eventually support paletted rendering but that comes later.) */
/* the image is expected to be rgba32, and must be converted to as such before rendering. */
struct image_buffer {
    union {
        u8* pixels;
        u32* pixels_u32;
    };
    u32 width;
    u32 height;
};

struct image_buffer image_buffer_load_from_file(const char* file_path) {
    s32 width;
    s32 height;
    s32 components;

    u8* image_buffer = stbi_load(file_path, &width, &height, &components, 4);
    struct image_buffer result = (struct image_buffer) {
        .pixels = image_buffer,
        .width  = width,
        .height = height,
    };
    return result;
}

void image_buffer_free(struct image_buffer* image) {
    assertion(image->pixels);
    system_heap_memory_deallocate(image->pixels);
}

/* always an RGBA32 bit framebuffer */
struct software_framebuffer {
    u32 width;
    u32 height;
    u8* pixels;
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

/* we would like temporary arenas yes... */
struct software_framebuffer software_framebuffer_create(struct memory_arena* arena, u32 width, u32 height) {
    u8* pixels = memory_arena_push(arena, width * height * sizeof(u32));

    return (struct software_framebuffer) {
        .width  = width,
        .height = height,
        .pixels = pixels,
    };
}

void software_framebuffer_clear_buffer(struct software_framebuffer* framebuffer, union color32u8 rgba) {
    memory_set32(framebuffer->pixels, framebuffer->width * framebuffer->height * sizeof(u32), rgba.rgba_packed);
}

void software_framebuffer_draw_quad(struct software_framebuffer* framebuffer, struct rectangle_f32 destination, union color32u8 rgba) {
    s32 start_x = (s32)destination.x;
    s32 start_y = (s32)destination.y;
    s32 end_x   = (s32)(destination.x + destination.w);
    s32 end_y   = (s32)(destination.y + destination.h);


    u32* framebuffer_pixels_as_32 = (u32*)framebuffer->pixels;
    for (u32 y_cursor = start_y; y_cursor < end_y; ++y_cursor) {
        if (y_cursor >= 0 && y_cursor < framebuffer->height)
            for (u32 x_cursor = start_x; x_cursor < end_x; ++x_cursor) {
                if (x_cursor >= 0 && x_cursor < framebuffer->width) {
                    u32 stride = framebuffer->width;
#if 0
                    framebuffer_pixels_as_32[y_cursor * stride + x_cursor] = rgba.rgba_packed;
#else
                    {
                        float alpha = rgba.a / 255.0f;
                        framebuffer->pixels[y_cursor * stride * 4 + x_cursor * 4 + 0] = (framebuffer->pixels[y_cursor * stride * 4 + x_cursor * 4 + 0] * (1 - alpha)) + (rgba.r * alpha);
                        framebuffer->pixels[y_cursor * stride * 4 + x_cursor * 4 + 1] = (framebuffer->pixels[y_cursor * stride * 4 + x_cursor * 4 + 1] * (1 - alpha)) + (rgba.g * alpha);
                        framebuffer->pixels[y_cursor * stride * 4 + x_cursor * 4 + 2] = (framebuffer->pixels[y_cursor * stride * 4 + x_cursor * 4 + 2] * (1 - alpha)) + (rgba.b * alpha);
                    }
                    framebuffer->pixels[y_cursor * stride * 4 + x_cursor * 4 + 3] = 255;
#endif
                }
            }
    }
}

enum software_framebuffer_draw_image_ex_flags {
    SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_HORIZONTALLY = BIT(0),
    SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_VERTICALLY   = BIT(1),
};

void software_framebuffer_draw_image_ex(struct software_framebuffer* framebuffer, struct image_buffer image, struct rectangle_f32 destination, struct rectangle_f32 src, union color32f32 modulation, u32 flags) {
    if ((src.x == 0) && (src.y == 0) && (src.w == 0) && (src.h == 0)) {
        src.w = image.width;
        src.h = image.height;
    }

    f32 scale_ratio_w = (f32)src.w  / destination.w;
    f32 scale_ratio_h = (f32)src.h  / destination.h;

    s32 start_x = (s32)destination.x;
    s32 start_y = (s32)destination.y;
    s32 end_x   = (s32)(destination.x + destination.w);
    s32 end_y   = (s32)(destination.y + destination.h);

    u32* framebuffer_pixels_as_32 = (u32*)framebuffer->pixels;
    for (u32 y_cursor = start_y; y_cursor < end_y; ++y_cursor) {
        if (y_cursor >= 0 && y_cursor < framebuffer->height)
            for (u32 x_cursor = start_x; x_cursor < end_x; ++x_cursor) {
                if (x_cursor >= 0 && x_cursor < framebuffer->width) {
                    u32 stride       = framebuffer->width;
                    u32 image_stride = image.width;

                    s32 image_sample_x = floor((src.x + src.w) - ((end_x - x_cursor) * scale_ratio_w));
                    s32 image_sample_y = floor((src.y + src.h) - ((end_y - y_cursor) * scale_ratio_h));

                    if ((flags & SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_HORIZONTALLY))
                        image_sample_x = floor(((end_x - x_cursor) * scale_ratio_w) + src.x);

                    if ((flags & SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_VERTICALLY))
                        image_sample_y = floor(((end_y - y_cursor) * scale_ratio_h) + src.y);

                    union color32u8 sampled_pixel = (union color32u8) { .rgba_packed = image.pixels_u32[image_sample_y * image_stride + image_sample_x] };

                    sampled_pixel.r *= modulation.r;
                    sampled_pixel.g *= modulation.g;
                    sampled_pixel.b *= modulation.b;
                    sampled_pixel.a *= modulation.a;

#if 0
                    framebuffer_pixels_as_32[y_cursor * stride + x_cursor] = sampled_pixel.rgba_packed;
#else
                    {
                        float alpha = sampled_pixel.a / 255.0f;
                        framebuffer->pixels[y_cursor * stride * 4 + x_cursor * 4 + 0] = (framebuffer->pixels[y_cursor * stride * 4 + x_cursor * 4 + 0] * (1 - alpha)) + (sampled_pixel.r * alpha);
                        framebuffer->pixels[y_cursor * stride * 4 + x_cursor * 4 + 1] = (framebuffer->pixels[y_cursor * stride * 4 + x_cursor * 4 + 1] * (1 - alpha)) + (sampled_pixel.g * alpha);
                        framebuffer->pixels[y_cursor * stride * 4 + x_cursor * 4 + 2] = (framebuffer->pixels[y_cursor * stride * 4 + x_cursor * 4 + 2] * (1 - alpha)) + (sampled_pixel.b * alpha);
                    }
                    framebuffer->pixels[y_cursor * stride * 4 + x_cursor * 4 + 3] = 255;
#endif
                }
            }
    }

}
