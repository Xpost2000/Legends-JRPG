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

    u8* image_buffer = stbi_load(filepath.data, &width, &height, &components, 4);
    struct image_buffer result = (struct image_buffer) {
        .pixels = image_buffer,
        .width  = width,
        .height = height,
    };
    return result;
}

void image_buffer_write_to_disk(struct image_buffer* image, string as) {
    char filename[256] = {};
    snprintf(filename, 256, "%s.bmp", as.data);
    stbi_write_bmp(filename, image->width, image->height, 4, image->pixels);
    _debugprintf("screenshot produced.");
}

void image_buffer_free(struct image_buffer* image) {
    assertion(image->pixels);
    system_heap_memory_deallocate(image->pixels);
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

f32 font_cache_text_width(struct font_cache* font_cache, string text) {
    /* NOTE font_caches are monospaced */
    return font_cache->tile_width * text.length;
}

/* we would like temporary arenas yes... */
struct software_framebuffer software_framebuffer_create(struct memory_arena* arena, u32 width, u32 height) {
    u8* pixels = memory_arena_push(arena, width * height * sizeof(u32));
    /* u8* pixels = system_heap_memory_allocate(width*height*sizeof(u32)); */

    return (struct software_framebuffer) {
        .width  = width,
        .height = height,
        .pixels = pixels,
    };
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
                bad_case;                                               \
        }                                                               \
    } while(0)

void software_framebuffer_draw_quad(struct software_framebuffer* framebuffer, struct rectangle_f32 destination, union color32u8 rgba, u8 blend_mode) {
    s32 start_x = clamp_s32((s32)destination.x, 0, framebuffer->width);
    s32 start_y = clamp_s32((s32)destination.y, 0, framebuffer->height);
    s32 end_x   = clamp_s32((s32)(destination.x + destination.w), 0, framebuffer->width);
    s32 end_y   = clamp_s32((s32)(destination.y + destination.h), 0, framebuffer->height);

    u32* framebuffer_pixels_as_32 = (u32*)framebuffer->pixels;
    unused(framebuffer_pixels_as_32);

    for (s32 y_cursor = start_y; y_cursor < end_y; ++y_cursor) {
        for (s32 x_cursor = start_x; x_cursor < end_x; ++x_cursor) {
            _BlendPixel_Scalar(framebuffer, x_cursor, y_cursor, rgba, blend_mode);
        }
    }
}

#ifndef USE_SIMD_OPTIMIZATIONS
void software_framebuffer_draw_image_ex(struct software_framebuffer* framebuffer, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 src, union color32f32 modulation, u32 flags, u8 blend_mode) {
    if ((destination.x == 0) && (destination.y == 0) && (destination.w == 0) && (destination.h == 0)) {
        destination.w = framebuffer->width;
        destination.h = framebuffer->height;
    }

    if ((src.x == 0) && (src.y == 0) && (src.w == 0) && (src.h == 0)) {
        src.w = image->width;
        src.h = image->height;
    }

    f32 scale_ratio_w = (f32)src.w  / destination.w;
    f32 scale_ratio_h = (f32)src.h  / destination.h;

    s32 start_x = clamp_s32((s32)destination.x, 0, framebuffer->width);
    s32 start_y = clamp_s32((s32)destination.y, 0, framebuffer->height);
    s32 end_x   = clamp_s32((s32)(destination.x + destination.w), 0, framebuffer->width);
    s32 end_y   = clamp_s32((s32)(destination.y + destination.h), 0, framebuffer->height);

    s32 unclamped_end_x = (s32)(destination.x + destination.w);
    s32 unclamped_end_y = (s32)(destination.y + destination.h);

    u32* framebuffer_pixels_as_32 = (u32*)framebuffer->pixels;
    unused(framebuffer_pixels_as_32);

    s32 stride       = framebuffer->width;
    s32 image_stride = image->width;

    for (s32 y_cursor = start_y; y_cursor < end_y; ++y_cursor) {
        for (s32 x_cursor = start_x; x_cursor < end_x; ++x_cursor) {
            s32 image_sample_x = (s32)((src.x + src.w) - ((unclamped_end_x - x_cursor) * scale_ratio_w));
            s32 image_sample_y = (s32)((src.y + src.h) - ((unclamped_end_y - y_cursor) * scale_ratio_h));

            if ((flags & SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_HORIZONTALLY))
                image_sample_x = (s32)(((unclamped_end_x - x_cursor) * scale_ratio_w) + src.x);

            if ((flags & SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_VERTICALLY))
                image_sample_y = (s32)(((unclamped_end_y - y_cursor) * scale_ratio_h) + src.y);

            union color32f32 sampled_pixel = color32f32(image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 0],
                                                        image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 1],
                                                        image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 2],
                                                        image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 3]);

            sampled_pixel.r *= modulation.r;
            sampled_pixel.g *= modulation.g;
            sampled_pixel.b *= modulation.b;
            sampled_pixel.a *= modulation.a;

            _BlendPixel_Scalar(framebuffer, x_cursor, y_cursor, sampled_pixel, blend_mode);
        }
    }
}
#else
void software_framebuffer_draw_image_ex(struct software_framebuffer* framebuffer, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 src, union color32f32 modulation, u32 flags, u8 blend_mode) {
    if ((destination.x == 0) && (destination.y == 0) && (destination.w == 0) && (destination.h == 0)) {
        destination.w = framebuffer->width;
        destination.h = framebuffer->height;
    }

    if ((src.x == 0) && (src.y == 0) && (src.w == 0) && (src.h == 0)) {
        src.w = image->width;
        src.h = image->height;
    }

    f32 scale_ratio_w = (f32)src.w / destination.w;
    f32 scale_ratio_h = (f32)src.h / destination.h;

    s32 start_x = clamp_s32((s32)destination.x, 0, framebuffer->width);
    s32 start_y = clamp_s32((s32)destination.y, 0, framebuffer->height);
    s32 end_x   = clamp_s32((s32)(destination.x + destination.w), 0, framebuffer->width);
    s32 end_y   = clamp_s32((s32)(destination.y + destination.h), 0, framebuffer->height);

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

    __m128 inverse_255  = _mm_set1_ps(1.0 / 255.0f);

    for (s32 y_cursor = start_y; y_cursor < end_y; ++y_cursor) {
        for (s32 x_cursor = start_x; x_cursor < end_x; x_cursor += 4) {
            s32 image_sample_y = ((src.y + src.h) - ((unclamped_end_y - y_cursor) * scale_ratio_h));

            s32 image_sample_x  = ((src.x + src.w) - ((unclamped_end_x - (x_cursor))   * scale_ratio_w));
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

            __m128 red_channels = _mm_set_ps(
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x3 * 4 + 0],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x2 * 4 + 0],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x1 * 4 + 0],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x  * 4 + 0]
            );
            __m128 green_channels = _mm_set_ps(
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x3 * 4 + 1],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x2 * 4 + 1],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x1 * 4 + 1],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x  * 4 + 1]
            );
            __m128 blue_channels = _mm_set_ps(
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x3 * 4 + 2],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x2 * 4 + 2],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x1 * 4 + 2],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x  * 4 + 2]
            );
            __m128 alpha_channels  = _mm_set_ps(
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x3 * 4 + 3],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x2 * 4 + 3],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x1 * 4 + 3],
                image->pixels[image_sample_y * image_stride * 4 + image_sample_x  * 4 + 3]
            );

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
                    red_destination_channels   = _mm_add_ps(red_destination_channels,   _mm_mul_ps(red_channels, alpha_channels));
                    green_destination_channels = _mm_add_ps(green_destination_channels, _mm_mul_ps(green_channels, alpha_channels));
                    blue_destination_channels  = _mm_add_ps(blue_destination_channels,  _mm_mul_ps(blue_channels,  alpha_channels));
                } break;
                    bad_case;
            }

#define castF32_M128(X) ((f32*)(&X))
            for (int i = 0; i < 4; ++i) {
                if ((x_cursor + i >= framebuffer->width) ||
                    (((src.x + src.w) - ((unclamped_end_x - (x_cursor+i)) * scale_ratio_w))) >= src.x+src.w)
                    break;

                framebuffer->pixels_u32[y_cursor * framebuffer->width + (x_cursor+i)] = packu32(
                    255,
                    clamp_f32(castF32_M128(blue_destination_channels)[i],  0, 255),
                    clamp_f32(castF32_M128(green_destination_channels)[i], 0, 255),
                    clamp_f32(castF32_M128(red_destination_channels)[i],   0, 255)
                );
            }
#undef castF32_M128
        }
    }
}
#endif


void software_framebuffer_draw_line(struct software_framebuffer* framebuffer, v2f32 start, v2f32 end, union color32u8 rgba, u8 blend_mode) {
    u32 stride = framebuffer->width;

    if (start.y == end.y) {
        if (start.x > end.x) {
            Swap(start.x, end.x, f32);
        }

        for (s32 x_cursor = start.x; x_cursor < end.x; x_cursor++) {
            if (x_cursor < framebuffer->width && x_cursor >= 0 &&
                start.y  < framebuffer->height && start.y >= 0) {
                _BlendPixel_Scalar(framebuffer, x_cursor, (s32)floor(start.y), rgba, blend_mode);
            }
        }
    } else if (start.x == end.x) {
        if (start.y > end.y) {
            Swap(start.y, end.y, f32);
        }
        
        for (s32 y_cursor = start.y; y_cursor < end.y; y_cursor++) {
            if (start.x < framebuffer->width && start.x >= 0 &&
                y_cursor  < framebuffer->height && y_cursor >= 0) {
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
            if (x1 < framebuffer->width   && x1 >= 0 &&
                y1  < framebuffer->height && y1 >= 0) {
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

/* we do not have a draw glyph */
void software_framebuffer_draw_text(struct software_framebuffer* framebuffer, struct font_cache* font, float scale, v2f32 xy, string text, union color32f32 modulation, u8 blend_mode) {
    f32 x_cursor = xy.x;
    f32 y_cursor = xy.y;

    for (unsigned index = 0; index < text.length; ++index) {
        if (text.data[index] == '\n') {
            y_cursor += font->tile_height * scale;
            x_cursor =  xy.x;
        } else {
            s32 character_index = text.data[index] - 32;

            software_framebuffer_draw_image_ex(
                framebuffer, font,
                rectangle_f32(
                    x_cursor, y_cursor,
                    font->tile_width * scale,
                    font->tile_height * scale
                ),
                rectangle_f32(
                    (character_index % font->atlas_cols) * font->tile_width,
                    (character_index / font->atlas_cols) * font->tile_height,
                    font->tile_width, font->tile_height
                ),
                modulation,
                NO_FLAGS,
                blend_mode
            );

            x_cursor += font->tile_width * scale;
        }
    }
}

void software_framebuffer_copy_into(struct software_framebuffer* target, struct software_framebuffer* source) {
    if (target->width == source->width && target->height == source->height) {
        memory_copy(source->pixels, target->pixels, target->width * target->height * sizeof(u32));
    } else {
        software_framebuffer_draw_image_ex(target, source, RECTANGLE_F32_NULL, RECTANGLE_F32_NULL, color32f32(1,1,1,1), NO_FLAGS, 0);
    }
}

struct render_commands render_commands(struct camera camera) {
    return (struct render_commands) {
        .camera = camera 
    };
}

struct render_command* render_commands_new_command(struct render_commands* commands, s16 type) {
    struct render_command* command = &commands->commands[commands->command_count++];
    command->type = type;
    return command;
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
    command->destination = destination;
    command->image       = image;
    command->source      = source;
    command->flags       = flags;
    command->modulation  = rgba;
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
    command->font       = font;
    command->xy         = xy;
    command->scale      = scale;
    command->text       = text;
    command->modulation = rgba;
    command->flags      = 0;
    command->blend_mode    = blend_mode;
}

void render_commands_clear(struct render_commands* commands) {
    commands->command_count = 0;
}

void sort_render_commands(struct render_commands* commands) {
    /* TODO */
}

void software_framebuffer_render_commands(struct software_framebuffer* framebuffer, struct render_commands* commands) {
    if (commands->should_clear_buffer) {
        software_framebuffer_clear_buffer(framebuffer, commands->clear_buffer_color);
    }

    sort_render_commands(commands);

    f32 half_screen_width  = framebuffer->width/2;
    f32 half_screen_height = framebuffer->height/2;

    for (unsigned index = 0; index < commands->command_count; ++index) {
        struct render_command* command = &commands->commands[index];

        {
            command->start.x       *= commands->camera.zoom;
            command->start.y       *= commands->camera.zoom;
            command->end.x         *= commands->camera.zoom;
            command->end.y         *= commands->camera.zoom;
            command->destination.x *= commands->camera.zoom;
            command->destination.y *= commands->camera.zoom;
            command->destination.w *= commands->camera.zoom;
            command->destination.h *= commands->camera.zoom;
            command->xy.x          *= commands->camera.zoom;
            command->xy.y          *= commands->camera.zoom;
            command->scale         *= commands->camera.zoom;
        }

        if (commands->camera.centered) {
            command->start.x       += half_screen_width;
            command->start.y       += half_screen_height;
            command->end.x         += half_screen_width;
            command->end.y         += half_screen_height;
            command->destination.x += half_screen_width;
            command->destination.y += half_screen_height;
            command->xy.x          += half_screen_width;
            command->xy.y          += half_screen_height;
        }

        {
            command->start.x       -= commands->camera.xy.x;
            command->start.y       -= commands->camera.xy.y;
            command->end.x         -= commands->camera.xy.x;
            command->end.y         -= commands->camera.xy.y;
            command->destination.x -= commands->camera.xy.x;
            command->destination.y -= commands->camera.xy.y;
            command->xy.x          -= commands->camera.xy.x;
            command->xy.y          -= commands->camera.xy.y;
        }


        switch (command->type) {
            case RENDER_COMMAND_DRAW_QUAD: {
                software_framebuffer_draw_quad(
                    framebuffer,
                    command->destination,
                    command->modulation_u8,
                    command->blend_mode
                );
            } break;
            case RENDER_COMMAND_DRAW_IMAGE: {
                software_framebuffer_draw_image_ex(
                    framebuffer,
                    command->image,
                    command->destination,
                    command->source,
                    command->modulation,
                    command->flags,
                    command->blend_mode
                );
            } break;
            case RENDER_COMMAND_DRAW_TEXT: {
                software_framebuffer_draw_text(
                    framebuffer,
                    command->font,
                    command->scale,
                    command->xy,
                    command->text,
                    command->modulation,
                    command->blend_mode
                );
            } break;
            case RENDER_COMMAND_DRAW_LINE: {
                software_framebuffer_draw_line(
                    framebuffer,
                    command->start,
                    command->end,
                    command->modulation_u8,
                    command->blend_mode
                );
            } break;
        }
    }
}

/* requires an arena because we need an original copy of our framebuffer. */
/* NOTE technically a test of performance to see if this is doomed */

/*
  As expected this is the single most expensive operation.
  
  I don't want to have to force so many copies with this when I multithread though... It might be safe to always store a double buffer of the framebuffer
  itself? Will think about later.
  
  Well, I could SIMD this for a much easier performance fix...
  Then threading this would just be figuring out how to split this into multiple tiles/clusters.
*/
void software_framebuffer_kernel_convolution_ex(struct memory_arena* arena, struct software_framebuffer* framebuffer, f32* kernel, s16 width, s16 height, f32 divisor, f32 blend_t, s32 passes) {
    struct software_framebuffer unaltered_copy = software_framebuffer_create(arena, framebuffer->width, framebuffer->height);
    if (divisor == 0.0) divisor = 1;

    for (s32 pass = 0; pass < passes; pass++) {
        software_framebuffer_copy_into(&unaltered_copy, framebuffer);

        s32 framebuffer_width  = framebuffer->width;
        s32 framebuffer_height = framebuffer->height;

        s32 kernel_half_width = width/2;
        s32 kernel_half_height = height/2;

        for (s32 y_cursor = 0; y_cursor < framebuffer_height; ++y_cursor) {
            for (s32 x_cursor = 0; x_cursor < framebuffer_width; ++x_cursor) {
                f32 accumulation[3] = {};

                for (s32 y_cursor_kernel = -kernel_half_height; y_cursor_kernel <= kernel_half_height; ++y_cursor_kernel) {
                    for (s32 x_cursor_kernel = -kernel_half_width; x_cursor_kernel <= kernel_half_width; ++x_cursor_kernel) {
                        s32 sample_x = x_cursor_kernel + x_cursor;
                        s32 sample_y = y_cursor_kernel + y_cursor;

                        if (sample_x >= 0 && sample_x < framebuffer_width &&
                            sample_y >= 0 && sample_y < framebuffer_height) {

                            accumulation[0] += unaltered_copy.pixels[sample_y * framebuffer_width * 4 + sample_x * 4 + 0] * kernel[(y_cursor_kernel+1) * width + (x_cursor_kernel+1)];
                            accumulation[1] += unaltered_copy.pixels[sample_y * framebuffer_width * 4 + sample_x * 4 + 1] * kernel[(y_cursor_kernel+1) * width + (x_cursor_kernel+1)];
                            accumulation[2] += unaltered_copy.pixels[sample_y * framebuffer_width * 4 + sample_x * 4 + 2] * kernel[(y_cursor_kernel+1) * width + (x_cursor_kernel+1)];
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
    for (unsigned font_index = 0; font_index < assets->font_count; ++font_index) {
        struct font_cache* font = assets->fonts + font_index;
        font_cache_free(font);
    }

    for (unsigned image_index = 0; image_index < assets->image_count; ++image_index) {
        struct image_buffer* image = assets->images + image_index;
        image_buffer_free(image);
    }
}

/* TODO */
/* TODO: does not check for duplicates, since I don't hash yet */
image_id graphics_assets_load_image(struct graphics_assets* assets, string path) {
    image_id new_id = (image_id) { .index = assets->image_count + 1 };

    struct image_buffer* new_image = &assets->images[assets->image_count];
    string*              new_filepath_string = &assets->image_file_strings[assets->image_count++];
    *new_image                     = image_buffer_load_from_file(path);
    *new_filepath_string           = memory_arena_push_string(assets->arena, path);

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

    for (s32 index = 0; index < assets->image_count; ++index) {
        string current_filepath = assets->image_file_strings[index];

        if (string_equal(filepath, current_filepath)) {
            return (image_id) { .index = index + 1 };
        }
    }

    return (image_id) { .index = 0 };
}
