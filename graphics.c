/* 
   TODO:
   
   Resources system? Probably not strictly necessary.
   
   CLEANUP:
   Allow for more fine grained blending modes. We default to alpha blending always.

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

void software_framebuffer_draw_quad(struct software_framebuffer* framebuffer, struct rectangle_f32 destination, union color32u8 rgba) {
    s32 start_x = (s32)destination.x;
    s32 start_y = (s32)destination.y;
    s32 end_x   = (s32)(destination.x + destination.w);
    s32 end_y   = (s32)(destination.y + destination.h);


    u32* framebuffer_pixels_as_32 = (u32*)framebuffer->pixels;
    unused(framebuffer_pixels_as_32);
    for (s32 y_cursor = start_y; y_cursor < end_y; ++y_cursor) {
        if (y_cursor >= 0 && y_cursor < framebuffer->height)
            for (s32 x_cursor = start_x; x_cursor < end_x; ++x_cursor) {
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

int std = 0;
/* #define SIMD_TEST */
#ifndef SIMD_TEST
void software_framebuffer_draw_image_ex(struct software_framebuffer* framebuffer, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 src, union color32f32 modulation, u32 flags) {
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

            /* if ((flags & SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_HORIZONTALLY)) */
            /*     image_sample_x = (s32)(((unclamped_end_x - x_cursor) * scale_ratio_w) + src.x); */

            /* if ((flags & SOFTWARE_FRAMEBUFFER_DRAW_IMAGE_FLIP_VERTICALLY)) */
            /*     image_sample_y = (s32)(((unclamped_end_y - y_cursor) * scale_ratio_h) + src.y); */

            union color32f32 sampled_pixel = color32f32(image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 0],
                                                        image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 1],
                                                        image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 2],
                                                        image->pixels[image_sample_y * image_stride * 4 + image_sample_x * 4 + 3]);

            sampled_pixel.r *= modulation.r;
            sampled_pixel.g *= modulation.g;
            sampled_pixel.b *= modulation.b;
            sampled_pixel.a *= modulation.a;

            f32 alpha = sampled_pixel.a / 255.0f;
            framebuffer->pixels_u32[y_cursor * framebuffer->width + x_cursor] = packu32(255,
                                                                                        (framebuffer->pixels[y_cursor * stride * 4 + x_cursor * 4 + 2] * (1 - alpha)) + (sampled_pixel.b * alpha),
                                                                                        (framebuffer->pixels[y_cursor * stride * 4 + x_cursor * 4 + 1] * (1 - alpha)) + (sampled_pixel.g * alpha),
                                                                                        (framebuffer->pixels[y_cursor * stride * 4 + x_cursor * 4 + 0] * (1 - alpha)) + (sampled_pixel.r * alpha));
        }
    }
}
#else
/* SIMD test */
void software_framebuffer_draw_image_ex(struct software_framebuffer* framebuffer, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 src, union color32f32 modulation, u32 flags) {
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

    f32 inverse_255_singular = 1/255.0f;
    __m128 inverse_255  = _mm_load1_ps(&inverse_255_singular);

    s32 src_end_x = src.x + src.w;
    s32 src_end_y = src.y + src.h;

    /* TODO flipping */
    for (s32 y_cursor = start_y; y_cursor < end_y; ++y_cursor) {
        for (s32 x_cursor = start_x; x_cursor <= end_x; x_cursor += 4) {
            /* s32 image_sample_y = (s32)(src_end_y - ((unclamped_end_y - y_cursor) * scale_ratio_h)); */
            s32 image_sample_y = (s32)((src.y + src.h) - ((unclamped_end_y - y_cursor) * scale_ratio_h));

            __m128 red_channels = _mm_set_ps(
                image->pixels[image_sample_y * image_stride * 4 + (s32)(src_end_x - (unclamped_end_x - (x_cursor+3)) * scale_ratio_w) * 4 + 0],
                image->pixels[image_sample_y * image_stride * 4 + (s32)(src_end_x - (unclamped_end_x - (x_cursor+2)) * scale_ratio_w) * 4 + 0],
                image->pixels[image_sample_y * image_stride * 4 + (s32)(src_end_x - (unclamped_end_x - (x_cursor+1)) * scale_ratio_w) * 4 + 0],
                image->pixels[image_sample_y * image_stride * 4 + (s32)(src_end_x - (unclamped_end_x - (x_cursor+0)) * scale_ratio_w) * 4 + 0]
            );
            __m128 green_channels = _mm_set_ps(
                image->pixels[image_sample_y * image_stride * 4 + (s32)(src_end_x - (unclamped_end_x - (x_cursor+3)) * scale_ratio_w) * 4 + 1],
                image->pixels[image_sample_y * image_stride * 4 + (s32)(src_end_x - (unclamped_end_x - (x_cursor+2)) * scale_ratio_w) * 4 + 1],
                image->pixels[image_sample_y * image_stride * 4 + (s32)(src_end_x - (unclamped_end_x - (x_cursor+1)) * scale_ratio_w) * 4 + 1],
                image->pixels[image_sample_y * image_stride * 4 + (s32)(src_end_x - (unclamped_end_x - (x_cursor+0)) * scale_ratio_w) * 4 + 1]
            );
            __m128 blue_channels = _mm_set_ps(
                image->pixels[image_sample_y * image_stride * 4 + (s32)(src_end_x - (unclamped_end_x - (x_cursor+3)) * scale_ratio_w) * 4 + 2],
                image->pixels[image_sample_y * image_stride * 4 + (s32)(src_end_x - (unclamped_end_x - (x_cursor+2)) * scale_ratio_w) * 4 + 2],
                image->pixels[image_sample_y * image_stride * 4 + (s32)(src_end_x - (unclamped_end_x - (x_cursor+1)) * scale_ratio_w) * 4 + 2],
                image->pixels[image_sample_y * image_stride * 4 + (s32)(src_end_x - (unclamped_end_x - (x_cursor+0)) * scale_ratio_w) * 4 + 2]
            );
            __m128 alpha_channels  = _mm_set_ps(
                image->pixels[image_sample_y * image_stride * 4 + (s32)(src_end_x - (unclamped_end_x - (x_cursor+3)) * scale_ratio_w) * 4 + 3],
                image->pixels[image_sample_y * image_stride * 4 + (s32)(src_end_x - (unclamped_end_x - (x_cursor+2)) * scale_ratio_w) * 4 + 3],
                image->pixels[image_sample_y * image_stride * 4 + (s32)(src_end_x - (unclamped_end_x - (x_cursor+1)) * scale_ratio_w) * 4 + 3],
                image->pixels[image_sample_y * image_stride * 4 + (s32)(src_end_x - (unclamped_end_x - (x_cursor+0)) * scale_ratio_w) * 4 + 3]
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

            __m128 one_minus_alpha     = _mm_sub_ps(_mm_set1_ps(1), alpha_channels);
            red_destination_channels   = _mm_add_ps(_mm_mul_ps(red_destination_channels, one_minus_alpha),   _mm_mul_ps(red_channels, alpha_channels));
            green_destination_channels = _mm_add_ps(_mm_mul_ps(green_destination_channels, one_minus_alpha), _mm_mul_ps(green_channels, alpha_channels));
            blue_destination_channels  = _mm_add_ps(_mm_mul_ps(blue_destination_channels, one_minus_alpha),  _mm_mul_ps(blue_channels, alpha_channels));

#define castF32_M128(X) ((f32*)(&X))
            for (int i = 0; i < 4; ++i) {
                framebuffer->pixels_u32[y_cursor * framebuffer->width + (x_cursor+i)] = packu32(255,
                                                                                                castF32_M128(blue_destination_channels)[i],
                                                                                                castF32_M128(green_destination_channels)[i],
                                                                                                castF32_M128(red_destination_channels)[i]
                );
                if (x_cursor + i >= framebuffer->width) break;
            }
#undef castF32_M128
        }
    }
}
#endif


void software_framebuffer_draw_line(struct software_framebuffer* framebuffer, v2f32 start, v2f32 end, union color32u8 rgba) {
    u32 stride = framebuffer->width;

    if (start.x < 0)                   start.x = 0;
    if (end.x < 0)                     end.x   = 0;
    if (start.x > framebuffer->width)  start.x = framebuffer->width-1;
    if (end.x > framebuffer->width)    end.x   = framebuffer->width-1;
    if (start.y < 0)                   start.y = 0;
    if (end.y < 0)                     end.y   = 0;
    if (end.y > framebuffer->height)   end.y   = framebuffer->height-1;
    if (start.y > framebuffer->height) start.y = framebuffer->height-1;

    if (start.y == end.y) {
        if (start.x > end.x) {
            Swap(start.x, end.x, f32);
        }

        for (s32 x_cursor = start.x; x_cursor < end.x; x_cursor++) {
#if 0
            framebuffer->pixels_u32[(s32)floor(start.y) * framebuffer->width + x_cursor] = rgba.rgba_packed;
#else
            {
                float alpha = rgba.a / 255.0f;
                framebuffer->pixels[(s32)(floor(start.y)) * stride * 4 + x_cursor * 4 + 0] = (framebuffer->pixels[(s32)(floor(start.y)) * stride * 4 + x_cursor * 4 + 0] * (1 - alpha)) + (rgba.r * alpha);
                framebuffer->pixels[(s32)(floor(start.y)) * stride * 4 + x_cursor * 4 + 1] = (framebuffer->pixels[(s32)(floor(start.y)) * stride * 4 + x_cursor * 4 + 1] * (1 - alpha)) + (rgba.g * alpha);
                framebuffer->pixels[(s32)(floor(start.y)) * stride * 4 + x_cursor * 4 + 2] = (framebuffer->pixels[(s32)(floor(start.y)) * stride * 4 + x_cursor * 4 + 2] * (1 - alpha)) + (rgba.b * alpha);
            }
            framebuffer->pixels[(s32)(floor(start.y)) * stride * 4 + x_cursor * 4 + 3] = 255;
#endif
        }
    } else if (start.x == end.x) {
        if (start.y > end.y) {
            Swap(start.y, end.y, f32);
        }
        
        for (s32 y_cursor = start.y; y_cursor < end.y; y_cursor++) {
#if 0
            framebuffer->pixels_u32[y_cursor * framebuffer->width + (s32)floor(start.x)] = rgba.rgba_packed;
#else
            {
                float alpha = rgba.a / 255.0f;
                framebuffer->pixels[y_cursor * stride * 4 + (s32)floor(start.x) * 4 + 0] = (framebuffer->pixels[y_cursor * stride * 4 + (s32)floor(start.x) * 4 + 0] * (1 - alpha)) + (rgba.r * alpha);
                framebuffer->pixels[y_cursor * stride * 4 + (s32)floor(start.x) * 4 + 1] = (framebuffer->pixels[y_cursor * stride * 4 + (s32)floor(start.x) * 4 + 1] * (1 - alpha)) + (rgba.g * alpha);
                framebuffer->pixels[y_cursor * stride * 4 + (s32)floor(start.x) * 4 + 2] = (framebuffer->pixels[y_cursor * stride * 4 + (s32)floor(start.x) * 4 + 2] * (1 - alpha)) + (rgba.b * alpha);
            }
            framebuffer->pixels[y_cursor * stride * 4 + (s32)floor(start.x) * 4 + 3] = 255;
#endif
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
#if 0
            framebuffer->pixels_u32[y1 * framebuffer->width + x1] = rgba.rgba_packed;
#else
            framebuffer->pixels_u32[y1 * framebuffer->width + x1] = packu32(255,
                                                                            (framebuffer->pixels[y1 * stride * 4 + x1 * 4 + 2] * (1 - alpha)) + (rgba.b * alpha),
                                                                            (framebuffer->pixels[y1 * stride * 4 + x1 * 4 + 1] * (1 - alpha)) + (rgba.g * alpha),
                                                                            (framebuffer->pixels[y1 * stride * 4 + x1 * 4 + 0] * (1 - alpha)) + (rgba.r * alpha));
#endif

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
void software_framebuffer_draw_text(struct software_framebuffer* framebuffer, struct font_cache* font, float scale, v2f32 xy, string text, union color32f32 modulation) {
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
                NO_FLAGS
            );

            x_cursor += font->tile_width * scale;
        }
    }
}

void software_framebuffer_copy_into(struct software_framebuffer* target, struct software_framebuffer* source) {
    if (target->width == source->width && target->height == source->height) {
        memory_copy(source->pixels, target->pixels, target->width * target->height * sizeof(u32));
    } else {
        software_framebuffer_draw_image_ex(target, source, RECTANGLE_F32_NULL, RECTANGLE_F32_NULL, color32f32(1,1,1,1), NO_FLAGS);
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

void render_commands_push_quad(struct render_commands* commands, struct rectangle_f32 destination, union color32u8 rgba) {
    struct render_command* command = render_commands_new_command(commands, RENDER_COMMAND_DRAW_QUAD);
    command->destination   = destination;
    command->flags         = 0;
    command->modulation_u8 = rgba;
}

void render_commands_push_image(struct render_commands* commands, struct image_buffer* image, struct rectangle_f32 destination, struct rectangle_f32 source, union color32f32 rgba, u32 flags ){
    struct render_command* command = render_commands_new_command(commands, RENDER_COMMAND_DRAW_IMAGE);
    command->destination = destination;
    command->image       = image;
    command->source      = source;
    command->flags       = flags;
    command->modulation  = rgba;
}

void render_commands_push_line(struct render_commands* commands, v2f32 start, v2f32 end, union color32u8 rgba) {
    struct render_command* command = render_commands_new_command(commands, RENDER_COMMAND_DRAW_LINE);
    command->start         = start;
    command->end           = end;
    command->flags         = 0;
    command->modulation_u8 = rgba;
}

void render_commands_push_text(struct render_commands* commands, struct font_cache* font, f32 scale, v2f32 xy, string text, union color32f32 rgba) {
    struct render_command* command = render_commands_new_command(commands, RENDER_COMMAND_DRAW_TEXT);
    command->font       = font;
    command->xy         = xy;
    command->scale      = scale;
    command->text       = text;
    command->modulation = rgba;
    command->flags      = 0;
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
                    command->modulation_u8
                );
            } break;
            case RENDER_COMMAND_DRAW_IMAGE: {
                software_framebuffer_draw_image_ex(
                    framebuffer,
                    command->image,
                    command->destination,
                    command->source,
                    command->modulation,
                    command->flags
                );
            } break;
            case RENDER_COMMAND_DRAW_TEXT: {
                software_framebuffer_draw_text(
                    framebuffer,
                    command->font,
                    command->scale,
                    command->xy,
                    command->text,
                    command->modulation
                );
            } break;
            case RENDER_COMMAND_DRAW_LINE: {
                software_framebuffer_draw_line(
                    framebuffer,
                    command->start,
                    command->end,
                    command->modulation_u8
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

                framebuffer->pixels[y_cursor * framebuffer_width * 4 + x_cursor * 4 + 0] = framebuffer->pixels[y_cursor * framebuffer_width * 4 + x_cursor * 4 + 0] * (1 - blend_t) + (blend_t * accumulation[0]);
                framebuffer->pixels[y_cursor * framebuffer_width * 4 + x_cursor * 4 + 1] = framebuffer->pixels[y_cursor * framebuffer_width * 4 + x_cursor * 4 + 1] * (1 - blend_t) + (blend_t * accumulation[1]);
                framebuffer->pixels[y_cursor * framebuffer_width * 4 + x_cursor * 4 + 2] = framebuffer->pixels[y_cursor * framebuffer_width * 4 + x_cursor * 4 + 2] * (1 - blend_t) + (blend_t * accumulation[2]);
            }
        }
    }
}
