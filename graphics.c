/* 
   NOTE:
   
   consider making this easier for multithreading by allowing things to blit in tiled sets which
   allows for multithreading.
*/
#include "graphics_def.c"

/* doesn't take an allocator because of the way stb image works... */
struct image_buffer image_buffer_load_from_file(string filepath) {
    s32 width;
    s32 height;
    s32 components;

#ifndef EXPERIMENTAL_VFS
    u8* image_buffer = stbi_load(filepath.data, &width, &height, &components, 4);
#else
    /* STBIDEF stbi_uc *stbi_load_from_memory   (stbi_uc           const *buffer, int len   , int *x, int *y, int *channels_in_file, int desired_channels); */
    u8* image_buffer = NULL;
    {
        struct file_buffer buffer = read_entire_file(memory_arena_allocator(&game_arena), filepath);
        _debugprintf("%p, %d", buffer.buffer, (s32)buffer.length);
        image_buffer = stbi_load_from_memory(buffer.buffer, buffer.length, &width, &height, &components, 4);
        file_buffer_free(&buffer);
    }
#endif
    _debugprintf("tried to load: \"%.*s\"", filepath.length, filepath.data);
    if (!image_buffer) {
        _debugprintf("Failed to load \"%.*s\"", filepath.length, filepath.data);
    }
    assertion(image_buffer && "image load failed!");
    struct image_buffer result = (struct image_buffer) {
        .pixels = image_buffer,
        .width  = width,
        .height = height,
    };
    return result;
}

void image_buffer_write_to_disk(struct image_buffer* image, string as) {
    char filename[256] = {};
#if 0
    snprintf(filename, 256, "%s.bmp", as.data);
    stbi_write_bmp(filename, image->width, image->height, 4, image->pixels);
#else
    snprintf(filename, 256, "%s.jpg", as.data);
    stbi_write_jpg(filename, image->width, image->height, 4, image->pixels, 3);
#endif
    _debugprintf("screenshot produced.");
}

void image_buffer_free(struct image_buffer* image) {
    if (image->pixels) {
        system_heap_memory_deallocate(image->pixels);
        image->pixels = 0;
    }
}

struct font_cache font_cache_load_bitmap_font(string filepath, s32 tile_width, s32 tile_height, s32 atlas_rows, s32 atlas_columns) {
    struct font_cache result = {
        .type = FONT_CACHE_ATLAS_FIXED_ASCII,
        .tile_width = tile_width,
        .tile_height = tile_height,
        .atlas_rows = atlas_rows,
        .atlas_cols = atlas_columns
    };

    struct image_buffer atlas_image = image_buffer_load_from_file(filepath);

    result.width  = atlas_image.width;
    result.height = atlas_image.height;
    result.pixels = atlas_image.pixels;

    return result;
}

void font_cache_free(struct font_cache* font_cache) {
    image_buffer_free((struct image_buffer*) font_cache); 
}

f32 font_cache_text_height(struct font_cache* font_cache) {
    return font_cache->tile_height;
}

f32 font_cache_calculate_height_of(struct font_cache* font_cache, string str, f32 width_bounds, f32 scale) {
    f32 font_height = font_cache_text_height(font_cache);

    s32 units = 1;

    if (width_bounds == 0.0f) {
        for (s32 index = 0; index < str.length; ++index) {
            if (str.data[index] == '\n')
                units++;
        }
    } else {
        f32 cursor_x = 0;
        for (s32 index = 0; index < str.length; ++index) {
            cursor_x += font_cache->tile_width * scale;

            if (str.data[index] == '\n') {
                cursor_x = 0;
                units++;
            }

            if (cursor_x >= width_bounds) {
                cursor_x = 0;
                units++;
            }
        }
    }

    return units * font_height * scale;
}

f32 font_cache_text_width(struct font_cache* font_cache, string text, f32 scale) {
    /* NOTE font_caches are monospaced */
    return font_cache->tile_width * text.length * scale;
}

struct software_framebuffer software_framebuffer_create(u32 width, u32 height) {
    u8* pixels = system_heap_memory_allocate(width * height * sizeof(u32));

    return (struct software_framebuffer) {
        .width  = width,
        .height = height,
        .pixels = pixels,
    };
}

struct software_framebuffer software_framebuffer_create_from_arena(struct memory_arena* arena, u32 width, u32 height) {
    u8* pixels = memory_arena_push(arena, width * height * sizeof(u32));

    return (struct software_framebuffer) {
        .width  = width,
        .height = height,
        .pixels = pixels,
    };
}

void software_framebuffer_finish(struct software_framebuffer* framebuffer) {
    if (framebuffer->pixels) {
        system_heap_memory_deallocate(framebuffer->pixels);
        framebuffer->width  = 0;
        framebuffer->height = 0;
        framebuffer->pixels = 0;
    }
}

void software_framebuffer_clear_scissor(struct software_framebuffer* framebuffer) {
    framebuffer->scissor_w = framebuffer->scissor_h = framebuffer->scissor_x = framebuffer->scissor_y = 0;
}

void software_framebuffer_set_scissor(struct software_framebuffer* framebuffer, s32 x, s32 y, s32 w, s32 h) {
    framebuffer->scissor_x = x;
    framebuffer->scissor_y = y;
    framebuffer->scissor_w = w;
    framebuffer->scissor_h = h;
}

void software_framebuffer_clear_buffer(struct software_framebuffer* framebuffer, union color32u8 rgba) {
    memory_set32(framebuffer->pixels, framebuffer->width * framebuffer->height * sizeof(u32), rgba.rgba_packed);
}

/* this is a macro because I don't know if this will be inlined... */
#define _BlendPixel_Scalar(FRAMEBUFFER, X, Y, RGBA, BLEND_MODE) do {    \
        u32  stride                   = FRAMEBUFFER->width;             \
        switch (BLEND_MODE) {                                           \
            case BLEND_MODE_NONE: {                                     \
                float alpha = RGBA.a / 255.0f;                          \
                FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 0] =  (RGBA.r * alpha); \
                FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 1] =  (RGBA.g * alpha); \
                FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 2] =  (RGBA.b * alpha); \
                FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 3] =  (RGBA.a); \
            } break;                                                    \
            case BLEND_MODE_ALPHA: {                                    \
                {                                                       \
                    float alpha = RGBA.a / 255.0f;                      \
                    FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 0] = (FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 0] * (1 - alpha)) + (RGBA.r * alpha); \
                    FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 1] = (FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 1] * (1 - alpha)) + (RGBA.g * alpha); \
                    FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 2] = (FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 2] * (1 - alpha)) + (RGBA.b * alpha); \
                }                                                       \
                FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 3] = 255;  \
            } break;                                                    \
            case BLEND_MODE_ADDITIVE: {                                 \
                {                                                       \
                    float alpha = RGBA.a / 255.0f;                      \
                    u32 added_r = FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 0] + RGBA.r * alpha; \
                    u32 added_g = FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 1] + RGBA.g * alpha; \
                    u32 added_b = FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 2] + RGBA.b * alpha; \
                                                                        \
                    if (added_r > 255) added_r = 255;                   \
                    if (added_g > 255) added_g = 255;                   \
                    if (added_b > 255) added_b = 255;                   \
                                                                        \
                    FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 0] = added_r; \
                    FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 1] = added_g; \
                    FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 2] = added_b; \
                    FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 3] = 255; \
                }                                                       \
            } break;                                                    \
            case BLEND_MODE_MULTIPLICATIVE: {                           \
                {                                                       \
                    u32 modulated_r = FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 0] * RGBA.r/255.0f; \
                    u32 modulated_g = FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 1] * RGBA.g/255.0f; \
                    u32 modulated_b = FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 2] * RGBA.b/255.0f; \
                                                                        \
                    if (modulated_r > 255) modulated_r = 255;           \
                    if (modulated_g > 255) modulated_g = 255;           \
                    if (modulated_b > 255) modulated_b = 255;           \
                                                                        \
                    FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 0] = modulated_r; \
                    FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 1] = modulated_g; \
                    FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 2] = modulated_b; \
                    FRAMEBUFFER->pixels[Y * stride * 4 + X * 4 + 3] = 255; \
                }                                                       \
            } break;                                                    \
                bad_case;                                               \
        }                                                               \
    } while(0)

local bool _framebuffer_scissor_cull(struct software_framebuffer* framebuffer, s32 x, s32 y) {
    if (framebuffer->scissor_w == 0 || framebuffer->scissor_h == 0) {
        return false;
    }
    
    if (x < framebuffer->scissor_x                        ||
        y < framebuffer->scissor_y                        ||
        x > framebuffer->scissor_x+framebuffer->scissor_y ||
        y > framebuffer->scissor_y+framebuffer->scissor_y) {
        return true;
    }

    return false;
}

/* NOTE: Since I rarely draw standard quads for *legit* reasons, these don't have the ability to be shaded. */
#ifdef USE_SIMD_OPTIMIZATIONS
void software_framebuffer_draw_quad_clipped(struct software_framebuffer* framebuffer, struct rectangle_f32 destination, union color32u8 rgba, u8 blend_mode, struct rectangle_f32 clip_rect) {
    __m128i rect_edges_end   = _mm_set_epi32(clip_rect.x+clip_rect.w, clip_rect.y+clip_rect.h, clip_rect.x + clip_rect.w, clip_rect.y + clip_rect.h);
    __m128i rect_edges_start = _mm_set_epi32(clip_rect.x, clip_rect.y, clip_rect.x, clip_rect.y);
    union {
        s32 xyzw[4];
        __m128i vectors;
    } v;
    v.vectors =  _mm_set_epi32(destination.x, destination.y, destination.x+destination.w, destination.y+destination.h);
    v.vectors           = _mm_min_epi32(_mm_max_epi32(v.vectors, rect_edges_start), rect_edges_end);
    s32 start_x = (v.xyzw)[3];
    s32 start_y = (v.xyzw)[2];
    s32 end_x   = (v.xyzw)[1];
    s32 end_y   = (v.xyzw)[0];

    __m128 red_channels   = _mm_set1_ps((f32)rgba.r);
    __m128 green_channels = _mm_set1_ps((f32)rgba.g);
    __m128 blue_channels  = _mm_set1_ps((f32)rgba.b);
    __m128 alpha_channels = _mm_set1_ps((f32)rgba.a);
    __m128 inverse_255    = _mm_set1_ps(1.0 / 255.0f);

    alpha_channels = _mm_mul_ps(inverse_255, alpha_channels);

    __m128 zero           = _mm_set1_ps(0);
    __m128 two_fifty_five = _mm_set1_ps(255);

    s32 stride = framebuffer->width;
    for (s32 y_cursor = start_y; y_cursor < end_y; ++y_cursor) {
        for (s32 x_cursor = start_x; x_cursor < end_x; x_cursor += 4) {
            __m128 red_destination_channels = _mm_set_ps(
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+3) * 4 + 0],
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+2) * 4 + 0],
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+1) * 4 + 0],
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+0) * 4 + 0]
            );
            __m128 green_destination_channels = _mm_set_ps(
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+3) * 4 + 1],
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+2) * 4 + 1],
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+1) * 4 + 1],
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+0) * 4 + 1]
            );
            __m128 blue_destination_channels = _mm_set_ps(
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+3) * 4 + 2],
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+2) * 4 + 2],
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+1) * 4 + 2],
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+0) * 4 + 2]
            );

            switch (blend_mode) {
                case BLEND_MODE_NONE: {
                    red_destination_channels   = _mm_mul_ps(red_channels,   alpha_channels);
                    green_destination_channels = _mm_mul_ps(green_channels, alpha_channels);
                    blue_destination_channels  = _mm_mul_ps(blue_channels,  alpha_channels);
                } break;
                case BLEND_MODE_ALPHA: {
                    __m128 one_minus_alpha     = _mm_sub_ps(_mm_set1_ps(1), alpha_channels);
                    red_destination_channels   = _mm_add_ps(_mm_mul_ps(red_destination_channels,   one_minus_alpha),   _mm_mul_ps(red_channels, alpha_channels));
                    green_destination_channels = _mm_add_ps(_mm_mul_ps(green_destination_channels, one_minus_alpha),   _mm_mul_ps(green_channels, alpha_channels));
                    blue_destination_channels  = _mm_add_ps(_mm_mul_ps(blue_destination_channels,  one_minus_alpha),   _mm_mul_ps(blue_channels, alpha_channels));
                } break;
                case BLEND_MODE_ADDITIVE: {
                    red_destination_channels   = _mm_add_ps(red_destination_channels,   _mm_mul_ps(red_channels,   alpha_channels));
                    green_destination_channels = _mm_add_ps(green_destination_channels, _mm_mul_ps(green_channels, alpha_channels));
                    blue_destination_channels  = _mm_add_ps(blue_destination_channels,  _mm_mul_ps(blue_channels,  alpha_channels));
                } break;
                case BLEND_MODE_MULTIPLICATIVE: {
                    red_destination_channels   = _mm_mul_ps(red_destination_channels,   _mm_mul_ps(_mm_mul_ps(red_channels, alpha_channels),   inverse_255));
                    green_destination_channels = _mm_mul_ps(green_destination_channels, _mm_mul_ps(_mm_mul_ps(green_channels, alpha_channels), inverse_255));
                    blue_destination_channels  = _mm_mul_ps(blue_destination_channels,  _mm_mul_ps(_mm_mul_ps(blue_channels,  alpha_channels), inverse_255));
                } break;
                    bad_case;
            }

#define castF32_M128(X) ((f32*)(&X))
            blue_destination_channels  = _mm_min_ps(_mm_max_ps(blue_destination_channels , zero), two_fifty_five);
            green_destination_channels = _mm_min_ps(_mm_max_ps(green_destination_channels, zero), two_fifty_five);
            red_destination_channels   = _mm_min_ps(_mm_max_ps(red_destination_channels  , zero), two_fifty_five);


            for (int i = 0; i < 4; ++i) {
                if ((x_cursor + i >= clip_rect.x+clip_rect.w)) break;
                if (_framebuffer_scissor_cull(framebuffer, x_cursor+i, y_cursor)) continue;

                framebuffer->pixels_u32[y_cursor * framebuffer->width + (x_cursor+i)] = packu32(
                    255,
                    castF32_M128(blue_destination_channels)[i],
                    castF32_M128(green_destination_channels)[i],
                    castF32_M128(red_destination_channels)[i]
                );
            }
#undef castF32_M128
        }
    }
}
#else
void software_framebuffer_draw_quad_clipped(struct software_framebuffer* framebuffer, struct rectangle_f32 destination, union color32u8 rgba, u8 blend_mode, struct rectangle_f32 clip_rect) {
    s32 start_x = clamp_s32((s32)destination.x, clip_rect.x, clip_rect.x+clip_rect.w);
    s32 start_y = clamp_s32((s32)destination.y, clip_rect.y, clip_rect.y+clip_rect.h);
    s32 end_x   = clamp_s32((s32)(destination.x + destination.w), clip_rect.x, clip_rect.x+clip_rect.w);
    s32 end_y   = clamp_s32((s32)(destination.y + destination.h), clip_rect.y, clip_rect.y+clip_rect.h);

    u32* framebuffer_pixels_as_32 = (u32*)framebuffer->pixels;
    unused(framebuffer_pixels_as_32);

    for (s32 y_cursor = start_y; y_cursor < end_y; ++y_cursor) {
        for (s32 x_cursor = start_x; x_cursor < end_x; ++x_cursor) {
            if (_framebuffer_scissor_cull(framebuffer, x_cursor, y_cursor)) continue;
            _BlendPixel_Scalar(framebuffer, x_cursor, y_cursor, rgba, blend_mode);
        }
    }
}
#endif
void software_framebuffer_draw_quad(struct software_framebuffer* framebuffer, struct rectangle_f32 destination, union color32u8 rgba, u8 blend_mode) {
    software_framebuffer_draw_quad_clipped(framebuffer, destination, rgba, blend_mode, rectangle_f32(0, 0, framebuffer->width, framebuffer->height));
}

void software_framebuffer_draw_image_ex(struct software_framebuffer* framebuffer, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 src, union color32f32 modulation, u32 flags, u8 blend_mode) {
    software_framebuffer_draw_image_ex_clipped(framebuffer, image, destination, src, modulation, flags, blend_mode, rectangle_f32(0,0,framebuffer->width,framebuffer->height), 0, 0);
}

#ifndef USE_SIMD_OPTIMIZATIONS
void software_framebuffer_draw_image_ex_clipped(struct software_framebuffer* framebuffer, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 src, union color32f32 modulation, u32 flags, u8 blend_mode, struct rectangle_f32 clip_rect, shader_fn shader, void* shader_ctx) {
    if ((destination.x == 0) && (destination.y == 0) && (destination.w == 0) && (destination.h == 0)) {
        destination.w = framebuffer->width;
        destination.h = framebuffer->height;
    }

    if (!rectangle_f32_intersect(destination, clip_rect)) {
        return;
    }

    if ((src.x == 0) && (src.y == 0) && (src.w == 0) && (src.h == 0)) {
        src.w = image->width;
        src.h = image->height;
    }

    f32 scale_ratio_w = (f32)src.w  / destination.w;
    f32 scale_ratio_h = (f32)src.h  / destination.h;

    s32 start_x = clamp_s32((s32)destination.x, clip_rect.x, clip_rect.x+clip_rect.w);
    s32 start_y = clamp_s32((s32)destination.y, clip_rect.y, clip_rect.y+clip_rect.h);
    s32 end_x   = clamp_s32((s32)(destination.x + destination.w), clip_rect.x, clip_rect.x+clip_rect.w);
    s32 end_y   = clamp_s32((s32)(destination.y + destination.h), clip_rect.y, clip_rect.y+clip_rect.h);

    s32 unclamped_end_x = (s32)(destination.x + destination.w);
    s32 unclamped_end_y = (s32)(destination.y + destination.h);

    u32* framebuffer_pixels_as_32 = (u32*)framebuffer->pixels;
    unused(framebuffer_pixels_as_32);

    s32 stride       = framebuffer->width;
    s32 image_stride = image->width;

    for (s32 y_cursor = start_y; y_cursor < end_y; ++y_cursor) {
        for (s32 x_cursor = start_x; x_cursor < end_x; ++x_cursor) {
            if (_framebuffer_scissor_cull(framebuffer, x_cursor, y_cursor)) continue;
            s32 image_sample_x = (s32)((src.x + src.w) - ((unclamped_end_x - x_cursor) * scale_ratio_w));
            s32 image_sample_y = (s32)((src.y + src.h) - ((unclamped_end_y - y_cursor) * scale_ratio_h));

            if ((flags & SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_HORIZONTALLY))
                image_sample_x = (s32)(((unclamped_end_x - x_cursor) * scale_ratio_w) + src.x);

            if ((flags & SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_VERTICALLY))
                image_sample_y = (s32)(((unclamped_end_y - y_cursor) * scale_ratio_h) + src.y);

            image_sample_x %= image->width;
            image_sample_y %= image->height;

            union color32f32 sampled_pixel = color32f32(image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 0] / 255.0f,
                                                        image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 1] / 255.0f,
                                                        image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 2] / 255.0f,
                                                        image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 3] / 255.0f);

            if (shader) {
                sampled_pixel = shader(framebuffer, sampled_pixel, v2f32(x_cursor, y_cursor), shader_ctx);
            }
            sampled_pixel.r *= 255.0f;
            sampled_pixel.g *= 255.0f;
            sampled_pixel.b *= 255.0f;
            sampled_pixel.a *= 255.0f;

            sampled_pixel.r *= modulation.r;
            sampled_pixel.g *= modulation.g;
            sampled_pixel.b *= modulation.b;
            sampled_pixel.a *= modulation.a;

            _BlendPixel_Scalar(framebuffer, x_cursor, y_cursor, sampled_pixel, blend_mode);
        }
    }
}
#else
void software_framebuffer_draw_image_ex_clipped(struct software_framebuffer* framebuffer, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 src, union color32f32 modulation, u32 flags, u8 blend_mode, struct rectangle_f32 clip_rect, shader_fn shader, void* shader_ctx) {
    if ((destination.x == 0) && (destination.y == 0) && (destination.w == 0) && (destination.h == 0)) {
        destination.w = clip_rect.w;
        destination.h = clip_rect.h;
    }

    if (!rectangle_f32_intersect(destination, clip_rect)) {
        return;
    }

    if ((src.x == 0) && (src.y == 0) && (src.w == 0) && (src.h == 0)) {
        src.w = image->width;
        src.h = image->height;
    }

    f32 scale_ratio_w = (f32)src.w / destination.w;
    f32 scale_ratio_h = (f32)src.h / destination.h;

    __m128i rect_edges_end   = _mm_set_epi32(clip_rect.x+clip_rect.w, clip_rect.y+clip_rect.h, clip_rect.x + clip_rect.w, clip_rect.y + clip_rect.h);
    __m128i rect_edges_start = _mm_set_epi32(clip_rect.x, clip_rect.y, clip_rect.x, clip_rect.y);
    union {
        s32 xyzw[4];
        __m128i vectors;
    } v;
    v.vectors =  _mm_set_epi32(destination.x, destination.y, destination.x+destination.w, destination.y+destination.h);
    v.vectors           = _mm_min_epi32(_mm_max_epi32(v.vectors, rect_edges_start), rect_edges_end);
    s32 start_x = (v.xyzw)[3];
    s32 start_y = (v.xyzw)[2];
    s32 end_x   = (v.xyzw)[1];
    s32 end_y   = (v.xyzw)[0];

    s32 unclamped_end_x = (s32)(destination.x + destination.w);
    s32 unclamped_end_y = (s32)(destination.y + destination.h);

    u32* framebuffer_pixels_as_32 = (u32*)framebuffer->pixels;
    unused(framebuffer_pixels_as_32);

    s32 stride       = framebuffer->width;
    s32 image_stride = image->width;

    __m128 modulation_r = _mm_load1_ps(&modulation.r);
    __m128 modulation_g = _mm_load1_ps(&modulation.g);
    __m128 modulation_b = _mm_load1_ps(&modulation.b);
    __m128 modulation_a = _mm_load1_ps(&modulation.a);

    __m128 inverse_255    = _mm_set1_ps(1.0 / 255.0f);
    __m128 zero           = _mm_set1_ps(0);
    __m128 two_fifty_five = _mm_set1_ps(255);

    for (s32 y_cursor = start_y; y_cursor < end_y; ++y_cursor) {
        for (s32 x_cursor = start_x; x_cursor < end_x; x_cursor += 4) {
            s32 image_sample_y = ((src.y + src.h) - ((unclamped_end_y - y_cursor) * scale_ratio_h));
            s32 image_sample_x = ((src.x + src.w) - ((unclamped_end_x - (x_cursor))   * scale_ratio_w));

            s32 image_sample_x1 = ((src.x + src.w) - ((unclamped_end_x - (x_cursor+1)) * scale_ratio_w));
            s32 image_sample_x2 = ((src.x + src.w) - ((unclamped_end_x - (x_cursor+2)) * scale_ratio_w));
            s32 image_sample_x3 = ((src.x + src.w) - ((unclamped_end_x - (x_cursor+3)) * scale_ratio_w));

            if ((flags & SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_HORIZONTALLY)) {
                image_sample_x  = (s32)(((unclamped_end_x - x_cursor) * scale_ratio_w) + src.x);
                image_sample_x1 = (s32)(((unclamped_end_x - (x_cursor + 1)) * scale_ratio_w) + src.x);
                image_sample_x2 = (s32)(((unclamped_end_x - (x_cursor + 2)) * scale_ratio_w) + src.x);
                image_sample_x3 = (s32)(((unclamped_end_x - (x_cursor + 3)) * scale_ratio_w) + src.x);
            }

            if ((flags & SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_VERTICALLY))
                image_sample_y = (s32)(((unclamped_end_y - y_cursor) * scale_ratio_h) + src.y);

            union color32f32 sampled_pixels[4];
            sampled_pixels[0] = color32f32(
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x3 * 4 + 0],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x3 * 4 + 1],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x3 * 4 + 2],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x3 * 4 + 3]
            );
            sampled_pixels[1] = color32f32(
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x2 * 4 + 0],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x2 * 4 + 1],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x2 * 4 + 2],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x2 * 4 + 3]
            );
            sampled_pixels[2] = color32f32(
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x1 * 4 + 0],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x1 * 4 + 1],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x1 * 4 + 2],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x1 * 4 + 3]
            );
            sampled_pixels[3] = color32f32(
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 0],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 1],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 2],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 3]
            );

            /* NOTE: This too can be SIMD-ed */
            {
                sampled_pixels[0].r /= 255.0f;
                sampled_pixels[1].r /= 255.0f;
                sampled_pixels[2].r /= 255.0f;
                sampled_pixels[3].r /= 255.0f;
                sampled_pixels[0].g /= 255.0f;
                sampled_pixels[1].g /= 255.0f;
                sampled_pixels[2].g /= 255.0f;
                sampled_pixels[3].g /= 255.0f;
                sampled_pixels[0].b /= 255.0f;
                sampled_pixels[1].b /= 255.0f;
                sampled_pixels[2].b /= 255.0f;
                sampled_pixels[3].b /= 255.0f;
                sampled_pixels[0].a /= 255.0f;
                sampled_pixels[1].a /= 255.0f;
                sampled_pixels[2].a /= 255.0f;
                sampled_pixels[3].a /= 255.0f;
            }
            if (shader) {
                sampled_pixels[0] = shader(framebuffer, sampled_pixels[0], v2f32(x_cursor+3, y_cursor), shader_ctx);
                sampled_pixels[1] = shader(framebuffer, sampled_pixels[1], v2f32(x_cursor+2, y_cursor), shader_ctx);
                sampled_pixels[2] = shader(framebuffer, sampled_pixels[2], v2f32(x_cursor+1, y_cursor), shader_ctx);
                sampled_pixels[3] = shader(framebuffer, sampled_pixels[3], v2f32(x_cursor, y_cursor), shader_ctx);
            }
            {
                sampled_pixels[0].r *= 255.0f;
                sampled_pixels[1].r *= 255.0f;
                sampled_pixels[2].r *= 255.0f;
                sampled_pixels[3].r *= 255.0f;
                sampled_pixels[0].g *= 255.0f;
                sampled_pixels[1].g *= 255.0f;
                sampled_pixels[2].g *= 255.0f;
                sampled_pixels[3].g *= 255.0f;
                sampled_pixels[0].b *= 255.0f;
                sampled_pixels[1].b *= 255.0f;
                sampled_pixels[2].b *= 255.0f;
                sampled_pixels[3].b *= 255.0f;
                sampled_pixels[0].a *= 255.0f;
                sampled_pixels[1].a *= 255.0f;
                sampled_pixels[2].a *= 255.0f;
                sampled_pixels[3].a *= 255.0f;
            }

            __m128 red_channels   = _mm_set_ps(sampled_pixels[0].r, sampled_pixels[1].r, sampled_pixels[2].r, sampled_pixels[3].r);
            __m128 green_channels = _mm_set_ps(sampled_pixels[0].g, sampled_pixels[1].g, sampled_pixels[2].g, sampled_pixels[3].g);
            __m128 blue_channels  = _mm_set_ps(sampled_pixels[0].b, sampled_pixels[1].b, sampled_pixels[2].b, sampled_pixels[3].b);
            __m128 alpha_channels = _mm_set_ps(sampled_pixels[0].a, sampled_pixels[1].a, sampled_pixels[2].a, sampled_pixels[3].a);

            __m128 red_destination_channels = _mm_set_ps(
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+3) * 4 + 0],
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+2) * 4 + 0],
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+1) * 4 + 0],
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+0) * 4 + 0]
            );
            __m128 green_destination_channels = _mm_set_ps(
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+3) * 4 + 1],
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+2) * 4 + 1],
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+1) * 4 + 1],
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+0) * 4 + 1]
            );
            __m128 blue_destination_channels = _mm_set_ps(
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+3) * 4 + 2],
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+2) * 4 + 2],
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+1) * 4 + 2],
                framebuffer->pixels[y_cursor * stride * 4 + (x_cursor+0) * 4 + 2]
            );

            red_channels   = _mm_mul_ps(modulation_r, red_channels);
            green_channels = _mm_mul_ps(modulation_g, green_channels);
            blue_channels  = _mm_mul_ps(modulation_b, blue_channels);
            alpha_channels = _mm_mul_ps(modulation_a, _mm_mul_ps(inverse_255, alpha_channels));

            /* NOTE this is the only simd optimized procedure since it's the most expensive one */
            /* might not be able to macro the blend modes anyway */

            switch (blend_mode) {
                case BLEND_MODE_NONE: {
                    red_destination_channels   = _mm_mul_ps(red_channels,   alpha_channels);
                    green_destination_channels = _mm_mul_ps(green_channels, alpha_channels);
                    blue_destination_channels  = _mm_mul_ps(blue_channels,  alpha_channels);
                } break;
                case BLEND_MODE_ALPHA: {
                    __m128 one_minus_alpha     = _mm_sub_ps(_mm_set1_ps(1), alpha_channels);
                    red_destination_channels   = _mm_add_ps(_mm_mul_ps(red_destination_channels,   one_minus_alpha),   _mm_mul_ps(red_channels, alpha_channels));
                    green_destination_channels = _mm_add_ps(_mm_mul_ps(green_destination_channels, one_minus_alpha),   _mm_mul_ps(green_channels, alpha_channels));
                    blue_destination_channels  = _mm_add_ps(_mm_mul_ps(blue_destination_channels,  one_minus_alpha),   _mm_mul_ps(blue_channels, alpha_channels));
                } break;
                case BLEND_MODE_ADDITIVE: {
                    red_destination_channels   = _mm_add_ps(red_destination_channels,   _mm_mul_ps(red_channels,   alpha_channels));
                    green_destination_channels = _mm_add_ps(green_destination_channels, _mm_mul_ps(green_channels, alpha_channels));
                    blue_destination_channels  = _mm_add_ps(blue_destination_channels,  _mm_mul_ps(blue_channels,  alpha_channels));
                } break;
                case BLEND_MODE_MULTIPLICATIVE: {
                    red_destination_channels   = _mm_mul_ps(red_destination_channels,   _mm_mul_ps(_mm_mul_ps(red_channels, alpha_channels),   inverse_255));
                    green_destination_channels = _mm_mul_ps(green_destination_channels, _mm_mul_ps(_mm_mul_ps(green_channels, alpha_channels), inverse_255));
                    blue_destination_channels  = _mm_mul_ps(blue_destination_channels,  _mm_mul_ps(_mm_mul_ps(blue_channels,  alpha_channels), inverse_255));
                } break;
                    bad_case;
            }

#define castF32_M128(X) ((f32*)(&X))
            blue_destination_channels  = _mm_min_ps(_mm_max_ps(blue_destination_channels , zero), two_fifty_five);
            green_destination_channels = _mm_min_ps(_mm_max_ps(green_destination_channels, zero), two_fifty_five);
            red_destination_channels   = _mm_min_ps(_mm_max_ps(red_destination_channels  , zero), two_fifty_five);

            for (int i = 0; i < 4; ++i) {
                if (_framebuffer_scissor_cull(framebuffer, x_cursor+i, y_cursor)) continue;
                if ((x_cursor + i >= clip_rect.x+clip_rect.w) || (((src.x + src.w) - ((unclamped_end_x - (x_cursor+i)) * scale_ratio_w))) >= src.x+src.w) break;

                framebuffer->pixels_u32[y_cursor * framebuffer->width + (x_cursor+i)] = packu32(
                    255,
                    castF32_M128(blue_destination_channels)[i],
                    castF32_M128(green_destination_channels)[i],
                    castF32_M128(red_destination_channels)[i]
                );
            }
#undef castF32_M128
        }
    }
}
#endif


void software_framebuffer_draw_line_clipped(struct software_framebuffer* framebuffer, v2f32 start, v2f32 end, union color32u8 rgba, u8 blend_mode, struct rectangle_f32 clip_rect) {
    u32 stride = framebuffer->width;

    if (start.y == end.y) {
        if (start.x > end.x) {
            Swap(start.x, end.x, f32);
        }

        for (s32 x_cursor = start.x; x_cursor < end.x; x_cursor++) {
            if (_framebuffer_scissor_cull(framebuffer, x_cursor, start.y)) continue;
            if (x_cursor < clip_rect.w+clip_rect.x && x_cursor >= clip_rect.x &&
                start.y  < clip_rect.h+clip_rect.y && start.y >= clip_rect.y) {
                _BlendPixel_Scalar(framebuffer, x_cursor, (s32)floor(start.y), rgba, blend_mode);
            }
        }
    } else if (start.x == end.x) {
        if (start.y > end.y) {
            Swap(start.y, end.y, f32);
        }
        
        for (s32 y_cursor = start.y; y_cursor < end.y; y_cursor++) {
            if (_framebuffer_scissor_cull(framebuffer, start.x, y_cursor)) continue;
            if (start.x < clip_rect.x+clip_rect.w && start.x    >= clip_rect.x &&
                y_cursor  < clip_rect.y+clip_rect.h && y_cursor >= clip_rect.y) {
                _BlendPixel_Scalar(framebuffer, (s32)floor(start.x), y_cursor, rgba, blend_mode);
            }
        }
    } else {
        s32 x1 = start.x;
        s32 x2 = end.x;
        s32 y1 = start.y;
        s32 y2 = end.y;

        s32 delta_x = abs(x2 - x1);
        s32 delta_y = -abs(y2 - y1);
        s32 sign_x  = 0;
        s32 sign_y  = 0;

        if (x1 < x2) sign_x = 1;
        else         sign_x = -1;

        if (y1 < y2) sign_y = 1;
        else         sign_y = -1;

        s32 error_accumulator = delta_x + delta_y;
        float alpha = rgba.a / 255.0f;

        for (;;) {
            if (x1 < clip_rect.x+clip_rect.w   && x1 >= clip_rect.x && y1 < clip_rect.y+clip_rect.h && y1 >= clip_rect.y) {
                if (_framebuffer_scissor_cull(framebuffer, x1, y1)) continue;
                _BlendPixel_Scalar(framebuffer, x1, y1, rgba, blend_mode);
            }

            if (x1 == x2 && y1 == y2) return;

            s32 old_error_x2 = 2 * error_accumulator;

            if (old_error_x2 >= delta_y) {
                if (x1 != x2) {
                    error_accumulator += delta_y;
                    x1 += sign_x;
                }
            }

            if (old_error_x2 <= delta_x) {
                if (y1 != y2) {
                    error_accumulator += delta_x;
                    y1 += sign_y;
                }
            }
        }
    }
}
void software_framebuffer_draw_line(struct software_framebuffer* framebuffer, v2f32 start, v2f32 end, union color32u8 rgba, u8 blend_mode) {
    software_framebuffer_draw_line_clipped(framebuffer, start, end, rgba, blend_mode, rectangle_f32(0, 0, framebuffer->width, framebuffer->height));
}

local void software_framebuffer_draw_glyph_clipped(struct software_framebuffer* framebuffer, struct font_cache* font, float scale, v2f32 xy, s32 character, union color32f32 modulation, u8 blend_mode, struct rectangle_f32 clip_rect) {
    f32 x_cursor = xy.x;
    f32 y_cursor = xy.y;

    software_framebuffer_draw_image_ex_clipped(
        framebuffer, (struct image_buffer*)font,
        rectangle_f32(
            x_cursor, y_cursor,
            font->tile_width  * scale,
            font->tile_height * scale
        ),
        rectangle_f32(
            (character % font->atlas_cols) * font->tile_width,
            (character / font->atlas_cols) * font->tile_height,
            font->tile_width, font->tile_height
        ),
        modulation,
        NO_FLAGS,
        blend_mode,
        clip_rect,
        0,0
    );
}

local void software_framebuffer_draw_text_clipped(struct software_framebuffer* framebuffer, struct font_cache* font, float scale, v2f32 xy, string text, union color32f32 modulation, u8 blend_mode, struct rectangle_f32 clip_rect) {
    f32 x_cursor = xy.x;
    f32 y_cursor = xy.y;

    for (unsigned index = 0; index < text.length; ++index) {
        if (text.data[index] == '\n') {
            y_cursor += font->tile_height * scale;
            x_cursor =  xy.x;
        } else {
            s32 character_index = text.data[index] - 32;

            software_framebuffer_draw_glyph_clipped(framebuffer, font, scale, v2f32(x_cursor, y_cursor), character_index, modulation, blend_mode, clip_rect);
            x_cursor += font->tile_width * scale;
        }
    }
}

void software_framebuffer_draw_text(struct software_framebuffer* framebuffer, struct font_cache* font, float scale, v2f32 xy, string text, union color32f32 modulation, u8 blend_mode)  {
    software_framebuffer_draw_text_clipped(framebuffer, font, scale, xy, text, modulation, blend_mode, rectangle_f32(0, 0, framebuffer->width, framebuffer->height));
}

void software_framebuffer_draw_glyph(struct software_framebuffer* framebuffer, struct font_cache* font, f32 scale, v2f32 xy, char glyph, union color32f32 modulation, u8 blend_mode) {
    software_framebuffer_draw_glyph_clipped(framebuffer, font, scale, xy, glyph, modulation, blend_mode, rectangle_f32(0, 0, framebuffer->width, framebuffer->height));
}

/* TODO: provide clipped versions */
void software_framebuffer_draw_text_bounds(struct software_framebuffer* framebuffer, struct font_cache* font, f32 scale, v2f32 xy, f32 bounds_w, string text, union color32f32 modulation, u8 blend_mode) {
    f32 x_cursor = xy.x;
    f32 y_cursor = xy.y;

    for (unsigned index = 0; index < text.length; ++index) {
        if (text.data[index] == '\n') {
            y_cursor += font->tile_height * scale;
            x_cursor = xy.x;
        } else {
            s32 character_index = text.data[index] - 32;

            software_framebuffer_draw_glyph_clipped(framebuffer, font, scale, v2f32(x_cursor, y_cursor), character_index, modulation, blend_mode, rectangle_f32(0,0, framebuffer->width, framebuffer->height));
            x_cursor += font->tile_width * scale;

            if (x_cursor >= xy.x+bounds_w) {
                x_cursor = xy.x;
                y_cursor += font->tile_height * scale;
            }
        }
    }
}

void software_framebuffer_draw_text_bounds_centered(struct software_framebuffer* framebuffer, struct font_cache* font, f32 scale, struct rectangle_f32 bounds, string text, union color32f32 modulation, u8 blend_mode) {
    f32 text_width  = font_cache_text_width(font, text, scale);
    f32 text_height = font_cache_calculate_height_of(font, text, bounds.w, scale);
    v2f32 centered_starting_position = v2f32(0,0);

    centered_starting_position.x = bounds.x + (bounds.w/2) - (text_width/2);
    centered_starting_position.y = bounds.y + (bounds.h/2) - (text_height/2);

    software_framebuffer_draw_text_bounds(framebuffer, font, scale, centered_starting_position, bounds.w, text, modulation, blend_mode);
}

void software_framebuffer_copy_into(struct software_framebuffer* target, struct software_framebuffer* source) {
    if (target->width == source->width && target->height == source->height) {
        memory_copy(source->pixels, target->pixels, target->width * target->height * sizeof(u32));
    } else {
        software_framebuffer_draw_image_ex(target, (struct image_buffer*)source, RECTANGLE_F32_NULL, RECTANGLE_F32_NULL, color32f32(1,1,1,1), NO_FLAGS, 0);
    }
}

struct render_commands render_commands(struct memory_arena* arena, s32 capacity, struct camera camera) {
    struct render_commands result = {};
    result.camera = camera;
    result.command_capacity = capacity;
    result.commands         = memory_arena_push(arena, capacity * sizeof(*result.commands));
    return result;
}

struct render_command* render_commands_new_command(struct render_commands* commands, s16 type) {
    struct render_command* command = &commands->commands[commands->command_count++];
    command->type = type;
    return command;
}

void render_commands_set_shader(struct render_commands* commands, shader_fn shader, void* context) {
    struct render_command* last_command = &commands->commands[commands->command_count - 1];
    last_command->shader = shader;
    last_command->shader_ctx = context;
}

void render_commands_push_quad(struct render_commands* commands, struct rectangle_f32 destination, union color32u8 rgba, u8 blend_mode) {
    struct render_command* command = render_commands_new_command(commands, RENDER_COMMAND_DRAW_QUAD);
    command->destination   = destination;
    command->flags         = 0;
    command->modulation_u8 = rgba;
    command->blend_mode    = blend_mode;
}

void render_commands_push_image(struct render_commands* commands, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 source, union color32f32 rgba, u32 flags, u8 blend_mode){
    struct render_command* command = render_commands_new_command(commands, RENDER_COMMAND_DRAW_IMAGE);
    command->destination   = destination;
    command->image         = image;
    command->source        = source;
    command->flags         = flags;
    command->modulation_u8 = color32f32_to_color32u8(rgba);
    command->blend_mode    = blend_mode;
}

void render_commands_push_line(struct render_commands* commands, v2f32 start, v2f32 end, union color32u8 rgba, u8 blend_mode) {
    struct render_command* command = render_commands_new_command(commands, RENDER_COMMAND_DRAW_LINE);
    command->start         = start;
    command->end           = end;
    command->flags         = 0;
    command->modulation_u8 = rgba;
    command->blend_mode    = blend_mode;
}

void render_commands_push_text(struct render_commands* commands, struct font_cache* font, f32 scale, v2f32 xy, string text, union color32f32 rgba, u8 blend_mode) {
    struct render_command* command = render_commands_new_command(commands, RENDER_COMMAND_DRAW_TEXT);
    command->font          = font;
    command->xy            = xy;
    command->scale         = scale;
    command->text          = text;
    command->modulation_u8 = color32f32_to_color32u8(rgba);
    command->flags         = 0;
    command->blend_mode    = blend_mode;
}

void render_commands_clear(struct render_commands* commands) {
    commands->command_count = 0;
}

void sort_render_commands(struct render_commands* commands) {
    /* NOTE: Not needed? I seem to know the explicit order of everything I'm drawing... */
}

void software_framebuffer_use_shader(struct software_framebuffer* framebuffer, shader_fn shader, void* context) {
    framebuffer->active_shader         = shader;
    framebuffer->active_shader_context = context;
}

/* TODO: The main benefit of render commands, is that we should be able to parallelize them.
   This should lead to a massive speed up, my SIMD optimization is kind of shit since it stabilizes but doesn't
   really improve framerate.
   
Time to parallelize.
*/
struct render_commands_job_details {
    struct software_framebuffer* framebuffer;
    struct render_commands*      commands;
    struct rectangle_f32         clip_rect;
};

void software_framebuffer_render_commands_tiled(struct software_framebuffer* framebuffer, struct render_commands* commands, struct rectangle_f32 clip_rect) {
    for (unsigned index = 0; index < commands->command_count; ++index) {
        struct render_command* command = &commands->commands[index];

        switch (command->type) {
            case RENDER_COMMAND_DRAW_QUAD: {
                software_framebuffer_draw_quad_clipped(
                    framebuffer,
                    command->destination,
                    command->modulation_u8,
                    command->blend_mode,
                    clip_rect
                );
            } break;
            case RENDER_COMMAND_DRAW_IMAGE: {
                software_framebuffer_draw_image_ex_clipped(
                    framebuffer,
                    command->image,
                    command->destination,
                    command->source,
                    color32u8_to_color32f32(command->modulation_u8),
                    command->flags,
                    command->blend_mode,
                    clip_rect,
                    command->shader,
                    command->shader_ctx
                );
            } break;
            case RENDER_COMMAND_DRAW_TEXT: {
                software_framebuffer_draw_text_clipped(
                    framebuffer,
                    command->font,
                    command->scale,
                    command->xy,
                    command->text,
                    color32u8_to_color32f32(command->modulation_u8),
                    command->blend_mode,
                    clip_rect
                );
            } break;
            case RENDER_COMMAND_DRAW_LINE: {
                software_framebuffer_draw_line_clipped(
                    framebuffer,
                    command->start,
                    command->end,
                    command->modulation_u8,
                    command->blend_mode,
                    clip_rect
                );
            } break;
        }
    }
}

s32 thread_software_framebuffer_render_commands_tiles(void* context) {
    struct render_commands_job_details* job_details = context;
    software_framebuffer_render_commands_tiled(job_details->framebuffer,
                                               job_details->commands,
                                               job_details->clip_rect);
    return 0;
}

local void transform_command_into_clip_space(v2f32 resolution, struct render_command* command, struct camera* camera, v2f32 trauma_displacement) {
    f32 half_screen_width  = resolution.x/2;
    f32 half_screen_height = resolution.y/2;

    {
        command->start.x       *= camera->zoom;
        command->start.y       *= camera->zoom;
        command->end.x         *= camera->zoom;
        command->end.y         *= camera->zoom;
        command->destination.x *= camera->zoom;
        command->destination.y *= camera->zoom;
        command->destination.w *= camera->zoom;
        command->destination.h *= camera->zoom;
        command->scale         *= camera->zoom;
    }

    if (camera->centered) {
        command->start.x       += half_screen_width;
        command->start.y       += half_screen_height;
        command->end.x         += half_screen_width;
        command->end.y         += half_screen_height;
        command->destination.x += half_screen_width;
        command->destination.y += half_screen_height;
    }

    {
        command->start.x       -= camera->xy.x + (trauma_displacement.x);
        command->start.y       -= camera->xy.y + (trauma_displacement.y);
        command->end.x         -= camera->xy.x + (trauma_displacement.x);
        command->end.y         -= camera->xy.y + (trauma_displacement.y);
        command->destination.x -= camera->xy.x + (trauma_displacement.x);
        command->destination.y -= camera->xy.y + (trauma_displacement.y);
    }
}
                                  
void software_framebuffer_render_commands(struct software_framebuffer* framebuffer, struct render_commands* commands) {
    if (commands->should_clear_buffer) {
        software_framebuffer_clear_buffer(framebuffer, commands->clear_buffer_color);
    }

    sort_render_commands(commands);

    /* move all things into clip space */
    v2f32 displacement      = camera_displacement_from_trauma(&commands->camera);

    for (unsigned index = 0; index < commands->command_count; ++index) {
        struct render_command* command = &commands->commands[index];
        transform_command_into_clip_space(v2f32(SCREEN_WIDTH, SCREEN_HEIGHT), command, &commands->camera, displacement);
    }

#ifndef MULTITHREADED_EXPERIMENTAL
    software_framebuffer_render_commands_tiled(framebuffer, commands, rectangle_f32(0,0,framebuffer->width,framebuffer->height));
#else
    s32 JOB_W  = 2;
    s32 JOB_H  = 2;
    s32 TILE_W = framebuffer->width / JOB_W;
    s32 TILE_H = framebuffer->height / JOB_H;

    struct render_commands_job_details* job_details = memory_arena_push(&scratch_arena, sizeof(*job_details) * (JOB_W*JOB_H));

    for (s32 y = 0; y < JOB_H; ++y) {
        for (s32 x = 0; x < JOB_W; ++x) {
            struct rectangle_f32 clip_rect = (struct rectangle_f32) { x * TILE_W, y * TILE_H, TILE_W, TILE_H };

            struct render_commands_job_details* current_details = &job_details[y*JOB_W+x];

            current_details->framebuffer = framebuffer;
            current_details->commands    = commands;
            current_details->clip_rect   = clip_rect;
            
            thread_pool_add_job(thread_software_framebuffer_render_commands_tiles, current_details);
        }
    }

    thread_pool_synchronize_tasks();
#endif
}

/* requires an arena because we need an original copy of our framebuffer. */
/* NOTE technically a test of performance to see if this is doomed */

struct postprocess_job_shared {
    /* 
       should allow for job types such as generic shader 
       or specialized cases such as this
    */
    f32* kernel;
    s32  kernel_width;
    s32  kernel_height;

    f32 divisor;
    s32 passes;

    struct software_framebuffer* unaltered_framebuffer_copy;
    struct software_framebuffer* framebuffer;

    f32                          blend_t;
};
struct postprocess_job_details {
    struct postprocess_job_shared* shared;
    struct rectangle_f32         clip_rect;
};

s32 thread_software_framebuffer_kernel_convolution(void* job) {
    struct postprocess_job_details* details = job;
    software_framebuffer_kernel_convolution_ex_bounded(*details->shared->unaltered_framebuffer_copy,
                                                       details->shared->framebuffer,
                                                       details->shared->kernel,
                                                       details->shared->kernel_width,
                                                       details->shared->kernel_height,
                                                       details->shared->divisor,
                                                       details->shared->blend_t,
                                                       details->shared->passes,
                                                       details->clip_rect);
    return 0;
}

void software_framebuffer_kernel_convolution_ex(struct memory_arena* arena, struct software_framebuffer* framebuffer, f32* kernel, s16 kernel_width, s16 kernel_height, f32 divisor, f32 blend_t, s32 passes) {
#ifndef MULTITHREADED_EXPERIMENTAL
    struct software_framebuffer unaltered_copy = software_framebuffer_create_from_arena(arena, framebuffer->width, framebuffer->height);
    software_framebuffer_copy_into(&unaltered_copy, framebuffer);
    software_framebuffer_kernel_convolution_ex_bounded(unaltered_copy, framebuffer, kernel, kernel_width, kernel_height, divisor, blend_t, passes, rectangle_f32(0,0,framebuffer->width,framebuffer->height));
#else
    struct software_framebuffer unaltered_buffer = software_framebuffer_create_from_arena(arena, framebuffer->width, framebuffer->height);
    software_framebuffer_copy_into(&unaltered_buffer, framebuffer);

    /* We don't handle un-even divisions, which is kind of bad. This is mostly a "proof of concept" */
    s32 JOBS_W      = 4;
    s32 JOBS_H      = 4;
    s32 CLIP_W      = (framebuffer->width)/JOBS_W;
    s32 CLIP_H      = (framebuffer->height)/JOBS_H;
    s32 REMAINDER_W = (framebuffer->width) % JOBS_W;
    s32 REMAINDER_H = (framebuffer->height) % JOBS_H;

    struct postprocess_job_shared shared_buffer =  (struct postprocess_job_shared) {
        .kernel                     = kernel,
        .kernel_width               = kernel_width,
        .kernel_height              = kernel_height,
        .unaltered_framebuffer_copy = &unaltered_buffer,
        .framebuffer                = framebuffer,
        .blend_t                    = blend_t,
        .divisor                    = divisor,
        .passes                     = passes,
    };

    struct postprocess_job_details* job_buffers = memory_arena_push(arena, sizeof(*job_buffers) * (JOBS_W*JOBS_H));

#if 0
    _debugprintf("%d, %d (%d r, %d r)", framebuffer->width, framebuffer->height, REMAINDER_W, REMAINDER_H);
#endif
    for (s32 y = 0; y < JOBS_H; ++y) {
        for (s32 x = 0; x < JOBS_W; ++x) {
            struct rectangle_f32            clip_rect      = (struct rectangle_f32){x * CLIP_W, y * CLIP_H, CLIP_W, CLIP_H};
            struct postprocess_job_details* current_buffer = &job_buffers[y*JOBS_W+x];

            if (x == JOBS_W-1) {
                clip_rect.w += REMAINDER_W;
            }

            if (y == JOBS_H-1) {
                clip_rect.h += REMAINDER_H;
            }

            {
                current_buffer->shared                     = &shared_buffer;
                current_buffer->clip_rect                  = clip_rect;
            }

            thread_pool_add_job(thread_software_framebuffer_kernel_convolution, current_buffer);
        }
    }

    thread_pool_synchronize_tasks();
#endif
}
/* does not thread itself. */
#ifdef USE_SIMD_OPTIMIZATIONS
void software_framebuffer_kernel_convolution_ex_bounded(struct software_framebuffer unaltered_copy, struct software_framebuffer* framebuffer, f32* kernel, s16 kernel_width, s16 kernel_height, f32 divisor, f32 blend_t, s32 passes, struct rectangle_f32 clip) {
#define castF32_M128(X) ((f32*)(&X))
    if (divisor == 0.0) divisor = 1;

    s32 framebuffer_width  = framebuffer->width;
    s32 framebuffer_height = framebuffer->height;

    s32 kernel_half_width =  kernel_width/2;
    s32 kernel_half_height = kernel_height/2;

    __m128 zero             = _mm_set1_ps(0);
    __m128 two_fifty_five   = _mm_set1_ps(255);
    __m128 one_over_divisor = _mm_set1_ps(1/divisor);
    __m128 one_minus_blend_t = _mm_set1_ps(1-blend_t);

    for (s32 pass = 0; pass < passes; pass++) {
        for (s32 y_cursor = clip.y; y_cursor < clip.y+clip.h; ++y_cursor) {
            for (s32 x_cursor = clip.x; x_cursor < clip.w+clip.x; ++x_cursor) {
                __m128 accumulation = _mm_set1_ps(0);

                for (s32 y_cursor_kernel = -kernel_half_height; y_cursor_kernel <= kernel_half_height; ++y_cursor_kernel) {
                    for (s32 x_cursor_kernel = -kernel_half_width; x_cursor_kernel <= kernel_half_width; ++x_cursor_kernel) {
                        s32 sample_x = x_cursor_kernel + x_cursor;
                        s32 sample_y = y_cursor_kernel + y_cursor;

                        __m128 kernel_value = _mm_set1_ps(kernel[(y_cursor_kernel+1) * kernel_width + (x_cursor_kernel+1)]);
                        __m128 pixel_val    = _mm_set_ps(0,
                                                         unaltered_copy.pixels[sample_y * framebuffer_width * 4 + sample_x * 4 + 2],
                                                         unaltered_copy.pixels[sample_y * framebuffer_width * 4 + sample_x * 4 + 1],
                                                         unaltered_copy.pixels[sample_y * framebuffer_width * 4 + sample_x * 4 + 0]);

                        if (sample_x >= 0 && sample_x < framebuffer_width &&
                            sample_y >= 0 && sample_y < framebuffer_height) {
                            accumulation = _mm_add_ps(accumulation, _mm_mul_ps(pixel_val, kernel_value));
                        }
                    }
                }

                accumulation = _mm_min_ps(_mm_max_ps(_mm_mul_ps(accumulation, one_over_divisor), zero), two_fifty_five);

                __m128 pixel_val = _mm_set_ps(0,
                                              framebuffer->pixels[y_cursor * framebuffer_width * 4 + x_cursor * 4 + 2],
                                              framebuffer->pixels[y_cursor * framebuffer_width * 4 + x_cursor * 4 + 1],
                                              framebuffer->pixels[y_cursor * framebuffer_width * 4 + x_cursor * 4 + 0]);
                pixel_val = _mm_add_ps(_mm_mul_ps(pixel_val, one_minus_blend_t), _mm_mul_ps(_mm_set1_ps(blend_t), accumulation));

                framebuffer->pixels[y_cursor * framebuffer_width * 4 + x_cursor * 4 + 0] = castF32_M128(pixel_val)[0];
                framebuffer->pixels[y_cursor * framebuffer_width * 4 + x_cursor * 4 + 1] = castF32_M128(pixel_val)[1];
                framebuffer->pixels[y_cursor * framebuffer_width * 4 + x_cursor * 4 + 2] = castF32_M128(pixel_val)[2];
            }
        }
    }
#undef castF32_M128
}
#else
void software_framebuffer_kernel_convolution_ex_bounded(struct software_framebuffer unaltered_copy, struct software_framebuffer* framebuffer, f32* kernel, s16 kernel_width, s16 kernel_height, f32 divisor, f32 blend_t, s32 passes, struct rectangle_f32 clip) {
    if (divisor == 0.0) divisor = 1;
    s32 framebuffer_width  = framebuffer->width;
    s32 framebuffer_height = framebuffer->height;

    s32 kernel_half_width =  kernel_width/2;
    s32 kernel_half_height = kernel_height/2;

    for (s32 pass = 0; pass < passes; pass++) {
        for (s32 y_cursor = clip.y; y_cursor < clip.y+clip.h; ++y_cursor) {
            for (s32 x_cursor = clip.x; x_cursor < clip.w+clip.x; ++x_cursor) {
                f32 accumulation[3] = {};

                for (s32 y_cursor_kernel = -kernel_half_height; y_cursor_kernel <= kernel_half_height; ++y_cursor_kernel) {
                    for (s32 x_cursor_kernel = -kernel_half_width; x_cursor_kernel <= kernel_half_width; ++x_cursor_kernel) {
                        s32 sample_x = x_cursor_kernel + x_cursor;
                        s32 sample_y = y_cursor_kernel + y_cursor;

                        if (sample_x >= 0 && sample_x < framebuffer_width &&
                            sample_y >= 0 && sample_y < framebuffer_height) {
                            accumulation[0] += unaltered_copy.pixels[sample_y * framebuffer_width * 4 + sample_x * 4 + 0] * kernel[(y_cursor_kernel+1) * kernel_width + (x_cursor_kernel+1)];
                            accumulation[1] += unaltered_copy.pixels[sample_y * framebuffer_width * 4 + sample_x * 4 + 1] * kernel[(y_cursor_kernel+1) * kernel_width + (x_cursor_kernel+1)];
                            accumulation[2] += unaltered_copy.pixels[sample_y * framebuffer_width * 4 + sample_x * 4 + 2] * kernel[(y_cursor_kernel+1) * kernel_width + (x_cursor_kernel+1)];
                        }
                    }
                }

                accumulation[0] = clamp_f32(accumulation[0] / divisor, 0, 255.0f);
                accumulation[1] = clamp_f32(accumulation[1] / divisor, 0, 255.0f);
                accumulation[2] = clamp_f32(accumulation[2] / divisor, 0, 255.0f);

                /* NOTE does not blend any pixels, other than what's in blend_t, but that's a convenience thing sort of. */
                framebuffer->pixels[y_cursor * framebuffer_width * 4 + x_cursor * 4 + 0] = framebuffer->pixels[y_cursor * framebuffer_width * 4 + x_cursor * 4 + 0] * (1 - blend_t) + (blend_t * accumulation[0]);
                framebuffer->pixels[y_cursor * framebuffer_width * 4 + x_cursor * 4 + 1] = framebuffer->pixels[y_cursor * framebuffer_width * 4 + x_cursor * 4 + 1] * (1 - blend_t) + (blend_t * accumulation[1]);
                framebuffer->pixels[y_cursor * framebuffer_width * 4 + x_cursor * 4 + 2] = framebuffer->pixels[y_cursor * framebuffer_width * 4 + x_cursor * 4 + 2] * (1 - blend_t) + (blend_t * accumulation[2]);
            }
        }
    }
}
#endif

struct run_shader_job_details_shared {
    struct software_framebuffer* framebuffer;
    shader_fn shader;
    void* context;
};

struct run_shader_job_details {
    struct run_shader_job_details_shared* shared;
    struct rectangle_f32 src_rect;
};

s32 thread_software_framebuffer_run_shader(void* context) {
    struct run_shader_job_details* job_details = context;
    struct rectangle_f32 src_rect = job_details->src_rect;

    for (s32 y = src_rect.y; y < src_rect.y+src_rect.h; ++y) {
        for (s32 x = src_rect.x; x < src_rect.x+src_rect.w; ++x) {
            union color32f32 source_pixel = color32f32(
                job_details->shared->framebuffer->pixels[y * job_details->shared->framebuffer->width * 4 + x * 4 + 0] / 255.0f,
                job_details->shared->framebuffer->pixels[y * job_details->shared->framebuffer->width * 4 + x * 4 + 1] / 255.0f,
                job_details->shared->framebuffer->pixels[y * job_details->shared->framebuffer->width * 4 + x * 4 + 2] / 255.0f,
                job_details->shared->framebuffer->pixels[y * job_details->shared->framebuffer->width * 4 + x * 4 + 3] / 255.0f
            );

            union color32f32 new_pixel = job_details->shared->shader(job_details->shared->framebuffer, source_pixel, v2f32(x, y), job_details->shared->context);

            new_pixel.r = clamp_f32(new_pixel.r, 0, 1);
            new_pixel.g = clamp_f32(new_pixel.g, 0, 1);
            new_pixel.b = clamp_f32(new_pixel.b, 0, 1);
            new_pixel.a = clamp_f32(new_pixel.a, 0, 1);

            job_details->shared->framebuffer->pixels[y * job_details->shared->framebuffer->width * 4 + x * 4 + 0] = new_pixel.r * 255.0f;
            job_details->shared->framebuffer->pixels[y * job_details->shared->framebuffer->width * 4 + x * 4 + 1] = new_pixel.g * 255.0f;
            job_details->shared->framebuffer->pixels[y * job_details->shared->framebuffer->width * 4 + x * 4 + 2] = new_pixel.b * 255.0f;
            job_details->shared->framebuffer->pixels[y * job_details->shared->framebuffer->width * 4 + x * 4 + 3] = new_pixel.a * 255.0f;
        }
    }

    return 0;
}

void software_framebuffer_run_shader(struct software_framebuffer* framebuffer, struct rectangle_f32 src_rect, shader_fn shader, void* context) {
#ifndef MULTITHREADED_EXPERIMENTAL
    for (s32 y = src_rect.y; y < src_rect.y+src_rect.h; ++y) {
        for (s32 x = src_rect.x; x < src_rect.x+src_rect.w; ++x) {
            union color32f32 source_pixel = color32f32(
                framebuffer->pixels[y * framebuffer->width * 4 + x * 4 + 0] / 255.0f,
                framebuffer->pixels[y * framebuffer->width * 4 + x * 4 + 1] / 255.0f,
                framebuffer->pixels[y * framebuffer->width * 4 + x * 4 + 2] / 255.0f,
                framebuffer->pixels[y * framebuffer->width * 4 + x * 4 + 3] / 255.0f
            );

            union color32f32 new_pixel = shader(framebuffer, source_pixel, v2f32(x, y), context);

            new_pixel.r = clamp_f32(new_pixel.r, 0, 1);
            new_pixel.g = clamp_f32(new_pixel.g, 0, 1);
            new_pixel.b = clamp_f32(new_pixel.b, 0, 1);
            new_pixel.a = clamp_f32(new_pixel.a, 0, 1);

            framebuffer->pixels[y * framebuffer->width * 4 + x * 4 + 0] = new_pixel.r * 255.0f;
            framebuffer->pixels[y * framebuffer->width * 4 + x * 4 + 1] = new_pixel.g * 255.0f;
            framebuffer->pixels[y * framebuffer->width * 4 + x * 4 + 2] = new_pixel.b * 255.0f;
            framebuffer->pixels[y * framebuffer->width * 4 + x * 4 + 3] = new_pixel.a * 255.0f;
        }
    }
#else
    s32 JOBS_W      = 4;
    s32 JOBS_H      = 4;
    s32 CLIP_W      = (framebuffer->width)/JOBS_W;
    s32 CLIP_H      = (framebuffer->height)/JOBS_H;
    s32 REMAINDER_W = (framebuffer->width) % JOBS_W;
    s32 REMAINDER_H = (framebuffer->height) % JOBS_H;

    struct run_shader_job_details_shared shared_buffer =  (struct run_shader_job_details_shared) {
        .framebuffer = framebuffer,
        .context     = context,
        .shader      = shader
    };

    struct run_shader_job_details* job_buffers = memory_arena_push(&scratch_arena, sizeof(*job_buffers) * (JOBS_W*JOBS_H));

    for (s32 y = 0; y < JOBS_H; ++y) {
        for (s32 x = 0; x < JOBS_W; ++x) {
            struct rectangle_f32            clip_rect      = (struct rectangle_f32){x * CLIP_W, y * CLIP_H, CLIP_W, CLIP_H};
            struct run_shader_job_details*  current_buffer = &job_buffers[y*JOBS_W+x];

            if (x == JOBS_W-1) {
                clip_rect.w += REMAINDER_W;
            }

            if (y == JOBS_H-1) {
                clip_rect.h += REMAINDER_H;
            }

            {
                current_buffer->shared                     = &shared_buffer;
                current_buffer->src_rect                  = clip_rect;
            }

            thread_pool_add_job(thread_software_framebuffer_run_shader, current_buffer);
        }
    }

    thread_pool_synchronize_tasks();
#endif
}

#undef _BlendPixel_Scalar

struct graphics_assets graphics_assets_create(struct memory_arena* arena, u32 font_limit, u32 image_limit) {
    struct graphics_assets assets = {
        .font_capacity  = font_limit,
        .image_capacity = image_limit,
    };

    assets.arena              = arena;
    assets.images             = memory_arena_push(arena, sizeof(*assets.images) * image_limit);
    assets.image_file_strings = memory_arena_push(arena, sizeof(*assets.image_file_strings) *image_limit);
    assets.fonts              = memory_arena_push(arena, sizeof(*assets.fonts)  * font_limit);

    return assets;
}

void graphics_assets_finish(struct graphics_assets* assets) {
    for (unsigned image_index = 0; image_index < assets->image_count; ++image_index) {
        struct image_buffer* image = assets->images + image_index;
        _debugprintf("destroying img: %p (%d) (%dx%d %p)", image, image_index, image->width, image->height, image->pixels);
        image_buffer_free(image);
    }
    for (unsigned font_index = 0; font_index < assets->font_count; ++font_index) {
        struct font_cache* font = assets->fonts + font_index;
        _debugprintf("destroying font: %p (%d)", font, font_index);
        font_cache_free(font);
    }
}

image_id graphics_assets_load_image(struct graphics_assets* assets, string path) {
    for (s32 index = 0; index < assets->image_count; ++index) {
        string filepath = assets->image_file_strings[index];

        if (string_equal(path, filepath)) {
            return (image_id){.index = index+1};
        }
    }

    image_id new_id = (image_id) { .index = assets->image_count + 1 };
    _debugprintf("img loaded: %.*s", path.length, path.data);

    struct image_buffer* new_image           = &assets->images[assets->image_count];
    string*              new_filepath_string = &assets->image_file_strings[assets->image_count++];
    *new_filepath_string                     = memory_arena_push_string(assets->arena, path);
    *new_image                               = image_buffer_load_from_file(*new_filepath_string);

    return new_id;
}

font_id graphics_assets_load_bitmap_font(struct graphics_assets* assets, string path, s32 tile_width, s32 tile_height, s32 atlas_rows, s32 atlas_columns) {
    font_id new_id = (font_id) { .index = assets->font_count + 1 };

    struct font_cache* new_font   = &assets->fonts[assets->font_count++];
    *new_font                     = font_cache_load_bitmap_font(path, tile_width, tile_height, atlas_rows, atlas_columns);

    return new_id;
}

struct font_cache* graphics_assets_get_font_by_id(struct graphics_assets* assets, font_id font) {
    assertion(font.index > 0 && font.index <= assets->font_count);
    return &assets->fonts[font.index-1];
}

struct image_buffer* graphics_assets_get_image_by_id(struct graphics_assets* assets, image_id image) {
    assertion(image.index > 0 && image.index <= assets->image_count);
    return &assets->images[image.index-1];
}

image_id graphics_assets_get_image_by_filepath(struct graphics_assets* assets, string filepath) {
    assertion(filepath.data && filepath.length && "Bad string?");
    return graphics_assets_load_image(assets, filepath);
}

struct lightmask_buffer lightmask_buffer_create(u32 buffer_width, u32 buffer_height) {
    struct lightmask_buffer result;
    result.width       = buffer_width;
    result.height      = buffer_height;
    result.mask_buffer = system_heap_memory_allocate(buffer_width * buffer_height);
    lightmask_buffer_clear(&result);
    return result;
}

void lightmask_buffer_clear(struct lightmask_buffer* buffer) {
    zero_memory(buffer->mask_buffer, buffer->width*buffer->height);
}

static inline u8 lightmask_buffer_get_pixel(struct lightmask_buffer* buffer, s32 x, s32 y) {
    u32 index = y * buffer->width + x;
    return buffer->mask_buffer[index];
}

static inline void lightmask_buffer_put_pixel(struct lightmask_buffer* buffer, s32 x, s32 y, u8 value, u8 blend_mode) {
    u32 index = y * buffer->width + x;

    switch (blend_mode) {
        case LIGHTMASK_BLEND_NONE: {
            buffer->mask_buffer[index] = value;
        } break;
        case LIGHTMASK_BLEND_OR: {
            if (lightmask_buffer_get_pixel(buffer, x, y) == 255) {
                // ...
            } else {
                buffer->mask_buffer[index] = value;
            }
        } break;
    }
}

void lightmask_buffer_blit_image_clipped(struct lightmask_buffer* buffer, struct rectangle_f32 clip_rect, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 src, u8 flags, u8 blend_mode, u8 v) {
    if ((destination.x == 0) && (destination.y == 0) && (destination.w == 0) && (destination.h == 0)) {
        destination.w = buffer->width;
        destination.h = buffer->height;
    }

    if (!rectangle_f32_intersect(destination, clip_rect)) {
        return;
    }

    if ((src.x == 0) && (src.y == 0) && (src.w == 0) && (src.h == 0)) {
        src.w = image->width;
        src.h = image->height;
    }

    f32 scale_ratio_w = (f32)src.w  / destination.w;
    f32 scale_ratio_h = (f32)src.h  / destination.h;

    s32 start_x = clamp_s32((s32)destination.x, clip_rect.x, clip_rect.x+clip_rect.w);
    s32 start_y = clamp_s32((s32)destination.y, clip_rect.y, clip_rect.y+clip_rect.h);
    s32 end_x   = clamp_s32((s32)(destination.x + destination.w), clip_rect.x, clip_rect.x+clip_rect.w);
    s32 end_y   = clamp_s32((s32)(destination.y + destination.h), clip_rect.y, clip_rect.y+clip_rect.h);

    s32 unclamped_end_x = (s32)(destination.x + destination.w);
    s32 unclamped_end_y = (s32)(destination.y + destination.h);

    s32 stride       = buffer->width;
    s32 image_stride = image->width;

    for (s32 y_cursor = start_y; y_cursor < end_y; ++y_cursor) {
        for (s32 x_cursor = start_x; x_cursor < end_x; ++x_cursor) {
            s32 image_sample_x = (s32)((src.x + src.w) - ((unclamped_end_x - x_cursor) * scale_ratio_w));
            s32 image_sample_y = (s32)((src.y + src.h) - ((unclamped_end_y - y_cursor) * scale_ratio_h));

            if ((flags & SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_HORIZONTALLY))
                image_sample_x = (s32)(((unclamped_end_x - x_cursor) * scale_ratio_w) + src.x);

            if ((flags & SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_VERTICALLY))
                image_sample_y = (s32)(((unclamped_end_y - y_cursor) * scale_ratio_h) + src.y);

            union color32f32 sampled_pixel = color32f32(image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 0] / 255.0f,
                                                        image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 1] / 255.0f,
                                                        image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 2] / 255.0f,
                                                        image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 3] / 255.0f);

            if (sampled_pixel.a == 1.0) {
                lightmask_buffer_put_pixel(buffer, x_cursor, y_cursor, v, blend_mode);
            }
        }
    }
}

void lightmask_buffer_blit_rectangle_clipped(struct lightmask_buffer* buffer, struct rectangle_f32 clip_rect, struct rectangle_f32 destination, u8 blend_mode, u8 v) {
    s32 start_x = clamp_s32((s32)destination.x, clip_rect.x, clip_rect.x+clip_rect.w);
    s32 start_y = clamp_s32((s32)destination.y, clip_rect.y, clip_rect.y+clip_rect.h);
    s32 end_x   = clamp_s32((s32)(destination.x + destination.w), clip_rect.x, clip_rect.x+clip_rect.w);
    s32 end_y   = clamp_s32((s32)(destination.y + destination.h), clip_rect.y, clip_rect.y+clip_rect.h);

    for (s32 y_cursor = start_y; y_cursor < end_y; ++y_cursor) {
        for (s32 x_cursor = start_x; x_cursor < end_x; ++x_cursor) {
            lightmask_buffer_put_pixel(buffer, x_cursor, y_cursor, v, blend_mode);
        }
    }
}

void lightmask_buffer_blit_image(struct lightmask_buffer* buffer, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 src, u8 flags, u8 blend_mode, u8 v) {
    lightmask_buffer_blit_image_clipped(buffer, rectangle_f32(0, 0, buffer->width, buffer->height), image, destination, src, flags, blend_mode, v);
}

void lightmask_buffer_blit_rectangle(struct lightmask_buffer* buffer, struct rectangle_f32 destination, u8 blend_mode, u8 v) {
    lightmask_buffer_blit_rectangle_clipped(buffer, rectangle_f32(0, 0, buffer->width, buffer->height), destination, blend_mode, v);
}

f32 lightmask_buffer_lit_percent(struct lightmask_buffer* buffer, s32 x, s32 y) {
    u32 index = y * buffer->width + x;
    u8 value = buffer->mask_buffer[index];

    return (value) / 255.0f;
}

bool lightmask_buffer_is_lit(struct lightmask_buffer* buffer, s32 x, s32 y) {
    u32 index = y * buffer->width + x;

    if (buffer->mask_buffer[index] != 0) {
        return 1;
    }

    return 0;
}

void lightmask_buffer_finish(struct lightmask_buffer* buffer) {
    if (buffer->mask_buffer) {
        system_heap_memory_deallocate(buffer->mask_buffer);
        buffer->mask_buffer = NULL;

        buffer->width  = 0;
        buffer->height = 0;
    }
}
